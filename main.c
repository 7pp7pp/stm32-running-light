#include "stm32f10x.h"

#define LED_COUNT           4U
#define STEP_INTERVAL_MS    180U
#define DEBOUNCE_MS         20U

#define LED_PINS            (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3)
#define KEY_PIN             GPIO_Pin_0
#define KEY_PORT            GPIOB
#define PC13_LED_PIN        GPIO_Pin_13
#define PC13_LED_PORT       GPIOC

static const uint16_t kLedPins[LED_COUNT] = {
    GPIO_Pin_0,
    GPIO_Pin_1,
    GPIO_Pin_2,
    GPIO_Pin_3
};

volatile uint32_t g_tick_ms = 0;

static uint8_t  s_ledIndex;
static int8_t   s_direction;
static uint8_t  s_keyRaw;
static uint8_t  s_keyStable;
static uint32_t s_keyLastChange;

static void GPIO_Init_All(void);
static void App_Init(void);
static uint8_t Key_ReadRaw(void);
static void Led_Write(uint8_t index, BitAction state);
static void Led_AllOff(void);
static void RunningLight_Advance(void);
static void RunningLight_ToggleDirection(void);

int main(void)
{
    uint32_t lastStep;
    uint8_t raw;
    uint32_t now;

    GPIO_Init_All();
    App_Init();

    if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
    {
        while (1)
        {
        }
    }

    lastStep = g_tick_ms;

    while (1)
    {
        raw = Key_ReadRaw();
        now = g_tick_ms;

        if (raw != s_keyRaw)
        {
            s_keyRaw = raw;
            s_keyLastChange = now;
        }

        if (((now - s_keyLastChange) >= DEBOUNCE_MS) && (raw != s_keyStable))
        {
            s_keyStable = raw;
            if (s_keyStable != 0U)
            {
                RunningLight_ToggleDirection();
            }
        }

        if ((now - lastStep) >= STEP_INTERVAL_MS)
        {
            lastStep = now;
            RunningLight_Advance();
        }
    }
}

static void GPIO_Init_All(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_ResetBits(GPIOA, LED_PINS);

    GPIO_InitStructure.GPIO_Pin   = LED_PINS;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin  = KEY_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);

    GPIO_SetBits(PC13_LED_PORT, PC13_LED_PIN);
    GPIO_InitStructure.GPIO_Pin   = PC13_LED_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(PC13_LED_PORT, &GPIO_InitStructure);
}

static void App_Init(void)
{
    s_ledIndex = 0U;
    s_direction = 1;

    Led_AllOff();
    Led_Write(s_ledIndex, Bit_SET);

    s_keyRaw = Key_ReadRaw();
    s_keyStable = s_keyRaw;
    s_keyLastChange = g_tick_ms;
}

static void Led_Write(uint8_t index, BitAction state)
{
    if (index < LED_COUNT)
    {
        GPIO_WriteBit(GPIOA, kLedPins[index], state);
    }
}

static void Led_AllOff(void)
{
    GPIO_ResetBits(GPIOA, LED_PINS);
}

static void RunningLight_Advance(void)
{
    Led_Write(s_ledIndex, Bit_RESET);

    if (s_direction > 0)
    {
        s_ledIndex = (uint8_t)((s_ledIndex + 1U) % LED_COUNT);
    }
    else
    {
        s_ledIndex = (uint8_t)((s_ledIndex + LED_COUNT - 1U) % LED_COUNT);
    }

    Led_Write(s_ledIndex, Bit_SET);
}

static void RunningLight_ToggleDirection(void)
{
    s_direction = (s_direction > 0) ? -1 : 1;
}

static uint8_t Key_ReadRaw(void)
{
    return (GPIO_ReadInputDataBit(KEY_PORT, KEY_PIN) == Bit_RESET) ? 1U : 0U;
}

void SysTick_Handler(void)
{
    static uint16_t heartbeat = 0;

    g_tick_ms++;

    heartbeat++;
    if (heartbeat >= 500U)
    {
        heartbeat = 0;
        GPIOC->ODR ^= PC13_LED_PIN;
    }
}
