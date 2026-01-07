#pragma once

#include <cstdint>
#include <chrono>

namespace dandy::constants {

// Protocol version
constexpr uint16_t PROTOCOL_VERSION = 1;

// Server defaults
constexpr uint16_t DEFAULT_PORT = 7777;
constexpr uint16_t TICK_RATE = 60;  // ticks per second
constexpr auto TICK_DURATION = std::chrono::milliseconds(1000 / TICK_RATE);

// Map defaults
constexpr uint16_t DEFAULT_MAP_WIDTH = 256;
constexpr uint16_t DEFAULT_MAP_HEIGHT = 256;
constexpr uint8_t DEFAULT_VIEW_RADIUS = 15;

// Player/Game limits
constexpr uint8_t MAX_PLAYERS_PER_GAME = 8;
constexpr uint8_t MAX_TANKS_PER_PLAYER = 5;
constexpr uint32_t MAX_MESSAGE_SIZE = 65536;

// Tank defaults
constexpr uint16_t DEFAULT_TANK_HEALTH = 100;
constexpr uint16_t DEFAULT_TANK_FUEL = 100;
constexpr uint16_t DEFAULT_TANK_AMMO = 20;

// Sensor ranges
constexpr uint8_t CAMERA_RANGE = 10;
constexpr uint8_t CAMERA_ARC = 90;     // degrees
constexpr uint8_t IR_RANGE = 15;
constexpr uint8_t RADAR_RANGE = 25;

// Combat
constexpr uint8_t MAIN_GUN_RANGE = 20;
constexpr uint8_t RELOAD_TICKS = 10;   // ~166ms at 60 tick
constexpr uint16_t MIN_DAMAGE = 25;
constexpr uint16_t MAX_DAMAGE = 50;

// Movement
constexpr uint8_t MAX_SPEED = 3;
constexpr uint8_t FUEL_PER_CELL = 1;
constexpr uint8_t ROTATION_PER_TICK = 45;  // degrees

// Cooldowns (in ticks)
constexpr uint8_t CAMERA_COOLDOWN = 1;
constexpr uint8_t IR_COOLDOWN = 30;     // 500ms
constexpr uint8_t RADAR_COOLDOWN = 60;  // 1000ms

// Resources
constexpr uint16_t FUEL_DEPOT_AMOUNT = 50;
constexpr uint16_t AMMO_DEPOT_AMOUNT = 10;
constexpr uint16_t HEALTH_PACK_AMOUNT = 30;

// Upgrades
constexpr uint8_t RADAR_UPGRADE_BONUS = 10;
constexpr uint16_t SHIELD_DURATION_TICKS = 300;  // 5 seconds
constexpr uint16_t SPEED_BOOST_DURATION_TICKS = 600;  // 10 seconds

// Terrain movement costs (ticks per cell)
constexpr uint8_t GROUND_COST = 1;
constexpr uint8_t ROAD_COST = 1;  // no slowdown
constexpr uint8_t TREE_COST = 2;  // slower through forest
constexpr uint8_t WALL_COST = 255;  // impassable
constexpr uint8_t WATER_COST = 255; // impassable
constexpr uint8_t MOUNTAIN_COST = 255; // impassable

// Network
constexpr size_t RECV_BUFFER_SIZE = 8192;
constexpr size_t SEND_BUFFER_SIZE = 8192;
constexpr auto CONNECTION_TIMEOUT = std::chrono::seconds(30);
constexpr auto PING_INTERVAL = std::chrono::seconds(5);

// Scoring
constexpr uint16_t POINTS_PER_KILL = 100;
constexpr uint16_t POINTS_PER_HIT = 10;
constexpr uint16_t POINTS_PER_RESOURCE = 5;

} // namespace dandy::constants
