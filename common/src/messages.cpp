#include "dandy/protocol.hpp"
#include "dandy/serialization.hpp"

namespace dandy {

// ============================================================
// Client -> Server Messages
// ============================================================

std::vector<uint8_t> AuthenticateMsg::serialize() const {
    ByteWriter writer;
    writer.write_string(name);
    writer.write_string(token);
    return writer.take();
}

std::expected<AuthenticateMsg, ErrorCode> AuthenticateMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    AuthenticateMsg msg;
    auto name = reader.read_string();
    if (!name) return std::unexpected(name.error());
    msg.name = *name;
    auto token = reader.read_string();
    if (!token) return std::unexpected(token.error());
    msg.token = *token;
    return msg;
}

std::vector<uint8_t> JoinGameMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(game_id);
    writer.write_u8(slot);
    return writer.take();
}

std::expected<JoinGameMsg, ErrorCode> JoinGameMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    JoinGameMsg msg;
    auto gid = reader.read_u32();
    if (!gid) return std::unexpected(gid.error());
    msg.game_id = *gid;
    auto s = reader.read_u8();
    if (!s) return std::unexpected(s.error());
    msg.slot = *s;
    return msg;
}

std::vector<uint8_t> PlaceTankMsg::serialize() const {
    ByteWriter writer;
    writer.write_coord(position);
    writer.write_u8(static_cast<uint8_t>(direction));
    return writer.take();
}

std::expected<PlaceTankMsg, ErrorCode> PlaceTankMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    PlaceTankMsg msg;
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.position = *pos;
    auto dir = reader.read_u8();
    if (!dir) return std::unexpected(dir.error());
    msg.direction = static_cast<Direction>(*dir);
    return msg;
}

std::vector<uint8_t> MoveMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u8(static_cast<uint8_t>(direction));
    writer.write_u8(speed);
    return writer.take();
}

std::expected<MoveMsg, ErrorCode> MoveMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    MoveMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto dir = reader.read_u8();
    if (!dir) return std::unexpected(dir.error());
    msg.direction = static_cast<Direction>(*dir);
    auto spd = reader.read_u8();
    if (!spd) return std::unexpected(spd.error());
    msg.speed = *spd;
    return msg;
}

std::vector<uint8_t> RotateMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u16(angle);
    return writer.take();
}

std::expected<RotateMsg, ErrorCode> RotateMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    RotateMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto ang = reader.read_u16();
    if (!ang) return std::unexpected(ang.error());
    msg.angle = *ang;
    return msg;
}

std::vector<uint8_t> FireMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_coord(target);
    return writer.take();
}

std::expected<FireMsg, ErrorCode> FireMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    FireMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto tgt = reader.read_coord();
    if (!tgt) return std::unexpected(tgt.error());
    msg.target = *tgt;
    return msg;
}

std::vector<uint8_t> ScanCameraMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u16(arc);
    return writer.take();
}

std::expected<ScanCameraMsg, ErrorCode> ScanCameraMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    ScanCameraMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto a = reader.read_u16();
    if (!a) return std::unexpected(a.error());
    msg.arc = *a;
    return msg;
}

std::vector<uint8_t> ScanIRMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    return writer.take();
}

std::expected<ScanIRMsg, ErrorCode> ScanIRMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    ScanIRMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    return msg;
}

std::vector<uint8_t> ScanRadarMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    return writer.take();
}

std::expected<ScanRadarMsg, ErrorCode> ScanRadarMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    ScanRadarMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    return msg;
}

std::vector<uint8_t> CollectResourceMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    return writer.take();
}

std::expected<CollectResourceMsg, ErrorCode> CollectResourceMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    CollectResourceMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    return msg;
}

std::vector<uint8_t> GetMapViewMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u8(radius);
    return writer.take();
}

std::expected<GetMapViewMsg, ErrorCode> GetMapViewMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    GetMapViewMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto r = reader.read_u8();
    if (!r) return std::unexpected(r.error());
    msg.radius = *r;
    return msg;
}

std::vector<uint8_t> DisconnectMsg::serialize() const {
    ByteWriter writer;
    writer.write_u8(static_cast<uint8_t>(reason));
    return writer.take();
}

