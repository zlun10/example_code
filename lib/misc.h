#ifndef _MISC_H
#define _MISC_H

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef UNUSED
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define UNUSED __attribute__((unused))
#elif defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// 计算m的n次方 (m^n)
uint32_t POW(uint8_t m, uint8_t n);

bool CharToHex(char ch, uint8_t *hex);
uint16_t String2Hex(char *str, uint8_t *hex, uint16_t len);
uint8_t *GetSub(uint8_t *data, uint32_t dataLen, char *sub, uint32_t subLen);
bool ParseInt(uint8_t *str, char *prefix, char *suffix, uint8_t offset, uint8_t len, int32_t *result);
uint16_t HEX2DEC(uint8_t *hex, uint8_t len);



void block_delayMs_96MHz(uint32_t ms);
void block_delayMs_4M(uint32_t ms);

// 线性映射函数
int32_t line_map(int32_t value, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max);
float mapf(float value, float in_min, float in_max, float out_min, float out_max);

#endif
