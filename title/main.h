#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f10x.h"

#define LCD_CTRL_PORT GPIOA
#define LCD_RS_PIN GPIO_Pin_5
#define LCD_RW_PIN GPIO_Pin_6
#define LCD_EN_PIN GPIO_Pin_7

#define LCD_DATA_PORT GPIOB
#define LCD_D4_PIN GPIO_Pin_0
#define LCD_D5_PIN GPIO_Pin_1
#define LCD_D6_PIN GPIO_Pin_2
#define LCD_D7_PIN GPIO_Pin_3

#define DS18B20_PORT GPIOB
#define DS18B20_PIN GPIO_Pin_4

#define KEY_PORT GPIOA
#define KEY1_PIN GPIO_Pin_0
#define KEY2_PIN GPIO_Pin_1
#define KEY3_PIN GPIO_Pin_2
#define KEY4_PIN GPIO_Pin_3
#define SW_WATER_PIN GPIO_Pin_4

#define RELAY_PORT GPIOA
#define RELAY_HEAT_PIN GPIO_Pin_8
#define RELAY_FILL_PIN GPIO_Pin_9

#define TEMP_HIGH_DEFAULT 60
#define TEMP_LOW_DEFAULT 40
#define TEMP_HIGH_MIN 50
#define TEMP_HIGH_MAX 90
#define TEMP_LOW_MIN 10
#define TEMP_LOW_MAX 50

/* PC14/PC15 are the 32.768 kHz low-speed oscillator pins on STM32F103.
 * This program intentionally uses the internal 8 MHz HSI clock. */
#define MAIN_LOOP_PERIOD_MS          10U
#define LCD_REFRESH_PERIOD_MS       500U
#define DS18B20_CONVERSION_TIME_MS  750U
#define DS18B20_RETRY_PERIOD_MS    1000U
#define SENSOR_ERROR_TEMPERATURE  (-999.0f)

#define DS18B20_STATUS_OK           0U
#define DS18B20_STATUS_NO_PRESENCE  1U
#define DS18B20_STATUS_BAD_DATA     2U

#define LCD_RS_HIGH() GPIO_SetBits(LCD_CTRL_PORT, LCD_RS_PIN)
#define LCD_RS_LOW() GPIO_ResetBits(LCD_CTRL_PORT, LCD_RS_PIN)
#define LCD_RW_HIGH() GPIO_SetBits(LCD_CTRL_PORT, LCD_RW_PIN)
#define LCD_RW_LOW() GPIO_ResetBits(LCD_CTRL_PORT, LCD_RW_PIN)
#define LCD_EN_HIGH() GPIO_SetBits(LCD_CTRL_PORT, LCD_EN_PIN)
#define LCD_EN_LOW() GPIO_ResetBits(LCD_CTRL_PORT, LCD_EN_PIN)

#define RELAY_HEAT_ON() GPIO_SetBits(RELAY_PORT, RELAY_HEAT_PIN)
#define RELAY_HEAT_OFF() GPIO_ResetBits(RELAY_PORT, RELAY_HEAT_PIN)
#define RELAY_FILL_ON() GPIO_SetBits(RELAY_PORT, RELAY_FILL_PIN)
#define RELAY_FILL_OFF() GPIO_ResetBits(RELAY_PORT, RELAY_FILL_PIN)

#define KEY1_PRESSED() (GPIO_ReadInputDataBit(KEY_PORT, KEY1_PIN) == RESET)
#define KEY2_PRESSED() (GPIO_ReadInputDataBit(KEY_PORT, KEY2_PIN) == RESET)
#define KEY3_PRESSED() (GPIO_ReadInputDataBit(KEY_PORT, KEY3_PIN) == RESET)
#define KEY4_PRESSED() (GPIO_ReadInputDataBit(KEY_PORT, KEY4_PIN) == RESET)
#define SW_WATER_LACK() (GPIO_ReadInputDataBit(KEY_PORT, SW_WATER_PIN) == RESET)

extern float g_temperature;
extern int g_temp_high;
extern int g_temp_low;
extern uint8_t g_heating;
extern uint8_t g_filling;
extern uint8_t g_water_lack;
extern uint8_t g_too_hot;
extern uint8_t g_sensor_error;
extern uint8_t g_ds18b20_status;

void SystemClock_Config(void);
void GPIO_Init_All(void);
void DelayTimer_Init(void);
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);

#endif
