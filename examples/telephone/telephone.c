#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "telephone.h"
#include "../../hal/inc/hal_key.h"
#include "../../dev/inc/log.h"
#include "../../dev/inc/sys_tick.h"
#include "../../lib/misc.h"
#include "../../lib/key.h"
#include "../../lib/storage.h"
#include "../../dev/inc/module.h"
#include "../../dev/inc/at.h"
#include "../../app/app_module.h"
#include "../../app/app_sleep.h"
#include "../../app/app_lowpower.h"
#include "../../lib/rb.h"
#include "../../config/config.h"

#define VOLUME_MAX  8
#define VOLUME_MIN  1

#define AUTO_CALL_TIMEOUT 6000// ms

typedef void (*App_evt_handler)(void);

typedef enum {
    CONFIG_SID = 0,
    CONFIG_GID,
    CONFIG_MID,
    CONFIG_SGID,
    CONFIG_ID_MAX,
}config_id_m;

static const char *app_state_name_tbl[] = {
    "APP_STATE_IDLE",
    "APP_STATE_BROADCAST",
    "APP_STATE_NUMDIAL",
    "APP_STATE_CONFIG",
    "APP_STATE_CALLING_MASTER",
    "APP_STATE_CALLED_SLAVE",
    "APP_STATE_INCALL",
    "APP_STATE_RECORD",
};

static const char *app_state_name(app_state_m s)
{
    size_t idx = (size_t) s;
    size_t max = sizeof(app_state_name_tbl) / sizeof(app_state_name_tbl[0]);
    if(idx < max) return app_state_name_tbl[idx];
    return "UNKNOWN_STATE";
}

typedef struct {
    app_state_m state;
    app_state_m statePre;
    config_id_m configId_state;// 配置号码的子状态
    uint32_t auto_pick_tick;
    rb_t evt_rb;
    App_evt_handler handle[EVT_NUM];
    bool loopToneFlag; // 1:开
}App_cxt_t;

static App_cxt_t g_app_cxt;

static App_cxt_t *get_app_cxt(void)
{
    return &g_app_cxt;
}

bool is_app_idle(void)
{
    App_cxt_t *cxt = get_app_cxt();
    return (cxt->state == APP_STATE_IDLE);
}

app_state_m get_app_sta(void)
{
    App_cxt_t *cxt = get_app_cxt();
    return (cxt->state);
}

Id_temp_msg g_id_temp_msg;// id表

Id_temp_msg *get_temp_id(void)
{
    return &g_id_temp_msg;
}

static void set_app_sta(app_state_m state)
{
    SysParam_t *sysParam = getSysParam();
    App_cxt_t *cxt = get_app_cxt();
    if(state == APP_STATE_IDLE)
    {


        memset((void *) &sysParam->modeflag_status, 0, sizeof(sysParam->modeflag_status));
    }
    cxt->state = state;
}

/*--------------------------------------------------------------------------------------------------------*/

static void start_boardcast_pre(void)
{
    SysParam_t *sysParam = getSysParam();
    sysParam->broadcast = 1;
    at_send(AT_CMD_BROADCAST);
    LOG_DEBUG("start bcast pre\n");
}

static void stop_boardcast_pre(void)
{
    SysParam_t *sysParam = getSysParam();
    sysParam->broadcast = 0;
    at_send(AT_CMD_BROADCAST);
    LOG_DEBUG("stop bcast pre\n");
}

static void start_boardcast_act(void)
{
    set_app_sta(APP_STATE_BROADCAST);
    LOG("start bcast\n");
}

static void stop_boardcast_act(void)
{
    App_cxt_t *cxt = get_app_cxt();
    cxt->state = APP_STATE_IDLE;
    LOG("stop bcast\n");
}

static void switch_ptt(void)
{
    App_cxt_t *cxt = get_app_cxt();
    SysParam_t *sysParam = getSysParam();
    sysParam->nvmParams.sgid_flag = !sysParam->nvmParams.sgid_flag;
    refreshNVM_EN();
    at_send(AT_CMD_SIDSET);
    LOG("switch ptt\n");
}

