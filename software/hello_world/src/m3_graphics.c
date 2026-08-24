// m3_graphics.c - Rafiq Danial Bin Rajman, 893273
// M3 drawing code, moved out of helloworld.c so the M4 file only has the driver and the lvgl part. not called anymore, lvgl draws now

#include "m3_graphics.h"
#include "display_driver.h"

// DRAWING SETUP

void fill_rect(int x, int y, int w, int h, uint16_t c)
{
    if (w <= 0 || h <= 0) return;  //if width and height (-)
    open_window((uint16_t)x, (uint16_t)y,
                (uint16_t)(x+w-1), (uint16_t)(y+h-1));
    for (long i = 0; i < (long)w*h; i++)
        pixel_out(c);
    close_window();
}

void fill_screen(uint16_t c)  //single dot
{
    fill_rect(0, 0, LCD_W, LCD_H, c);
}

void put_pixel(int x, int y, uint16_t c)
{
    if (x < 0 || y < 0 || x >= LCD_W || y >= LCD_H) return;
    fill_rect(x, y, 1, 1, c);   // pixel = 1x1 rect
}

// outline = 4 thin rects
void rect_outline(int x, int y, int w, int h, uint16_t c)
{
    fill_rect(x, y, w, 1, c);       // top side
    fill_rect(x, y+h-1, w, 1, c);   // bottom side
    fill_rect(x, y, 1, h, c);
    fill_rect(x+w-1, y, 1, h, c);
}

// M3: lines

// bresenham , two cases: flat (x drives) and steep (y drives). d tells when the slow axis has to step.
// [AI] built with AI help from the wikipedia pseudocode,
// checked the flat case on paper for (0,0) to (6,3)
static void line_flat(int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int ystep = 1;
    if (dy < 0) { ystep = -1; dy = -dy; }
    int d = 2*dy - dx;
    int y = y0;
    for (int x = x0; x <= x1; x++) {
        put_pixel(x, y, c);
        if (d > 0) {
            y += ystep;
            d -= 2*dx;
        }
        d += 2*dy;
    }
}
static void line_steep(int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xstep = 1;
    if (dx < 0) { xstep = -1; dx = -dx; }
    int d = 2*dx - dy;
    int x = x0;
    for (int y = y0; y <= y1; y++) {
        put_pixel(x, y, c);
        if (d > 0) {
            x += xstep;
            d -= 2*dy;
        }
        d += 2*dx;
    }
}
void draw_line(int x0, int y0, int x1, int y1, uint16_t c)
{
    int adx = (x1 > x0) ? x1-x0 : x0-x1;
    int ady = (y1 > y0) ? y1-y0 : y0-y1;
    if (adx >= ady) {
        // flat: walk left to right
        if (x0 > x1) line_flat(x1, y1, x0, y0, c);
        else         line_flat(x0, y0, x1, y1, c);
    } else {
        // steep: walk top to bottom
        if (y0 > y1) line_steep(x1, y1, x0, y0, c);
        else         line_steep(x0, y0, x1, y1, c);
    }
}

// M3: circle

// midpoint circle, textbook version, start at top (0,r), d = 3-2r.
// every point mirrored into all 8 octants, circle is 8x symmetric.
// [AI] d update rules from the textbook algorithm
void draw_circle(int cx, int cy, int r, uint16_t c)
{
    int x = 0;
    int y = r;
    int d = 3 - 2*r;
    while (x <= y) {
        put_pixel(cx + x, cy + y, c);
        put_pixel(cx - x, cy + y, c);
        put_pixel(cx + x, cy - y, c);
        put_pixel(cx - x, cy - y, c);
        put_pixel(cx + y, cy + x, c);
        put_pixel(cx - y, cy + x, c);
        put_pixel(cx + y, cy - x, c);
        put_pixel(cx - y, cy - x, c);
        if (d < 0) {
            d += 4*x + 6;
        } else {
            d += 4*(x - y) + 10;
            y--;
        }
        x++;
    }
}

// M3: text

// [AI] for the help for the list. font 5x7: 5 bytes per char, 1 byte = 1 column, bit0 = top row.
// lookup over the string: position in string = position in table.
// IMPORTANT: string and table must be in the same order or you get
// wrong letters. only the chars i need, rest comes when needed
static const char font_chars[] = "RAFIQDNL 0123456789,MESTBJ";
static const uint8_t font_data[][5] = {
    {0x7F,0x09,0x19,0x29,0x46},   // R
    {0x7E,0x11,0x11,0x11,0x7E},   // A
    {0x7F,0x09,0x09,0x09,0x01},   // F
    {0x00,0x41,0x7F,0x41,0x00},   // I
    {0x3E,0x41,0x51,0x21,0x5E},   // Q
    {0x7F,0x41,0x41,0x41,0x3E},   // D
    {0x7F,0x04,0x08,0x10,0x7F},   // N
    {0x7F,0x40,0x40,0x40,0x40},   // L
    {0x00,0x00,0x00,0x00,0x00},   // space
    {0x3E,0x51,0x49,0x45,0x3E},   // 0
    {0x00,0x42,0x7F,0x40,0x00},   // 1
    {0x42,0x61,0x51,0x49,0x46},   // 2
    {0x21,0x41,0x45,0x4B,0x31},   // 3
    {0x18,0x14,0x12,0x7F,0x10},   // 4
    {0x27,0x45,0x45,0x45,0x39},   // 5
    {0x3C,0x4A,0x49,0x49,0x30},   // 6
    {0x01,0x71,0x09,0x05,0x03},   // 7
    {0x36,0x49,0x49,0x49,0x36},   // 8
    {0x06,0x49,0x49,0x29,0x1E},   // 9
    {0x00,0x60,0x60,0x00,0x00},   // ,
    {0x7F,0x02,0x0C,0x02,0x7F},   // M
    {0x7F,0x49,0x49,0x49,0x41},   // E
    {0x46,0x49,0x49,0x49,0x31},   // S
    {0x01,0x01,0x7F,0x01,0x01},   // T
    {0x7F,0x49,0x49,0x49,0x36},   // B 
    {0x20,0x40,0x41,0x3F,0x01},   // J
};

static int font_index(char ch) {
    for (int i = 0; font_chars[i] != 0; i++)
        if (font_chars[i] == ch) return i;
    return -1;  // dont have it, skip
}

// scaling: every font dot becomes a scale x scale block, letter
// gets stamped out of little squares.
// [AI] helped check my font bytes against grid paper
void draw_char(int x, int y, char ch, uint16_t c, uint16_t bg, int scale)
{
    int idx = font_index(ch);
    if (idx < 0) return;   // dont know it, skip

    // fill the whole cell with background first, otherwise old pixels show through inside the letter
    
    fill_rect(x, y, 5*scale, 7*scale, bg);

    for (int col = 0; col < 5; col++) {
        uint8_t bits = font_data[idx][col];
        for (int row = 0; row < 7; row++) {
            if ((bits >> row) & 1)
                fill_rect(x + col*scale, y + row*scale, scale, scale, c);
        }
    }
}

void draw_text(int x, int y, const char *s, uint16_t c, uint16_t bg, int scale)
{
    while (*s) {
        draw_char(x, y, *s, c, bg, scale);
        x += 6*scale;   // 5 wide + 1 gap
        s++;
    }
}