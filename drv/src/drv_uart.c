#include "../inc/drv_uart.h"
#include <stdbool.h>
#include <stdint.h>
#include "cw32l011_gpio.h"
#include "cw32l011_uart.h"
#include "cw32l011_sysctrl.h"
#include "../../drv/inc/drv_timer.h"
#include "../../dev/inc/log.h"
#include "../../lib/misc.h"
#include "../../config/config.h"
#include "../../drv/inc/drv_gpio.h"
#include "../../dev/inc/sys_tick.h"

UartRcv_s at_uart = { 0 };

UartRcv_s *getAtUart(void)
{
    return &at_uart;
}

void drv_logUart_init(void)
{
    // af gpio
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    GPIO_InitStructure.Pins = GPIO_PIN_15;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(CW_GPIOC, &GPIO_InitStructure);
    PC15_AFx_UART2TXD();

    // uart
    UART_InitTypeDef UART_InitStructure = { 0 };
    UART_InitStructure.UART_BaudRate = 921600;
    UART_InitStructure.UART_Over = UART_Over_16;
    UART_InitStructure.UART_Source = UART_Source_PCLK;
    UART_InitStructure.UART_UclkFreq = SYSCTRL_GetHClkFreq();
    UART_InitStructure.UART_StartBit = UART_StartBit_FE;
    UART_InitStructure.UART_StopBits = UART_StopBits_1;
    UART_InitStructure.UART_Parity = UART_Parity_No;
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    UART_InitStructure.UART_Mode = UART_Mode_Tx;
    UART_Init(CW_UART2, &UART_InitStructure);
}

void drv_logUart_deInit(void)
{
    UART2_DeInit();
    __SYSCTRL_UART2_CLK_DISABLE();
    GPIO_DeInit(CW_GPIOC, GPIO_PIN_15);
}


bool buadrate_scan(bool delayFlag)
{

    SysParam_t *sysParam = getSysParam();
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    GPIO_InitStructure.Pins = GPIO_PIN_9;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);
    PA09_AFx_UART1TXD();
    GPIO_InitStructure.Pins = GPIO_PIN_10;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);
    PA10_AFx_UART1RXD();

    UART_InitTypeDef UART_InitStructure = { 0 };
    UART_InitStructure.UART_Over = UART_Over_16;
    UART_InitStructure.UART_Source = UART_Source_PCLK;
    UART_InitStructure.UART_UclkFreq = SYSCTRL_GetHClkFreq();
    UART_InitStructure.UART_StartBit = UART_StartBit_FE;
    UART_InitStructure.UART_StopBits = UART_StopBits_1;
    UART_InitStructure.UART_Parity = UART_Parity_No;
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    UART_InitStructure.UART_Mode = UART_Mode_Tx | UART_Mode_Rx;

    if(delayFlag)
        timer_delayMs(500);

    UART_InitStructure.UART_BaudRate = MCU_BAUDRATE_INIT;
    UART1_DeInit();
    UART_Init(CW_UART1, &UART_InitStructure);
    drv_atUart_clear();

    uint8_t cnt = 0;
    while(cnt++ < 10) {
        UART_SendString(CW_UART1, "AT\r\n");
        uint32_t waitStart = millis();
        while((millis() - waitStart) < 120) {
            if(UART_GetFlagStatus(CW_UART1, UART_FLAG_RC) == SET) {
                UART_ClearFlag(CW_UART1, UART_FLAG_RC);
                uint8_t byte = UART_ReceiveData_8bit(CW_UART1);
                if(at_uart.line_len < (UART_LINE_BUF_LEN - 1)) {
                    at_uart.line_buffer[at_uart.line_len++] = byte;
                } else {
                    LOG_ERROR("scan buffer overflow");
                    break;
                }
            }
        }
        if(strstr((char *) at_uart.line_buffer, "AT_OK")) {
            sysParam->baudrate = MCU_BAUDRATE_INIT;
            return true;
        }
    }

    LOG_ERROR("MCU baudrate scan failed");
    sysParam->baudrate = MODULE_BAUDRATE_INIT;

    return false;

}

