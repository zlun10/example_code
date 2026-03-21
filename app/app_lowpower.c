#include "app_lowpower.h"
#include <stdint.h>
#include <stdbool.h>
#include "cw32l011_sysctrl.h"
#include "cw32l011_gpio.h"
#include "../config/config.h"
#include "../drv/inc/drv_gpio.h"
#include "../drv/inc/drv_uart.h"
#include "../drv/inc/drv_timer.h"
#include "../drv/inc/drv_rcc.h"
#include "../lib/misc.h"
#include "../lib/storage.h"
#include "../lib/key.h"
#include "../lib/storage.h"
#include "../dev/inc/at.h"
#include "../dev/inc/module.h"
#include "../dev/inc/log.h"
#include "../dev/inc/sys_tick.h"
#include "../app/app_module.h"
#include "../app/app_sleep.h"
#include "../examples/telephone/telephone.h"

#define POWER_KEY_IRQn GPIOC_IRQn

typedef enum {
    LOWPOWER_IDLE = 0,
    LOWPOWER_SLEEP,
    LOWPOWER_DEEPSLEEP,
    LOWPOWER_RESUMING,
} LowPowerSta_e;

typedef struct {
    LowPowerSta_e sta;
    uint32_t sleepTick;
    bool resumeTrue;   // debounce
}LowPower_cxt_t;

static LowPower_cxt_t cxt = {
    .sta = LOWPOWER_IDLE,
    .sleepTick = 0,
    .resumeTrue = false,
};

static LowPower_cxt_t *get_cxt(void)
{
    return &cxt;
}

static void pKeyISR_EN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = POWER_KEY_PIN;
    GPIO_Init(POWER_KEY_PORT, &GPIO_InitStruct);
    GPIO_ConfigFilter(POWER_KEY_PORT, POWER_KEY_PIN, GPIO_FLTCLK_HCLK8);
    GPIOC_INTFLAG_CLR(POWER_KEY_PIN);
    NVIC_ClearPendingIRQ(POWER_KEY_IRQn);
    NVIC_EnableIRQ(POWER_KEY_IRQn);
    NVIC_SetPriority(POWER_KEY_IRQn, 1);
}

static void pKeyISR_DIS(void)
{
    GPIOC_INTFLAG_CLR(POWER_KEY_PIN);
    NVIC_ClearPendingIRQ(POWER_KEY_IRQn);
    NVIC_DisableIRQ(POWER_KEY_IRQn);

    gpio_modecfg(GPIO_POWER_OUT, GPIO_MODE_IN);
}

static void allKeyISR_EN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = GPIO_PIN_7;
    GPIO_Init(CW_GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.IT = GPIO_IT_FALLING;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = GPIO_PIN_13;
    GPIO_Init(CW_GPIOC, &GPIO_InitStruct);
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pins = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
    GPIO_WritePin(CW_GPIOA, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_Pin_RESET);

    GPIOA_INTFLAG_CLR(GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8);
    GPIOB_INTFLAG_CLR(GPIO_PIN_7);
    GPIOC_INTFLAG_CLR(GPIO_PIN_13);
    NVIC_ClearPendingIRQ(GPIOA_IRQn);
    NVIC_EnableIRQ(GPIOA_IRQn);
    NVIC_SetPriority(GPIOA_IRQn, 1);
    NVIC_ClearPendingIRQ(GPIOB_IRQn);
    NVIC_EnableIRQ(GPIOB_IRQn);
    NVIC_SetPriority(GPIOB_IRQn, 1);
    NVIC_ClearPendingIRQ(GPIOC_IRQn);
    NVIC_EnableIRQ(GPIOC_IRQn);
    NVIC_SetPriority(GPIOC_IRQn, 1);
}

static void allKeyISR_DIS(void)
{
    GPIOA_INTFLAG_CLR(GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8);
    GPIOB_INTFLAG_CLR(GPIO_PIN_7);
    GPIOC_INTFLAG_CLR(GPIO_PIN_13);
    NVIC_ClearPendingIRQ(GPIOA_IRQn);
    NVIC_DisableIRQ(GPIOA_IRQn);
    NVIC_ClearPendingIRQ(GPIOB_IRQn);
    NVIC_DisableIRQ(GPIOB_IRQn);
    NVIC_ClearPendingIRQ(GPIOC_IRQn);
    NVIC_DisableIRQ(GPIOC_IRQn);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
    GPIO_WritePin(CW_GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_Pin_SET);
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = GPIO_PIN_7;
    GPIO_Init(CW_GPIOB, &GPIO_InitStruct);
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_InitStruct.Pins = GPIO_PIN_13;
    GPIO_Init(CW_GPIOC, &GPIO_InitStruct);
}

static UNUSED void deepSleepProc(void)
{
    suspend_sysTick();
    telep_deInit();
    app_sleep_deInit();

    app_module_deInit();
    app_key_deInit();
    timer_deInit();
    log_deInit();
    flash_deInit();
    pKeyISR_EN();
    rcc_deInit();
    SYSCTRL_GotoDeepSleep();
}

