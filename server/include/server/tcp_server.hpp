#pragma once

#include "dandy/protocol.hpp"
#include "dandy/constants.hpp"
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>

namespace dandy::server {

class Session;
class GameEngine;

// TCP Server
class TcpServer {
public:
    explicit TcpServer(uint16_t port = constants::DEFAULT_PORT);
    ~TcpServer();

    // Lifecycle
    bool start();
    void stop();
    [[nodiscard]] bool is_running() const { return running_.load(); }

    // Set game engine reference
    void set_game_engine(GameEngine* engine) { game_engine_ = engine; }

    // Broadcast to all sessions
    void broadcast(const Message& msg);

    // Send to specific player
    void send_to_player(PlayerId player_id, const Message& msg);

    // Get active session count
    [[nodiscard]] size_t session_count() const;

private:
    void accept_loop();
    void handle_client(int client_fd, sockaddr_in client_addr);
    void cleanup_sessions();

    uint16_t port_;
    int server_fd_{-1};
    std::atomic<bool> running_{false};
    std::thread accept_thread_;
    std::thread cleanup_thread_;

    std::mutex sessions_mutex_;
    std::unordered_map<PlayerId, std::shared_ptr<Session>> sessions_;
    std::vector<std::shared_ptr<Session>> pending_sessions_;

    GameEngine* game_engine_{nullptr};
};

} // namespace dandy::server
