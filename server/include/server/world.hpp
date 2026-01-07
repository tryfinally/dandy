#pragma once

#include "dandy/types.hpp"
#include <vector>
#include <random>
#include <optional>
#include <unordered_map>
#include <functional>

namespace dandy::server {

// Resource spawn on map
struct Resource {
    Coord position;
    ResourceType type;
    uint16_t amount;
    bool collected{false};
    uint32_t respawn_tick{0};
};

// World/Map class
class World {
public:
    World(uint16_t width, uint16_t height, uint64_t seed = 0);

    // Map access
    [[nodiscard]] uint16_t width() const { return width_; }
    [[nodiscard]] uint16_t height() const { return height_; }
    [[nodiscard]] Terrain terrain_at(int16_t x, int16_t y) const;
    [[nodiscard]] Terrain terrain_at(Coord pos) const { return terrain_at(pos.x, pos.y); }

    // Bounds checking
    [[nodiscard]] bool in_bounds(int16_t x, int16_t y) const;
    [[nodiscard]] bool in_bounds(Coord pos) const { return in_bounds(pos.x, pos.y); }

    // Movement checks
    [[nodiscard]] bool is_passable(int16_t x, int16_t y) const;
    [[nodiscard]] bool is_passable(Coord pos) const { return is_passable(pos.x, pos.y); }
    [[nodiscard]] uint8_t movement_cost(Terrain t) const;

    // Line of sight / projectile path
    [[nodiscard]] bool has_line_of_sight(Coord from, Coord to) const;
    [[nodiscard]] std::optional<Coord> trace_projectile(Coord from, Coord to, int max_range) const;

    // Resource management
    void spawn_resources(int count);
    [[nodiscard]] std::optional<Resource*> resource_at(Coord pos);
    void collect_resource(Coord pos);
    void update_resources(TickNumber tick);

    // Map generation
    void generate();

    // Get view around a point
    [[nodiscard]] MapView get_view(Coord center, uint8_t radius, TankId viewer) const;

    // Entity position tracking (for map view)
    void set_entity_at(Coord pos, EntityType entity);
    void clear_entity_at(Coord pos);
    [[nodiscard]] EntityType entity_at(Coord pos) const;

    // Find spawn point
    [[nodiscard]] std::optional<Coord> find_spawn_point() const;

private:
    uint16_t width_;
    uint16_t height_;
    std::vector<Terrain> terrain_;
    std::vector<EntityType> entities_;
    std::vector<Resource> resources_;
    std::mt19937_64 rng_;

    [[nodiscard]] size_t index(int16_t x, int16_t y) const {
        return static_cast<size_t>(y) * width_ + static_cast<size_t>(x);
    }

    void generate_terrain();
    void add_obstacles();
    void add_roads();
};

} // namespace dandy::server
