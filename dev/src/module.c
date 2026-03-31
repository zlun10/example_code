/**
 * @file module.c
 * @brief 模组业务逻辑层实现
 *
 * 本模块负责：
 *   1. 通过 HAL 接口（hal_module.h）完成硬件初始化，不直接操作寄存器
 *   2. 维护模组中断通知计数器（中断安全）
 *   3. 实现 hal_module_notify_callback()，供 HAL ISR 回调
 *   4. 提供 module_synch() 将系统参数下发到模组
 *
 * 移植说明：
 *   本文件无需修改。移植时只需实现 hal/src/hal_module.c 中的硬件接口。
 */
#include <stdint.h>
#include "../inc/module.h"
#include "../../hal/inc/hal_module.h"
#include "../../dev/inc/at.h"

/* 模组中断通知计数器（volatile，供中断安全访问） */
static volatile uint32_t module_notify_cnt = 0;

/* ============================================================
 * 内部辅助函数
 * ============================================================ */

static void module_notify_clr(void)
{
    __disable_irq();
    module_notify_cnt = 0;
    __enable_irq();
}

/* ============================================================
 * HAL 回调实现（由 hal_module.c 中的 ISR 调用）
 * ============================================================ */

/**
 * @brief 模组中断通知回调
 *
 * 在 ISR 上下文中被 hal_module.c 调用，递增通知计数。
 * hal_module.h 中声明此函数为需由 dev 层实现的回调。
 */
void hal_module_notify_callback(void)
{
    module_notify_cnt++;
}

/* ============================================================
 * 对外接口实现
 * ============================================================ */

void module_init(void)
{
    module_notify_clr();
    hal_module_notify_init();
}

void module_deInit(void)
{
    hal_module_notify_deInit();
    module_notify_clr();
}

void module_notify_post(void *param)
{
    /* param 为定时器事件回调的参数（StartTimerEvent 传入 NULL），暂未使用 */
    (void)param;
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
    hal_module_notify_init();
}

void module_notify_deInit(void)
{
    hal_module_notify_deInit();
}

void MODULE_RST(void)
{
    hal_module_rst();
}

void module_synch(void)
{
    at_send(AT_CMD_SET_SLEEP);
    at_send(AT_CMD_VOLUME);
    at_send(AT_CMD_GIDSET);
    at_send(AT_CMD_SIDSET);
    at_send(AT_CMD_BAT);
    at_send(AT_CMD_BAUDRATE);
}

void module_notify_resume(void)
{
    hal_module_notify_resume();
}
