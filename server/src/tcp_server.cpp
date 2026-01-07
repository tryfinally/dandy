#include "server/tcp_server.hpp"
#include "server/session.hpp"
#include "server/game_engine.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

namespace dandy::server {

TcpServer::TcpServer(uint16_t port)
    : port_(port)
{
}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // Allow address reuse
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_fd_);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(server_fd_);
        return false;
    }

    if (listen(server_fd_, 10) < 0) {
        std::cerr << "Failed to listen" << std::endl;
        close(server_fd_);
        return false;
    }

    running_.store(true);

    // Start accept thread
    accept_thread_ = std::thread(&TcpServer::accept_loop, this);

    // Start cleanup thread
    cleanup_thread_ = std::thread(&TcpServer::cleanup_sessions, this);

    std::cout << "Server listening on port " << port_ << std::endl;
    return true;
}

void TcpServer::stop() {
    if (!running_.exchange(false)) return;

    // Close server socket to unblock accept
    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }

    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }

    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }

    // Close all sessions
    {
        std::lock_guard lock(sessions_mutex_);
        for (auto& [id, session] : sessions_) {
            session->stop();
        }
        sessions_.clear();
        for (auto& session : pending_sessions_) {
            session->stop();
        }
        pending_sessions_.clear();
    }
}

void TcpServer::accept_loop() {
    while (running_.load()) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        if (client_fd < 0) {
            if (running_.load()) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr)
                  << ":" << ntohs(client_addr.sin_port) << std::endl;

        // Create session
        auto session = std::make_shared<Session>(client_fd, game_engine_);

        {
            std::lock_guard lock(sessions_mutex_);
            pending_sessions_.push_back(session);
        }

        session->start();

        // Send welcome message
        WelcomeMsg welcome(constants::PROTOCOL_VERSION, constants::TICK_RATE,
                          constants::MAX_TANKS_PER_PLAYER);
        session->send(welcome);
    }
}

void TcpServer::cleanup_sessions() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::lock_guard lock(sessions_mutex_);

        // Remove disconnected sessions
        std::erase_if(sessions_, [](const auto& pair) {
            return !pair.second->is_connected();
        });

        // Check pending sessions for timeout
        auto now = std::chrono::steady_clock::now();
        std::erase_if(pending_sessions_, [&](const auto& session) {
            if (!session->is_connected()) return true;
            auto idle = now - session->last_activity();
            return idle > constants::CONNECTION_TIMEOUT;
        });
    }
}

void TcpServer::broadcast(const Message& msg) {
    auto data = frame_message(msg);

    std::lock_guard lock(sessions_mutex_);
    for (auto& [id, session] : sessions_) {
        if (session->is_connected()) {
            session->send(msg);
        }
    }
}

void TcpServer::send_to_player(PlayerId player_id, const Message& msg) {
    std::lock_guard lock(sessions_mutex_);
    auto it = sessions_.find(player_id);
    if (it != sessions_.end() && it->second->is_connected()) {
        it->second->send(msg);
    }
}

size_t TcpServer::session_count() const {
    std::lock_guard lock(const_cast<std::mutex&>(sessions_mutex_));
    return sessions_.size();
}

} // namespace dandy::server