void drv_atUart_init(uint32_t baudrate)
{

    GPIO_InitTypeDef GPIO_InitStructure = { 0 };

    //  配置为 TX (输出)
    GPIO_InitStructure.Pins = GPIO_PIN_9;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);
    PA09_AFx_UART1TXD();

    //  配置为 RX (输入)
    GPIO_InitStructure.Pins = GPIO_PIN_10;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);
    PA10_AFx_UART1RXD();

    // 初始化环形缓冲区和行缓冲区
    // rb_init(&at_uart.rb, (uint8_t *) at_uart.rb_buffer, UART_RB_SIZE);
    at_uart.line_len = 0;
    at_uart.line_ready = false;
    memset((void *) at_uart.line_buffer, 0, sizeof(at_uart.line_buffer));

    // uart
    UART_InitTypeDef UART_InitStructure = { 0 };
    UART_InitStructure.UART_BaudRate = baudrate;
    UART_InitStructure.UART_Over = UART_Over_16;
    UART_InitStructure.UART_Source = UART_Source_PCLK;
    UART_InitStructure.UART_UclkFreq = SYSCTRL_GetHClkFreq();
    UART_InitStructure.UART_StartBit = UART_StartBit_FE;
    UART_InitStructure.UART_StopBits = UART_StopBits_1;
    UART_InitStructure.UART_Parity = UART_Parity_No;
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    UART_InitStructure.UART_Mode = UART_Mode_Tx | UART_Mode_Rx;
    UART_Init(CW_UART1, &UART_InitStructure);

    NVIC_SetPriority(UART1_IRQn, 1);
    NVIC_EnableIRQ(UART1_IRQn);
    UART_ITConfig(CW_UART1, UART_IT_RC, ENABLE);// 使能接收中断

}

void drv_atUart_deInit(void)
{

    NVIC_DisableIRQ(UART1_IRQn);
    UART_ITConfig(CW_UART1, UART_IT_RC, DISABLE);// 禁用接收中断
    UART1_DeInit();
    __SYSCTRL_UART1_CLK_DISABLE();
    GPIO_DeInit(CW_GPIOA, GPIO_PIN_10 | GPIO_PIN_9);

}

void drv_atUart_clear(void)
{
    // rb_init(&at_uart.rb, (uint8_t *) at_uart.rb_buffer, UART_RB_SIZE);
    at_uart.line_len = 0;
    at_uart.line_ready = false;
    memset((void *) at_uart.line_buffer, 0, sizeof(at_uart.line_buffer));
}

static void drv_atUart_clearLine(void)
{
    at_uart.line_len = 0;
    at_uart.line_ready = false;
    memset((void *) at_uart.line_buffer, 0, sizeof(at_uart.line_buffer));
}

void drv_atUart_sendSTR(char *str)
{
    UART_SendString(CW_UART1, str);
    UART_SendString(CW_UART1, "\r\n");

}

bool drv_atUart_getLine(char *out_buf, uint16_t buf_size)
{
    if(!at_uart.line_ready || out_buf == NULL || buf_size == 0) {
        return false;
    }

    uint16_t copy_len = at_uart.line_len;
    if(copy_len >= buf_size) {
        copy_len = buf_size - 1;
    }

    memcpy(out_buf, (void *) at_uart.line_buffer, copy_len);
    out_buf[copy_len] = '\0';

    drv_atUart_clearLine();

    return true;
}

// 处理一字节的时间不超过下一个字节收到的时间 87us (115200bps)
void UART1_IRQHandler(void)
{
    if(UART_GetITStatus(CW_UART1, UART_IT_RC) != RESET)
    {
        uint8_t byte = UART_ReceiveData_8bit(CW_UART1);

        if(!at_uart.line_ready) {
            if(at_uart.line_len < (UART_LINE_BUF_LEN - 1)) {
                at_uart.line_buffer[at_uart.line_len++] = byte;

                // 检测到完整的一行（\r\n结尾）
                if(at_uart.line_len >= 2) {
                    if(at_uart.line_buffer[at_uart.line_len - 2] == '\r' &&
                        at_uart.line_buffer[at_uart.line_len - 1] == '\n') {
                        at_uart.line_buffer[at_uart.line_len] = '\0';
                        at_uart.line_ready = true;
                    }
                }
            } else {
                // 行缓冲区溢出，清空重新开始
                LOG_ERROR("UART line buffer overflow");
                at_uart.line_len = 0;
                memset((void *) at_uart.line_buffer, 0, sizeof(at_uart.line_buffer));
            }
        }
        UART_ClearITPendingBit(CW_UART1, UART_IT_RC);
    }
}

