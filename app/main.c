#include "main.h"
void mcu_init(void);

#define VERSION "V0.0.1"

int main(void)
{
    MODULE_RST();
    mcu_init();

    app_module_init();
    app_key_init();
    app_sleep_init();
    telep_init();

    LOG_INFO("System Init Success! %s", VERSION);


    while(1) {
        app_module_task();
        app_lowPower_test();
        refresh_flash_task();
        ProcessTimerEvent();
        app_proc();
    }
}

void mcu_init(void)
{
    rcc_init();
    timer_init();
    flash_init();
    systick_init();
    log_init();
    timerEventInit();
    StartTimerEvent(TIMER_ID_KEY_SCAN, ClkSrc10ms, 1, 1, key_scan, NULL);
    StartTimerEvent(TIMER_ID_SYN_SCAN, ClkSrc1second, 1, 5, module_notify_post, NULL);
}
