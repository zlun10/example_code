# README

# 嵌入式无线对讲机控制器 — 开发手册

## 目录

1. [项目简介](#1-项目简介)
2. [代码架构](#2-代码架构)
3. [目录结构](#3-目录结构)
4. [模块说明](#4-模块说明)
5. [AT 命令层使用说明](#5-at-命令层使用说明)
6. [模组业务层使用说明](#6-模组业务层使用说明)
7. [移植指南](#7-移植指南)
8. [接口参考](#8-接口参考)

---

## 1. 项目简介

本项目是基于 **CW32L011** MCU 的嵌入式无线对讲机控制程序，通过 AT 命令与无线模组通信，实现拨号、接听、挂断、广播、音量控制等功能。

**重构目标：**
- 将 AT 命令逻辑与底层硬件解耦，通过 HAL（硬件抽象层）隔离
- 将模组业务逻辑与底层 GPIO/中断代码分离
- 使得移植到其他芯片时，只需实现 `hal/` 目录下的接口函数，上层代码无需改动

---

## 2. 代码架构

```
┌─────────────────────────────────────────────────┐
│               应用层 (app/ examples/)            │
│   main.c  app_module.c  app_lowpower.c  ...      │
├─────────────────────────────────────────────────┤
│               设备层 (dev/)                      │
│   at.c（AT 状态机）  module.c（模组业务逻辑）   │
│   log.c  sys_tick.c  matrix.c                   │
├────────────────────┬────────────────────────────┤
│   硬件抽象层 (hal/)│  通用库 (lib/)              │
│   hal-at.h/c       │  rb.c  misc.c  key.c  ...  │
│   hal_module.h/c   │                            │
│   hal-storage.h/c  │                            │
├────────────────────┴────────────────────────────┤
│               驱动层 (drv/)                      │
│   drv_uart.c  drv_gpio.c  drv_timer.c  ...      │
├─────────────────────────────────────────────────┤
│               芯片 SDK (chip/cw32l011/)          │
│   StdDriver/  CMSIS/                            │
└─────────────────────────────────────────────────┘
```

**各层职责：**

| 层级 | 目录 | 职责 | 可移植性 |
|------|------|------|---------|
| 应用层 | `app/`, `examples/` | 业务逻辑、状态机、UI | 较高 |
| 设备层 | `dev/` | AT协议处理、模组事件管理 | **完全可移植** |
| HAL 层 | `hal/` | 硬件操作接口（需按平台实现） | 接口固定，实现可替换 |
| 驱动层 | `drv/` | 芯片外设驱动 | 芯片专用 |
| 芯片 SDK | `chip/` | 芯片厂商库 | 芯片专用 |

---

## 3. 目录结构

```
example_code/
├── app/                    应用层
│   ├── main.c              系统入口与初始化
│   ├── app_module.c/h      模组应用层（对 dev/module 的封装）
│   ├── app_lowpower.c/h    低功耗管理
│   └── app_sleep.c/h       休眠控制
│
├── dev/                    设备层（平台无关）
│   ├── inc/
│   │   ├── at.h            AT 命令接口
│   │   ├── module.h        模组业务接口
│   │   ├── log.h           日志接口
│   │   └── sys_tick.h      系统滴答计时
│   └── src/
│       ├── at.c            AT 命令状态机（仅调用 hal-at.h 接口）
│       ├── module.c        模组业务逻辑（仅调用 hal_module.h 接口）
│       ├── log.c
│       └── sys_tick.c
│
├── hal/                    硬件抽象层（移植时修改此目录）
│   ├── inc/
│   │   ├── hal-at.h        AT UART 抽象接口
│   │   ├── hal_module.h    模组硬件抽象接口
│   │   └── hal-storage.h   存储抽象接口
│   └── src/
│       ├── hal-at.c        CW32L011 AT UART 实现
│       ├── hal_module.c    CW32L011 模组 GPIO/ISR 实现
│       └── hal-storage.c   CW32L011 Flash 实现
│
├── drv/                    驱动层（CW32L011 专用）
│   ├── inc/drv_uart.h      UART 驱动接口
│   ├── inc/drv_gpio.h      GPIO 驱动接口
│   └── src/...
│
├── lib/                    通用库（平台无关）
│   ├── rb.c/h              环形缓冲区
│   ├── misc.c/h            工具函数（字符串、延时等）
│   ├── key.c/h             按键扫描库
│   ├── storage.c/h         参数存储管理
│   └── timer_affair.c/h    软件定时器
│
├── chip/cw32l011/          芯片 SDK
│   ├── CMSIS/              ARM CMSIS 核心
│   └── StdDriver/          CW32L011 外设驱动库
│
├── config/
│   ├── config.h            系统参数结构体定义
│   └── config.c
│
└── examples/telephone/     电话业务示例
    ├── telephone.h
    └── telephone.c
```

---

## 4. 模块说明

### 4.1 AT 命令模块 (`dev/at.c`)

AT 命令模块实现了与无线模组通信的完整状态机：

- **状态机流程：** `AT_IDLE` → `AT_SEND` → `AT_WAIT_RECV` → `AT_IDLE`
- **队列化发送：** 所有 AT 命令通过环形缓冲区排队，按序执行
- **超时重试：** 未收到响应时自动重试，最多 3 次
- **多行响应：** 支持累积多行响应后统一解析
- **事件上报：** 解析完成后通过事件队列通知上层

**与底层完全解耦：** `at.c` 只调用 `hal-at.h` 定义的接口，不包含任何芯片头文件。

### 4.2 模组业务模块 (`dev/module.c`)

- 维护模组中断通知计数器（中断安全）
- 实现 `hal_module_notify_callback()`，供 HAL ISR 回调
- 通过 `module_synch()` 将系统参数同步到模组
- 所有硬件操作委托给 `hal_module.h` 接口

**与底层完全解耦：** `module.c` 不包含任何芯片头文件（无 `cw32l011_gpio.h` 等）。

### 4.3 AT HAL 层 (`hal/hal-at.c`)

封装了所有 UART 相关的硬件操作：
- 初始化/反初始化 AT UART
- 格式化发送 AT 命令（自动追加 `\r\n`）
- 查询行接收状态，读取一行数据
- 支持运行时更改波特率

### 4.4 模组 HAL 层 (`hal/hal_module.c`)

封装了所有模组相关的 GPIO/中断操作：
- 模组硬件复位（RST 引脚控制）
- 模组通知引脚的中断配置（上升沿触发）
- CW32L011 GPIOB ISR 实现（包含 AT_ISR 和 PTT 唤醒）

---

## 5. AT 命令层使用说明

### 5.1 初始化

```c
// 在应用初始化阶段调用（at.c 内部通过 HAL 完成 UART 初始化）
at_init();
```

### 5.2 发送 AT 命令

```c
// 将命令入队（非阻塞）
at_send(AT_CMD_CALL);       // 发起呼叫
at_send(AT_CMD_HANGUP);     // 挂断
at_send(AT_CMD_BAT);        // 查询电量
at_send(AT_CMD_VOLUME);     // 设置音量（从系统参数读取）
```

### 5.3 设置铃声

```c
// 必须在 AT_CMD_PLAYRING 命令之前设置铃声类型
at_set_tone(TONE_RING_RX1);
at_send(AT_CMD_PLAYRING);
```

### 5.4 获取事件

```c
// 在主循环中轮询 AT 事件
AT_evt_e evt;
while(at_evt_pop(&evt)) {
    switch(evt) {
    case AT_EVT_REFRESH_BAT:
        // 电量已更新到 sysParam->bat
        break;
    case AT_EVT_POWER_OFF_ACT:
        // 模组请求关机
        break;
    case AT_EVT_SLEEP_ACT:
        // 模组进入休眠
        break;
    }
}
```

### 5.5 主循环

```c
// at_task() 必须在主循环中持续调用
while(1) {
    at_task();
}
```

---

## 6. 模组业务层使用说明

### 6.1 初始化序列

```c
buadrate_scan(1);   // 自动检测/对齐模组波特率
at_init();          // 初始化 AT 模块（含 UART）
module_init();      // 初始化模组业务层（含中断通知引脚）
module_synch();     // 同步系统参数到模组
```

### 6.2 主循环处理

```c
// app_module_task() 封装了通知轮询和 AT 任务
void app_module_task(void)
{
    if(module_notify_poll()) {
        at_send(AT_CMD_MODEFLAG);   // 有通知时查询模组状态
    }
    at_task();
}
```

### 6.3 获取模组事件

```c
Module_evt_t evt;
while(app_module_evt_pop(&evt)) {
    switch(evt.evt) {
    case MODULE_EVT_CALL:         break;  // 发起呼叫
    case MODULE_EVT_RING:         break;  // 被叫响铃
    case MODULE_EVT_INCALL_ACT:   break;  // 进入通话
    case MODULE_EVT_EXIT_INCALL:  break;  // 通话结束
    case MODULE_EVT_HANGUP:       break;  // 挂断
    case MODULE_EVT_REFRESH_BAT:  break;  // 电量刷新
    case MODULE_EVT_POWER_OFF:    break;  // 关机请求
    case MODULE_EVT_SLEEP:        break;  // 进入休眠
    }
}
```

---

## 7. 移植指南

移植到新芯片/平台只需两步：

### 7.1 实现 AT HAL 接口（`hal/src/hal-at.c`）

```c
/* 以下函数需替换为目标平台的 UART 实现 */

void hal_at_init(void)
{
    /* 1. 获取目标波特率（可从系统参数或硬编码） */
    /* 2. 初始化 UART TX/RX 引脚 */
    /* 3. 配置 UART 参数（波特率、数据位、停止位、校验位） */
    /* 4. 使能接收中断 */
}

void hal_at_deInit(void)
{
    /* 禁用 UART 中断，关闭时钟，释放引脚 */
}

void hal_at_sendData(const char *format, ...)
{
    /* 格式化字符串后通过 UART 发送（追加 "\r\n"） */
}

void hal_at_clear(void)
{
    /* 清空接收缓冲区 */
}

bool hal_at_is_line_ready(void)
{
    /* 返回是否已收到完整的一行数据（"\r\n" 结尾） */
}

bool hal_at_get_line(char *out_buf, uint16_t buf_size)
{
    /* 将当前行数据复制到 out_buf，并清空内部缓冲区 */
    /* 返回 true 表示成功，false 表示无数据 */
}

void hal_at_reinit_baudrate(uint32_t baudrate)
{
    /* 以新波特率重新配置 UART（用于 AT+BAUDRATE 切换） */
}
```

**接收中断实现要点：**
```c
/* 在 UART 接收中断中，逐字节写入行缓冲区 */
/* 检测到 '\r\n' 时置 line_ready = true */
void YOUR_UART_IRQHandler(void)
{
    uint8_t byte = read_uart_byte();
    /* 写入缓冲区，检测 \r\n 行结束符 */
}
```

### 7.2 实现模组 HAL 接口（`hal/src/hal_module.c`）

```c
/* 以下函数需替换为目标平台的 GPIO/中断实现 */

void hal_module_rst(void)
{
    /* 操作顺序：
     * 1. 设置电源控制引脚为输出并拉高（使能电源）
     * 2. 设置 RST 引脚为输出并拉低（复位有效）
     * 3. 延时约 20ms
     * 4. 将 RST 引脚拉高（释放复位）
     */
}

void hal_module_notify_init(void)
{
    /* 将模组通知引脚配置为上升沿触发输入中断
     * 使能对应的 NVIC 中断通道
     */
}

void hal_module_notify_deInit(void)
{
    /* 禁用中断，释放引脚 */
}

void hal_module_notify_resume(void)
{
    /* 唤醒后将 RST 引脚配置为输出并拉高 */
}

/* 在目标平台的 GPIO 中断 ISR 中调用 hal_module_notify_callback() */
void YOUR_GPIO_IRQHandler(void)
{
    if(/* 模组通知引脚触发 */) {
        /* 清除中断标志 */
        hal_module_notify_callback();   /* 必须调用此函数通知 dev 层 */
    }
}
```

> **注意：** `hal_module_notify_callback()` 已在 `dev/src/module.c` 中实现，
> 移植时无需修改 `module.c`，只需在目标平台的 ISR 中调用它即可。

### 7.3 移植检查清单

- [ ] 实现 `hal/src/hal-at.c` 中的所有 7 个函数
- [ ] 实现 `hal/src/hal_module.c` 中的所有 4 个函数
- [ ] 在目标平台 ISR 中调用 `hal_module_notify_callback()`
- [ ] 确认 `HAL_AT_LINE_BUF_LEN`（`hal/inc/hal-at.h`）不小于模组最长单行响应
- [ ] 根据实际硬件修改 `drv/src/drv_gpio.c` 中的 GPIO 引脚映射表
- [ ] 根据实际硬件修改 `app/app_lowpower.c` 中的 `POWER_KEY_PORT/PIN` 定义

---

## 8. 接口参考

### 8.1 AT HAL 接口（`hal/inc/hal-at.h`）

| 函数 | 说明 |
|------|------|
| `hal_at_init()` | 初始化 AT UART（从系统参数读取波特率） |
| `hal_at_deInit()` | 反初始化 AT UART |
| `hal_at_sendData(fmt, ...)` | 格式化发送 AT 命令（自动追加 `\r\n`） |
| `hal_at_clear()` | 清空接收缓冲区 |
| `hal_at_is_line_ready()` | 查询是否已收到完整行 |
| `hal_at_get_line(buf, size)` | 获取并消费一行接收数据 |
| `hal_at_reinit_baudrate(baud)` | 重新初始化 UART 波特率 |

### 8.2 模组 HAL 接口（`hal/inc/hal_module.h`）

| 函数 | 说明 |
|------|------|
| `hal_module_rst()` | 执行模组硬件复位 |
| `hal_module_notify_init()` | 初始化模组通知中断引脚 |
| `hal_module_notify_deInit()` | 反初始化模组通知中断引脚 |
| `hal_module_notify_resume()` | 唤醒后恢复 RST 引脚状态 |
| `hal_module_notify_callback()` | 中断通知回调（dev 层实现，HAL ISR 调用） |

### 8.3 AT 命令接口（`dev/inc/at.h`）

| 函数 | 说明 |
|------|------|
| `at_init()` | 初始化 AT 模块 |
| `at_deInit()` | 反初始化 AT 模块 |
| `at_send(cmd)` | 将 AT 命令入队 |
| `at_set_tone(tone)` | 设置铃声类型（在 `AT_CMD_PLAYRING` 前调用） |
| `at_task()` | AT 状态机驱动（主循环调用） |
| `at_evt_pop(evt)` | 从事件队列取出一个事件 |
| `at_clear()` | 清空 AT 接收缓冲区 |

### 8.4 模组业务接口（`dev/inc/module.h`）

| 函数 | 说明 |
|------|------|
| `module_init()` | 初始化模组业务层（含 HAL 通知引脚） |
| `module_deInit()` | 反初始化模组业务层 |
| `module_notify_poll()` | 查询并消费一个模组通知 |
| `module_notify_post(param)` | 软件触发一个模组通知（用于定时轮询） |
| `module_synch()` | 同步系统参数到模组 |
| `MODULE_RST()` | 执行模组硬件复位 |
| `module_notify_resume()` | 唤醒后恢复模组引脚状态 |

---

*文档版本：重构后初始版本 v1.0.0 — 2026-03-31*

