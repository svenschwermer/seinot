#include "soundctrl.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <alsa/asoundlib.h>

static double get_normalized_volume(snd_mixer_elem_t *elem,
                                    snd_mixer_selem_channel_id_t channel,
                                    long min, long max);
static int set_normalized_volume(snd_mixer_elem_t *elem, double volume, int dir,
                                 long min, long max);

static snd_mixer_elem_t *get_selem(snd_mixer_t *mixer, const char *selem_name)
{
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);

    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t *elem = snd_mixer_find_selem(mixer, sid);
    if (!elem)
        ERR("Unable to find simple control \"%s\"", selem_name);
    return elem;
}
#if 0
static int unmute_selem(snd_mixer_t *mixer, const char *selem_name)
{
    snd_mixer_elem_t *elem = get_selem(mixer, selem_name);
    if (!elem)
        return -1;
    int err = snd_mixer_selem_set_playback_switch_all(elem, 1);
    if (err < 0)
        ERR("Failed to unmute \"%s\": %s", selem_name, snd_strerror(err));
    return err;
}
#endif
struct event_func_data
{
    double initial_volume; // 0..1
    double volume_increment;
    double max_volume; // 0..1

    snd_mixer_elem_t *elem;
    long min, max;
    int fd;
};

static void *event_func(void *param)
{
    struct event_func_data *data = param;

    while (1)
    {
        struct input_event ev;
        int n = read(data->fd, &ev, sizeof(ev));
        if (n != sizeof(ev))
        {
            ERR("Short read on event file");
            continue;
        }

        if (ev.type != EV_REL)
            continue;

        double cur_vol = get_normalized_volume(data->elem, SND_MIXER_SCHN_FRONT_LEFT,
                                               data->min, data->max);
        if (cur_vol > data->max_volume)
            cur_vol = data->max_volume;

        double set_vol = cur_vol + data->volume_increment * ev.value;
        if (set_vol < 0.0)
            set_vol = 0.0;
        else if (set_vol > data->max_volume)
            set_vol = data->max_volume;

        if (cur_vol == set_vol)
        {
            INFO("Volume at %s", (cur_vol == 0.0) ? "min" : "max");
            continue;
        }

        INFO("Setting volume to %.1f%% (was %.1f%%)", set_vol * 100, cur_vol * 100);
        set_normalized_volume(data->elem, set_vol, 0, data->min, data->max);
    }

    return NULL;
}

static int get_env_percent(const char *name, double *val, double default_percent)
{
    const char *s = getenv(name);
    if (!s)
    {
        INFO("Defaulting %s to %.1f%%", name, default_percent);
        *val = default_percent / 100;
        return 0;
    }

    char *endptr = NULL;
    errno = 0;
    *val = strtod(s, &endptr);
    if (errno)
    {
        ERR("Failed to convert environment variable %s (=%s) to floating-point number: %s", name, s, strerror(errno));
        return 1;
    }
    if (endptr == s)
    {
        ERR("Failed to convert environment variable %s (=%s) to floating-point number", name, s);
        return 1;
    }

    *val /= 100;
    return 0;
}

