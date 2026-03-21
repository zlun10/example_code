#ifndef _APP_SLEEP_H
#define _APP_SLEEP_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SLEEP_EVT_NONE = 0,
    SLEEP_EVT_SLEEP,
    SLEEP_EVT_RESUME,
}Sleep_evt_e;

// event
void app_sleep_init(void);
void app_sleep_deInit(void);
void app_sleep_refresh(void);

// sync
bool app_sleep_evt_pop(Sleep_evt_e *evt);

#endif