static UNUSED void sleepProc(void)
{
    suspend_sysTick();
    LOG_DEBUG("Enter sleep proc");
    telep_deInit();
    app_sleep_deInit();

    app_module_deInit();
    timer_deInit();
    log_deInit();
    flash_deInit();
    allKeyISR_EN();
    rcc_deInit();
    SYSCTRL_GotoDeepSleep();
}

static UNUSED void deepSleep_resume(void)
{
    resume_sysTick();
    LowPower_cxt_t *cxt = get_cxt();
    rcc_init();
    module_notify_resume();
    pKeyISR_DIS();
    log_init();
    timer_init();
    flash_resume();
    app_key_init();
    app_sleep_init();
    telep_init();
    cxt->sleepTick = millis();
    gpio_modecfg(GPIO_POWER_OUT, GPIO_MODE_OUT);
    gpio_set(GPIO_POWER_OUT, GPIO_RESET);
    LOG_INFO("Deep sleep resume");
}

static bool loopResume(void)
{
    gpio_set(GPIO_POWER_OUT, GPIO_RESET);
    block_delayMs_96MHz(20);
    gpio_set(GPIO_POWER_OUT, GPIO_SET);

    if(buadrate_scan(0) == false) {
        return false;
    } else {
        return true;
    }
}

static UNUSED uint8_t sleep_resume(void)
{
    const uint8_t MAX_RETRY = 3;
    uint8_t retry;

    resume_sysTick();
    rcc_init();
    timer_init();
    gpio_modecfg(GPIO_POWER_OUT, GPIO_MODE_OUT);

    for(uint8_t retry = 0; retry < MAX_RETRY; retry++) {
        bool result = loopResume();
        if(result == true) {
            break;
        }
    }

    if(retry >= MAX_RETRY) {
        return 0;
    }

    allKeyISR_DIS();
    log_init();
    flash_resume();
    app_key_init();
    at_init();
    at_send(AT_CMD_BAT);
    module_init();

    telep_init();

    return 1;
}

/**
 * @brief 模拟按下电源键唤醒模组的过程
 */
static UNUSED int8_t simulatePowerKeyPress(void)
{
    LowPower_cxt_t *cxt = get_cxt();
    if(millis() - cxt->sleepTick > RESUME_DELAY_MS) {
        gpio_set(GPIO_POWER_OUT, GPIO_SET);
        LOG_INFO("Simulate power key press");
        if(!buadrate_scan(1)) {
            LOG_FATAL("AT UART baudrate scan failed after deepsleep resume");
            return -1;
        }
        LOG_INFO("System HCLK Frequency: %lu Hz", SYSCTRL_GetHClkFreq());
        return 1;
    } else {
        return 0;
    }
}

void app_lowPower_test(void)
{
    LowPower_cxt_t *cxt = get_cxt();
    switch(cxt->sta)
    {
    case LOWPOWER_IDLE:
        break;
    case LOWPOWER_SLEEP:
        sleepProc();
        if(!sleep_resume()) {
            cxt->sta = LOWPOWER_SLEEP;
            break;
        }
        cxt->sta = LOWPOWER_IDLE;
        break;
    case LOWPOWER_DEEPSLEEP:
        deepSleepProc();
        if(!cxt->resumeTrue) {
            cxt->sta = LOWPOWER_DEEPSLEEP;
            break;
        }
        deepSleep_resume();
        cxt->sta = LOWPOWER_RESUMING;
        break;
    case LOWPOWER_RESUMING:
        switch(simulatePowerKeyPress()) {
        case -1:
            cxt->sta = LOWPOWER_DEEPSLEEP;
            LOG_INFO("Deep sleep resume failed");
            break;
        case 0:
            break;
        case 1:
            app_module_init();

            cxt->sta = LOWPOWER_IDLE;
            LOG_INFO("System resume complete");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

}

bool is_lowPower(void)
{
    LowPower_cxt_t *cxt = get_cxt();
    return (cxt->sta == LOWPOWER_IDLE);
}

void enter_sleep(void)
{
    LowPower_cxt_t *cxt = get_cxt();
    cxt->sta = LOWPOWER_SLEEP;
}

void enter_deepSleep(void)
{
    LowPower_cxt_t *cxt = get_cxt();
    cxt->sta = LOWPOWER_DEEPSLEEP;
}

void GPIOC_IRQHandler(void)
{
    LowPower_cxt_t *cxt = get_cxt();
    if(CW_GPIOC->ISR & POWER_KEY_PIN) {
        GPIOC_INTFLAG_CLR(POWER_KEY_PIN);
        block_delayMs_4M(5);
        if(gpio_get(GPIO_POWER_IN) == GPIO_RESET) {
            cxt->resumeTrue = true;
        } else {
            cxt->resumeTrue = false;
        }
    }
}

void GPIOA_IRQHandler(void)
{
    if(CW_GPIOA->ISR & (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8))
    {
        GPIOA_INTFLAG_CLR(GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_8);
    }
}
