#include "main.h"
#include "lcd1602.h"
#include "ds18b20.h"

float g_temperature = SENSOR_ERROR_TEMPERATURE;
int g_temp_high = TEMP_HIGH_DEFAULT;
int g_temp_low = TEMP_LOW_DEFAULT;
uint8_t g_heating = 0;
uint8_t g_filling = 0;
uint8_t g_water_lack = 0;
uint8_t g_too_hot = 0;
uint8_t g_sensor_error = 1;
uint8_t g_ds18b20_status = DS18B20_STATUS_NO_PRESENCE;

void Delay_us(uint32_t us)
{
    uint16_t slice;

    /* TIM2 is clocked from the 8 MHz APB1 clock and prescaled to 1 MHz.
     * This is deliberately independent from SysTick: Proteus does not always
     * model SysTick at the same rate as the MCU's configured HSI clock. */
    while (us != 0U)
    {
        slice = (us > 60000U) ? 60000U : (uint16_t)us;
        TIM_SetCounter(TIM2, 0U);
        while (TIM_GetCounter(TIM2) < slice)
        {
        }
        us -= slice;
    }
}

void DelayTimer_Init(void)
{
    TIM_TimeBaseInitTypeDef timer;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 7U;       /* 8 MHz / (7 + 1) = 1 MHz */
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    timer.TIM_Period = 0xFFFFU;
    timer.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_SetCounter(TIM2, 0U);
    TIM_Cmd(TIM2, ENABLE);
}

void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

void SystemClock_Config(void)
{
    RCC_DeInit();
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET) {}
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    while (RCC_GetSYSCLKSource() != 0x00) {}

    /* Proteus can retain the RTE default (72 MHz) in SystemCoreClock even
     * after the simulated RCC has switched to HSI.  All 1-Wire timings depend
     * on this value, so make the 8 MHz HSI setting explicit. */
    SystemCoreClock = 8000000U;
}

void GPIO_Init_All(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    gpio.GPIO_Pin = LCD_RS_PIN | LCD_RW_PIN | LCD_EN_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LCD_CTRL_PORT, &gpio);

    gpio.GPIO_Pin = LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN;
    GPIO_Init(LCD_DATA_PORT, &gpio);

    gpio.GPIO_Pin = DS18B20_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(DS18B20_PORT, &gpio);
    DS18B20_DQ_RELEASE();

    gpio.GPIO_Pin = KEY1_PIN | KEY2_PIN | KEY3_PIN | KEY4_PIN | SW_WATER_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KEY_PORT, &gpio);
    GPIO_SetBits(KEY_PORT, gpio.GPIO_Pin);

    gpio.GPIO_Pin = RELAY_HEAT_PIN | RELAY_FILL_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(RELAY_PORT, &gpio);
    RELAY_HEAT_OFF();
    RELAY_FILL_OFF();
}

static void KEY_Scan(void)
{
    static uint8_t previous_sample = 0;
    static uint8_t stable_state = 0;
    static uint8_t stable_count = 0;
    uint8_t current = 0;
    uint8_t pressed;

    if (KEY1_PRESSED()) current |= 0x01;
    if (KEY2_PRESSED()) current |= 0x02;
    if (KEY3_PRESSED()) current |= 0x04;
    if (KEY4_PRESSED()) current |= 0x08;

    if (current != previous_sample)
    {
        previous_sample = current;
        stable_count = 0;
        return;
    }

    if (stable_count < 3U)
    {
        stable_count++;
        return;
    }

    pressed = current & (uint8_t)~stable_state;
    stable_state = current;

    if (pressed & 0x01)
    {
        if (++g_temp_high > TEMP_HIGH_MAX) g_temp_high = TEMP_HIGH_MAX;
    }
    if (pressed & 0x02)
    {
        if (--g_temp_high < TEMP_HIGH_MIN) g_temp_high = TEMP_HIGH_MIN;
        if (g_temp_high <= g_temp_low) g_temp_high = g_temp_low + 1;
    }
    if (pressed & 0x04)
    {
        if (++g_temp_low > TEMP_LOW_MAX) g_temp_low = TEMP_LOW_MAX;
        if (g_temp_low >= g_temp_high) g_temp_low = g_temp_high - 1;
    }
    if (pressed & 0x08)
    {
        if (--g_temp_low < TEMP_LOW_MIN) g_temp_low = TEMP_LOW_MIN;
    }
}

static void Sensor_Update(void)
{
    static uint16_t elapsed_ms = DS18B20_RETRY_PERIOD_MS;
    static uint8_t converting = 0;
    float temperature;

    if (converting)
    {
        elapsed_ms += MAIN_LOOP_PERIOD_MS;
        if (elapsed_ms < DS18B20_CONVERSION_TIME_MS) return;

        temperature = DS18B20_ReadTemperature();
        if (temperature > -100.0f && temperature < 126.0f)
        {
            g_temperature = temperature;
            g_sensor_error = 0;
        }
        else
        {
            g_sensor_error = 1;
        }

        elapsed_ms = 0;
        converting = DS18B20_StartConversion();
        if (!converting) g_sensor_error = 1;
        return;
    }

    elapsed_ms += MAIN_LOOP_PERIOD_MS;
    if (elapsed_ms < DS18B20_RETRY_PERIOD_MS) return;

    elapsed_ms = 0;
    converting = DS18B20_StartConversion();
    if (!converting) g_sensor_error = 1;
}

static void Control_Logic(void)
{
    g_water_lack = SW_WATER_LACK() ? 1 : 0;

    if (g_water_lack)
    {
        g_filling = 1;
        RELAY_FILL_ON();
    }
    else
    {
        g_filling = 0;
        RELAY_FILL_OFF();
    }

    if (g_sensor_error || g_water_lack)
    {
        g_heating = 0;
        g_too_hot = 0;
        RELAY_HEAT_OFF();
        return;
    }

    if (g_temperature > (float)g_temp_high)
    {
        g_too_hot = 1;
        g_heating = 0;
        RELAY_HEAT_OFF();
    }
    else if (g_temperature < (float)g_temp_low)
    {
        g_too_hot = 0;
        g_heating = 1;
        RELAY_HEAT_ON();
    }
    else
    {
        g_too_hot = 0;
        g_heating = 0;
        RELAY_HEAT_OFF();
    }
}

int main(void)
{
    uint16_t lcd_elapsed_ms = LCD_REFRESH_PERIOD_MS;

    SystemClock_Config();
    DelayTimer_Init();
    GPIO_Init_All();
    LCD_Init();
    LCD_DisplayStatus(g_temperature, g_temp_high, g_temp_low,
                      0, 0, 0, 0);

    while (1)
    {
        Sensor_Update();
        KEY_Scan();
        Control_Logic();

        lcd_elapsed_ms += MAIN_LOOP_PERIOD_MS;
        if (lcd_elapsed_ms >= LCD_REFRESH_PERIOD_MS)
        {
            lcd_elapsed_ms = 0;
            LCD_DisplayStatus(g_temperature, g_temp_high, g_temp_low,
                              g_heating, g_filling, g_water_lack, g_too_hot);
        }

        Delay_ms(MAIN_LOOP_PERIOD_MS);
    }
}
