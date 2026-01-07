# Dandy Tank Arena - Network Protocol Specification

## Overview

The Dandy protocol is a binary TCP protocol for client-server communication in the tank battle arena game. All multi-byte integers are transmitted in **little-endian** format.

## Connection Flow

```
Client                                  Server
   │                                       │
   │──────── TCP Connect ─────────────────>│
   │                                       │
   │<─────── WELCOME ──────────────────────│
   │                                       │
   │──────── AUTHENTICATE ────────────────>│
   │                                       │
   │<─────── AUTH_RESULT ──────────────────│
   │                                       │
   │──────── JOIN_GAME ───────────────────>│
   │                                       │
   │<─────── GAME_STATE ───────────────────│
   │                                       │
   │──────── PLACE_TANK ──────────────────>│
   │<─────── TANK_PLACED ──────────────────│
   │                                       │
   │      ┌─── Game Loop ───┐              │
   │      │                 │              │
   │──────│─ COMMAND ───────│─────────────>│
   │<─────│─ EVENT ─────────│──────────────│
   │      │                 │              │
   │      └─────────────────┘              │
   │                                       │
   │──────── DISCONNECT ──────────────────>│
   │                                       │
```

## Message Frame Format

All messages follow this frame structure:

```
┌────────────┬────────────┬─────────────────────┐
│  Length    │  Type      │  Payload            │
│  (4 bytes) │  (2 bytes) │  (Length-2 bytes)   │
└────────────┴────────────┴─────────────────────┘
```

| Field   | Size    | Description                           |
|---------|---------|---------------------------------------|
| Length  | 4 bytes | Total payload size (including Type)   |
| Type    | 2 bytes | Message type identifier               |
| Payload | varies  | Message-specific data                 |

## Message Types

### Type ID Ranges

| Range       | Category              |
|-------------|----------------------|
| 0x0001-0x00FF | Client → Server Commands |
| 0x0100-0x01FF | Server → Client Responses |
| 0x0200-0x02FF | Server → Client Events |
| 0x0300-0x03FF | Bidirectional |

### Client → Server Messages

#### AUTHENTICATE (0x0001)
Authenticate player with server.

```
┌──────────────┬───────────────┐
│ name_len (1) │ name (var)    │
├──────────────┼───────────────┤
│ token_len(1) │ token (var)   │
└──────────────┴───────────────┘
```

#### JOIN_GAME (0x0002)
Request to join an active game.

```
┌──────────────┬───────────────┐
│ game_id (4)  │ slot (1)      │
└──────────────┴───────────────┘
```
- `game_id`: 0 = any available game
- `slot`: Preferred team slot (0 = auto-assign)

#### PLACE_TANK (0x0003)
Place a new tank on the map.

```
┌──────────┬──────────┬───────────┐
│ x (2)    │ y (2)    │ dir (1)   │
└──────────┴──────────┴───────────┘
```
- `x`, `y`: Grid coordinates
- `dir`: Initial direction (0-7, N=0, NE=1, E=2, etc.)

#### MOVE (0x0010)
Move a tank.

```
┌──────────────┬───────────┬───────────┐
│ tank_id (4)  │ dir (1)   │ speed (1) │
└──────────────┴───────────┴───────────┘
```
- `dir`: Direction (0-7)
- `speed`: 0=stop, 1=slow, 2=normal, 3=fast

#### ROTATE (0x0011)
Rotate tank turret.

```
┌──────────────┬─────────────┐
│ tank_id (4)  │ angle (2)   │
└──────────────┴─────────────┘
```
- `angle`: Absolute angle in degrees (0-359)

#### FIRE (0x0012)
Fire main weapon.

```
┌──────────────┬───────────┬───────────┐
│ tank_id (4)  │ target_x(2)│ target_y(2)│
└──────────────┴───────────┴───────────┘
```

#### SCAN_CAMERA (0x0020)
Request camera scan.

```
┌──────────────┬───────────┐
│ tank_id (4)  │ arc (2)   │
└──────────────┴───────────┘
```
- `arc`: Scan arc in degrees (max 90°)

#### SCAN_IR (0x0021)
Request infrared scan.

```
┌──────────────┐
│ tank_id (4)  │
└──────────────┘
```

#### SCAN_RADAR (0x0022)
Request radar scan.

```
┌──────────────┐
│ tank_id (4)  │
└──────────────┘
```

#### COLLECT_RESOURCE (0x0030)
Attempt to collect nearby resource.

```
┌──────────────┐
│ tank_id (4)  │
└──────────────┘
```

#### GET_MAP_VIEW (0x0040)
Request visible map area.

