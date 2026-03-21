#include "timer_affair.h"
#include "../dev/inc/sys_tick.h"

static TmrEvt_s tmrEvt[MAX_TIMER_EVT_NUM];	           // 定时器/计时器队列
static uint8_t realMaxTmrEvtNum = 0;

static void UpdateMaxTmrNum(void)
{
    uint8_t i;
    uint8_t num = MAX_FIXED_TIMER_EVT_NUM;

    for(i = MAX_FIXED_TIMER_EVT_NUM; i < MAX_TIMER_EVT_NUM; i++) {
        if(tmrEvt[i].timerEvtFlg.isWorking == 1) {
            num = i + 1;
        }
    }
    realMaxTmrEvtNum = num;
}

void timerEventInit(void)
{
    uint8_t i;

    for(i = 0; i < MAX_TIMER_EVT_NUM; i++) {
        tmrEvt[i].timerEvtFlg.clkSrc = ClkSrc10ms;
        tmrEvt[i].timerEvtFlg.isForever = 0;
        tmrEvt[i].timerEvtFlg.isWorking = 0;
        tmrEvt[i].tmrEvtTrigCnt = 0;
        tmrEvt[i].pEvtCb = (void *) 0;
    }
    realMaxTmrEvtNum = MAX_FIXED_TIMER_EVT_NUM;
}

/**
 * @brief 启动特定的定时器
 * @param  evtId  定时器的编号ID
 * @param  clkSrc  时钟源, 可选10ms,100ms,1s,1h
 * @param  isForever  指定是单次定时事件，还是周期性的
 * @param  tmrEvtTrigCnt
 * @param  pEvtCb
 * @param  pPara
 */
void StartTimerEvent(EventId_e evtId, ClkSrc_e clkSrc, uint8_t isForever,
    uint16_t tmrEvtTrigCnt, void (*pEvtCb)(void *), void *pPara)
{
    if(evtId < MAX_FIXED_TIMER_EVT_NUM) {
        tmrEvt[evtId].timerEvtFlg.clkSrc = clkSrc;
        tmrEvt[evtId].timerEvtFlg.isForever = isForever;
        tmrEvt[evtId].timerEvtFlg.isWorking = 1;
        tmrEvt[evtId].tmrEvtTrigCnt = tmrEvtTrigCnt;
        tmrEvt[evtId].tmrEvtTrigCntDn = tmrEvtTrigCnt;
        tmrEvt[evtId].pEvtCb = pEvtCb;
        tmrEvt[evtId].pPara = pPara;
        UpdateMaxTmrNum();
    }
}

/**
 * @brief  分配一timer, 0xFF: 表示无效
 * @param  clkSrc
 * @param  isForever
 * @param  tmrEvtTrigCnt
 * @param  pEvtCb
 * @param  pPara
 * @return uint8_t
 */
uint8_t CreateTmrEvt(ClkSrc_e clkSrc, uint8_t isForever,
    uint32_t tmrEvtTrigCnt, void (*pEvtCb)(void *), void *pPara)
{
    uint8_t i;
    TmrEvt_s *pTimer;

    for(i = MAX_FIXED_TIMER_EVT_NUM; i < MAX_TIMER_EVT_NUM; i++) {
        pTimer = &tmrEvt[i];

        if(pTimer->timerEvtFlg.isWorking == 0) {
            pTimer->timerEvtFlg.isWorking = 1;
            pTimer->timerEvtFlg.clkSrc = clkSrc;
            pTimer->timerEvtFlg.isForever = isForever;
            pTimer->tmrEvtTrigCnt = tmrEvtTrigCnt;
            pTimer->tmrEvtTrigCntDn = tmrEvtTrigCnt;
            pTimer->pEvtCb = pEvtCb;
            pTimer->pPara = pPara;

            if(i >= realMaxTmrEvtNum)
                realMaxTmrEvtNum = i + 1;
            return i;
        }
    }
    return 0xFF;
}

/**
 * @brief
 * @param  u8TimerID
 */
void StopTmrEvt(uint8_t u8TimerID)
{
    TmrEvt_s *pTimer;

    if(u8TimerID < MAX_TIMER_EVT_NUM) {
        pTimer = &tmrEvt[u8TimerID];

        if(pTimer->timerEvtFlg.isForever == 1) {// 是一直执行的Timer
            pTimer->tmrEvtTrigCntDn = pTimer->tmrEvtTrigCnt;
            pTimer->timerEvtFlg.isWorking = 1;
        } else {
            pTimer->timerEvtFlg.isWorking = 0;
            UpdateMaxTmrNum();
        }
    }
}

