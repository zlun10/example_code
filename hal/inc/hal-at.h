#ifndef _HAL_AT_H
#define _HAL_AT_H

#include <stdint.h>

void hal_at_init(void);
void hal_at_deInit(void);
void hal_at_sendData(const char *format, ...);
uint8_t *hal_at_buff(void);
void hal_at_clear(void);

#endif