```
┌──────────────┬───────────┐
│ tank_id (4)  │ radius (1)│
└──────────────┴───────────┘
```

#### DISCONNECT (0x00FF)
Graceful disconnect.

```
┌──────────────┐
│ reason (1)   │
└──────────────┘
```
- `reason`: 0=quit, 1=timeout, 2=error

### Server → Client Responses

#### WELCOME (0x0100)
Initial server greeting.

```
┌────────────┬────────────┬────────────────┐
│ version(2) │ tick_rate(2)│ max_tanks(1)  │
└────────────┴────────────┴────────────────┘
```

#### AUTH_RESULT (0x0101)
Authentication result.

```
┌──────────┬────────────┬─────────────────┐
│ success(1)│ player_id(4)│ msg_len(1)+msg │
└──────────┴────────────┴─────────────────┘
```

#### GAME_STATE (0x0102)
Current game state snapshot.

```
┌────────────┬────────────┬────────────┐
│ game_id(4) │ status(1)  │ tick(4)    │
├────────────┼────────────┼────────────┤
│ map_w(2)   │ map_h(2)   │ n_players(1)│
├────────────┴────────────┴────────────┤
│ player_list[n_players]               │
└──────────────────────────────────────┘
```

Player entry:
```
┌────────────┬─────────────┬────────────┐
│ player_id(4)│ name_len(1) │ name(var)  │
├────────────┼─────────────┼────────────┤
│ score(4)   │ n_tanks(1)  │            │
└────────────┴─────────────┴────────────┘
```

#### TANK_PLACED (0x0103)
Confirmation of tank placement.

```
┌──────────────┬──────────┬──────────┬───────────┐
│ tank_id (4)  │ x (2)    │ y (2)    │ success(1)│
└──────────────┴──────────┴──────────┴───────────┘
```

#### SCAN_RESULT (0x0110)
Sensor scan results.

```
┌──────────────┬────────────┬─────────────┐
│ tank_id (4)  │ scan_type(1)│ n_contacts(2)│
├──────────────┴────────────┴─────────────┤
│ contacts[n_contacts]                     │
└──────────────────────────────────────────┘
```

Contact entry:
```
┌──────────┬──────────┬───────────┬───────────┐
│ x (2)    │ y (2)    │ type (1)  │ flags (1) │
└──────────┴──────────┴───────────┴───────────┘
```
- `type`: 0=unknown, 1=tank, 2=projectile, 3=resource, 4=obstacle
- `flags`: bit0=moving, bit1=friendly, bit2=damaged

#### MAP_VIEW (0x0111)
Visible map data.

```
┌──────────────┬────────┬────────┬────────┬────────┐
│ tank_id (4)  │ cx (2) │ cy (2) │ w (1)  │ h (1)  │
├──────────────┴────────┴────────┴────────┴────────┤
│ cells[w * h] (1 byte each)                       │
└──────────────────────────────────────────────────┘
```

Cell encoding:
```
┌─────────────┬──────────────┐
│ terrain (4b)│ entity (4b)  │
└─────────────┴──────────────┘
```
- terrain: 0=ground, 1=wall, 2=water, 3=mountain, 4=tree, 5=road
- entity: 0=empty, 1=tank_self, 2=tank_enemy, 3=projectile, 4=fuel, 5=ammo, 6=health, 7=upgrade

#### TANK_STATUS (0x0112)
Tank status update.

```
┌──────────────┬──────────┬──────────┬───────────┐
│ tank_id (4)  │ x (2)    │ y (2)    │ dir (1)   │
├──────────────┼──────────┼──────────┼───────────┤
│ health (2)   │ fuel (2) │ ammo (2) │ turret(2) │
├──────────────┼──────────┼──────────┼───────────┤
│ speed (1)    │ flags(1) │ score(4) │           │
└──────────────┴──────────┴──────────┴───────────┘
```
- `flags`: bit0=moving, bit1=reloading, bit2=damaged, bit3=shielded

### Server → Client Events

#### EVENT_HIT (0x0200)
Tank was hit.

```
┌──────────────┬───────────────┬────────────┐
│ tank_id (4)  │ attacker_id(4)│ damage (2) │
├──────────────┼───────────────┼────────────┤
│ hit_x (2)    │ hit_y (2)     │ remaining(2)│
└──────────────┴───────────────┴────────────┘
```

#### EVENT_DESTROYED (0x0201)
Tank was destroyed.

```
┌──────────────┬───────────────┬────────────┐
│ tank_id (4)  │ killer_id (4) │ points (2) │
└──────────────┴───────────────┴────────────┘
```

#### EVENT_OBSTACLE (0x0202)
Movement blocked by obstacle.

