#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

enum LogLevel { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_CRIT };

extern enum LogLevel g_logLevel;

#define LOG_ERROR(fmt, ...)  do { fprintf(stderr, "[ERROR] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while(0)
#define LOG_WARN(fmt, ...)   do { if (g_logLevel <= LOG_WARN)  fprintf(stderr, "[WARN]  " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_INFO(fmt, ...)   do { if (g_logLevel <= LOG_INFO)  fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOG_DEBUG(fmt, ...)  do { if (g_logLevel <= LOG_DEBUG) fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while(0)

#endif
