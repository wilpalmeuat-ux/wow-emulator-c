#ifndef _STUB_WINSOCK2_H
#define _STUB_WINSOCK2_H
#include <stdint.h>
#include <string.h>
typedef uintptr_t SOCKET;
typedef int BOOL;
typedef unsigned long DWORD;
typedef void* HANDLE;
typedef void* LPVOID;
typedef unsigned long u_long;
#define INVALID_SOCKET ((SOCKET)(~0))
#define SOCKET_ERROR (-1)
#define IPPROTO_TCP 6
#define TCP_NODELAY 1
#define SOL_SOCKET 0xFFFF
#define SO_REUSEADDR 4
#define AF_INET 2
#define SOCK_STREAM 1
#define INADDR_ANY 0
#define SD_RECEIVE 0
#define SD_SEND 1
#define SD_BOTH 2
#define FIONBIO 0x5421
#define INET_ADDRSTRLEN 16
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define WINAPI
struct in_addr { unsigned long s_addr; };
struct sockaddr_in { short sin_family; unsigned short sin_port; struct in_addr sin_addr; char pad[8]; };
struct sockaddr { short sa_family; char sa_data[14]; };
typedef int socklen_t;
static inline SOCKET socket(int a,int b,int c){(void)a;(void)b;(void)c;return 0;}
static inline int bind(SOCKET s,const struct sockaddr*a,int l){(void)s;(void)a;(void)l;return 0;}
static inline int listen(SOCKET s,int b){(void)s;(void)b;return 0;}
static inline SOCKET accept(SOCKET s,struct sockaddr*a,int*l){(void)s;(void)a;(void)l;return 0;}
static inline int send(SOCKET s,const char*b,int l,int f){(void)s;(void)b;(void)l;(void)f;return l;}
static inline int recv(SOCKET s,char*b,int l,int f){(void)s;(void)b;(void)l;(void)f;return 0;}
static inline int closesocket(SOCKET s){(void)s;return 0;}
static inline int setsockopt(SOCKET s,int l,int o,const char*v,int vl){(void)s;(void)l;(void)o;(void)v;(void)vl;return 0;}
static inline int ioctlsocket(SOCKET s,long cmd,u_long*a){(void)s;(void)cmd;(void)a;return 0;}
static inline unsigned short htons(unsigned short v){return v;}
static inline const char*inet_ntop(int af,const void*src,char*dst,int sz){(void)af;(void)src;(void)sz;dst[0]=0;return dst;}
static inline int WSAGetLastError(void){return 0;}
typedef struct{int a;}WSADATA;
static inline int WSAStartup(int v,WSADATA*d){(void)v;(void)d;return 0;}
static inline void WSACleanup(void){}
#endif
