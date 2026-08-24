// display_driver.h - Rafiq Danial Bin Rajman, 893273
// the low level stuff that helloworld.c and m3_graphics.c both need.
// only declarations here, the code itself stays in helloworld.c

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdint.h>

#define LCD_W  320 //width
#define LCD_H 480 //height

// colors of the RGB565
#define BLACK    0x0000u
#define WHITE    0xFFFFu
#define RED      0xF800u
#define GREEN    0x07E0u
#define BLUE     0x001Fu
#define YELLOW   0xFFE0u
#define CYAN     0x07FFu
#define MAGENTA  0xF81Fu
#define ORANGE   0xFD20u

// these live in helloworld.c
void open_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void close_window(void);
void pixel_out(uint16_t c);

#endif