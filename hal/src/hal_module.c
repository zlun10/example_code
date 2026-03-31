/**
 * @file hal_module.c
 * @brief 模组硬件抽象层（HAL）— CW32L011 平台实现
 *
 * 本文件实现了 hal_module.h 中定义的所有硬件操作接口，
 * 依赖 CW32L011 的片上外设（GPIO、NVIC）和板级驱动（drv_gpio）。
 *
 * 移植说明：
 *   将本文件中的所有函数替换为目标芯片/平台的对应实现即可。
 *   上层 dev/src/module.c 及应用层代码不需要做任何修改。
 */

#include "../inc/hal_module.h"
#include "../../drv/inc/drv_gpio.h"
#include "../../lib/misc.h"
#include "cw32l011_gpio.h"

/* 模组中断通知引脚（CW32L011 平台：GPIOB.0） */
#define MODULE_ISR_PORT    CW_GPIOB
#define MODULE_ISR_PIN     GPIO_PIN_0

/* ============================================================
 * 接口实现
 * ============================================================ */

void hal_module_rst(void)
{
    /* 1. 使能模组电源 */
    gpio_modecfg(GPIO_POWER_OUT, GPIO_MODE_OUT);
    gpio_set(GPIO_POWER_OUT, GPIO_SET);

    /* 2. 拉低 RST（复位有效），延时后释放 */
    gpio_modecfg(GPIO_RST, GPIO_MODE_OUT);
    gpio_set(GPIO_RST, GPIO_RESET);
    block_delayMs_4M(20);
    gpio_set(GPIO_RST, GPIO_SET);
}

void hal_module_notify_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.IT   = GPIO_IT_RISING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = MODULE_ISR_PIN;
    GPIO_Init(MODULE_ISR_PORT, &GPIO_InitStruct);

    GPIOB_INTFLAG_CLR(MODULE_ISR_PIN);
    NVIC_EnableIRQ(GPIOB_IRQn);
    NVIC_SetPriority(GPIOB_IRQn, 0);
}

void hal_module_notify_deInit(void)
{
    GPIOB_INTFLAG_CLR(MODULE_ISR_PIN);
    NVIC_DisableIRQ(GPIOB_IRQn);
    gpio_deinit(GPIO_AT_ISR);
}

void hal_module_notify_resume(void)
{
    gpio_modecfg(GPIO_RST, GPIO_MODE_OUT);
    gpio_set(GPIO_RST, GPIO_SET);
}

/* ============================================================
 * 中断服务程序（ISR）
 * 说明：ISR 名称与芯片绑定，不可移植；移植时替换为目标平台的 ISR 名称即可。
 * ============================================================ */

void GPIOB_IRQHandler(void)
{
    /* 模组通知中断（GPIOB.0 上升沿） */
    if(MODULE_ISR_PORT->ISR & MODULE_ISR_PIN) {
        GPIOB_INTFLAG_CLR(MODULE_ISR_PIN);
        hal_module_notify_callback();   /* 通知 dev 层 */
    }

    /* PTT 按键唤醒引脚（GPIOB.7），仅清标志，无需额外处理 */
    if(CW_GPIOB->ISR & GPIO_PIN_7) {
        GPIOB_INTFLAG_CLR(GPIO_PIN_7);
    }
}
