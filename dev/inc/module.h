#ifndef _MODULE_H
#define _MODULE_H

#pragma anon_unions

#include <stdint.h>
#include <stdbool.h>
#include "cw32l011_gpio.h"

typedef struct {
    union {
        uint8_t bytes[4];
        struct {
            uint32_t call : 1;           // bit 0 - 发起呼叫
            uint32_t ring : 1;           // bit 1 - 被拨打响铃
            uint32_t talk_master : 1;    // bit 2 - 作为主机正在通话中
            uint32_t talk_slave : 1;     // bit 3 - 作为从机正在通话中
            uint32_t broadcast_m : 1;    // bit 4 - 主机广播
            uint32_t broadcast_s : 1;    // bit 5 - 从机广播
            uint32_t setting : 1;        // bit 6 - 进入群组设置
            uint32_t invite : 1;         // bit 7 - 进入群组邀请模式
            uint32_t hangup : 1;         // bit 8 - 执行挂断中
            uint32_t powerOff : 1;       // bit 9 - 模组请求关机
            uint32_t reserved : 22;      // bit 10-31
        };
    };
} modeflag_status_t;

// power实体按键
#define POWER_KEY_PORT    CW_GPIOC
#define POWER_KEY_PIN     GPIO_PIN_13

// 模组的电源控制
#define POWER_PORT    CW_GPIOA
#define POWER_PIN     GPIO_PIN_8

// evt
void module_init(void);
void module_deInit(void);

// mode flag notify
bool module_notify_poll(void);
void module_notify_post(void *param);
void module_synch(void);

void MODULE_RST(void);
void module_notify_init(void);
void module_notify_deInit(void);
void module_notify_resume(void);

#endif
