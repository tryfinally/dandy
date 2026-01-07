#include "server/world.hpp"
#include "dandy/constants.hpp"
#include <algorithm>
#include <cmath>
#include <queue>

namespace dandy::server {

World::World(uint16_t width, uint16_t height, uint64_t seed)
    : width_(width)
    , height_(height)
    , terrain_(static_cast<size_t>(width) * height, Terrain::Ground)
    , entities_(static_cast<size_t>(width) * height, EntityType::Empty)
    , rng_(seed == 0 ? std::random_device{}() : seed)
{
}

Terrain World::terrain_at(int16_t x, int16_t y) const {
    if (!in_bounds(x, y)) {
        return Terrain::Wall;  // Out of bounds is impassable
    }
    return terrain_[index(x, y)];
}

bool World::in_bounds(int16_t x, int16_t y) const {
    return x >= 0 && x < static_cast<int16_t>(width_) &&
           y >= 0 && y < static_cast<int16_t>(height_);
}

bool World::is_passable(int16_t x, int16_t y) const {
    if (!in_bounds(x, y)) return false;
    Terrain t = terrain_at(x, y);
    return t != Terrain::Wall && t != Terrain::Water && t != Terrain::Mountain;
}

uint8_t World::movement_cost(Terrain t) const {
    switch (t) {
        case Terrain::Ground: return constants::GROUND_COST;
        case Terrain::Road:   return constants::ROAD_COST;
        case Terrain::Tree:   return constants::TREE_COST;
        case Terrain::Wall:   return constants::WALL_COST;
        case Terrain::Water:  return constants::WATER_COST;
        case Terrain::Mountain: return constants::MOUNTAIN_COST;
    }
    return 255;
}

bool World::has_line_of_sight(Coord from, Coord to) const {
    // Bresenham's line algorithm
    int dx = std::abs(to.x - from.x);
    int dy = -std::abs(to.y - from.y);
    int sx = from.x < to.x ? 1 : -1;
    int sy = from.y < to.y ? 1 : -1;
    int err = dx + dy;

    int x = from.x;
    int y = from.y;

    while (true) {
        if (x == to.x && y == to.y) return true;

        Terrain t = terrain_at(static_cast<int16_t>(x), static_cast<int16_t>(y));
        if (t == Terrain::Wall || t == Terrain::Mountain) {
            // Check if this is the starting position
            if (x != from.x || y != from.y) return false;
        }

        int e2 = 2 * err;
        if (e2 >= dy) {
            if (x == to.x) break;
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            if (y == to.y) break;
            err += dx;
            y += sy;
        }
    }
    return true;
}

std::optional<Coord> World::trace_projectile(Coord from, Coord to, int max_range) const {
    // Trace line and find first blocking obstacle
    int dx = std::abs(to.x - from.x);
    int dy = -std::abs(to.y - from.y);
    int sx = from.x < to.x ? 1 : -1;
    int sy = from.y < to.y ? 1 : -1;
    int err = dx + dy;

    int x = from.x;
    int y = from.y;
    int steps = 0;

    while (steps < max_range) {
        if (x == to.x && y == to.y) {
            return Coord{static_cast<int16_t>(x), static_cast<int16_t>(y)};
        }

        int e2 = 2 * err;
        if (e2 >= dy) {
            if (x == to.x) break;
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            if (y == to.y) break;
            err += dx;
            y += sy;
        }

        steps++;

        // Check for blocking terrain (skip starting position)
        if (steps > 0) {
            Terrain t = terrain_at(static_cast<int16_t>(x), static_cast<int16_t>(y));
            if (t == Terrain::Wall || t == Terrain::Mountain) {
                return Coord{static_cast<int16_t>(x), static_cast<int16_t>(y)};
            }
        }
    }

    return std::nullopt;  // Out of range
}

void World::spawn_resources(int count) {
    std::uniform_int_distribution<int> x_dist(5, width_ - 6);
    std::uniform_int_distribution<int> y_dist(5, height_ - 6);
    std::uniform_int_distribution<int> type_dist(0, 5);
    std::uniform_int_distribution<uint16_t> amount_dist(20, 50);

    resources_.clear();
    resources_.reserve(count);

    for (int i = 0; i < count; ++i) {
        Coord pos;
        int attempts = 0;
        do {
            pos = {static_cast<int16_t>(x_dist(rng_)), static_cast<int16_t>(y_dist(rng_))};
            attempts++;
        } while (!is_passable(pos.x, pos.y) && attempts < 100);

        if (attempts >= 100) continue;

        Resource res;
        res.position = pos;
        res.type = static_cast<ResourceType>(type_dist(rng_));
        res.amount = amount_dist(rng_);
        res.collected = false;

        // Set entity on map
        switch (res.type) {
            case ResourceType::Fuel: set_entity_at(pos, EntityType::FuelDepot); break;
            case ResourceType::Ammo: set_entity_at(pos, EntityType::AmmoDepot); break;
            case ResourceType::Health: set_entity_at(pos, EntityType::HealthPack); break;
            case ResourceType::RadarUpgrade: set_entity_at(pos, EntityType::RadarUpgrade); break;
            case ResourceType::ShieldBoost: set_entity_at(pos, EntityType::ShieldBoost); break;
            case ResourceType::SpeedBoost: set_entity_at(pos, EntityType::SpeedBoost); break;
        }

        resources_.push_back(res);
    }
}

std::optional<Resource*> World::resource_at(Coord pos) {
    for (auto& res : resources_) {
        if (!res.collected && res.position == pos) {
            return &res;
        }
    }
    return std::nullopt;
}

void World::collect_resource(Coord pos) {
    for (auto& res : resources_) {
        if (!res.collected && res.position == pos) {
            res.collected = true;
            clear_entity_at(pos);
            break;
        }
    }
}

void World::update_resources(TickNumber tick) {
    // Respawn collected resources after delay
    for (auto& res : resources_) {
        if (res.collected && res.respawn_tick > 0 && tick >= res.respawn_tick) {
            res.collected = false;
            // Set entity on map again
            switch (res.type) {
                case ResourceType::Fuel: set_entity_at(res.position, EntityType::FuelDepot); break;
                case ResourceType::Ammo: set_entity_at(res.position, EntityType::AmmoDepot); break;
                case ResourceType::Health: set_entity_at(res.position, EntityType::HealthPack); break;
                case ResourceType::RadarUpgrade: set_entity_at(res.position, EntityType::RadarUpgrade); break;
                case ResourceType::ShieldBoost: set_entity_at(res.position, EntityType::ShieldBoost); break;
                case ResourceType::SpeedBoost: set_entity_at(res.position, EntityType::SpeedBoost); break;
            }
        }
    }
}

void World::generate() {
    generate_terrain();
    add_obstacles();
    add_roads();
    spawn_resources(50);  // Default resource count
}

void World::generate_terrain() {
    // Fill with ground
    std::fill(terrain_.begin(), terrain_.end(), Terrain::Ground);

    // Add border walls
    for (int x = 0; x < width_; ++x) {
        terrain_[index(x, 0)] = Terrain::Wall;
        terrain_[index(x, height_ - 1)] = Terrain::Wall;
    }
    for (int y = 0; y < height_; ++y) {
        terrain_[index(0, y)] = Terrain::Wall;
        terrain_[index(width_ - 1, y)] = Terrain::Wall;
    }
}

void World::add_obstacles() {
    std::uniform_int_distribution<int> x_dist(10, width_ - 11);
    std::uniform_int_distribution<int> y_dist(10, height_ - 11);
    std::uniform_int_distribution<int> size_dist(2, 8);
    std::uniform_int_distribution<int> type_dist(0, 3);

    // Add random obstacle clusters
    int num_clusters = (width_ * height_) / 800;
    for (int i = 0; i < num_clusters; ++i) {
        int cx = x_dist(rng_);
        int cy = y_dist(rng_);
        int size = size_dist(rng_);
        int type = type_dist(rng_);

        Terrain t;
        switch (type) {
            case 0: t = Terrain::Wall; break;
            case 1: t = Terrain::Water; break;
            case 2: t = Terrain::Mountain; break;
            default: t = Terrain::Tree; break;
        }

        // Create cluster
        for (int dx = -size/2; dx <= size/2; ++dx) {
            for (int dy = -size/2; dy <= size/2; ++dy) {
                int x = cx + dx;
                int y = cy + dy;
                if (x > 1 && x < width_ - 2 && y > 1 && y < height_ - 2) {
                    // Random shape
                    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
                    if (prob(rng_) < 0.7f) {
                        terrain_[index(x, y)] = t;
                    }
                }
            }
        }
    }
}

void World::add_roads() {
    // Add some horizontal and vertical roads
    std::uniform_int_distribution<int> pos_dist_x(20, width_ - 21);
    std::uniform_int_distribution<int> pos_dist_y(20, height_ - 21);

    int num_roads = 3 + (width_ + height_) / 100;
    for (int i = 0; i < num_roads; ++i) {
        bool horizontal = (i % 2 == 0);
        if (horizontal) {
            int y = pos_dist_y(rng_);
            for (int x = 5; x < width_ - 5; ++x) {
                if (terrain_[index(x, y)] == Terrain::Ground) {
                    terrain_[index(x, y)] = Terrain::Road;
                }
            }
        } else {
            int x = pos_dist_x(rng_);
            for (int y = 5; y < height_ - 5; ++y) {
                if (terrain_[index(x, y)] == Terrain::Ground) {
                    terrain_[index(x, y)] = Terrain::Road;
                }
            }
        }
    }
}

MapView World::get_view(Coord center, uint8_t radius, TankId viewer) const {
    MapView view;
    view.viewer_tank = viewer;
    view.center = center;
    view.width = radius * 2 + 1;
    view.height = radius * 2 + 1;

    int start_x = center.x - radius;
    int start_y = center.y - radius;

    view.cells.reserve(view.width * view.height);

    for (int dy = 0; dy < view.height; ++dy) {
        for (int dx = 0; dx < view.width; ++dx) {
            int16_t x = static_cast<int16_t>(start_x + dx);
            int16_t y = static_cast<int16_t>(start_y + dy);

            Cell cell;
            cell.terrain = terrain_at(x, y);
            cell.entity = entity_at({x, y});
            view.cells.push_back(cell);
        }
    }

    return view;
}

void World::set_entity_at(Coord pos, EntityType entity) {
    if (in_bounds(pos.x, pos.y)) {
        entities_[index(pos.x, pos.y)] = entity;
    }
}

void World::clear_entity_at(Coord pos) {
    if (in_bounds(pos.x, pos.y)) {
        entities_[index(pos.x, pos.y)] = EntityType::Empty;
    }
}

EntityType World::entity_at(Coord pos) const {
    if (!in_bounds(pos.x, pos.y)) {
        return EntityType::Empty;
    }
    return entities_[index(pos.x, pos.y)];
}

std::optional<Coord> World::find_spawn_point() const {
    std::mt19937_64 spawn_rng(std::random_device{}());
    std::uniform_int_distribution<int> x_dist(10, width_ - 11);
    std::uniform_int_distribution<int> y_dist(10, height_ - 11);

    for (int attempts = 0; attempts < 100; ++attempts) {
        int16_t x = static_cast<int16_t>(x_dist(spawn_rng));
        int16_t y = static_cast<int16_t>(y_dist(spawn_rng));

        if (is_passable(x, y) && entity_at({x, y}) == EntityType::Empty) {
            return Coord{x, y};
        }
    }
    return std::nullopt;
}

} // namespace dandy::server
