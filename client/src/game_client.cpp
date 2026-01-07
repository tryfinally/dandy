#include "client/game_client.hpp"
#include <iostream>

namespace dandy::client {

// TankController implementation
TankController::TankController(TankId id, GameClient* client)
    : tank_id_(id)
    , client_(client)
{
    status_.id = id;
}

void TankController::move(Direction direction, uint8_t speed) {
    client_->send_move(tank_id_, direction, speed);
}

void TankController::rotate_turret(uint16_t angle) {
    client_->send_rotate(tank_id_, angle);
}

void TankController::fire(Coord target) {
    client_->send_fire(tank_id_, target);
}

void TankController::scan_camera(uint16_t arc) {
    client_->send_scan_camera(tank_id_, arc);
}

void TankController::scan_ir() {
    client_->send_scan_ir(tank_id_);
}

void TankController::scan_radar() {
    client_->send_scan_radar(tank_id_);
}

void TankController::request_map_view(uint8_t radius) {
    client_->send_map_view_request(tank_id_, radius);
}

bool TankController::can_fire() const {
    return status_.ammo > 0 && !status_.is_reloading();
}

// GameClient implementation
GameClient::GameClient()
    : display_(DisplayMode::ASCII)
{
}

GameClient::~GameClient() {
    stop();
    disconnect();
}

bool GameClient::connect(const std::string& host, uint16_t port) {
    tcp_client_.set_message_handler([this](std::unique_ptr<Message> msg) {
        handle_message(std::move(msg));
    });

    return tcp_client_.connect(host, port);
}

void GameClient::disconnect() {
    tcp_client_.disconnect();
}

bool GameClient::is_connected() const {
    return tcp_client_.is_connected();
}

bool GameClient::authenticate(const std::string& name, const std::string& token) {
    player_name_ = name;
    AuthenticateMsg msg(name, token);
    tcp_client_.send(msg);
    return true;
}

bool GameClient::load_plugin(const std::filesystem::path& plugin_path) {
    loaded_plugin_ = plugin_loader_.load(plugin_path);
    return loaded_plugin_ != nullptr;
}

bool GameClient::place_tank(Coord position, Direction direction) {
    PlaceTankMsg msg(position, direction);
    tcp_client_.send(msg);
    return true;
}

TankController* GameClient::get_tank(TankId id) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tanks_.find(id);
    return it != tanks_.end() ? it->second.get() : nullptr;
}

void GameClient::send_move(TankId tank_id, Direction direction, uint8_t speed) {
    MoveMsg msg(tank_id, direction, speed);
    tcp_client_.send(msg);
}

void GameClient::send_rotate(TankId tank_id, uint16_t angle) {
    RotateMsg msg(tank_id, angle);
    tcp_client_.send(msg);
}

void GameClient::send_fire(TankId tank_id, Coord target) {
    FireMsg msg(tank_id, target);
    tcp_client_.send(msg);
}

void GameClient::send_scan_camera(TankId tank_id, uint16_t arc) {
    ScanCameraMsg msg(tank_id, arc);
    tcp_client_.send(msg);
}

void GameClient::send_scan_ir(TankId tank_id) {
    ScanIRMsg msg(tank_id);
    tcp_client_.send(msg);
}

void GameClient::send_scan_radar(TankId tank_id) {
    ScanRadarMsg msg(tank_id);
    tcp_client_.send(msg);
}

void GameClient::send_map_view_request(TankId tank_id, uint8_t radius) {
    GetMapViewMsg msg(tank_id, radius);
    tcp_client_.send(msg);
}

void GameClient::run() {
    if (running_.exchange(true)) return;

    game_thread_ = std::thread(&GameClient::game_loop, this);
}

void GameClient::stop() {
    running_.store(false);
    if (game_thread_.joinable()) {
        game_thread_.join();
    }
}

