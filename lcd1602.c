#include "lcd1602.h"

static uint8_t LCD_FormatTemperature(float temp, char *text)
{
    int32_t tenths;
    uint32_t magnitude;
    uint32_t whole;
    uint8_t length = 0;

    tenths = (int32_t)((temp >= 0.0f) ? (temp * 10.0f + 0.5f)
                                        : (temp * 10.0f - 0.5f));
    if (tenths < 0)
    {
        text[length++] = '-';
        magnitude = (uint32_t)(-tenths);
    }
    else
    {
        magnitude = (uint32_t)tenths;
    }

    whole = magnitude / 10U;
    if (whole >= 100U)
    {
        text[length++] = (char)('0' + (whole / 100U));
        text[length++] = (char)('0' + ((whole / 10U) % 10U));
    }
    else if (whole >= 10U)
    {
        text[length++] = (char)('0' + (whole / 10U));
    }
    text[length++] = (char)('0' + (whole % 10U));
    text[length++] = '.';
    text[length++] = (char)('0' + (magnitude % 10U));
    return length;
}

static uint8_t LCD_DisplayNeedsRefresh(float temp, int high, int low,
                                       uint8_t heating, uint8_t filling,
                                       uint8_t water_lack, uint8_t too_hot)
{
    static uint8_t initialized = 0;
    static int16_t last_temp_tenths;
    static int last_high;
    static int last_low;
    static uint8_t last_heating;
    static uint8_t last_filling;
    static uint8_t last_water_lack;
    static uint8_t last_too_hot;
    static uint8_t last_ds18b20_status;
    int16_t temp_tenths;

    if (temp < -100.0f || temp > 125.0f)
    {
        temp_tenths = (int16_t)-32768;
    }
    else
    {
        temp_tenths = (int16_t)((temp >= 0.0f) ? (temp * 10.0f + 0.5f)
                                                   : (temp * 10.0f - 0.5f));
    }

    if (initialized && temp_tenths == last_temp_tenths && high == last_high &&
        low == last_low && heating == last_heating && filling == last_filling &&
        water_lack == last_water_lack && too_hot == last_too_hot &&
        g_ds18b20_status == last_ds18b20_status)
    {
        return 0;
    }

    initialized = 1;
    last_temp_tenths = temp_tenths;
    last_high = high;
    last_low = low;
    last_heating = heating;
    last_filling = filling;
    last_water_lack = water_lack;
    last_too_hot = too_hot;
    last_ds18b20_status = g_ds18b20_status;
    return 1;
}

static void LCD_DataBusInput(void)
{
    /* PB0..PB3: floating inputs; PB4 and above are left untouched. */
    LCD_DATA_PORT->CRL = (LCD_DATA_PORT->CRL & ~0x0000FFFFU) | 0x00004444U;
}

static void LCD_DataBusOutput(void)
{
    /* PB0..PB3: 2 MHz push-pull outputs. */
    LCD_DATA_PORT->CRL = (LCD_DATA_PORT->CRL & ~0x0000FFFFU) | 0x00002222U;
}

static void LCD_WaitReady(void)
{
    uint16_t timeout = 500U;
    uint8_t busy;

    LCD_RS_LOW();
    LCD_RW_HIGH();
    LCD_DataBusInput();

    do
    {
        LCD_EN_HIGH();
        Delay_us(20);
        busy = GPIO_ReadInputDataBit(LCD_DATA_PORT, LCD_D7_PIN);
        LCD_EN_LOW();
        Delay_us(20);

        /* Complete the low nibble read; its value is the address counter. */
        LCD_EN_HIGH();
        Delay_us(20);
        LCD_EN_LOW();
        Delay_us(20);
    } while (busy && --timeout);

    LCD_RW_LOW();
    LCD_DataBusOutput();
    Delay_us(20);
}

static void LCD_WriteNibble(uint8_t value)
{
    GPIO_WriteBit(LCD_DATA_PORT, LCD_D4_PIN, (value & 0x01) ? Bit_SET : Bit_RESET);
    GPIO_WriteBit(LCD_DATA_PORT, LCD_D5_PIN, (value & 0x02) ? Bit_SET : Bit_RESET);
    GPIO_WriteBit(LCD_DATA_PORT, LCD_D6_PIN, (value & 0x04) ? Bit_SET : Bit_RESET);
    GPIO_WriteBit(LCD_DATA_PORT, LCD_D7_PIN, (value & 0x08) ? Bit_SET : Bit_RESET);
    LCD_EN_HIGH();
    Delay_us(20);
    LCD_EN_LOW();
    Delay_us(200);
}

