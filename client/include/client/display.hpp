#pragma once

#include "dandy/types.hpp"
#include <string>
#include <vector>
#include <mutex>

namespace dandy::client {

// Display mode
enum class DisplayMode {
    ASCII,   // Simple ASCII characters
    Emoji    // Unicode emoji display
};

// Console display for the game
class Display {
public:
    explicit Display(DisplayMode mode = DisplayMode::ASCII);

    // Set display mode
    void set_mode(DisplayMode mode) { mode_ = mode; }

    // Clear screen
    void clear();

    // Render map view
    void render_map(const MapView& view, const TankStatus& status);

    // Render status bar
    void render_status(const TankStatus& status);

    // Render event log
    void add_event(const std::string& event);
    void render_events();

    // Render full UI
    void render(const MapView& view, const TankStatus& status);

    // Show message
    void show_message(const std::string& message);

    // Get terminal size
    [[nodiscard]] std::pair<int, int> terminal_size() const;

private:
    std::string cell_to_string(const Cell& cell) const;
    std::string terrain_string(Terrain t) const;
    std::string entity_string(EntityType e) const;
    std::string direction_arrow(Direction d) const;
    std::string health_bar(uint16_t current, uint16_t max, int width) const;

    DisplayMode mode_;
    std::vector<std::string> event_log_;
    static constexpr size_t MAX_EVENTS = 10;
    std::mutex events_mutex_;
};

} // namespace dandy::client
