#ifndef _SYS_TICK_H
#define _SYS_TICK_H

#include <stdint.h>

// event
void systick_init(void);
void suspend_sysTick(void);
void resume_sysTick(void);

// status
uint32_t millis(void);

#endif
