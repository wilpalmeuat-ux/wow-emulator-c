/* log.c -- Windows-compatible logging implementation */
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

static FILE* g_fp = NULL;
LogLevel g_log_level = LOG_DEBUG;
FILE* g_log_file = NULL;

void log_init(const char* path, LogLevel min_level) {
    g_log_level = min_level;
    if (path) {
        g_fp = fopen(path, "a");
        g_log_file = g_fp;
    }
}

void log_shutdown(void) {
    if (g_fp && g_fp != stdout) {
        fclose(g_fp);
        g_fp = NULL;
    }
}

void log_set_level(LogLevel level) {
    g_log_level = level;
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

static void log_vwrite(LogLevel level, const char* file, int line, const char* fmt, va_list ap) {
    if (level < g_log_level) return;

    time_t now = time(NULL);
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_buf);

    fprintf(stdout, "[%s][%s] ", timestamp, level_str(level));
    if (file) fprintf(stdout, "[%s:%d] ", file, line);

    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);

    if (g_fp) {
        fprintf(g_fp, "[%s][%s] ", timestamp, level_str(level));
        if (file) fprintf(g_fp, "[%s:%d] ", file, line);
        vfprintf(g_fp, fmt, ap);
        fprintf(g_fp, "\n");
        fflush(g_fp);
    }
}

void log_write(LogLevel level, const char* file, int line, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_vwrite(level, file, line, fmt, ap);
    va_end(ap);
}
