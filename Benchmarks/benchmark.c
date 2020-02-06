

#include <stdio.h>
#include "platform.h"
#include "xexample_tx.h"
#include "xexample_rx.h"
#include <stdlib.h>
#include "xil_cache.h"
#include "xil_mmu.h"
#include "xpseudo_asm.h"

#include "xil_types.h"

#include "xparameters.h"

#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"

#define PSU_OCM_RAM_0	(0xFFFC0000)
#define OCM_BUFF1 (0xFFFC0000)
#define OCM_BUFF2 (0xFFFD0000)

volatile int* ocm_buff1 = (int*)OCM_BUFF1;
volatile int* ocm_buff2 = (int*)OCM_BUFF2;

void delay(int delay)
{
	for(int i = 0; i < delay; i++)
	{
		// spin
	}
}

volatile int count = 0;
XScuGic InterruptController; /* Instance of the Interrupt Controller */
static XScuGic_Config *GicConfig;/* The configuration parameters of the controller */
XExample_tx* AXI_Config;
XExample_rx* AXI_Configrx;

void XTmrCtr_InterruptHandler()
{
	printf("XExample_IsDone() %d\n",XExample_rx_IsDone(AXI_Configrx));
	printf("XExample_InterruptGetStatus() %d\n",XExample_rx_InterruptGetStatus(AXI_Configrx));
	XExample_rx_InterruptClear(AXI_Configrx,XExample_rx_InterruptGetStatus(AXI_Configrx));
	count = 10;
}

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
	/*
    * Connect the interrupt controller interrupt handler to the hardware
	* interrupt handling logic in the ARM processor.
	*/
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler) XScuGic_InterruptHandler,XScuGicInstancePtr);
	/*
    * Enable interrupts in the ARM
	*/
	Xil_ExceptionEnable();
	return XST_SUCCESS;
}

int ScuGicInterrupt_Init(u16 DeviceId, XExample_rx* AXI_Config)
{
int Status;
/*
* Initialize the interrupt controller driver so that it is ready to
* use.
* */
	GicConfig = XScuGic_LookupConfig(DeviceId);
	if (NULL == GicConfig) {
	return XST_FAILURE;
	}
	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,	GicConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
/*
* Setup the Interrupt System
* */
	Status = SetUpInterruptSystem(&InterruptController);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
/*
* Connect a device driver handler that will be called when an
* interrupt for the device occurs, the device driver handler performs
* the specific interrupt processing for the device
*/
	Status = XScuGic_Connect(&InterruptController,XPAR_FABRIC_EXAMPLE_RX_0_INTERRUPT_INTR,(Xil_ExceptionHandler)XTmrCtr_InterruptHandler,(void *)AXI_Config);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
/*
* Enable the interrupt for the device and then cause (simulate) an
* interrupt so the handlers will be called
*/
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_EXAMPLE_RX_0_INTERRUPT_INTR);
	return XST_SUCCESS;
}

int main()
{
    //
	printf("Starting Loop Back Test \n");

    //
    init_platform();

    //
    // select which memory type allocate memory
    // and setup the PL ports 
    // instantiate a port object with RD / WR


    //
    // 
    //


    cleanup_platform();
    return 0;
}