static void start_incall_pre(void)
{
    at_send(AT_CMD_CANCEL_MUTE);
    at_send(AT_CMD_PICKUP);

    LOG("start incall pre\n");
}

static void start_incall_act(void)
{
    SysParam_t *sysParam = getSysParam();
    sysParam->nvmParams.data.recordIdFlag &= 0x0f;
    refreshNVM_EN();

    set_app_sta(APP_STATE_INCALL);
    LOG("start incall act\n");
}

static void exit_incall_pre(void)
{
    SysParam_t *sysParam = getSysParam();
    at_send(AT_CMD_SET_MUTE);
    at_send(AT_CMD_HANGUP);
    LOG("end incall pre\n");
}

static void exit_incall_act(void)
{
    set_app_sta(APP_STATE_IDLE);
    LOG("end incall act\n");
}

static void start_direct_call(void)
{
    // stop_loop_tone();
    at_send(AT_CMD_CALL);
    LOG("start direct call\n");
}

static void dialNumConvert(uint8_t *num, uint8_t len)
{
    uint8_t count = 4 - len;
    uint8_t temp[4] = { 0 };
    for(int8_t i = len - 1; i >= 0; i--)
    {
        temp[i + count] = num[i];
    }

    memcpy(num, temp, 4);
}

static void start_calling_pre(void)
{
    dialNumConvert(g_id_temp_msg.did, g_id_temp_msg.did_index);

    SysParam_t *sysParam = getSysParam();
    App_cxt_t *cxt = get_app_cxt();

    switch(cxt->state)
    {
    case APP_STATE_IDLE:
        memcpy(sysParam->call_id_temp, sysParam->nvmParams.mid, MID_INDEX_MAX);
        break;
    case APP_STATE_NUMDIAL:
        memcpy(sysParam->call_id_temp, g_id_temp_msg.did, DID_INDEX_MAX);
        break;
    case APP_STATE_RECORD:
        break;

    default:
        break;
    }



    at_send(AT_CMD_CALL_ID);
    LOG_INFO("start calling pre:id %d\n", HEX2DEC(sysParam->call_id_temp, 4));
    LOG("start calling pre\n");
}

static void exit_calling_pre(void)
{
    at_send(AT_CMD_HANGUP);
    LOG("exit calling pre\n");
}

static void start_called_pre(void)
{

    App_cxt_t *cxt = get_app_cxt();

    at_send(AT_CMD_RECORDID);
    set_app_sta(APP_STATE_CALLED_SLAVE);
    cxt->auto_pick_tick = millis();

    LOG("start called pre\n");
}

static void exit_called_pre(void)
{
    at_send(AT_CMD_HANGUP);
    LOG("exit called pre\n");
}

static void start_calling_act(void)
{
    set_app_sta(APP_STATE_CALLING_MASTER);
    LOG("start calling act\n");
}

static void exit_calling_act(void)
{
    set_app_sta(APP_STATE_IDLE);
    LOG("exit calling act\n");
}

static void start_called_act(void)
{
    if(get_app_sta() != APP_STATE_CALLED_SLAVE)
    {
        LOG_WARN("start called act but state is not CALLED_SLAVE, state: %s",
            app_state_name(get_app_sta()));
        return;
    }
    SysParam_t *sysParam = getSysParam();
    App_cxt_t *cxt = get_app_cxt();

    cxt->auto_pick_tick = millis();

    LOG("start called act\n");
}

static void exit_called_act(void)
{
    set_app_sta(APP_STATE_IDLE);
    LOG("end called act\n");
}

static void power_off_pre(void)
{
    at_send(AT_CMD_SHUTDOWN);
    LOG("power off pre\n");
}

static void power_off_act(void)
{
    enter_deepSleep();
    LOG("power off act\n");
}

static void sleep_pre(void)
{
    at_send(AT_CMD_SLEEP);

    LOG("sleep pre\n");
}

static void sleep_act(void)
{
    enter_sleep();
    LOG("enter sleep\n");
}