void DestroyTmrEvt(uint8_t u8TimerID)
{
    TmrEvt_s *pTimer;

    if(u8TimerID < MAX_TIMER_EVT_NUM) {
        pTimer = &tmrEvt[u8TimerID];
        pTimer->timerEvtFlg.isWorking = 0;
        pTimer->timerEvtFlg.isForever = 0;
        UpdateMaxTmrNum();
    }
}

static void Process10msEvent(void)
{
    uint8_t i;
    TmrEvt_s *pTimer;

    for(i = 0; i < realMaxTmrEvtNum; i++) {
        pTimer = &tmrEvt[i];
        if(pTimer->timerEvtFlg.isWorking && (ClkSrc10ms == pTimer->timerEvtFlg.clkSrc)) {
            // 该定时器在工作中且基于10mS且没超时
            if(pTimer->tmrEvtTrigCntDn > 0) {
                pTimer->tmrEvtTrigCntDn--;
            }

            if(pTimer->tmrEvtTrigCntDn == 0) {
                void *pPara = pTimer->pPara;
                StopTmrEvt(i);

                if(pTimer->pEvtCb != 0) {
                    pTimer->pEvtCb(pPara);
                }
            }
        }
    }
}

static void Process100msEvent(void)
{
    uint8_t i;
    TmrEvt_s *pTimer;

    for(i = 0; i < realMaxTmrEvtNum; i++) {
        pTimer = &tmrEvt[i];
        if(pTimer->timerEvtFlg.isWorking && (ClkSrc100ms == pTimer->timerEvtFlg.clkSrc)) {
            //该定时器在工作中且基于100mS且没超时
            if(pTimer->tmrEvtTrigCntDn > 0)
                pTimer->tmrEvtTrigCntDn--;

            if(pTimer->tmrEvtTrigCntDn == 0) {
                if(pTimer->pEvtCb != 0) {
                    pTimer->pEvtCb(pTimer->pPara);
                }
                StopTmrEvt(i);
            }
        }
    }
}

static void Process1sEvent(void)
{
    uint8_t i;
    TmrEvt_s *pTimer;

    for(i = 0; i < realMaxTmrEvtNum; i++) {
        pTimer = &tmrEvt[i];

        if(pTimer->timerEvtFlg.isWorking && (ClkSrc1second == pTimer->timerEvtFlg.clkSrc)) {
            //该定时器在工作中且基于1S且没超时
            if(pTimer->tmrEvtTrigCntDn > 0) {
                pTimer->tmrEvtTrigCntDn--;
            }

            if(pTimer->tmrEvtTrigCntDn == 0) {
                if(pTimer->pEvtCb != 0) {
                    pTimer->pEvtCb(pTimer->pPara);
                }
                StopTmrEvt(i);
            }
        }
    }
}

static void Process1hEvent(void)
{
    uint8_t i;
    TmrEvt_s *pTimer;

    for(i = 0; i < realMaxTmrEvtNum; i++) {
        pTimer = &tmrEvt[i];

        if(pTimer->timerEvtFlg.isWorking && (ClkSrc1hour == pTimer->timerEvtFlg.clkSrc)) {
            //该定时器在工作中且基于1小时且没超时
            if(pTimer->tmrEvtTrigCntDn > 0) {
                pTimer->tmrEvtTrigCntDn--;
            }

            if(pTimer->tmrEvtTrigCntDn == 0) {
                if(pTimer->pEvtCb != 0) {
                    pTimer->pEvtCb(pTimer->pPara);
                }
                StopTmrEvt(i);
            }
        }
    }
}

void ProcessTimerEvent(void)
{
    static uint32_t sysTickCntFor10ms = 0;
    static uint32_t sysTickCntFor100ms = 0;
    static uint32_t sysTickCntFor1s = 0;
    static uint32_t sysTickCntFor1h = 0;

    if(millis() >= (sysTickCntFor10ms + 10)) {
        sysTickCntFor10ms = millis();
        Process10msEvent();
    }

    if(millis() >= (sysTickCntFor100ms + 100)) {
        sysTickCntFor100ms = millis();
        Process100msEvent();
    }

    if(millis() >= (sysTickCntFor1s + 1000)) {
        sysTickCntFor1s = millis();
        Process1sEvent();
    }

    if(millis() >= (sysTickCntFor1h + 3600000)) {
        sysTickCntFor1h = millis();
        Process1hEvent();
    }
}

