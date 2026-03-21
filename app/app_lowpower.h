#ifndef _APP_LOWPOWER_H
#define _APP_LOWPOWER_H

#include <stdbool.h>

#define RESUME_DELAY_MS 3300 // <10的倍数> 大于模组判定时间3s

// event
void enter_sleep(void);
void enter_deepSleep(void);

// loop
void app_lowPower_test(void);

// status
bool is_lowPower(void);

#endif
