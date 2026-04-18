#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static FILE* g_fp;
static LogLevel g_min_level;

void log_init(const char* path, LogLevel min_level) {
    g_min_level = min_level;
    if (path) {
        g_fp = fopen(path, "a");
    } else {
        g_fp = NULL;
    }
}

void log_shutdown(void) {
    if (g_fp) fclose(g_fp);
}

static const char* level_str(LogLevel lvl) {
    switch (lvl) {
        case LOG_TRACE: return "TRACE";
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO ";
        case LOG_WARN:  return "WARN ";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default:        return "???? ";
    }
}

void log_write(LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (level < g_min_level) return;

    time_t now = time(NULL);
    struct tm tm_buf;
#ifdef _WIN32
    struct tm* tm = localtime_s(&tm_buf, &now) == 0 ? &tm_buf : &tm_buf;
#else
    struct tm* tm = localtime_r(now, &tm_buf);
#endif

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(stdout, "[%s][%s][%s:%d] ", timestamp, level_str(level), file, line);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);

    fprintf(stdout, "\n");

    if (g_fp) {
        fprintf(g_fp, "[%s][%s][%s:%d] ", timestamp, level_str(level), file, line);
        va_start(ap, fmt);
        vfprintf(g_fp, fmt, ap);
        va_end(ap);
        fprintf(g_fp, "\n");
        fflush(g_fp);
    }
}
