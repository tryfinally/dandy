#include "server/tcp_server.hpp"
#include "server/game_engine.hpp"
#include <iostream>
#include <csignal>
#include <cstring>

using namespace dandy::server;
using namespace dandy;

namespace {
    std::atomic<bool> g_running{true};
    TcpServer* g_server{nullptr};
    GameEngine* g_engine{nullptr};

    void signal_handler(int signum) {
        std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
        g_running.store(false);
        if (g_engine) g_engine->stop();
        if (g_server) g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    uint16_t port = constants::DEFAULT_PORT;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            std::cout << "Dandy Tank Arena Server\n"
                      << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  -p, --port PORT    Port to listen on (default: "
                      << constants::DEFAULT_PORT << ")\n"
                      << "  -h, --help         Show this help message\n";
            return 0;
        }
    }

    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "====================================\n"
              << "    Dandy Tank Arena Server v1.0    \n"
              << "====================================\n" << std::endl;

    // Create game engine
    GameEngine engine;
    g_engine = &engine;

    // Create TCP server
    TcpServer server(port);
    g_server = &server;

    // Link them together
    server.set_game_engine(&engine);
    engine.set_server(&server);

    // Start components
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    engine.start();

    std::cout << "\nServer running. Press Ctrl+C to stop.\n" << std::endl;

    // Main loop - just wait for shutdown signal
    while (g_running.load() && server.is_running() && engine.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    engine.stop();
    server.stop();

    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}
