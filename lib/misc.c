#include "misc.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define __NOP() __asm volatile ("nop")

/**** 进制转换 ****/
/**
 * @brief 十六进制字符转十六进制数
 * @param ch 十六进制字符
 * @param hex 十六进制数
 * @return 是否成功
 */
bool CharToHex(char ch, uint8_t *hex)
{
    if(ch >= '0' && ch <= '9')
    {
        *hex = ch - '0';
        return true;
    }

    if(ch >= 'A' && ch <= 'F')
    {
        *hex = ch - 'A' + 10;
        return true;
    }

    if(ch >= 'a' && ch <= 'f')
    {
        *hex = ch - 'a' + 10;
        return true;
    }

    return false;
}

/**
 * @brief 十六进制字符串转十六进制数组
 * @param str 十六进制字符串
 * @param hex 十六进制数组，注意长度，避免溢出
 * @param len 字符串长度
 * @return 转换成功的数组长度
 */
uint16_t String2Hex(char *str, uint8_t *hex, uint16_t len)
{
    uint8_t hb, lb;
    uint32_t i = 0;
    uint16_t outlen = 0;

    if(len == 0)
    {
        return 0;
    }

    if(len % 2 != 0 && CharToHex(str[0], &lb))
    {
        i++;
        hex[outlen++] = lb;
    }

    while(i < len)
    {
        if(!CharToHex(str[i++], &hb) || !CharToHex(str[i++], &lb))
        {
            break;
        }

        hex[outlen++] = (hb << 4) | lb;
    }

    return outlen;
}

/**
 * @brief 获取子数据
 * @param data 完整数据
 * @param dataLen 完整数据的长度
 * @param sub 要获取的子数据
 * @param subLen 子数据的长度
 * @return 子数据在完整数据中的地址
 */
uint8_t *GetSub(uint8_t *data, uint32_t dataLen, char *sub, uint32_t subLen)
{
    if((dataLen == 0 && (dataLen = strlen((char *) data)) == 0) || (subLen == 0 && (subLen = strlen((char *) sub)) == 0))
    {
        return NULL;
    }

    while(dataLen >= subLen) {
        if(*data == *sub) {
            uint8_t *pData = data + 1;
            uint8_t *pSub = (uint8_t *) sub + 1;
            uint32_t sameLen = 1;
            while(sameLen < subLen && *pData++ == *pSub++) {
                sameLen++;
            }
            if(sameLen == subLen) {
                return data;
            }
        }

        data++;
        dataLen--;
    }

    return NULL;
}

/**
 * @brief 从字符串里解析一个整数
 * @param str 字符串，如果prefix不为NULL，从prefix结束之后解析整数
 * @param prefix 识别前缀（不包含），如果为NULL则直接从str解析整数
 * @param suffix 识别后缀（不包含），如果为NULL则根据len判断数据结束，如果len也为0则直至碰到非数字
 * @param offset 起始地址偏移量
 * @param len 数据长度，如果为0则根据suffix判断数据结束，如果suffix也为NULL则直至碰到非数字
 * @param result 解析结果保存地址
 * @return 是否解析成功
 */
bool ParseInt(uint8_t *str, char *prefix, char *suffix, uint8_t offset, uint8_t len, int32_t *result)
{
    uint8_t *pStart = prefix == NULL ? str : GetSub(str, strlen((char *) str), prefix, strlen(prefix));
    if(pStart == NULL)
    {
        return false;
    }

    pStart += (prefix == NULL ? 0 : strlen(prefix)) + offset;
    uint8_t *pEnd = (suffix != NULL) ? GetSub(pStart, strlen((char *) pStart), suffix, strlen(suffix)) : (pStart + (len > 0 ? len : 10));
    if(pEnd <= pStart)
    {
        return false;
    }

    int8_t sign = 1;
    if(*pStart == '-') {
        sign = -1;
        pStart++;
        if(suffix == NULL && len == 0) {
            pEnd++;
        }
    }

    int32_t value = 0;
    uint64_t temp = 0;

    while(pStart < pEnd)
    {
        if(*pStart < '0' || *pStart > '9')
        {
            if(suffix == NULL && len == 0)
            {
                break;
            }
            return false;
        }

        value = value * 10 + (*pStart - '0');
        temp = temp * 10 + (*pStart - '0');
        pStart++;
    }

    if(sign == 1 && temp > 0x7fffffff)
    {
        // LOG_DEBUG("false");
        return false;
    }
    if(result != NULL)
    {
        *result = sign * value;
    }

    return true;
}

uint16_t HEX2DEC(uint8_t *hex, uint8_t len)
{
    uint16_t dec = 0;
    for(uint8_t i = 0; i < len; i++) {
        dec = dec * 10 + hex[i];
    }
    return dec;
}

/**** 一些通用 ****/
/**
 * @brief 定时器延时的补充；主频96MHz
 *
 * @param ms
 */
void block_delayMs_96MHz(uint32_t ms)
{
    for(volatile uint32_t i = 0; i < 5000 * ms; i++)
    {
        __NOP();
    }
}

/**
 * @brief 定时器延时的补充；主频96MHz
 *
 * @param ms
 */
void block_delayMs_4M(uint32_t ms)
{
    for(volatile uint32_t i = 0; i < 412 * ms; i++)
    {
        __NOP();
    }
}

// 计算m的n次方 (m^n)
inline uint32_t POW(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while(n--) result *= m;
    return result;
}

/**
 * @brief 线性映射函数（整数版本）
 * @param value 输入值
 * @param in_min 输入范围最小值
 * @param in_max 输入范围最大值
 * @param out_min 输出范围最小值
 * @param out_max 输出范围最大值
 * @return 映射后的值
 * @note 使用公式: output = (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
 */
int32_t line_map(int32_t value, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    // 防止除零错误
    if(in_max == in_min)
    {
        return out_min;
    }

    // 限制输入值在输入范围内（可选）
    if(value < in_min) value = in_min;
    if(value > in_max) value = in_max;

    // 线性映射计算
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief 线性映射函数（浮点版本）
 * @param value 输入值
 * @param in_min 输入范围最小值
 * @param in_max 输入范围最大值
 * @param out_min 输出范围最小值
 * @param out_max 输出范围最大值
 * @return 映射后的值
 * @note 浮点版本提供更高精度，适用于需要精确计算的场景
 */
float mapf(float value, float in_min, float in_max, float out_min, float out_max)
{
    // 防止除零错误
    if(in_max == in_min)
    {
        return out_min;
    }

    // 限制输入值在输入范围内（可选）
    if(value < in_min) value = in_min;
    if(value > in_max) value = in_max;

    // 线性映射计算
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


