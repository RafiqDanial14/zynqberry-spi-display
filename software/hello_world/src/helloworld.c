/*
 * SPI0 + ReadID Test - Rafiq 893273
 *
 * 2 parts:
 *  1) check SPI0 controller works (read registers)
 *  2) try ReadID from display -> comes back all zeros (the M1 problem)
 *
 * addresses from UG585, display = ILI9486 on SPI0
 */

#include "xparameters.h"  // names + addresses for the chip
#include "xspips.h"       // spi functions
#include "xgpiops.h"      // gpio functions (for DC and RST pins)
#include "xil_printf.h"   // print to putty
#include "xil_io.h"       // read/write registers directly
#include "sleep.h"        // for usleep (wait/pause)

#define DC   54   // DC pin (EMIO 54) - tells display: command /0 or data/1
#define RST  55   // RST pin (EMIO 55) - resets the display

// SPI0 register addresses from UG585
#define SPI0_CR    0xE0006000   // config register, 06000 is spi config register lives
#define SPI0_ER    0xE0006014   // enable register
#define SPI0_ID    0xE00060FC   // module id, should be 0x00090106

XSpiPs Spi;     // holds spi settings
XGpioPs Gpio;   // holds gpio settings

// reset the display - timing from ILI9486 datasheet
void resetDisplay()
{
    XGpioPs_WritePin(&Gpio, RST, 1); usleep(20000);   // rst high (normal run state)
    XGpioPs_WritePin(&Gpio, RST, 0); usleep(20000);   // rst low (reset pulse)
    XGpioPs_WritePin(&Gpio, RST, 1); usleep(150000);  // rst high again, wait before talk
}

int main()
{
    xil_printf("\r\nSPI0 + ReadID Test - Rafiq Danial Bin Rajman 893273\r\n");  // title

    // gpio setup (DC and RST pins) 
    XGpioPs_Config *gc = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_BASEADDR);  // find gpio info
    XGpioPs_CfgInitialize(&Gpio, gc, gc->BaseAddr);        // turn on gpio
    XGpioPs_SetDirectionPin(&Gpio, DC, 1);          // DC = output
    XGpioPs_SetOutputEnablePin(&Gpio, DC, 1);       // turn on DC driver
    XGpioPs_SetDirectionPin(&Gpio, RST, 1);         // RST = output
    XGpioPs_SetOutputEnablePin(&Gpio, RST, 1);      // turn on RST driver

    // spi0 setup 
    XSpiPs_Config *sc = XSpiPs_LookupConfig(XPAR_XSPIPS_0_BASEADDR);  // find spi0 info
    XSpiPs_CfgInitialize(&Spi, sc, sc->BaseAddress);                 // turn on spi0
    XSpiPs_SetOptions(&Spi, XSPIPS_MASTER_OPTION | XSPIPS_FORCE_SSELECT_OPTION);  // master + manual CS
    XSpiPs_SetClkPrescaler(&Spi, XSPIPS_CLK_PRESCALE_32);  // clock speed (divide by 32)
    XSpiPs_SetSlaveSelect(&Spi, 0x00);                     // pick device 0 (display)

    //  PART 1: check SPI0 controller works 
    xil_printf("\r\nPart 1: SPI0 controller \r\n");

    u32 cr = Xil_In32(SPI0_CR);                  // read config register
    xil_printf("CR = 0x%08X (non-zero = alive)\r\n", cr);  // print it

    u32 id = Xil_In32(SPI0_ID);                  // read module id
    xil_printf("ID = 0x%08X (should be 0x00090106)\r\n", id);  // print it

    Xil_Out32(SPI0_ER, 0x1);                     // write 1 to enable register
    u32 er = Xil_In32(SPI0_ER);                  // read it back
    xil_printf("ER = 0x%08X (write works)\r\n", er);  // print it

    // PART 2: try ReadID from display (the M1 problem) 
    xil_printf("\r\n Part 2: ReadID from display \r\n");

    resetDisplay();                              // reset the display first
    xil_printf("display reset done\r\n");

    u8 tx[4] = {0x04, 0, 0, 0};                  // command 0x04 + 3 dummy bytes
    u8 rx[4] = {0};                              // empty box for the answer
    XGpioPs_WritePin(&Gpio, DC, 0);              // DC low = this is a command
    XSpiPs_PolledTransfer(&Spi, tx, rx, 4);      // send command + read answer back

    // print what came back (its all zeros = the problem)
    xil_printf("ReadID (0x04) = %02X %02X %02X %02X\r\n", rx[0], rx[1], rx[2], rx[3]);
    

    xil_printf("\r\ndone\r\n");   
    return 0;                     // end program
}