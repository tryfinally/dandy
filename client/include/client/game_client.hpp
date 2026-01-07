#pragma once

#include "client/tcp_client.hpp"
#include "client/display.hpp"
#include "client/plugin_loader.hpp"
#include "client/tank_interface.hpp"
#include "dandy/protocol.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace dandy::client {

// Tank controller implementation
class TankController : public ITankController {
public:
    TankController(TankId id, class GameClient* client);

    // ITankController interface
    [[nodiscard]] TankId tank_id() const override { return tank_id_; }
    [[nodiscard]] const TankStatus& status() const override { return status_; }
    void move(Direction direction, uint8_t speed = 2) override;
    void rotate_turret(uint16_t angle) override;
    void fire(Coord target) override;
    void scan_camera(uint16_t arc = 90) override;
    void scan_ir() override;
    void scan_radar() override;
    void request_map_view(uint8_t radius = 15) override;
    [[nodiscard]] Coord position() const override { return status_.position; }
    [[nodiscard]] Direction direction() const override { return status_.direction; }
    [[nodiscard]] bool is_alive() const override { return status_.is_alive(); }
    [[nodiscard]] bool can_fire() const override;
    [[nodiscard]] bool can_move() const override { return status_.fuel > 0; }

    // Update status
    void update_status(const TankStatus& status) { status_ = status; }

private:
    TankId tank_id_;
    TankStatus status_;
    GameClient* client_;
};

// Main game client
class GameClient {
public:
    GameClient();
    ~GameClient();

    // Connection
    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    [[nodiscard]] bool is_connected() const;

    // Authentication
    bool authenticate(const std::string& name, const std::string& token = "");
    [[nodiscard]] bool is_authenticated() const { return authenticated_; }
    [[nodiscard]] PlayerId player_id() const { return player_id_; }

    // Load AI plugin
    bool load_plugin(const std::filesystem::path& plugin_path);

    // Tank management
    bool place_tank(Coord position, Direction direction);
    [[nodiscard]] TankController* get_tank(TankId id);
    [[nodiscard]] const std::vector<TankId>& tank_ids() const { return tank_ids_; }

    // Send commands
    void send_move(TankId tank_id, Direction direction, uint8_t speed);
    void send_rotate(TankId tank_id, uint16_t angle);
    void send_fire(TankId tank_id, Coord target);
    void send_scan_camera(TankId tank_id, uint16_t arc);
    void send_scan_ir(TankId tank_id);
    void send_scan_radar(TankId tank_id);
    void send_map_view_request(TankId tank_id, uint8_t radius);

    // Run game loop
    void run();
    void stop();

    // Display access
    Display& display() { return display_; }

    // Game info
    [[nodiscard]] const GameInfo& game_info() const { return game_info_; }
    [[nodiscard]] const MapView& last_map_view() const { return last_map_view_; }

private:
    void handle_message(std::unique_ptr<Message> msg);
    void game_loop();

    // Message handlers
    void handle_welcome(const WelcomeMsg& msg);
    void handle_auth_result(const AuthResultMsg& msg);
    void handle_game_state(const GameStateMsg& msg);
    void handle_tank_placed(const TankPlacedMsg& msg);
    void handle_tank_status(const TankStatusUpdateMsg& msg);
    void handle_scan_result(const ScanResultMsg& msg);
    void handle_map_view(const MapViewResponseMsg& msg);
    void handle_event_hit(const EventHitMsg& msg);
    void handle_event_destroyed(const EventDestroyedMsg& msg);
    void handle_event_obstacle(const EventObstacleMsg& msg);
    void handle_event_fire_result(const EventFireResultMsg& msg);
    void handle_event_collected(const EventCollectedMsg& msg);
    void handle_event_tick(const EventTickMsg& msg);
    void handle_error(const ErrorResponseMsg& msg);

    TcpClient tcp_client_;
    Display display_;
    PluginLoader plugin_loader_;

    std::atomic<bool> running_{false};
    std::thread game_thread_;

    bool authenticated_{false};
    PlayerId player_id_{0};
    std::string player_name_;
    GameInfo game_info_;
    MapView last_map_view_;
    TickNumber current_tick_{0};

    std::mutex tanks_mutex_;
    std::vector<TankId> tank_ids_;
    std::unordered_map<TankId, std::unique_ptr<TankController>> tanks_;
    std::unordered_map<TankId, std::unique_ptr<ITankAI>> tank_ais_;

    std::unique_ptr<LoadedPlugin> loaded_plugin_;
};

} // namespace dandy::client
