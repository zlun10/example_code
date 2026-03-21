/**
 * @file at.c
 * @brief AT命令处理模块
 * @note
 * 已知：即使正确发过去，模组可能是接收失败 -> 重试 （概率很小）
 *      模组的中断通知：
*/
#include "../inc/at.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../../dev/inc/log.h"
#include "../../dev/inc/sys_tick.h"

#include "../../lib/misc.h"
#include "../../lib/rb.h"

#include "../../drv/inc/drv_uart.h"
#include "../../hal/inc/hal-at.h"
#include "../../config/config.h"

#include "../../examples/telephone/telephone.h"

#define AT_WAIT_TIMEOUT 2000 // ms

// AT命令相关常量
#define MODULE_VOLUME_MIN      1
#define MODULE_VOLUME_MAX      8
#define MODULE_VOLUME_OFFSET   8    // 模组音量偏移量（输入1-8映射到9-16）

// AT命令字符串映射表
static const char *g_at_cmd_str_table[] = {
    "NONE",
    "call",
    "PICKUP",
    "call id",
    "hangup",
    "set vol",
    "set mute",
    "cancel mute",
    "set gid",
    "set sid",
    "modeflag",
    "get bat",
    "get recordid",
    "restore",
    "play ring",
    "baudrate",
    "set sleep",
    "broadcast",
    "shutdown",
    "setting",
    "sleep",
    "version",
};

static const char *get_at_cmd_str(AtCmd_e cmd)
{
    if(cmd < sizeof(g_at_cmd_str_table) / sizeof(g_at_cmd_str_table[0])) {
        return g_at_cmd_str_table[cmd];
    }
    return "AT_CMD_UNKNOWN";
}

// AT响应字符串表
typedef enum {
    AT_RSP_OK,
    AT_RSP_ERROR,
    AT_RSP_MODE_FLAG,
    AT_RSP_BAT,
    AT_RSP_RECORDID,
    AT_RSP_VERSION,
    AT_RSP_MAX
} AtRspStr_e;

static const char *at_rsp_str_table[AT_RSP_MAX] = {
    "AT_OK\r\n",
    "ERROR\r\n",
    "+MODEFLAG:",
    "+BAT:",
    "+RECORDID:",
    "+VER:",
};

typedef enum {
    MODE_FLAG_NONE = 0,
    MODE_FLAG_MASTER_CALL,        // 主机呼叫中
    MODE_FLAG_SLAVE_RING,         // 从机振铃中（被呼叫）
    MODE_FLAG_MASTER_TALKING,     // 主机通话中
    MODE_FLAG_SLAVE_TALKING,      // 从机通话中
    MODE_FLAG_MASTER_BROADCAST,   // 广播发送中
    MODE_FLAG_SLAVE_BROADCAST,    // 广播接听中
    MODE_FLAG_SETTING,            // 配对中
    MODE_FLAG_INVITE,             // 配对邀请中
    MODE_FLAG_HANGUP,             // 正在挂断
    MODE_FLAG_MAX,
} mode_flag_e;

typedef struct {
    At_status_e status;
    At_status_e status_prev;
    AtCmd_e     cmd;
    uint32_t    waitCnt;    // 用于AT回复等待计时
    uint32_t    waitTime;   // 用于AT回复等待计时
    mode_flag_e modeFlag; // 模组反馈的状态
    rb_t cmd_rb;
    rb_t tone_rb;          // 铃声队列
    rb_t evt_rb;
    uint8_t retry_cnt;
    char multi_line_buf[256];  // 多行响应缓冲区
    uint16_t multi_line_len;   // 多行响应长度
} At_context_s;

At_context_s g_at_context;

static At_context_s *get_at_cxt(void)
{
    return &g_at_context;
}

static uint8_t at_cmd_buf[24];
static uint8_t at_tone_buf[24];
static uint8_t at_evt_buf[24];

void at_init(void)
{
    At_context_s *cxt = get_at_cxt();
    hal_at_init();

    memset(cxt, 0, sizeof(*cxt));
    rb_init(&cxt->cmd_rb, at_cmd_buf, sizeof(at_cmd_buf));
    rb_init(&cxt->tone_rb, at_tone_buf, sizeof(at_tone_buf));
    rb_init(&cxt->evt_rb, at_evt_buf, sizeof(at_evt_buf));
}

