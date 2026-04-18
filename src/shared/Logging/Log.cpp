#include "Logging/Log.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>

static FILE* g_logFile = nullptr;
static LogLevel g_minLevel = LOG_INFO;

void Log_Init(const std::string& file) {
    if (!file.empty()) g_logFile = fopen(file.c_str(), "a");
}
void Log_Close() { if (g_logFile) { fclose(g_logFile); g_logFile = nullptr; } }
void Log_SetLevel(LogLevel l) { g_minLevel = l; }

void Log_Message(LogLevel level, const char*, int, const char* fmt, ...) {
    if (level < g_minLevel) return;
    const char* LSTR[] = { "DEBUG", "INFO ", "WARN ", "ERROR", "FATAL" };
    time_t now = time(nullptr);
    struct tm t; localtime_r(&now, &t);
    char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &t);
    FILE* out = g_logFile ? g_logFile : stderr;
    fprintf(out, "[%s] [%s] ", ts, LSTR[level]);
    va_list args; va_start(args, fmt); vfprintf(out, fmt, args); va_end(args);
    fprintf(out, "\n"); fflush(out);
}