static void sleep_refresh(void)
{
    app_sleep_refresh();
    LOG_INFO("sleep refresh\n");
}

static void vol_up(void)
{
    SysParam_t *sysParam = getSysParam();
    if(sysParam->nvmParams.volume < VOLUME_MAX) {
        sysParam->nvmParams.volume++;
        refreshNVM_EN();
    }

    at_send(AT_CMD_VOLUME);
    at_set_tone(TONE_DIAL_BUTTON);
    at_send(AT_CMD_PLAYRING);

    LOG("vol up\n");
}

static void vol_down(void)
{


    SysParam_t *sysParam = getSysParam();
    if(sysParam->nvmParams.volume > VOLUME_MIN) {
        sysParam->nvmParams.volume--;
        refreshNVM_EN();
    }

    at_send(AT_CMD_VOLUME);
    at_set_tone(TONE_DIAL_BUTTON);
    at_send(AT_CMD_PLAYRING);
    LOG("vol down\n");
}

static void switch_mute(void)
{
    SysParam_t *sysParam = getSysParam();
    sysParam->mute = !sysParam->mute;
    at_send(AT_CMD_SET_MUTE);

    LOG("switch mute\n");
}

static void switch_autoCall(void)
{
    SysParam_t *sysParam = getSysParam();
    sysParam->nvmParams.data.autoCall = !sysParam->nvmParams.data.autoCall;
    LOG_DEBUG("autoCall: %d\n", sysParam->nvmParams.data.autoCall);
    refreshNVM_EN();

    LOG("switch autoCall\n");
}


static void enter_pair(void)
{
    at_send(AT_CMD_SETTING);
}

static void exit_pair(void)
{


}

static void enter_setting_pre(void)
{

}

static void exit_setting_pre(void)
{

}

static void enter_setting_act(void)
{

}

static void exit_setting_act(void)
{

}

const App_evt_handler app_evt_handlers[EVT_NUM] = {
    [EVT_NONE] = NULL,
    [EVT_START_BROADCAST_PRE] = start_boardcast_pre,
    [EVT_EXIT_BROADCAST_PRE] = stop_boardcast_pre,
    [EVT_START_BROADCAST_ACT] = start_boardcast_act,
    [EVT_EXIT_BROADCAST_ACT] = stop_boardcast_act,
    [EVT_PTT_SWITCH] = switch_ptt,

    [EVT_START_INCALL_PRE] = start_incall_pre,
    [EVT_START_INCALL_ACT] = start_incall_act,
    [EVT_EXIT_INCALL_PRE] = exit_incall_pre,
    [EVT_EXIT_INCALL_ACT] = exit_incall_act,

    [EVT_START_DIRECT_CALL] = start_direct_call,

    [EVT_START_CALLING_PRE] = start_calling_pre,
    [EVT_EXIT_CALLING_PRE] = exit_calling_pre,
    [EVT_START_CALLED_PRE] = start_called_pre,
    [EVT_EXIT_CALLED_PRE] = exit_called_pre,
    [EVT_START_CALLING_ACT] = start_calling_act,
    [EVT_EXIT_CALLING_ACT] = exit_calling_act,
    [EVT_START_CALLED_ACT] = start_called_act,
    [EVT_EXIT_CALLED_ACT] = exit_called_act,

    [EVT_VOLUME_UP] = vol_up,
    [EVT_VOLUME_DOWN] = vol_down,

    [EVT_MUTE_SWITCH] = switch_mute,
    [EVT_AUTOCALL_SWITCH] = switch_autoCall,

    [EVT_POWER_OFF_PRE] = power_off_pre,
    [EVT_POWER_OFF_ACT] = power_off_act,
    [EVT_SLEEP_PRE] = sleep_pre,
    [EVT_SLEEP_ACT] = sleep_act,
    [EVT_SLEEP_REFRESH] = sleep_refresh,

    [EVT_ENTER_PAIR] = enter_pair,
    [EVT_EXIT_PAIR] = exit_pair,
    [EVT_ENTER_PSETTING_PRE] = enter_setting_pre,
    [EVT_EXIT_SETTING_PRE] = exit_setting_pre,
    [EVT_ENTER_SETTING_ACT] = enter_setting_act,
    [EVT_EXIT_SETTING_ACT] = exit_setting_act,
};

