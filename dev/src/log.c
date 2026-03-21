#include "../inc/log.h"
#include <stdarg.h>
#include <stdio.h>
#include "cw32l011_uart.h"
#include "../../config/config.h"
#include "../../drv/inc/drv_uart.h"
#include "../../lib/RTT/SEGGER_RTT.h"
#include "../../lib/misc.h"
#include "../../dev/inc/sys_tick.h"

#ifdef EN_SW_DEBUG
#define RTT_EN 1
#else
#define RTT_EN 0
#endif

#define UART_LOG_EN 0

#define COLOR_EN 0
#define TIMESTAMP_EN 1

// 日志等级颜色定义（ANSI颜色代码）
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_MAGENTA "\033[35m"

log_level_m current_log_level = LOG_LEVEL_DEBUG;

const char *log_level_color(log_level_m level) {
    switch(level) {
    case LOG_LEVEL_DEBUG: return COLOR_BLUE;
    case LOG_LEVEL_INFO:  return COLOR_GREEN;
    case LOG_LEVEL_WARN:  return COLOR_YELLOW;
    case LOG_LEVEL_ERROR: return COLOR_RED;
    case LOG_LEVEL_FATAL: return COLOR_MAGENTA;
    default: return COLOR_RESET;
    }
}

static UNUSED char *uint32_to_string(uint32_t value, char *buffer) {
    if(value == 0) {
        *buffer++ = '0';
        *buffer = '\0';
        return buffer;
    }

    char *p = buffer;
    char temp[12];
    int i = 0;

    while(value > 0) {
        uint32_t q = value / 10;
        uint32_t r = value - (q * 10);
        temp[i++] = r + '0';
        value = q;
    }

    while(i > 0) {
        *p++ = temp[--i];
    }
    *p = '\0';
    return buffer;
}

void log_init(void)
{
#if RTT_EN
    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(0, "debug", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    SEGGER_RTT_ConfigUpBuffer(1, "info", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    SEGGER_RTT_ConfigUpBuffer(2, "error", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);

    SEGGER_RTT_printf(0, "segger init success\r\n");
    LOG("--------------segger init success------------------\r\n");
    SEGGER_RTT_TerminalOut(2, "------------------segger init success-------------------\r\n");
#elif UART_LOG_EN
    drv_logUart_init();
#endif

    LOG_DEBUG("Log Init");
}

void log_deInit(void)
{
#if RTT_EN
    // SEGGER_RTT_Init();
#elif UART_LOG_EN
    drv_logUart_deInit();
#endif
}

void log_print(log_level_m level, const char *format, ...) {
    if(level < current_log_level) {
        return;
    }

#if RTT_EN
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

#if COLOR_EN
    SEGGER_RTT_printf(0, "%s%s %s\n", log_level_color(level), COLOR_RESET, buffer);
#else

#if TIMESTAMP_EN
    uint32_t timestamp = millis();
    char timestamp_str[12];
    uint32_to_string(timestamp, timestamp_str);
    SEGGER_RTT_printf(0, "%s %s\n", timestamp_str, buffer);
#else
    SEGGER_RTT_printf(0, "%s\n", buffer);
#endif

#endif

#elif UART_LOG_EN
    printf("%s[%s]%s ", log_level_color(level), COLOR_RESET);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
#endif
}

#if UART_LOG_EN
int fputc(int ch, FILE *f)
{
    // 将 \n 转换为 \r\n，便于串口终端显示
    if(ch == '\n') {
        while(UART_GetFlagStatus(CW_UART2, UART_FLAG_TXE) == RESET) {}
        UART_SendData_8bit(CW_UART2, '\r');
    }
    while(UART_GetFlagStatus(CW_UART2, UART_FLAG_TXE) == RESET) {}
    UART_SendData_8bit(CW_UART2, (uint8_t) ch);
    return ch;
}
#endif
