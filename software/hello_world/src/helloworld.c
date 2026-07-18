// M3 Minimalgrafik - Rafiq Danial Bin Rajman, 893273 

#include "xgpiops.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"
#include <stdint.h>

// hardware 
#define SPI_BASE XPAR_XSPIPS_0_BASEADDR
#define GPIO_BASE  XPAR_XGPIOPS_0_BASEADDR
#define PIN_DC 54u   // emio gpio 54 -> K11 -> header 18
#define PIN_RST  55u  // 55 -> K13 -> header 22
#define LCD_W  320
#define LCD_H 480

// spi registers, offsets from UG585
#define R_CR   0x00   // config page 562 
#define R_SR 0x04
#define R_ER   0x14   // enable page 583
#define R_TXD  0x1C
#define R_RXD 0x20

#define REG_RD(o)    (*(volatile uint32_t*)(SPI_BASE+(o)))
#define REG_WR(o,v)  (*(volatile uint32_t*)(SPI_BASE+(o))=(v))

// CR bits needed
#define CR_MASTER   (1u << 0)
#define CR_DIV64  (0x5u << 3)   // ~166MHz/64 = ~2.6MHz
#define CR_MANCS    (1u << 14)
#define CR_CSBITS (0xFu << 10)
#define CR_CS_ON    (0xEu << 10)  // ss0 low = selected 
#define CR_CS_OFF (0xFu << 10)

#define SR_TXFULL   (1u << 3)
#define SR_RXAVAIL  (1u << 4)

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

static XGpioPs gpio;

// low level spi

static void cs_select(void)
{
    REG_WR(R_CR, (REG_RD(R_CR) & ~CR_CSBITS) | CR_CS_ON);
}
static void cs_release(void) {
    REG_WR(R_CR, (REG_RD(R_CR) & ~CR_CSBITS) | CR_CS_OFF);
}
// never do a transfer while cs is released (0xF), controller hangs in the wait loops. learned that during the pin debugging


static void spi_send(uint8_t b)
{
    while (REG_RD(R_SR) & SR_TXFULL) {}      // wait for room
    REG_WR(R_TXD, b);
    while (!(REG_RD(R_SR) & SR_RXAVAIL)) {}  // byte really left?
    (void)REG_RD(R_RXD);   // spi always gets one back, has to be drained or the fifo fills up 
                              
}

// write_command / write_data like most drivers do
// command = DC low, data = DC high for spi
static void write_command(uint8_t c)
{
    cs_select();
    XGpioPs_WritePin(&gpio, PIN_DC, 0);
    spi_send(c);
    cs_release();
}
static void write_data(uint8_t d) {
    cs_select();
    XGpioPs_WritePin(&gpio, PIN_DC, 1);
    spi_send(d);
    cs_release();
}

static void spi_setup(void)
{
    REG_WR(R_ER, 0);   // off while changing settings
    REG_WR(R_CR, CR_MASTER | CR_MANCS | CR_DIV64 | CR_CS_OFF);
    REG_WR(R_ER, 1);
}

// display init

static int lcd_init(void)
{
    XGpioPs_Config *cfg = XGpioPs_LookupConfig(GPIO_BASE);
    if (!cfg) return XST_FAILURE;
    if (XGpioPs_CfgInitialize(&gpio, cfg, cfg->BaseAddr) != XST_SUCCESS)
        return XST_FAILURE;
    // direction and output enable, both needed or the pin stays quiet
    XGpioPs_SetDirectionPin   (&gpio, PIN_DC, 1);
    XGpioPs_SetOutputEnablePin(&gpio, PIN_DC, 1);
    XGpioPs_SetDirectionPin   (&gpio, PIN_RST, 1);
    XGpioPs_SetOutputEnablePin(&gpio, PIN_RST, 1);

    spi_setup();

    // hardware reset, timing from datasheet ili9488
    XGpioPs_WritePin(&gpio, PIN_RST, 1); usleep(5000);
    XGpioPs_WritePin(&gpio, PIN_RST, 0); usleep(20000);
    XGpioPs_WritePin(&gpio, PIN_RST, 1); usleep(150000);

    xil_printf("GPIO+SPI OK\r\n");

    // init sequence, values from ili9488 datasheet and TFT_eSPI
    write_command(0xE0);   // pos gamma
    write_data(0x00); write_data(0x03); write_data(0x09); write_data(0x08);
    write_data(0x16); write_data(0x0A); write_data(0x3F); write_data(0x78);
    write_data(0x4C); write_data(0x09); write_data(0x0A); write_data(0x08);
    write_data(0x16); write_data(0x1A); write_data(0x0F);

    write_command(0xE1);   // neg gamma
    write_data(0x00); write_data(0x16); write_data(0x19); write_data(0x03);
    write_data(0x0F); write_data(0x05); write_data(0x32); write_data(0x45);
    write_data(0x46); write_data(0x04); write_data(0x0E); write_data(0x0D);
    write_data(0x35); write_data(0x37); write_data(0x0F);

    write_command(0xC0);   // power 1
    write_data(0x17); write_data(0x15);
    write_command(0xC1);   // power 2
    write_data(0x41);
    write_command(0xC5);   // vcom
    write_data(0x00); write_data(0x12); write_data(0x80);
    write_command(0x36);   // orientation (madctl)
    write_data(0x48);
    write_command(0x3A);   // pixel format
    write_data(0x66);   // 18 bit, spi cant do anything else
    write_command(0xB0);
    write_data(0x00);
    write_command(0xB1);   // frame rate
    write_data(0xA0);
    write_command(0xB4);
    write_data(0x02);
    write_command(0xB6);   // display function
    write_data(0x02); write_data(0x02); write_data(0x3B);
    write_command(0xB7);
    write_data(0xC6);
    write_command(0xF7);   // adjust control
    write_data(0xA9); write_data(0x51); write_data(0x2C); write_data(0x82);

    write_command(0x11);   // sleep out
    usleep(120000);        // datasheet says wait 120ms
    write_command(0x29);   // display on
    usleep(25000);

    xil_printf("LCD Init OK\r\n");
    return XST_SUCCESS;
}

