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

#define ACTIVE_CORES 3

void delay(int delay)
{
	for(int i = 0; i < delay; i++)
	{
		// spin
	}
}


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
		if(*ptr == 0xdeadbeef || ((*ptr % ACTIVE_CORES) == 0) )
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
	while(*ptr != ACTIVE_CORES)
	{
		usleep(1);
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
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	for(int i = 0; i < buffer_size; i++)
	{
		simple_write( STREAM_ID_0, i );
	}

	XTime_GetTime(&timer_end);

	printf("cycles to write %u %s bytes: %lu\n",buffer_size * buffer_size*buffer_size*buffer_size*sizeof(uint64_t),mem_type_to_str(memory),timer_end-timer_start);
	float cycles = timer_end-timer_start;
	float bytes = buffer_size * buffer_size *buffer_size * buffer_size * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \n",tput);


	XTime_GetTime(&timer_start);
	//
	//
	// Read data and confirm
	for(int k = 0; k < buffer_size; k++)
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	for(int i = 0; i < buffer_size; i++)
	{
		simple_read( STREAM_ID_1 );
	}

	XTime_GetTime(&timer_end);


	printf("cycles to read %u %s bytes: %lu\n",buffer_size *buffer_size*buffer_size*buffer_size*sizeof(uint64_t),mem_type_to_str(memory),timer_end-timer_start);
	cycles = timer_end-timer_start;
	bytes = buffer_size * buffer_size *buffer_size * buffer_size * sizeof(uint64_t);
	tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \r\n\n",tput);

	//
	//
	// destroy streams
	stream_destroy( STREAM_ID_0 );
	stream_destroy( STREAM_ID_1 );

	return XST_SUCCESS;
}


int run_benchmark_ps_ps_flow( int buffer_size, memory_type memory, int time, axi_port_type port )
{
	//
	//
	clear_streams();

	//
	//
	// Create tx Stream
	stream_create( STREAM_ID_0, buffer_size, RX, memory, port );
	stream_init(STREAM_ID_0 );

	setup_mutex();

	synchronize();

	int expected = 0;
	//
	//
	// Send data
	for(int w = 0; w < buffer_size; w++)
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	for(int i = 0; i < buffer_size; i++)
	{
		uint32_t data = circular_read( STREAM_ID_0 );
		if(expected != data)
		{
			printf("Expected: %d received: %d \n",expected,data);
			print_state(STREAM_ID_0);
			assert(expected == data);
		}
		expected++;
	}

	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	volatile XTime* ptr = (0xFFFE0008);
	timer_start = *ptr;
	float cycles = timer_end-timer_start;
	float bytes = buffer_size *buffer_size * buffer_size * buffer_size * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("PS PS Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	stream_destroy( STREAM_ID_0 );

	synchronize();

	return XST_SUCCESS;
}

int run_benchmark_ps_pl_flow( int buffer_size, memory_type memory, int time, axi_port_type port )
{
	//
	//
	clear_streams();


	//
	//
	// Create rx stream
	stream_create( STREAM_ID_3, buffer_size, RX, memory, port );
	stream_init(STREAM_ID_3 );
//	print_state(STREAM_ID_1);
	start_stream( STREAM_ID_3 );

	setup_mutex();

	synchronize();

	uint64_t expected = 0;
	int end = (buffer_size*buffer_size*buffer_size*buffer_size)-80;
	//
	//
	// Send data
	for(int w = 0; w < buffer_size; w++)
	for(int k = 0; k < buffer_size; k++)
	for(int j = 0; j < buffer_size; j++)
	for(int i = 0; i < buffer_size; i++)
	{
		    //print_state(STREAM_ID_1);
		    //print_state(STREAM_ID_1);
		    uint64_t data = circular_read( STREAM_ID_3 );
			//printf("core-0 read %d\n",data);
//			print_state(STREAM_ID_3);
			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				print_state(STREAM_ID_1);
				assert(expected == data);
			}
			expected++;
//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}
			if(expected >= end ){
				break;
			}
	}

	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	volatile XTime* ptr = (0xFFFE0008);
	timer_start = *ptr;
	float cycles = timer_end-timer_start;
	float bytes = expected * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("PL PS Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	stream_destroy( STREAM_ID_3 );

	synchronize();

	return XST_SUCCESS;
}