std::expected<DisconnectMsg, ErrorCode> DisconnectMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    DisconnectMsg msg;
    auto r = reader.read_u8();
    if (!r) return std::unexpected(r.error());
    msg.reason = static_cast<DisconnectReason>(*r);
    return msg;
}

// ============================================================
// Server -> Client Responses
// ============================================================

std::vector<uint8_t> WelcomeMsg::serialize() const {
    ByteWriter writer;
    writer.write_u16(version);
    writer.write_u16(tick_rate);
    writer.write_u8(max_tanks);
    return writer.take();
}

std::expected<WelcomeMsg, ErrorCode> WelcomeMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    WelcomeMsg msg;
    auto v = reader.read_u16();
    if (!v) return std::unexpected(v.error());
    msg.version = *v;
    auto tr = reader.read_u16();
    if (!tr) return std::unexpected(tr.error());
    msg.tick_rate = *tr;
    auto mt = reader.read_u8();
    if (!mt) return std::unexpected(mt.error());
    msg.max_tanks = *mt;
    return msg;
}

std::vector<uint8_t> AuthResultMsg::serialize() const {
    ByteWriter writer;
    writer.write_u8(success ? 1 : 0);
    writer.write_u32(player_id);
    writer.write_string(message);
    return writer.take();
}

std::expected<AuthResultMsg, ErrorCode> AuthResultMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    AuthResultMsg msg;
    auto s = reader.read_u8();
    if (!s) return std::unexpected(s.error());
    msg.success = (*s != 0);
    auto pid = reader.read_u32();
    if (!pid) return std::unexpected(pid.error());
    msg.player_id = *pid;
    auto m = reader.read_string();
    if (!m) return std::unexpected(m.error());
    msg.message = *m;
    return msg;
}

std::vector<uint8_t> GameStateMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(game.id);
    writer.write_u8(static_cast<uint8_t>(game.status));
    writer.write_u32(game.current_tick);
    writer.write_u16(game.map_width);
    writer.write_u16(game.map_height);
    writer.write_u8(static_cast<uint8_t>(game.players.size()));
    for (const auto& p : game.players) {
        writer.write_u32(p.id);
        writer.write_string(p.name);
        writer.write_u32(p.score);
        writer.write_u8(p.tank_count);
    }
    return writer.take();
}

std::expected<GameStateMsg, ErrorCode> GameStateMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    GameStateMsg msg;
    auto id = reader.read_u32();
    if (!id) return std::unexpected(id.error());
    msg.game.id = *id;
    auto status = reader.read_u8();
    if (!status) return std::unexpected(status.error());
    msg.game.status = static_cast<GameStatus>(*status);
    auto tick = reader.read_u32();
    if (!tick) return std::unexpected(tick.error());
    msg.game.current_tick = *tick;
    auto w = reader.read_u16();
    if (!w) return std::unexpected(w.error());
    msg.game.map_width = *w;
    auto h = reader.read_u16();
    if (!h) return std::unexpected(h.error());
    msg.game.map_height = *h;
    auto np = reader.read_u8();
    if (!np) return std::unexpected(np.error());
    for (uint8_t i = 0; i < *np; ++i) {
        PlayerInfo p;
        auto pid = reader.read_u32();
        if (!pid) return std::unexpected(pid.error());
        p.id = *pid;
        auto name = reader.read_string();
        if (!name) return std::unexpected(name.error());
        p.name = *name;
        auto score = reader.read_u32();
        if (!score) return std::unexpected(score.error());
        p.score = *score;
        auto tc = reader.read_u8();
        if (!tc) return std::unexpected(tc.error());
        p.tank_count = *tc;
        msg.game.players.push_back(std::move(p));
    }
    return msg;
}

std::vector<uint8_t> TankPlacedMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_coord(position);
    writer.write_u8(success ? 1 : 0);
    writer.write_u8(static_cast<uint8_t>(error));
    return writer.take();
}

