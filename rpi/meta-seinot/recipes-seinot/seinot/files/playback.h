#ifndef PLAYBACK_H_
#define PLAYBACK_H_

struct playback_data;
typedef struct playback_data* playback_handle;

playback_handle playback_init(void);
void playback_deinit(playback_handle data);
int playback_start(playback_handle data, const char *path);
int playback_stop(playback_handle data);

#endif
