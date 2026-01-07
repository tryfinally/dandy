#include "client/game_client.hpp"
#include "dandy/constants.hpp"
#include <iostream>
#include <csignal>
#include <cstring>
#include <thread>

using namespace dandy::client;
using namespace dandy;

namespace {
    std::atomic<bool> g_running{true};
    GameClient* g_client{nullptr};

    void signal_handler(int signum) {
        std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
        g_running.store(false);
        if (g_client) g_client->stop();
    }
}

void print_help(const char* program) {
    std::cout << "Dandy Tank Arena Client\n"
              << "Usage: " << program << " [options]\n"
              << "Options:\n"
              << "  -h, --host HOST    Server hostname (default: localhost)\n"
              << "  -p, --port PORT    Server port (default: " << constants::DEFAULT_PORT << ")\n"
              << "  -n, --name NAME    Player name (default: Player)\n"
              << "  -a, --ai PATH      Path to AI plugin (.so file)\n"
              << "  -e, --emoji        Use emoji display mode\n"
              << "  --help             Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string host = "localhost";
    uint16_t port = constants::DEFAULT_PORT;
    std::string player_name = "Player";
    std::string ai_plugin_path;
    bool use_emoji = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--host") == 0) {
            if (i + 1 < argc) {
                host = argv[++i];
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port = static_cast<uint16_t>(std::stoi(argv[++i]));
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0) {
            if (i + 1 < argc) {
                player_name = argv[++i];
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--ai") == 0) {
            if (i + 1 < argc) {
                ai_plugin_path = argv[++i];
            }
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--emoji") == 0) {
            use_emoji = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
    }

    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "====================================\n"
              << "    Dandy Tank Arena Client v1.0    \n"
              << "====================================\n" << std::endl;

    // Create game client
    GameClient client;
    g_client = &client;

    if (use_emoji) {
        client.display().set_mode(DisplayMode::Emoji);
    }

    // Load AI plugin if specified
    if (!ai_plugin_path.empty()) {
        if (!client.load_plugin(ai_plugin_path)) {
            std::cerr << "Failed to load AI plugin: " << ai_plugin_path << std::endl;
            return 1;
        }
    }

    // Connect to server
    std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
    if (!client.connect(host, port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    // Wait for welcome message
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Authenticate
    std::cout << "Authenticating as " << player_name << "..." << std::endl;
    client.authenticate(player_name);

    // Wait for authentication
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!client.is_authenticated()) {
        std::cerr << "Authentication failed" << std::endl;
        return 1;
    }

    // Place initial tank (at a default position, server will adjust if needed)
    std::cout << "Placing tank..." << std::endl;
    client.place_tank({50, 50}, Direction::North);

    // Start game loop
    client.run();

    std::cout << "\nGame running. Press Ctrl+C to stop.\n" << std::endl;

    // Main loop
    while (g_running.load() && client.is_connected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    client.stop();
    client.disconnect();

    std::cout << "Client shutdown complete." << std::endl;
    return 0;
}
