# Dandy Tank Arena

A networked multiplayer tank battle arena game written in C++23. Players control robot tanks on a cell-based map, using sensors to detect enemies and resources while engaging in tactical combat.

## Features

- **Server-based game simulation** with 60 tick/second game loop
- **TCP-based networking** with binary protocol
- **Sensor systems**: Camera, Infrared, and Radar with different capabilities
- **Resource collection**: Fuel, ammo, health, and upgrades
- **Plugin architecture**: Tank AI implemented as shared objects (.so)
- **ASCII/Emoji map display** for terminal-based visualization

## Building

### Requirements

- C++23 compatible compiler (GCC 13+ or Clang 17+)
- CMake 3.25+
- POSIX-compliant system (Linux, macOS)

### Build Steps

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)
```

### Build Output

After building, you'll find:
- `bin/dandy_server` - Game server executable
- `bin/dandy_client` - Game client executable
- `lib/simple_ai.so` - Simple AI plugin
- `lib/aggressive_ai.so` - Aggressive AI plugin

## Running

### Start the Server

```bash
./bin/dandy_server --port 7777
```

Server options:
- `-p, --port PORT` - Port to listen on (default: 7777)
- `-h, --help` - Show help

### Start a Client

```bash
# Without AI (manual observation)
./bin/dandy_client --host localhost --port 7777 --name Player1

# With Simple AI
./bin/dandy_client --host localhost --port 7777 --name AIBot1 --ai lib/simple_ai.so

# With Aggressive AI
./bin/dandy_client --host localhost --port 7777 --name AIBot2 --ai lib/aggressive_ai.so

# With emoji display
./bin/dandy_client --host localhost --port 7777 --name Player1 --emoji
```

Client options:
- `-h, --host HOST` - Server hostname (default: localhost)
- `-p, --port PORT` - Server port (default: 7777)
- `-n, --name NAME` - Player name
- `-a, --ai PATH` - Path to AI plugin (.so file)
- `-e, --emoji` - Use emoji display mode
- `--help` - Show help

## Game Mechanics

### Map

The map is a 256x256 grid with various terrain types:
- `.` Ground - Normal movement
- `#` Wall - Blocks movement and projectiles
- `~` Water - Impassable
- `^` Mountain - Impassable, blocks line of sight
- `T` Trees - Slows movement, partial cover
- `=` Road - Fast movement

### Resources

- `F` Fuel Depot - Refuel your tank
- `A` Ammo Depot - Reload ammunition
- `+` Health Pack - Repair damage
- `R` Radar Upgrade - Extended radar range
- `S` Shield Boost - Temporary damage reduction
- `>` Speed Boost - Temporary speed increase

### Sensors

| Sensor | Range | Arc | Detects | Cooldown |
|--------|-------|-----|---------|----------|
| Camera | 10 | 90° | Visible | ~16ms |
| IR | 15 | 360° | Heat | 500ms |
| Radar | 25 | 360° | Metal | 1000ms |

### Combat

- Main gun range: 20 cells
- Damage: 25-50 (varies by distance)
- Reload time: 10 ticks (~166ms)
- Ammo capacity: 20 rounds

## Writing AI Plugins

Create a shared library implementing the `ITankAI` interface:

```cpp
#include "client/tank_interface.hpp"

class MyAI : public dandy::client::ITankAI {
public:
    explicit MyAI(dandy::client::ITankController* controller)
        : controller_(controller) {}

    void on_init(dandy::TankId tank_id) override {
        // Initialize
    }

    void on_tick(dandy::TickNumber tick) override {
        // Make decisions each tick
        controller_->scan_radar();

        if (controller_->can_fire()) {
            // Fire at target
        }

        controller_->move(dandy::Direction::North, 2);
    }

    void on_spawn(dandy::Coord position, dandy::Direction direction) override {}
    void on_hit(dandy::TankId attacker, uint16_t damage, uint16_t remaining) override {}
    void on_destroyed(dandy::TankId killer) override {}
    void on_scan_result(dandy::ScanType type, const std::vector<dandy::Contact>& contacts) override {}
    void on_fire_result(dandy::FireResult result, dandy::Coord hit_pos, dandy::TankId target, uint16_t damage) override {}
    void on_obstacle(dandy::Coord position, dandy::Terrain terrain) override {}
    void on_resource_collected(dandy::ResourceType type, uint16_t amount) override {}
    void on_map_update(const dandy::MapView& view) override {}

private:
    dandy::client::ITankController* controller_;
};

DANDY_REGISTER_PLUGIN(MyAI, "My AI", "1.0", "Author", "Description")
```

Build your plugin:

```cmake
add_library(my_ai SHARED my_ai.cpp)
target_link_libraries(my_ai PRIVATE dandy::common)
set_target_properties(my_ai PROPERTIES PREFIX "")
```

## Protocol

See [docs/PROTOCOL.md](docs/PROTOCOL.md) for the complete network protocol specification.

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for system architecture documentation.

## Project Structure

```
dandy/
├── common/           # Shared library (types, protocol)
│   ├── include/dandy/
│   └── src/
├── server/           # Game server
│   ├── include/server/
│   └── src/
├── client/           # Game client
│   ├── include/client/
│   └── src/
├── plugins/          # AI plugins
│   ├── simple_ai/
│   └── aggressive_ai/
└── docs/             # Documentation
```

## License

MIT License - See [LICENSE](LICENSE) file.
