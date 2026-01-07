#include "server/game_engine.hpp"
#include "dandy/constants.hpp"
#include <cmath>
#include <random>

namespace dandy::server {

void GameEngine::fire_projectile(Tank& shooter, Coord target) {
    if (!shooter.can_fire()) return;

    // Calculate damage based on distance
    double distance = shooter.position().distance_to(target);
    if (distance > constants::MAIN_GUN_RANGE) {
        // Out of range
        EventFireResultMsg result;
        result.tank_id = shooter.id();
        result.result = FireResult::OutOfRange;
        result.hit_position = target;
        send_to_player(shooter.owner(), result);
        return;
    }

    shooter.fire();

    // Calculate damage (more damage at closer range)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dmg_dist(constants::MIN_DAMAGE, constants::MAX_DAMAGE);
    uint16_t base_damage = dmg_dist(gen);
    double range_factor = 1.0 - (distance / constants::MAIN_GUN_RANGE) * 0.5;
    uint16_t damage = static_cast<uint16_t>(base_damage * range_factor);

    // Create projectile
    Projectile proj;
    proj.shooter_id = shooter.id();
    proj.shooter_owner = shooter.owner();
    proj.origin = shooter.position();
    proj.target = target;
    proj.current = shooter.position();
    proj.damage = damage;
    proj.fired_tick = current_tick_;
    proj.active = true;

    std::lock_guard lock(projectiles_mutex_);
    projectiles_.push_back(proj);
}

void GameEngine::resolve_hit(Projectile& proj, Tank& target) {
    target.take_damage(proj.damage);

    // Send hit event to target owner
    EventHitMsg hit_event;
    hit_event.tank_id = target.id();
    hit_event.attacker_id = proj.shooter_id;
    hit_event.damage = proj.damage;
    hit_event.hit_position = target.position();
    hit_event.remaining_health = target.health();
    send_to_player(target.owner(), hit_event);

    // Send fire result to shooter
    EventFireResultMsg fire_result;
    fire_result.tank_id = proj.shooter_id;
    fire_result.result = FireResult::HitTank;
    fire_result.hit_position = target.position();
    fire_result.target_id = target.id();
    fire_result.damage = proj.damage;
    send_to_player(proj.shooter_owner, fire_result);

    // Award points for hit
    {
        std::lock_guard lock(tanks_mutex_);
        auto it = tanks_.find(proj.shooter_id);
        if (it != tanks_.end()) {
            it->second.add_score(constants::POINTS_PER_HIT);
        }
    }

    // Check for destruction
    if (!target.is_alive()) {
        // Send destroyed event
        EventDestroyedMsg destroyed;
        destroyed.tank_id = target.id();
        destroyed.killer_id = proj.shooter_id;
        destroyed.points = constants::POINTS_PER_KILL;
        broadcast(destroyed);

        // Award kill points
        {
            std::lock_guard lock(tanks_mutex_);
            auto it = tanks_.find(proj.shooter_id);
            if (it != tanks_.end()) {
                it->second.add_score(constants::POINTS_PER_KILL);
            }
        }

        // Update player score
        {
            std::lock_guard lock(players_mutex_);
            auto pit = players_.find(proj.shooter_owner);
            if (pit != players_.end()) {
                pit->second.score += constants::POINTS_PER_KILL;
            }
        }

        // Clear tank from world
        world_->clear_entity_at(target.position());
    }

    proj.active = false;
}

void GameEngine::update_projectiles() {
    std::lock_guard proj_lock(projectiles_mutex_);
    std::lock_guard tank_lock(tanks_mutex_);

    for (auto& proj : projectiles_) {
        if (!proj.active) continue;

        // Trace projectile path
        auto hit_pos = world_->trace_projectile(proj.origin, proj.target, constants::MAIN_GUN_RANGE);

        if (hit_pos) {
            // Check if hit a tank
            bool hit_tank = false;
            for (auto& [tank_id, tank] : tanks_) {
                if (!tank.is_alive()) continue;
                if (tank.id() == proj.shooter_id) continue;  // Don't hit self
                if (tank.position() == *hit_pos) {
                    resolve_hit(proj, tank);
                    hit_tank = true;
                    break;
                }
            }

            if (!hit_tank) {
                // Hit obstacle
                EventFireResultMsg fire_result;
                fire_result.tank_id = proj.shooter_id;
                fire_result.result = FireResult::HitObstacle;
                fire_result.hit_position = *hit_pos;
                fire_result.target_id = 0;
                fire_result.damage = 0;
                send_to_player(proj.shooter_owner, fire_result);
                proj.active = false;
            }
        } else {
            // Projectile reached target without hitting anything - check for tank at target
            for (auto& [tank_id, tank] : tanks_) {
                if (!tank.is_alive()) continue;
                if (tank.id() == proj.shooter_id) continue;
                // Check if target is close to tank position
                if (tank.position().manhattan_distance(proj.target) <= 1) {
                    resolve_hit(proj, tank);
                    break;
                }
            }

            if (proj.active) {
                // Miss
                EventFireResultMsg fire_result;
                fire_result.tank_id = proj.shooter_id;
                fire_result.result = FireResult::Miss;
                fire_result.hit_position = proj.target;
                fire_result.target_id = 0;
                fire_result.damage = 0;
                send_to_player(proj.shooter_owner, fire_result);
                proj.active = false;
            }
        }
    }

    // Remove inactive projectiles
    std::erase_if(projectiles_, [](const Projectile& p) { return !p.active; });
}

std::vector<Contact> GameEngine::perform_scan(TankId tank_id, ScanType type, uint16_t arc) {
    std::vector<Contact> contacts;

    Tank* scanner = get_tank(tank_id);
    if (!scanner || !scanner->is_alive()) return contacts;

    int range = 0;
    switch (type) {
        case ScanType::Camera:
            range = constants::CAMERA_RANGE;
            break;
        case ScanType::Infrared:
            range = constants::IR_RANGE;
            arc = 360;  // Full circle
            break;
        case ScanType::Radar:
            range = scanner->radar_range();
            arc = 360;  // Full circle
            break;
    }

    Coord scanner_pos = scanner->position();
    uint16_t facing = scanner->turret_angle();

    std::lock_guard lock(tanks_mutex_);
    for (const auto& [tid, tank] : tanks_) {
        if (tid == tank_id) continue;  // Don't detect self
        if (!tank.is_alive()) continue;

        Coord target_pos = tank.position();
        double distance = scanner_pos.distance_to(target_pos);

        if (distance > range) continue;

        // Check arc (for camera)
        if (arc < 360) {
            double dx = target_pos.x - scanner_pos.x;
            double dy = target_pos.y - scanner_pos.y;
            double angle = std::atan2(dy, dx) * 180.0 / M_PI;
            if (angle < 0) angle += 360;

            double diff = std::abs(angle - facing);
            if (diff > 180) diff = 360 - diff;
            if (diff > arc / 2) continue;
        }

        // Check line of sight for camera
        if (type == ScanType::Camera) {
            if (!world_->has_line_of_sight(scanner_pos, target_pos)) continue;
        }

        Contact contact;
        contact.position = target_pos;
        contact.type = (tank.owner() == scanner->owner())
                       ? EntityType::TankFriendly
                       : EntityType::TankEnemy;
        contact.flags = 0;
        if (tank.status().is_moving()) contact.flags |= 0x01;
        if (tank.owner() == scanner->owner()) contact.flags |= 0x02;
        if (tank.status().is_damaged()) contact.flags |= 0x04;

        contacts.push_back(contact);
    }

    // Also detect resources with radar
    if (type == ScanType::Radar) {
        for (int dx = -range; dx <= range; ++dx) {
            for (int dy = -range; dy <= range; ++dy) {
                Coord pos{static_cast<int16_t>(scanner_pos.x + dx),
                          static_cast<int16_t>(scanner_pos.y + dy)};
                if (scanner_pos.distance_to(pos) > range) continue;

                auto resource = world_->resource_at(pos);
                if (resource) {
                    Contact contact;
                    contact.position = pos;
                    switch ((*resource)->type) {
                        case ResourceType::Fuel: contact.type = EntityType::FuelDepot; break;
                        case ResourceType::Ammo: contact.type = EntityType::AmmoDepot; break;
                        case ResourceType::Health: contact.type = EntityType::HealthPack; break;
                        case ResourceType::RadarUpgrade: contact.type = EntityType::RadarUpgrade; break;
                        case ResourceType::ShieldBoost: contact.type = EntityType::ShieldBoost; break;
                        case ResourceType::SpeedBoost: contact.type = EntityType::SpeedBoost; break;
                    }
                    contact.flags = 0;
                    contacts.push_back(contact);
                }
            }
        }
    }

    // Start cooldown
    scanner->start_scan_cooldown(type);

    return contacts;
}

} // namespace dandy::server