std::expected<TankPlacedMsg, ErrorCode> TankPlacedMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    TankPlacedMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.position = *pos;
    auto s = reader.read_u8();
    if (!s) return std::unexpected(s.error());
    msg.success = (*s != 0);
    auto err = reader.read_u8();
    if (!err) return std::unexpected(err.error());
    msg.error = static_cast<ErrorCode>(*err);
    return msg;
}

std::vector<uint8_t> ScanResultMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u8(static_cast<uint8_t>(scan_type));
    writer.write_u16(static_cast<uint16_t>(contacts.size()));
    for (const auto& c : contacts) {
        writer.write_coord(c.position);
        writer.write_u8(static_cast<uint8_t>(c.type));
        writer.write_u8(c.flags);
    }
    return writer.take();
}

std::expected<ScanResultMsg, ErrorCode> ScanResultMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    ScanResultMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto st = reader.read_u8();
    if (!st) return std::unexpected(st.error());
    msg.scan_type = static_cast<ScanType>(*st);
    auto nc = reader.read_u16();
    if (!nc) return std::unexpected(nc.error());
    for (uint16_t i = 0; i < *nc; ++i) {
        Contact c;
        auto pos = reader.read_coord();
        if (!pos) return std::unexpected(pos.error());
        c.position = *pos;
        auto type = reader.read_u8();
        if (!type) return std::unexpected(type.error());
        c.type = static_cast<EntityType>(*type);
        auto flags = reader.read_u8();
        if (!flags) return std::unexpected(flags.error());
        c.flags = *flags;
        msg.contacts.push_back(c);
    }
    return msg;
}

std::vector<uint8_t> MapViewResponseMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(view.viewer_tank);
    writer.write_coord(view.center);
    writer.write_u8(view.width);
    writer.write_u8(view.height);
    for (const auto& cell : view.cells) {
        writer.write_u8(cell.encode());
    }
    return writer.take();
}

std::expected<MapViewResponseMsg, ErrorCode> MapViewResponseMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    MapViewResponseMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.view.viewer_tank = *tid;
    auto center = reader.read_coord();
    if (!center) return std::unexpected(center.error());
    msg.view.center = *center;
    auto w = reader.read_u8();
    if (!w) return std::unexpected(w.error());
    msg.view.width = *w;
    auto h = reader.read_u8();
    if (!h) return std::unexpected(h.error());
    msg.view.height = *h;
    size_t cell_count = msg.view.width * msg.view.height;
    msg.view.cells.reserve(cell_count);
    for (size_t i = 0; i < cell_count; ++i) {
        auto c = reader.read_u8();
        if (!c) return std::unexpected(c.error());
        msg.view.cells.push_back(Cell::decode(*c));
    }
    return msg;
}

std::vector<uint8_t> TankStatusUpdateMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(status.id);
    writer.write_coord(status.position);
    writer.write_u8(static_cast<uint8_t>(status.direction));
    writer.write_u16(status.health);
    writer.write_u16(status.fuel);
    writer.write_u16(status.ammo);
    writer.write_u16(status.turret_angle);
    writer.write_u8(status.speed);
    writer.write_u8(status.flags);
    writer.write_u32(status.score);
    return writer.take();
}

std::expected<TankStatusUpdateMsg, ErrorCode> TankStatusUpdateMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    TankStatusUpdateMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.status.id = *tid;
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.status.position = *pos;
    auto dir = reader.read_u8();
    if (!dir) return std::unexpected(dir.error());
    msg.status.direction = static_cast<Direction>(*dir);
    auto hp = reader.read_u16();
    if (!hp) return std::unexpected(hp.error());
    msg.status.health = *hp;
    auto fuel = reader.read_u16();
    if (!fuel) return std::unexpected(fuel.error());
    msg.status.fuel = *fuel;
    auto ammo = reader.read_u16();
    if (!ammo) return std::unexpected(ammo.error());
    msg.status.ammo = *ammo;
    auto turret = reader.read_u16();
    if (!turret) return std::unexpected(turret.error());
    msg.status.turret_angle = *turret;
    auto spd = reader.read_u8();
    if (!spd) return std::unexpected(spd.error());
    msg.status.speed = *spd;
    auto flags = reader.read_u8();
    if (!flags) return std::unexpected(flags.error());
    msg.status.flags = *flags;
    auto score = reader.read_u32();
    if (!score) return std::unexpected(score.error());
    msg.status.score = *score;
    return msg;
}

