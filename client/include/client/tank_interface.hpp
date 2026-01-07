#pragma once

#include "dandy/types.hpp"
#include <vector>
#include <string>
#include <functional>

namespace dandy::client {

// Forward declarations
class GameClient;

// Tank AI plugin interface
// Plugin authors implement this interface
class ITankAI {
public:
    virtual ~ITankAI() = default;

    // Called once when the AI is loaded
    virtual void on_init(TankId tank_id) = 0;

    // Called each tick to decide actions
    // The AI should call methods on the controller to command the tank
    virtual void on_tick(TickNumber tick) = 0;

    // Event callbacks
    virtual void on_spawn(Coord position, Direction direction) = 0;
    virtual void on_hit(TankId attacker, uint16_t damage, uint16_t remaining_health) = 0;
    virtual void on_destroyed(TankId killer) = 0;
    virtual void on_scan_result(ScanType type, const std::vector<Contact>& contacts) = 0;
    virtual void on_fire_result(FireResult result, Coord hit_pos, TankId target, uint16_t damage) = 0;
    virtual void on_obstacle(Coord position, Terrain terrain) = 0;
    virtual void on_resource_collected(ResourceType type, uint16_t amount) = 0;
    virtual void on_map_update(const MapView& view) = 0;
};

// Tank controller interface - provided to AI for issuing commands
class ITankController {
public:
    virtual ~ITankController() = default;

    // Tank info
    [[nodiscard]] virtual TankId tank_id() const = 0;
    [[nodiscard]] virtual const TankStatus& status() const = 0;

    // Commands
    virtual void move(Direction direction, uint8_t speed = 2) = 0;
    virtual void rotate_turret(uint16_t angle) = 0;
    virtual void fire(Coord target) = 0;

    // Sensors
    virtual void scan_camera(uint16_t arc = 90) = 0;
    virtual void scan_ir() = 0;
    virtual void scan_radar() = 0;

    // Request map view
    virtual void request_map_view(uint8_t radius = 15) = 0;

    // Utility
    [[nodiscard]] virtual Coord position() const = 0;
    [[nodiscard]] virtual Direction direction() const = 0;
    [[nodiscard]] virtual bool is_alive() const = 0;
    [[nodiscard]] virtual bool can_fire() const = 0;
    [[nodiscard]] virtual bool can_move() const = 0;
};

// Plugin factory function type
using CreateTankAI = ITankAI* (*)(ITankController* controller);
using DestroyTankAI = void (*)(ITankAI* ai);

// Plugin info
struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
};

using GetPluginInfo = PluginInfo (*)();

} // namespace dandy::client

// Macros for plugin authors
#define DANDY_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))

#define DANDY_REGISTER_PLUGIN(ClassName, PluginName, Version, Author, Description) \
    DANDY_PLUGIN_EXPORT dandy::client::ITankAI* create_tank_ai(dandy::client::ITankController* controller) { \
        return new ClassName(controller); \
    } \
    DANDY_PLUGIN_EXPORT void destroy_tank_ai(dandy::client::ITankAI* ai) { \
        delete ai; \
    } \
    DANDY_PLUGIN_EXPORT dandy::client::PluginInfo get_plugin_info() { \
        return {PluginName, Version, Author, Description}; \
    }
