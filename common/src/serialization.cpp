#include "dandy/types.hpp"
#include <cmath>

namespace dandy {

double Coord::distance_to(const Coord& other) const {
    double dx = static_cast<double>(x - other.x);
    double dy = static_cast<double>(y - other.y);
    return std::sqrt(dx * dx + dy * dy);
}

Coord direction_delta(Direction dir) {
    switch (dir) {
        case Direction::North:     return {0, -1};
        case Direction::NorthEast: return {1, -1};
        case Direction::East:      return {1, 0};
        case Direction::SouthEast: return {1, 1};
        case Direction::South:     return {0, 1};
        case Direction::SouthWest: return {-1, 1};
        case Direction::West:      return {-1, 0};
        case Direction::NorthWest: return {-1, -1};
    }
    return {0, 0};
}

namespace symbols {

char terrain_to_char(Terrain t) {
    switch (t) {
        case Terrain::Ground:   return GROUND;
        case Terrain::Wall:     return WALL;
        case Terrain::Water:    return WATER;
        case Terrain::Mountain: return MOUNTAIN;
        case Terrain::Tree:     return TREE;
        case Terrain::Road:     return ROAD;
    }
    return UNKNOWN;
}

char entity_to_char(EntityType e) {
    switch (e) {
        case EntityType::Empty:        return ' ';
        case EntityType::TankSelf:     return TANK_SELF;
        case EntityType::TankEnemy:    return TANK_ENEMY;
        case EntityType::TankFriendly: return TANK_FRIENDLY;
        case EntityType::Projectile:   return PROJECTILE;
        case EntityType::FuelDepot:    return FUEL;
        case EntityType::AmmoDepot:    return AMMO;
        case EntityType::HealthPack:   return HEALTH;
        case EntityType::RadarUpgrade: return RADAR;
        case EntityType::ShieldBoost:  return SHIELD;
        case EntityType::SpeedBoost:   return SPEED;
        case EntityType::Explosion:    return EXPLOSION;
    }
    return UNKNOWN;
}

const char* terrain_to_emoji(Terrain t) {
    switch (t) {
        case Terrain::Ground:   return "  ";  // Empty space
        case Terrain::Wall:     return "🧱";
        case Terrain::Water:    return "🌊";
        case Terrain::Mountain: return "⛰️ ";
        case Terrain::Tree:     return "🌲";
        case Terrain::Road:     return "🛤️ ";
    }
    return "❓";
}

const char* entity_to_emoji(EntityType e) {
    switch (e) {
        case EntityType::Empty:        return nullptr;
        case EntityType::TankSelf:     return "🚜";
        case EntityType::TankEnemy:    return "💀";
        case EntityType::TankFriendly: return "🤝";
        case EntityType::Projectile:   return "💣";
        case EntityType::FuelDepot:    return "⛽";
        case EntityType::AmmoDepot:    return "🔫";
        case EntityType::HealthPack:   return "❤️ ";
        case EntityType::RadarUpgrade: return "📡";
        case EntityType::ShieldBoost:  return "🛡️ ";
        case EntityType::SpeedBoost:   return "⚡";
        case EntityType::Explosion:    return "💥";
    }
    return "❓";
}

} // namespace symbols
} // namespace dandy
