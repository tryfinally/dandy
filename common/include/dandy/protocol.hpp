#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <expected>
#include <memory>
#include "types.hpp"

namespace dandy {

// Message type IDs
enum class MessageType : uint16_t {
    // Client -> Server (0x0001 - 0x00FF)
    Authenticate = 0x0001,
    JoinGame = 0x0002,
    PlaceTank = 0x0003,
    Move = 0x0010,
    Rotate = 0x0011,
    Fire = 0x0012,
    ScanCamera = 0x0020,
    ScanIR = 0x0021,
    ScanRadar = 0x0022,
    CollectResource = 0x0030,
    GetMapView = 0x0040,
    Disconnect = 0x00FF,

    // Server -> Client (0x0100 - 0x01FF)
    Welcome = 0x0100,
    AuthResult = 0x0101,
    GameState = 0x0102,
    TankPlaced = 0x0103,
    ScanResult = 0x0110,
    MapViewResponse = 0x0111,
    TankStatusUpdate = 0x0112,
    ErrorResponse = 0x01FF,

    // Server -> Client Events (0x0200 - 0x02FF)
    EventHit = 0x0200,
    EventDestroyed = 0x0201,
    EventObstacle = 0x0202,
    EventFireResult = 0x0203,
    EventCollected = 0x0204,
    EventTankSpawned = 0x0205,
    EventTankMoved = 0x0206,
    EventGameOver = 0x0210,
    EventTick = 0x02FF,

    // Bidirectional (0x0300 - 0x03FF)
    Ping = 0x0300,
    Pong = 0x0301,
};

// Error codes
enum class ErrorCode : uint8_t {
    Success = 0x00,
    InvalidFormat = 0x01,
    AuthFailed = 0x02,
    GameNotFound = 0x03,
    GameFull = 0x04,
    InvalidCoords = 0x05,
    TankNotFound = 0x06,
    NotYourTank = 0x07,
    InsufficientResources = 0x08,
    OnCooldown = 0x09,
    InvalidTarget = 0x0A,
    PositionOccupied = 0x0B,
    Unknown = 0xFF
};

// Message frame header
struct MessageHeader {
    uint32_t length;  // Total payload size including type
    MessageType type;
};

constexpr size_t HEADER_SIZE = sizeof(uint32_t) + sizeof(uint16_t);

// Base message class
struct Message {
    MessageType type;

    virtual ~Message() = default;
    [[nodiscard]] virtual std::vector<uint8_t> serialize() const = 0;

protected:
    explicit Message(MessageType t) : type(t) {}
};

// ============================================================
// Client -> Server Messages
// ============================================================

struct AuthenticateMsg : Message {
    std::string name;
    std::string token;

