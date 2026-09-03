// M5 Touch - Rafiq Danial Bin Rajman, 893273
// lvgl v9 in, flush callback + tick. driver is still the M3 one
// the M3 drawing functions are in m3_graphics.c now, not in this file
//
// M5: touch on SS1, working now. the reason it did not work for so long
// was NOT in this file, it was in the block design: SPI0_SS_I was left
// unconnected. that is the only slave select INPUT the PS SPI has and it
// belongs to SS0. mode fail detection watches it. with SS0 asserted the
// controller drives the line itself so nothing is flagged, but with SS1
// asserted the floating input still read low and the controller took
// that for a second master. it then set MODE_FAIL and cleared SPI_EN,
// so every SS1 transfer was aborted before it started (UG585 p.13).
//
// it stayed invisible because spi_setup below wrote the whole config
// register and cleared bit 17 MODEFAIL_GEN_EN, which switches the
// detection off. the silicon sets that bit at reset - the controller
// reads 0x00020000 before any configuration. now it is kept set.
//
// FIX: xlconstant = 1 tied to SPI0_SS_I in the block design.

// there are parts made with AI help (Claude), marked [AI] where it helped most
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
#define R_CR   0x00   // config page 562 - settings
#define R_SR 0x04  //status
#define R_ER   0x14   // enable page 583 - on/off. 14
#define R_TXD  0x1C // outbox - bytes coming off, 28
#define R_RXD 0x20  //inbox - bytes coming in, 32

#define REG_RD(o)    (*(volatile uint32_t*)(SPI_BASE+(o)))  //read
#define REG_WR(o,v)  (*(volatile uint32_t*)(SPI_BASE+(o))=(v)) //write

// CR bits needed
#define CR_MASTER   (1u << 0)
#define CR_DIV64  (0x5u << 3)   // ~166MHz/64 = ~2.6MHz
#define CR_MANCS    (1u << 14)
#define CR_CSBITS (0xFu << 10)  // all 4 positions
#define CR_CS_ON    (0xEu << 10)  // ss0 low = selected for display, 0xE 1110
#define CR_CS_TOUCH (0xDu << 10)  // ss1 low = touch selected, 0xD 1101
#define CR_CS_OFF (0xFu << 10)  //nobody, 0xF 1111

// M5: the touch chip is slower than the display, xpt2046 driver notes
// say stay under 2.5Mbit
#define CR_MODEFAIL_EN (1u << 17) // mode fail generation. the silicon sets this at reset, spi_setup used to clear it
                                  // position 17 turns fault alarm ON

#define SR_MODEFAIL (1u << 1)   // also clears SPI_EN, UG585 p.13
#define SR_TXFULL   (1u << 3)
#define SR_RXAVAIL  (1u << 4)

static XGpioPs gpio;

// low level spi

static void cs_select(void)
{
    REG_WR(R_CR, (REG_RD(R_CR) & ~CR_CSBITS) | CR_CS_ON);  //display
}
static void cs_release(void) {
    REG_WR(R_CR, (REG_RD(R_CR) & ~CR_CSBITS) | CR_CS_OFF);
}
// never do a transfer while cs is released (0xF), controller hangs in the wait loops. learned that during the pin debugging

// M5 touch
// touch sits on ss1, display on ss0. only one may be active at a time

// select the touch chip. simple now - the clock switching and the extra
// deselect step that used to be in here were attempts to work around the
// MODE_FAIL problem, and none of them helped because the cause was in the
// block design. one write is enough.
static void cs_touch(void)
{
    REG_WR(R_CR, (REG_RD(R_CR) & ~CR_CSBITS) | CR_CS_TOUCH);
}

#define TOUCH_GUARD  200000u

