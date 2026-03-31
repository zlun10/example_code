#ifndef _MODULE_H
#define _MODULE_H

/**
 * @file module.h
 * @brief 模组业务逻辑层接口
 *
 * 本模块负责管理与无线模组（通过 AT 接口通信）相关的业务逻辑：
 *   - 模组硬件初始化（通过 HAL 层实现，与芯片解耦）
 *   - 模组中断通知的注册与查询
 *   - 模组与系统参数的同步
 *
 * 依赖关系：
 *   本层仅依赖 hal_module.h（HAL 接口）和标准库，
 *   不包含任何芯片专用头文件，保证可移植性。
 */

#pragma anon_unions

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 模组状态标志位（与模组 AT+MODEFLAG 响应对应）
 */
typedef struct {
    union {
        uint8_t bytes[4];
        struct {
            uint32_t call : 1;           /* bit 0 - 发起呼叫 */
            uint32_t ring : 1;           /* bit 1 - 被拨打响铃 */
            uint32_t talk_master : 1;    /* bit 2 - 作为主机正在通话中 */
            uint32_t talk_slave : 1;     /* bit 3 - 作为从机正在通话中 */
            uint32_t broadcast_m : 1;    /* bit 4 - 主机广播 */
            uint32_t broadcast_s : 1;    /* bit 5 - 从机广播 */
            uint32_t setting : 1;        /* bit 6 - 进入群组设置 */
            uint32_t invite : 1;         /* bit 7 - 进入群组邀请模式 */
            uint32_t hangup : 1;         /* bit 8 - 执行挂断中 */
            uint32_t powerOff : 1;       /* bit 9 - 模组请求关机 */
            uint32_t reserved : 22;      /* bit 10-31 */
        };
    };
} modeflag_status_t;

/* ============================================================
 * 接口声明
 * ============================================================ */

/** @brief 初始化模组业务层（含 HAL 通知引脚初始化） */
void module_init(void);

/** @brief 反初始化模组业务层（含 HAL 通知引脚反初始化） */
void module_deInit(void);

/** @brief 查询是否有模组通知待处理（消费一个通知计数） */
bool module_notify_poll(void);

/** @brief 主动投递一个模组通知（可用于软件触发，例如定时轮询） */
void module_notify_post(void *param);

/** @brief 将系统参数同步到模组（发送一组 AT 命令） */
void module_synch(void);

/** @brief 执行模组硬件复位 */
void MODULE_RST(void);

/** @brief 初始化模组中断通知引脚（内部使用） */
void module_notify_init(void);

/** @brief 反初始化模组中断通知引脚（内部使用） */
void module_notify_deInit(void);

/** @brief 唤醒后恢复模组 RST 引脚状态 */
void module_notify_resume(void);

#endif /* _MODULE_H */
