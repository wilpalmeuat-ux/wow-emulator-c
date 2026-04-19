#ifndef _STUB_WINDOWS_H
#define _STUB_WINDOWS_H
#include <stdint.h>
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define MAX_PATH 260
#define WINAPI
typedef unsigned long DWORD;
typedef int BOOL;
typedef void* HANDLE;
typedef void* LPVOID;
typedef struct{int a;}CRITICAL_SECTION;
static inline void InitializeCriticalSection(CRITICAL_SECTION*c){(void)c;}
static inline void DeleteCriticalSection(CRITICAL_SECTION*c){(void)c;}
static inline void EnterCriticalSection(CRITICAL_SECTION*c){(void)c;}
static inline void LeaveCriticalSection(CRITICAL_SECTION*c){(void)c;}
static inline HANDLE CreateThread(void*a,int b,void*fn,void*arg,int f,void*id){(void)a;(void)b;(void)fn;(void)arg;(void)f;(void)id;return(HANDLE)1;}
static inline DWORD WaitForSingleObject(HANDLE h,DWORD ms){(void)h;(void)ms;return 0;}
static inline int CloseHandle(HANDLE h){(void)h;return 1;}
static inline void Sleep(DWORD ms){(void)ms;}
static inline BOOL SetConsoleCtrlHandler(void*fn,BOOL a){(void)fn;(void)a;return 1;}
#define MAKEWORD(a,b) (((a)<<8)|(b))
typedef struct{char cFileName[260];}WIN32_FIND_DATAA;
static inline HANDLE FindFirstFileA(const char*p,WIN32_FIND_DATAA*d){(void)p;(void)d;return(HANDLE)(uintptr_t)-1;}
static inline int FindNextFileA(HANDLE h,WIN32_FIND_DATAA*d){(void)h;(void)d;return 0;}
static inline void FindClose(HANDLE h){(void)h;}
#endif
#define INVALID_HANDLE_VALUE ((HANDLE)(uintptr_t)-1)
