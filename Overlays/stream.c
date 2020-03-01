#include <stdint.h>
#include "stream.h"
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <stdio.h>
#include "sleep.h"

#define IP_32
#ifdef IP_32
#include "xexample_tx_piped64.h"
#include "xexample_rx_piped64.h"
#else
#include "xexample_tx_128.h"
#include "xexample_rx_128.h"
#endif

#include "xil_cache.h"
#include "xil_mmu.h"
#include "xpseudo_asm.h"
#include "xil_types.h"
#include "xparameters.h"

#include "stream.h"


#define MAX_NUM_PORTS INVALID_PORT
#define MAX_NUM_MEMORIES INVALID_MEMORY
#define NUM_CORES 4

#define MAX_OCM_PARTITONS 6
#define OCM_BUFF1 (0xFFFC0000)
#define OCM_BUFF2 OCM_BUFF1 + (200*8)
#define OCM_BUFF3 OCM_BUFF2 + (200*8)
#define OCM_BUFF4 OCM_BUFF3 + (200*8)
#define OCM_BUFF5 OCM_BUFF4 + (200*8)
#define OCM_BUFF6 OCM_BUFF5 + (200*8)

// pointer to stream instances
static stream_t* streams[NUM_CORES][MAX_NUMBER_OF_STREAMS];

// can partition our ocm #TODO change this later
static uint32_t ocm_buff[MAX_NUMBER_OF_STREAMS] = {
		OCM_BUFF1,
		OCM_BUFF2,
		OCM_BUFF3,
		OCM_BUFF4,
		OCM_BUFF5,
		OCM_BUFF6
};


static uint8_t id_array[MAX_NUMBER_OF_STREAMS] = {
		XPAR_EXAMPLE_TX_PIPED64_0_DEVICE_ID, // stream 0 transmits to pl on device 0
		XPAR_EXAMPLE_RX_PIPED64_0_DEVICE_ID, // stream 1 reads from PL on device 0
		//XPAR_EXAMPLE_TX_P64_1_DEVICE_ID, // stream 2 transmits to pl on device 1
		//XPAR_EXAMPLE_RX_P64_1_DEVICE_ID, // stream 3 reads from pl on device 1
		//XPAR_EXAMPLE_TX_P64_2_DEVICE_ID, // stream 4 reads from pl on device 2
		//XPAR_EXAMPLE_RX_P64_2_DEVICE_ID, // stream 5 reads from pl on device 2
};

//static uint8_t rx_id_array[MAX_NUMBER_OF_STREAMS] = {
//		-1,
//		XPAR_EXAMPLE_RX_P64_0_DEVICE_ID, // stream 1 reads from PL on device 0
//
//		XPAR_EXAMPLE_RX_P64_1_DEVICE_ID,  // stream 3 reads from pl on device 1
//		XPAR_EXAMPLE_RX_P64_2_DEVICE_ID, // stream 5 reads from pl on device 2
//
//};
//static uint8_t tx_id_array[MAX_NUMBER_OF_STREAMS] = {
//		XPAR_EXAMPLE_TX_P64_0_DEVICE_ID, // stream 0 transmits to pl on device 0
//		XPAR_EXAMPLE_TX_P64_1_DEVICE_ID, // stream 2 transmits to pl on device 1
//		XPAR_EXAMPLE_TX_P64_2_DEVICE_ID
//
//};

#define PRESENCE_BIT ( 0x100000000 ) // 1 << 32
#define MASK32 (0xFFFFFFFF) // (1 << 32) - 1

//volatile uint32_t* ocm_buff_tx = (int*)OCM_BUFF1;
//volatile uint32_t* ocm_buff_rx = (int*)OCM_BUFF2;


void clear_streams()
{
	for(int i = 0; i < MAX_NUMBER_OF_STREAMS; i++)
	{
		streams[XPAR_CPU_ID][i] = NULL;
	}
}


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


static stream_t* safe_get_stream( stream_id_type stream_id )
{
	// bounds check
	assert(stream_id >= 0 && stream_id < MAX_NUMBER_OF_STREAMS);

	//
	stream_t* stream = streams[XPAR_CPU_ID][stream_id];

	return stream;
}


