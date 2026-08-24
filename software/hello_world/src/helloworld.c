// M4 LVGL Port - Rafiq Danial Bin Rajman, 893273
// lvgl v9 in, flush callback + tick. driver is still the M3 one
// the M3 drawing functions are in m3_graphics.c now, not in this file
// made with AI help (Claude), marked [AI] where it helped most
// Source : LVGL porting docs, UG585 (CR p.562, ER p.583), ili9488 datasheet
//
// !! just copying the lvgl folder into src is NOT enough, cmake doesnt
// build it. had to put it in CMakeLists.txt otherwise the linker doesnt find lv_init and all the rest.

#include "xgpiops.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "sleep.h"
#include <stdint.h>
#include <stddef.h>    // NULL

#include "lvgl/lvgl.h"   // lvgl folder sits next to this file

#include "display_driver.h"   // sizes, colors, window functions
#include "m3_graphics.h"      // M3 drawing, not used anymore but still in the project

// hardware 
#define SPI_BASE XPAR_XSPIPS_0_BASEADDR
#define GPIO_BASE  XPAR_XGPIOPS_0_BASEADDR
#define PIN_DC 54u   // emio gpio 54 -> K11 -> header 18
#define PIN_RST  55u  // 55 -> K13 -> header 22

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
void open_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
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
void close_window(void)
{
    cs_release();
}

// RGB565 -> only 3 bytes because the ili9488 over spi only takes 18 bit
void pixel_out(uint16_t c)
{
    spi_send((c & 0xF800) >> 8);  //red
    spi_send((c & 0x07E0) >> 3);  //green
    spi_send((c & 0x001F) << 3);  //blue
}

// main

// counts every flushed stripe, just so i have something live on the screen
static uint32_t frame_count = 0;

// flush callback. lvgl gives me a finished rectangle and i just push it through my window+pixel path from M3
// px_map is rgb565 so 2 bytes per pixel
// [AI] the callback signature and the lvgl calls (lv_display_flush_ready)
// came with AI help, id never used lvgl before. the open_window /
// spi_send / close_window part inside is my own M3 code
static void my_flush_cb(lv_display_t *disp, const lv_area_t *area,
                        uint8_t *px_map)
{
    uint16_t *p = (uint16_t *)px_map;

    open_window((uint16_t)area->x1, (uint16_t)area->y1,
                (uint16_t)area->x2, (uint16_t)area->y2);

    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    for (int32_t i = 0; i < w * h; i++) {
        uint16_t c = *p++;
        spi_send((c & 0xF800) >> 8);
        spi_send((c & 0x07E0) >> 3);
        spi_send((c & 0x001F) << 3);
    }
    close_window();

    frame_count++;   // for the display, counts every flushed stripe

    lv_display_flush_ready(disp);   // without this lvgl waits forever and you
                                    // only see the first stripe
}

// tick, lvgl wants to know how many ms passed
// [AI] that lvgl needs a tick callback at all and how to register it
// it came from the lvgl documents with AI help
// normally use xtime_l.h for this but thats not in my bsp so i count myself
//  loop sleeps 5ms so +5 per round. runs a bit slow
// because the loop itself takes time too but for animations its fine
static uint32_t tick_ms = 0;

static uint32_t my_tick_cb(void)
{
    return tick_ms;
}

// [AI] partial render mode and how to size the buffer worked out with
// AI help, the 40 lines are my own choice after looking at the ram
// draw buffer, 40 lines. partial mode = lvgl renders in stripes that size.
// bigger would be fewer flush calls but more ram, full screen
// would be 320*480*2 = 300kb
#define BUF_LINES 40
static uint8_t draw_buf[LCD_W * BUF_LINES * 2];

// main

// LIVE VALUES on the screen 
// idea: instead of just my name standing there i show numbers that
// actually change while it runs. uptime and frame count show that the
// loop is alive, the memory bar shows how much of lvgls pool is used.
// the memory part is also useful for the performance analysis later

static lv_obj_t *lbl_uptime;
static lv_obj_t *lbl_frames;
static lv_obj_t *lbl_mem;
static lv_obj_t *bar_mem;

