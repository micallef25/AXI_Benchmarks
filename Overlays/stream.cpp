#include <stdint.h>
#include "stream.h"
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <stdio.h>

#include "xexample_tx.h"
#include "xexample_rx.h"
#include "xil_cache.h"
#include "xil_mmu.h"
#include "xpseudo_asm.h"
#include "xil_types.h"
#include "xparameters.h"

// TODO first create the generic API's showing that it works
// then we can complicate and do per port design
//
//
#define NUM_INSTANCES 1

// TODO figure this out 
// typedef read
// typedef write


// func_ptrs ptr_table[NUM_INSTANCES] = 
// {
// 	{
// 		.read = simple_read;
// 		.write = simple_write;
// 	}
// };


// InterruptHandler()
// {

// }

/*
* useful conversions
*
*/
stream::axi_port_type stream::port_str_to_type( char* str )
{
	if( !str )
		return INVALID_PORT;

	for( int n=0; n < MAX_NUM_PORTS; n++ )
	{
		if( strcasecmp(str, port_type_to_str((axi_port_type)n)) == 0 )
			return (axi_port_type)n;
	}

	return INVALID_PORT;
}

const char* stream::port_type_to_str( axi_port_type port )
{
	switch(port)
	{
		case HPC0:	return "HPC0";
		case HPC1:	return "HPC1";
		case HP0:	return "HP0";
		case HP1:	return "HP1";
		case HP2:	return "HP2";
		case HP3:	return "HP3";
		default: return "ERROR";
	}
}


stream::memory_type stream::mem_str_to_type( char* str )
{
	if( !str )
		return INVALID_MEMORY;

	for( int n=0; n < MAX_NUM_MEMORIES; n++ )
	{
		if( strcasecmp(str, mem_type_to_str((memory_type)n)) == 0 )
			return (memory_type)n;
	}

	return INVALID_MEMORY;
}

const char* stream::mem_type_to_str( memory_type memory )
{
	switch(memory)
	{
		case DDR:	return "DDR";
		case OCM:	return "OCM";
		case CACHE:	return "CACHE";
		default: return "ERROR";
	}
}





// should consider making ports a singleton paradigm and streams contain ports
stream::stream( uint32_t buff_size )
{
	this->ptr = 0;
	this->buff = (volatile uint32_t*)malloc( sizeof(uint32_t)*buff_size );
	this->axi_config_tx = (XExample_tx*)malloc(sizeof(XExample_tx));
	this->axi_config_rx = (XExample_rx*)malloc(sizeof(XExample_rx));
	this->buff_size = buff_size;
	assert(this->buff != NULL && this->axi_config_tx != NULL && this->axi_config_rx != NULL);
}

stream::~stream()
{
	// actually only need one buffer
	free((void*)this->buff);
	free((void*)this->axi_config_tx);
	free((void*)this->axi_config_rx);
	this->buff = NULL;
}

// pass in which port you want and who is master and slave for this stream
int stream::init_tx( axi_port_type port_type, uint32_t ps_id )
{
	//some set up may be for just running in software 
	// need to check output if the CPU needs to register this in the TLB
	// may need to create alot of memory up front so that in PS to PS we can share pointers
	ps_id = XPAR_EXAMPLE_TX_0_DEVICE_ID;
	int rval = XExample_tx_Initialize( axi_config_tx, ps_id);
    if(rval != XST_SUCCESS)
    {
       	xil_printf("error in init\r\n");
       	return XST_FAILURE;
    }

    // comppilers are stupid and this is the only way I can get rid of the error without losing precision when passed to set input
    u32 offset = *((u32*)(&buff));

	// point FPGA to buffer
    XExample_tx_Set_input_r( axi_config_tx,offset );

	// tell it to restart immediately
	// Xexample_autorestart()

	//
	// SetupInterrupts()

	// slave 3 from CCI man page. This enables snooping from the HPC0 and 1 ports
	if( port_type == HPC0 || port_type == HPC1 )
	{
    	Xil_Out32(0xFD6E4000,0x1);
    	dmb();
    }

    // mark our memory regions as outer shareable which means it will not live in L1 but L2
    // TODO make this a parameter for user to pass in or atleast a macro
    Xil_SetTlbAttributes((UINTPTR)buff, 0x605);
    dmb();

    //XExample_tx_Start(axi_config_tx);

    return XST_SUCCESS;
}

