#include "apm32f0xx.h"
#include "apm32f0xx_fmc.h"
#include "apm32f0xx_rcm.h"
#include "system.h"

static volatile uint32_t tick_ms;

void system_init(void)
{
    FMC_EnablePrefetchBuffer();
    FMC_SetLatency(FMC_LATENCY_1);
    RCM_EnableHSI48();
    while (RCM_ReadStatusFlag(RCM_FLAG_HSI48RDY) == RESET) { }
    RCM_ConfigAHB(RCM_SYSCLK_DIV_1);
    RCM_ConfigAPB(RCM_HCLK_DIV_1);
    RCM_ConfigSYSCLK(RCM_SYSCLK_SEL_HSI48);
    while (RCM_ReadSYSCLKSource() != RCM_SYSCLK_SEL_HSI48) { }
    SystemCoreClock = 48000000U;
    (void)SysTick_Config(SystemCoreClock / 1000U);
}

uint32_t system_millis(void) { return tick_ms; }

void system_delay_ms(uint32_t delay)
{
    uint32_t start = tick_ms;
    while ((uint32_t)(tick_ms - start) < delay) { }
}

void system_tick_isr(void) { ++tick_ms; }

uint32_t system_irq_save(void)
{
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    __DSB();
    __ISB();
    return state;
}

void system_irq_restore(uint32_t state)
{
    if ((state & 1U) == 0U) __enable_irq();
}

void system_hex32(char *out, uint32_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    for (uint8_t i = 0U; i < 8U; ++i) {
        out[i] = hex[(val >> ((7U - i) * 4U)) & 0x0FU];
    }
    out[8] = '\0';
}