void at_deInit(void)
{
    At_context_s *cxt = get_at_cxt();
    hal_at_deInit();

    memset(cxt, 0, sizeof(*cxt));
}

void send_cmd_pro(AtCmd_e cmd)
{
    SysParam_t *sysParam = getSysParam();

    switch(cmd) {
    case AT_CMD_VERSION:
        hal_at_sendData("AT+VER?");
        break;
    case AT_CMD_CALL:
        hal_at_sendData("AT+CALL");
        break;
    case AT_CMD_PICKUP:
        hal_at_sendData("AT+PICKUP");
        break;
    case AT_CMD_CALL_ID: {
        uint8_t par[4];
        memcpy(par, sysParam->call_id_temp, 4);
        hal_at_sendData("AT+CALL=%02X%02X%02X%02X", par[0], par[1], par[2], par[3]);
        break;
    }
    case AT_CMD_HANGUP:
        hal_at_sendData("AT+HANGUP");
        break;
    case AT_CMD_VOLUME:
    {
        // 将系统音量 1-8 映射到模组音量 2-16
        uint8_t volume = (uint8_t) line_map(sysParam->nvmParams.volume, 1, 8, 2, 16);
        hal_at_sendData("AT+VOLUME=%d", volume);
        break;
    }
    case AT_CMD_SET_MUTE:
        hal_at_sendData("AT+MUTE=%d", sysParam->mute);
        break;
    case AT_CMD_CANCEL_MUTE:
        hal_at_sendData("AT+MUTE=%d", 0);
        break;
    case AT_CMD_GIDSET:
    {
        uint8_t gid = sysParam->nvmParams.gid[0] * 10 + sysParam->nvmParams.gid[1];
        hal_at_sendData("AT+GROUPGID=%02X%02X", gid, gid);
        break;
    }
    case AT_CMD_SIDSET:
    {
        uint16_t sid = HEX2DEC(sysParam->nvmParams.sid, 4);
        uint8_t sid_buff[4] = { 0 };
        sid_buff[0] = (sysParam->nvmParams.sgid_flag << 7) | (sysParam->nvmParams.sgid_id & 0x7F);
        sid_buff[2] = (sid >> 8) & 0xFF;
        sid_buff[3] = sid & 0xFF;
        hal_at_sendData("AT+GROUPSID=%02X%02X%02X%02X",
            sid_buff[0], sid_buff[1], sid_buff[2], sid_buff[3]);
        break;
    }
    case AT_CMD_MODEFLAG:
        hal_at_sendData("AT+MODEFLAG?");
        break;
    case AT_CMD_BAT:
        hal_at_sendData("AT+BAT?");
        break;
    case AT_CMD_RECORDID:
        hal_at_sendData("AT+RECORDID?");
        break;
    case AT_CMD_RESTORE:
        hal_at_sendData("AT+RESTORE");
        break;
    case AT_CMD_PLAYRING:
    {
        At_context_s *cxt = get_at_cxt();
        uint8_t tone;
        if(rb_read(&cxt->tone_rb, &tone, 0) > 0) {
            hal_at_sendData("AT+PLAYRING=%d", tone);
        } else {
            LOG_ERROR("AT_CMD_PLAYRING but no tone in rb");
        }
        break;
    }
    case AT_CMD_BAUDRATE:
        hal_at_sendData("AT+BAUDRATE=%d", MCU_BAUDRATE_INIT);
        break;
    case AT_CMD_SET_SLEEP:
        hal_at_sendData("AT+SLEEP=%d", 0); // 0：是不休眠
        break;
    case AT_CMD_BROADCAST:
        hal_at_sendData("AT+BROADCAST=%d", sysParam->broadcast); // 1: 开启广播 0：关闭广播
        break;
    case AT_CMD_SHUTDOWN:
        hal_at_sendData("AT+SHUTDOWN");
        break;
    case AT_CMD_SLEEP:
        hal_at_sendData("AT+SLEEP");
        break;
    case AT_CMD_SETTING:
        hal_at_sendData("AT+SETTING");
        break;
    case AT_CMD_INVITE:
        hal_at_sendData("AT+INVITE");
        break;
    case AT_CMD_MID:
    {
        Id_temp_msg *temp_id = get_temp_id();
        uint8_t *params = temp_id->cid;
        hal_at_sendData("AT+MID=%02x%02x%02x%02x", params[0], params[1], params[2], params[3]);
        break;
    }
    default:
        LOG_WARN("Unknown AT command: %d", cmd);
        break;
    }
}

