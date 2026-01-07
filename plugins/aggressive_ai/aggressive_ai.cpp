#include "client/tank_interface.hpp"
#include <random>
#include <algorithm>
#include <cmath>
#include <queue>

namespace {

using namespace dandy;
using namespace dandy::client;

// Aggressive AI: Hunt and destroy enemies
// Strategy:
// 1. Actively search for enemies with constant scanning
// 2. Chase and engage enemies relentlessly
// 3. Only retreat when critically damaged
// 4. Use predictive targeting

class AggressiveAI : public ITankAI {
public:
    explicit AggressiveAI(ITankController* controller)
        : controller_(controller)
        , rng_(std::random_device{}())
    {
    }

    void on_init(TankId tank_id) override {
        tank_id_ = tank_id;
    }

    void on_spawn(Coord position, Direction direction) override {
        current_direction_ = direction;
        state_ = State::Hunting;
        // Immediate scan
        controller_->scan_radar();
    }

    void on_tick(TickNumber tick) override {
        if (!controller_->is_alive()) return;

        tick_count_++;
        const auto& status = controller_->status();

        // State machine
        switch (state_) {
            case State::Hunting:
                do_hunting(status);
                break;
            case State::Engaging:
                do_engaging(status);
                break;
            case State::Retreating:
                do_retreating(status);
                break;
        }

        // Regular scanning
        if (tick_count_ % 30 == 0) {
            controller_->scan_radar();
        }
    }

    void on_hit(TankId attacker, uint16_t damage, uint16_t remaining_health) override {
        // Track attacker
        attacker_id_ = attacker;

        // Check if we should retreat
        if (remaining_health < 25) {
            state_ = State::Retreating;
        } else {
            // Fight back!
            state_ = State::Engaging;
        }
    }

    void on_destroyed(TankId killer) override {
        // Rage
    }

    void on_scan_result(ScanType type, const std::vector<Contact>& contacts) override {
        Coord my_pos = controller_->position();

        // Find closest enemy
        double closest_dist = 9999.0;
        target_known_ = false;

        for (const auto& contact : contacts) {
            if (contact.type == EntityType::TankEnemy) {
                double dist = my_pos.distance_to(contact.position);
                if (dist < closest_dist) {
                    closest_dist = dist;
                    target_position_ = contact.position;
                    target_moving_ = contact.is_moving();
                    target_known_ = true;
                }
            }
        }

        // Found an enemy, switch to engaging
        if (target_known_) {
            state_ = State::Engaging;
            last_known_target_ = target_position_;
        }
    }

    void on_fire_result(FireResult result, Coord hit_pos, TankId target, uint16_t damage) override {
        if (result == FireResult::HitTank) {
            // Keep attacking!
            consecutive_hits_++;
        } else {
            consecutive_hits_ = 0;
            // Adjust aim slightly
            if (result == FireResult::Miss) {
                // Target might have moved, lead the shot next time
                lead_target_ = true;
            }
        }
    }

    void on_obstacle(Coord position, Terrain terrain) override {
        // Pathfind around obstacle
        std::uniform_int_distribution<int> turn(1, 3);
        int turn_dir = (rng_() % 2 == 0) ? 1 : -1;
        int new_dir = (static_cast<int>(current_direction_) + turn_dir * turn(rng_) + 8) % 8;
        current_direction_ = static_cast<Direction>(new_dir);
        controller_->move(current_direction_, 3);
    }

    void on_resource_collected(ResourceType type, uint16_t amount) override {
        // Only care about ammo when engaging
        if (type == ResourceType::Ammo) {
            // Good, more firepower
        }
    }

    void on_map_update(const MapView& view) override {
        // Could implement more sophisticated pathfinding
    }

private:
    enum class State {
        Hunting,    // Searching for enemies
        Engaging,   // Attacking an enemy
        Retreating  // Running away to heal
    };

    void do_hunting(const TankStatus& status) {
        // Spiral search pattern
        if (tick_count_ % 5 == 0) {
            search_direction_ = (search_direction_ + 1) % 8;
            controller_->move(static_cast<Direction>(search_direction_), 3);
            current_direction_ = static_cast<Direction>(search_direction_);
        }

        // Frequent scans while hunting
        if (tick_count_ % 15 == 0) {
            controller_->scan_radar();
        }
    }

