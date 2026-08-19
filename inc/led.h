#ifndef LED_H
#define LED_H

#include <stdint.h>

#define LED_DURATION 25U

void led_init(void);
void led_blue_blink(uint8_t numblinks);
void led_green_on(void);
void led_green_off(void);
void led_blue_on(void);
void led_process(void);

#endif
