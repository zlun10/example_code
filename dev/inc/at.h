#ifndef _AT_H
#define _AT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AT_CMD_NONE = 0,
    AT_CMD_CALL,    // 触发事件 打电话
    AT_CMD_PICKUP,  // 接听电话
    AT_CMD_CALL_ID, // 后接号码的 call 操作
    AT_CMD_HANGUP,
    AT_CMD_VOLUME,
    AT_CMD_SET_MUTE,
    AT_CMD_CANCEL_MUTE,
    AT_CMD_GIDSET,
    AT_CMD_SIDSET,
    AT_CMD_MODEFLAG,
    AT_CMD_BAT,
    AT_CMD_RECORDID,
    AT_CMD_RESTORE,
    AT_CMD_PLAYRING, // 播放铃声
    AT_CMD_BAUDRATE, // 设置波特率
    AT_CMD_SET_SLEEP,// 设置睡眠时间
    AT_CMD_BROADCAST,
    AT_CMD_SHUTDOWN, // 关机
    AT_CMD_SETTING,  // 进入PEIDUI
    AT_CMD_SLEEP,    // 休眠
    AT_CMD_VERSION,  // 获取版本号
    AT_CMD_INVITE,   // 进入邀请模式
    AT_CMD_MID,      // 设置模组记忆id
} AtCmd_e;

typedef enum {
    TONE_DIAL_BUTTON = 0,
    TONE_DIAL_LONG,
    TONE_HANG_OFF,
    TONE_LOW_POWER,
    TONE_PAIR_COMP,
    TONE_PAIR_ING,
    TONE_PAIR_RESET,
    TONE_POWER_OFF,
    TONE_POWER_ON,
    TONE_RING_RX1,
    TONE_RING_TX,
    TONE_WAITDIAL,
    TONE_WAIT_HANG_UP,
    TONE_CLOSE, // 关闭播报
    TONE_MAX,   // 占位符，表示无效的tone值
}ToneName_e;

typedef enum {
    AT_IDLE = 0,
    AT_SEND,
    AT_WAIT_RECV,
    AT_STATUS_ERROR,
} At_status_e;

typedef enum {
    AT_EVT_NONE = 0,
    AT_EVT_REFRESH_BAT,
    AT_EVT_RECORDID_UPDATED,
    AT_EVT_POWER_OFF_ACT,
    AT_EVT_SLEEP_ACT,
    AT_EVT_CALLED_ACT,
    AT_EVT_PICKUP_ACT,
}AT_evt_e;

// evt
void at_init(void);
void at_deInit(void);
void at_send(AtCmd_e cmd);
void at_set_tone(ToneName_e tone);
void send_cmd_pro(AtCmd_e cmd);
void parserVersion(const char *at_buff);
void at_clear(void);

// loop
void at_task(void);

// sync
bool at_evt_pop(AT_evt_e *evt);

#endif
