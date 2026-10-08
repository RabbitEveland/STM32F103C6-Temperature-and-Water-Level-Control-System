# STM32F103C6 温度与水位控制系统

基于 STM32F103C6、LCD1602 和 DS18B20 的 Keil MDK 工程，用于 Proteus 仿真或移植至实物板。程序读取温度和缺水开关状态，并控制加热、加水继电器。

## 功能

- LCD1602 4 位模式显示当前温度、上下限和传感器状态；
- DS18B20 温度采集；
- 四个按键调节温度上、下限；
- 缺水时启动加水继电器并关闭加热；
- 温度低于下限时加热，高于上限时停止加热；
- DS18B20 使用 TIM2 的 1 MHz 硬件定时，保证 1-Wire 微秒级时序。

## 引脚连接

| 外设 | STM32F103C6 引脚 |
| --- | --- |
| LCD RS / RW / EN | PA5 / PA6 / PA7 |
| LCD D4 / D5 / D6 / D7 | PB0 / PB1 / PB2 / PB3 |
| DS18B20 DQ | PB4（4.7 kΩ 上拉至 3.3 V） |
| KEY1 / KEY2 / KEY3 / KEY4 | PA0 / PA1 / PA2 / PA3（另一端接 GND，内部上拉） |
| 缺水开关 | PA4（闭合为低电平，内部上拉） |
| 加热继电器 | PA8，经 1 kΩ 电阻驱动 2N2222 |
| 加水继电器 | PA9，经 1 kΩ 电阻驱动 2N2222 |

LCD 的 VSS 接 GND，VDD 接 5 V，VO 接 10 kΩ 电位器中间端。DS18B20 的 VDD 接 3.3 V、GND 接 GND。

> 继电器线圈需并联反向续流二极管；晶体管不能直接驱动线圈。

## 构建与运行

1. 用 Keil MDK 打开 `666.uvprojx`，选择 Target 1 后 Build。
2. 输出固件为 `Objects/666.hex`，将其指定给 Proteus 中 STM32F103C6 的 **Program File**。
3. Proteus 器件的 **Crystal Frequency** 设置为 `8MHz`。
4. 启动仿真，温度会在首次转换后显示。

工程内部使用 STM32 的 8 MHz HSI；PC14/PC15 是低速 32.768 kHz 振荡器引脚，不应接 8 MHz 主晶振。

## 工程结构

```text
main.c                 主程序、控制逻辑、TIM2 微秒延时
ds18b20.c              DS18B20 1-Wire 驱动
lcd1602.c              LCD1602 4 位模式驱动
title/                 头文件
RTE/                   Keil Run-Time Environment 配置
666.uvprojx            Keil 工程
Objects/666.hex        已生成的 Proteus 加载文件
```

## 说明

为避免 LCD 控制器繁忙时写入导致乱码，LCD 驱动会读取忙标志后再写入。DS18B20 在复位后会检测完整的 presence 脉冲窗口；若 LCD 显示 `DS NO PRESENCE`，优先检查 DQ 是否接在 PB4、4.7 kΩ 是否上拉到 3.3 V，以及 DS18B20 的 VDD/GND 是否正确。
