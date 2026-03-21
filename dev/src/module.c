/*
rst       -- modlue MCRL
power out -- module power control
at_isr    -- module rsq
*/
#include <stdint.h>
#include "../inc/module.h"
#include "../../drv/inc/drv_gpio.h"
#include "../../drv/inc/drv_uart.h"
#include "../../lib/misc.h"
#include "../../lib/storage.h"
#include "../../dev/inc/at.h"
#include "../../dev/inc/sys_tick.h"
#include "../../config/config.h"

static volatile uint32_t module_notify_cnt = 0;

#define ISR_PORT    CW_GPIOB
#define ISR_PIN     GPIO_PIN_0

static void module_notify_clr(void)
{
    __disable_irq();
    module_notify_cnt = 0;
    __enable_irq();
}

void module_init(void)
{
    module_notify_clr();
}

void module_deInit(void)
{
    module_notify_clr();
}

void GPIOB_IRQHandler(void)
{
    if(ISR_PORT->ISR & ISR_PIN) {
        GPIOB_INTFLAG_CLR(ISR_PIN);
        module_notify_cnt++;
    }

    if(CW_GPIOB->ISR & GPIO_PIN_7) { // 还用作唤醒
        GPIOB_INTFLAG_CLR(GPIO_PIN_7);
    }
}

void module_notify_post(void *param)
{
    __disable_irq();
    module_notify_cnt++;
    __enable_irq();
}

bool module_notify_poll(void)
{
    bool flag = false;

    if(module_notify_cnt > 0) {
        flag = true;
        __disable_irq();
        module_notify_cnt--;
        __enable_irq();
    }

    return flag;
}

void module_notify_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.IT = GPIO_IT_RISING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = ISR_PIN;
    GPIO_Init(ISR_PORT, &GPIO_InitStruct);

    GPIOB_INTFLAG_CLR(ISR_PIN);
    NVIC_EnableIRQ(GPIOB_IRQn);
    NVIC_SetPriority(GPIOB_IRQn, 0);
}

void module_notify_deInit(void)
{
    GPIOB_INTFLAG_CLR(ISR_PIN);
    NVIC_DisableIRQ(GPIOB_IRQn);
    gpio_deinit(GPIO_AT_ISR);
}

void MODULE_RST(void)
{
    gpio_modecfg(GPIO_POWER_OUT, GPIO_MODE_OUT);
    gpio_set(GPIO_POWER_OUT, GPIO_SET);

    gpio_modecfg(GPIO_RST, GPIO_MODE_OUT);
    gpio_set(GPIO_RST, GPIO_RESET);
    block_delayMs_4M(20);
    gpio_set(GPIO_RST, GPIO_SET);
}

void module_synch(void)
{
    SysParam_t *sysParam = getSysParam();

    at_send(AT_CMD_SET_SLEEP);
    at_send(AT_CMD_VOLUME);
    at_send(AT_CMD_GIDSET);
    at_send(AT_CMD_SIDSET);
    at_send(AT_CMD_BAT);
    at_send(AT_CMD_BAUDRATE);
}

void module_notify_resume(void)
{
    gpio_modecfg(GPIO_RST, GPIO_MODE_OUT);
    gpio_set(GPIO_RST, GPIO_SET);
}
