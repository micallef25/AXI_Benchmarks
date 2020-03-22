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

// my stuff
#include "../Overlays/stream.h"
#include "assert.h"
#include "xmutex.h"

#define ACTIVE_CORES 4

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
	volatile int* ptr = (volatile int*)(0xFFFE0000);
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


int run_benchmark_memory( stream::memory_t memory, stream::axi_port_t port )
{
	int Status;
    XTime timer_start=0;
    XTime timer_end=0;
    XTime_StartTimer();

    int loops = 100;

	//
	//
	//printf("Format: Memory_Config,Total_Read_Latency,Bytes_Read... Running Test with buffer size %d and rest time of %d\n",buffer_size,time);

	//
	//
	// Create tx Stream
    stream Input_Stream( stream::stream_id_t::STREAM_ID_0, stream::direction_t::RX, memory, port );
	Input_Stream.stream_init();

	//
	//
	// Create rx stream
	stream Input_Stream2( stream::stream_id_t::STREAM_ID_1, stream::direction_t::RX, memory, port );
	Input_Stream2.stream_init();



	XTime_GetTime(&timer_start);

	//
	//
	// Send data
	for(int k = 0; k < loops; k++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.simple_write(i);
	}

	XTime_GetTime(&timer_end);

	printf("cycles to write %u %s bytes: %lu\n",loops*loops*loops*loops*sizeof(uint64_t),Input_Stream.mem_type_to_str(memory),timer_end-timer_start);
	float cycles = timer_end-timer_start;
	float bytes = loops * loops *loops * loops * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \n",tput);


	XTime_GetTime(&timer_start);
	//
	//
	// Read data and confirm
	for(int k = 0; k < loops; k++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream2.simple_read();
	}

	XTime_GetTime(&timer_end);


	printf("cycles to write %u %s bytes: %lu\n",loops*loops*loops*loops*sizeof(uint64_t),Input_Stream2.mem_type_to_str(memory),timer_end-timer_start);
	cycles = timer_end-timer_start;
	bytes = loops * loops *loops * loops * sizeof(uint64_t);
	tput = (bytes/cycles) * 1.2;
	printf("Throughput ~ %f GBps \r\n\n",tput);

	//
	//
	// destroy streams
	Input_Stream.~stream();
	Input_Stream2.~stream();

	return XST_SUCCESS;
}


int run_benchmark_ps_ps_flow( stream::memory_t memory, stream::axi_port_t port )
{
	int loops = 100;


	//
	//
	// Create rx stream
	stream Stream( stream::stream_id_t::STREAM_ID_0, stream::direction_t::RX, memory, port );
	Stream.stream_init();

	setup_mutex();

	synchronize();

	uint64_t expected = 0;
	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    uint64_t data = Stream.read();

			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				Stream.print_state();
				assert(expected == data);
			}
			expected++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	volatile XTime* ptr = (volatile XTime*)(0xFFFE0008);
	timer_start = *ptr;
	float cycles = timer_end-timer_start;
	float bytes = expected * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("PL PS Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	Stream.~stream();

	synchronize();

	return XST_SUCCESS;
}

int run_benchmark_ps_pl_flow( stream::memory_t memory, stream::axi_port_t port )
{
	int loops = 100;


	//
	//
	// Create rx stream
    stream Stream( stream::stream_id_t::STREAM_ID_5, stream::direction_t::RX, memory, port );
	Stream.stream_init();

	// starts the PL IP associated
	Stream.start_stream();

	setup_mutex();

	synchronize();

	uint64_t expected = 0;
	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    uint64_t data = Stream.read();

			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				Stream.print_state();
				assert(expected == data);
			}
			expected++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	volatile XTime* ptr = (volatile XTime*)(0xFFFE0008);
	timer_start = *ptr;
	float cycles = timer_end-timer_start;
	float bytes = expected * sizeof(uint64_t);
	float tput = (bytes/cycles) * 1.2;
	printf("PL PS Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	Stream.~stream();

	synchronize();

	return XST_SUCCESS;
}

