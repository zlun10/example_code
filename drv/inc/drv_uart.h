#ifndef _DRV_UART_H
#define _DRV_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "../../lib/rb.h"

#define UART_RB_SIZE       128  // 环形缓冲区大小
#define UART_LINE_BUF_LEN  100  // 单行缓冲区大小

typedef struct {
    rb_t rb;                                        // 环形缓冲区
    // uint8_t rb_buffer[UART_RB_SIZE];
    volatile char line_buffer[UART_LINE_BUF_LEN];   // 单行数据缓冲区
    volatile uint8_t line_len;                      // 当前行长度
    volatile bool line_ready;                       // 一行数据就绪标志
    // volatile uint32_t last_rcv_tick;
} UartRcv_s;

// event
bool buadrate_scan(bool delayFlag);
void drv_atUart_init(uint32_t baudrate);
void drv_atUart_deInit(void);
void drv_atUart_clear(void);
void drv_atUart_clearLine(void);
void drv_atUart_sendSTR(char *str);
bool drv_atUart_getLine(char *out_buf, uint16_t buf_size);

void drv_logUart_init(void);
void drv_logUart_deInit(void);

UartRcv_s *getAtUart(void);

#endif
