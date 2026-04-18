/* wincompat.c — Windows POSIX compatibility shims */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

/* Sleep is Windows-native */
#define sleep(x) Sleep((x) * 1000)

/* getpid */
int getpid(void) { return (int)GetCurrentProcessId(); }

/* gettimeofday — not available on Windows */
int gettimeofday(struct timeval* tv, void* tz) {
    struct _timeb tb;
    memset(&tb, 0, sizeof(tb));
    _ftime_s(&tb);
    if (tv) {
        tv->tv_sec = (long)tb.time;
        tv->tv_usec = (long)(tb.millitm * 1000);
    }
    (void)tz;
    return 0;
}

/* inet_pton shim (Windows has it but not always available) */
#ifndef inet_pton
int inet_pton(int af, const char* src, void* dst) {
    if (af == AF_INET) {
        struct in_addr addr4;
        if (InetPton(AF_INET, src, &addr4) == 1) {
            *(struct in_addr*)dst = addr4;
            return 1;
        }
        return 0;
    }
    if (af == AF_INET6) {
        struct in_addr6 addr6;
        if (InetPton(AF_INET6, src, &addr6) == 1) {
            *(struct in_addr6*)dst = addr6;
            return 1;
        }
        return 0;
    }
    return -1;
}
#endif

/* pthread shims for Windows */
typedef struct {
    HANDLE handle;
    void* (*start_routine)(void*);
    void* arg;
} pthread_t_internal;

static DWORD WINAPI _pthread_start(LPVOID arg) {
    pthread_t_internal* info = (pthread_t_internal*)arg;
    void* ret = info->start_routine(info->arg);
    free(info);
    return 0;
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    (void)attr;
    pthread_t_internal* info = (pthread_t_internal*)malloc(sizeof(pthread_t_internal));
    if (!info) return EAGAIN;
    info->start_routine = start_routine;
    info->arg = arg;
    HANDLE h = CreateThread(NULL, 0, _pthread_start, info, 0, NULL);
    if (!h) { free(info); return EAGAIN; }
    *thread = (pthread_t)h;
    return 0;
}

int pthread_join(pthread_t thread, void** retval) {
    (void)retval;
    WaitForSingleObject((HANDLE)thread, INFINITE);
    CloseHandle((HANDLE)thread);
    return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    (void)attr;
    InitializeCriticalSection(mutex);
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    DeleteCriticalSection(mutex);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    EnterCriticalSection(mutex);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    LeaveCriticalSection(mutex);
    return 0;
}

/* strcasecmp / stricmp */
int strcasecmp(const char* s1, const char* s2) {
    return _stricmp(s1, s2);
}

int strncasecmp(const char* s1, const char* s2, size_t n) {
    return _strnicmp(s1, s2, n);
}

/* getpagesize */
int getpagesize(void) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwPageSize;
}

/* localtime_r */
struct tm* localtime_r(const time_t* timer, struct tm* result) {
    struct tm* tmp = localtime(timer);
    if (tmp && result) {
        *result = *tmp;
        return result;
    }
    return tmp;
}

/* gmtime_r */
struct tm* gmtime_r(const time_t* timer, struct tm* result) {
    struct tm* tmp = gmtime(timer);
    if (tmp && result) {
        *result = *tmp;
        return result;
    }
    return tmp;
}
