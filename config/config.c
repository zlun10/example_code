#include "config.h"
#include <stdint.h>

const uint8_t MID_INIT_ARRAY[MID_INDEX_MAX] = { 6, 6, 6, 6 };
const uint8_t SID_INIT_ARRAY[SID_INDEX_MAX] = { 8, 8, 8, 8 };
const uint8_t GID_INIT_ARRAY[GID_INDEX_MAX] = { 9, 9 };

static SysParam_t g_sysParam = { 0 };

SysParam_t *getSysParam(void)
{
    return &g_sysParam;
}