std::vector<uint8_t> ErrorResponseMsg::serialize() const {
    ByteWriter writer;
    writer.write_u8(static_cast<uint8_t>(code));
    writer.write_string(message);
    return writer.take();
}

std::expected<ErrorResponseMsg, ErrorCode> ErrorResponseMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    ErrorResponseMsg msg;
    auto c = reader.read_u8();
    if (!c) return std::unexpected(c.error());
    msg.code = static_cast<ErrorCode>(*c);
    auto m = reader.read_string();
    if (!m) return std::unexpected(m.error());
    msg.message = *m;
    return msg;
}

// ============================================================
// Events
// ============================================================

std::vector<uint8_t> EventHitMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u32(attacker_id);
    writer.write_u16(damage);
    writer.write_coord(hit_position);
    writer.write_u16(remaining_health);
    return writer.take();
}

std::expected<EventHitMsg, ErrorCode> EventHitMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventHitMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto aid = reader.read_u32();
    if (!aid) return std::unexpected(aid.error());
    msg.attacker_id = *aid;
    auto dmg = reader.read_u16();
    if (!dmg) return std::unexpected(dmg.error());
    msg.damage = *dmg;
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.hit_position = *pos;
    auto hp = reader.read_u16();
    if (!hp) return std::unexpected(hp.error());
    msg.remaining_health = *hp;
    return msg;
}

std::vector<uint8_t> EventDestroyedMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u32(killer_id);
    writer.write_u16(points);
    return writer.take();
}

std::expected<EventDestroyedMsg, ErrorCode> EventDestroyedMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventDestroyedMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto kid = reader.read_u32();
    if (!kid) return std::unexpected(kid.error());
    msg.killer_id = *kid;
    auto pts = reader.read_u16();
    if (!pts) return std::unexpected(pts.error());
    msg.points = *pts;
    return msg;
}

std::vector<uint8_t> EventObstacleMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_coord(position);
    writer.write_u8(static_cast<uint8_t>(terrain_type));
    return writer.take();
}

std::expected<EventObstacleMsg, ErrorCode> EventObstacleMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventObstacleMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.position = *pos;
    auto t = reader.read_u8();
    if (!t) return std::unexpected(t.error());
    msg.terrain_type = static_cast<Terrain>(*t);
    return msg;
}

std::vector<uint8_t> EventFireResultMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u8(static_cast<uint8_t>(result));
    writer.write_coord(hit_position);
    writer.write_u32(target_id);
    writer.write_u16(damage);
    return writer.take();
}

std::expected<EventFireResultMsg, ErrorCode> EventFireResultMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventFireResultMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto r = reader.read_u8();
    if (!r) return std::unexpected(r.error());
    msg.result = static_cast<FireResult>(*r);
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.hit_position = *pos;
    auto tgt = reader.read_u32();
    if (!tgt) return std::unexpected(tgt.error());
    msg.target_id = *tgt;
    auto dmg = reader.read_u16();
    if (!dmg) return std::unexpected(dmg.error());
    msg.damage = *dmg;
    return msg;
}

std::vector<uint8_t> EventCollectedMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u8(static_cast<uint8_t>(resource_type));
    writer.write_u16(amount);
    writer.write_coord(position);
    return writer.take();
}

std::expected<EventCollectedMsg, ErrorCode> EventCollectedMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventCollectedMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto rt = reader.read_u8();
    if (!rt) return std::unexpected(rt.error());
    msg.resource_type = static_cast<ResourceType>(*rt);
    auto amt = reader.read_u16();
    if (!amt) return std::unexpected(amt.error());
    msg.amount = *amt;
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.position = *pos;
    return msg;
}

std::vector<uint8_t> EventTankSpawnedMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_u32(owner_id);
    writer.write_coord(position);
    writer.write_u8(is_friendly ? 1 : 0);
    return writer.take();
}

