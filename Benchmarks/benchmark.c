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
#include "assert.h"
#include "xmutex.h"




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

XMutex Mutex[4];	/* Mutex instance */
#define MUTEX_NUM 0

int setup_mutex()
{
	XMutex_Config *ConfigPtr;
	XStatus Status;
	u32 TimeoutCount = 0;

	u16 MutexDeviceID = 0;
	if(XPAR_CPU_ID == 0)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_0_DEVICE_ID;
	}
	if(XPAR_CPU_ID == 1)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_1_DEVICE_ID;
	}
	if(XPAR_CPU_ID == 2)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_2_DEVICE_ID;
	}
	if(XPAR_CPU_ID == 3)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_3_DEVICE_ID;
	}


	/*
	 * Lookup configuration data in the device configuration table.
	 * Use this configuration info down below when initializing this
	 * driver instance.
	 */
	ConfigPtr = XMutex_LookupConfig(MutexDeviceID);
	if (ConfigPtr == (XMutex_Config *)NULL) {
		return XST_FAILURE;
	}

	/*
	 * Perform the rest of the initialization.
	 */
	Status = XMutex_CfgInitialize(&Mutex[XPAR_CPU_ID], ConfigPtr,
					ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}

void synchronize()
{
	volatile int* ptr = (0xFFFE0000);
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	while(1)
	{
		XMutex_Lock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Acquire lock */

		// default value of OCM is set to 0xdeadbeef so first processor to grab lock can clear
		if(*ptr == 0xdeadbeef)
		{
			*ptr = 0;
			*ptr = *ptr + 1;
			XMutex_Unlock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Release lock */
			break;
		}
		else
		{
			*ptr = *ptr + 1;
			XMutex_Unlock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Release lock */
			break;
		}
	}
	while(*ptr != 2)
	{
		usleep(200);
	}
}

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


	for(int k = 0; k < 2; k+=2)
	{
	printf("testing stream %d : %d \n",k,k+1);

		//
		//
		// Create tx Stream
		stream_create( k, buffer_size, TX, memory, port );
		stream_init(k);

	//
	//
	// Create rx stream
	stream_create( k+1, buffer_size, RX, memory, port );
	stream_init(k+1 );

	// start streams
	start_stream( k );
	start_stream( k+1 );

	//
	//
	// Send data

	XTime_GetTime(&timer_start);

	for(int i = 0; i < buffer_size; i++)
	{
		simple_write( k, i );
		usleep( time );

		printf("%s,%s,%u,%u,%u,%u\n",port_type_to_str(port),mem_type_to_str(memory),query_metric(0),query_metric(1),query_metric(2),query_metric(3));

	}
	for(int i = 0; i < buffer_size; i++)
	{
		printf("simple_read(%d)\n",simple_read( k+1 ));
//		sleep(1);
//		u32 data = simple_read( STREAM_ID_1 );
	}
//	simple_write( STREAM_ID_0, 0xdeadbeef);

	XTime_GetTime(&timer_end);
	printf("%s test run cycles %lu\n",mem_type_to_str(memory),timer_end-timer_start);
	}
	//
	//
	// Wait for data back
//	while(1)
//	{
//		//printf("sleeping\n");
//		usleep(500);
//
//		if(is_stream_done(STREAM_ID_1))
//			break;
//	}
//
//
//	//
//	//
//	// Read data and confirm
//	for(int i = 0; i < buffer_size; i++)
//	{
//		//printf("simple_read(%d)\n",simple_read( STREAM_ID_1 ));
////		sleep(1);
//		u32 data = simple_read( STREAM_ID_1 );
//	}

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

	printf("cycles to write %u %s bytes: %lu\n",buffer_size*buffer_size*buffer_size*sizeof(uint64_t),mem_type_to_str(memory),timer_end-timer_start);
	float cycles = timer_end-timer_start;
	float bytes = buffer_size *buffer_size * buffer_size * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \n",tput);


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
	printf("cycles to read %u %s bytes: %lu\n",buffer_size*buffer_size*buffer_size*sizeof(uint64_t),mem_type_to_str(memory),timer_end-timer_start);
	cycles = timer_end-timer_start;
	bytes = buffer_size *buffer_size * buffer_size * sizeof(uint64_t);
	tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \r\n\n",tput);
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


