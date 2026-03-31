/**
 * @file hal-at.c
 * @brief AT 通信硬件抽象层（HAL）— CW32L011 平台实现
 *
 * 本文件实现了 hal-at.h 中定义的所有 AT UART 操作接口，
 * 依赖 CW32L011 的 UART 驱动（drv_uart）。
 *
 * 移植说明：
 *   将本文件中的所有函数替换为目标芯片/平台的对应 UART 实现即可。
 *   上层 dev/src/at.c 不需要做任何修改。
 */

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "../inc/hal-at.h"
#include "../../drv/inc/drv_uart.h"
#include "../../config/config.h"

void hal_at_init(void)
{
    const SysParam_t *sysParam = getSysParam();
    drv_atUart_init(sysParam->baudrate);
}

void hal_at_deInit(void)
{
    drv_atUart_deInit();
}

void hal_at_sendData(const char *format, ...)
{
    char printf_buf[30];
    va_list args;
    va_start(args, format);
    vsnprintf(printf_buf, sizeof(printf_buf), format, args);
    va_end(args);
    drv_atUart_sendSTR(printf_buf);
}

void hal_at_clear(void)
{
    drv_atUart_clear();
}

uint8_t *hal_at_buff(void)
{
    return (uint8_t *) getAtUart()->line_buffer;
}

bool hal_at_is_line_ready(void)
{
    return getAtUart()->line_ready;
}

bool hal_at_get_line(char *out_buf, uint16_t buf_size)
{
    return drv_atUart_getLine(out_buf, buf_size);
}

void hal_at_reinit_baudrate(uint32_t baudrate)
{
    drv_atUart_init(baudrate);
}
