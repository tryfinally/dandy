#include "server/session.hpp"
#include "server/game_engine.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>

namespace dandy::server {

Session::Session(int socket_fd, GameEngine* engine)
    : socket_fd_(socket_fd)
    , game_engine_(engine)
    , last_activity_(std::chrono::steady_clock::now())
{
    read_buffer_.resize(constants::RECV_BUFFER_SIZE);
}

Session::~Session() {
    stop();
}

void Session::start() {
    if (connected_.exchange(true)) return;

    read_thread_ = std::thread(&Session::read_loop, this);
    write_thread_ = std::thread(&Session::write_loop, this);
}

void Session::stop() {
    if (!connected_.exchange(false)) return;

    // Close socket
    if (socket_fd_ >= 0) {
        shutdown(socket_fd_, SHUT_RDWR);
        close(socket_fd_);
        socket_fd_ = -1;
    }

    // Wake up write thread
    {
        std::lock_guard lock(write_mutex_);
        write_cv_.notify_all();
    }

    if (read_thread_.joinable()) {
        read_thread_.join();
    }
    if (write_thread_.joinable()) {
        write_thread_.join();
    }

    // Notify game engine of disconnect
    if (authenticated_ && game_engine_) {
        game_engine_->remove_player(player_id_);
    }
}

void Session::send(const Message& msg) {
    if (!connected_.load()) return;

    auto data = frame_message(msg);

    {
        std::lock_guard lock(write_mutex_);
        write_queue_.push(std::move(data));
    }
    write_cv_.notify_one();
}

void Session::authenticate(PlayerId id, const std::string& name) {
    player_id_ = id;
    player_name_ = name;
    authenticated_ = true;
}

void Session::read_loop() {
    std::vector<uint8_t> buffer(constants::RECV_BUFFER_SIZE);
    std::vector<uint8_t> message_buffer;

    while (connected_.load()) {
        // Poll for readable data
        pollfd pfd{};
        pfd.fd = socket_fd_;
        pfd.events = POLLIN;

        int result = poll(&pfd, 1, 1000);  // 1 second timeout
        if (result < 0) {
            if (connected_.load()) {
                std::cerr << "Poll error for session " << player_id_ << std::endl;
            }
            break;
        }
        if (result == 0) continue;  // Timeout

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            break;
        }

        if (pfd.revents & POLLIN) {
            ssize_t bytes_read = recv(socket_fd_, buffer.data(), buffer.size(), 0);
            if (bytes_read <= 0) {
                break;
            }

            last_activity_ = std::chrono::steady_clock::now();

            // Append to message buffer
            message_buffer.insert(message_buffer.end(),
                                 buffer.begin(), buffer.begin() + bytes_read);

            // Try to parse complete messages
            while (message_buffer.size() >= HEADER_SIZE) {
                auto header_result = parse_header(message_buffer);
                if (!header_result) {
                    std::cerr << "Failed to parse header" << std::endl;
                    message_buffer.clear();
                    break;
                }

                auto& header = *header_result;
                size_t total_size = sizeof(uint32_t) + header.length;

                if (message_buffer.size() < total_size) {
                    break;  // Need more data
                }

                // Extract payload (skip length and type)
                std::span<const uint8_t> payload(
                    message_buffer.data() + HEADER_SIZE,
                    header.length - sizeof(uint16_t));

                auto msg_result = parse_message(header.type, payload);
                if (msg_result) {
                    process_message(std::move(*msg_result));
                } else {
                    std::cerr << "Failed to parse message type "
                              << static_cast<int>(header.type) << std::endl;
                }

                // Remove processed message from buffer
                message_buffer.erase(message_buffer.begin(),
                                    message_buffer.begin() + total_size);
            }
        }
    }

    connected_.store(false);
}

void Session::write_loop() {
    while (connected_.load()) {
        std::vector<uint8_t> data;

        {
            std::unique_lock lock(write_mutex_);
            write_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !write_queue_.empty() || !connected_.load();
            });

            if (!connected_.load()) break;
            if (write_queue_.empty()) continue;

            data = std::move(write_queue_.front());
            write_queue_.pop();
        }

        // Send data
        size_t total_sent = 0;
        while (total_sent < data.size() && connected_.load()) {
            ssize_t sent = ::send(socket_fd_, data.data() + total_sent,
                                 data.size() - total_sent, MSG_NOSIGNAL);
            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                connected_.store(false);
                break;
            }
            total_sent += sent;
        }
    }
}

void Session::process_message(std::unique_ptr<Message> msg) {
    if (!game_engine_) return;

    switch (msg->type) {
        case MessageType::Authenticate: {
            auto* auth = static_cast<AuthenticateMsg*>(msg.get());
            auto id = game_engine_->add_player(auth->name, shared_from_this());
            authenticate(id, auth->name);

            AuthResultMsg result(true, id, "Welcome " + auth->name);
            send(result);

            // Send game state
            GameStateMsg state(game_engine_->game_info());
            send(state);
            break;
        }

        case MessageType::JoinGame: {
            // Already in game, just send current state
            GameStateMsg state(game_engine_->game_info());
            send(state);
            break;
        }

        case MessageType::Ping: {
            auto* ping = static_cast<PingMsg*>(msg.get());
            PongMsg pong(ping->sequence, 0);
            send(pong);
            break;
        }

        case MessageType::Disconnect: {
            stop();
            break;
        }

        default:
            // Forward to game engine
            if (authenticated_) {
                game_engine_->handle_message(player_id_, std::move(msg));
            }
            break;
    }
}

} // namespace dandy::server
