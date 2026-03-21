/**
 * @file storage.c
 * @brief flash存储相关
 * @note 128页，每页512字节；后面几页轮询进行存储数据
 */
#include "storage.h"
#include <stdint.h>
#include "../hal/inc/hal-storage.h"

#include "../config/config.h"

#include "../dev/inc/log.h"
#include "../dev/inc/sys_tick.h"


#define PAGE_SIZE 512 // bytes
#define USE_PAGE_NUM 4
#define USE_PAGE_FIRST (128 - USE_PAGE_NUM) // 120 -127
#define PIECE_MAX_CNT (PAGE_SIZE / sizeof(NvmParam_t))
 // nvmParams占用36Bytes, 1页能用14次 (approximately)
#define FLASH_REFRESH_TIME 3000 // ms

typedef struct
{
    bool     refresh_en;
    uint32_t refresh_tick;
    uint8_t  current_page;
    uint8_t  current_piece;
}Flash_cxt;

static Flash_cxt flash_cxt = { false, 0, USE_PAGE_FIRST, 0 };

static Flash_cxt *get_storage_cxt(void)
{
    return &flash_cxt;
}

static void refresh_flash(void)
{
    Flash_cxt *cxt = get_storage_cxt();
    SysParam_t *sysParam = getSysParam();

    if(++cxt->current_piece > PIECE_MAX_CNT - 1) {
        cxt->current_piece = 0;
        if(++cxt->current_page > USE_PAGE_FIRST + USE_PAGE_NUM - 1)
            cxt->current_page = USE_PAGE_FIRST;

        // 擦除上一页
        uint8_t last_page = (cxt->current_page == USE_PAGE_FIRST) ? (USE_PAGE_FIRST + USE_PAGE_NUM - 1) : (cxt->current_page - 1);
        __disable_irq();
        hal_storage_unlockPage(last_page);
        while(hal_storage_erasePage(last_page));
        hal_storage_lockPage();
        __enable_irq();
    }

    __disable_irq();// 带擦除整个时间5ms；不擦除直接写200us
    hal_storage_unlockPage(cxt->current_page);
    hal_storage_writeBytes(PAGE_SIZE * cxt->current_page + sizeof(sysParam->nvmParams) * cxt->current_piece, (uint8_t *) &sysParam->nvmParams, sizeof(sysParam->nvmParams));
    hal_storage_lockPage();
    __enable_irq();

    LOG_INFO("Flash write complete at page %d piece %d", cxt->current_page, cxt->current_piece);
}

/**
* @brief 查找哪个是最新的
* @note  理论上只有一页上有magicID：1、先找页 2、倒序找最新
*/
static uint8_t scanFlash(void)
{
    Flash_cxt *cxt = get_storage_cxt();
    SysParam_t *sysParam = getSysParam();
    int8_t target_page = -1;

    for(uint8_t i = 0; i < USE_PAGE_NUM; i++)
    {
        uint32_t addr = PAGE_SIZE * (USE_PAGE_FIRST + i);
        NvmParam_t temp;
        memcpy((uint8_t *) &temp, (void *) addr, sizeof(NvmParam_t));

        if(temp.magicID == FLASH_MAGIC_ID)
        {
            target_page = USE_PAGE_FIRST + i;
            LOG_DEBUG("Flash scan found valid page at %d", target_page);
            break;
        }
    }

    if(target_page >= 0)
    {
        for(int8_t j = PIECE_MAX_CNT - 1; j >= 0; j--)
        {
            uint32_t addr = PAGE_SIZE * target_page + j * sizeof(sysParam->nvmParams);// 8:offset
            memcpy((uint8_t *) &sysParam->nvmParams, (void *) addr, sizeof(sysParam->nvmParams));

            if(sysParam->nvmParams.magicID == FLASH_MAGIC_ID) {
                cxt->current_page = target_page;
                cxt->current_piece = j;
                sysParam->is_writed = 1;
                LOG_DEBUG("Flash scan complete at page %d piece %d", cxt->current_page, cxt->current_piece);
                return 1;
            }
        }
        LOG_DEBUG("Flash scan found no valid piece in page %d", target_page);
    }

    // 未找到有效数据
    LOG_DEBUG("Flash scan found no valid data");
    return 0;
}

void flash_init(void)
{
    Flash_cxt *cxt = get_storage_cxt();
    SysParam_t *sysParam = getSysParam();
    hal_storage_init();

    if(!scanFlash()) {
        // 默认参数
        memcpy(sysParam->nvmParams.mid, MID_INIT_ARRAY, sizeof(MID_INIT_ARRAY));
        memcpy(sysParam->nvmParams.sid, SID_INIT_ARRAY, sizeof(SID_INIT_ARRAY));
        memcpy(sysParam->nvmParams.gid, GID_INIT_ARRAY, sizeof(GID_INIT_ARRAY));
        sysParam->nvmParams.sgid_flag = 0;
        sysParam->nvmParams.sgid_id = 0;
        sysParam->nvmParams.volume = VOLUME_INIT;
        sysParam->nvmParams.data.autoCall = AUTO_CALL_INIT;
        sysParam->nvmParams.data.recordIdFlag = 0x00;
        memset((void *) sysParam->nvmParams.recordId, 0, sizeof(sysParam->nvmParams.recordId));
        sysParam->nvmParams.magicID = FLASH_MAGIC_ID;

        hal_storage_unlockPage(cxt->current_page);
        hal_storage_writeBytes(PAGE_SIZE * cxt->current_page + sizeof(sysParam->nvmParams) * cxt->current_piece, (uint8_t *) &sysParam->nvmParams, sizeof(sysParam->nvmParams));
        hal_storage_lockPage();
    }
}

void flash_deInit(void)
{
    Flash_cxt *cxt = get_storage_cxt();
    hal_storage_deInit();
    cxt->refresh_en = false;
}

void flash_resume(void)
{
    hal_storage_init();
}

void flash_addRecordId(uint8_t *id)
{
    SysParam_t *sysParam = getSysParam();
    for(uint8_t i = RECORD_ID_COUNT - 1; i > 0; i--) {
        memcpy(sysParam->nvmParams.recordId[i], sysParam->nvmParams.recordId[i - 1], DID_INDEX_MAX);
    }
    sysParam->nvmParams.data.recordIdFlag = 0x10 | (sysParam->nvmParams.data.recordIdFlag >> 1);// 最新的在最高位
    memcpy(sysParam->nvmParams.recordId[0], id, DID_INDEX_MAX);
    refreshNVM_EN();
}

void refreshNVM_EN(void)
{
    Flash_cxt *cxt = get_storage_cxt();
    cxt->refresh_en = true;
    cxt->refresh_tick = millis();
}

void refresh_flash_task(void)
{
    Flash_cxt *cxt = get_storage_cxt();
    if(cxt->refresh_en) {
        if(millis() - cxt->refresh_tick < FLASH_REFRESH_TIME) {
            return;
        }
        refresh_flash();
        LOG_DEBUG("Flash NVM refresh");
        cxt->refresh_en = false;
    }
}
