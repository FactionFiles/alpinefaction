#pragma once

#include <cstdint>
#include <common/rfproto.h>
#include "../rf/multi.h"

#pragma pack(push, 1)

// Forward declarations
namespace rf
{
    struct Object;
    struct Player;
    struct Vector3;
    struct Matrix3;
    struct Entity;
}

enum class af_packet_type : uint8_t
{
    af_ping_location_req = 0x50,        // Alpine 1.1
    af_ping_location = 0x51,            // Alpine 1.1
    af_damage_notify = 0x52,            // Alpine 1.1
    af_obj_update = 0x53,               // Alpine 1.1
};

struct af_ping_location_req_packet
{
    RF_GamePacketHeader header;
    RF_Vector pos;
};

struct af_ping_location_packet
{
    RF_GamePacketHeader header;
    uint8_t player_id;
    RF_Vector pos;
};

struct af_damage_notify_packet
{
    RF_GamePacketHeader header;
    uint8_t player_id;
    uint16_t damage;
    uint8_t flags;
};

struct af_obj_update // members of af_obj_update_packet
{
    uint32_t obj_handle;
    uint8_t current_primary_weapon;
    uint8_t ammo_type;
    uint16_t clip_ammo;
    uint16_t reserve_ammo;
};

struct af_obj_update_packet
{
    RF_GamePacketHeader header;
    af_obj_update objects[];
};

#pragma pack(pop)

bool af_process_packet(const void* data, int len, const rf::NetAddr& addr, rf::Player* player);
void af_send_packet(rf::Player* player, const void* data, int len, bool is_reliable);

void af_send_ping_location_req_packet(rf::Vector3* pos);
static void af_process_ping_location_req_packet(const void* data, size_t len, const rf::NetAddr& addr);
void af_send_ping_location_packet_to_team(rf::Vector3* pos, uint8_t player_id, rf::ubyte team);
static void af_process_ping_location_packet(const void* data, size_t len, const rf::NetAddr& addr);
void af_send_damage_notify_packet(uint8_t player_id, float damage, bool died, rf::Player* player);
static void af_process_damage_notify_packet(const void* data, size_t len, const rf::NetAddr& addr);
void af_send_obj_update_packet(rf::Player* player);
static void af_process_obj_update_packet(const void* data, size_t len, const rf::NetAddr& addr);
