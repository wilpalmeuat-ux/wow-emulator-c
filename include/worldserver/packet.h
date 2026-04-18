#include <stddef.h>
#ifndef WORLD_PACKET_H
#define WORLD_PACKET_H
#include <stdint.h>
#define MAX_PACKET_SIZE 32767
#define MAX_CLIENTS 1000
typedef struct {
    uint32_t opcode;
    uint32_t size;
    uint8_t data[MAX_PACKET_SIZE];
} WorldPacket;
typedef struct {
    uint64_t guid;
    uint32_t map_id;
    float position_x;
    float position_y;
    float position_z;
    float orientation;
    uint32_t zone;
} PlayerPosition;
void send_packet(uint32_t client_fd, WorldPacket* pkt);
void send_to_all(WorldPacket* pkt);
void send_smsg_char_enum(uint32_t client_fd);
void send_smsg_login_verify_world(uint32_t client_fd, uint32_t mapid, float x, float y, float z);
void send_smsg_update_object(uint32_t client_fd, const uint8_t* data, size_t len);
void send_smsg_spell_cooldown(uint32_t client_fd, uint32_t spell_id, uint32_t ms);
void build_smsg_update(WorldPacket* pkt, const uint8_t* update_data, size_t update_len);
#endif