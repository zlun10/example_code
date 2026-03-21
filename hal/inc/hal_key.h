#ifndef _HAL_KEY_H
#define _HAL_KEY_H

#include <stdint.h>

typedef enum {
    KEY_NUM1,
    KEY_NUM2,
    KEY_NUM3,
    KEY_Group,
    KEY_NUM4,
    KEY_NUM5,
    KEY_NUM6,
    KEY_Mode,
    KEY_NUM7,
    KEY_NUM8,
    KEY_NUM9,
    KEY_HANGUP,
    KEY_DOWN,
    KEY_NUM0,
    KEY_UP,
    KEY_CALL,
    KEY_MATRIX_NUM,
    KEY_PTT = KEY_MATRIX_NUM,
    KEY_POWER,
    KEY_NUM,
} KEY_e;

// evt
void hal_key_init(void);
void hal_key_deInit(void);

// loop
void hal_key_scan(void);

// status
uint8_t hal_key_getLevel(KEY_e key_num);

#endif
