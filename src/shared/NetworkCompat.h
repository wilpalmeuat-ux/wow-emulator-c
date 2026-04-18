/* NetworkCompat.h — Windows network + thread compatibility layer
 * WinSock2 + Windows API only. No POSIX.
 */
#ifndef _NETWORK_COMPAT_H
#define _NETWORK_COMPAT_H

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

typedef int socklen_t;
typedef SOCKET socket_t;
#define SOCKET_INVALID INVALID_SOCKET
#define SOCKET_ERROR_VAL SOCKET_ERROR
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
#define closesocket closesocket

#define SOCKET_IS_ERROR(s) ((s) == INVALID_SOCKET || (s) == SOCKET_ERROR)

/* pthread stubs (for source compatibility with code that expects pthreads) */
#include <pthread.h>

/* log_init conflict — rename on Windows */
#define log_init windows_log_init

#endif