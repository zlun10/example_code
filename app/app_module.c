#include <stdint.h>
#include <string.h>
#include "app_module.h"
#include "../drv/inc/drv_uart.h"
#include "../dev/inc/log.h"
#include "../dev/inc/module.h"
#include "../dev/inc/at.h"
#include "../lib/misc.h"
#include "../config/config.h"

// 用于边缘检测的上一次状态
static modeflag_status_t last_modeflag_status = { 0 };

void app_module_init(void)
{
    buadrate_scan(1);
    at_init();
    module_init();
    module_synch();
}

void app_module_deInit(void)
{
    module_deInit();
    at_deInit();
}

void app_module_task(void)
{
    if(module_notify_poll()) {
        at_send(AT_CMD_MODEFLAG);
    }

    at_task();
}

static bool module_pop(Module_evt_t *evt)
{
    SysParam_t *sysParam = getSysParam();
    modeflag_status_t *curr = &sysParam->modeflag_status;
    modeflag_status_t *last = &last_modeflag_status;

    if(!last->call && curr->call && !curr->broadcast_m && !curr->broadcast_s) {
        last->call = curr->call;
        evt->evt = MODULE_EVT_CALL;
        LOG_DEBUG("module start calling\n");
        return true;
    }

    if(!last->ring && curr->ring) {
        last->ring = curr->ring;
        evt->evt = MODULE_EVT_RING;
        LOG_INFO("module start ring\n");
        return true;
    }

    if(!last->talk_master && curr->talk_master) {
        last->talk_master = curr->talk_master;
        evt->evt = MODULE_EVT_INCALL_ACT;
        LOG_INFO("module start incall\n");
        return true;
    }

    // talk_slave 从机通话开始
    if(!last->talk_slave && curr->talk_slave) {
        last->talk_slave = curr->talk_slave;
        evt->evt = MODULE_EVT_INCALL_ACT;
        LOG_INFO("module start incall\n");
        return true;
    }

    // 通话结束检测：从通话状态退出
    if((last->talk_master && !curr->talk_master) ||
        (last->talk_slave && !curr->talk_slave)) {
        last->talk_master = curr->talk_master;
        last->talk_slave = curr->talk_slave;
        evt->evt = MODULE_EVT_EXIT_INCALL;
        LOG_INFO("module end incall\n");
        return true;
    }

    // broadcast 广播开始（主机或从机）
    if((!last->broadcast_m && curr->broadcast_m) ||
        (!last->broadcast_s && curr->broadcast_s)) {
        last->broadcast_m = curr->broadcast_m;
        last->broadcast_s = curr->broadcast_s;
        evt->evt = MODULE_EVT_BROADCAST_START;
        LOG_DEBUG("module broadcast start\n");
        return true;
    }

    // broadcast 广播结束
    if((last->broadcast_m && !curr->broadcast_m) ||
        (last->broadcast_s && !curr->broadcast_s)) {
        last->broadcast_m = curr->broadcast_m;
        last->broadcast_s = curr->broadcast_s;
        evt->evt = MODULE_EVT_BROADCAST_END;
        LOG_DEBUG("module broadcast end\n");
        return true;
    }

    // 事件入队，时间上有说法
    if((last->call && !curr->call) || (!last->hangup && curr->hangup) || (last->ring && !curr->ring)) {
        last->call = curr->call;
        last->hangup = curr->hangup;
        last->ring = curr->ring;
        evt->evt = MODULE_EVT_HANGUP;
        LOG_INFO("module hangup\n");
        return true;
    }

    last->call = curr->call;
    last->ring = curr->ring;
    last->broadcast_m = curr->broadcast_m;
    last->broadcast_s = curr->broadcast_s;
    last->setting = curr->setting;
    last->invite = curr->invite;
    last->hangup = curr->hangup;

    return false;
}

/* 一方面通过中断被动判断事件，另一方面通过AT指令的返回值获取事件 */
bool app_module_evt_pop(Module_evt_t *evt)
{
    if(evt == NULL) {
        return false;
    }

    if(module_pop(evt)) {
        return true;
    }


    AT_evt_e at_evt;
    while(at_evt_pop(&at_evt))
    {
        switch(at_evt)
        {
        case AT_EVT_REFRESH_BAT:
            evt->evt = MODULE_EVT_REFRESH_BAT;
            return true;
        case AT_EVT_RECORDID_UPDATED:
            evt->evt = MODULE_EVT_RECORDID_UPDATED;
            return true;
        case AT_EVT_POWER_OFF_ACT:
            evt->evt = MODULE_EVT_POWER_OFF;
            return true;
        case AT_EVT_SLEEP_ACT:
            evt->evt = MODULE_EVT_SLEEP;
            return true;
        case AT_EVT_CALLED_ACT:
            evt->evt = MODULE_EVT_CALLED;
            return true;
        case AT_EVT_PICKUP_ACT:
            evt->evt = MODULE_EVT_INCALL_ACT;
            return true;

        default:
            return false;
        }
    }

    return false;
}