void at_send(AtCmd_e cmd)
{
    At_context_s *cxt = get_at_cxt();
    rb_write(&cxt->cmd_rb, cmd);
}

void at_set_tone(ToneName_e tone)
{
    At_context_s *cxt = get_at_cxt();
    rb_write(&cxt->tone_rb, (uint8_t) tone);
}

void at_clear(void)
{
    hal_at_clear();
}

static void statusChange(At_status_e new_status)
{
    At_context_s *cxt = get_at_cxt();
    if(new_status == AT_IDLE) {
        cxt->cmd = AT_CMD_NONE;
        at_clear();
    }
    cxt->status = new_status;
}

/* 不清除上次的数据，重新来过一遍at流程 */
static void status_retry(void)
{
    At_context_s *cxt = get_at_cxt();
    cxt->retry_cnt++;
    LOG_INFO("AT cmd retry cnt:%d, cmd:%s", cxt->retry_cnt, get_at_cmd_str(cxt->cmd));
    if(cxt->retry_cnt > 3) {
        cxt->retry_cnt = 0;
        LOG("AT cmd retry exceed!!!");
        return;
    }
    at_clear();
    cxt->status = AT_SEND;
}

static char module_version[8];
void parserVersion(const char *at_buff)
{
    SysParam_t *sysParam = getSysParam();
    At_context_s *cxt = get_at_cxt();
    char *pos = strstr(at_buff, at_rsp_str_table[AT_RSP_VERSION]);
    if(pos) {
        pos += strlen(at_rsp_str_table[AT_RSP_VERSION]);
        while(*pos == ' ' || *pos == '\t') pos++;
        uint8_t len = strcspn(pos, "\r\n");
        if(len == 0) {
            LOG_ERROR("version len is 0");
            return;
        }
        if(len > (sizeof(module_version) - 1)) {
            len = sizeof(module_version) - 1;
        }
        memcpy(module_version, pos, len);
        module_version[len] = '\0';
        sysParam->module_version = module_version;
        LOG_INFO("module: %s", sysParam->module_version);
        return;
    }
}

static uint8_t parserModeFlag(const char *at_buff, modeflag_status_t *status)
{
    char *pos = strstr(at_buff, at_rsp_str_table[AT_RSP_MODE_FLAG]);
    if(pos) {
        pos += strlen(at_rsp_str_table[AT_RSP_MODE_FLAG]);
        uint8_t tmpId[4] = { 0 };
        if(String2Hex((char *) pos, tmpId, 8) != 4)
        {
            LOG_ERROR("modeflag String2Hex fail");
            return 0;
        }

        // 大小端转换
        status->bytes[0] = tmpId[3];
        status->bytes[1] = tmpId[2];
        status->bytes[2] = tmpId[1];
        status->bytes[3] = tmpId[0];

        LOG_INFO("modeflag: %02X%02X%02X%02X", tmpId[0], tmpId[1], tmpId[2], tmpId[3]);

        return 1;
    } else {
        LOG_ERROR("modeflag prefix no fd");
        LOG_ERROR("resp:%s", at_buff);
        return 0;
    }
}

