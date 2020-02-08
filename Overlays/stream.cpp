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


#define MAX_NUM_PORTS INVALID_PORT
#define MAX_NUM_MEMORIES INVALID_MEMORY



// pointer to stream instances
static stream_t* streams[MAX_NUMBER_OF_STREAMS];



/*
* useful conversions
*
*/
axi_port_type port_str_to_type( char* str )
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

const char* port_type_to_str( axi_port_type port )
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


memory_type mem_str_to_type( char* str )
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

const char* mem_type_to_str( memory_type memory )
{
	switch(memory)
	{
		case DDR:	return "DDR";
		case OCM:	return "OCM";
		case CACHE:	return "CACHE";
		default: return "ERROR";
	}
}


static stream_t* safe_get_stream( stream_id_t stream_id )
{
	// bounds check
	assert(stream_id >= 0 && stream_id < MAX_NUMBER_OF_STREAMS)

	//
	stream_t* stream streams[stream_id];

	return stream;
}


// should consider making ports a singleton paradigm and streams contain ports
stream_id_t stream_create( uint32_t buff_size, direction_type direction,memory_type mem, axi_port_type port  );
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should not be created already this cases a memory leak
	assert( stream == NULL );
	
	// create instance
	stream = (stream_t*)malloc( sizeof(stream_t) );
	assert(stream != NULL);

	// create buffer and set ptr
	stream->ptr = 0;
	stream->buff = (volatile uint32_t*)malloc( sizeof(uint32_t)*buff_size );

	// depending on the direction get the correct IP instance
	if( direction == TX )
	{
		stream->axi_config_tx = (XExample_tx*)malloc(sizeof(XExample_tx));
		stream->axi_config_rx = NULL;
		stream->X_ID = XPAR_EXAMPLE_TX_0_DEVICE_ID;
		assert(stream->axi_config_rx != NULL);
	}
	else if( direction == RX )
	{
		stream->axi_config_tx = NULL;
		stream->axi_config_rx = (XExample_rx*)malloc(sizeof(XExample_rx));
		stream->X_ID = XPAR_EXAMPLE_RX_0_DEVICE_ID;
		assert(stream->axi_config_rx != NULL);
	}
	else
	{
		// invalid arg crash the program
		assert(0);
	}


	// set the unique characteristics of the stream
	stream->memory = mem;
	stream->port = port;
	stream->buff_size = buff_size;
	stream->direction = direction;
}

void stream_destroy( stream_id_t stream_id)
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	// actually only need one buffer
	free((void*)stream->buff);
	stream->buff = NULL;

	// depending on the direction get the correct IP instance
	if( direction == TX )
	{
		free((void*)stream->axi_config_tx);
		stream->axi_config_tx = NULL;
	}
	else if( direction == RX )
	{
		free((void*)stream->axi_config_rx);
		stream->axi_config_rx = NULL;
	}
	else
	{
		// invalid arg crash the program
		assert(0);
	}

	// Finally free our stream and our table entry
	free(stream);
	streams[stream_id] = NULL;
}

// pass in which port you want and who is master and slave for this stream
int stream_init( stream_id_t stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	int rval = XST_FAILURE;

	// initialize our drivers
	if(stream->direction == TX )
	{
		rval = XExample_tx_Initialize( stream->axi_config_tx, stream->X_ID );
	}
	else if(stream->direction == RX)
	{
		rval = XExample_rx_Initialize( stream->axi_config_rx, stream->X_ID );
	}
	else
	{
		// invalid arg crash the program
		assert(0);
	}

	// ensure drivers are created successfully
    if(rval != XST_SUCCESS)
    {
       	xil_printf("error in init\r\n");
       	return XST_FAILURE;
    }


    // comppilers are stupid and this is the only way I can get rid of the error without losing precision when passed to set input
    // u32 offset = *((u32*)(&buff));

	// point FPGA to buffers
	if(stream->direction == TX )
	{
		XExample_tx_Set_input_r( axi_config_tx,(u32)stream->buff );
	}
	else if(stream->direction == RX)
	{
		XExample_rx_Set_output_r( axi_config_tx,(u32)stream->buff );
	}

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

    return XST_SUCCESS;
}




/*
* Simple read write with no handshaking
*/
void stream::simple_write( stream_id_t stream, uint32_t data )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	stream->buff[stream->ptr] = data;
	stream->ptr++;
	stream->ptr = stream->ptr % stream->buff_size;
}


uint32_t stream::simple_read( stream_t stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	uint32_t temp;
	temp = stream->buff[stream->ptr];
	stream->ptr++;
	stream->ptr = stream->ptr % stream->buff_size;
	return temp;
}


u32 is_stream_done(stream_id_t )
{
		//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	if(stream->direction == TX )
	{
		XExample_tx_Done(stream->axi_config_tx);
	}
	else if(stream->direction == RX)
	{
		XExample_rx_Done(stream->axi_config_rx);
	}
}

void start_stream( stream_id_t stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	if(stream->direction == TX )
	{
		XExample_tx_Start(stream->axi_config_tx);
	}
	else if(stream->direction == RX)
	{
		XExample_rx_Start(stream->axi_config_rx);
	}

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
