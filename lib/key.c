/**
 * @brief   按键处理
 */
#include <stdbool.h>
#include "key.h"
#include "../hal/inc/hal_key.h"
#include "../lib/rb.h"
#include "../dev/inc/log.h"

#define KEY_POWER_LONG_PRESS 400 // 10ms

typedef enum {
    KEY_STA_IDLE,
    KEY_STA_PRESSED,
    KEY_STA_WAIT,
}key_sta_m;

typedef struct {
    key_sta_m sta;
    uint32_t count;
}key_t;

typedef struct
{
    key_t keys[KEY_NUM];
    key_evt_e key_evt[KEY_NUM];
    rb_t evt_rb;
}Key_cxt_t;

static Key_cxt_t g_key_cxt;

static Key_cxt_t *get_key_cxt(void)
{
    return &g_key_cxt;
}

static uint8_t evt_buff[32];

void app_key_init(void)
{
    hal_key_init();
    memset(&g_key_cxt, 0, sizeof(g_key_cxt));
    rb_init(&g_key_cxt.evt_rb, evt_buff, sizeof(evt_buff));
}

void app_key_deInit(void)
{
    hal_key_deInit();

    memset(&g_key_cxt, 0, sizeof(g_key_cxt));
}

void key_scan(void *param)
{
    (void) param;
    Key_cxt_t *cxt = get_key_cxt();
    hal_key_scan();

    // 按键状态的产生与消耗应当同步
    for(uint8_t i = 0; i < KEY_NUM; i++)
    {
        uint8_t pinValue = hal_key_getLevel((KEY_e) i);

        switch(cxt->keys[i].sta) {
        case KEY_STA_IDLE:
            cxt->key_evt[i] = KEY_VAL_IDLE;
            if(pinValue == KEY_PRESSED_VALUE) {
                cxt->keys[i].sta = KEY_STA_PRESSED;
                cxt->keys[i].count = 0;
            }
            break;

        case KEY_STA_PRESSED:
            if(pinValue == KEY_PRESSED_VALUE) {
                cxt->keys[i].count++;

                // PTT
                if(i == KEY_PTT && cxt->keys[i].count >= 20) {
                    cxt->keys[i].sta = KEY_STA_WAIT;
                    cxt->key_evt[i] = KEY_VAL_HOLD;
                    Key_evt_t evt;
                    evt.key_num = i;
                    evt.evt = cxt->key_evt[i];
                    rb_write(&cxt->evt_rb, evt.date);
                    cxt->keys[i].count = 0;
                    break;
                }

                // POWER
                if(i == KEY_POWER) {
                    if(cxt->keys[i].count >= KEY_POWER_LONG_PRESS)
                    {
                        cxt->keys[i].sta = KEY_STA_WAIT;
                        cxt->key_evt[i] = KEY_VAL_HOLD;
                        Key_evt_t evt;
                        evt.key_num = i;
                        evt.evt = cxt->key_evt[i];
                        rb_write(&cxt->evt_rb, evt.date);
                        cxt->keys[i].count = 0;
                        break;
                    }
                } else { /* wait release */
                    if(cxt->keys[i].count >= 300) {
                        cxt->keys[i].sta = KEY_STA_WAIT;
                        cxt->key_evt[i] = KEY_VAL_LONG_PRESSED;
                        Key_evt_t evt;
                        evt.key_num = i;
                        evt.evt = cxt->key_evt[i];
                        rb_write(&cxt->evt_rb, evt.date);
                        cxt->keys[i].count = 0;
                    } else {
                        cxt->key_evt[i] = KEY_VAL_IDLE;
                    }
                }
            } else {
                if(cxt->keys[i].count < 5) {
                    cxt->keys[i].sta = KEY_STA_IDLE;
                    cxt->key_evt[i] = KEY_VAL_IDLE;
                } else {
                    cxt->keys[i].sta = KEY_STA_IDLE;
                    cxt->key_evt[i] = KEY_VAL_CLICK;
                    Key_evt_t evt;
                    evt.key_num = i;
                    evt.evt = cxt->key_evt[i];
                    rb_write(&cxt->evt_rb, evt.date);
                }
                cxt->keys[i].count = 0;
            }
            break;

        case KEY_STA_WAIT:
            cxt->key_evt[i] = KEY_VAL_IDLE;
            if(pinValue != KEY_PRESSED_VALUE) {
                cxt->keys[i].sta = KEY_STA_IDLE;
                cxt->keys[i].count = 0;

                if(i == KEY_PTT) {
                    cxt->key_evt[i] = KEY_VAL_RELEASED;
                    Key_evt_t evt;
                    evt.key_num = i;
                    evt.evt = cxt->key_evt[i];
                    rb_write(&cxt->evt_rb, evt.date);
                }
            }
            break;

        default:
            cxt->keys[i].sta = KEY_STA_IDLE;
            cxt->keys[i].count = 0;
            cxt->key_evt[i] = KEY_VAL_IDLE;
            break;
        }
    }
}

void key_direct_evt(KEY_e i, key_evt_e evt)
{
    Key_cxt_t *cxt = get_key_cxt();
    Key_evt_t k_evt;
    k_evt.key_num = i;
    k_evt.evt = evt;
    rb_write(&cxt->evt_rb, k_evt.date);
}

bool key_evt_pop(Key_evt_t *evt)
{
    Key_cxt_t *cxt = get_key_cxt();

    if(evt == NULL) {
        return false;
    }

    if(rb_read(&cxt->evt_rb, &evt->date, 0) > 0) {
        return true;
    }

    return false;
}

bool is_all_key_idle(void)
{
    Key_cxt_t *cxt = get_key_cxt();

    for(uint8_t i = 0; i < KEY_NUM; i++) {
        if(cxt->keys[i].count != 0) {
            // LOG_DEBUG("key %d count %d", i, cxt->keys[i].count);
            return false;
        }
    }


    return true;
}
