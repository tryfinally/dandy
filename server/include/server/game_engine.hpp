#pragma once

#include "dandy/protocol.hpp"
#include "dandy/constants.hpp"
#include "server/world.hpp"
#include "server/tank.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>

namespace dandy::server {

class Session;
class TcpServer;

// Player data
struct Player {
    PlayerId id{0};
    std::string name;
    uint32_t score{0};
    std::vector<TankId> tanks;
    std::weak_ptr<Session> session;
    bool connected{true};
};

// Pending action from client
struct PendingAction {
    PlayerId player_id;
    std::unique_ptr<Message> message;
};

// Game engine - manages game state and logic
class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    // Lifecycle
    void start();
    void stop();
    [[nodiscard]] bool is_running() const { return running_.load(); }

    // Set network server reference
    void set_server(TcpServer* server) { tcp_server_ = server; }

    // Player management
    PlayerId add_player(const std::string& name, std::shared_ptr<Session> session);
    void remove_player(PlayerId id);
    Player* get_player(PlayerId id);

    // Tank management
    TankId place_tank(PlayerId player_id, Coord position, Direction direction);
    Tank* get_tank(TankId id);
    std::vector<Tank*> get_player_tanks(PlayerId player_id);

    // Queue action for processing
    void queue_action(PlayerId player_id, std::unique_ptr<Message> msg);

    // Direct action processing (called from network thread for immediate response)
    void handle_message(PlayerId player_id, std::unique_ptr<Message> msg);

    // Game state
    [[nodiscard]] const World& world() const { return *world_; }
    [[nodiscard]] GameInfo game_info() const;
    [[nodiscard]] TickNumber current_tick() const { return current_tick_; }

    // Scan operations
    std::vector<Contact> perform_scan(TankId tank_id, ScanType type, uint16_t arc = 90);

    // Send event to player
    void send_to_player(PlayerId player_id, const Message& msg);
    void broadcast(const Message& msg);

private:
    void game_loop();
    void process_tick();
    void process_pending_actions();
    void update_tanks();
    void update_projectiles();
    void check_collisions();

    // Action handlers
    void handle_move(PlayerId player_id, const MoveMsg& msg);
    void handle_rotate(PlayerId player_id, const RotateMsg& msg);
    void handle_fire(PlayerId player_id, const FireMsg& msg);
    void handle_scan_camera(PlayerId player_id, const ScanCameraMsg& msg);
    void handle_scan_ir(PlayerId player_id, const ScanIRMsg& msg);
    void handle_scan_radar(PlayerId player_id, const ScanRadarMsg& msg);
    void handle_collect(PlayerId player_id, const CollectResourceMsg& msg);
    void handle_get_map_view(PlayerId player_id, const GetMapViewMsg& msg);

    // Combat helpers
    void fire_projectile(Tank& shooter, Coord target);
    void resolve_hit(Projectile& proj, Tank& target);

    std::atomic<bool> running_{false};
    std::thread game_thread_;

    std::unique_ptr<World> world_;
    GameId game_id_{1};
    TickNumber current_tick_{0};
    GameStatus status_{GameStatus::Running};

    std::mutex players_mutex_;
    std::unordered_map<PlayerId, Player> players_;
    PlayerId next_player_id_{1};

    std::mutex tanks_mutex_;
    std::unordered_map<TankId, Tank> tanks_;
    TankId next_tank_id_{1000};

    std::mutex projectiles_mutex_;
    std::vector<Projectile> projectiles_;

    std::mutex actions_mutex_;
    std::queue<PendingAction> pending_actions_;

    TcpServer* tcp_server_{nullptr};
};

} // namespace dandy::server
