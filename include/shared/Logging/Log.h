#pragma once

#include <string>
#include <cstdint>

enum LogLevel { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

void Log_Init(const std::string& file = "");
void Log_Close();
void Log_SetLevel(LogLevel level);
void Log_Message(LogLevel level, const char* file, int line, const char* fmt, ...);

#define LOG_DEBUG(fmt, ...) Log_Message(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Log_Message(LOG_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Log_Message(LOG_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Log_Message(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) Log_Message(LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
