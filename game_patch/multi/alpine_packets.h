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
    af_ping_location_req = 0x50,
    af_ping_location = 0x51,
};

struct af_ping_location_req_packet
{
    RF_GamePacketHeader header;
    struct RF_Vector pos;
};

struct af_ping_location_packet
{
    RF_GamePacketHeader header;
    uint8_t player_id;
    struct RF_Vector pos;
};

bool af_process_packet(const void* data, int len, const rf::NetAddr& addr, rf::Player* player);
void af_send_packet(rf::Player* player, const void* data, int len, bool is_reliable);

void af_send_ping_location_req_packet(rf::Vector3* pos);
static void af_process_ping_location_req_packet(const void* data, size_t len, const rf::NetAddr& addr);
void af_send_ping_location_packet_to_team(rf::Vector3* pos, uint8_t player_id, rf::ubyte team);
static void af_process_ping_location_packet(const void* data, size_t len, const rf::NetAddr& addr);


#pragma pack(pop)
