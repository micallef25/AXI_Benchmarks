// libs
#include <stdio.h>

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
#include "xtime_l.h"
#include "../axipmon/axipmon_imp.h"
// my stuff
#include "../Overlays/stream.h"


#define IP_32

#ifdef IP_32
#include "xexample_tx_64.h"
#include "xexample_rx_64.h"
#else
#include "xexample_tx_128.h"
#include "xexample_rx_128.h"
#endif


void delay(int delay)
{
	for(int i = 0; i < delay; i++)
	{
		// spin
	}
}

//volatile int count = 0;
//XScuGic InterruptController; /* Instance of the Interrupt Controller */
//static XScuGic_Config *GicConfig;/* The configuration parameters of the controller */
//
//
//void XTmrCtr_InterruptHandler()
//{
//	//printf("XExample_IsDone() %d\n",XExample_rx_IsDone(AXI_Configrx));
//	//printf("XExample_InterruptGetStatus() %d\n",XExample_rx_InterruptGetStatus(AXI_Configrx));
//	//XExample_rx_InterruptClear(AXI_Configrx,XExample_rx_InterruptGetStatus(AXI_Configrx));
//	count = 10;
//}
//
//int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
//{
//	/*
//    * Connect the interrupt controller interrupt handler to the hardware
//	* interrupt handling logic in the ARM processor.
//	*/
//	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler) XScuGic_InterruptHandler,XScuGicInstancePtr);
//	/*
//    * Enable interrupts in the ARM
//	*/
//	Xil_ExceptionEnable();
//	return XST_SUCCESS;
//}
//
//int ScuGicInterrupt_Init(u16 DeviceId, XExample_rx* AXI_Config)
//{
//int Status;
///*
//* Initialize the interrupt controller driver so that it is ready to
//* use.
//* */
//	GicConfig = XScuGic_LookupConfig(DeviceId);
//	if (NULL == GicConfig) {
//	return XST_FAILURE;
//	}
//	Status = XScuGic_CfgInitialize(&InterruptController, GicConfig,	GicConfig->CpuBaseAddress);
//	if (Status != XST_SUCCESS)
//	{
//		return XST_FAILURE;
//	}
///*
//* Setup the Interrupt System
//* */
//	Status = SetUpInterruptSystem(&InterruptController);
//	if (Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
///*
//* Connect a device driver handler that will be called when an
//* interrupt for the device occurs, the device driver handler performs
//* the specific interrupt processing for the device
//*/
//	Status = XScuGic_Connect(&InterruptController,XPAR_FABRIC_EXAMPLE_RX_0_INTERRUPT_INTR,(Xil_ExceptionHandler)XTmrCtr_InterruptHandler,(void *)AXI_Config);
//	if (Status != XST_SUCCESS) {
//		return XST_FAILURE;
//	}
///*
//* Enable the interrupt for the device and then cause (simulate) an
//* interrupt so the handlers will be called
//*/
//	XScuGic_Enable(&InterruptController, XPAR_FABRIC_EXAMPLE_RX_0_INTERRUPT_INTR);
//	return XST_SUCCESS;
//}

int run_benchmark( int buffer_size, memory_type memory, int time, axi_port_type port )
{
	u32 Metrics;
	u32 ClkCntHigh = 0x0;
	u32 ClkCntLow = 0x0;
	int Status;
	int slot=0;
    XTime timer_start=0;
    XTime timer_end=0;
    XTime_StartTimer();

	clear_streams();

	if(port == HPC0)
	{
		slot = 1;
	}
	else if(port == HP0)
	{
		slot = 0;
	}

	//
	// Start AXIPMON
	Status = Setup_AxiPmon(slot);
	if (Status != XST_SUCCESS) {
		xil_printf("AXI Performance Monitor Polled Failed To Start\r\n");
		return XST_FAILURE;
	}

	//
	//
	printf("Format: Memory_Config,Total_Read_Latency,Read_Transactions,Total_Write_latency,Write_Transactions\n");

	//
	//
	// Create tx Stream
	stream_create( STREAM_ID_0, buffer_size, TX, memory, port );
	stream_init(STREAM_ID_0 );

	//
	//
	// Create rx stream
	stream_create( STREAM_ID_1, buffer_size, RX, memory, port );
	stream_init(STREAM_ID_1 );

	// start streams
	start_stream( STREAM_ID_0 );
	start_stream( STREAM_ID_1 );

	//
	//
	// Send data

	XTime_GetTime(&timer_start);

	for(int i = 0; i < buffer_size; i++)
	{
		simple_write( STREAM_ID_0, i );
		usleep( time );

		printf("%s,%s,%u,%u,%u,%u\n",port_type_to_str(port),mem_type_to_str(memory),query_metric(0),query_metric(1),query_metric(2),query_metric(3));

	}

	simple_write( STREAM_ID_0, 0xdeadbeef);

	XTime_GetTime(&timer_end);
	printf("%s,%s,%u,%u,%u,%u\n",port_type_to_str(port),mem_type_to_str(memory),query_metric(0),query_metric(1),query_metric(2),query_metric(3));
	printf("%s test run cycles %lu\n",mem_type_to_str(memory),timer_end-timer_start);
	//
	//
	// Wait for data back
	while(1)
	{
		//printf("sleeping\n");
		usleep(500);

		if(is_stream_done(STREAM_ID_1))
			break;
	}


	//
	//
	// Read data and confirm
	for(int i = 0; i < buffer_size; i++)
	{
		//printf("simple_read(%d)\n",simple_read( STREAM_ID_1 ));
//		sleep(1);
		u32 data = simple_read( STREAM_ID_1 );
	}

	//
	//
	// shut down performance monitors
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
	//printf("Metrics %d\n",Metrics);

	return XST_SUCCESS;
}

int run_benchmark_memory( int buffer_size, memory_type memory, int time, axi_port_type port )
{
	int Status;
    XTime timer_start=0;
    XTime timer_end=0;
    XTime_StartTimer();

	clear_streams();


	//
	//
	//printf("Format: Memory_Config,Total_Read_Latency,Bytes_Read... Running Test with buffer size %d and rest time of %d\n",buffer_size,time);

	//
	//
	// Create tx Stream
	stream_create( STREAM_ID_0, buffer_size, TX, memory, port );
	stream_init(STREAM_ID_0 );

	//
	//
	// Create rx stream
	stream_create( STREAM_ID_1, buffer_size, RX, memory, port );
	stream_init(STREAM_ID_1 );



	XTime_GetTime(&timer_start);

	//
	//
	// Send data
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	for(int i = 0; i < buffer_size; i++)
	{
		simple_write( STREAM_ID_0, i );
	}

	XTime_GetTime(&timer_end);

	printf("cycles to write %u %s %s bytes: %lu\n",buffer_size*buffer_size*buffer_size*sizeof(uint64_t),port_type_to_str(port),mem_type_to_str(memory),timer_end-timer_start);
	XTime_GetTime(&timer_start);
	//
	//
	// Read data and confirm
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	for(int i = 0; i < buffer_size; i++)
	{
		simple_read( STREAM_ID_1 );
	}
	XTime_GetTime(&timer_end);
	printf("cycles to write %u %s %s bytes: %lu\n",buffer_size*buffer_size*buffer_size*sizeof(uint64_t),port_type_to_str(port),mem_type_to_str(memory),timer_end-timer_start);

	//
	//
	// destroy streams
	stream_destroy( STREAM_ID_0 );
	stream_destroy( STREAM_ID_1 );

	//
	//
	// print results
	//printf("Metrics %d\n",Metrics);

	return XST_SUCCESS;
}
