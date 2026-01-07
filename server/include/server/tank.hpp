#pragma once

#include "dandy/types.hpp"
#include "dandy/constants.hpp"
#include <vector>

namespace dandy::server {

class World;

// Tank entity
class Tank {
public:
    Tank(TankId id, PlayerId owner, Coord position, Direction direction);

    // Getters
    [[nodiscard]] TankId id() const { return status_.id; }
    [[nodiscard]] PlayerId owner() const { return status_.owner; }
    [[nodiscard]] Coord position() const { return status_.position; }
    [[nodiscard]] Direction direction() const { return status_.direction; }
    [[nodiscard]] uint16_t turret_angle() const { return status_.turret_angle; }
    [[nodiscard]] uint16_t health() const { return status_.health; }
    [[nodiscard]] uint16_t fuel() const { return status_.fuel; }
    [[nodiscard]] uint16_t ammo() const { return status_.ammo; }
    [[nodiscard]] bool is_alive() const { return status_.is_alive(); }
    [[nodiscard]] const TankStatus& status() const { return status_; }

    // Movement
    bool move(Direction dir, uint8_t speed, World& world);
    void rotate_turret(uint16_t angle);
    void set_direction(Direction dir) { status_.direction = dir; }

    // Combat
    [[nodiscard]] bool can_fire() const;
    void fire();  // Consume ammo and start cooldown
    void take_damage(uint16_t damage);
    void destroy();

    // Resources
    void add_fuel(uint16_t amount);
    void add_ammo(uint16_t amount);
    void add_health(uint16_t amount);
    void apply_upgrade(ResourceType type);

    // Sensors
    [[nodiscard]] bool can_scan(ScanType type) const;
    void start_scan_cooldown(ScanType type);
    [[nodiscard]] uint8_t radar_range() const;

    // Update (called each tick)
    void update(TickNumber tick);

    // Score
    void add_score(uint32_t points) { status_.score += points; }

private:
    TankStatus status_;
    TickNumber last_move_tick_{0};
    TickNumber reload_end_tick_{0};
    TickNumber camera_cooldown_end_{0};
    TickNumber ir_cooldown_end_{0};
    TickNumber radar_cooldown_end_{0};
    TickNumber shield_end_tick_{0};
    TickNumber speed_boost_end_tick_{0};
    TickNumber current_tick_{0};
};

// Projectile in flight
struct Projectile {
    TankId shooter_id;
    PlayerId shooter_owner;
    Coord origin;
    Coord target;
    Coord current;
    uint16_t damage;
    TickNumber fired_tick;
    bool active{true};
};

} // namespace dandy::server