// should consider making ports a singleton paradigm and streams contain ports
int  stream_create( stream_id_type stream_id, uint32_t buff_size, direction_type direction, memory_type mem, axi_port_type port  )
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
	if( mem == CACHE || mem == DDR )
	{
		stream->buff = (volatile uint64_t*)malloc( sizeof(uint64_t)*buff_size );
		assert(stream->buff != NULL);
	}
	else if( mem == OCM )
	{
		stream->buff = ocm_buff[stream_id];
	}

	stream->X_ID = id_array[stream_id];
	stream->head = 0;
	stream->tail = 0;
	stream->full = 0;
	memset(stream->buff,0,buff_size*sizeof(uint64_t));

	// depending on the direction get the correct IP instance
	if( direction == TX )
	{
#ifdef IP_32
		stream->axi_config_tx = (XExample_tx_piped64*)malloc(sizeof(XExample_tx_piped64));
#else
		stream->axi_config_tx = (XExample_tx_128*)malloc(sizeof(XExample_tx_128));
#endif
		stream->axi_config_rx = NULL;
		assert(stream->axi_config_tx != NULL);
	}
	else if( direction == RX )
	{
		stream->axi_config_tx = NULL;
#ifdef IP_32
		stream->axi_config_rx = (XExample_rx_piped64*)malloc(sizeof(XExample_rx_piped64));
#else
		stream->axi_config_rx = (XExample_rx_128*)malloc(sizeof(XExample_rx_128));
#endif
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
	stream->buff_size = buff_size-3;
	stream->direction = direction;
	streams[XPAR_CPU_ID][stream_id] = stream;

	return XST_SUCCESS;
}

void stream_destroy( stream_id_type stream_id)
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	// OCM is not heap managed
	if(stream->memory != OCM)
		free((void*)stream->buff);

	stream->buff = NULL;

	// depending on the direction get the correct IP instance
	if( stream->direction == TX )
	{
		free((void*)stream->axi_config_tx);
		stream->axi_config_tx = NULL;
	}
	else if( stream->direction == RX )
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
	streams[XPAR_CPU_ID][stream_id] = NULL;
}

// pass in which port you want and who is master and slave for this stream
int stream_init( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	int rval = XST_FAILURE;

	// initialize our drivers
	if(stream->direction == TX )
	{
#ifdef IP_32
		rval = XExample_tx_piped64_Initialize( stream->axi_config_tx, stream->X_ID );
		XExample_tx_piped64_EnableAutoRestart(stream->axi_config_tx);
#else
		rval = XExample_tx_128_Initialize( stream->axi_config_tx, stream->X_ID );
#endif
	}
	else if(stream->direction == RX)
	{
#ifdef IP_32
		rval = XExample_rx_piped64_Initialize( stream->axi_config_rx, stream->X_ID );
		XExample_rx_piped64_EnableAutoRestart(stream->axi_config_rx);
#else
		rval = XExample_rx_128_Initialize( stream->axi_config_rx, stream->X_ID );
#endif
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
#ifdef IP_32
		XExample_tx_piped64_Set_input_r( stream->axi_config_tx,(u32)stream->buff );
#else
		XExample_tx_128_Set_input_data( stream->axi_config_tx,(u32)stream->buff );
#endif
	}
	else if(stream->direction == RX)
	{
#ifdef IP_32
		XExample_rx_piped64_Set_output_r( stream->axi_config_rx,(u32)stream->buff );
#else
		XExample_rx_128_Set_output_data( stream->axi_config_rx,(u32)stream->buff );
#endif
	}

	// tell it to restart immediately
	// Xexample_autorestart()

	//
	// SetupInterrupts()

	// slave 3 from CCI man page. This enables snooping from the HPC0 and 1 ports
	if( stream->memory == DDR || stream->memory == OCM )
	{
    	Xil_Out32(0xFD6E4000,0x0);
    	dmb();
		Xil_SetTlbAttributes((UINTPTR)stream->buff, NORM_NONCACHE);
		dmb();
    }

    // mark our memory regions as outer shareable which means it will not live in L1 but L2
    // TODO make this a parameter for user to pass in or atleast a macro
	if( stream->memory == CACHE /*|| stream->memory == OCM */)
	{
    	Xil_Out32(0xFD6E4000,0x1);
    	dmb();
		Xil_SetTlbAttributes((UINTPTR)stream->buff, 0x605);
		dmb();
	}
    return XST_SUCCESS;
}




/*
* Simple read write with no handshaking
*/
void simple_write( stream_id_type stream_id, uint32_t data )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	stream->buff[stream->ptr] = data;
	stream->ptr++;
	stream->ptr = stream->ptr % stream->buff_size;
//	Xil_DCacheFlushRange((INTPTR)stream->buff, stream->buff_size);
}


uint32_t simple_read( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	uint32_t temp;
	temp = stream->buff[stream->ptr];
	stream->ptr++;
	stream->ptr = stream->ptr % stream->buff_size;
//	Xil_DCacheFlushRange((INTPTR)stream->ptr, stream->buff_size);

	//
	return temp;
}


uint32_t is_stream_done( stream_id_type stream_id )
{
		//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	if(stream->direction == TX )
	{
#ifdef IP_32
		return XExample_tx_piped64_IsDone(stream->axi_config_tx);
#else
		return XExample_tx_128_IsDone(stream->axi_config_tx);
#endif
	}
	else if(stream->direction == RX)
	{
#ifdef IP_32
		return XExample_rx_piped64_IsDone(stream->axi_config_rx);
#else
		return XExample_rx_128_IsDone(stream->axi_config_rx);
#endif
	}
	return 0;
}

