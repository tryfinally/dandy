#include "dandy/protocol.hpp"
#include "dandy/serialization.hpp"
#include <cmath>

namespace dandy {

// ============================================================
// Utility functions
// ============================================================

std::expected<MessageHeader, ErrorCode> parse_header(std::span<const uint8_t> data) {
    if (data.size() < HEADER_SIZE) {
        return std::unexpected(ErrorCode::InvalidFormat);
    }

    ByteReader reader(data);
    auto length = reader.read_u32();
    if (!length) return std::unexpected(length.error());

    auto type_raw = reader.read_u16();
    if (!type_raw) return std::unexpected(type_raw.error());

    return MessageHeader{*length, static_cast<MessageType>(*type_raw)};
}

std::vector<uint8_t> frame_message(const Message& msg) {
    auto payload = msg.serialize();

    ByteWriter writer(HEADER_SIZE + payload.size());
    writer.write_u32(static_cast<uint32_t>(payload.size() + sizeof(uint16_t)));
    writer.write_u16(static_cast<uint16_t>(msg.type));
    writer.write_bytes(payload);

    return writer.take();
}

std::expected<std::unique_ptr<Message>, ErrorCode> parse_message(
    MessageType type, std::span<const uint8_t> payload)
{
    switch (type) {
        // Client -> Server
        case MessageType::Authenticate: {
            auto result = AuthenticateMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<AuthenticateMsg>(std::move(*result));
        }
        case MessageType::JoinGame: {
            auto result = JoinGameMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<JoinGameMsg>(std::move(*result));
        }
        case MessageType::PlaceTank: {
            auto result = PlaceTankMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<PlaceTankMsg>(std::move(*result));
        }
        case MessageType::Move: {
            auto result = MoveMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<MoveMsg>(std::move(*result));
        }
        case MessageType::Rotate: {
            auto result = RotateMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<RotateMsg>(std::move(*result));
        }
        case MessageType::Fire: {
            auto result = FireMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<FireMsg>(std::move(*result));
        }
        case MessageType::ScanCamera: {
            auto result = ScanCameraMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<ScanCameraMsg>(std::move(*result));
        }
        case MessageType::ScanIR: {
            auto result = ScanIRMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<ScanIRMsg>(std::move(*result));
        }
        case MessageType::ScanRadar: {
            auto result = ScanRadarMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<ScanRadarMsg>(std::move(*result));
        }
        case MessageType::CollectResource: {
            auto result = CollectResourceMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<CollectResourceMsg>(std::move(*result));
        }
        case MessageType::GetMapView: {
            auto result = GetMapViewMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<GetMapViewMsg>(std::move(*result));
        }
        case MessageType::Disconnect: {
            auto result = DisconnectMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<DisconnectMsg>(std::move(*result));
        }

        // Server -> Client Responses
        case MessageType::Welcome: {
            auto result = WelcomeMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<WelcomeMsg>(std::move(*result));
        }
        case MessageType::AuthResult: {
            auto result = AuthResultMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<AuthResultMsg>(std::move(*result));
        }
        case MessageType::GameState: {
            auto result = GameStateMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<GameStateMsg>(std::move(*result));
        }
        case MessageType::TankPlaced: {
            auto result = TankPlacedMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<TankPlacedMsg>(std::move(*result));
        }
        case MessageType::ScanResult: {
            auto result = ScanResultMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<ScanResultMsg>(std::move(*result));
        }
        case MessageType::MapViewResponse: {
            auto result = MapViewResponseMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<MapViewResponseMsg>(std::move(*result));
        }
        case MessageType::TankStatusUpdate: {
            auto result = TankStatusUpdateMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<TankStatusUpdateMsg>(std::move(*result));
        }
        case MessageType::ErrorResponse: {
            auto result = ErrorResponseMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<ErrorResponseMsg>(std::move(*result));
        }

        // Events
        case MessageType::EventHit: {
            auto result = EventHitMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventHitMsg>(std::move(*result));
        }
        case MessageType::EventDestroyed: {
            auto result = EventDestroyedMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventDestroyedMsg>(std::move(*result));
        }
        case MessageType::EventObstacle: {
            auto result = EventObstacleMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventObstacleMsg>(std::move(*result));
        }
        case MessageType::EventFireResult: {
            auto result = EventFireResultMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventFireResultMsg>(std::move(*result));
        }
        case MessageType::EventCollected: {
            auto result = EventCollectedMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventCollectedMsg>(std::move(*result));
        }
        case MessageType::EventTankSpawned: {
            auto result = EventTankSpawnedMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventTankSpawnedMsg>(std::move(*result));
        }
        case MessageType::EventTankMoved: {
            auto result = EventTankMovedMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventTankMovedMsg>(std::move(*result));
        }
        case MessageType::EventGameOver: {
            auto result = EventGameOverMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventGameOverMsg>(std::move(*result));
        }
        case MessageType::EventTick: {
            auto result = EventTickMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<EventTickMsg>(std::move(*result));
        }

        // Bidirectional
        case MessageType::Ping: {
            auto result = PingMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<PingMsg>(std::move(*result));
        }
        case MessageType::Pong: {
            auto result = PongMsg::deserialize(payload);
            if (!result) return std::unexpected(result.error());
            return std::make_unique<PongMsg>(std::move(*result));
        }

        default:
            return std::unexpected(ErrorCode::InvalidFormat);
    }
}

} // namespace dandy
