#include "server/game_engine.hpp"
#include "server/session.hpp"
#include "server/tcp_server.hpp"
#include <iostream>
#include <chrono>

namespace dandy::server {

GameEngine::GameEngine()
    : world_(std::make_unique<World>(constants::DEFAULT_MAP_WIDTH,
                                     constants::DEFAULT_MAP_HEIGHT))
{
    world_->generate();
}

GameEngine::~GameEngine() {
    stop();
}

void GameEngine::start() {
    if (running_.exchange(true)) return;

    game_thread_ = std::thread(&GameEngine::game_loop, this);
    std::cout << "Game engine started" << std::endl;
}

void GameEngine::stop() {
    if (!running_.exchange(false)) return;

    if (game_thread_.joinable()) {
        game_thread_.join();
    }
    std::cout << "Game engine stopped" << std::endl;
}

PlayerId GameEngine::add_player(const std::string& name, std::shared_ptr<Session> session) {
    std::lock_guard lock(players_mutex_);

    PlayerId id = next_player_id_++;
    Player player;
    player.id = id;
    player.name = name;
    player.score = 0;
    player.session = session;
    player.connected = true;

    players_[id] = std::move(player);

    std::cout << "Player " << name << " (ID: " << id << ") joined" << std::endl;
    return id;
}

void GameEngine::remove_player(PlayerId id) {
    std::lock_guard lock(players_mutex_);

    auto it = players_.find(id);
    if (it != players_.end()) {
        it->second.connected = false;
        std::cout << "Player " << it->second.name << " (ID: " << id << ") disconnected" << std::endl;
    }
}

Player* GameEngine::get_player(PlayerId id) {
    std::lock_guard lock(players_mutex_);
    auto it = players_.find(id);
    return it != players_.end() ? &it->second : nullptr;
}

TankId GameEngine::place_tank(PlayerId player_id, Coord position, Direction direction) {
    // Verify position is valid
    if (!world_->is_passable(position.x, position.y)) {
        return 0;
    }
    if (world_->entity_at(position) != EntityType::Empty) {
        return 0;
    }

    std::lock_guard tank_lock(tanks_mutex_);
    std::lock_guard player_lock(players_mutex_);

    // Check player tank limit
    auto pit = players_.find(player_id);
    if (pit == players_.end()) return 0;
    if (pit->second.tanks.size() >= constants::MAX_TANKS_PER_PLAYER) {
        return 0;
    }

    TankId id = next_tank_id_++;
    tanks_.emplace(id, Tank(id, player_id, position, direction));

    pit->second.tanks.push_back(id);
    world_->set_entity_at(position, EntityType::TankSelf);

    std::cout << "Tank " << id << " placed at (" << position.x << ", " << position.y
              << ") for player " << player_id << std::endl;

    // Broadcast spawn event
    EventTankSpawnedMsg spawn;
    spawn.tank_id = id;
    spawn.owner_id = player_id;
    spawn.position = position;
    broadcast(spawn);

    return id;
}

Tank* GameEngine::get_tank(TankId id) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tanks_.find(id);
    return it != tanks_.end() ? &it->second : nullptr;
}

std::vector<Tank*> GameEngine::get_player_tanks(PlayerId player_id) {
    std::vector<Tank*> result;
    std::lock_guard tank_lock(tanks_mutex_);
    std::lock_guard player_lock(players_mutex_);

    auto pit = players_.find(player_id);
    if (pit == players_.end()) return result;

    for (TankId tid : pit->second.tanks) {
        auto it = tanks_.find(tid);
        if (it != tanks_.end() && it->second.is_alive()) {
            result.push_back(&it->second);
        }
    }
    return result;
}

void GameEngine::queue_action(PlayerId player_id, std::unique_ptr<Message> msg) {
    std::lock_guard lock(actions_mutex_);
    pending_actions_.push(PendingAction{player_id, std::move(msg)});
}

void GameEngine::handle_message(PlayerId player_id, std::unique_ptr<Message> msg) {
    switch (msg->type) {
        case MessageType::PlaceTank: {
            auto* place = static_cast<PlaceTankMsg*>(msg.get());
            TankId tid = place_tank(player_id, place->position, place->direction);

            TankPlacedMsg response;
            response.tank_id = tid;
            response.position = place->position;
            response.success = (tid != 0);
            response.error = tid != 0 ? ErrorCode::Success : ErrorCode::PositionOccupied;
            send_to_player(player_id, response);
            break;
        }

        case MessageType::Move: {
            auto* move = static_cast<MoveMsg*>(msg.get());
            handle_move(player_id, *move);
            break;
        }

        case MessageType::Rotate: {
            auto* rotate = static_cast<RotateMsg*>(msg.get());
            handle_rotate(player_id, *rotate);
            break;
        }

        case MessageType::Fire: {
            auto* fire = static_cast<FireMsg*>(msg.get());
            handle_fire(player_id, *fire);
            break;
        }

        case MessageType::ScanCamera: {
            auto* scan = static_cast<ScanCameraMsg*>(msg.get());
            handle_scan_camera(player_id, *scan);
            break;
        }

        case MessageType::ScanIR: {
            auto* scan = static_cast<ScanIRMsg*>(msg.get());
            handle_scan_ir(player_id, *scan);
            break;
        }

        case MessageType::ScanRadar: {
            auto* scan = static_cast<ScanRadarMsg*>(msg.get());
            handle_scan_radar(player_id, *scan);
            break;
        }

        case MessageType::CollectResource: {
            auto* collect = static_cast<CollectResourceMsg*>(msg.get());
            handle_collect(player_id, *collect);
            break;
        }

        case MessageType::GetMapView: {
            auto* view = static_cast<GetMapViewMsg*>(msg.get());
            handle_get_map_view(player_id, *view);
            break;
        }

        default:
            break;
    }
}

