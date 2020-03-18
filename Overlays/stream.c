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
#define OCM_BUFF2 OCM_BUFF1 + (RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF3 OCM_BUFF2 + (RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF4 OCM_BUFF3 + (RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF5 OCM_BUFF4 + (RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF6 OCM_BUFF5 + (RAW_BUFFER_SIZE * sizeof(uint64_t))

// pointer to stream instances
static stream_t* streams[NUM_CORES][MAX_NUMBER_OF_STREAMS];

// can partition our ocm #TODO change this later
static volatile uint64_t* ocm_buff[MAX_NUMBER_OF_STREAMS] = {
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
		XPAR_EXAMPLE_TX_PIPED64_1_DEVICE_ID, // stream 2 transmits to pl on device 1
		XPAR_EXAMPLE_RX_PIPED64_1_DEVICE_ID, // stream 3 reads from pl on device 1
//		XPAR_EXAMPLE_TX_PIPED64_2_DEVICE_ID, // stream 4 reads from pl on device 2
//		XPAR_EXAMPLE_RX_PIPED64_2_DEVICE_ID, // stream 5 reads from pl on device 2
};

#define PRESENCE_BIT ( 0x100000000 ) // 1 << 32
#define MASK32 (0xFFFFFFFF) // (1 << 32) - 1


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
	if( mem == CACHE || mem == DDR )
	{
		stream->buff = (volatile uint64_t*)malloc( sizeof(uint64_t)*RAW_BUFFER_SIZE );
		assert(stream->buff != NULL);
	}
	else if( mem == OCM )
	{
		stream->buff = (volatile uint64_t*)ocm_buff[stream_id];
	}

	stream->X_ID = id_array[stream_id];


	memset(stream->buff,0,RAW_BUFFER_SIZE*sizeof(uint64_t));

	// depending on the direction get the correct IP instance
	if( direction == TX )
	{
		stream->axi_config_tx = (XExample_tx_piped64*)malloc(sizeof(XExample_tx_piped64));
		stream->axi_config_rx = NULL;
		assert(stream->axi_config_tx != NULL);
	}
	else if( direction == RX )
	{
		stream->axi_config_tx = NULL;
		stream->axi_config_rx = (XExample_rx_piped64*)malloc(sizeof(XExample_rx_piped64));
		assert(stream->axi_config_rx != NULL);
	}
	else
	{
		// invalid arg crash the program
		assert(0);
	}


	// set the unique characteristics of the stream
	stream->ptr = 0;
	stream->head = 0;
	stream->tail = 0;
	stream->memory = mem;
	stream->port = port;
	stream->buff_size = MASK;
	stream->direction = direction;
	stream->bytes_written = 0;
	stream->bytes_read = 0;
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
		rval = XExample_tx_piped64_Initialize( stream->axi_config_tx, stream->X_ID );
		usleep(5);
		XExample_tx_piped64_EnableAutoRestart(stream->axi_config_tx);
	}
	else if(stream->direction == RX)
	{
		rval = XExample_rx_piped64_Initialize( stream->axi_config_rx, stream->X_ID );
		usleep(5);
		XExample_rx_piped64_EnableAutoRestart(stream->axi_config_rx);
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
		XExample_tx_piped64_Set_input_r( stream->axi_config_tx,(u32)stream->buff );
	}
	else if(stream->direction == RX)
	{
		XExample_rx_piped64_Set_output_r( stream->axi_config_rx,(u32)stream->buff );
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
		return XExample_tx_piped64_IsDone(stream->axi_config_tx);
	}
	else if(stream->direction == RX)
	{
		return XExample_rx_piped64_IsDone(stream->axi_config_rx);
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
		XExample_tx_piped64_Start(stream->axi_config_tx);
		//XExample_tx_piped64_EnableAutoRestart(stream->axi_config_tx);
	}
	else if(stream->direction == RX)
	{
		XExample_rx_piped64_Start(stream->axi_config_rx);
		//XExample_rx_piped64_EnableAutoRestart(stream->axi_config_rx);
	}

}


/*
* Simple read write with no handshaking
*/
void presence_write( stream_id_type stream_id, uint32_t data )
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


uint32_t presence_read( stream_id_type stream_id )
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
			//if(timeout == 2000)
			//	break;
		}
	}

	//
	return -1;
}

/*
* write a byte of data
*/
void circular_write( stream_id_type stream_id, uint64_t data )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	//uint64_t temp = stream->buff[100];
	//uint64_t temp2 = stream->buff[101];

	while(1)
	{

		//
		//
		//stream->tail = stream->buff[TAIL_POINTER];

		//
		// Not allowed to write when stream is full or the next update will collide
		// always leave 1 space
		if( ( (stream->head + 1)  == stream->tail) || (( (stream->head +1) & stream->buff_size) ==  stream->tail ) )
		{
			//
			//
			stream->tail = stream->buff[TAIL_POINTER];
        }
        else
        {


        	//
        	// write and update local pointer
        	stream->buff[stream->head] = data;
        	dsb();
            stream->head++;
            stream->head = stream->head & stream->buff_size;


//            printf("head: %d tail: %d full: %d\n",stream->head,stream->tail,stream->full);


            //
            // update state
            stream->buff[HEAD_POINTER] = stream->head;

            stream->bytes_written++;
            break;
        }
	}
}