int init_sound(void)
{
    int ret = 0;

    snd_mixer_t *mixer = NULL;
    ret = snd_mixer_open(&mixer, 0);
    if (ret < 0)
    {
        ERR("Mixer open error: %s", snd_strerror(ret));
        goto out;
    }

    ret = snd_mixer_attach(mixer, "default");
    if (ret < 0)
    {
        ERR("Mixer attach error: %s", snd_strerror(ret));
        goto close_mixer;
    }

    ret = snd_mixer_selem_register(mixer, NULL, NULL);
    if (ret < 0)
    {
        ERR("Mixer register error: %s", snd_strerror(ret));
        goto close_mixer;
    }

    ret = snd_mixer_load(mixer);
    if (ret < 0)
    {
        ERR("Mixer load error: %s", snd_strerror(ret));
        goto close_mixer;
    }
#if 0
    ret = unmute_selem(mixer, "Left Output Mixer PCM");
    if (ret < 0)
        goto close_mixer;

    ret = unmute_selem(mixer, "Right Output Mixer PCM");
    if (ret < 0)
        goto close_mixer;

    snd_mixer_elem_t *elem = get_selem(mixer, "Speaker");
    if (!elem)
    {
        ret = -1;
        goto close_mixer;
    }
    ret = snd_mixer_selem_set_playback_dB_all(elem, 0, 0);
    if (ret < 0)
    {
        ERR("Failed to set volume: %s", snd_strerror(ret));
        goto close_mixer;
    }
#endif
    struct event_func_data *data = malloc(sizeof(struct event_func_data));
    if (!data)
    {
        ERR("Failed to allocate");
        goto close_mixer;
    }

    if (get_env_percent("INITIAL_VOLUME_PERCENT", &data->initial_volume, 20))
        goto free_data;
    if (get_env_percent("VOLUME_INCREMENT_PERCENT", &data->volume_increment, 2))
        goto free_data;
    if (get_env_percent("MAX_VOLUME_PERCENT", &data->max_volume, 50))
        goto free_data;

    data->elem = get_selem(mixer, "Digital");
    if (!data->elem)
    {
        ret = -1;
        goto free_data;
    }

    ret = snd_mixer_selem_get_playback_dB_range(data->elem, &data->min, &data->max);
    if (ret < 0 || data->min >= data->max)
    {
        ERR("Failed to get volume range: %s", snd_strerror(ret));
        goto free_data;
    }

    INFO("Setting initial volume to %.0f%%", data->initial_volume * 100);
    ret = set_normalized_volume(data->elem, data->initial_volume, 0, data->min, data->max);
    if (ret < 0)
        ERR("Failed to set initial volume: %s", snd_strerror(ret));

    data->fd = open("/dev/input/event0", O_RDONLY);
    if (data->fd < 0)
    {
        ERR("Failed to open event file");
        goto free_data;
    }

    pthread_t pid;
    ret = pthread_create(&pid, NULL, event_func, data);
    if (ret != 0)
    {
        ERR("Failed to start thread: %s", strerror(errno));
        goto close_fd;
    }

    return 0;

close_fd:
    close(data->fd);
free_data:
    free(data);
close_mixer:
    snd_mixer_close(mixer);
out:
    return ret;
}

// The following functions have been taken from the sources of alsamixer and
// slightly adapted

#define MAX_LINEAR_DB_SCALE 24

static inline bool use_linear_dB_scale(long dBmin, long dBmax)
{
    return dBmax - dBmin <= MAX_LINEAR_DB_SCALE * 100;
}

static long lrint_dir(double x, int dir)
{
    if (dir > 0)
        return lrint(ceil(x));
    else if (dir < 0)
        return lrint(floor(x));
    else
        return lrint(x);
}

static double get_normalized_volume(snd_mixer_elem_t *elem,
                                    snd_mixer_selem_channel_id_t channel,
                                    long min, long max)
{
    long value;
    double normalized, min_norm;
    int err;

    err = snd_mixer_selem_get_playback_dB(elem, channel, &value);
    if (err < 0)
        return 0;

    if (use_linear_dB_scale(min, max))
        return (value - min) / (double)(max - min);

    normalized = pow(10, (value - max) / 6000.0);
    if (min != SND_CTL_TLV_DB_GAIN_MUTE)
    {
        min_norm = pow(10, (min - max) / 6000.0);
        normalized = (normalized - min_norm) / (1 - min_norm);
    }

    return normalized;
}

static int set_normalized_volume(snd_mixer_elem_t *elem, double volume, int dir,
                                 long min, long max)
{
    long value;
    double min_norm;

    if (use_linear_dB_scale(min, max))
    {
        value = lrint_dir(volume * (max - min), dir) + min;
        return snd_mixer_selem_set_playback_dB_all(elem, value, dir);
    }

    if (min != SND_CTL_TLV_DB_GAIN_MUTE)
    {
        min_norm = pow(10, (min - max) / 6000.0);
        volume = volume * (1 - min_norm) + min_norm;
    }
    value = lrint_dir(6000.0 * log10(volume), dir) + max;
    return snd_mixer_selem_set_playback_dB_all(elem, value, dir);
}
