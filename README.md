# Can_STM32_Sensor —— OBD-II 模拟器（坡道定点停车与起步科目）

本工程基于 **STM32F103 + HAL + FreeRTOS**，实现一个 **CAN 总线 OBD-II 模拟器**，为 `i.MX6ULL` 驾驶培训评分系统提供车辆状态信号。通过在 CAN 总线上周期广播车身状态帧、响应标准诊断请求，i.MX6ULL 端可用 SocketCAN 解析并据此完成"坡道定点停车与起步"科目的实时评分。

同时利用板载 **旋转编码器 + OLED + 按键** 实现了 HMI 菜单交互，使模拟器支持自动剧本演示与人工实时干预（自动/手动切换、参数设定），方便课设现场调试与答辩演示。

---

## 一、功能特性

- **标准 OBD-II 诊断响应（SAE J1979）**
  - Mode 01 当前数据：PID `0x0C` 转速、`0x0D` 车速、`0x05` 冷却液温度、`0x11` 节气门开度；不支持 PID 返回标准 NRC `0x12`
  - Mode 03 读取故障码 DTC（示例 P0100）
  - 支持功能寻址 `0x7DF` 请求，响应地址 `0x7E8`
- **自定义车身状态广播**
  - `0x360`：档位 / 刹车 / 手刹
  - `0x361`：安全带 / 车门 / 左转向灯
- **坡道定点停车与起步自动剧本**（7 阶段状态机）
  - 待机 → 点火 → 上坡加速 → 定点刹车 → 停车 → 坡道起步 → 完成回环
- **HMI 人机交互**
  - OLED 菜单 + 旋转编码器滚动/调节 + 按键确认/回车
  - 自动（AUTO）/手动（MANU）双模式
  - 可调节车速、节气门、档位，开关手刹/刹车/转向灯/安全带/故障码
  - 快捷键：开始/暂停、复位场景

---

## 二、系统架构

沿用 **三层分层** 设计，规则自下而上依赖：

```text
App 层（业务逻辑）
├── app_obd2.c   OBD-II 模拟器（状态机 + 诊断应答 + 车身广播）
├── app_hmi.c    HMI 菜单任务（OLED + 编码器 + 按键）
├── app_uart.c   UART 调试输出
└── app_main.c   任务装配入口
        │ 调用
Service 层（RTOS 同步封装）
├── svc_interface.h/.c  全局接口注册表 svc_interface
├── svc_can.c   CAN 收发封装（消息队列 + 互斥锁）
├── svc_oled.c  OLED 显存管理（互斥锁保护）
├── svc_key.c   按键事件服务
├── svc_encoder.c 旋转编码器计数
└── svc_uart.c / svc_i2c.c
        │ 调用
Bsp 层（硬件驱动）
├── bsp_can.c  bsp_oled.c  bsp_key.c
├── bsp_encoder.c  bsp_uart.c  bsp_i2c.c
```

**任务划分（FreeRTOS）**

| 任务 | 周期 | 职责 |
|------|------|------|
| `HmiTask` | 50ms | 扫描编码器/按键，刷新 OLED 菜单，控制模拟器状态 |
| `Obd2Task` | 100ms | 推进车辆模拟、应答诊断请求、广播车身帧 |
| `UartTask` | 事件 | 处理 UART 指令 |

---

## 三、CAN 协议定义

### 诊断地址（标准 OBD-II）

| 方向 | CAN ID | 说明 |
|------|--------|------|
| i.MX6ULL → MCU | `0x7DF` | 诊断请求（功能寻址） |
| MCU → i.MX6ULL | `0x7E8` | 诊断响应 |

### 自定义车身状态帧（周期广播，100ms）

| CAN ID | 字节 | 含义 |
|--------|------|------|
| `0x360` | D0 D1 D2 | 档位 / 刹车 / 手刹 |
| `0x361` | D0 D1 D2 | 安全带 / 车门 / 左转向灯 |

### 档位编码

```text
0 = P(驻车)   1 = R(倒挡)   2 = N(空挡)   3 = 1挡(坡道起步挡)   4 = D(前进)
```

### 已支持的诊断 PID

| Service | PID | 含义 | 字节 |
|---------|-----|------|------|
| 01 | 0C | 发动机转速 | 2 |
| 01 | 0D | 车速 | 1 |
| 01 | 05 | 冷却液温度 | 1 |
| 01 | 11 | 节气门开度 | 1 |
| 03 | — | 故障码 DTC | 2 |

---

## 四、HMI 菜单交互

### 硬件接线

| 外设 | 引脚 |
|------|------|
| 旋转编码器 A/B | PA6 / PA7（TIM3 正交计数） |
| 编码器按键 (SW) | PB0 |
| 开始 / 暂停键 | PB1 |
| 复位键 | PB10 |
| OLED | I2C（SSD1306 128×64） |

### 操作说明

| 操作 | 作用 |
|------|------|
| 转动编码器 | 上下滚动选择菜单项 / 调节数值 |
| SW 短按 (PB0) | 确认 / 切换 / 进入调节态 |
| SW 长按 (PB0) | 返回菜单顶部 |
| PB1 开始键 | 运行 ↔ 暂停（全局快捷键） |
| PB10 复位键 | 复位场景到待机（全局快捷键） |

### 菜单项

```text
MODE   自动(AUTO)/手动(MANU) 切换
PAUSE  运行/暂停
RESET  复位场景
SPEED  手动车速 0~80 km/h
THROT  手动节气门 0~100%
GEAR   档位循环 P→R→N→1→D
HBRAK  手刹 开/关
BRAKE  刹车 开/关
TURN   左转向灯 开/关
BELT   安全带 开/关
DTC    故障码 开/关
```

> 手动模式下脚本不推进，转速随节气门联动，其余参数由菜单实时设定，便于演示不同操作对 CAN 输出的影响。

---

## 五、工程说明

- 开发环境：**Keil MDK（ARMCC V5）**
- 工程文件：`MDK-ARM/Can_STM32_Sensor.uvprojx`
- 硬件：STM32F103 + TJA1050 CAN 收发器（已配置 `CAN_MODE_NORMAL`）+ SSD1306 OLED + 旋转编码器
- 中间件：FreeRTOS（CMSIS-RTOS v2）

### 编译与烧录

1. 使用 Keil 打开 `MDK-ARM/Can_STM32_Sensor.uvprojx`
2. 选择目标 `Can_STM32_Sensor`，`Rebuild`
3. 连接 ST-Link 烧录，打开串口查看启动日志

---

## 参考与学习

- [开源手表项目（分层架构参考）](https://oshwhub.com/no_chicken/zhi-neng-shou-biao-OV-Watch_V2.2)
- [韦东山教程 FreeRTOS 下 UART 的封装](https://www.bilibili.com/video/BV1jx4y1Y7N8/?spm_id_from=333.337.search-card.all.click)