void LCD_WriteCmd(uint8_t command)
{
    LCD_WaitReady();
    LCD_RS_LOW();
    LCD_RW_LOW();
    LCD_WriteNibble(command >> 4);
    LCD_WriteNibble(command & 0x0F);
    Delay_us(50);
}

void LCD_WriteData(uint8_t data)
{
    LCD_WaitReady();
    LCD_RS_HIGH();
    LCD_RW_LOW();
    LCD_WriteNibble(data >> 4);
    LCD_WriteNibble(data & 0x0F);
    Delay_us(50);
}

void LCD_Init(void)
{
    Delay_ms(300);
    LCD_RS_LOW();
    LCD_RW_LOW();
    LCD_EN_LOW();
    LCD_WriteNibble(0x03);
    Delay_ms(50);
    LCD_WriteNibble(0x03);
    Delay_ms(20);
    LCD_WriteNibble(0x03);
    Delay_ms(20);
    LCD_WriteNibble(0x02);
    Delay_ms(20);
    LCD_WriteCmd(LCD_CMD_FUNC_SET);
    LCD_WriteCmd(0x08);
    LCD_WriteCmd(LCD_CMD_CLEAR);
    LCD_WriteCmd(LCD_CMD_ENTRY);
    LCD_WriteCmd(LCD_CMD_DISPLAY_ON);
}

void LCD_Clear(void)
{
    LCD_WriteCmd(LCD_CMD_CLEAR);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    LCD_WriteCmd((row ? 0xC0 : 0x80) + col);
}

void LCD_WriteChar(char c)
{
    LCD_WriteData((uint8_t)c);
}

void LCD_WriteString(const char *str)
{
    while (*str) LCD_WriteChar(*str++);
}

void LCD_DisplayStatus(float temp, int high, int low,
                       uint8_t heating, uint8_t filling,
                       uint8_t water_lack, uint8_t too_hot)
{
    char line1[17];
    const char *line2;
    uint8_t position;
    uint8_t temp_length;
    uint8_t i;
    char temp_text[7];

    if (!LCD_DisplayNeedsRefresh(temp, high, low, heating, filling,
                                 water_lack, too_hot))
    {
        return;
    }

    if (temp < -100.0f || temp > 125.0f)
    {
        line1[0] = 'T'; line1[1] = ':'; line1[2] = 'E';
        line1[3] = 'r'; line1[4] = 'r'; line1[5] = ' ';
        if (g_ds18b20_status == DS18B20_STATUS_NO_PRESENCE)
        {
            line2 = "DS NO PRESENCE  ";
        }
        else if (g_ds18b20_status == DS18B20_STATUS_BAD_DATA)
        {
            line2 = "DS DATA INVALID ";
        }
        else
        {
            line2 = "DS18B20 ERR     ";
        }
    }
    else
    {
        temp_length = LCD_FormatTemperature(temp, temp_text);
        position = 0;
        line1[position++] = 'T';
        line1[position++] = ':';
        for (i = 0; i < temp_length; i++) line1[position++] = temp_text[i];
        if (temp_length <= 4U) line1[position++] = ' ';
        line1[position++] = 'H';
        line1[position++] = ':';
        line1[position++] = (char)('0' + high / 10);
        line1[position++] = (char)('0' + high % 10);
        line1[position++] = ' ';
        line1[position++] = 'L';
        line1[position++] = ':';
        line1[position++] = (char)('0' + low / 10);
        line1[position++] = (char)('0' + low % 10);
        while (position < 16U) line1[position++] = ' ';
        line1[16] = '\0';

        if (water_lack && filling) line2 = "Filling Water...";
        else if (water_lack) line2 = "Water Lack!     ";
        else if (too_hot) line2 = "Water too hot!  ";
        else if (heating) line2 = "Heating...      ";
        else line2 = "Normal  READY   ";

        LCD_SetCursor(0, 0);
        LCD_WriteString(line1);
        LCD_SetCursor(1, 0);
        LCD_WriteString(line2);
        return;
    }

    line1[6] = ' '; line1[7] = 'H'; line1[8] = ':';
    line1[9] = '0' + high / 10; line1[10] = '0' + high % 10;
    line1[11] = ' '; line1[12] = 'L'; line1[13] = ':';
    line1[14] = '0' + low / 10; line1[15] = '0' + low % 10;
    line1[16] = '\0';

    LCD_SetCursor(0, 0);
    LCD_WriteString(line1);
    LCD_SetCursor(1, 0);
    LCD_WriteString(line2);
}
