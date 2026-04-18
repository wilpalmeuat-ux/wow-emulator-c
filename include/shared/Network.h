#include "shared/NetworkCompat.h"
#ifndef NETWORK_H
#define NETWORK_H

#include "Types.h"
#include "ByteBuffer.h"

typedef void (*PacketHandler)(uint8* hdr, ByteBuffer* body, void* ctx);

typedef struct {
    int              fd;
    struct sockaddr_in addr;
    bool             alive;
    bool             authenticated;
    uint8            recvBuf[16384];
    uint32           recvLen;
    pthread_mutex_t  writeLock;
    /* auth */
    char             username[65];
    uint32           accountId;
    /* world */
    ObjectGuid       playerGuid;
    uint32           currentMap;
    float            position[4];
    uint8            build[4];
    struct ClientSocket* next;
} ClientSocket;

ClientSocket* ClientSocket_New(int fd, struct sockaddr_in* addr);
void          ClientSocket_Delete(ClientSocket* cs);
void          ClientSocket_Send(ClientSocket* cs, uint16 opcode, ByteBuffer* data);
void          ClientSocket_SendRaw(ClientSocket* cs, const uint8* data, uint32 len);
void          ClientSocket_Receive(ClientSocket* cs);
bool          ClientSocket_Tick(ClientSocket* cs);
const char*   ClientSocket_GetIP(ClientSocket* cs);

typedef struct {
    int              listenFd;
    uint16           port;
    volatile bool    running;
    void*            context;
    PacketHandler    handlers[OPCODE_TABLE_SIZE];
    pthread_mutex_t  clientsLock;
    ClientSocket*    clients;
    pthread_t        acceptThread;
    pthread_t        tickThread;
} Server;

Server* Server_New(uint16 port, void* ctx);
void    Server_Delete(Server* s);
bool    Server_Start(Server* s);
void    Server_Stop(Server* s);
void    Server_RegisterHandler(Server* s, uint16 opcode, PacketHandler fn);
void    Server_Broadcast(Server* s, uint16 opcode, ByteBuffer* buf);

#endif
