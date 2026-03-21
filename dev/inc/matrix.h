#ifndef _MATRIX_H
#define _MATRIX_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    KEY_INPUT = 0,
    KEY_WAIT,
    KEY_OUTPUT
}KEY_SCAN_S;

// 根据行列的排列
typedef enum {
    M_KEY_NUM1,
    M_KEY_NUM2,
    M_KEY_NUM3,
    M_KEY_Group,
    M_KEY_NUM4,
    M_KEY_NUM5,
    M_KEY_NUM6,
    M_KEY_Mode,
    M_KEY_NUM7,
    M_KEY_NUM8,
    M_KEY_NUM9,
    M_KEY_HANGUP,
    M_KEY_DOWN,
    M_KEY_NUM0,
    M_KEY_UP,
    M_KEY_CALL,
    M_KEY_NUM,
} M_key_e;

// event
void matrix_init(void);
void matrix_deInit(void);

// loop
void matrixScan(void);// 10ms

// status
uint8_t matrixGet(M_key_e key_num);

#endif