// pixel window

// opens the paint window. after this cs stays selected and dc stays on data so pixels can be pushed directly with spi_send
// dont forget close_window at the end
static void open_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    write_command(0x2A);   // columns from-to
    write_data(x0 >> 8); write_data(x0 & 0xFF);
    write_data(x1 >> 8); write_data(x1 & 0xFF);
    write_command(0x2B);   // rows from-to
    write_data(y0 >> 8); write_data(y0 & 0xFF);
    write_data(y1 >> 8); write_data(y1 & 0xFF);
    write_command(0x2C);   // memory write
    cs_select();
    XGpioPs_WritePin(&gpio, PIN_DC, 1);   // pixels are data
}
static void close_window(void)
{
    cs_release();
}

// RGB565 -> only 3 bytes because the ili9488 over spi only takes 18 bit
static void pixel_out(uint16_t c)
{
    spi_send((c & 0xF800) >> 8);
    spi_send((c & 0x07E0) >> 3);
    spi_send((c & 0x001F) << 3);
}

// basic drawing 

static void fill_rect(int x, int y, int w, int h, uint16_t c)
{
    if (w <= 0 || h <= 0) return;
    open_window((uint16_t)x, (uint16_t)y,
                (uint16_t)(x+w-1), (uint16_t)(y+h-1));
    for (long i = 0; i < (long)w*h; i++)
        pixel_out(c);
    close_window();
}

static void fill_screen(uint16_t c)
{
    fill_rect(0, 0, LCD_W, LCD_H, c);
}

static void put_pixel(int x, int y, uint16_t c)
{
    if (x < 0 || y < 0 || x >= LCD_W || y >= LCD_H) return;
    fill_rect(x, y, 1, 1, c);   // pixel = 1x1 rect
}

// outline = 4 thin rects
static void rect_outline(int x, int y, int w, int h, uint16_t c)
{
    fill_rect(x, y, w, 1, c);       // top side
    fill_rect(x, y+h-1, w, 1, c);   // bottom side
    fill_rect(x, y, 1, h, c);
    fill_rect(x+w-1, y, 1, h, c);
}

// M3: lines

// bresenham like in the lecture, two cases: flat (x drives) and
// steep (y drives). d tells when the slow axis has to step.
// [AI] built with AI help from the lecture/wikipedia pseudocode,
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
static void draw_line(int x0, int y0, int x1, int y1, uint16_t c)
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
static void draw_circle(int cx, int cy, int r, uint16_t c)
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
static void draw_char(int x, int y, char ch, uint16_t c, uint16_t bg, int scale)
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

static void draw_text(int x, int y, const char *s, uint16_t c, uint16_t bg, int scale)
{
    while (*s) {
        draw_char(x, y, *s, c, bg, scale);
        x += 6*scale;   // 5 wide + 1 gap
        s++;
    }
}

// main

int main(void)
{
    xil_printf("\r\n M3 test card - Rafiq Danial Bin Rajman 893273 \r\n");

    if (lcd_init() != XST_SUCCESS) {
        xil_printf("init failed\r\n");
        return -1;
    }

    xil_printf("background\r\n");
    fill_screen(BLACK);

    // checklist 1: whole display area. border sits on the outermost pixels 0,0 to 319,479 - if an edge line is missing or cut off something is wrong with the drawing area
    xil_printf("border\r\n");
    rect_outline(0, 0, LCD_W, LCD_H, CYAN);

    // checklist 2: orientation. label must show up top left,
    // anywhere else means madctl (0x36) is wrong
    xil_printf("origin label\r\n");
    draw_text(6, 6, "0,0", RED, BLACK, 1);

    // checklist 3: center. diagonals corner to corner cross exactly in the middle, cross + circles sit at computed center 160,240 
     
    xil_printf("diagonals\r\n");
    draw_line(0, 0, LCD_W-1, LCD_H-1, MAGENTA);
    draw_line(LCD_W-1, 0, 0, LCD_H-1, MAGENTA);

    // line fan from the first demo version, dont need it here anymore
    // because the diagonals already show the lines work
    //for (int i = 0; i < 8; i++)
    //    draw_line(20, 460, 40 + i*35, 150, MAGENTA);

    xil_printf("center\r\n");
    fill_rect(120, 200, 81, 81, BLACK); // calm zone so the cross is visible
    fill_rect(140, 240, 41, 1, WHITE); // cross
    fill_rect(160, 220, 1, 41, WHITE);
    draw_circle(160, 240, 60, RED);  //for the circle
    draw_circle(160, 240, 45, YELLOW);
    draw_circle(160, 240, 30, GREEN);

    // name at the bottom part of screen
    xil_printf("text\r\n");
    draw_text(30, 380, "RAFIQ DANIAL BIN RAJMAN", WHITE, BLACK, 2);
    draw_text(30, 405, "893273  M3", YELLOW, BLACK, 2);

    xil_printf("done\r\n");
    while (1) {}
    return 0;
}