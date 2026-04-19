/* Stub Windows headers for Linux compilation testing only */
#ifndef _STUB_WINDOWS_H
#define _STUB_WINDOWS_H

/* WinSock2 stubs */
typedef unsigned long long SOCKET;
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
#define SO_REUSEADDR 0x0004
#define AF_INET 2
#define SOCK_STREAM 1
#define INADDR_ANY 0
#define SD_RECEIVE 0
#define SD_SEND 1
#define SD_BOTH 2
#define FIONBIO 0x5421
#define INET_ADDRSTRLEN 16
#define TRUE 1
#define FALSE 0
#define WINAPI
#define MAX_PATH 260

struct sockaddr_in { int sin_family; unsigned short sin_port; struct { unsigned long s_addr; } sin_addr; char padding[8]; };
struct sockaddr { int sa_family; char sa_data[14]; };
typedef int socklen_t;

static inline SOCKET socket(int a, int b, int c) { (void)a;(void)b;(void)c; return 0; }
static inline int bind(SOCKET s, const struct sockaddr* a, int l) { (void)s;(void)a;(void)l; return 0; }
static inline int listen(SOCKET s, int b) { (void)s;(void)b; return 0; }
static inline SOCKET accept(SOCKET s, struct sockaddr* a, int* l) { (void)s;(void)a;(void)l; return 0; }
static inline int send(SOCKET s, const char* b, int l, int f) { (void)s;(void)b;(void)l;(void)f; return l; }
static inline int recv(SOCKET s, char* b, int l, int f) { (void)s;(void)b;(void)l;(void)f; return 0; }
static inline int closesocket(SOCKET s) { (void)s; return 0; }
static inline int setsockopt(SOCKET s, int l, int o, const char* v, int vl) { (void)s;(void)l;(void)o;(void)v;(void)vl; return 0; }
static inline int ioctlsocket(SOCKET s, long cmd, u_long* a) { (void)s;(void)cmd;(void)a; return 0; }
static inline unsigned short htons(unsigned short v) { return v; }
static inline const char* inet_ntop(int af, const void* src, char* dst, int size) { (void)af;(void)src;(void)size; dst[0]=0; return dst; }
static inline int WSAGetLastError(void) { return 0; }
typedef struct { int a; } WSADATA;
static inline int WSAStartup(int v, WSADATA* d) { (void)v;(void)d; return 0; }
static inline void WSACleanup(void) {}

/* Windows thread stubs */
typedef struct { int a; } CRITICAL_SECTION;
static inline void InitializeCriticalSection(CRITICAL_SECTION* c) { (void)c; }
static inline void DeleteCriticalSection(CRITICAL_SECTION* c) { (void)c; }
static inline void EnterCriticalSection(CRITICAL_SECTION* c) { (void)c; }
static inline void LeaveCriticalSection(CRITICAL_SECTION* c) { (void)c; }
static inline HANDLE CreateThread(void* a, int b, void* fn, void* arg, int f, void* id) { (void)a;(void)b;(void)fn;(void)arg;(void)f;(void)id; return (HANDLE)1; }
static inline DWORD WaitForSingleObject(HANDLE h, DWORD ms) { (void)h;(void)ms; return 0; }
static inline int CloseHandle(HANDLE h) { (void)h; return 1; }
static inline void Sleep(DWORD ms) { (void)ms; }
static inline BOOL SetConsoleCtrlHandler(void* fn, BOOL add) { (void)fn;(void)add; return 1; }
typedef DWORD MAKEWORD_t;
#define MAKEWORD(a,b) (((a)<<8)|(b))

/* MySQL stubs */
typedef struct { int dummy; } MYSQL;
typedef struct { int dummy; } MYSQL_RES;
typedef char** MYSQL_ROW;
static inline MYSQL* mysql_init(MYSQL* m) { (void)m; return (MYSQL*)1; }
static inline MYSQL* mysql_real_connect(MYSQL* m, const char* h, const char* u, const char* p, const char* d, int port, void* s, int f) { (void)m;(void)h;(void)u;(void)p;(void)d;(void)port;(void)s;(void)f; return m; }
static inline int mysql_query(MYSQL* m, const char* q) { (void)m;(void)q; return 0; }
static inline MYSQL_RES* mysql_store_result(MYSQL* m) { (void)m; return 0; }
static inline int mysql_num_fields(MYSQL_RES* r) { (void)r; return 0; }
static inline int mysql_num_rows(MYSQL_RES* r) { (void)r; return 0; }
static inline MYSQL_ROW mysql_fetch_row(MYSQL_RES* r) { (void)r; return 0; }
static inline void mysql_free_result(MYSQL_RES* r) { (void)r; }
static inline void mysql_close(MYSQL* m) { (void)m; }
static inline const char* mysql_error(MYSQL* m) { (void)m; return "stub"; }
static inline unsigned long mysql_real_escape_string(MYSQL* m, char* to, const char* from, unsigned long len) { (void)m; if(to&&from) strncpy(to,from,len); return len; }
static inline int mysql_select_db(MYSQL* m, const char* d) { (void)m;(void)d; return 0; }
static inline unsigned long long mysql_insert_id(MYSQL* m) { (void)m; return 0; }

/* WIN32_FIND_DATA stubs */
typedef struct { char cFileName[260]; } WIN32_FIND_DATAA;
static inline HANDLE FindFirstFileA(const char* p, WIN32_FIND_DATAA* d) { (void)p;(void)d; return (HANDLE)-1; }
static inline int FindNextFileA(HANDLE h, WIN32_FIND_DATAA* d) { (void)h;(void)d; return 0; }
static inline void FindClose(HANDLE h) { (void)h; }

#endif
