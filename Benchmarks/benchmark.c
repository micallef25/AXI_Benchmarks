// libs
#include <stdio.h>
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
#include "sleep.h"

#include "../axipmon/axipmon_imp.h"
// my stuff
#include "../Overlays/stream.h"


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


void XTmrCtr_InterruptHandler()
{
	//printf("XExample_IsDone() %d\n",XExample_rx_IsDone(AXI_Configrx));
	//printf("XExample_InterruptGetStatus() %d\n",XExample_rx_InterruptGetStatus(AXI_Configrx));
	//XExample_rx_InterruptClear(AXI_Configrx,XExample_rx_InterruptGetStatus(AXI_Configrx));
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

int run_benchmark()
{
	u32 Metrics;
	u32 ClkCntHigh = 0x0;
	u32 ClkCntLow = 0x0;
	int Status;

	clear_streams();
	// Start AXIPMON
	Status = Setup_AxiPmon();
	if (Status != XST_SUCCESS) {
		xil_printf("AXI Performance Monitor Polled Failed To Start\r\n");
		return XST_FAILURE;
	}
	printf("Starting Test Run\n");

	//
	//
	// Create tx Stream
	stream_create( STREAM_ID_0, 100, TX, CACHE, HPC0 );
	stream_init(STREAM_ID_0 );
	//
	//
	// Create rx stream
	stream_create( STREAM_ID_1, 100, RX, CACHE, HPC0 );
	stream_init(STREAM_ID_1 );

	// start streams
	start_stream( STREAM_ID_0 );
	start_stream( STREAM_ID_1 );

	//
	//
	// Send data
	for(int i = 0; i < 100; i++)
	{
		simple_write( STREAM_ID_0, i );
	}

	simple_write( STREAM_ID_0, 0xdeadbeef);

	//
	//
	//
	// Wait for data back
	while(1)
	{
		printf("sleeping\n");
		sleep(2);
		if(is_stream_done(STREAM_ID_1))
			break;
	}
	//
	//
	//
	// Read data and confirm
	for(int i = 0; i < 100; i++)
	{
		printf("simple_read(%d)\n",simple_read( STREAM_ID_1 ));
	}

	//
	//
	// shut down and destroy streams
	Status = Shutdown_AxiPmon(&Metrics,&ClkCntHigh,&ClkCntLow);
	if (Status != XST_SUCCESS) {
		xil_printf("AXI Performance Monitor Polled Failed To Shutdown\r\n");
		return XST_FAILURE;
	}

	//
	//
	// destroy streams
	stream_destroy( STREAM_ID_0 );
	stream_destroy( STREAM_ID_1 );

	//
	//
	// print results
	printf("Metrics %d\n",Metrics);

	return XST_SUCCESS;
}
