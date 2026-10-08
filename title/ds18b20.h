#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"

#define DS18B20_CMD_SKIP_ROM 0xCC
#define DS18B20_CMD_CONVERT_T 0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE

#define DS18B20_DQ_LOW() GPIO_ResetBits(DS18B20_PORT, DS18B20_PIN)
#define DS18B20_DQ_RELEASE() GPIO_SetBits(DS18B20_PORT, DS18B20_PIN)
#define DS18B20_DQ_READ() GPIO_ReadInputDataBit(DS18B20_PORT, DS18B20_PIN)

uint8_t DS18B20_Reset(void);
uint8_t DS18B20_StartConversion(void);
float DS18B20_ReadTemperature(void);

#endif
