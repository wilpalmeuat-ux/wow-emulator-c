#ifndef WOW_LOG_H
#define WOW_LOG_H
#include <stdio.h>
typedef enum { LOG_DEBUG=0, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL } LogLevel;
extern LogLevel g_log_level;
extern FILE* g_log_file;
void log_init(const char* filename);
void log_set_level(LogLevel level);
void log_msg(LogLevel level, const char* fmt, ...);
#define LOG_DEBUG(fmt, ...) log_msg(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_msg(LOG_INFO,  fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_msg(LOG_WARN,  fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_msg(LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log_msg(LOG_FATAL, fmt, ##__VA_ARGS__)
#endif