static uint8_t parserRecordID(const char *at_buff)
{
    At_context_s *cxt = get_at_cxt();
    SysParam_t *sysParam = getSysParam();
    char *pos = strstr(at_buff, at_rsp_str_table[AT_RSP_RECORDID]);
    if(pos) {
        pos += strlen(at_rsp_str_table[AT_RSP_RECORDID]);
        uint8_t tmpId[4] = { 0 };
        if(String2Hex((char *) pos, tmpId, 8) != 4)
        {
            LOG_ERROR("recordid String2Hex fail");
            return 0;
        }
        memcpy(sysParam->record_id_temp, tmpId, 4);
        rb_write(&cxt->evt_rb, AT_EVT_CALLED_ACT);
        rb_write(&cxt->evt_rb, AT_EVT_RECORDID_UPDATED);

        LOG_INFO("record id:%02X%02X%02X%02X", tmpId[0], tmpId[1], tmpId[2], tmpId[3]);

        return 1;
    } else {
        LOG_ERROR("recordid prefix no fd");
        return 0;
    }
}

static uint8_t parserBat(const char *at_buff)
{
    SysParam_t *sysParam = getSysParam();
    At_context_s *cxt = get_at_cxt();
    char *pos = strstr(at_buff, at_rsp_str_table[AT_RSP_BAT]);
    if(pos) {
        pos += strlen(at_rsp_str_table[AT_RSP_BAT]);
        int32_t bat_value = 0;
        ParseInt((uint8_t *) pos, NULL, "\r\n", 0, 0, &bat_value);
        if((uint8_t) bat_value != sysParam->bat) {
            sysParam->bat = (uint8_t) bat_value;
            rb_write(&cxt->evt_rb, AT_EVT_REFRESH_BAT);
        }
        LOG_INFO("BATTERY: %d", sysParam->bat);

        return 1;
    }
    return 0;
}

typedef void (*AtCmdHandler_f)(const char *at_buff);

typedef struct {
    AtCmd_e cmd;
    AtCmdHandler_f handler;
} AtCmdHandlerEntry_t;

static void handle_modeflag(const char *at_buff)
{
    SysParam_t *sysParam = getSysParam();
    if(parserModeFlag(at_buff, &sysParam->modeflag_status)) {
        LOG_INFO("Parsed: MODE");
    } else {
        LOG_ERROR("Parse MODEFLAG fail");
    }
}

static void handle_recordid(const char *at_buff)
{
    if(parserRecordID(at_buff)) {
        LOG_INFO("Parsed: RECORDID");
    } else {
        LOG_ERROR("Parse RECORDID fail");
    }
}

static void handle_bat(const char *at_buff)
{
    if(parserBat(at_buff)) {
        LOG_INFO("Parsed: BAT");
    } else {
        LOG_ERROR("Parse BAT fail");
    }
}

static void handle_baudrate(const char *at_buff)
{
    (void) at_buff;  // 未使用
    drv_atUart_init(MCU_BAUDRATE_INIT);
    LOG_DEBUG("handle baudrate\n");
}

static void handle_shutdown(const char *at_buff)
{
    (void) at_buff;  // 未使用
    At_context_s *cxt = get_at_cxt();
    rb_write(&cxt->evt_rb, AT_EVT_POWER_OFF_ACT);
    LOG_DEBUG("handle shutdown\n");
}

static void handle_sleep(const char *at_buff)
{
    (void) at_buff;  // 未使用
    At_context_s *cxt = get_at_cxt();
    rb_write(&cxt->evt_rb, AT_EVT_SLEEP_ACT);
    LOG_DEBUG("handle sleep\n");
}

static void handle_pickup(const char *at_buff)
{
    (void) at_buff;  // 未使用
    At_context_s *cxt = get_at_cxt();
    rb_write(&cxt->evt_rb, AT_EVT_PICKUP_ACT);
    LOG_DEBUG("handle pickup\n");
}

// AT命令响应处理回调表
static const AtCmdHandlerEntry_t g_at_cmd_handlers[] = {
    {AT_CMD_MODEFLAG, handle_modeflag},
    {AT_CMD_RECORDID, handle_recordid},
    {AT_CMD_BAT,      handle_bat},
    {AT_CMD_BAUDRATE, handle_baudrate},
    {AT_CMD_SHUTDOWN, handle_shutdown},
    {AT_CMD_SLEEP,    handle_sleep},
    {AT_CMD_PICKUP,   handle_pickup},
    {AT_CMD_VERSION,  parserVersion},
};

