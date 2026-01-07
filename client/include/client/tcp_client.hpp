#pragma once

#include "dandy/protocol.hpp"
#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>

namespace dandy::client {

class TcpClient {
public:
    using MessageHandler = std::function<void(std::unique_ptr<Message>)>;

    TcpClient();
    ~TcpClient();

    // Connection
    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    [[nodiscard]] bool is_connected() const { return connected_.load(); }

    // Send message
    void send(const Message& msg);

    // Set message handler
    void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }

    // Get server info (after welcome)
    [[nodiscard]] uint16_t server_version() const { return server_version_; }
    [[nodiscard]] uint16_t tick_rate() const { return tick_rate_; }
    [[nodiscard]] uint8_t max_tanks() const { return max_tanks_; }

private:
    void read_loop();
    void write_loop();
    void process_message(std::unique_ptr<Message> msg);

    int socket_fd_{-1};
    std::atomic<bool> connected_{false};

    std::thread read_thread_;
    std::thread write_thread_;

    std::mutex write_mutex_;
    std::queue<std::vector<uint8_t>> write_queue_;
    std::condition_variable write_cv_;

    MessageHandler message_handler_;

    // Server info
    uint16_t server_version_{0};
    uint16_t tick_rate_{60};
    uint8_t max_tanks_{5};
};

} // namespace dandy::client
