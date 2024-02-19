#ifndef LED_H_
#define LED_H_

enum led_color {
    LED_COLOR_GREEN,
    LED_COLOR_RED,
    LED_COLOR_AMBER,
};

int led_set_solid(enum led_color color);
int led_set_blink(enum led_color color, int period_ms);

#endif
