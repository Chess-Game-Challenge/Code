#ifndef DISPLAY_H
#define DISPLAY_H

#include "lvgl.h"

extern lv_disp_t *global_disp;  // Declaração da variável Global

void initDisplay(void);
void displayText(const char *text);

#endif // DISPLAY_H