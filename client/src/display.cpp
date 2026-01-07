#include "client/display.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>

namespace dandy::client {

Display::Display(DisplayMode mode)
    : mode_(mode)
{
}

void Display::clear() {
    // ANSI escape to clear screen and move cursor to top
    std::cout << "\033[2J\033[H" << std::flush;
}

void Display::render_map(const MapView& view, const TankStatus& status) {
    std::cout << "\n";

    // Top border
    std::cout << "+" << std::string(view.width * 2, '-') << "+\n";

    for (int y = 0; y < view.height; ++y) {
        std::cout << "|";
        for (int x = 0; x < view.width; ++x) {
            const Cell& cell = view.cells[y * view.width + x];

            // Check if this is the tank's position
            int world_x = view.center.x - view.width / 2 + x;
            int world_y = view.center.y - view.height / 2 + y;

            if (world_x == status.position.x && world_y == status.position.y) {
                // Render tank with direction indicator
                std::cout << direction_arrow(status.direction) << " ";
            } else {
                std::cout << cell_to_string(cell);
            }
        }
        std::cout << "|\n";
    }

    // Bottom border
    std::cout << "+" << std::string(view.width * 2, '-') << "+\n";
}

void Display::render_status(const TankStatus& status) {
    std::cout << "\n";
    std::cout << "=== Tank Status ===\n";
    std::cout << "Position: (" << status.position.x << ", " << status.position.y << ")  ";
    std::cout << "Direction: " << direction_arrow(status.direction) << "  ";
    std::cout << "Turret: " << status.turret_angle << "°\n";

    std::cout << "Health: " << health_bar(status.health, status.max_health, 20);
    std::cout << " " << status.health << "/" << status.max_health << "\n";

    std::cout << "Fuel:   " << health_bar(status.fuel, status.max_fuel, 20);
    std::cout << " " << status.fuel << "/" << status.max_fuel << "\n";

    std::cout << "Ammo:   " << status.ammo << "/" << status.max_ammo << "  ";
    std::cout << "Score: " << status.score << "\n";

    // Status flags
    std::cout << "Status: ";
    if (status.is_moving()) std::cout << "[MOVING] ";
    if (status.is_reloading()) std::cout << "[RELOADING] ";
    if (status.is_damaged()) std::cout << "[DAMAGED] ";
    if (status.is_shielded()) std::cout << "[SHIELDED] ";
    if (!status.is_alive()) std::cout << "[DESTROYED] ";
    std::cout << "\n";
}

void Display::add_event(const std::string& event) {
    std::lock_guard lock(events_mutex_);
    event_log_.push_back(event);
    if (event_log_.size() > MAX_EVENTS) {
        event_log_.erase(event_log_.begin());
    }
}

void Display::render_events() {
    std::lock_guard lock(events_mutex_);

    std::cout << "\n=== Events ===\n";
    for (const auto& event : event_log_) {
        std::cout << "  " << event << "\n";
    }
}

void Display::render(const MapView& view, const TankStatus& status) {
    clear();
    std::cout << "====== DANDY TANK ARENA ======\n";
    render_map(view, status);
    render_status(status);
    render_events();
    std::cout << std::flush;
}

void Display::show_message(const std::string& message) {
    std::cout << message << std::endl;
}

std::pair<int, int> Display::terminal_size() const {
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        return {ws.ws_col, ws.ws_row};
    }
    return {80, 24};  // Default
}

std::string Display::cell_to_string(const Cell& cell) const {
    // If there's an entity, show it, otherwise show terrain
    if (cell.entity != EntityType::Empty) {
        return entity_string(cell.entity);
    }
    return terrain_string(cell.terrain);
}

std::string Display::terrain_string(Terrain t) const {
    if (mode_ == DisplayMode::Emoji) {
        return symbols::terrain_to_emoji(t);
    }

    switch (t) {
        case Terrain::Ground:   return ". ";
        case Terrain::Wall:     return "##";
        case Terrain::Water:    return "~~";
        case Terrain::Mountain: return "^^";
        case Terrain::Tree:     return "TT";
        case Terrain::Road:     return "==";
    }
    return "??";
}

std::string Display::entity_string(EntityType e) const {
    if (mode_ == DisplayMode::Emoji) {
        const char* emoji = symbols::entity_to_emoji(e);
        return emoji ? emoji : "  ";
    }

    switch (e) {
        case EntityType::Empty:        return "  ";
        case EntityType::TankSelf:     return "@@";
        case EntityType::TankEnemy:    return "**";
        case EntityType::TankFriendly: return "&&";
        case EntityType::Projectile:   return "oo";
        case EntityType::FuelDepot:    return "FF";
        case EntityType::AmmoDepot:    return "AA";
        case EntityType::HealthPack:   return "++";
        case EntityType::RadarUpgrade: return "RR";
        case EntityType::ShieldBoost:  return "SS";
        case EntityType::SpeedBoost:   return ">>";
        case EntityType::Explosion:    return "XX";
    }
    return "??";
}

std::string Display::direction_arrow(Direction d) const {
    switch (d) {
        case Direction::North:     return "^";
        case Direction::NorthEast: return "/";
        case Direction::East:      return ">";
        case Direction::SouthEast: return "\\";
        case Direction::South:     return "v";
        case Direction::SouthWest: return "/";
        case Direction::West:      return "<";
        case Direction::NorthWest: return "\\";
    }
    return "?";
}

std::string Display::health_bar(uint16_t current, uint16_t max, int width) const {
    std::ostringstream ss;
    ss << "[";

    float percent = max > 0 ? static_cast<float>(current) / max : 0.0f;
    int filled = static_cast<int>(percent * width);

    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            ss << "=";
        } else {
            ss << " ";
        }
    }

    ss << "]";
    return ss.str();
}

} // namespace dandy::client
