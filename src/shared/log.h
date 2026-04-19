/* log.h -- Logging interface for the WoW emulator */
#ifndef WOW_LOG_H
#define WOW_LOG_H

#include <stdio.h>

typedef enum { LOG_DEBUG=0, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL } LogLevel;

extern LogLevel g_log_level;
extern FILE* g_log_file;

void log_init(const char* filename);
void log_set_level(LogLevel level);
void log_msg(LogLevel level, const char* fmt, ...);

/* Convenience macros -- these do NOT clash with Logging.h because
 * Logging.h defines LOG_DEBUG(...) as a do{} while(0) macro and
 * log.h defines log_msg() as a function.  Code should prefer
 * the Logging.h macros or call log_msg() directly. */

#endif
