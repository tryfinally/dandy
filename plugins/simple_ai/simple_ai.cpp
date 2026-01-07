#include "client/tank_interface.hpp"
#include <random>
#include <algorithm>
#include <cmath>

namespace {

using namespace dandy;
using namespace dandy::client;

// Simple AI: Basic survival and opportunistic combat
// Strategy:
// 1. Regularly scan surroundings
// 2. Move towards resources when low
// 3. Attack enemies in range
// 4. Avoid getting stuck

class SimpleAI : public ITankAI {
public:
    explicit SimpleAI(ITankController* controller)
        : controller_(controller)
        , rng_(std::random_device{}())
    {
    }

    void on_init(TankId tank_id) override {
        tank_id_ = tank_id;
    }

    void on_spawn(Coord position, Direction direction) override {
        current_direction_ = direction;
        // Initial radar scan
        controller_->scan_radar();
    }

    void on_tick(TickNumber tick) override {
        if (!controller_->is_alive()) return;

        tick_count_++;

        // Periodic radar scan
        if (tick_count_ % 60 == 0) {
            controller_->scan_radar();
            return;
        }

        // Periodic map view request
        if (tick_count_ % 30 == 0) {
            controller_->request_map_view(15);
            return;
        }

        const auto& status = controller_->status();

        // If we have a target, try to attack
        if (has_target_ && controller_->can_fire()) {
            controller_->fire(target_position_);
            has_target_ = false;
            return;
        }

        // If low on resources, seek them out
        if (status.fuel < 30 && has_resource_target_) {
            move_towards(resource_position_);
            return;
        }

        // Random exploration
        if (tick_count_ % 10 == 0) {
            std::uniform_int_distribution<int> dir_dist(0, 7);
            Direction new_dir = static_cast<Direction>(dir_dist(rng_));
            controller_->move(new_dir, 2);
            current_direction_ = new_dir;
        }
    }

    void on_hit(TankId attacker, uint16_t damage, uint16_t remaining_health) override {
        // When hit, try to retaliate
        if (last_enemy_position_.x != 0 || last_enemy_position_.y != 0) {
            target_position_ = last_enemy_position_;
            has_target_ = true;
        }

        // Also try to move away
        Direction escape = opposite_direction(current_direction_);
        controller_->move(escape, 3);
        current_direction_ = escape;
    }

    void on_destroyed(TankId killer) override {
        // Nothing to do when destroyed
    }

    void on_scan_result(ScanType type, const std::vector<Contact>& contacts) override {
        // Process contacts
        double closest_enemy_dist = 9999.0;
        double closest_resource_dist = 9999.0;
        Coord my_pos = controller_->position();

        for (const auto& contact : contacts) {
            double dist = my_pos.distance_to(contact.position);

            // Track enemies
            if (contact.type == EntityType::TankEnemy) {
                if (dist < closest_enemy_dist) {
                    closest_enemy_dist = dist;
                    last_enemy_position_ = contact.position;

                    // If in range, set as target
                    if (dist <= 20) {
                        target_position_ = contact.position;
                        has_target_ = true;
                    }
                }
            }

            // Track resources
            if (contact.type == EntityType::FuelDepot ||
                contact.type == EntityType::AmmoDepot ||
                contact.type == EntityType::HealthPack) {

                if (dist < closest_resource_dist) {
                    closest_resource_dist = dist;
                    resource_position_ = contact.position;
                    has_resource_target_ = true;
                }
            }
        }
    }

    void on_fire_result(FireResult result, Coord hit_pos, TankId target, uint16_t damage) override {
        // If we hit, keep attacking same position
        if (result == FireResult::HitTank) {
            target_position_ = hit_pos;
            has_target_ = true;
        }
    }

    void on_obstacle(Coord position, Terrain terrain) override {
        // When blocked, turn and try another direction
        std::uniform_int_distribution<int> turn(-2, 2);
        int new_dir = (static_cast<int>(current_direction_) + turn(rng_) + 8) % 8;
        current_direction_ = static_cast<Direction>(new_dir);
        controller_->move(current_direction_, 2);
    }

    void on_resource_collected(ResourceType type, uint16_t amount) override {
        has_resource_target_ = false;
    }

    void on_map_update(const MapView& view) override {
        // Could analyze map for pathfinding, but keeping simple
    }

private:
    void move_towards(Coord target) {
        Coord my_pos = controller_->position();
        int dx = target.x - my_pos.x;
        int dy = target.y - my_pos.y;

        Direction dir;
        if (std::abs(dx) > std::abs(dy)) {
            dir = dx > 0 ? Direction::East : Direction::West;
        } else {
            dir = dy > 0 ? Direction::South : Direction::North;
        }

        controller_->move(dir, 2);
        current_direction_ = dir;
    }

    Direction opposite_direction(Direction d) {
        return static_cast<Direction>((static_cast<int>(d) + 4) % 8);
    }

    ITankController* controller_;
    TankId tank_id_{0};
    std::mt19937 rng_;
    uint32_t tick_count_{0};
    Direction current_direction_{Direction::North};

    bool has_target_{false};
    Coord target_position_{0, 0};
    Coord last_enemy_position_{0, 0};

    bool has_resource_target_{false};
    Coord resource_position_{0, 0};
};

} // anonymous namespace

// Plugin registration
DANDY_REGISTER_PLUGIN(
    SimpleAI,
    "Simple AI",
    "1.0",
    "Dandy Team",
    "Basic survival and opportunistic combat AI"
)
