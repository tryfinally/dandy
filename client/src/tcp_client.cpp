#include "client/tcp_client.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

namespace dandy::client {

TcpClient::TcpClient() = default;

TcpClient::~TcpClient() {
    disconnect();
}

bool TcpClient::connect(const std::string& host, uint16_t port) {
    // Resolve hostname
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    std::string port_str = std::to_string(port);

    int status = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
    if (status != 0) {
        std::cerr << "Failed to resolve host: " << gai_strerror(status) << std::endl;
        return false;
    }

    // Try each address
    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        socket_fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (socket_fd_ < 0) continue;

        if (::connect(socket_fd_, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  // Success
        }

        close(socket_fd_);
        socket_fd_ = -1;
    }

    freeaddrinfo(result);

    if (socket_fd_ < 0) {
        std::cerr << "Failed to connect to " << host << ":" << port << std::endl;
        return false;
    }

    connected_.store(true);

    // Start threads
    read_thread_ = std::thread(&TcpClient::read_loop, this);
    write_thread_ = std::thread(&TcpClient::write_loop, this);

    std::cout << "Connected to " << host << ":" << port << std::endl;
    return true;
}

void TcpClient::disconnect() {
    if (!connected_.exchange(false)) return;

    if (socket_fd_ >= 0) {
        shutdown(socket_fd_, SHUT_RDWR);
        close(socket_fd_);
        socket_fd_ = -1;
    }

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
}

void TcpClient::send(const Message& msg) {
    if (!connected_.load()) return;

    auto data = frame_message(msg);

    {
        std::lock_guard lock(write_mutex_);
        write_queue_.push(std::move(data));
    }
    write_cv_.notify_one();
}

void TcpClient::read_loop() {
    std::vector<uint8_t> buffer(constants::RECV_BUFFER_SIZE);
    std::vector<uint8_t> message_buffer;

    while (connected_.load()) {
        pollfd pfd{};
        pfd.fd = socket_fd_;
        pfd.events = POLLIN;

        int result = poll(&pfd, 1, 1000);
        if (result < 0) {
            if (connected_.load()) {
                std::cerr << "Poll error" << std::endl;
            }
            break;
        }
        if (result == 0) continue;

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            break;
        }

        if (pfd.revents & POLLIN) {
            ssize_t bytes_read = recv(socket_fd_, buffer.data(), buffer.size(), 0);
            if (bytes_read <= 0) {
                break;
            }

            message_buffer.insert(message_buffer.end(),
                                 buffer.begin(), buffer.begin() + bytes_read);

            while (message_buffer.size() >= HEADER_SIZE) {
                auto header_result = parse_header(message_buffer);
                if (!header_result) {
                    message_buffer.clear();
                    break;
                }

                auto& header = *header_result;
                size_t total_size = sizeof(uint32_t) + header.length;

                if (message_buffer.size() < total_size) {
                    break;
                }

                std::span<const uint8_t> payload(
                    message_buffer.data() + HEADER_SIZE,
                    header.length - sizeof(uint16_t));

                auto msg_result = parse_message(header.type, payload);
                if (msg_result) {
                    process_message(std::move(*msg_result));
                }

                message_buffer.erase(message_buffer.begin(),
                                    message_buffer.begin() + total_size);
            }
        }
    }

    connected_.store(false);
}

void TcpClient::write_loop() {
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

void TcpClient::process_message(std::unique_ptr<Message> msg) {
    // Handle welcome specially to extract server info
    if (msg->type == MessageType::Welcome) {
        auto* welcome = static_cast<WelcomeMsg*>(msg.get());
        server_version_ = welcome->version;
        tick_rate_ = welcome->tick_rate;
        max_tanks_ = welcome->max_tanks;
    }

    if (message_handler_) {
        message_handler_(std::move(msg));
    }
}

} // namespace dandy::client
