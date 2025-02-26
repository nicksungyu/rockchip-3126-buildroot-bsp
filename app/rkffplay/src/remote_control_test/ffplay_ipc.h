#ifndef FFPLAY_IPC_H_
#define FFPLAY_IPC_H_

void media_sync_ext(char *ext_pos);
void media_play(const char* file_path, int hWnd, int start_time);
void media_exit(void);
void media_pause(void);
void media_restore(void);

#endif // FFPLAY_IPC_H_