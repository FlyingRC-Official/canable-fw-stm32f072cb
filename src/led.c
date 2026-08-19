#include "apm32f0xx_gpio.h"
#include "apm32f0xx_rcm.h"
#include "led.h"
#include "system.h"

#define LED_BLUE_PIN GPIO_PIN_0
#define LED_GREEN_PIN GPIO_PIN_1
#define LED_RED_PIN GPIO_PIN_2

static uint32_t blue_last_on;
static uint32_t green_last_on;
static uint32_t blue_last_off;
static uint32_t green_last_off;

void led_init(void)
{
    GPIO_Config_T gpio;
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_GPIOA);
    GPIO_ConfigStructInit(&gpio);
    gpio.pin = LED_BLUE_PIN | LED_GREEN_PIN | LED_RED_PIN;
    gpio.mode = GPIO_MODE_OUT;
    gpio.outtype = GPIO_OUT_TYPE_PP;
    gpio.speed = GPIO_SPEED_10MHz;
    gpio.pupd = GPIO_PUPD_PU;
    GPIO_Config(GPIOA, &gpio);
    GPIO_SetBit(GPIOA, gpio.pin);
}

void led_green_on(void)
{
    uint32_t now = system_millis();
    if (green_last_on == 0U && (uint32_t)(now - green_last_off) > LED_DURATION) {
        GPIO_ClearBit(GPIOA, LED_GREEN_PIN);
        green_last_on = now;
    }
}

void led_green_off(void) { GPIO_SetBit(GPIOA, LED_GREEN_PIN); }

void led_blue_blink(uint8_t numblinks)
{
    for (uint8_t i = 0U; i < numblinks; ++i) {
        GPIO_ClearBit(GPIOA, LED_BLUE_PIN);
        system_delay_ms(100U);
        GPIO_SetBit(GPIOA, LED_BLUE_PIN);
        system_delay_ms(100U);
    }
}

void led_blue_on(void)
{
    uint32_t now = system_millis();
    if (blue_last_on == 0U && (uint32_t)(now - blue_last_off) > LED_DURATION) {
        GPIO_ClearBit(GPIOA, LED_BLUE_PIN);
        blue_last_on = now;
    }
}

void led_process(void)
{
    uint32_t now = system_millis();
    if (blue_last_on != 0U && (uint32_t)(now - blue_last_on) > LED_DURATION) {
        GPIO_SetBit(GPIOA, LED_BLUE_PIN);
        blue_last_on = 0U;
        blue_last_off = now;
    }
    if (green_last_on != 0U && (uint32_t)(now - green_last_on) > LED_DURATION) {
        GPIO_SetBit(GPIOA, LED_GREEN_PIN);
        green_last_on = 0U;
        green_last_off = now;
    }
}
