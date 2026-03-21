#ifndef _DRV_TIMER_H
#define _DRV_TIMER_H

#include <stdint.h>

void timer_init(void);
void timer_deInit(void);
uint16_t timer_getTick(void);
uint8_t timer_exceed(uint16_t ref, uint16_t us);
void timer_delayUs(uint16_t us);
void timer_delayMs(uint32_t ms);

#endif