void at_task(void)
{
    SysParam_t *sysParam = getSysParam();
    At_context_s *cxt = get_at_cxt();

    if(cxt->status != cxt->status_prev) {
        // LOG_DEBUG("AT S change: %d -> %d", cxt->status_prev, cxt->status); // 太频繁，对rtt log有影响
        cxt->status_prev = cxt->status;
    }

    switch(cxt->status)
    {
    case AT_IDLE:
        if(rb_read(&cxt->cmd_rb, (uint8_t *) &cxt->cmd, 0) > 0) {
            cxt->retry_cnt = 0;
            statusChange(AT_SEND);
            LOG_INFO("dequeue cmd:%s", get_at_cmd_str(cxt->cmd));
        }
        break;
    case AT_SEND:
        send_cmd_pro(cxt->cmd);
        cxt->waitCnt = millis();
        if(cxt->cmd == AT_CMD_VOLUME || cxt->cmd == AT_CMD_PLAYRING) {
            cxt->waitTime = 500; // 音量和铃声的AT指令，模组处理较快，缩短等待时间
        } else {
            cxt->waitTime = AT_WAIT_TIMEOUT;
        }
        cxt->multi_line_len = 0;
        memset(cxt->multi_line_buf, 0, sizeof(cxt->multi_line_buf));
        statusChange(AT_WAIT_RECV);
        break;
    case AT_WAIT_RECV:
    {
        if((millis() - cxt->waitCnt) < cxt->waitTime) {
            UartRcv_s *uart = getAtUart();

            if(uart->line_ready) {
                char line_buf[UART_LINE_BUF_LEN];

                if(drv_atUart_getLine(line_buf, sizeof(line_buf))) {
                    uint16_t line_len = strlen(line_buf);
                    if((cxt->multi_line_len + line_len) < sizeof(cxt->multi_line_buf)) {
                        memcpy(cxt->multi_line_buf + cxt->multi_line_len, line_buf, line_len);
                        cxt->multi_line_len += line_len;
                        cxt->multi_line_buf[cxt->multi_line_len] = '\0';
                    } else {
                        LOG_ERROR("Multi-line buffer overflow");
                    }

                    // 检查是否收到完整响应（AT_OK或ERROR）
                    bool has_ok = (strstr(cxt->multi_line_buf, at_rsp_str_table[AT_RSP_OK]) != NULL);
                    bool has_error = (strstr(cxt->multi_line_buf, at_rsp_str_table[AT_RSP_ERROR]) != NULL);

                    if(has_ok) {
                        bool handled = false;

                        for(uint8_t i = 0; i < sizeof(g_at_cmd_handlers) / sizeof(g_at_cmd_handlers[0]); i++) {
                            if(cxt->cmd == g_at_cmd_handlers[i].cmd) {
                                g_at_cmd_handlers[i].handler(cxt->multi_line_buf);  // 传递完整的多行数据
                                handled = true;
                                break;
                            }
                        }

                        if(!handled) LOG_INFO("only ok parsed");
                        statusChange(AT_IDLE);
                    } else if(has_error) {
                        LOG_ERROR("AT response ERROR: %s", cxt->multi_line_buf);
                        statusChange(AT_STATUS_ERROR);
                    }
                    // else: 继续等待更多行数据
                }
            }

        } else {
            if(cxt->cmd == AT_CMD_VOLUME || cxt->cmd == AT_CMD_PLAYRING) {
                LOG_DEBUG("not ok parsed");
                statusChange(AT_IDLE);
                break;
            }
            status_retry();
            LOG_WARN("AT Command timeout");
        }
        break;
    }
    case AT_STATUS_ERROR:
    {
        LOG_ERROR("at status Error");
        statusChange(AT_IDLE);
        break;
    }

    default:
        break;
    }
}

bool at_evt_pop(AT_evt_e *evt)
{
    At_context_s *cxt = get_at_cxt();

    if(evt == NULL) {
        return false;
    }

    if(rb_read(&cxt->evt_rb, (uint8_t *) evt, 0) > 0) {
        return true;
    }

    return false;
}
