/* log.h -- Logging interface for the WoW emulator */
#ifndef WOW_LOG_H
#define WOW_LOG_H
#include <stdio.h>  /* FILE must be defined before extern FILE* declaration */
#include <stdarg.h>

typedef enum { LOG_TRACE=0, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL } LogLevel;

extern LogLevel g_log_level;
extern FILE* g_log_file;

void log_init(const char* path, LogLevel min_level);
void log_shutdown(void);
void log_set_level(LogLevel level);
void log_write(LogLevel level, const char* file, int line, const char* fmt, ...);

#define LOG_TRACE(...) log_write(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) log_write(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_write(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_write(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_write(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) log_write(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif
