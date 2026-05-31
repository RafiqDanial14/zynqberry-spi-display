#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xspips.h"
#include "xgpiops.h"
#include "sleep.h"

// DC = GPIO24 = EMIO[20] = PS GPIO 74
// RST = GPIO25 = EMIO[21] = PS GPIO 75
#define DC_GPIO_PIN   74
#define RST_GPIO_PIN  75

static XSpiPs  SpiInst;
static XGpioPs GpioInst;

void DC_Low(void)   { XGpioPs_WritePin(&GpioInst, DC_GPIO_PIN,  0); }
void DC_High(void)  { XGpioPs_WritePin(&GpioInst, DC_GPIO_PIN,  1); }
void RST_Low(void)  { XGpioPs_WritePin(&GpioInst, RST_GPIO_PIN, 0); }
void RST_High(void) { XGpioPs_WritePin(&GpioInst, RST_GPIO_PIN, 1); }

void LCD_Reset(void) {
    RST_High(); usleep(10000);
    RST_Low();  usleep(20000);
    RST_High(); usleep(150000);
    xil_printf("LCD reset done\r\n");
}

void LCD_Cmd(u8 cmd) {
    DC_Low();
    XSpiPs_Transfer(&SpiInst, &cmd, NULL, 1);
}

void LCD_Data(u8 data) {
    DC_High();
    XSpiPs_Transfer(&SpiInst, &data, NULL, 1);
}

void LCD_ReadID(void) {
    u8 tx[4] = {0x04, 0x00, 0x00, 0x00};
    u8 rx[4] = {0x00, 0x00, 0x00, 0x00};
    DC_Low();
    XSpiPs_Transfer(&SpiInst, tx, rx, 4);
    xil_printf("ReadID bytes: %02X %02X %02X %02X\r\n",
               rx[0], rx[1], rx[2], rx[3]);
    // ILI9486 expected: xx 00 94 86
}

int main(void) {
    init_platform();
    xil_printf("\r\n=== ILI9486 SPI Bare-Metal Test ===\r\n");

    // ---- PS GPIO init ----
    XGpioPs_Config *GpioCfg = XGpioPs_LookupConfig(0);
    if (GpioCfg == NULL) {
        xil_printf("GPIO LookupConfig FAILED\r\n");
        return -1;
    }
    if (XGpioPs_CfgInitialize(&GpioInst, GpioCfg, GpioCfg->BaseAddr) != XST_SUCCESS) {
        xil_printf("GPIO CfgInitialize FAILED\r\n");
        return -1;
    }
    XGpioPs_SetDirectionPin(&GpioInst, DC_GPIO_PIN,  1);
    XGpioPs_SetOutputEnablePin(&GpioInst, DC_GPIO_PIN,  1);
    XGpioPs_SetDirectionPin(&GpioInst, RST_GPIO_PIN, 1);
    XGpioPs_SetOutputEnablePin(&GpioInst, RST_GPIO_PIN, 1);
    xil_printf("GPIO init OK\r\n");

    // ---- PS SPI init ----
    XSpiPs_Config *SpiCfg = XSpiPs_LookupConfig(0);
    if (SpiCfg == NULL) {
        xil_printf("SPI LookupConfig FAILED\r\n");
        return -1;
    }
    if (XSpiPs_CfgInitialize(&SpiInst, SpiCfg, SpiCfg->BaseAddress) != XST_SUCCESS) {
        xil_printf("SPI CfgInitialize FAILED\r\n");
        return -1;
    }
    XSpiPs_SetOptions(&SpiInst,
                      XSPIPS_MASTER_OPTION |
                      XSPIPS_FORCE_SSELECT_OPTION |
                      XSPIPS_MANUAL_START_OPTION);

    // SPI clock = ~160MHz / 32 = ~5MHz (safe for testing)
    XSpiPs_SetClkPrescaler(&SpiInst, XSPIPS_CLK_PRESCALE_32);

    // Assert CS line 0
    XSpiPs_SetSlaveSelect(&SpiInst, 0x00);

    xil_printf("SPI init OK\r\n");

    // ---- Test sequence ----
    LCD_Reset();

    LCD_Cmd(0x00);  // NOP
    xil_printf("NOP sent\r\n");

    LCD_ReadID();   // Expected: xx 00 94 86 for ILI9486

    LCD_Cmd(0x01);  // Software Reset
    usleep(150000);
    xil_printf("SW Reset sent\r\n");

    LCD_Cmd(0x11);  // Sleep Out
    usleep(150000);
    xil_printf("Sleep Out sent\r\n");

    xil_printf("=== Test complete ===\r\n");
    cleanup_platform();
    return 0;
}