static uint8_t evt_buff[32];

void telep_init(void)
{
    App_cxt_t *cxt = get_app_cxt();
    memset(cxt, 0, sizeof(App_cxt_t));
    rb_init(&cxt->evt_rb, evt_buff, sizeof(evt_buff));
    memcpy(cxt->handle, app_evt_handlers, sizeof(app_evt_handlers));
    memset(&g_id_temp_msg, 0, sizeof(g_id_temp_msg));
}

void telep_deInit(void)
{
    memset(&g_app_cxt, 0, sizeof(g_app_cxt));
    memset(&g_id_temp_msg, 0, sizeof(g_id_temp_msg));
}

static bool num_key_evt_pop(Key_evt_t *evt)
{
    App_cxt_t *cxt = get_app_cxt();
    bool ret = true;

    if(evt == NULL) return false;
    if(evt->evt != KEY_VAL_CLICK)
        return false;
    if(evt->key_num != KEY_NUM0 && evt->key_num != KEY_NUM1 &&
        evt->key_num != KEY_NUM2 && evt->key_num != KEY_NUM3 &&
        evt->key_num != KEY_NUM4 && evt->key_num != KEY_NUM5 &&
        evt->key_num != KEY_NUM6 && evt->key_num != KEY_NUM7 &&
        evt->key_num != KEY_NUM8 && evt->key_num != KEY_NUM9)
        return false;
    if(cxt->state != APP_STATE_IDLE && cxt->state != APP_STATE_NUMDIAL
        && cxt->state != APP_STATE_CONFIG && cxt->state != APP_STATE_SETTING) {
        return false;
    }

    if(cxt->state == APP_STATE_NUMDIAL && g_id_temp_msg.did_index >= DID_INDEX_MAX) {
        return false;
    }
    if(cxt->configId_state == CONFIG_SID && g_id_temp_msg.sid_index >= SID_INDEX_MAX) {
        return false;
    }
    if(cxt->configId_state == CONFIG_GID && g_id_temp_msg.gid_index >= GID_INDEX_MAX) {
        return false;
    }
    if(cxt->configId_state == CONFIG_MID && g_id_temp_msg.mid_index >= MID_INDEX_MAX) {
        return false;
    }
    if(cxt->state == APP_STATE_SETTING && g_id_temp_msg.cid_index >= CID_INDEX_MAX) {
        return false;
    }

    if(cxt->state == APP_STATE_CONFIG) {
        switch(evt->key_num)
        {
        case KEY_NUM0: rb_write(&cxt->evt_rb, EVT_NUM0_C); break;
        case KEY_NUM1: rb_write(&cxt->evt_rb, EVT_NUM1_C); break;
        case KEY_NUM2: rb_write(&cxt->evt_rb, EVT_NUM2_C); break;
        case KEY_NUM3: rb_write(&cxt->evt_rb, EVT_NUM3_C); break;
        case KEY_NUM4: rb_write(&cxt->evt_rb, EVT_NUM4_C); break;
        case KEY_NUM5: rb_write(&cxt->evt_rb, EVT_NUM5_C); break;
        case KEY_NUM6: rb_write(&cxt->evt_rb, EVT_NUM6_C); break;
        case KEY_NUM7: rb_write(&cxt->evt_rb, EVT_NUM7_C); break;
        case KEY_NUM8: rb_write(&cxt->evt_rb, EVT_NUM8_C); break;
        case KEY_NUM9: rb_write(&cxt->evt_rb, EVT_NUM9_C); break;
        default: ret = false; break;
        }
        return true;
    }

    if(cxt->state == APP_STATE_IDLE) {
        rb_write(&cxt->evt_rb, EVT_START_DIAL);
    }

    // if(cxt->state == APP_STATE_NUMDIAL || cxt->state == APP_STATE_SETTING) {
    switch(evt->key_num)
    {
    case KEY_NUM0: rb_write(&cxt->evt_rb, EVT_NUM0_D); break;
    case KEY_NUM1: rb_write(&cxt->evt_rb, EVT_NUM1_D); break;
    case KEY_NUM2: rb_write(&cxt->evt_rb, EVT_NUM2_D); break;
    case KEY_NUM3: rb_write(&cxt->evt_rb, EVT_NUM3_D); break;
    case KEY_NUM4: rb_write(&cxt->evt_rb, EVT_NUM4_D); break;
    case KEY_NUM5: rb_write(&cxt->evt_rb, EVT_NUM5_D); break;
    case KEY_NUM6: rb_write(&cxt->evt_rb, EVT_NUM6_D); break;
    case KEY_NUM7: rb_write(&cxt->evt_rb, EVT_NUM7_D); break;
    case KEY_NUM8: rb_write(&cxt->evt_rb, EVT_NUM8_D); break;
    case KEY_NUM9: rb_write(&cxt->evt_rb, EVT_NUM9_D); break;
    default: ret = false; break;
    }
    return ret;
    // }
}

