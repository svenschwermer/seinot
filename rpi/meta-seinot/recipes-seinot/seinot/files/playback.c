#include "playback.h"
#include "led.h"
#include "log.h"
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <out123.h>
#include <mpg123.h>

struct playback_data
{
    mpg123_handle *mh;
    out123_handle *ao;

    size_t buffer_size;
    void *buffer;

    pthread_t pid;
    bool thread_started;
};

playback_handle playback_init(void)
{
    playback_handle data = malloc(sizeof(struct playback_data));
    if (!data)
    {
        ERR("malloc failed");
        return NULL;
    }

    int err;
    data->mh = mpg123_new(NULL, &err);
    if (!data->mh)
    {
        ERR("mpg123_new failed: %s", mpg123_plain_strerror(err));
        goto free_data;
    }

    data->ao = out123_new();
    if (!data->ao)
    {
        ERR("out123_new failed");
        goto del_mpg123;
    }

    return data;

del_mpg123:
    mpg123_delete(data->mh);
free_data:
    free(data);
    return NULL;
}

void playback_deinit(playback_handle data)
{
    out123_del(data->ao);
    mpg123_delete(data->mh);
    free(data);
}

static void *playback_thread(void *param)
{
    playback_handle data = param;
    size_t decoded;
    int err;

    do
    {
        err = mpg123_read(data->mh, data->buffer, data->buffer_size, &decoded);
        size_t played = out123_play(data->ao, data->buffer, decoded);
        if (played != decoded)
            ERR("played less than decoded: %zu != %zu", played, decoded);
    } while (decoded > 0 && err == MPG123_OK);

    if (err == MPG123_ERR)
        ERR("mpg123_read failed: %s", mpg123_strerror(data->mh));

    led_set_solid(LED_COLOR_GREEN);

    return NULL;
}

int playback_start(playback_handle data, const char *path)
{
    if (mpg123_open(data->mh, path) != MPG123_OK)
    {
        ERR("Failed to open \"%s\": %s", path, mpg123_strerror(data->mh));
        goto error_out;
    }

    long rate;
    int channels, encoding;
    if (mpg123_getformat(data->mh, &rate, &channels, &encoding) != MPG123_OK)
    {
        ERR("Failed to get format for \"%s\": %s", path, mpg123_strerror(data->mh));
        goto close_mpg123;
    }

    mpg123_format_none(data->mh);
    mpg123_format(data->mh, rate, channels, encoding);

    if (out123_open(data->ao, NULL, NULL) != OUT123_OK)
    {
        ERR("out123_open failed: %s", out123_strerror(data->ao));
        goto close_mpg123;
    }

    if (out123_start(data->ao, rate, channels, encoding) != OUT123_OK)
    {
        ERR("Failed to start output: %s", out123_strerror(data->ao));
        goto close_out123;
    }

    data->buffer_size = mpg123_outblock(data->mh);
    data->buffer = malloc(data->buffer_size);
    if (!data->buffer)
    {
        ERR("Failed to allocate buffer");
        goto stop_out123;
    }

    if (pthread_create(&data->pid, NULL, playback_thread, data))
    {
        ERR("Failed to create playback thread: %s", strerror(errno));
        goto free_buffer;
    }
    data->thread_started = true;

    led_set_blink(LED_COLOR_GREEN, 800);

    return 0;

free_buffer:
    free(data->buffer);
stop_out123:
    out123_stop(data->ao);
close_out123:
    out123_close(data->ao);
close_mpg123:
    mpg123_close(data->mh);
error_out:
    return 1;
}

int playback_stop(playback_handle data)
{
    if (!data->thread_started)
        return 0;

    if (pthread_cancel(data->pid))
    {
        ERR("Failed to cancel playback thread: %s", strerror(errno));
        return 1;
    }

    void *ret;
    if (pthread_join(data->pid, &ret))
    {
        ERR("Failed to wait for playback thread: %s", strerror(errno));
        return 1;
    }

    free(data->buffer);
    out123_stop(data->ao);
    out123_close(data->ao);
    mpg123_close(data->mh);
    data->thread_started = false;

    led_set_solid(LED_COLOR_GREEN);

    return 0;
}
