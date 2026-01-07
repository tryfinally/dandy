#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <chrono>
#include <compare>

namespace dandy {

// Basic type aliases
using PlayerId = uint32_t;
using TankId = uint32_t;
using GameId = uint32_t;
using TickNumber = uint32_t;

// Coordinate type
struct Coord {
    int16_t x{0};
    int16_t y{0};

    auto operator<=>(const Coord&) const = default;

    [[nodiscard]] Coord operator+(const Coord& other) const {
        return {static_cast<int16_t>(x + other.x), static_cast<int16_t>(y + other.y)};
    }

    [[nodiscard]] Coord operator-(const Coord& other) const {
        return {static_cast<int16_t>(x - other.x), static_cast<int16_t>(y - other.y)};
    }

    [[nodiscard]] double distance_to(const Coord& other) const;
    [[nodiscard]] int manhattan_distance(const Coord& other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }
};

// Direction enum (8-directional)
enum class Direction : uint8_t {
    North = 0,
    NorthEast = 1,
    East = 2,
    SouthEast = 3,
    South = 4,
    SouthWest = 5,
    West = 6,
    NorthWest = 7
};

// Get direction delta for movement
[[nodiscard]] Coord direction_delta(Direction dir);

// Terrain types
enum class Terrain : uint8_t {
    Ground = 0,    // Normal movement
    Wall = 1,      // Blocks movement & projectiles
    Water = 2,     // Impassable
    Mountain = 3,  // Blocks movement, high ground
    Tree = 4,      // Partial cover, slows movement
    Road = 5       // Faster movement
};

// Entity types that can occupy cells
enum class EntityType : uint8_t {
    Empty = 0,
    TankSelf = 1,
    TankEnemy = 2,
    TankFriendly = 3,
    Projectile = 4,
    FuelDepot = 5,
    AmmoDepot = 6,
    HealthPack = 7,
    RadarUpgrade = 8,
    ShieldBoost = 9,
    SpeedBoost = 10,
    Explosion = 11
};

// Resource types
enum class ResourceType : uint8_t {
    Fuel = 0,
    Ammo = 1,
    Health = 2,
    RadarUpgrade = 3,
    ShieldBoost = 4,
    SpeedBoost = 5
};

// Scan types
enum class ScanType : uint8_t {
    Camera = 0,    // Visual, limited arc
    Infrared = 1,  // Heat detection, 360°
    Radar = 2      // Metal detection, 360°, longest range
};

// Game status
enum class GameStatus : uint8_t {
    Waiting = 0,
    Starting = 1,
    Running = 2,
    Paused = 3,
    Ended = 4
};

// Fire result
enum class FireResult : uint8_t {
    Miss = 0,
    HitTank = 1,
    HitObstacle = 2,
    OutOfRange = 3,
    NoAmmo = 4,
    Reloading = 5
};

// Disconnect reason
enum class DisconnectReason : uint8_t {
    Quit = 0,
    Timeout = 1,
    Error = 2,
    Kicked = 3
};

// Contact from scan
struct Contact {
    Coord position;
    EntityType type{EntityType::Empty};
    uint8_t flags{0};  // bit0=moving, bit1=friendly, bit2=damaged

    [[nodiscard]] bool is_moving() const { return flags & 0x01; }
    [[nodiscard]] bool is_friendly() const { return flags & 0x02; }
    [[nodiscard]] bool is_damaged() const { return flags & 0x04; }
};

// Tank status
struct TankStatus {
    TankId id{0};
    PlayerId owner{0};
    Coord position;
    Direction direction{Direction::North};
    uint16_t turret_angle{0};  // 0-359 degrees

    uint16_t health{100};
    uint16_t max_health{100};
    uint16_t fuel{100};
    uint16_t max_fuel{100};
    uint16_t ammo{20};
    uint16_t max_ammo{20};

    uint8_t speed{0};          // Current speed (0-3)
    uint8_t flags{0};          // bit0=moving, bit1=reloading, bit2=damaged, bit3=shielded
    uint32_t score{0};

    // Cooldowns (in ticks)
    uint8_t reload_cooldown{0};
    uint8_t scan_cooldown{0};

    // Upgrades
    uint8_t radar_range_bonus{0};
    uint8_t speed_bonus{0};

    [[nodiscard]] bool is_moving() const { return flags & 0x01; }
    [[nodiscard]] bool is_reloading() const { return flags & 0x02; }
    [[nodiscard]] bool is_damaged() const { return flags & 0x04; }
    [[nodiscard]] bool is_shielded() const { return flags & 0x08; }
    [[nodiscard]] bool is_alive() const { return health > 0; }
    [[nodiscard]] float health_percent() const { return max_health > 0 ? static_cast<float>(health) / max_health * 100.0f : 0.0f; }
};

// Cell in map view
struct Cell {
    Terrain terrain{Terrain::Ground};
    EntityType entity{EntityType::Empty};

    [[nodiscard]] uint8_t encode() const {
        return (static_cast<uint8_t>(terrain) & 0x0F) |
               ((static_cast<uint8_t>(entity) & 0x0F) << 4);
    }

    static Cell decode(uint8_t encoded) {
        return {
            static_cast<Terrain>(encoded & 0x0F),
            static_cast<EntityType>((encoded >> 4) & 0x0F)
        };
    }
};

// Map view data
struct MapView {
    TankId viewer_tank{0};
    Coord center;
    uint8_t width{0};
    uint8_t height{0};
    std::vector<Cell> cells;

    [[nodiscard]] std::optional<Cell> at(int16_t x, int16_t y) const {
        int16_t local_x = x - (center.x - width / 2);
        int16_t local_y = y - (center.y - height / 2);
        if (local_x < 0 || local_x >= width || local_y < 0 || local_y >= height) {
            return std::nullopt;
        }
        return cells[local_y * width + local_x];
    }
};

// Player info
struct PlayerInfo {
    PlayerId id{0};
    std::string name;
    uint32_t score{0};
    uint8_t tank_count{0};
    bool connected{true};
};

// Game info
struct GameInfo {
    GameId id{0};
    GameStatus status{GameStatus::Waiting};
    TickNumber current_tick{0};
    uint16_t map_width{256};
    uint16_t map_height{256};
    std::vector<PlayerInfo> players;
};

// Symbols for rendering
namespace symbols {
    // Terrain
    constexpr char GROUND = '.';
    constexpr char WALL = '#';
    constexpr char WATER = '~';
    constexpr char MOUNTAIN = '^';
    constexpr char TREE = 'T';
    constexpr char ROAD = '=';

    // Entities
    constexpr char TANK_SELF = '@';
    constexpr char TANK_ENEMY = '*';
    constexpr char TANK_FRIENDLY = '&';
    constexpr char PROJECTILE = 'o';
    constexpr char FUEL = 'F';
    constexpr char AMMO = 'A';
    constexpr char HEALTH = '+';
    constexpr char RADAR = 'R';
    constexpr char SHIELD = 'S';
    constexpr char SPEED = '>';
    constexpr char EXPLOSION = 'X';
    constexpr char UNKNOWN = '?';

    [[nodiscard]] char terrain_to_char(Terrain t);
    [[nodiscard]] char entity_to_char(EntityType e);
    [[nodiscard]] const char* terrain_to_emoji(Terrain t);
    [[nodiscard]] const char* entity_to_emoji(EntityType e);
}

} // namespace dandy
