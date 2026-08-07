/* NetworkCompat.h -- Windows-only network + thread compatibility layer
 * WinSock2 + Windows API. No POSIX.
 */
#ifndef _NETWORK_COMPAT_H
#define _NETWORK_COMPAT_H

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

typedef int socklen_t;
typedef SOCKET socket_t;
#define SOCKET_INVALID INVALID_SOCKET
#define SOCKET_ERROR_VAL SOCKET_ERROR
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
/* closesocket is already defined by WinSock2 */

#define SOCKET_IS_ERROR(s) ((s) == INVALID_SOCKET || (s) == SOCKET_ERROR)

/* ----------------------------------------------------------------
 *  Lightweight pthread-compatible wrappers using Windows API
 *  These allow existing code that uses pthread_* to compile on
 *  Windows without pulling in a full pthreads-win32 library.
 * ---------------------------------------------------------------- */
#ifndef _PTHREAD_COMPAT_DEFINED
#define _PTHREAD_COMPAT_DEFINED

typedef HANDLE            pthread_t;
typedef CRITICAL_SECTION  pthread_mutex_t;
typedef void*             pthread_attr_t;
typedef void*             pthread_mutexattr_t;

typedef struct {
    HANDLE event;
    int    waiters;
} pthread_cond_t;

/* Thread */
static inline int pthread_create(pthread_t* t, const pthread_attr_t* attr,
                                 void* (*start)(void*), void* arg) {
    (void)attr;
    *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start, arg, 0, NULL);
    return (*t == NULL) ? -1 : 0;
}
static inline int pthread_join(pthread_t t, void** retval) {
    (void)retval;
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
    return 0;
}
static inline int pthread_detach(pthread_t t) {
    CloseHandle(t);
    return 0;
}

/* Mutex */
static inline int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t* a) {
    (void)a;
    InitializeCriticalSection(m);
    return 0;
}
static inline int pthread_mutex_destroy(pthread_mutex_t* m) {
    DeleteCriticalSection(m);
    return 0;
}
static inline int pthread_mutex_lock(pthread_mutex_t* m) {
    EnterCriticalSection(m);
    return 0;
}
static inline int pthread_mutex_unlock(pthread_mutex_t* m) {
    LeaveCriticalSection(m);
    return 0;
}

/* Condition variable (simplified) */
static inline int pthread_cond_init(pthread_cond_t* c, const void* attr) {
    (void)attr;
    c->event = CreateEvent(NULL, TRUE, FALSE, NULL);
    c->waiters = 0;
    return 0;
}
static inline int pthread_cond_destroy(pthread_cond_t* c) {
    CloseHandle(c->event);
    return 0;
}
static inline int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m) {
    c->waiters++;
    LeaveCriticalSection(m);
    WaitForSingleObject(c->event, INFINITE);
    EnterCriticalSection(m);
    c->waiters--;
    if (c->waiters == 0) ResetEvent(c->event);
    return 0;
}
static inline int pthread_cond_signal(pthread_cond_t* c) {
    SetEvent(c->event);
    return 0;
}
static inline int pthread_cond_broadcast(pthread_cond_t* c) {
    SetEvent(c->event);
    return 0;
}

#endif /* _PTHREAD_COMPAT_DEFINED */

/* Sleep compat */
#define usleep(us) Sleep((us) / 1000)

#else
/* Fallback for non-Windows (should not be used in production) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
typedef int socket_t;
#define SOCKET_INVALID (-1)
#define SOCKET_IS_ERROR(s) ((s) < 0)
#define closesocket close
#endif

/* Opcode table size for handler registration */
#ifndef OPCODE_TABLE_SIZE
#define OPCODE_TABLE_SIZE 0x8000
#endif

#endif /* _NETWORK_COMPAT_H */