void GameClient::game_loop() {
    while (running_.load() && is_connected()) {
        // Tick all AI instances
        {
            std::lock_guard lock(tanks_mutex_);
            for (auto& [tank_id, ai] : tank_ais_) {
                ai->on_tick(current_tick_);
            }
        }

        // Request map updates for display
        if (!tank_ids_.empty()) {
            TankId first_tank = tank_ids_.front();
            send_map_view_request(first_tank, constants::DEFAULT_VIEW_RADIUS);
        }

        // Update display
        if (!last_map_view_.cells.empty()) {
            std::lock_guard lock(tanks_mutex_);
            if (!tanks_.empty()) {
                auto& first_tank = tanks_.begin()->second;
                display_.render(last_map_view_, first_tank->status());
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void GameClient::handle_message(std::unique_ptr<Message> msg) {
    switch (msg->type) {
        case MessageType::Welcome:
            handle_welcome(static_cast<WelcomeMsg&>(*msg));
            break;
        case MessageType::AuthResult:
            handle_auth_result(static_cast<AuthResultMsg&>(*msg));
            break;
        case MessageType::GameState:
            handle_game_state(static_cast<GameStateMsg&>(*msg));
            break;
        case MessageType::TankPlaced:
            handle_tank_placed(static_cast<TankPlacedMsg&>(*msg));
            break;
        case MessageType::TankStatusUpdate:
            handle_tank_status(static_cast<TankStatusUpdateMsg&>(*msg));
            break;
        case MessageType::ScanResult:
            handle_scan_result(static_cast<ScanResultMsg&>(*msg));
            break;
        case MessageType::MapViewResponse:
            handle_map_view(static_cast<MapViewResponseMsg&>(*msg));
            break;
        case MessageType::EventHit:
            handle_event_hit(static_cast<EventHitMsg&>(*msg));
            break;
        case MessageType::EventDestroyed:
            handle_event_destroyed(static_cast<EventDestroyedMsg&>(*msg));
            break;
        case MessageType::EventObstacle:
            handle_event_obstacle(static_cast<EventObstacleMsg&>(*msg));
            break;
        case MessageType::EventFireResult:
            handle_event_fire_result(static_cast<EventFireResultMsg&>(*msg));
            break;
        case MessageType::EventCollected:
            handle_event_collected(static_cast<EventCollectedMsg&>(*msg));
            break;
        case MessageType::EventTick:
            handle_event_tick(static_cast<EventTickMsg&>(*msg));
            break;
        case MessageType::ErrorResponse:
            handle_error(static_cast<ErrorResponseMsg&>(*msg));
            break;
        default:
            break;
    }
}

void GameClient::handle_welcome(const WelcomeMsg& msg) {
    display_.show_message("Connected to server v" + std::to_string(msg.version));
}

void GameClient::handle_auth_result(const AuthResultMsg& msg) {
    if (msg.success) {
        authenticated_ = true;
        player_id_ = msg.player_id;
        display_.show_message("Authenticated as player " + std::to_string(player_id_));
        display_.add_event(msg.message);
    } else {
        display_.show_message("Authentication failed: " + msg.message);
    }
}

void GameClient::handle_game_state(const GameStateMsg& msg) {
    game_info_ = msg.game;
    display_.show_message("Joined game " + std::to_string(game_info_.id) +
                         " (" + std::to_string(game_info_.players.size()) + " players)");
}

void GameClient::handle_tank_placed(const TankPlacedMsg& msg) {
    if (msg.success) {
        std::lock_guard lock(tanks_mutex_);

        // Create controller
        auto controller = std::make_unique<TankController>(msg.tank_id, this);
        TankController* ctrl_ptr = controller.get();
        tanks_[msg.tank_id] = std::move(controller);
        tank_ids_.push_back(msg.tank_id);

        // Create AI if plugin is loaded
        if (loaded_plugin_) {
            auto ai = std::unique_ptr<ITankAI>(loaded_plugin_->create_ai(ctrl_ptr));
            if (ai) {
                ai->on_init(msg.tank_id);
                ai->on_spawn(msg.position, Direction::North);
                tank_ais_[msg.tank_id] = std::move(ai);
            }
        }

        display_.add_event("Tank " + std::to_string(msg.tank_id) + " placed at (" +
                          std::to_string(msg.position.x) + ", " +
                          std::to_string(msg.position.y) + ")");
    } else {
        display_.add_event("Failed to place tank");
    }
}

void GameClient::handle_tank_status(const TankStatusUpdateMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tanks_.find(msg.status.id);
    if (it != tanks_.end()) {
        it->second->update_status(msg.status);
    }
}

void GameClient::handle_scan_result(const ScanResultMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tank_ais_.find(msg.tank_id);
    if (it != tank_ais_.end()) {
        it->second->on_scan_result(msg.scan_type, msg.contacts);
    }

    display_.add_event("Scan: " + std::to_string(msg.contacts.size()) + " contacts");
}

void GameClient::handle_map_view(const MapViewResponseMsg& msg) {
    last_map_view_ = msg.view;

    std::lock_guard lock(tanks_mutex_);
    auto it = tank_ais_.find(msg.view.viewer_tank);
    if (it != tank_ais_.end()) {
        it->second->on_map_update(msg.view);
    }
}

void GameClient::handle_event_hit(const EventHitMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tank_ais_.find(msg.tank_id);
    if (it != tank_ais_.end()) {
        it->second->on_hit(msg.attacker_id, msg.damage, msg.remaining_health);
    }

    display_.add_event("HIT! Tank " + std::to_string(msg.tank_id) +
                      " took " + std::to_string(msg.damage) + " damage");
}

void GameClient::handle_event_destroyed(const EventDestroyedMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tank_ais_.find(msg.tank_id);
    if (it != tank_ais_.end()) {
        it->second->on_destroyed(msg.killer_id);
    }

    display_.add_event("DESTROYED! Tank " + std::to_string(msg.tank_id));
}

void GameClient::handle_event_obstacle(const EventObstacleMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tank_ais_.find(msg.tank_id);
    if (it != tank_ais_.end()) {
        it->second->on_obstacle(msg.position, msg.terrain_type);
    }

    display_.add_event("Blocked by obstacle at (" +
                      std::to_string(msg.position.x) + ", " +
                      std::to_string(msg.position.y) + ")");
}

void GameClient::handle_event_fire_result(const EventFireResultMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tank_ais_.find(msg.tank_id);
    if (it != tank_ais_.end()) {
        it->second->on_fire_result(msg.result, msg.hit_position, msg.target_id, msg.damage);
    }

    std::string result_str;
    switch (msg.result) {
        case FireResult::Miss: result_str = "MISS"; break;
        case FireResult::HitTank: result_str = "HIT TANK"; break;
        case FireResult::HitObstacle: result_str = "HIT OBSTACLE"; break;
        case FireResult::OutOfRange: result_str = "OUT OF RANGE"; break;
        case FireResult::NoAmmo: result_str = "NO AMMO"; break;
        case FireResult::Reloading: result_str = "RELOADING"; break;
    }
    display_.add_event("Fire result: " + result_str);
}

void GameClient::handle_event_collected(const EventCollectedMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tank_ais_.find(msg.tank_id);
    if (it != tank_ais_.end()) {
        it->second->on_resource_collected(msg.resource_type, msg.amount);
    }

    std::string type_str;
    switch (msg.resource_type) {
        case ResourceType::Fuel: type_str = "Fuel"; break;
        case ResourceType::Ammo: type_str = "Ammo"; break;
        case ResourceType::Health: type_str = "Health"; break;
        case ResourceType::RadarUpgrade: type_str = "Radar Upgrade"; break;
        case ResourceType::ShieldBoost: type_str = "Shield"; break;
        case ResourceType::SpeedBoost: type_str = "Speed Boost"; break;
    }
    display_.add_event("Collected " + type_str + " +" + std::to_string(msg.amount));
}

void GameClient::handle_event_tick(const EventTickMsg& msg) {
    current_tick_ = msg.tick;
}

void GameClient::handle_error(const ErrorResponseMsg& msg) {
    display_.add_event("Error: " + msg.message);
}

} // namespace dandy::client
