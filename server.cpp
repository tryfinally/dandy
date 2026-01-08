#include <iostream>
#include <vector>
#include <random>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <map>
#include <sstream>
#include <algorithm>

// Terrain types
enum Terrain : char {
    EMPTY = '.',
    WATER = '~',      // 10% - not crossable
    MOUNTAIN = '^',   // 15% - not crossable, blocks scanning
    AMMO_DEPOT = 'A', // 5%
    UPGRADE = 'U'     // 2%
};

struct Tank {
    int id;
    int x, y;
    int ammo = 10;
    int health = 100;
    bool alive = true;
};

class GameServer {
private:
    int width, height;
    std::vector<std::vector<char>> map;
    std::map<int, Tank> tanks;
    std::mutex game_mutex;
    int next_tank_id = 1;
    int server_fd;

public:
    GameServer(int w = 1000, int h = 1000) : width(w), height(h) {
        generateMap();
    }

    void generateMap() {
        map.resize(height, std::vector<char>(width, EMPTY));

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 100);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int roll = dis(gen);
                if (roll <= 10) {
                    map[y][x] = WATER;       // 10%
                } else if (roll <= 25) {
                    map[y][x] = MOUNTAIN;    // 15%
                } else if (roll <= 30) {
                    map[y][x] = AMMO_DEPOT;  // 5%
                } else if (roll <= 32) {
                    map[y][x] = UPGRADE;     // 2%
                }
            }
        }

        std::cout << "Map generated: " << width << "x" << height << std::endl;
    }

    std::pair<int, int> findSpawnPoint() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis_x(0, width - 1);
        std::uniform_int_distribution<> dis_y(0, height - 1);

        int x, y;
        do {
            x = dis_x(gen);
            y = dis_y(gen);
        } while (map[y][x] != EMPTY);

        return {x, y};
    }

    int spawnTank() {
        std::lock_guard<std::mutex> lock(game_mutex);

        auto [x, y] = findSpawnPoint();
        Tank tank;
        tank.id = next_tank_id++;
        tank.x = x;
        tank.y = y;
        tanks[tank.id] = tank;

        std::cout << "Tank " << tank.id << " spawned at (" << x << ", " << y << ")" << std::endl;
        return tank.id;
    }

    std::string getMapAroundTank(int tank_id, int radius = 10) {
        std::lock_guard<std::mutex> lock(game_mutex);

        if (tanks.find(tank_id) == tanks.end()) {
            return "ERROR: Tank not found\n";
        }

        Tank& tank = tanks[tank_id];
        std::stringstream ss;

        ss << "TANK_POS " << tank.x << " " << tank.y << "\n";
        ss << "TANK_STATS ammo=" << tank.ammo << " health=" << tank.health << "\n";
        ss << "MAP_START " << radius << "\n";

        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                int mx = tank.x + dx;
                int my = tank.y + dy;

                if (mx < 0 || mx >= width || my < 0 || my >= height) {
                    ss << '#'; // Out of bounds
                } else if (dx == 0 && dy == 0) {
                    ss << '@'; // Tank position
                } else {
                    // Check if another tank is here
                    bool tank_here = false;
                    for (auto& [id, t] : tanks) {
                        if (id != tank_id && t.alive && t.x == mx && t.y == my) {
                            ss << 'T';
                            tank_here = true;
                            break;
                        }
                    }
                    if (!tank_here) {
                        ss << map[my][mx];
                    }
                }
            }
            ss << "\n";
        }

        ss << "MAP_END\n";
        return ss.str();
    }

    std::string processCommand(int tank_id, const std::string& cmd) {
        std::lock_guard<std::mutex> lock(game_mutex);

        if (tanks.find(tank_id) == tanks.end()) {
            return "ERROR: Tank not found\n";
        }

        Tank& tank = tanks[tank_id];

        if (cmd == "LOOK") {
            // Return map (handled separately without lock)
        } else if (cmd == "NORTH" || cmd == "SOUTH" || cmd == "EAST" || cmd == "WEST") {
            int nx = tank.x, ny = tank.y;
            if (cmd == "NORTH") ny--;
            else if (cmd == "SOUTH") ny++;
            else if (cmd == "EAST") nx++;
            else if (cmd == "WEST") nx--;

            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                char terrain = map[ny][nx];
                if (terrain != WATER && terrain != MOUNTAIN) {
                    tank.x = nx;
                    tank.y = ny;

                    // Pick up items
                    if (terrain == AMMO_DEPOT) {
                        tank.ammo += 5;
                        map[ny][nx] = EMPTY;
                        return "OK: Moved and picked up ammo (+5)\n";
                    } else if (terrain == UPGRADE) {
                        tank.health = std::min(100, tank.health + 25);
                        map[ny][nx] = EMPTY;
                        return "OK: Moved and picked up upgrade (+25 health)\n";
                    }
                    return "OK: Moved\n";
                } else {
                    return "ERROR: Cannot cross " + std::string(terrain == WATER ? "water" : "mountain") + "\n";
                }
            } else {
                return "ERROR: Out of bounds\n";
            }
        }

        return "ERROR: Unknown command\n";
    }

    void handleClient(int client_fd) {
        int tank_id = spawnTank();

        std::string welcome = "WELCOME " + std::to_string(tank_id) + "\n";
        welcome += "ARENA_SIZE " + std::to_string(width) + " " + std::to_string(height) + "\n";
        send(client_fd, welcome.c_str(), welcome.size(), 0);

        // Send initial map view
        std::string map_view = getMapAroundTank(tank_id);
        send(client_fd, map_view.c_str(), map_view.size(), 0);

        char buffer[1024];
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes <= 0) break;

            std::string cmd(buffer);
            // Trim whitespace
            cmd.erase(cmd.find_last_not_of(" \n\r\t") + 1);

            std::string response;
            if (cmd == "LOOK") {
                response = getMapAroundTank(tank_id);
            } else {
                response = processCommand(tank_id, cmd);
                if (response.substr(0, 2) == "OK") {
                    response += getMapAroundTank(tank_id);
                }
            }

            send(client_fd, response.c_str(), response.size(), 0);
        }

        // Remove tank on disconnect
        {
            std::lock_guard<std::mutex> lock(game_mutex);
            tanks.erase(tank_id);
        }

        close(client_fd);
        std::cout << "Tank " << tank_id << " disconnected" << std::endl;
    }

    void start(int port = 8080) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return;
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Failed to bind" << std::endl;
            return;
        }

        listen(server_fd, 10);
        std::cout << "Server listening on port " << port << std::endl;
        std::cout << "Arena: " << width << "x" << height << std::endl;

        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

            if (client_fd >= 0) {
                std::cout << "Client connected from " << inet_ntoa(client_addr.sin_addr) << std::endl;
                std::thread(&GameServer::handleClient, this, client_fd).detach();
            }
        }
    }
};

int main(int argc, char* argv[]) {
    int width = 1000;
    int height = 1000;
    int port = 8080;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--width" && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --width N   Arena width (default: 1000)\n"
                      << "  --height N  Arena height (default: 1000)\n"
                      << "  --port N    Server port (default: 8080)\n";
            return 0;
        }
    }

    GameServer server(width, height);
    server.start(port);

    return 0;
}
