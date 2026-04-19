/* Logging.h -- Windows-compatible logging macros */
#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

enum LogLevel { LOG_TRACE_LVL = 0, LOG_DEBUG_LVL, LOG_INFO_LVL, LOG_WARN_LVL, LOG_ERROR_LVL, LOG_CRIT_LVL };

extern enum LogLevel g_logLevel;

#define LOG_ERROR(fmt, ...)  do { fprintf(stderr, "[ERROR] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)
#define LOG_WARN(fmt, ...)   do { if (g_logLevel <= LOG_WARN_LVL)  fprintf(stderr, "[WARN]  " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_INFO(fmt, ...)   do { if (g_logLevel <= LOG_INFO_LVL)  fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_DEBUG(fmt, ...)  do { if (g_logLevel <= LOG_DEBUG_LVL) fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)

#endif