std::expected<EventTankSpawnedMsg, ErrorCode> EventTankSpawnedMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventTankSpawnedMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto oid = reader.read_u32();
    if (!oid) return std::unexpected(oid.error());
    msg.owner_id = *oid;
    auto pos = reader.read_coord();
    if (!pos) return std::unexpected(pos.error());
    msg.position = *pos;
    auto f = reader.read_u8();
    if (!f) return std::unexpected(f.error());
    msg.is_friendly = (*f != 0);
    return msg;
}

std::vector<uint8_t> EventTankMovedMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tank_id);
    writer.write_coord(from);
    writer.write_coord(to);
    writer.write_u8(static_cast<uint8_t>(direction));
    return writer.take();
}

std::expected<EventTankMovedMsg, ErrorCode> EventTankMovedMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventTankMovedMsg msg;
    auto tid = reader.read_u32();
    if (!tid) return std::unexpected(tid.error());
    msg.tank_id = *tid;
    auto f = reader.read_coord();
    if (!f) return std::unexpected(f.error());
    msg.from = *f;
    auto t = reader.read_coord();
    if (!t) return std::unexpected(t.error());
    msg.to = *t;
    auto dir = reader.read_u8();
    if (!dir) return std::unexpected(dir.error());
    msg.direction = static_cast<Direction>(*dir);
    return msg;
}

std::vector<uint8_t> EventGameOverMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(winner);
    writer.write_u8(reason);
    writer.write_u32(duration_ticks);
    writer.write_u8(static_cast<uint8_t>(final_scores.size()));
    for (const auto& [pid, score] : final_scores) {
        writer.write_u32(pid);
        writer.write_u32(score);
    }
    return writer.take();
}

std::expected<EventGameOverMsg, ErrorCode> EventGameOverMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventGameOverMsg msg;
    auto w = reader.read_u32();
    if (!w) return std::unexpected(w.error());
    msg.winner = *w;
    auto r = reader.read_u8();
    if (!r) return std::unexpected(r.error());
    msg.reason = *r;
    auto d = reader.read_u32();
    if (!d) return std::unexpected(d.error());
    msg.duration_ticks = *d;
    auto ns = reader.read_u8();
    if (!ns) return std::unexpected(ns.error());
    for (uint8_t i = 0; i < *ns; ++i) {
        auto pid = reader.read_u32();
        if (!pid) return std::unexpected(pid.error());
        auto score = reader.read_u32();
        if (!score) return std::unexpected(score.error());
        msg.final_scores.emplace_back(*pid, *score);
    }
    return msg;
}

std::vector<uint8_t> EventTickMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(tick);
    writer.write_u64(timestamp);
    return writer.take();
}

std::expected<EventTickMsg, ErrorCode> EventTickMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    EventTickMsg msg;
    auto t = reader.read_u32();
    if (!t) return std::unexpected(t.error());
    msg.tick = *t;
    auto ts = reader.read_u64();
    if (!ts) return std::unexpected(ts.error());
    msg.timestamp = *ts;
    return msg;
}

// ============================================================
// Bidirectional
// ============================================================

std::vector<uint8_t> PingMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(sequence);
    return writer.take();
}

std::expected<PingMsg, ErrorCode> PingMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    PingMsg msg;
    auto seq = reader.read_u32();
    if (!seq) return std::unexpected(seq.error());
    msg.sequence = *seq;
    return msg;
}

std::vector<uint8_t> PongMsg::serialize() const {
    ByteWriter writer;
    writer.write_u32(sequence);
    writer.write_u32(latency_ms);
    return writer.take();
}

std::expected<PongMsg, ErrorCode> PongMsg::deserialize(std::span<const uint8_t> data) {
    ByteReader reader(data);
    PongMsg msg;
    auto seq = reader.read_u32();
    if (!seq) return std::unexpected(seq.error());
    msg.sequence = *seq;
    auto lat = reader.read_u32();
    if (!lat) return std::unexpected(lat.error());
    msg.latency_ms = *lat;
    return msg;
}

} // namespace dandy
