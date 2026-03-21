#ifndef _TELEPHONE_H
#define _TELEPHONE_H

#include <stdint.h>
#include <stdbool.h>
#include "../../config/config.h"

typedef enum {
    APP_STATE_IDLE = 0,         // 默认待机
    APP_STATE_BROADCAST,        // 对讲广播
    APP_STATE_NUMDIAL,          // 输入号码    输入一半？
    APP_STATE_CONFIG,           // 修改id     修改一半？
    APP_STATE_SETTING,          // 配置阶段
    APP_STATE_CALLING_MASTER,   // 主动振铃
    APP_STATE_CALLED_SLAVE,     // 被动振铃   自动接听功能？
    APP_STATE_INCALL,           // 主机正在拨打电话   如果模组异常？
    APP_STATE_RECORD            // 查询通话记录
} app_state_m;

typedef enum {
    EVT_NONE = 0,

    // 对讲相关
    EVT_START_BROADCAST_PRE,
    EVT_EXIT_BROADCAST_PRE,
    EVT_START_BROADCAST_ACT,
    EVT_EXIT_BROADCAST_ACT,
    EVT_PTT_SWITCH,

    // 配置相关
    EVT_START_CONFIG,
    EVT_NEXT_CONFIG,
    EVT_EXIT_CONFIG,
    EVT_NUM0_C,
    EVT_NUM1_C,
    EVT_NUM2_C,
    EVT_NUM3_C,
    EVT_NUM4_C,
    EVT_NUM5_C,
    EVT_NUM6_C,
    EVT_NUM7_C,
    EVT_NUM8_C,
    EVT_NUM9_C,

    // 拨号相关
    EVT_START_DIAL,
    EVT_EXIT_DIAL,
    EVT_NUM0_D,
    EVT_NUM1_D,
    EVT_NUM2_D,
    EVT_NUM3_D,
    EVT_NUM4_D,
    EVT_NUM5_D,
    EVT_NUM6_D,
    EVT_NUM7_D,
    EVT_NUM8_D,
    EVT_NUM9_D,

    // handset
    EVT_PICKUP,
    EVT_PICKDOWN,

    // 通话相关
    EVT_START_INCALL_PRE,
    EVT_START_INCALL_ACT,
    EVT_EXIT_INCALL_PRE,
    EVT_EXIT_INCALL_ACT,

    EVT_START_DIRECT_CALL, // 直接拨号

    // 振铃相关 calling 主动振铃  called 被动振铃
    EVT_START_CALLING_PRE,
    EVT_EXIT_CALLING_PRE,
    EVT_START_CALLING_ACT,
    EVT_EXIT_CALLING_ACT,
    EVT_START_CALLED_PRE,
    EVT_EXIT_CALLED_PRE,
    EVT_START_CALLED_ACT,
    EVT_EXIT_CALLED_ACT,

    // 音量相关
    EVT_VOLUME_UP,
    EVT_VOLUME_DOWN,

    // 功能
    EVT_MUTE_SWITCH,
    EVT_AUTOCALL_SWITCH,
    EVT_BAT_REFRESH,

    // 通话记录相关
    EVT_START_RECORD,
    EVT_EXIT_RECORD,
    EVT_REFRESH_RECORD,
    EVT_LCD_UP,
    EVT_LCD_DOWN,

    // 系统相关
    EVT_POWER_OFF_PRE,
    EVT_POWER_OFF_ACT,
    EVT_SLEEP_PRE,
    EVT_SLEEP_ACT,
    EVT_SLEEP_REFRESH,

    // 配对
    EVT_ENTER_PAIR,
    EVT_EXIT_PAIR,
    EVT_ENTER_PSETTING_PRE,
    EVT_EXIT_SETTING_PRE,
    EVT_ENTER_SETTING_ACT,
    EVT_EXIT_SETTING_ACT,

    // 版本显示
    EVT_EXIT_SHOW_VER,

    EVT_NUM,
} App_evt_e;

typedef struct
{
    uint8_t did[DID_INDEX_MAX];
    uint8_t sid[SID_INDEX_MAX];
    uint8_t gid[GID_INDEX_MAX];
    uint8_t mid[MID_INDEX_MAX];
    uint8_t cid[CID_INDEX_MAX];
    uint8_t sgid[SGID_INDEX_MAX];
    uint8_t did_index;
    uint8_t sid_index;
    uint8_t gid_index;
    uint8_t mid_index;
    uint8_t cid_index;
    uint8_t sgid_index;
}Id_temp_msg;

// evt
void telep_init(void);
void telep_deInit(void);
bool is_app_idle(void);
app_state_m get_app_sta(void);
Id_temp_msg *get_temp_id(void);

// loop
void app_proc(void);

#endif
