#include "../inc/sys_tick.h"
#include <stdint.h>
#include "cw32l011_systick.h"
#include "cw32l011_sysctrl.h"

void systick_init(void)
{
    InitTick(SYSCTRL_GetHClkFreq());
}

inline uint32_t millis(void)
{
    return GetTick();
}

inline void suspend_sysTick(void)
{
    SuspendTick();
}

inline void resume_sysTick(void)
{
    ResumeTick();
}
