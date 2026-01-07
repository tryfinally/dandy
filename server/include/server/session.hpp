#pragma once

#include "dandy/protocol.hpp"
#include "dandy/constants.hpp"
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

namespace dandy::server {

class GameEngine;

// Client session handler
class Session : public std::enable_shared_from_this<Session> {
public:
    using MessageHandler = std::function<void(Session&, std::unique_ptr<Message>)>;

    Session(int socket_fd, GameEngine* engine);
    ~Session();

    // Start async read/write threads
    void start();
    void stop();

    // Send message to client
    void send(const Message& msg);

    // Accessors
    [[nodiscard]] bool is_connected() const { return connected_.load(); }
    [[nodiscard]] bool is_authenticated() const { return authenticated_; }
    [[nodiscard]] PlayerId player_id() const { return player_id_; }
    [[nodiscard]] const std::string& player_name() const { return player_name_; }
    [[nodiscard]] std::chrono::steady_clock::time_point last_activity() const { return last_activity_; }

    // Authentication
    void authenticate(PlayerId id, const std::string& name);

private:
    void read_loop();
    void write_loop();
    void process_message(std::unique_ptr<Message> msg);

    int socket_fd_;
    GameEngine* game_engine_;
    std::atomic<bool> connected_{false};
    bool authenticated_{false};
    PlayerId player_id_{0};
    std::string player_name_;
    std::chrono::steady_clock::time_point last_activity_;

    std::thread read_thread_;
    std::thread write_thread_;

    std::mutex write_mutex_;
    std::queue<std::vector<uint8_t>> write_queue_;
    std::condition_variable write_cv_;

    std::vector<uint8_t> read_buffer_;
    size_t read_pos_{0};
};

} // namespace dandy::server