//
// reads one byte from the buffer
//
uint64_t circular_read( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );

	int timeout = 0;
	uint64_t data = 0;


	while(1)
	{

//		//
//		// not allowed to read when the pointers are equal and the full bit is zero
//		// can read when pointers are equal and fullness is set this means writer is much faster
//		//
//		stream->head = stream->buff[HEAD_POINTER];

		if( (stream->tail == stream->head) )
		{
			//
			// not allowed to read when the pointers are equal and the full bit is zero
			// can read when pointers are equal and fullness is set this means writer is much faster
			//
			stream->head = stream->buff[HEAD_POINTER];
        }
		else
		{
			//
            // grab data and update pointers
			//
			data = stream->buff[stream->tail];
            stream->tail++;
            stream->tail = stream->tail & stream->buff_size;

//            printf("head: %d tail: %d full: %d\n",stream->tail,stream->tail,stream->full);

            // write our new updated position
            stream->buff[TAIL_POINTER] = stream->tail;

            stream->bytes_read++;
            return data;

		}
	}

	//
	return -1;
}

/*
* do one single transaction attempting to get as many bytes as possbile
*/
uint32_t burst_write_single( stream_id_type stream_id, uint64_t* data, uint32_t length )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	// write the requested bytes indefinitely
	while(1)
	{
		stream->tail = stream->buff[TAIL_POINTER];

		//
		// ensure it is safe to write.
		// Not allowed to write when stream is full or the next update will collide
		// always leave 1 space
		if( ( (stream->head + 1)  == stream->tail) || (( (stream->head +1) % stream->buff_size) ==  stream->tail ) )
		{

        }
        else
        {
        	//
        	// compute how many bytes it is safe to write
        	int bytes_to_write = stream->tail - (stream->head+1);

        	//
        	// if bytes a negative occurs this is the case where head is 90 and tail has wrapped around
        	// recompute
        	if(bytes_to_write < 0){
        		bytes_to_write = stream->buff_size - (stream->head+1) + stream->tail;
        	}

        	//
        	// choose the minimum in the case where we have enough space to write 10 bytes but want to only write 2
        	bytes_to_write = (bytes_to_write < length) ? bytes_to_write : length;

        	// -1 so that we do not over write?
        	// burst write our data
        	for(int h=0; h < bytes_to_write; h++)
        	{
        		stream->buff[stream->head] = data[h];  //grab a byte from the buffer
        		stream->head++;  //incrementthe tail
        		stream->head = stream->head % stream->buff_size;
        	}


            // printf("head: %d tail: %d full: %d\n",stream->head,temp_tail,stream->full);

            // update state
            stream->buff[HEAD_POINTER] = stream->head;


            // return how many bytes were transferred
            return bytes_to_write;

        }
	}
	return -1;
}

/*
 *  this will return how many bytes were written in a single transaction
 */
uint32_t burst_read_single( stream_id_type stream_id,uint64_t* data_out )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	while(1)
	{
		stream->head = stream->buff[HEAD_POINTER];

		//
		// not allowed to read when the pointers are equal and the full bit is zero
		// can read if our pointers are equal and the full bit is set though
		if( stream->tail == stream->head  )
		{
			// can not read
        }
		else
		{
		    //
			// compute how many bytes we can read
			int bytes_read = stream->head - stream->tail;

			//
			// if the amount is negative this is the case where our head ptr has wrapped around
			// but tail pointer has not
			if(bytes_read < 0){
				bytes_read = (stream->buff_size - stream->tail) + stream->head;
			}
			// burst read
			for(int h = 0; h < bytes_read; h++)
			{
				data_out[h] = stream->buff[stream->tail];  //grab a byte from the buffer
				stream->tail++;  //incrementthe tail
				stream->tail = stream->tail % stream->buff_size;
				stream->bytes_read++;
			}

//			stream->bytes_written++;
            //printf("head: %d tail: %d full: %d\n",temp_head,stream->tail,stream->buff[FULL_BIT]);


			// write our new updated position
            stream->buff[TAIL_POINTER] = stream->tail;


            return bytes_read;

		}
	}

	//
	return -1;
}

/*
 *  read a set amount of bytes.
 *  will not return until the specified amount of bytes is read
 */
uint32_t burst_read( stream_id_type stream_id,uint64_t* data_out, uint32_t length )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	// refresh our status
	volatile uint64_t temp_head = stream->buff[HEAD_POINTER];
