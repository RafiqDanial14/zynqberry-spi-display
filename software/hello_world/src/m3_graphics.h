// m3_graphics.h - Rafiq Danial Bin Rajman, 893273
// the M3 drawing functions. not called anymore since lvgl does the drawing now, but kept so the M3 work stays in the project

#ifndef M3_GRAPHICS_H
#define M3_GRAPHICS_H

#include <stdint.h>

void fill_rect(int x, int y, int w, int h, uint16_t c);
void fill_screen(uint16_t c);
void put_pixel(int x, int y, uint16_t c);
void rect_outline(int x, int y, int w, int h, uint16_t c);
void draw_line(int x0, int y0, int x1, int y1, uint16_t c);
void draw_circle(int cx, int cy, int r, uint16_t c);
void draw_char(int x, int y, char ch, uint16_t c, uint16_t bg, int scale);
void draw_text(int x, int y, const char *s, uint16_t c, uint16_t bg, int scale);

#endif