typedef struct {
    KEY_e key_num;
    key_evt_e key_evt;
    App_evt_e app_evt;
} Key_evt_map_t;

static const Key_evt_map_t idle_key_map[] = {
    {KEY_PTT,   KEY_VAL_HOLD,         EVT_START_BROADCAST_PRE},
    {KEY_CALL,  KEY_VAL_CLICK,        EVT_START_RECORD   },
    {KEY_Mode,  KEY_VAL_LONG_PRESSED, EVT_ENTER_PAIR },
    {KEY_Group, KEY_VAL_LONG_PRESSED, EVT_START_CONFIG   },
    {KEY_Mode,  KEY_VAL_CLICK,        EVT_START_CALLING_PRE  },
    {KEY_UP,    KEY_VAL_CLICK,        EVT_VOLUME_UP      },
    {KEY_DOWN,  KEY_VAL_CLICK,        EVT_VOLUME_DOWN    },
    {KEY_UP,    KEY_VAL_LONG_PRESSED, EVT_AUTOCALL_SWITCH},
    {KEY_DOWN,  KEY_VAL_LONG_PRESSED, EVT_MUTE_SWITCH},
    {KEY_POWER, KEY_VAL_HOLD,         EVT_POWER_OFF_PRE  },
    {KEY_NUM1,  KEY_VAL_LONG_PRESSED, EVT_PTT_SWITCH  },
    {KEY_NUM2,  KEY_VAL_LONG_PRESSED, EVT_START_DIRECT_CALL  },
};

