#include <stdint.h>
#include <stdarg.h>
#include "../inc/hal-at.h"
#include "../../drv/inc/drv_uart.h"
#include "../../config/config.h"

void hal_at_init(void)
{
    const SysParam_t *sysParam = getSysParam();
    drv_atUart_init(sysParam->baudrate);
    module_notify_init();
}

void hal_at_deInit(void)
{
    drv_atUart_deInit();
    module_notify_deInit();
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
