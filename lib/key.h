#ifndef _KEY_H
#define _KEY_H

#pragma anon_unions

#include <stdint.h>
#include <stdbool.h>
#include "../hal/inc/hal_key.h"

#define KEY_PRESSED_VALUE (0) // 低电平按下

typedef enum {
    KEY_VAL_IDLE,
    KEY_VAL_CLICK,
    KEY_VAL_LONG_PRESSED,
    KEY_VAL_HOLD,           // 短按按住
    KEY_VAL_RELEASED,       // 短按按住释放
} key_evt_e;

typedef struct {
    union {
        uint8_t date;
        struct {
            uint8_t  key_num : 5;
            uint8_t  evt : 3;
        };
    };
}Key_evt_t;

typedef enum {
    KEY_PTT_IDLE,
    KEY_PTT_ACTIVE,
    KEY_PTT_UNACTIVE,
} key_ptt_t;

// event
void app_key_init(void);
void app_key_deInit(void);
void key_direct_evt(KEY_e i, key_evt_e evt);

// loop
void key_scan(void *); // 10ms

// sync
bool key_evt_pop(Key_evt_t *evt);
bool is_all_key_idle(void);

#endif