static void app_key_push(Key_evt_t *evt)
{
    SysParam_t *sysParam = getSysParam();
    App_cxt_t *cxt = get_app_cxt();

    if(num_key_evt_pop(evt))
        return;

    switch(cxt->state)
    {
    case APP_STATE_IDLE:
    {
        for(uint8_t i = 0; i < sizeof(idle_key_map) / sizeof(idle_key_map[0]); i++) {
            if(evt->key_num == idle_key_map[i].key_num && evt->evt == idle_key_map[i].key_evt) {
                rb_write(&cxt->evt_rb, idle_key_map[i].app_evt);
                break;
            }
        }
        break;
    }
    case APP_STATE_BROADCAST:
    {
        if(evt->key_num == KEY_PTT && evt->evt == KEY_VAL_RELEASED) {
            rb_write(&cxt->evt_rb, EVT_EXIT_BROADCAST_PRE);
        } else if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_BROADCAST_PRE);
        }
        break;
    }
    case APP_STATE_NUMDIAL:
    {
        if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_DIAL);
        } else if(evt->key_num == KEY_CALL && evt->evt == KEY_VAL_CLICK) {
            // if(g_id_temp_msg.did_index == DID_INDEX_MAX)
            rb_write(&cxt->evt_rb, EVT_START_CALLING_PRE);
        } else if(evt->key_num == KEY_UP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_VOLUME_UP);
        } else if(evt->key_num == KEY_DOWN && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_VOLUME_DOWN);
        }
        break;
    }
    case APP_STATE_CONFIG:
    {
        if(evt->key_num == KEY_Group && evt->evt == KEY_VAL_CLICK) {
            if(cxt->configId_state == CONFIG_ID_MAX - 1) {
                rb_write(&cxt->evt_rb, EVT_EXIT_CONFIG);
            } else {
                rb_write(&cxt->evt_rb, EVT_NEXT_CONFIG);
            }
        } else if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_CONFIG);
        }
        break;
    }
    case APP_STATE_SETTING:
    {
        if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_PAIR);
        } else if(evt->key_num == KEY_CALL && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_ENTER_PSETTING_PRE);
        }
        break;
    }
    case APP_STATE_CALLING_MASTER:
    {
        if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_CALLING_PRE);
        }
        break;
    }
    case APP_STATE_CALLED_SLAVE:
    {
        if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_CALLED_PRE);
        } else if(evt->key_num == KEY_CALL && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_START_INCALL_PRE);
        }
        break;
    }
    case APP_STATE_INCALL:
    {
        if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_INCALL_PRE);
        } else if(evt->key_num == KEY_UP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_VOLUME_UP);
        } else if(evt->key_num == KEY_DOWN && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_VOLUME_DOWN);
        }
        break;
    }
    case APP_STATE_RECORD:
    {
        if(evt->key_num == KEY_HANGUP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_EXIT_RECORD);
        } else if(evt->key_num == KEY_CALL && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_START_CALLING_PRE);
        } else if(evt->key_num == KEY_UP && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_LCD_UP);
        } else if(evt->key_num == KEY_DOWN && evt->evt == KEY_VAL_CLICK) {
            rb_write(&cxt->evt_rb, EVT_LCD_DOWN);
        }
        break;
    }

    default:
        break;
    }
}