    AuthenticateMsg() : Message(MessageType::Authenticate) {}
    AuthenticateMsg(std::string n, std::string t)
        : Message(MessageType::Authenticate), name(std::move(n)), token(std::move(t)) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<AuthenticateMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct JoinGameMsg : Message {
    GameId game_id{0};  // 0 = any game
    uint8_t slot{0};    // 0 = auto-assign

    JoinGameMsg() : Message(MessageType::JoinGame) {}
    JoinGameMsg(GameId gid, uint8_t s) : Message(MessageType::JoinGame), game_id(gid), slot(s) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<JoinGameMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct PlaceTankMsg : Message {
    Coord position;
    Direction direction{Direction::North};

    PlaceTankMsg() : Message(MessageType::PlaceTank) {}
    PlaceTankMsg(Coord pos, Direction dir)
        : Message(MessageType::PlaceTank), position(pos), direction(dir) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<PlaceTankMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct MoveMsg : Message {
    TankId tank_id{0};
    Direction direction{Direction::North};
    uint8_t speed{1};

    MoveMsg() : Message(MessageType::Move) {}
    MoveMsg(TankId tid, Direction dir, uint8_t spd)
        : Message(MessageType::Move), tank_id(tid), direction(dir), speed(spd) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<MoveMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct RotateMsg : Message {
    TankId tank_id{0};
    uint16_t angle{0};

    RotateMsg() : Message(MessageType::Rotate) {}
    RotateMsg(TankId tid, uint16_t ang) : Message(MessageType::Rotate), tank_id(tid), angle(ang) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<RotateMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct FireMsg : Message {
    TankId tank_id{0};
    Coord target;

    FireMsg() : Message(MessageType::Fire) {}
    FireMsg(TankId tid, Coord tgt) : Message(MessageType::Fire), tank_id(tid), target(tgt) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<FireMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct ScanCameraMsg : Message {
    TankId tank_id{0};
    uint16_t arc{90};

    ScanCameraMsg() : Message(MessageType::ScanCamera) {}
    ScanCameraMsg(TankId tid, uint16_t a) : Message(MessageType::ScanCamera), tank_id(tid), arc(a) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<ScanCameraMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct ScanIRMsg : Message {
    TankId tank_id{0};

    ScanIRMsg() : Message(MessageType::ScanIR) {}
    explicit ScanIRMsg(TankId tid) : Message(MessageType::ScanIR), tank_id(tid) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<ScanIRMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct ScanRadarMsg : Message {
    TankId tank_id{0};

    ScanRadarMsg() : Message(MessageType::ScanRadar) {}
    explicit ScanRadarMsg(TankId tid) : Message(MessageType::ScanRadar), tank_id(tid) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<ScanRadarMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct CollectResourceMsg : Message {
    TankId tank_id{0};

    CollectResourceMsg() : Message(MessageType::CollectResource) {}
    explicit CollectResourceMsg(TankId tid) : Message(MessageType::CollectResource), tank_id(tid) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<CollectResourceMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct GetMapViewMsg : Message {
    TankId tank_id{0};
    uint8_t radius{15};

    GetMapViewMsg() : Message(MessageType::GetMapView) {}
    GetMapViewMsg(TankId tid, uint8_t r) : Message(MessageType::GetMapView), tank_id(tid), radius(r) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<GetMapViewMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct DisconnectMsg : Message {
    DisconnectReason reason{DisconnectReason::Quit};

    DisconnectMsg() : Message(MessageType::Disconnect) {}
    explicit DisconnectMsg(DisconnectReason r) : Message(MessageType::Disconnect), reason(r) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<DisconnectMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

// ============================================================
// Server -> Client Responses
// ============================================================

struct WelcomeMsg : Message {
    uint16_t version{1};
    uint16_t tick_rate{60};
    uint8_t max_tanks{5};

    WelcomeMsg() : Message(MessageType::Welcome) {}
    WelcomeMsg(uint16_t v, uint16_t tr, uint8_t mt)
        : Message(MessageType::Welcome), version(v), tick_rate(tr), max_tanks(mt) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<WelcomeMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct AuthResultMsg : Message {
    bool success{false};
    PlayerId player_id{0};
    std::string message;

    AuthResultMsg() : Message(MessageType::AuthResult) {}
    AuthResultMsg(bool s, PlayerId pid, std::string msg)
        : Message(MessageType::AuthResult), success(s), player_id(pid), message(std::move(msg)) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<AuthResultMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct GameStateMsg : Message {
    GameInfo game;

    GameStateMsg() : Message(MessageType::GameState) {}
    explicit GameStateMsg(GameInfo g) : Message(MessageType::GameState), game(std::move(g)) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<GameStateMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct TankPlacedMsg : Message {
    TankId tank_id{0};
    Coord position;
    bool success{false};
    ErrorCode error{ErrorCode::Success};

    TankPlacedMsg() : Message(MessageType::TankPlaced) {}
    TankPlacedMsg(TankId tid, Coord pos, bool s, ErrorCode err = ErrorCode::Success)
        : Message(MessageType::TankPlaced), tank_id(tid), position(pos), success(s), error(err) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<TankPlacedMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct ScanResultMsg : Message {
    TankId tank_id{0};
    ScanType scan_type{ScanType::Camera};
    std::vector<Contact> contacts;

    ScanResultMsg() : Message(MessageType::ScanResult) {}
    ScanResultMsg(TankId tid, ScanType st, std::vector<Contact> c)
        : Message(MessageType::ScanResult), tank_id(tid), scan_type(st), contacts(std::move(c)) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<ScanResultMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct MapViewResponseMsg : Message {
    MapView view;

    MapViewResponseMsg() : Message(MessageType::MapViewResponse) {}
    explicit MapViewResponseMsg(MapView v) : Message(MessageType::MapViewResponse), view(std::move(v)) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<MapViewResponseMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct TankStatusUpdateMsg : Message {
    TankStatus status;

    TankStatusUpdateMsg() : Message(MessageType::TankStatusUpdate) {}
    explicit TankStatusUpdateMsg(TankStatus s) : Message(MessageType::TankStatusUpdate), status(s) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<TankStatusUpdateMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct ErrorResponseMsg : Message {
    ErrorCode code{ErrorCode::Unknown};
    std::string message;

    ErrorResponseMsg() : Message(MessageType::ErrorResponse) {}
    ErrorResponseMsg(ErrorCode c, std::string msg)
        : Message(MessageType::ErrorResponse), code(c), message(std::move(msg)) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<ErrorResponseMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

// ============================================================
// Server -> Client Events
// ============================================================

struct EventHitMsg : Message {
    TankId tank_id{0};
    TankId attacker_id{0};
    uint16_t damage{0};
    Coord hit_position;
    uint16_t remaining_health{0};

    EventHitMsg() : Message(MessageType::EventHit) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventHitMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventDestroyedMsg : Message {
    TankId tank_id{0};
    TankId killer_id{0};
    uint16_t points{0};

    EventDestroyedMsg() : Message(MessageType::EventDestroyed) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventDestroyedMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventObstacleMsg : Message {
    TankId tank_id{0};
    Coord position;
    Terrain terrain_type{Terrain::Wall};

    EventObstacleMsg() : Message(MessageType::EventObstacle) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventObstacleMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventFireResultMsg : Message {
    TankId tank_id{0};
    FireResult result{FireResult::Miss};
    Coord hit_position;
    TankId target_id{0};
    uint16_t damage{0};

    EventFireResultMsg() : Message(MessageType::EventFireResult) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventFireResultMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventCollectedMsg : Message {
    TankId tank_id{0};
    ResourceType resource_type{ResourceType::Fuel};
    uint16_t amount{0};
    Coord position;

    EventCollectedMsg() : Message(MessageType::EventCollected) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventCollectedMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventTankSpawnedMsg : Message {
    TankId tank_id{0};
    PlayerId owner_id{0};
    Coord position;
    bool is_friendly{false};

    EventTankSpawnedMsg() : Message(MessageType::EventTankSpawned) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventTankSpawnedMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventTankMovedMsg : Message {
    TankId tank_id{0};
    Coord from;
    Coord to;
    Direction direction{Direction::North};

    EventTankMovedMsg() : Message(MessageType::EventTankMoved) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventTankMovedMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventGameOverMsg : Message {
    PlayerId winner{0};
    uint8_t reason{0};
    uint32_t duration_ticks{0};
    std::vector<std::pair<PlayerId, uint32_t>> final_scores;

    EventGameOverMsg() : Message(MessageType::EventGameOver) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventGameOverMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct EventTickMsg : Message {
    TickNumber tick{0};
    uint64_t timestamp{0};

    EventTickMsg() : Message(MessageType::EventTick) {}
    EventTickMsg(TickNumber t, uint64_t ts) : Message(MessageType::EventTick), tick(t), timestamp(ts) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<EventTickMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

// ============================================================
// Bidirectional Messages
// ============================================================

struct PingMsg : Message {
    uint32_t sequence{0};

    PingMsg() : Message(MessageType::Ping) {}
    explicit PingMsg(uint32_t seq) : Message(MessageType::Ping), sequence(seq) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<PingMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

struct PongMsg : Message {
    uint32_t sequence{0};
    uint32_t latency_ms{0};

    PongMsg() : Message(MessageType::Pong) {}
    PongMsg(uint32_t seq, uint32_t lat) : Message(MessageType::Pong), sequence(seq), latency_ms(lat) {}

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    static std::expected<PongMsg, ErrorCode> deserialize(std::span<const uint8_t> data);
};

// ============================================================
// Message parsing utilities
// ============================================================

// Parse a message header from raw bytes
std::expected<MessageHeader, ErrorCode> parse_header(std::span<const uint8_t> data);

// Create a framed message with header
std::vector<uint8_t> frame_message(const Message& msg);

// Parse any message from raw bytes (after header)
std::expected<std::unique_ptr<Message>, ErrorCode> parse_message(
    MessageType type, std::span<const uint8_t> payload);

} // namespace dandy
