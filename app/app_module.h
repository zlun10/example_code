#ifndef _APP_MODULE_H
#define _APP_MODULE_H

#pragma anon_unions

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODULE_EVT_NONE = 0,
    MODULE_EVT_CALL,              // 发起呼叫
    MODULE_EVT_RING,              // 被拨打响铃
    MODULE_EVT_INCALL_PRE,        // 进入通话
    MODULE_EVT_INCALL_ACT,        // 进入通话
    MODULE_EVT_EXIT_INCALL,       // 退出通话
    MODULE_EVT_BROADCAST_START,   // 广播开始（主机或从机）
    MODULE_EVT_BROADCAST_END,     // 广播结束
    MODULE_EVT_SETTING,           // 进入群组设置
    MODULE_EVT_INVITE,            // 进入群组邀请
    MODULE_EVT_HANGUP,            // 正在挂断
    MODULE_EVT_POWER_OFF,         // 关机请求
    MODULE_EVT_REFRESH_BAT,       // 电量刷新
    MODULE_EVT_RECORDID_UPDATED,  // 通话记录更新
    MODULE_EVT_SLEEP,             // 休眠
    MODULE_EVT_CALLED,            // 确认被叫
}module_evt_e;

typedef struct {
    union {
        uint32_t date;
        struct {
            uint8_t evt;
            uint8_t reserved[3];
        };
    };
}Module_evt_t;

// evt
void app_module_init(void);
void app_module_deInit(void);

// loop
void app_module_task(void);

// sync
bool app_module_evt_pop(Module_evt_t *evt);

#endif