// pass in which port you want and who is master and slave for this stream
int stream::init_rx( axi_port_type port_type, uint32_t ps_id )
{
	//some set up may be for just running in software
	// need to check output if the CPU needs to register this in the TLB
	// may need to create alot of memory up front so that in PS to PS we can share pointers
	ps_id = XPAR_EXAMPLE_TX_0_DEVICE_ID;
	int rval = XExample_rx_Initialize( axi_config_rx, ps_id);
    if(rval != XST_SUCCESS)
    {
       	xil_printf("error in init\r\n");
       	return XST_FAILURE;
    }

    // comppilers are stupid and this is the only way I can get rid of the error without losing precision when passed to set input
    u32 offset = *((u32*)(&buff));

	// point FPGA to buffer
    XExample_rx_Set_outbuff( axi_config_rx,offset );

	// tell it to restart immediately
	// Xexample_autorestart()

	//
	// SetupInterrupts()

	// slave 3 from CCI man page. This enables snooping from the HPC0 and 1 ports
	if( port_type == HPC0 || port_type == HPC1 )
	{
    	Xil_Out32(0xFD6E4000,0x1);
    	dmb();
    }

    // mark our memory regions as outer shareable which means it will not live in L1 but L2
    // TODO make this a parameter for user to pass in or atleast a macro
    Xil_SetTlbAttributes((UINTPTR)buff, 0x605);
    dmb();

    //XExample_rx_Start(axi_config_rx);

    return XST_SUCCESS;
}



/*
* Simple read write with no handshaking
*/
void stream::simple_write( uint32_t data )
{
	buff[ptr] = data;
	ptr++;
	ptr = ptr % buff_size;
}


uint32_t stream::simple_read()
{
	uint32_t temp;
	temp = buff[ptr];
	ptr++;
	ptr = ptr % buff_size;
	return temp;
}


u32 stream::is_stream_done()
{
	return XExample_rx_IsDone(axi_config_rx);
}

void stream::start_tx()
{
//	XExample_rx_Start(axi_config_rx);
	XExample_tx_Start(axi_config_tx);
}

void stream::start_rx()
{
	XExample_rx_Start(axi_config_rx);
//	XExample_tx_Start(axi_config_tx);
}

//// could potentially make a table of pointers that is unique to each port
////
//uint32_t stream::msg_read()
//{
//	// spin until a new message has arrived
//	while(1)
//	{
//
//		volatile uint64_t data = buffer[ptr];
//
//		// msg is not ready, delay
//		if(data == msg_counter)
//		{
//			for(int i = 0; i < DELAY; i++)
//			{
//				//delay this could be optmized out with a better compiler whic we want to avoid
//			}
//		}
//		else
//		{
//			ptr++;
//			ptr = ptr % BUFF_SIZE;
//			return (data & ~msg_counter);
//		}
//	}
//}
//
//void stream::msg_write( uint32_t data )
//{
//	// append out message counter and write to memory
//	buffer[ptr] = data | msg_counter;
//	ptr++;
//	ptr = ptr % BUFF_SIZE;
//	// need to handle waiting to make sure sure that data is not overwritten
//}

// /*
// * wrapper functions will point to other functions
// *
// */ 
// // change uint32_t to a typedef
// // a wrapper function to our write function. allows easy experimentation with writes
// void stream::write( uint32_t data )
// {
// 	assert(id != INVALID_PORT);
// 	ptr_table[id]->write(data);
// }

// uint32_t stream::read()
// {
// 	assert(id != INVALID_PORT);
// 	return ptr_table[id]->read();
// }
