
#include <stdio.h>
#include "xparameters.h"
#include "xil_printf.h"

#include "xscugic.h"
#include "scugic_header.h"

#include "xaxivdma.h"
#include "axivdma_header.h"

#include "xdmaps.h"
#include "dmaps_header.h"

#include "xiicps.h"
#include "iicps_header.h"

#include "xqspips.h"
#include "qspips_header.h"

#include "xscutimer.h"
#include "scutimer_header.h"

#include "xscuwdt.h"
#include "scuwdt_header.h"

#include "xspips.h"
#include "spips_header.h"

#include "xttcps.h"
#include "ttcps_header.h"

#include "xuartps.h"
#include "uartps_header.h"

#include "xwdtps.h"
#include "wdtps_header.h"
int main ()
{

    static XScuGic intc;
    static XDmaPs dmac_s;
    static XScuTimer scutimer;
    static XScuWdt scuwdt;
    static XTtcPs ttc0;
    static XTtcPs ttc1;

    print("---Entering main---\n\r");

    {
        int status;
        print("\r\nRunning ScuGicInterruptSetup for intc ... \r\n");
        status = ScuGicInterruptSetup(&intc, XPAR_INTC_BASEADDR);
        if (status == 0) {
            print("ScuGicInterruptSetup PASSED \r\n");
        } else {
            print("ScuGicInterruptSetup FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning AxiVDMASelfTestExample for video_out_axi_vdma_0 ... \r\n");
        status = AxiVDMASelfTestExample(XPAR_VIDEO_OUT_AXI_VDMA_0_BASEADDR);
        if (status == 0) {
            print("AxiVDMASelfTestExample PASSED \r\n");
        } else {
            print("AxiVDMASelfTestExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning XDmaPs_Example_W_Intr for dmac_s ... \r\n");
        status = XDmaPs_Example_W_Intr(&dmac_s, XPAR_DMAC_S_BASEADDR);
        if (status == 0) {
            print("XDmaPs_Example_W_Intr PASSED \r\n");
        } else {
            print("XDmaPs_Example_W_Intr FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning IicPsSelfTestExample for i2c1 ... \r\n");
        status = IicPsSelfTestExample(XPAR_I2C1_BASEADDR);
        if (status == 0) {
            print("IicPsSelfTestExample PASSED \r\n");
        } else {
            print("IicPsSelfTestExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning QspiPsSelfTestExample for qspi ... \r\n");
        status = QspiPsSelfTestExample(XPAR_QSPI_BASEADDR);
        if (status == 0) {
            print("QspiPsSelfTestExample PASSED \r\n");
        } else {
            print("QspiPsSelfTestExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning ScuTimerPolledExample for scutimer ... \r\n");
        status = ScuTimerPolledExample(&scutimer, XPAR_SCUTIMER_BASEADDR);
        if (status == 0) {
            print("ScuTimerPolledExample PASSED \r\n");
        } else {
            print("ScuTimerPolledExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning ScuTimerIntrExample for scutimer ... \r\n");
        status = ScuTimerIntrExample(&scutimer, XPAR_SCUTIMER_BASEADDR);
        if (status == 0) {
            print("ScuTimerIntrExample PASSED \r\n");
        } else {
            print("ScuTimerIntrExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning ScuWdtIntrExample for scuwdt ... \r\n");
        status = ScuWdtIntrExample(&scuwdt, XPAR_SCUWDT_BASEADDR);
        if (status == 0) {
            print("ScuWdtIntrExample PASSED \r\n");
        } else {
            print("ScuWdtIntrExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning SpiPsSelfTestExample for spi0 ... \r\n");
        status = SpiPsSelfTestExample(XPAR_SPI0_BASEADDR);
        if (status == 0) {
            print("SpiPsSelfTestExample PASSED \r\n");
        } else {
            print("SpiPsSelfTestExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning TmrInterruptExample for ttc0 ... \r\n");
        status = TmrInterruptExample(&ttc0, XPAR_TTC0_BASEADDR);
        if (status == 0) {
            print("TmrInterruptExample PASSED \r\n");
        } else {
            print("TmrInterruptExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning TmrInterruptExample for ttc1 ... \r\n");
        status = TmrInterruptExample(&ttc1, XPAR_TTC1_BASEADDR);
        if (status == 0) {
            print("TmrInterruptExample PASSED \r\n");
        } else {
            print("TmrInterruptExample FAILED \r\n");
        }
    }

    {
        int status;
        print("\r\nRunning WdtPsSelfTestExample for watchdog0 ... \r\n");
        status = WdtPsSelfTestExample(XPAR_WATCHDOG0_BASEADDR);
        if (status == 0) {
            print("WdtPsSelfTestExample PASSED \r\n");
        } else {
            print("WdtPsSelfTestExample FAILED \r\n");
        }
    }

    print("---Exiting main---");
    return 0;
}
