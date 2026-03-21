#include <stdint.h>
#include <stddef.h>
#include "app_sleep.h"
#include "../dev/inc/sys_tick.h"
#include "../dev/inc/at.h"
#include "../dev/inc/log.h"
#include "../lib/key.h"
#include "../examples/telephone/telephone.h"

#define SLEEP_TIMEOUT 10000 // ms

typedef struct
{
    uint32_t last_ms;
}Sleep_cxt_t;

Sleep_cxt_t sleep_cxt = { 0 };

Sleep_cxt_t *get_sleep_cxt(void)
{
    return &sleep_cxt;
}

void app_sleep_init(void)
{
    Sleep_cxt_t *cxt = get_sleep_cxt();
    cxt->last_ms = millis();
}

void app_sleep_deInit(void)
{
    Sleep_cxt_t *cxt = get_sleep_cxt();
    cxt->last_ms = millis();
}

bool app_sleep_evt_pop(Sleep_evt_e *sleep_evt)
{
    if(sleep_evt == NULL) {
        return false;
    }

    Sleep_cxt_t *cxt = get_sleep_cxt();

    if(!is_app_idle() || !is_all_key_idle()) {
        cxt->last_ms = millis();
    }

    if(millis() - cxt->last_ms > SLEEP_TIMEOUT) {
        cxt->last_ms = millis();
        *sleep_evt = SLEEP_EVT_SLEEP;
        return true;
    }

    return false;
}

void app_sleep_refresh(void)
{
    Sleep_cxt_t *cxt = get_sleep_cxt();
    cxt->last_ms = millis();
}