    void do_engaging(const TankStatus& status) {
        if (!target_known_) {
            // Lost target, go to last known position
            if (last_known_target_.x != 0 || last_known_target_.y != 0) {
                move_towards(last_known_target_);
            } else {
                state_ = State::Hunting;
            }
            return;
        }

        Coord my_pos = controller_->position();
        double dist = my_pos.distance_to(target_position_);

        // If in range, FIRE!
        if (dist <= 20 && controller_->can_fire()) {
            Coord aim_pos = target_position_;

            // Lead the target if it's moving
            if (target_moving_ && lead_target_) {
                // Simple prediction: assume target continues in same direction
                // Add small offset in direction target was moving
                std::uniform_int_distribution<int> offset(-2, 2);
                aim_pos.x += static_cast<int16_t>(offset(rng_));
                aim_pos.y += static_cast<int16_t>(offset(rng_));
            }

            controller_->fire(aim_pos);

            // Don't just stand there, strafe!
            strafe();
        } else {
            // Close the distance aggressively
            move_towards(target_position_);
        }

        // Keep scanning to track target
        if (tick_count_ % 20 == 0) {
            controller_->scan_camera(90);
        }
    }

    void do_retreating(const TankStatus& status) {
        // If health is recovered enough, go back to hunting
        if (status.health > 50) {
            state_ = State::Hunting;
            return;
        }

        // Run away from last known target
        if (target_known_ || (last_known_target_.x != 0 || last_known_target_.y != 0)) {
            Coord escape_from = target_known_ ? target_position_ : last_known_target_;
            move_away_from(escape_from);
        } else {
            // Random retreat
            if (tick_count_ % 5 == 0) {
                std::uniform_int_distribution<int> dir(0, 7);
                current_direction_ = static_cast<Direction>(dir(rng_));
                controller_->move(current_direction_, 3);
            }
        }

        // Look for health packs
        if (tick_count_ % 30 == 0) {
            controller_->scan_radar();
        }
    }

    void move_towards(Coord target) {
        Coord my_pos = controller_->position();
        int dx = target.x - my_pos.x;
        int dy = target.y - my_pos.y;

        // Calculate direction
        Direction dir;
        if (dx > 0 && dy < 0) dir = Direction::NorthEast;
        else if (dx > 0 && dy > 0) dir = Direction::SouthEast;
        else if (dx < 0 && dy > 0) dir = Direction::SouthWest;
        else if (dx < 0 && dy < 0) dir = Direction::NorthWest;
        else if (dx > 0) dir = Direction::East;
        else if (dx < 0) dir = Direction::West;
        else if (dy > 0) dir = Direction::South;
        else dir = Direction::North;

        controller_->move(dir, 3);  // Full speed ahead!
        current_direction_ = dir;
    }

    void move_away_from(Coord threat) {
        Coord my_pos = controller_->position();
        int dx = my_pos.x - threat.x;
        int dy = my_pos.y - threat.y;

        Direction dir;
        if (dx > 0 && dy < 0) dir = Direction::NorthEast;
        else if (dx > 0 && dy > 0) dir = Direction::SouthEast;
        else if (dx < 0 && dy > 0) dir = Direction::SouthWest;
        else if (dx < 0 && dy < 0) dir = Direction::NorthWest;
        else if (dx > 0) dir = Direction::East;
        else if (dx < 0) dir = Direction::West;
        else if (dy > 0) dir = Direction::South;
        else dir = Direction::North;

        controller_->move(dir, 3);
        current_direction_ = dir;
    }

    void strafe() {
        // Move perpendicular to target direction
        int strafe_dir = (static_cast<int>(current_direction_) + (rng_() % 2 == 0 ? 2 : -2) + 8) % 8;
        controller_->move(static_cast<Direction>(strafe_dir), 2);
    }

    ITankController* controller_;
    TankId tank_id_{0};
    std::mt19937 rng_;
    uint32_t tick_count_{0};
    Direction current_direction_{Direction::North};
    State state_{State::Hunting};

    bool target_known_{false};
    Coord target_position_{0, 0};
    Coord last_known_target_{0, 0};
    bool target_moving_{false};
    bool lead_target_{false};
    int consecutive_hits_{0};

    TankId attacker_id_{0};
    int search_direction_{0};
};

} // anonymous namespace

// Plugin registration
DANDY_REGISTER_PLUGIN(
    AggressiveAI,
    "Aggressive AI",
    "1.0",
    "Dandy Team",
    "Hunt and destroy - aggressive combat AI that relentlessly pursues enemies"
)