int run_benchmark_flow( int buffer_size, memory_type memory, int time, axi_port_type port )
{
	int Status;
	u32 Metrics;
	u32 ClkCntHigh = 0x0;
	u32 ClkCntLow = 0x0;
	clear_streams();

	//Status = Setup_AxiPmon(0);
	//if (Status != XST_SUCCESS) {
	//	xil_printf("AXI Performance Monitor Polled Failed To Start\r\n");
	//	return XST_FAILURE;
	//}

	//
	//
	// Create tx Stream
	//stream_create( STREAM_ID_0, buffer_size, TX, memory, port );
	//stream_init(STREAM_ID_0 );

	//
	//
	// Create rx stream
	stream_create( STREAM_ID_1, buffer_size, RX, memory, port );
	stream_init(STREAM_ID_1 );
	start_stream( STREAM_ID_1 );
	setup_mutex();

	synchronize();

//	start_stream( STREAM_ID_5 );
	int expected = 0;
	//
	//
	// Send data
	for(int w = 0; w < buffer_size; w++)
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	{
		for(int i = 0; i < buffer_size; i++)
		{
			uint32_t data = block_read( STREAM_ID_1);
			//printf("%s,%s,%u,%u,%u,%u,%u,%u\n",port_type_to_str(port),mem_type_to_str(memory),query_metric(0),query_metric(1),query_metric(2),query_metric(3),query_metric(4),query_metric(5));
			//printf("is done %d\n",is_stream_done( STREAM_ID_5 ));
			//printf("data received from read: %d \n",data);
			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				assert(expected == data);
			}
			expected++;
		}
	}
	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	volatile XTime* ptr = (0xFFFE0008);
	timer_start = *ptr;
	float cycles = timer_end-timer_start;
	float bytes = buffer_size *buffer_size * buffer_size * buffer_size * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	stream_destroy( STREAM_ID_1 );
//	stream_destroy( STREAM_ID_3 );

	//
	//
	// print results
	//Status = Shutdown_AxiPmon(&Metrics,&ClkCntHigh,&ClkCntLow);
	//if (Status != XST_SUCCESS) {
	//	xil_printf("AXI Performance Monitor Polled Failed To Shutdown\r\n");
	//	return XST_FAILURE;
	//}

	return XST_SUCCESS;
}

int run_benchmark_flow2( int buffer_size, memory_type memory, int time, axi_port_type port )
{
	int Status;
	u32 Metrics;
	u32 ClkCntHigh = 0x0;
	u32 ClkCntLow = 0x0;
	clear_streams();


	uint64_t test[200];

	for(int i = 0; i < 200; i++)
	{
		test[i] = 0;
	}

	//Status = Setup_AxiPmon(0);
	//if (Status != XST_SUCCESS) {
	//	xil_printf("AXI Performance Monitor Polled Failed To Start\r\n");
	//	return XST_FAILURE;
	//}

	//
	//
	// Create tx Stream
	//stream_create( STREAM_ID_0, buffer_size, TX, memory, port );
	//stream_init(STREAM_ID_0 );

	//
	//
	// Create rx stream
	stream_create( STREAM_ID_0, buffer_size, RX, memory, port );
	stream_init(STREAM_ID_0 );
	start_stream( STREAM_ID_0 );
	setup_mutex();

	synchronize();

	int bytes_read = 0;
//	while(bytes_read != 200){
//		//int nbytes = burst_read( STREAM_ID_0, &test[bytes_read]);
////		printf("bytes read %d\n",nbytes);
////		bytes_read+=nbytes;
//	}

//	start_stream( STREAM_ID_5 );
	int expected = 0;
	//
	//
	// Send data
	for(int w = 0; w < buffer_size; w++)
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	{
		for(int i = 0; i < buffer_size; i++)
		{

			uint32_t data = block_read2( STREAM_ID_0);
			//printf("%s,%s,%u,%u,%u,%u,%u,%u\n",port_type_to_str(port),mem_type_to_str(memory),query_metric(0),query_metric(1),query_metric(2),query_metric(3),query_metric(4),query_metric(5));
			//printf("is done %d\n",is_stream_done( STREAM_ID_5 ));
			//printf("data received from read: %d \n",data);
			if(expected != data)
			{
//				volatile XTime* ptr1 = (0xFFFC0318);
//				volatile XTime* ptr2 = (0xFFFC0310);
//				volatile XTime* ptr3 = (0xFFFC0308);
//				printf("head : %u tail %u full %u \n",*ptr1,*ptr2,*ptr3);
				printf("Expected: %d received: %d \n",expected,data);
//				printf("head : %u tail %u full %u \n",*ptr1,*ptr2,*ptr3);
				assert(expected == data);
			}
			expected++;
//			usleep(100);
		}
	}
	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	volatile XTime* ptr = (0xFFFE0008);
	timer_start = *ptr;
	float cycles = timer_end-timer_start;
	float bytes = buffer_size* buffer_size *buffer_size * buffer_size * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	stream_destroy( STREAM_ID_0 );
//	stream_destroy( STREAM_ID_3 );

	//
	//
	// print results
	//Status = Shutdown_AxiPmon(&Metrics,&ClkCntHigh,&ClkCntLow);
	//if (Status != XST_SUCCESS) {
	//	xil_printf("AXI Performance Monitor Polled Failed To Shutdown\r\n");
	//	return XST_FAILURE;
	//}

	return XST_SUCCESS;
}

