#ifndef _TIMER_AFFAIR_H_
#define _TIMER_AFFAIR_H_

#include <stdbool.h>
#include <stdint.h>

#define MAX_TIMER_EVT_NUM                 (15)  // 最大定时器的个数

typedef enum {
    TIMER_ID_KEY_SCAN = 0,
    TIMER_ID_HANDSET_SCAN,
    TIMER_ID_LED_SCAN,
    TIMER_ID_SYN_SCAN,          // 同步模组
    TIMER_ID_SCREEN_BLINK,
    TIMER_ID_AUTOSLEEP,
    TIMER_ID_UNDERLINE_BLINK,
    MAX_FIXED_TIMER_EVT_NUM,    // 最大固定定时
} EventId_e;

typedef enum {
    ClkSrc10ms,
    ClkSrc100ms,
    ClkSrc1second,
    ClkSrc1hour,
} ClkSrc_e;

typedef struct {
    uint8_t isWorking;
    ClkSrc_e clkSrc;
    uint8_t isForever;
}TimerEvtFlag_s;

typedef struct {
    TimerEvtFlag_s timerEvtFlg;
    uint16_t    tmrEvtTrigCnt;      //延迟时间
    uint16_t    tmrEvtTrigCntDn;    //生存周期
    void        (*pEvtCb)(void *);  //定时器时间到回调函数
    void *pPara;                    //回调函数参数
} TmrEvt_s;

// evt
void timerEventInit(void);

uint8_t CreateTmrEvt(ClkSrc_e clkSrc, uint8_t isForever,
    uint32_t tmrEvtTrigCnt, void (*pEvtCb)(void *), void *pPara);
void StartTimerEvent(EventId_e evtId, ClkSrc_e clkSrc, uint8_t isForever, uint16_t tmrEvtTrigCnt,
    void (*pEvtCb)(void *), void *pPara);
void StopTmrEvt(uint8_t u8TimerID);
void DestroyTmrEvt(uint8_t u8TimerID);

// loop
void ProcessTimerEvent(void);

#endif