// like spi_send but KEEPS the byte that comes back instead of throwing it away.
// up to M4 the project only ever wrote, the touch is the first time a returning byte has to be correct.
// the guard counter stops a dead slave hanging the whole lvgl loop.
// the failed flag was added later: returning 0xFF alone is ambiguous
// because 0xFF is also a valid byte, so the caller could not tell the
// difference between an error and real data
static uint8_t spi_transfer(uint8_t b, int *failed)
{
    uint32_t guard = 0;
    uint32_t sr;

    *failed = 0;

    while (REG_RD(R_SR) & SR_TXFULL) {
        if (++guard > TOUCH_GUARD) { *failed = 1; return 0xFF; }
    }

    REG_WR(R_TXD, b);

    guard = 0;
    for (;;) {  // keep loop until one of these happens
        sr = REG_RD(R_SR);  //read status number

        if (sr & SR_RXAVAIL) return (uint8_t)REG_RD(R_RXD);  // is position 4 on?, if reply arrived , grab and stop

        // this check only means anything because bit 17 is kept set now
        if (sr & SR_MODEFAIL) { *failed = 1; return 0xFF; }

        if (++guard > TOUCH_GUARD) { *failed = 1; return 0xFF; }
    }
}

// command bytes from the xpt2046 datasheet. the chip on my display is an HR2046, a command compatible clone
// [AI] which command byte does what, and the 2 dummy bytes + >>3 reassembly, 
// came from the datasheet with AI help
#define TOUCH_CMD_X  0xD0  //x axis
#define TOUCH_CMD_Y  0x90  //y axis

// raw values run 0..4095. an untouched panel reads at the very top because the two layers are not in contact
#define TOUCH_MIN_VALID   200
#define TOUCH_MAX_VALID   3900

// calibration, measured by pressing the four corners and reading the
// position off the screen:
//   top-left  7,476    top-right   314,474
//   bot-left 10,0      bot-right   297,10
// X came out the right way round, Y is inverted and gets flipped below
#define TOUCH_RAW_X_MIN   300
#define TOUCH_RAW_X_MAX   3800
#define TOUCH_RAW_Y_MIN   300
#define TOUCH_RAW_Y_MAX   3800

// one axis, one conversion. the chip select stays LOW for all three
// bytes - command plus two dummy bytes, 24 clocks in total.
// the old version read X and Y inside a single selection, 
// but the datasheet (p.21) wants one selection per conversion
static int touch_read_axis(uint8_t cmd, uint16_t *value)
{
    uint8_t hi, lo;
    int failed;

    cs_touch();

    (void)spi_transfer(cmd, &failed);
    if (failed) { cs_release(); return -1; }

    hi = spi_transfer(0x00, &failed);
    if (failed) { cs_release(); return -1; }

    lo = spi_transfer(0x00, &failed);
    if (failed) { cs_release(); return -1; }

    cs_release();

    // 12 bit answer spread over two bytes and shifted left by 3
    *value = (uint16_t)(((hi << 8) | lo) >> 3);
    return 0;
}

// reads both axes and converts to pixels. returns 1 if it looks like a real press.
//  three samples with the middle one taken, 
// because single readings jump around a lot while the finger pressure changes
static int touch_read(uint16_t *sx, uint16_t *sy)
{
    uint16_t xs[3], ys[3];  // makes 3 slots
    uint16_t rx, ry, t;
    int i, j;
    long v;

    for (i = 0; i < 3; i++) {   // loop runs 3 times
        if (touch_read_axis(TOUCH_CMD_Y, &ys[i]) != 0) return 0;
        if (touch_read_axis(TOUCH_CMD_X, &xs[i]) != 0) return 0;
    }

    // small bubble sort on 3 values, then take the middle one
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2 - i; j++) {
            if (xs[j] > xs[j+1]) { t = xs[j]; xs[j] = xs[j+1]; xs[j+1] = t; }
            if (ys[j] > ys[j+1]) { t = ys[j]; ys[j] = ys[j+1]; ys[j+1] = t; }
        }
    }
    rx = xs[1];
    ry = ys[1];

    // nothing pressed looks like the top of the range
    if (rx < TOUCH_MIN_VALID || rx > TOUCH_MAX_VALID) return 0;
    if (ry < TOUCH_MIN_VALID || ry > TOUCH_MAX_VALID) return 0;

    v = ((long)rx - TOUCH_RAW_X_MIN) * LCD_W /
        (TOUCH_RAW_X_MAX - TOUCH_RAW_X_MIN);
    if (v < 0) v = 0;
    if (v >= LCD_W) v = LCD_W - 1;
    *sx = (uint16_t)v;

    // Y is inverted on this panel: pressing the top gives a HIGH raw
    // value and the bottom a low one, so subtract from the max
    v = (long)(TOUCH_RAW_Y_MAX - ry) * LCD_H /
        (TOUCH_RAW_Y_MAX - TOUCH_RAW_Y_MIN);
    if (v < 0) v = 0;
    if (v >= LCD_H) v = LCD_H - 1;
    *sy = (uint16_t)v;

    return 1;
}



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
    REG_WR(R_CR, CR_MASTER | CR_MANCS | CR_MODEFAIL_EN | CR_DIV64 | CR_CS_OFF);
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

