#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>

void system_init(void);
uint32_t system_millis(void);
void system_delay_ms(uint32_t delay);
void system_tick_isr(void);
uint32_t system_irq_save(void);
void system_irq_restore(uint32_t state);
void system_hex32(char *out, uint32_t val);

#endif
