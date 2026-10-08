#ifndef __LCD1602_H
#define __LCD1602_H

#include "main.h"

#define LCD_CMD_CLEAR 0x01
#define LCD_CMD_ENTRY 0x06
#define LCD_CMD_DISPLAY_ON 0x0C
#define LCD_CMD_FUNC_SET 0x28

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_WriteChar(char c);
void LCD_WriteString(const char *str);
void LCD_WriteCmd(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_DisplayStatus(float temp, int high, int low,
                       uint8_t heating, uint8_t filling,
                       uint8_t water_lack, uint8_t too_hot);

#endif
