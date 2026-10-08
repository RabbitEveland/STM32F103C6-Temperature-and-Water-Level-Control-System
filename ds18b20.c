#include "ds18b20.h"

/* PB4 is in GPIOB->CRL (four configuration bits beginning at bit 16).
 * DQ is driven low only; it is then switched to an input with pull-up so the
 * external 4.7 kOhm resistor and the MCU never contend with the DS18B20. */
static void DS18B20_DQ_DriveLow(void)
{
    DS18B20_PORT->CRL = (DS18B20_PORT->CRL & ~(0x0FU << 16)) | (0x02U << 16);
    GPIO_ResetBits(DS18B20_PORT, DS18B20_PIN);
}

static void DS18B20_DQ_Release(void)
{
    GPIO_SetBits(DS18B20_PORT, DS18B20_PIN);
    DS18B20_PORT->CRL = (DS18B20_PORT->CRL & ~(0x0FU << 16)) | (0x08U << 16);
}

static void DS18B20_WriteBit(uint8_t bit)
{
    DS18B20_DQ_DriveLow();
    if (bit)
    {
        Delay_us(6);
        DS18B20_DQ_Release();
        Delay_us(64);
    }
    else
    {
        Delay_us(60);
        DS18B20_DQ_Release();
        Delay_us(10);
    }
}

static uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit;

    DS18B20_DQ_DriveLow();
    Delay_us(6);
    DS18B20_DQ_Release();
    Delay_us(12);
    bit = DS18B20_DQ_READ();
    Delay_us(55);
    return bit;
}

static void DS18B20_WriteByte(uint8_t value)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        DS18B20_WriteBit(value & 0x01);
        value >>= 1;
    }
}

static uint8_t DS18B20_ReadByte(void)
{
    uint8_t i;
    uint8_t value = 0;

    for (i = 0; i < 8; i++)
    {
        value >>= 1;
        if (DS18B20_ReadBit()) value |= 0x80;
    }
    return value;
}

uint8_t DS18B20_Reset(void)
{
    uint8_t presence;
    uint8_t i;

    DS18B20_DQ_DriveLow();
    Delay_us(480);
    DS18B20_DQ_Release();
    Delay_us(15);

    /* Sample the whole presence-pulse window.  This is more tolerant of the
     * event scheduling used by the Proteus DS18B20 model. */
    presence = 0;
    for (i = 0; i < 24U; i++)
    {
        if (DS18B20_DQ_READ() == RESET) presence = 1;
        Delay_us(10);
    }

    Delay_us(240);
    return presence;
}

uint8_t DS18B20_StartConversion(void)
{
    if (!DS18B20_Reset())
    {
        g_ds18b20_status = DS18B20_STATUS_NO_PRESENCE;
        return 0;
    }

    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_CONVERT_T);
    g_ds18b20_status = DS18B20_STATUS_OK;
    return 1;
}

float DS18B20_ReadTemperature(void)
{
    uint8_t low;
    uint8_t high;
    int16_t raw;

    if (!DS18B20_Reset())
    {
        g_ds18b20_status = DS18B20_STATUS_NO_PRESENCE;
        return SENSOR_ERROR_TEMPERATURE;
    }
    DS18B20_WriteByte(DS18B20_CMD_SKIP_ROM);
    DS18B20_WriteByte(DS18B20_CMD_READ_SCRATCH);
    low = DS18B20_ReadByte();
    high = DS18B20_ReadByte();

    raw = (int16_t)(((uint16_t)high << 8) | low);
    if (raw < -880 || raw > 2000)
    {
        g_ds18b20_status = DS18B20_STATUS_BAD_DATA;
        return SENSOR_ERROR_TEMPERATURE;
    }

    g_ds18b20_status = DS18B20_STATUS_OK;
    return (float)raw * 0.0625f;
}
