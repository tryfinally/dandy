#include "server/tank.hpp"
#include "server/world.hpp"

namespace dandy::server {

Tank::Tank(TankId id, PlayerId owner, Coord position, Direction direction)
{
    status_.id = id;
    status_.owner = owner;
    status_.position = position;
    status_.direction = direction;
    status_.turret_angle = static_cast<uint16_t>(direction) * 45;
    status_.health = constants::DEFAULT_TANK_HEALTH;
    status_.max_health = constants::DEFAULT_TANK_HEALTH;
    status_.fuel = constants::DEFAULT_TANK_FUEL;
    status_.max_fuel = constants::DEFAULT_TANK_FUEL;
    status_.ammo = constants::DEFAULT_TANK_AMMO;
    status_.max_ammo = constants::DEFAULT_TANK_AMMO;
    status_.speed = 0;
    status_.flags = 0;
    status_.score = 0;
}

bool Tank::move(Direction dir, uint8_t speed, World& world) {
    if (!is_alive()) return false;
    if (status_.fuel == 0) return false;
    if (speed > constants::MAX_SPEED) speed = constants::MAX_SPEED;

    // Get movement delta
    Coord delta = direction_delta(dir);
    Coord new_pos = status_.position + delta;

    // Check if passable
    if (!world.is_passable(new_pos.x, new_pos.y)) {
        return false;
    }

    // Check if occupied by another tank
    EntityType entity = world.entity_at(new_pos);
    if (entity == EntityType::TankSelf || entity == EntityType::TankEnemy ||
        entity == EntityType::TankFriendly) {
        return false;
    }

    // Update position
    world.clear_entity_at(status_.position);
    status_.position = new_pos;
    status_.direction = dir;
    status_.speed = speed;
    status_.flags |= 0x01;  // Moving flag

    // Consume fuel
    uint8_t cost = world.movement_cost(world.terrain_at(new_pos));
    if (status_.fuel >= cost) {
        status_.fuel -= cost;
    } else {
        status_.fuel = 0;
    }

    return true;
}

void Tank::rotate_turret(uint16_t angle) {
    status_.turret_angle = angle % 360;
}

bool Tank::can_fire() const {
    if (!is_alive()) return false;
    if (status_.ammo == 0) return false;
    if (current_tick_ < reload_end_tick_) return false;
    return true;
}

void Tank::fire() {
    if (!can_fire()) return;
    status_.ammo--;
    reload_end_tick_ = current_tick_ + constants::RELOAD_TICKS;
    status_.flags |= 0x02;  // Reloading flag
}

void Tank::take_damage(uint16_t damage) {
    if (!is_alive()) return;

    // Shield reduces damage
    if (status_.is_shielded()) {
        damage /= 2;
    }

    if (status_.health > damage) {
        status_.health -= damage;
        status_.flags |= 0x04;  // Damaged flag
    } else {
        status_.health = 0;
    }
}

void Tank::destroy() {
    status_.health = 0;
}

void Tank::add_fuel(uint16_t amount) {
    status_.fuel = std::min(static_cast<uint16_t>(status_.fuel + amount), status_.max_fuel);
}

void Tank::add_ammo(uint16_t amount) {
    status_.ammo = std::min(static_cast<uint16_t>(status_.ammo + amount), status_.max_ammo);
}

void Tank::add_health(uint16_t amount) {
    status_.health = std::min(static_cast<uint16_t>(status_.health + amount), status_.max_health);
    if (status_.health > status_.max_health / 2) {
        status_.flags &= ~0x04;  // Clear damaged flag
    }
}

void Tank::apply_upgrade(ResourceType type) {
    switch (type) {
        case ResourceType::RadarUpgrade:
            status_.radar_range_bonus += constants::RADAR_UPGRADE_BONUS;
            break;
        case ResourceType::ShieldBoost:
            shield_end_tick_ = current_tick_ + constants::SHIELD_DURATION_TICKS;
            status_.flags |= 0x08;  // Shielded flag
            break;
        case ResourceType::SpeedBoost:
            speed_boost_end_tick_ = current_tick_ + constants::SPEED_BOOST_DURATION_TICKS;
            status_.speed_bonus = 1;
            break;
        default:
            break;
    }
}

bool Tank::can_scan(ScanType type) const {
    switch (type) {
        case ScanType::Camera:
            return current_tick_ >= camera_cooldown_end_;
        case ScanType::Infrared:
            return current_tick_ >= ir_cooldown_end_;
        case ScanType::Radar:
            return current_tick_ >= radar_cooldown_end_;
    }
    return false;
}

void Tank::start_scan_cooldown(ScanType type) {
    switch (type) {
        case ScanType::Camera:
            camera_cooldown_end_ = current_tick_ + constants::CAMERA_COOLDOWN;
            break;
        case ScanType::Infrared:
            ir_cooldown_end_ = current_tick_ + constants::IR_COOLDOWN;
            break;
        case ScanType::Radar:
            radar_cooldown_end_ = current_tick_ + constants::RADAR_COOLDOWN;
            break;
    }
}

uint8_t Tank::radar_range() const {
    return constants::RADAR_RANGE + status_.radar_range_bonus;
}

void Tank::update(TickNumber tick) {
    current_tick_ = tick;

    // Clear moving flag (will be set again if moving)
    status_.speed = 0;
    status_.flags &= ~0x01;

    // Check reload cooldown
    if (tick >= reload_end_tick_) {
        status_.flags &= ~0x02;  // Clear reloading flag
    }

    // Check shield expiry
    if (shield_end_tick_ > 0 && tick >= shield_end_tick_) {
        status_.flags &= ~0x08;  // Clear shielded flag
        shield_end_tick_ = 0;
    }

    // Check speed boost expiry
    if (speed_boost_end_tick_ > 0 && tick >= speed_boost_end_tick_) {
        status_.speed_bonus = 0;
        speed_boost_end_tick_ = 0;
    }
}

} // namespace dandy::server
