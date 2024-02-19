#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "led.h"
#include "log.h"
#include "playback.h"
#include "nfc.h"
#include "soundctrl.h"

static const char *path_prefix = "/mnt/";
static volatile bool shutdown = false;

void signal_handler(int sig)
{
    (void)sig;
    shutdown = true;
}

static bool path_is_canonical(const char *path)
{
    char *resolved = realpath(path, NULL);
    if (!resolved)
    {
        ERR("realpath failed: %s", strerror(errno));
        return false;
    }
    bool is_canonical = (strcmp(path, resolved) == 0);
    free(resolved);
    return is_canonical;
}

int main()
{
    int exit_status = EXIT_FAILURE;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    nfc_context *context;
    nfc_device *pnd = open_nfc_initiator(&context);
    if (pnd == NULL)
        goto exit;

    if (init_sound())
        goto cleanup_nfc;

    playback_handle play = playback_init();
    if (!play)
    {
        ERR("failed to initialize playback");
        goto cleanup_nfc;
    }

    char *path = malloc(strlen(path_prefix) + ntag215_total_user_mem + 1);
    if (!path)
    {
        ERR("malloc failed");
        goto deinit_playback;
    }
    strcpy(path, path_prefix);

    led_set_solid(LED_COLOR_GREEN);

    bool got_tag = false;
    nfc_target nt;
    while (!shutdown)
    {
        // slow down the polling a bit
        if (usleep(100000) && errno == EINTR)
            continue;

        if (got_tag)
        {
            int ret = nfc_initiator_target_is_present(pnd, &nt);
            if (ret != 0)
            {
                INFO("tag removed");
                if (ret != NFC_ETGRELEASED)
                    ERR("nfc_initiator_target_is_present: %s", nfc_strerror(pnd));

                got_tag = false;

                if (playback_stop(play))
                    ERR("failed to stop playback");
            }
        }
        else // no tag
        {
            if (poll_ntag215(pnd, &nt) < 0)
                continue;

            got_tag = true;

            int ret = read_user_mem_str(pnd, path + strlen(path_prefix),
                                        ntag215_total_pages, ntag215_page_size);
            if (ret || !path_is_canonical(path))
                ERR("tag contents invalid: \"%s\"", path);
            else
            {
                INFO("new tag: path=%s", path);
                if (playback_start(play, path))
                    ERR("failed to start playback");
            }
        }
    }

    INFO("shutting down...");
    exit_status = EXIT_SUCCESS;

    if (got_tag)
        playback_stop(play);

    // cleanup follows
    free(path);
deinit_playback:
    playback_deinit(play);
cleanup_nfc:
    nfc_close(pnd);
    nfc_exit(context);
exit:
    exit(exit_status);
}
