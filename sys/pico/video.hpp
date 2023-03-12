#include "pico/stdlib.h"

void vid_begin();
void vid_end();
void vid_init();
void vid_preinit();
void vid_close();
void vid_setpal(int i, int r, int g, int b);
void vid_settitle(char *title);