// gets called by lvgl once a second, updates the labels
// [AI] lv_timer_create and lv_mem_monitor usage with AI help,the uptime maths and which values to show are mine
static void update_values(lv_timer_t *t)
{
    (void)t;

    // uptime from my tick counter, /1000 = seconds
    uint32_t secs = tick_ms / 1000;
    lv_label_set_text_fmt(lbl_uptime, "Uptime    %02d:%02d:%02d",
                          (int)(secs / 3600),
                          (int)((secs / 60) % 60),
                          (int)(secs % 60));

    lv_label_set_text_fmt(lbl_frames, "Frames    %d", (int)frame_count);

    // how much of the lvgl memory pool is in use
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    lv_label_set_text_fmt(lbl_mem, "LVGL Mem  %d %%", (int)mon.used_pct);
    lv_bar_set_value(bar_mem, (int32_t)mon.used_pct, LV_ANIM_OFF);
}

// main

int main(void)
{
    xil_printf("\r\n M4 LVGL monitor - Rafiq Danial Bin Rajman 893273 \r\n");

    if (lcd_init() != XST_SUCCESS) {
        xil_printf("init failed\r\n");
        return -1;
    }

    // start lvgl. order matters, lv_init has to be first
    // [AI] the order of these calls and the function names come from the lvgl porting docs with AI help
    lv_init();
    lv_tick_set_cb(my_tick_cb);

    lv_display_t *disp = lv_display_create(LCD_W, LCD_H);
    lv_display_set_flush_cb(disp, my_flush_cb);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    xil_printf("lvgl display ready\r\n");

    // dark background, default is white and looks unfinished
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x101828), 0);

    // [AI] the lvgl widget calls (label_create, obj_align, style_text_color,bar_create) came with AI help, the layout and what to display is mine 
    lv_obj_t *title = lv_label_create(lv_screen_active());  //set title text
    lv_label_set_text(title, "ZynqBerry LVGL Monitor");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 25);

    // the three live values. text gets filled in by update_values,just a placeholder until the first update
    lbl_uptime = lv_label_create(lv_screen_active());
    lv_label_set_text(lbl_uptime, "Uptime    00:00:00");
    lv_obj_set_style_text_color(lbl_uptime, lv_color_hex(0x33CCFF), 0);
    lv_obj_align(lbl_uptime, LV_ALIGN_CENTER, 0, -70);

    lbl_frames = lv_label_create(lv_screen_active());
    lv_label_set_text(lbl_frames, "Frames    0");
    lv_obj_set_style_text_color(lbl_frames, lv_color_hex(0x33CCFF), 0);
    lv_obj_align(lbl_frames, LV_ALIGN_CENTER, 0, -45);

    lbl_mem = lv_label_create(lv_screen_active());
    lv_label_set_text(lbl_mem, "LVGL Mem  0 %");
    lv_obj_set_style_text_color(lbl_mem, lv_color_hex(0x33CCFF), 0);
    lv_obj_align(lbl_mem, LV_ALIGN_CENTER, 0, -20);

    // bar for the memory, 0-100
    bar_mem = lv_bar_create(lv_screen_active());
    lv_obj_set_size(bar_mem, 240, 20);
    lv_obj_align(bar_mem, LV_ALIGN_CENTER, 0, 15);
    lv_bar_set_range(bar_mem, 0, 100);
    lv_bar_set_value(bar_mem, 0, LV_ANIM_OFF);

    lv_obj_t *name = lv_label_create(lv_screen_active());
    lv_label_set_text(name, "Rafiq Danial Bin Rajman");
    lv_obj_set_style_text_color(name, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(name, LV_ALIGN_BOTTOM_MID, 0, -45);

    lv_obj_t *matnr = lv_label_create(lv_screen_active());
    lv_label_set_text(matnr, "Matrikelnummer 893273");
    lv_obj_set_style_text_color(matnr, lv_color_hex(0x888888), 0);
    lv_obj_align(matnr, LV_ALIGN_BOTTOM_MID, 0, -22);

    // lvgl timer, calls update_values once a second. handy because i dont have to count that myself in the loop
    
    lv_timer_create(update_values, 1000, NULL);

    xil_printf("entering lvgl loop\r\n");

    // lvgl has to run regularly, from the documents say every few ms
    while (1) {
        lv_timer_handler();
        usleep(5000);
        tick_ms += 5;   // see above
    }
    return 0;
}