GameInfo GameEngine::game_info() const {
    GameInfo info;
    info.id = game_id_;
    info.status = status_;
    info.current_tick = current_tick_;
    info.map_width = world_->width();
    info.map_height = world_->height();

    std::lock_guard lock(const_cast<std::mutex&>(players_mutex_));
    for (const auto& [id, player] : players_) {
        if (!player.connected) continue;
        PlayerInfo pi;
        pi.id = player.id;
        pi.name = player.name;
        pi.score = player.score;
        pi.tank_count = static_cast<uint8_t>(player.tanks.size());
        info.players.push_back(pi);
    }
    return info;
}

void GameEngine::send_to_player(PlayerId player_id, const Message& msg) {
    if (tcp_server_) {
        tcp_server_->send_to_player(player_id, msg);
    }
}

void GameEngine::broadcast(const Message& msg) {
    if (tcp_server_) {
        tcp_server_->broadcast(msg);
    }
}

void GameEngine::game_loop() {
    using clock = std::chrono::steady_clock;
    auto next_tick = clock::now();

    while (running_.load()) {
        auto now = clock::now();
        if (now >= next_tick) {
            process_tick();
            next_tick += constants::TICK_DURATION;

            // If we're running behind, catch up
            if (clock::now() > next_tick) {
                next_tick = clock::now() + constants::TICK_DURATION;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void GameEngine::process_tick() {
    current_tick_++;

    process_pending_actions();
    update_tanks();
    update_projectiles();
    check_collisions();
    world_->update_resources(current_tick_);

    // Send tick event periodically
    if (current_tick_ % 60 == 0) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        EventTickMsg tick_msg(current_tick_, static_cast<uint64_t>(timestamp));
        broadcast(tick_msg);
    }
}

void GameEngine::process_pending_actions() {
    std::lock_guard lock(actions_mutex_);
    while (!pending_actions_.empty()) {
        auto action = std::move(pending_actions_.front());
        pending_actions_.pop();
        handle_message(action.player_id, std::move(action.message));
    }
}

void GameEngine::update_tanks() {
    std::lock_guard lock(tanks_mutex_);
    for (auto& [id, tank] : tanks_) {
        tank.update(current_tick_);
    }
}

void GameEngine::check_collisions() {
    // Resource collection check
    std::lock_guard tank_lock(tanks_mutex_);
    for (auto& [id, tank] : tanks_) {
        if (!tank.is_alive()) continue;

        auto resource = world_->resource_at(tank.position());
        if (!resource) continue;

        // Auto-collect resource
        Resource* res = *resource;
        switch (res->type) {
            case ResourceType::Fuel:
                tank.add_fuel(res->amount);
                break;
            case ResourceType::Ammo:
                tank.add_ammo(res->amount);
                break;
            case ResourceType::Health:
                tank.add_health(res->amount);
                break;
            default:
                tank.apply_upgrade(res->type);
                break;
        }

        // Send collected event
        EventCollectedMsg collected;
        collected.tank_id = tank.id();
        collected.resource_type = res->type;
        collected.amount = res->amount;
        collected.position = tank.position();
        send_to_player(tank.owner(), collected);

        tank.add_score(constants::POINTS_PER_RESOURCE);

        world_->collect_resource(tank.position());
        res->respawn_tick = current_tick_ + 60 * 30;  // 30 seconds
    }
}

// Action handlers
void GameEngine::handle_move(PlayerId player_id, const MoveMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tanks_.find(msg.tank_id);
    if (it == tanks_.end()) return;
    if (it->second.owner() != player_id) {
        ErrorResponseMsg err(ErrorCode::NotYourTank, "Not your tank");
        send_to_player(player_id, err);
        return;
    }

    Coord old_pos = it->second.position();
    bool moved = it->second.move(msg.direction, msg.speed, *world_);

    if (moved) {
        Coord new_pos = it->second.position();
        world_->set_entity_at(new_pos, EntityType::TankSelf);

        // Send move event
        EventTankMovedMsg move_event;
        move_event.tank_id = msg.tank_id;
        move_event.from = old_pos;
        move_event.to = new_pos;
        move_event.direction = msg.direction;
        broadcast(move_event);

        // Send status update
        TankStatusUpdateMsg status(it->second.status());
        send_to_player(player_id, status);
    } else {
        // Blocked
        Coord target = old_pos + direction_delta(msg.direction);
        EventObstacleMsg obstacle;
        obstacle.tank_id = msg.tank_id;
        obstacle.position = target;
        obstacle.terrain_type = world_->terrain_at(target);
        send_to_player(player_id, obstacle);
    }
}

void GameEngine::handle_rotate(PlayerId player_id, const RotateMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tanks_.find(msg.tank_id);
    if (it == tanks_.end()) return;
    if (it->second.owner() != player_id) return;

    it->second.rotate_turret(msg.angle);

    TankStatusUpdateMsg status(it->second.status());
    send_to_player(player_id, status);
}

void GameEngine::handle_fire(PlayerId player_id, const FireMsg& msg) {
    Tank* tank = nullptr;
    {
        std::lock_guard lock(tanks_mutex_);
        auto it = tanks_.find(msg.tank_id);
        if (it == tanks_.end()) return;
        if (it->second.owner() != player_id) return;
        tank = &it->second;
    }

    if (!tank->can_fire()) {
        if (tank->ammo() == 0) {
            EventFireResultMsg result;
            result.tank_id = msg.tank_id;
            result.result = FireResult::NoAmmo;
            send_to_player(player_id, result);
        } else {
            EventFireResultMsg result;
            result.tank_id = msg.tank_id;
            result.result = FireResult::Reloading;
            send_to_player(player_id, result);
        }
        return;
    }

    fire_projectile(*tank, msg.target);
}

void GameEngine::handle_scan_camera(PlayerId player_id, const ScanCameraMsg& msg) {
    Tank* tank = nullptr;
    {
        std::lock_guard lock(tanks_mutex_);
        auto it = tanks_.find(msg.tank_id);
        if (it == tanks_.end()) return;
        if (it->second.owner() != player_id) return;
        tank = &it->second;
    }

    if (!tank->can_scan(ScanType::Camera)) {
        ErrorResponseMsg err(ErrorCode::OnCooldown, "Camera on cooldown");
        send_to_player(player_id, err);
        return;
    }

    auto contacts = perform_scan(msg.tank_id, ScanType::Camera, msg.arc);
    ScanResultMsg result(msg.tank_id, ScanType::Camera, std::move(contacts));
    send_to_player(player_id, result);
}

void GameEngine::handle_scan_ir(PlayerId player_id, const ScanIRMsg& msg) {
    Tank* tank = nullptr;
    {
        std::lock_guard lock(tanks_mutex_);
        auto it = tanks_.find(msg.tank_id);
        if (it == tanks_.end()) return;
        if (it->second.owner() != player_id) return;
        tank = &it->second;
    }

    if (!tank->can_scan(ScanType::Infrared)) {
        ErrorResponseMsg err(ErrorCode::OnCooldown, "IR on cooldown");
        send_to_player(player_id, err);
        return;
    }

    auto contacts = perform_scan(msg.tank_id, ScanType::Infrared);
    ScanResultMsg result(msg.tank_id, ScanType::Infrared, std::move(contacts));
    send_to_player(player_id, result);
}

void GameEngine::handle_scan_radar(PlayerId player_id, const ScanRadarMsg& msg) {
    Tank* tank = nullptr;
    {
        std::lock_guard lock(tanks_mutex_);
        auto it = tanks_.find(msg.tank_id);
        if (it == tanks_.end()) return;
        if (it->second.owner() != player_id) return;
        tank = &it->second;
    }

    if (!tank->can_scan(ScanType::Radar)) {
        ErrorResponseMsg err(ErrorCode::OnCooldown, "Radar on cooldown");
        send_to_player(player_id, err);
        return;
    }

    auto contacts = perform_scan(msg.tank_id, ScanType::Radar);
    ScanResultMsg result(msg.tank_id, ScanType::Radar, std::move(contacts));
    send_to_player(player_id, result);
}

void GameEngine::handle_collect(PlayerId player_id, const CollectResourceMsg& msg) {
    std::lock_guard lock(tanks_mutex_);
    auto it = tanks_.find(msg.tank_id);
    if (it == tanks_.end()) return;
    if (it->second.owner() != player_id) return;

    // Collection happens automatically in check_collisions
    // This is just to trigger an immediate check
}

void GameEngine::handle_get_map_view(PlayerId player_id, const GetMapViewMsg& msg) {
    Tank* tank = nullptr;
    {
        std::lock_guard lock(tanks_mutex_);
        auto it = tanks_.find(msg.tank_id);
        if (it == tanks_.end()) return;
        if (it->second.owner() != player_id) return;
        tank = &it->second;
    }

    MapView view = world_->get_view(tank->position(), msg.radius, msg.tank_id);
    MapViewResponseMsg response(std::move(view));
    send_to_player(player_id, response);
}

} // namespace dandy::server