void start_stream( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	if(stream->direction == TX )
	{
#ifdef IP_32
		XExample_tx_piped64_Start(stream->axi_config_tx);
#else
		XExample_tx_128_Start(stream->axi_config_tx);
#endif
	}
	else if(stream->direction == RX)
	{
#ifdef IP_32
		XExample_rx_piped64_Start(stream->axi_config_rx);
#else
		XExample_rx_128_Start(stream->axi_config_rx);
#endif
	}

}


/*
* Simple read write with no handshaking
*/
void block_write( stream_id_type stream_id, uint32_t data )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	while(1)
	{
		uint64_t temp = stream->buff[stream->ptr];
		
		// bit is high meaning that the data is not consumed yet
		if( (temp & PRESENCE_BIT) == PRESENCE_BIT )
		{
			// wait some time
//			usleep(5);
		}
		else
		{
			// set the bit and write the data
			stream->buff[stream->ptr] = data | PRESENCE_BIT;
			stream->ptr++;
			stream->ptr = stream->ptr % stream->buff_size;
			break;
		}
	}
}


uint32_t block_read( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	int timeout = 0;

	while(1)
	{
		// read data
		uint64_t temp = stream->buff[stream->ptr];
		
		// bit is high meaning that the data is ready to be consumed
		if( (temp & PRESENCE_BIT) == PRESENCE_BIT )
		{
			// clear the bit
			//			stream->buff[stream->ptr] &= (~PRESENCE_BIT);
			stream->buff[stream->ptr] = 0;
			stream->ptr++;
			stream->ptr = stream->ptr % stream->buff_size;
			return (temp & MASK32);
		}
		else
		{
			// wait some time
//			usleep(5);
			timeout++;
			if(timeout == 2000)
				break;
		}
	}

	//
	return -1;
}

/*
* Simple read write with no handshaking
*/
void block_write2( stream_id_type stream_id, uint32_t data )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	uint16_t delay=0;

	// read tail pointer
	volatile uint64_t temp_tail = stream->buff[TAIL_POINTER];
	stream->full = stream->buff[FULL_BIT]; //
	while(1)
	{
		// read tail pointer
		//uint64_t temp_tail = stream->buff[TAIL_POINTER];

		if( ((stream->head + 1  == temp_tail && stream->full == 0)|| stream->full == 1) /*|| (temp_tail == 0 && stream->full == 1)*/ )
		{
			// can not write so we refresh our full state if we can not write
			stream->full = stream->buff[FULL_BIT]; //
			temp_tail = stream->buff[TAIL_POINTER];
        }
        else
        {
        	//if(stream->head == temp_tail) stream->full = 1;

        	// write and update local pointer
        	stream->buff[stream->head] = data;
            stream->head++;  //increment the head
            stream->head = stream->head % stream->buff_size;

            // check for fullness fullness is being set prematurely
            // when the wrap around occurs the read is waiting but the full bit is set then and the read occurs
            if(stream->head == temp_tail) stream->full = 1;


//            printf("head: %d tail: %d full: %d\n",stream->head,temp_tail,stream->full);

            // update state
            stream->buff[HEAD_POINTER] = stream->head;
            if(stream->full == 1)
            {
            	stream->buff[FULL_BIT] = 1;
            }

            break;
        }
	}
}

// data is written to head and read from tail!!!
uint32_t block_read2( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	int timeout = 0;
	uint32_t data = 0;
//	uint64_t full=0;

	// read head pointer
	volatile uint64_t temp_head = stream->buff[HEAD_POINTER];
	stream->full = stream->buff[FULL_BIT];// refresh our full status
	while(1)
	{
//		// read head pointer
//		uint64_t temp_head = stream->buff[HEAD_POINTER];
//		stream->full = stream->buff[FULL_BIT];// refresh our full status
		// not allowed to read when the pointers are equal and the full bit is zero
		//
		if( (stream->tail == temp_head && stream->full==0)  )
		{
			stream->full = stream->buff[FULL_BIT];// refresh our full status
			temp_head = stream->buff[HEAD_POINTER];
        }
		// head pointer is consumer and he has written something go get it
		else
		{

            data = stream->buff[stream->tail];  //grab a byte from the buffer
            stream->tail++;  //incrementthe tail
            stream->tail = stream->tail % stream->buff_size;

            //printf("head: %d tail: %d full: %d\n",temp_head,stream->tail,stream->buff[FULL_BIT]);
            // write our new updated position
            stream->buff[TAIL_POINTER] = stream->tail;
            if(stream->full == 1)
            {
         	   stream->buff[FULL_BIT] = 0;
         	   //stream->full=0;
         	   //printf("buff full \n");
            }

            //stream->full=0;

            return data;

		}
	}

	//
	return -1;
}

