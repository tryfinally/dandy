#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sstream>

class GameClient {
private:
    int sock_fd;
    int tank_id;
    int tank_x, tank_y;
    int arena_width, arena_height;
    int ammo, health;
    std::string last_map;

    void clearScreen() {
        std::cout << "\033[2J\033[H";
    }

    void setColor(char terrain) {
        switch (terrain) {
            case '~': std::cout << "\033[34m"; break;  // Water - blue
            case '^': std::cout << "\033[90m"; break;  // Mountain - gray
            case 'A': std::cout << "\033[33m"; break;  // Ammo - yellow
            case 'U': std::cout << "\033[35m"; break;  // Upgrade - magenta
            case '@': std::cout << "\033[32m"; break;  // Your tank - green
            case 'T': std::cout << "\033[31m"; break;  // Enemy tank - red
            case '#': std::cout << "\033[90m"; break;  // Out of bounds - gray
            default:  std::cout << "\033[0m";  break;  // Empty - default
        }
    }

    void resetColor() {
        std::cout << "\033[0m";
    }

    void displayMap() {
        clearScreen();

        std::cout << "╔════════════════════════════════════════════════════╗\n";
        std::cout << "║           DANDY TANKS - Arena Combat               ║\n";
        std::cout << "╠════════════════════════════════════════════════════╣\n";
        std::cout << "║ Tank #" << tank_id << " | Position: (" << tank_x << ", " << tank_y << ")\n";
        std::cout << "║ Health: " << health << " | Ammo: " << ammo << "\n";
        std::cout << "║ Arena: " << arena_width << "x" << arena_height << "\n";
        std::cout << "╠════════════════════════════════════════════════════╣\n";

        // Parse and display map
        std::istringstream iss(last_map);
        std::string line;
        bool in_map = false;

        while (std::getline(iss, line)) {
            if (line.find("MAP_START") != std::string::npos) {
                in_map = true;
                std::cout << "║ ";
                continue;
            }
            if (line == "MAP_END") {
                in_map = false;
                continue;
            }
            if (in_map) {
                std::cout << "║ ";
                for (char c : line) {
                    setColor(c);
                    std::cout << c << ' ';
                    resetColor();
                }
                std::cout << "║\n";
            }
        }

        std::cout << "╠════════════════════════════════════════════════════╣\n";
        std::cout << "║ Legend:                                            ║\n";
        std::cout << "║ ";
        setColor('@'); std::cout << "@"; resetColor(); std::cout << "=You  ";
        setColor('T'); std::cout << "T"; resetColor(); std::cout << "=Enemy  ";
        setColor('~'); std::cout << "~"; resetColor(); std::cout << "=Water  ";
        setColor('^'); std::cout << "^"; resetColor(); std::cout << "=Mountain  ";
        setColor('A'); std::cout << "A"; resetColor(); std::cout << "=Ammo  ";
        setColor('U'); std::cout << "U"; resetColor(); std::cout << "=Upgrade";
        std::cout << " ║\n";
        std::cout << "╠════════════════════════════════════════════════════╣\n";
        std::cout << "║ Controls: W/↑=North  S/↓=South  A/←=West  D/→=East ║\n";
        std::cout << "║           L=Look  Q=Quit                           ║\n";
        std::cout << "╚════════════════════════════════════════════════════╝\n";
        std::cout << "> ";
        std::cout.flush();
    }

    void parseResponse(const std::string& response) {
        std::istringstream iss(response);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.find("WELCOME") != std::string::npos) {
                sscanf(line.c_str(), "WELCOME %d", &tank_id);
            } else if (line.find("ARENA_SIZE") != std::string::npos) {
                sscanf(line.c_str(), "ARENA_SIZE %d %d", &arena_width, &arena_height);
            } else if (line.find("TANK_POS") != std::string::npos) {
                sscanf(line.c_str(), "TANK_POS %d %d", &tank_x, &tank_y);
            } else if (line.find("TANK_STATS") != std::string::npos) {
                sscanf(line.c_str(), "TANK_STATS ammo=%d health=%d", &ammo, &health);
            }
        }

        // Store the full response for map display
        if (response.find("MAP_START") != std::string::npos) {
            last_map = response;
        }
    }

public:
    GameClient() : sock_fd(-1), tank_id(0), tank_x(0), tank_y(0),
                   arena_width(0), arena_height(0), ammo(0), health(0) {}

    ~GameClient() {
        if (sock_fd >= 0) close(sock_fd);
    }

    bool connect(const std::string& host, int port) {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            std::cerr << "Invalid address" << std::endl;
            return false;
        }

        if (::connect(sock_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            return false;
        }

        // Receive welcome message and initial map
        char buffer[8192];
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            parseResponse(buffer);
        }

        return true;
    }

    void sendCommand(const std::string& cmd) {
        std::string msg = cmd + "\n";
        send(sock_fd, msg.c_str(), msg.size(), 0);

        char buffer[8192];
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            parseResponse(buffer);
        }
    }

    void run() {
        // Set terminal to raw mode for single-key input
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        displayMap();

        while (true) {
            char c;
            if (read(STDIN_FILENO, &c, 1) <= 0) break;

            std::string cmd;
            switch (c) {
                case 'w': case 'W': case 'A' - 64: // W or Up arrow
                    cmd = "NORTH";
                    break;
                case 's': case 'S': case 'B' - 64: // S or Down arrow
                    cmd = "SOUTH";
                    break;
                case 'a': case 'A' - 32: case 'D' - 64: // A or Left arrow
                    if (c == 'a' || c == 'A' - 32) cmd = "WEST";
                    break;
                case 'd': case 'D': case 'C' - 64: // D or Right arrow
                    cmd = "EAST";
                    break;
                case 'l': case 'L':
                    cmd = "LOOK";
                    break;
                case 'q': case 'Q':
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    clearScreen();
                    std::cout << "Thanks for playing!\n";
                    return;
                case '\033': // Escape sequence for arrow keys
                    char seq[2];
                    if (read(STDIN_FILENO, seq, 2) == 2 && seq[0] == '[') {
                        switch (seq[1]) {
                            case 'A': cmd = "NORTH"; break;
                            case 'B': cmd = "SOUTH"; break;
                            case 'C': cmd = "EAST"; break;
                            case 'D': cmd = "WEST"; break;
                        }
                    }
                    break;
            }

            if (!cmd.empty()) {
                sendCommand(cmd);
                displayMap();
            }
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --host IP   Server IP (default: 127.0.0.1)\n"
                      << "  --port N    Server port (default: 8080)\n";
            return 0;
        }
    }

    GameClient client;

    std::cout << "Connecting to " << host << ":" << port << "...\n";

    if (!client.connect(host, port)) {
        return 1;
    }

    client.run();

    return 0;
}