static void app_module_push(Module_evt_t *evt)
{
    SysParam_t *sysParam = getSysParam();
    App_cxt_t *cxt = get_app_cxt();

    switch(cxt->state)
    {
    case APP_STATE_IDLE:
    {
        if(evt->evt == MODULE_EVT_RING) {
            rb_write(&cxt->evt_rb, EVT_START_CALLED_PRE);
        } else if(evt->evt == MODULE_EVT_CALL) {
            rb_write(&cxt->evt_rb, EVT_START_CALLING_ACT);
        } else if(evt->evt == MODULE_EVT_BROADCAST_START) {
            rb_write(&cxt->evt_rb, EVT_START_BROADCAST_ACT);
        } else if(evt->evt == MODULE_EVT_REFRESH_BAT) {
            rb_write(&cxt->evt_rb, EVT_BAT_REFRESH);
        } else if(evt->evt == MODULE_EVT_POWER_OFF) {
            rb_write(&cxt->evt_rb, EVT_POWER_OFF_ACT);
        } else if(evt->evt == MODULE_EVT_SLEEP) {
            rb_write(&cxt->evt_rb, EVT_SLEEP_ACT);
        }
        // else if(evt->evt == MODULE_EVT_CALLED) {// 事件的pop有时间响应的延迟
        //     rb_write(&cxt->evt_rb, EVT_START_CALLED_ACT);
        // }
        else if(evt->evt == MODULE_EVT_HANGUP) {
            rb_write(&cxt->evt_rb, EVT_EXIT_CALLED_ACT);
        } else if(evt->evt == MODULE_EVT_INCALL_PRE) {
            rb_write(&cxt->evt_rb, EVT_START_INCALL_PRE);
        } else if(evt->evt == MODULE_EVT_INCALL_ACT) {
            rb_write(&cxt->evt_rb, EVT_START_INCALL_ACT);
        } else if(evt->evt == MODULE_EVT_RECORDID_UPDATED) {
            rb_write(&cxt->evt_rb, EVT_REFRESH_RECORD);
        }
        break;
    }
    case APP_STATE_BROADCAST:
    {
        if(evt->evt == MODULE_EVT_BROADCAST_END) {
            rb_write(&cxt->evt_rb, EVT_EXIT_BROADCAST_ACT);
        }
        break;
    }
    case APP_STATE_NUMDIAL:
    {
        if(evt->evt == MODULE_EVT_RING) {
            rb_write(&cxt->evt_rb, EVT_START_CALLED_PRE);
        } else if(evt->evt == MODULE_EVT_CALL) {
            rb_write(&cxt->evt_rb, EVT_START_CALLING_ACT);
        }

        break;
    }
    case APP_STATE_CONFIG:
    {
        if(evt->evt == MODULE_EVT_RING) {
            rb_write(&cxt->evt_rb, EVT_START_CALLED_PRE);
        }

        break;
    }
    case APP_STATE_CALLING_MASTER:
    {
        if(evt->evt == MODULE_EVT_HANGUP) {
            rb_write(&cxt->evt_rb, EVT_EXIT_CALLING_ACT);
        } else if(evt->evt == MODULE_EVT_INCALL_ACT) {
            rb_write(&cxt->evt_rb, EVT_START_INCALL_ACT);
        }

        break;
    }
    case APP_STATE_CALLED_SLAVE:
    {
        if(evt->evt == MODULE_EVT_HANGUP) {
            rb_write(&cxt->evt_rb, EVT_EXIT_CALLED_ACT);
        } else if(evt->evt == MODULE_EVT_RECORDID_UPDATED) {
            rb_write(&cxt->evt_rb, EVT_REFRESH_RECORD);
        } else if(evt->evt == MODULE_EVT_INCALL_ACT) {
            rb_write(&cxt->evt_rb, EVT_START_INCALL_ACT);
        } else if(evt->evt == MODULE_EVT_CALLED) {
            rb_write(&cxt->evt_rb, EVT_START_CALLED_ACT);
        }

        break;
    }
    case APP_STATE_INCALL:
    {
        if(evt->evt == MODULE_EVT_EXIT_INCALL) {
            rb_write(&cxt->evt_rb, EVT_EXIT_INCALL_ACT);
        }
        break;
    }
    case APP_STATE_RECORD:
    {
        if(evt->evt == MODULE_EVT_RING) {
            rb_write(&cxt->evt_rb, EVT_EXIT_INCALL_ACT);
        } else if(evt->evt == MODULE_EVT_CALL) {
            rb_write(&cxt->evt_rb, EVT_START_CALLING_ACT);
        }
        break;
    }

    default:
        break;
    }
}

static void app_evt_pop(void)
{
    App_cxt_t *cxt = get_app_cxt();
    uint8_t data;
    while(rb_read(&cxt->evt_rb, &data, 0) > 0) {
        App_evt_handler handler = cxt->handle[(App_evt_e) data];
        if(handler != NULL) {
            handler();
        }
    }
}

void app_proc(void)
{
    SysParam_t *sysParam = getSysParam();
    App_cxt_t *cxt = get_app_cxt();

    if(cxt->statePre != cxt->state) {
        LOG_DEBUG("app state: %s -> %s", app_state_name(cxt->statePre), app_state_name(cxt->state));
        cxt->statePre = cxt->state;
    }

    Sleep_evt_e sleep_evt;
    if(app_sleep_evt_pop(&sleep_evt)) {
        if(sleep_evt == SLEEP_EVT_SLEEP) {
            rb_write(&cxt->evt_rb, EVT_SLEEP_PRE);
        } else if(sleep_evt == SLEEP_EVT_RESUME) {
            rb_write(&cxt->evt_rb, EVT_SLEEP_REFRESH);
        }
    }

    Key_evt_t key_evt;
    if(key_evt_pop(&key_evt)) {
        app_key_push(&key_evt);
    }

    Module_evt_t module_evt;
    if(app_module_evt_pop(&module_evt)) {
        app_module_push(&module_evt);
    }

    app_evt_pop();
}