// touch state, shared between the input callback and the display
static uint16_t last_x = 0;
static uint16_t last_y = 0;

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

// [AI] partial render mode and how to size the buffer worked out with AI help, 
// the 40 lines are my own choice after looking at the ram lvgl input device callback.
//  lvgl calls this on its own schedule and asks where the finger is. 
// same pattern as the flush callback but in the other direction: 
// there i hand lvgl pixels, here i hand it a position and a state.
//  this replaces the putty printout the old version had.
// [AI] the callback signature and lv_indev_create came from the lvgl docs with AI help, 
// what happens inside is my own touch code
static void my_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint16_t sx, sy;

    if (touch_read(&sx, &sy)) {
        last_x = sx;
        last_y = sy;
        data->point.x = (int32_t)sx;
        data->point.y = (int32_t)sy;
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        // keep the last position, lvgl wants a sensible coordinate on
        // release too so it knows where the release happened
        data->point.x = (int32_t)last_x;
        data->point.y = (int32_t)last_y;
        data->state   = LV_INDEV_STATE_RELEASED;
    }
}

// draw buffer, 40 lines. partial mode = lvgl renders in stripes that size.
// bigger would be fewer flush calls but more ram, full screen
// would be 320*480*2 = 300kb
#define BUF_LINES 40
static uint8_t draw_buf[LCD_W * BUF_LINES * 2];

// main

// LIVE VALUES on the screen 
// idea: instead of just my name standing there i show numbers that actually change while it runs.
//  uptime and frame count show that the loop is alive,
//  the memory bar shows how much of lvgls pool is used.
// the memory part is also useful for the performance analysis later

static lv_obj_t *lbl_uptime;
static lv_obj_t *lbl_frames;
static lv_obj_t *lbl_mem;
static lv_obj_t *bar_mem;
static lv_obj_t *lbl_touch;
static lv_obj_t *lbl_btn;
static uint32_t  press_count = 0;

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

    // touch position, so the raw path is visible even without pressing
    lv_label_set_text_fmt(lbl_touch, "Touch     %d, %d", (int)last_x, (int)last_y);
}

// called by lvgl when the button is clicked. lvgl works out that a press landed on this widget,
//  i only supply the coordinates
// [AI] lv_event_cb signature with AI help
static void btn_clicked(lv_event_t *e)
{
    (void)e;
    press_count++;
    lv_label_set_text_fmt(lbl_btn, "Pressed %d", (int)press_count);
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

    // register the touch as an lvgl input device. from here lvgl works out
    // which widget a press landed on, i only report position and state
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_cb);
    xil_printf("touch input device registered\r\n");

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

    // touch position readout
    lbl_touch = lv_label_create(lv_screen_active());
    lv_label_set_text(lbl_touch, "Touch     -, -");
    lv_obj_set_style_text_color(lbl_touch, lv_color_hex(0x33CCFF), 0);
    lv_obj_align(lbl_touch, LV_ALIGN_CENTER, 0, 45);

    // a real button, to show the whole chain works: finger -> xpt2046 ->
    // spi -> input callback -> lvgl hit testing -> event
    lv_obj_t *btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(btn, 200, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 120);
    lv_obj_add_event_cb(btn, btn_clicked, LV_EVENT_CLICKED, NULL);

    lbl_btn = lv_label_create(btn);
    lv_label_set_text(lbl_btn, "Press me");
    lv_obj_center(lbl_btn);

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