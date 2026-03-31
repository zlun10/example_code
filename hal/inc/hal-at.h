#ifndef _HAL_AT_H
#define _HAL_AT_H

/**
 * @file hal-at.h
 * @brief AT 通信硬件抽象层（HAL）接口定义
 *
 * 本文件定义了 AT 命令收发所需的硬件操作接口。
 * 移植到新芯片/平台时，只需在对应的 hal/src/hal-at.c 中实现这些函数，
 * 上层 dev/src/at.c 及应用层代码无需任何修改。
 *
 * 接口说明：
 *   - hal_at_init()             : 初始化 AT UART 外设
 *   - hal_at_deInit()           : 反初始化 AT UART 外设
 *   - hal_at_sendData()         : 格式化发送 AT 命令字符串
 *   - hal_at_clear()            : 清空 AT 接收缓冲区
 *   - hal_at_buff()             : 获取 AT 接收原始缓冲区指针（可选）
 *   - hal_at_is_line_ready()    : 查询是否已接收到完整的一行数据
 *   - hal_at_get_line()         : 获取并消费一行接收数据
 *   - hal_at_reinit_baudrate()  : 重新初始化 UART 波特率
 */

#include <stdint.h>
#include <stdbool.h>

/** @brief 单行接收缓冲区最大长度（含终止符 '\0'） */
#define HAL_AT_LINE_BUF_LEN  100

/**
 * @brief 初始化 AT UART 外设
 *
 * 从系统参数中读取波特率，初始化 UART 硬件，使能接收中断。
 */
void hal_at_init(void);

/**
 * @brief 反初始化 AT UART 外设
 *
 * 禁用中断，关闭 UART 时钟，释放 GPIO 资源。
 */
void hal_at_deInit(void);

/**
 * @brief 格式化发送 AT 命令字符串（自动追加 "\r\n"）
 *
 * @param format printf 风格格式字符串
 * @param ...    可变参数
 */
void hal_at_sendData(const char *format, ...);

/**
 * @brief 清空 AT 接收缓冲区
 */
void hal_at_clear(void);

/**
 * @brief 获取 AT 接收原始缓冲区指针
 *
 * @return 指向内部行缓冲区的指针（只读）
 */
uint8_t *hal_at_buff(void);

/**
 * @brief 查询是否已接收到完整的一行数据（以 "\r\n" 结尾）
 *
 * @return true  — 已有完整行数据可读
 * @return false — 尚未接收完整行
 */
bool hal_at_is_line_ready(void);

/**
 * @brief 获取并消费一行接收数据
 *
 * 将当前行数据复制到 out_buf，并清空内部行缓冲区。
 * 调用前应先通过 hal_at_is_line_ready() 确认数据就绪。
 *
 * @param out_buf  输出缓冲区
 * @param buf_size 输出缓冲区大小（含终止符 '\0'）
 * @return true  — 成功获取一行数据
 * @return false — 无数据或参数无效
 */
bool hal_at_get_line(char *out_buf, uint16_t buf_size);

/**
 * @brief 重新初始化 UART 为指定波特率
 *
 * 在波特率切换（AT+BAUDRATE 命令响应）后调用，以新波特率重新配置 UART。
 *
 * @param baudrate 目标波特率
 */
void hal_at_reinit_baudrate(uint32_t baudrate);

#endif /* _HAL_AT_H */