```
┌──────────────┬──────────┬──────────┬───────────┐
│ tank_id (4)  │ x (2)    │ y (2)    │ type (1)  │
└──────────────┴──────────┴──────────┴───────────┘
```

#### EVENT_FIRE_RESULT (0x0203)
Result of firing.

```
┌──────────────┬───────────┬───────────┬───────────┐
│ tank_id (4)  │ result(1) │ hit_x(2)  │ hit_y(2)  │
├──────────────┼───────────┼───────────┼───────────┤
│ target_id(4) │ damage(2) │           │           │
└──────────────┴───────────┴───────────┴───────────┘
```
- `result`: 0=miss, 1=hit_tank, 2=hit_obstacle, 3=out_of_range, 4=no_ammo

#### EVENT_COLLECTED (0x0204)
Resource collected.

```
┌──────────────┬───────────┬───────────┬───────────┐
│ tank_id (4)  │ res_type(1)│ amount(2) │ x(2),y(2) │
└──────────────┴───────────┴───────────┴───────────┘
```

#### EVENT_GAME_OVER (0x0210)
Game ended.

```
┌────────────┬─────────────┬─────────────────┐
│ winner (4) │ reason (1)  │ duration (4)    │
├────────────┴─────────────┴─────────────────┤
│ final_scores[n_players]                    │
└────────────────────────────────────────────┘
```

#### EVENT_TICK (0x02FF)
Game tick heartbeat.

```
┌────────────┬────────────┐
│ tick (4)   │ timestamp(8)│
└────────────┴────────────┘
```

### Bidirectional Messages

#### PING (0x0300)
Keepalive ping.

```
┌────────────┐
│ seq (4)    │
└────────────┘
```

#### PONG (0x0301)
Keepalive response.

```
┌────────────┬────────────┐
│ seq (4)    │ latency(4) │
└────────────┴────────────┘
```

## Error Codes

| Code | Description              |
|------|--------------------------|
| 0x00 | Success                  |
| 0x01 | Invalid message format   |
| 0x02 | Authentication failed    |
| 0x03 | Game not found           |
| 0x04 | Game full                |
| 0x05 | Invalid coordinates      |
| 0x06 | Tank not found           |
| 0x07 | Not your tank            |
| 0x08 | Insufficient resources   |
| 0x09 | Action on cooldown       |
| 0x0A | Invalid target           |
| 0x0B | Position occupied        |
| 0xFF | Unknown error            |

## Data Types Reference

### Direction Enum
```
0 = North (N)
1 = NorthEast (NE)
2 = East (E)
3 = SouthEast (SE)
4 = South (S)
5 = SouthWest (SW)
6 = West (W)
7 = NorthWest (NW)
```

### Scan Type Enum
```
0 = Camera
1 = Infrared
2 = Radar
```

### Resource Type Enum
```
0 = Fuel
1 = Ammo
2 = Health
3 = Radar Upgrade
4 = Shield Boost
5 = Speed Boost
```

### Game Status Enum
```
0 = Waiting
1 = Starting
2 = Running
3 = Paused
4 = Ended
```

## Example Message Sequences

### Placing and Moving a Tank

```
Client: AUTHENTICATE
  name_len=5, name="Alpha", token_len=8, token="secret01"

Server: AUTH_RESULT
  success=1, player_id=42, msg="Welcome Alpha"

Client: JOIN_GAME
  game_id=0, slot=0

Server: GAME_STATE
  game_id=1, status=2, tick=15432, map_w=256, map_h=256, ...

Client: PLACE_TANK
  x=50, y=50, dir=2

Server: TANK_PLACED
  tank_id=1001, x=50, y=50, success=1

Client: MOVE
  tank_id=1001, dir=2, speed=2

Server: TANK_STATUS
  tank_id=1001, x=51, y=50, dir=2, health=100, fuel=99, ...
```

### Combat Sequence

```
Client: SCAN_RADAR
  tank_id=1001

Server: SCAN_RESULT
  tank_id=1001, scan_type=2, n_contacts=1
  contact: x=65, y=50, type=1, flags=0

Client: FIRE
  tank_id=1001, target_x=65, target_y=50

Server: EVENT_FIRE_RESULT
  tank_id=1001, result=1, hit_x=65, hit_y=50, target_id=2003, damage=35

Server: EVENT_HIT (to target's client)
  tank_id=2003, attacker_id=1001, damage=35, hit_x=65, hit_y=50, remaining=65
```

## Protocol Version

Current protocol version: **1.0**

Version negotiation occurs during WELCOME message. Clients should verify compatibility before proceeding.