//	stream->full = stream->buff[FULL_BIT];// refresh our full status

	uint32_t total_bytes_read = 0;

	while(1)
	{

		//
		// not allowed to read when the pointers are equal and the full bit is zero
		// can read if our pointers are equal and the full bit is set though
		if( stream->tail == temp_head  )
		{
			// refresh our status
//			stream->full = stream->buff[FULL_BIT];
			temp_head = stream->buff[HEAD_POINTER];
        }
		else
		{
		    //
			// compute how many bytes we can read
			int bytes_read = temp_head - stream->tail;

			//
			// if the amount is negative this is the case where our head ptr has wrapped around
			// but tail pointer has not
			if(bytes_read < 0)
				bytes_read = (stream->buff_size - temp_head) + stream->tail;

			//
			// if our full bit is set then we can burst read the entire buffer
//			if(bytes_read == 0 && stream->full == 1)
				bytes_read = stream->buff_size;

        	// choose the minimum as we do not want to overflow the users buffer
			bytes_read = (bytes_read < length) ? bytes_read : length;

			// burst read
			for(int h = 0; h < bytes_read; h++)
			{
				data_out[h+total_bytes_read] = stream->buff[stream->tail];  //grab a byte from the buffer
				stream->tail++;  //incrementthe tail
				stream->tail = stream->tail % stream->buff_size;
			}

			// update pointers
			total_bytes_read+=bytes_read;
			length -= bytes_read;


			//printf("head: %d tail: %d full: %d\n",temp_head,stream->tail,stream->buff[FULL_BIT]);


			// write our new updated position
			stream->buff[TAIL_POINTER] = stream->tail;

            // we should never read too many bytes that means some logic is wrong
            assert(length >= 0);

            // satisfied the users request
            if(length == 0)
            {
            	return bytes_read;
            }
		}
	}

	//
	return -1;
}


/*
* do burst writes and do not return until the length is satisfied
*/
uint32_t burst_write( stream_id_type stream_id, uint64_t* data, uint32_t length )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );


	// read tail pointer
	stream->tail = stream->buff[TAIL_POINTER];

	uint32_t bytes_written = 0;

	// write the requested bytes indefinitely
	while(1)
	{

		//
		// ensure it is safe to write.
		// it is unsafe to write if the next write will become the tail pointer
		// WE NEED TO CHECK FOR WRAPAROUND ERIC THATS THE BUG
		// or if our stream is full
		//
		// Not allowed to write when stream is full or the next update will collide
		// always leave 1 space
		if( ( (stream->head + 1)  == stream->tail) || (( (stream->head +1) % stream->buff_size) ==  stream->tail ) )
		{

        }
        else
        {
        	//
        	// compute how many bytes it is safe to write
        	int bytes_to_write = stream->tail - stream->head;

        	//
        	// if bytes a negative occurs this is the case where head is 90 and tail has wrapped around
        	// recompute
        	if(bytes_to_write < 0){
        		bytes_to_write = stream->buff_size - stream->head + stream->tail;
        	}


        	//
        	// choose the minimum in the case where we have enough space to write 10 bytes but want to only write 2
        	bytes_to_write = (bytes_to_write < length) ? bytes_to_write : length;

        	// -1 so that we do not over write?
        	// burst write our data
        	for(int h=0; h < bytes_to_write; h++)
        	{
        		stream->buff[stream->head] = data[h];  //grab a byte from the buffer
        	    stream->head++;  //incrementthe tail
        	    stream->head = stream->head % stream->buff_size;
        	}

        	// update
        	bytes_written+=bytes_to_write;
        	length -= bytes_to_write;


        	// printf("head: %d tail: %d full: %d\n",stream->head,temp_tail,stream->full);

        	// update state
        	stream->buff[HEAD_POINTER] = stream->head;
        	// we should never write too many bytes that means some logic is wrong
        	assert(length >= 0);

            if(0 == length)
            {
            	return bytes_written;
            }
        }
	}
	return -1;
}

void print_state( stream_id_type stream_id )
{
	//
	stream_t* stream = safe_get_stream( stream_id );

	// stream should be created already
	assert( stream != NULL );
	printf("head : %d tail : %d read : %d wrote : %d \n",stream->head,stream->tail,stream->bytes_read,stream->bytes_written);
	if(stream->direction == TX )
	{
		printf("IsDone: %d \n",XExample_tx_piped64_IsDone(stream->axi_config_tx));
		printf("Input %X \n",XExample_tx_piped64_Get_input_r( stream->axi_config_tx ));
	}
	else if(stream->direction == RX)
	{
		printf("IsDone: %d \n",XExample_rx_piped64_IsDone(stream->axi_config_rx));
		printf("Output %X \n",XExample_rx_piped64_Get_output_r( stream->axi_config_rx ));
	}
}
