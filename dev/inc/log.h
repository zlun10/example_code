#ifndef _LOG_H
#define _LOG_H

#include "../../lib/RTT/SEGGER_RTT.h"

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_m;

void log_init(void);
void log_deInit(void);
void log_print(log_level_m level, const char *format, ...);

#define LOG_DEBUG(format, ...) log_print(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  log_print(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  log_print(LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) log_print(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) log_print(LOG_LEVEL_FATAL, format, ##__VA_ARGS__)
#define LOG(format, ...)       SEGGER_RTT_TerminalOut(1, format, ##__VA_ARGS__);

#endif
