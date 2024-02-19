#include "led.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static const char led_dir[] = "/sys/class/leds/multicolor:indicator";

static int write_file(char *file, char *contents)
{
    char path[64];
    int ret = snprintf(path, sizeof(path), "%s/%s", led_dir, file);
    if (ret < 0)
        return 1;
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open %s: %s\n", path, strerror(errno));
        return 1;
    }
    ret = fprintf(fp, "%s\n", contents);
    if (ret < 0)
        fprintf(stderr, "ERROR: Failed to write to %s: %s\n", path, strerror(errno));
    fclose(fp);
    return (ret < 0);
}

static int led_set_trigger(char *trigger)
{
    return write_file("trigger", trigger);
}

static int led_set_color(enum led_color color)
{
    switch (color)
    {
    case LED_COLOR_GREEN:
        return write_file("multi_intensity", "1 0");
    case LED_COLOR_RED:
        return write_file("multi_intensity", "0 1");
    case LED_COLOR_AMBER:
        return write_file("multi_intensity", "1 1");
    default:
        fprintf(stderr, "ERROR: Invalid LED color %d\n", (int)color);
        return 1;
    }
}

static int led_set_brightness(bool on)
{
    return write_file("brightness", on ? "1" : "0");
}

static int led_set_delay(int on_ms, int off_ms)
{
    char buf[16];
    sprintf(buf, "%d\n", on_ms);
    int ret = write_file("delay_on", buf);
    if (ret)
        return ret;
    sprintf(buf, "%d\n", off_ms);
    return write_file("delay_off", buf);
}

int led_set_solid(enum led_color color)
{
    int ret;
    if ((ret = led_set_trigger("none")))
        return ret;
    if ((ret = led_set_color(color)))
        return ret;
    return led_set_brightness(true);
}

int led_set_blink(enum led_color color, int period_ms)
{
    int ret;
    if ((ret = led_set_trigger("timer")))
        return ret;
    if ((ret = led_set_color(color)))
        return ret;
    if ((ret = led_set_brightness(true)))
        return ret;
    return led_set_delay(period_ms / 2, period_ms / 2);
}
