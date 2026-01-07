# Dandy Tank Arena - Architecture Document

## Overview

Dandy is a networked tank battle arena game where players control robot tanks on a large cell-based map. The system consists of:

1. **Server**: Manages game state, physics, and broadcasts events
2. **Client**: Hosts tank AI plugins, handles communication, displays map
3. **Tank Plugins**: Shared objects (.so) implementing tank behavior

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         GAME SERVER                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │  Game Loop  │  │   Map/Grid  │  │    Event Dispatcher     │  │
│  │  (60 tick)  │  │  Management │  │                         │  │
│  └──────┬──────┘  └──────┬──────┘  └───────────┬─────────────┘  │
│         │                │                      │                │
│  ┌──────┴────────────────┴──────────────────────┴─────────────┐ │
│  │                    TCP Connection Manager                   │ │
│  └─────────────────────────────┬───────────────────────────────┘ │
└────────────────────────────────┼─────────────────────────────────┘
                                 │ TCP/IP
        ┌────────────────────────┼────────────────────────┐
        │                        │                        │
┌───────┴───────┐        ┌───────┴───────┐       ┌───────┴───────┐
│   CLIENT 1    │        │   CLIENT 2    │       │   CLIENT N    │
│  ┌─────────┐  │        │  ┌─────────┐  │       │  ┌─────────┐  │
│  │ Display │  │        │  │ Display │  │       │  │ Display │  │
│  └────┬────┘  │        │  └────┬────┘  │       │  └────┬────┘  │
│  ┌────┴────┐  │        │  ┌────┴────┐  │       │  ┌────┴────┐  │
│  │Protocol │  │        │  │Protocol │  │       │  │Protocol │  │
│  │ Handler │  │        │  │ Handler │  │       │  │ Handler │  │
│  └────┬────┘  │        │  └────┬────┘  │       │  └────┬────┘  │
│  ┌────┴────┐  │        │  ┌────┴────┐  │       │  ┌────┴────┐  │
│  │ Plugin  │  │        │  │ Plugin  │  │       │  │ Plugin  │  │
│  │ Loader  │  │        │  │ Loader  │  │       │  │ Loader  │  │
│  └────┬────┘  │        │  └────┬────┘  │       │  └────┬────┘  │
│  ┌────┴────┐  │        │  ┌────┴────┐  │       │  ┌────┴────┐  │
│  │Tank AI  │  │        │  │Tank AI  │  │       │  │Tank AI  │  │
│  │ (.so)   │  │        │  │ (.so)   │  │       │  │ (.so)   │  │
│  └─────────┘  │        │  └─────────┘  │       │  └─────────┘  │
└───────────────┘        └───────────────┘       └───────────────┘
```

## Component Design

### 1. Server Components

#### Game Engine (`server/engine/`)
- **GameLoop**: Fixed 60 tick/second simulation
- **World**: Manages the cell-based map grid
- **EntityManager**: Tracks all tanks, projectiles, resources
- **CombatSystem**: Handles firing, damage, destruction
- **SensorSystem**: Processes camera, IR, radar queries

#### Networking (`server/network/`)
- **TcpServer**: Accepts connections, manages sessions
- **SessionManager**: Maps connections to players
- **MessageSerializer**: Binary protocol encoding/decoding
- **EventBroadcaster**: Pushes events to relevant clients

### 2. Client Components

#### Display (`client/display/`)
- **MapRenderer**: ASCII/emoji map visualization
- **Console**: Terminal-based UI
- **StatusPanel**: Tank stats, resources, events

#### Communication (`client/network/`)
- **TcpClient**: Server connection handling
- **ProtocolHandler**: Message serialization
- **EventQueue**: Incoming event processing

#### Plugin System (`client/plugin/`)
- **PluginLoader**: Dynamic .so loading via dlopen
- **TankInterface**: Abstract interface for AI implementation
- **PluginManager**: Lifecycle management

### 3. Shared Library (`common/`)
- **Protocol**: Message types, serialization
- **Types**: Coordinate, Direction, TankState, etc.
- **Constants**: Game configuration values

## Map Design

### Grid System
- Map size: Configurable (default 256x256 cells)
- Each cell: 1 terrain type + optional entity

### Terrain Types
```
Symbol  Name           Effect
------  ----           ------
.       Ground         Normal movement
#       Wall           Blocks movement & projectiles
~       Water          Impassable
^       Mountain       Blocks movement, high ground
T       Tree/Forest    Partial cover, slows movement
=       Road           Faster movement
```

### Resources & Entities
```
Symbol  Name           Effect
------  ----           ------
F       Fuel Depot     Refuel tanks
A       Ammo Depot     Reload ammunition
+       Health Pack    Repair damage
R       Radar Upgrade  Extended radar range
S       Shield Boost   Temporary shield
@       Tank (self)    Player's tank
*       Tank (enemy)   Opponent's tank
o       Projectile     Active shell
X       Explosion      Impact/destruction
```

## Tank Capabilities

### Sensors
| Sensor | Range | Arc    | Detects      | Update Rate |
|--------|-------|--------|--------------|-------------|
| Camera | 10    | 90°    | Visible only | Real-time   |
| IR     | 15    | 360°   | Heat sources | 500ms       |
| Radar  | 25    | 360°   | All metal    | 1000ms      |

### Movement
- Speed: 1-3 cells/tick (based on terrain)
- Rotation: 45° per tick
- Fuel consumption: 1 unit per cell moved

### Combat
- Main gun: Range 20, reload 10 ticks
- Damage: 25-50 (based on range)
- Ammo capacity: 20 rounds (base)

## Directory Structure

```
dandy/
├── CMakeLists.txt              # Root CMake
├── docs/
│   ├── ARCHITECTURE.md         # This file
│   └── PROTOCOL.md             # Protocol specification
├── common/                     # Shared library
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── dandy/
│   │       ├── protocol.hpp
│   │       ├── types.hpp
│   │       ├── messages.hpp
│   │       └── constants.hpp
│   └── src/
│       ├── protocol.cpp
│       └── messages.cpp
├── server/                     # Game server
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── server/
│   │       ├── game_engine.hpp
│   │       ├── world.hpp
│   │       ├── entity.hpp
│   │       ├── tank.hpp
│   │       ├── tcp_server.hpp
│   │       └── session.hpp
│   └── src/
│       ├── main.cpp
│       ├── game_engine.cpp
│       ├── world.cpp
│       ├── entity.cpp
│       ├── tank.cpp
│       ├── tcp_server.cpp
│       └── session.cpp
├── client/                     # Game client
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── client/
│   │       ├── tcp_client.hpp
│   │       ├── display.hpp
│   │       ├── plugin_loader.hpp
│   │       └── tank_interface.hpp
│   └── src/
│       ├── main.cpp
│       ├── tcp_client.cpp
│       ├── display.cpp
│       └── plugin_loader.cpp
└── plugins/                    # Example AI plugins
    ├── CMakeLists.txt
    ├── simple_ai/
    │   ├── CMakeLists.txt
    │   └── simple_ai.cpp
    └── aggressive_ai/
        ├── CMakeLists.txt
        └── aggressive_ai.cpp
```

## Build Requirements

- C++23 compiler (GCC 13+ or Clang 17+)
- CMake 3.25+
- POSIX threads
- dlopen/dlsym for plugin loading

## Implementation Phases

### Phase 1: Foundation
1. Project structure and CMake setup
2. Common types and protocol definition
3. Basic TCP networking

### Phase 2: Server Core
1. Game world and map generation
2. Entity system (tanks, projectiles)
3. Game loop and tick system

### Phase 3: Client Core
1. TCP client connection
2. Protocol message handling
3. ASCII map display

### Phase 4: Game Logic
1. Movement and collision
2. Sensor systems (camera, IR, radar)
3. Combat (firing, damage, destruction)

### Phase 5: Plugin System
1. SO loading infrastructure
2. Tank AI interface definition
3. Example AI implementations

### Phase 6: Polish
1. Resource collection mechanics
2. Upgrade system
3. Game balancing
