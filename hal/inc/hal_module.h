#ifndef _HAL_MODULE_H
#define _HAL_MODULE_H

/**
 * @file hal_module.h
 * @brief 模组硬件抽象层（HAL）接口定义
 *
 * 本文件定义了模组驱动所需的硬件操作接口。
 * 移植到新芯片/平台时，只需在对应的 hal/src/hal_module.c 中实现这些函数，
 * 上层 dev/src/module.c 及应用层代码无需任何修改。
 *
 * 接口说明：
 *   - hal_module_rst()             : 执行模组硬件复位
 *   - hal_module_notify_init()     : 初始化模组中断通知引脚
 *   - hal_module_notify_deInit()   : 反初始化模组中断通知引脚
 *   - hal_module_notify_resume()   : 唤醒后恢复模组 RST 引脚
 *   - hal_module_notify_callback() : 中断回调（由 dev 层实现，HAL ISR 调用）
 *
 * 移植说明：
 *   1. 将 hal/src/hal_module.c 中的所有函数替换为目标平台的实现。
 *   2. 在目标平台的中断服务程序（ISR）中调用 hal_module_notify_callback()。
 */

#include <stdint.h>

/* ============================================================
 * HAL 接口声明
 * ============================================================ */

/**
 * @brief 执行模组硬件复位
 *
 * 操作顺序：
 *   1. 使能模组电源
 *   2. 将 RST 引脚拉低（复位有效）
 *   3. 延时约 20ms
 *   4. 将 RST 引脚拉高（释放复位）
 */
void hal_module_rst(void);

/**
 * @brief 初始化模组中断通知引脚
 *
 * 将指定引脚配置为上升沿触发中断，并使能对应 NVIC 通道。
 * 该引脚用于接收模组发出的状态变化通知。
 */
void hal_module_notify_init(void);

/**
 * @brief 反初始化模组中断通知引脚
 *
 * 禁用中断并释放引脚资源。
 */
void hal_module_notify_deInit(void);

/**
 * @brief 唤醒后恢复模组 RST 引脚状态
 *
 * 系统从低功耗模式唤醒后，需将 RST 引脚配置为输出并置高，
 * 以确保模组正常工作。
 */
void hal_module_notify_resume(void);

/**
 * @brief 模组中断通知回调函数（由 dev 层实现，HAL ISR 中调用）
 *
 * 当模组发出中断通知时，HAL 的中断服务程序（ISR）将调用此函数。
 * dev 层（module.c）负责实现此函数，用于更新内部通知计数器。
 *
 * @note 此函数在中断上下文中执行，实现时应保持简短。
 */
void hal_module_notify_callback(void);

#endif /* _HAL_MODULE_H */
