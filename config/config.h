#ifndef _CONFIG_H
#define _CONFIG_H

#pragma anon_unions

#include <stdint.h>
#include "../dev/inc/module.h"

// 默认参数定义
#define VOLUME_INIT    6 // 1 -- 8
#define AUTO_CALL_INIT 0 // 0: 关闭 1: 开启
#define BATTERY_INIT   60

/* baudrate */
#define MODULE_BAUDRATE_INIT  9600
#define MCU_BAUDRATE_INIT  115200

/* id */
#define DID_INDEX_MAX 4 // 拨号的号码长度；通话记录的号码长度
#define SID_INDEX_MAX 4
#define GID_INDEX_MAX 2 // 这个是两个字节，因为按键的习惯，在AT发送那里做了处理
#define MID_INDEX_MAX 4
#define CID_INDEX_MAX 4
#define SGID_INDEX_MAX 2
extern const uint8_t MID_INIT_ARRAY[MID_INDEX_MAX];
extern const uint8_t SID_INIT_ARRAY[SID_INDEX_MAX];
extern const uint8_t GID_INIT_ARRAY[GID_INDEX_MAX];
#define RECORD_ID_COUNT 5

/* sysParam */
typedef struct
{
    uint8_t mid[MID_INDEX_MAX]; // 保存的四位电话id // 这里是十进制
    uint8_t sid[SID_INDEX_MAX]; // 自己的四位ID
    uint8_t gid[GID_INDEX_MAX]; // 组ID
    union {
        uint8_t date;
        struct {
            uint8_t  sgid_flag : 1;
            uint8_t  sgid_id : 7;// 二级群组ID 0: 1开判断  1-7：ID
        } __attribute__((packed));
    } __attribute__((packed));
    uint8_t volume;             // 音量等级 1~8
    struct {
        uint8_t autoCall : 1;   // 自动接听开关
        uint8_t reserved : 2;
        uint8_t recordIdFlag : RECORD_ID_COUNT;   // 记录通话记录接听：1：未接听，0：已接听
    }__attribute__((packed)) data;
    uint32_t magicID;           // 用于校验
    uint8_t recordId[RECORD_ID_COUNT][DID_INDEX_MAX];     // 通话记录
} __attribute__((packed)) NvmParam_t;

typedef struct
{
    NvmParam_t nvmParams;
    uint8_t is_writed;                      // 是否是已经写入flash 1: 是 0: 否
    uint8_t mute;                           // 静音状态 1: 静音 0: 非静音
    uint8_t bat;                            // 电量百分比 0 ~ 100
    uint8_t broadcast;                      // 是否在广播 1: 是 0: 否
    uint32_t baudrate;                      // 模组的波特率
    modeflag_status_t modeflag_status;      // 模组那边的状态
    char *module_version;                   // 模组版本号字符串指针
    uint8_t  call_id_temp[4];
    uint8_t  record_id_temp[4];
} SysParam_t;

SysParam_t *getSysParam(void);

#endif
