#include <stdint.h>
#include "stream.h"
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <stdio.h>
#include "sleep.h"

#include "xil_cache.h"
#include "xil_mmu.h"
#include "xpseudo_asm.h"
#include "xil_types.h"
#include "xparameters.h"


#define MAX_NUM_PORTS INVALID_PORT
#define MAX_NUM_MEMORIES INVALID_MEMORY
#define NUM_CORES 4

#define MAX_OCM_PARTITONS 6
#define OCM_BUFF1 (0xFFFC0000)
#define OCM_BUFF2 OCM_BUFF1 + (stream::RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF3 OCM_BUFF2 + (stream::RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF4 OCM_BUFF3 + (stream::RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF5 OCM_BUFF4 + (stream::RAW_BUFFER_SIZE * sizeof(uint64_t))
#define OCM_BUFF6 OCM_BUFF5 + (stream::RAW_BUFFER_SIZE * sizeof(uint64_t))


// can partition our ocm #TODO change this later
static volatile uint64_t ocm_buff[stream::stream_id_t::MAX_NUMBER_OF_STREAMS] = {
		OCM_BUFF1,
		OCM_BUFF2,
		OCM_BUFF3,
		OCM_BUFF4,
		OCM_BUFF5,
		OCM_BUFF6
};


static uint8_t id_array[stream::stream_id_t::MAX_NUMBER_OF_STREAMS] = {
		XPAR_EXAMPLE_TX_PIPED64_0_DEVICE_ID, // stream 0 transmits to pl on device 0
		XPAR_EXAMPLE_RX_PIPED64_0_DEVICE_ID, // stream 1 reads from PL on device 0
		XPAR_EXAMPLE_TX_PIPED64_1_DEVICE_ID, // stream 2 transmits to pl on device 1
		XPAR_EXAMPLE_RX_PIPED64_1_DEVICE_ID, // stream 3 reads from pl on device 1
		XPAR_EXAMPLE_TX_PIPED64_2_DEVICE_ID, // stream 4 reads from pl on device 2
		XPAR_EXAMPLE_RX_PIPED64_2_DEVICE_ID, // stream 5 reads from pl on device 2
};


/*
* useful conversions
*
*/
stream::axi_port_t stream::port_str_to_type( char* str )
{
	if( !str )
		return INVALID_PORT;

	for( int n=0; n < MAX_NUM_PORTS; n++ )
	{
		if( strcasecmp(str, port_type_to_str((stream::axi_port_t)n)) == 0 )
			return (stream::axi_port_t)n;
	}

	return INVALID_PORT;
}

const char* stream::port_type_to_str( stream::axi_port_t port )
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


stream::memory_t stream::mem_str_to_type( char* str )
{
	if( !str )
		return INVALID_MEMORY;

	for( int n=0; n < MAX_NUM_MEMORIES; n++ )
	{
		if( strcasecmp(str, mem_type_to_str((stream::memory_t)n)) == 0 )
			return (stream::memory_t)n;
	}

	return INVALID_MEMORY;
}

const char* stream::mem_type_to_str( stream::memory_t memory )
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
stream::stream( stream::stream_id_t stream_id, stream::direction_t direction, stream::memory_t mem, stream::axi_port_t port  )
{
	// create buffer and set ptr
	if( mem == CACHE || mem == DDR )
	{
		this->buff = (volatile uint64_t*)malloc( sizeof(uint64_t)*RAW_BUFFER_SIZE );
		assert(this->buff != NULL);
	}
	else if( mem == OCM )
	{
		this->buff = (volatile uint64_t*)ocm_buff[stream_id];
	}

	this->bsp_id = id_array[stream_id];

	memset((void*)this->buff, 0, RAW_BUFFER_SIZE*sizeof(uint64_t));

	// depending on the direction get the correct IP instance
	if( direction == TX )
	{
		this->axi_config_tx = (XExample_tx_piped64*)malloc(sizeof(XExample_tx_piped64));
		this->axi_config_rx = NULL;
		assert(this->axi_config_tx != NULL);
	}
	else if( direction == RX )
	{
		this->axi_config_tx = NULL;
		this->axi_config_rx = (XExample_rx_piped64*)malloc(sizeof(XExample_rx_piped64));
		assert(this->axi_config_rx != NULL);
	}
	else
	{
		// invalid arg crash the program
		assert(0);
	}


	// set the unique characteristics of the stream
	this->ptr = 0;
	this->head = 0;
	this->tail = 0;
	this->memory = mem;
	this->port = port;
	this->buff_size = MASK;
	this->direction = direction;
	this->bytes_written = 0;
	this->bytes_read = 0;
//	reams[XPAR_CPU_ID][stream_id] = stream;

}

//void stream::stream_destroy( stream_id_type stream_id)
stream::~stream()
{

	// OCM is not heap managed
	if(this->memory != OCM)
		free((void*)this->buff);

	this->buff = NULL;

	// depending on the direction get the correct IP instance
	if( this->direction == TX )
	{
		free((void*)this->axi_config_tx);
		this->axi_config_tx = NULL;
	}
	else if( this->direction == RX )
	{
		free((void*)this->axi_config_rx);
		this->axi_config_rx = NULL;
	}
	else
	{
		// invalid arg crash the program
		assert(0);
	}

	// Finally free our stream and our table entry
	//free(stream);
	//streams[XPAR_CPU_ID][stream_id] = NULL;
}

// pass in which port you want and who is master and slave for this stream
int stream::stream_init()
{

	int rval = XST_FAILURE;

	// initialize our drivers
	if(this->direction == TX )
	{
		rval = XExample_tx_piped64_Initialize( this->axi_config_tx, this->bsp_id );
		usleep(5);
		XExample_tx_piped64_EnableAutoRestart(this->axi_config_tx);
	}
	else if(this->direction == RX)
	{
		rval = XExample_rx_piped64_Initialize( this->axi_config_rx, this->bsp_id );
		usleep(5);
		XExample_rx_piped64_EnableAutoRestart(this->axi_config_rx);
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
     u32 offset = *((u32*)(&buff));

	// point FPGA to buffers
	if(this->direction == TX )
	{
		XExample_tx_piped64_Set_input_r( this->axi_config_tx,offset );
	}
	else if(this->direction == RX)
	{
		XExample_rx_piped64_Set_output_r( this->axi_config_rx,offset );
	}

	// tell it to restart immediately
	// Xexample_autorestart()

	//
	// SetupInterrupts()

	// slave 3 from CCI man page. This enables snooping from the HPC0 and 1 ports
	if( this->memory == DDR || this->memory == OCM )
	{
    	Xil_Out32(0xFD6E4000,0x0);
    	dmb();
		Xil_SetTlbAttributes((UINTPTR)this->buff, NORM_NONCACHE);
		dmb();
    }

    // mark our memory regions as outer shareable which means it will not live in L1 but L2
    // TODO make this a parameter for user to pass in or atleast a macro
	if( this->memory == CACHE /*|| this->memory == OCM */)
	{
    	Xil_Out32(0xFD6E4000,0x1);
    	dmb();
		Xil_SetTlbAttributes((UINTPTR)this->buff, 0x605);
		dmb();
	}
    return XST_SUCCESS;
}




/*
* Simple read write with no handshaking
*/
void stream::simple_write( uint32_t data )
{
	//
	this->buff[this->ptr] = data;
	this->ptr++;
	this->ptr = this->ptr % this->buff_size;
}


uint32_t stream::simple_read()
{
	uint32_t temp;

	temp = this->buff[this->ptr];
	this->ptr++;
	this->ptr = this->ptr % this->buff_size;

	//
	return temp;
}


uint32_t stream::is_stream_done()
{
	if(this->direction == TX )
	{
		return XExample_tx_piped64_IsDone(this->axi_config_tx);
	}
	else if(this->direction == RX)
	{
		return XExample_rx_piped64_IsDone(this->axi_config_rx);
	}
	return 0;
}

void stream::start_stream()
{
	if(this->direction == TX )
	{
		XExample_tx_piped64_Start(this->axi_config_tx);
		//XExample_tx_piped64_EnableAutoRestart(this->axi_config_tx);
	}
	else if(this->direction == RX)
	{
		XExample_rx_piped64_Start(this->axi_config_rx);
		//XExample_rx_piped64_EnableAutoRestart(this->axi_config_rx);
	}
}

/*
* write a byte of data
*/
void stream::write( uint64_t data )
{

	while(1)
	{

		//
		// Not allowed to write when stream is full or the next update will collide
		// always leave 1 space
		if( ( (this->head + 1)  == this->tail) || (( (this->head +1) & this->buff_size) ==  this->tail ) )
		{
			//
			//
			this->tail = this->buff[TAIL_POINTER];
        }
        else
        {


        	//
        	// write and update local pointer
        	this->buff[this->head] = data;
        	dsb();
            this->head++;
            this->head = this->head & this->buff_size;


//            printf("head: %d tail: %d full: %d\n",this->head,this->tail,this->full);


            //
            // update state
            this->buff[HEAD_POINTER] = this->head;

            this->bytes_written++;
            break;
        }
	}
}

//
// reads one byte from the buffer
//
uint64_t stream::read()
{

	int timeout = 0;
	uint64_t data = 0;


	while(1)
	{

		if( (this->tail == this->head) )
		{
			//
			// not allowed to read when the pointers are equal and the full bit is zero
			// can read when pointers are equal and fullness is set this means writer is much faster
			//
			this->head = this->buff[HEAD_POINTER];
        }
		else
		{
			//
            // grab data and update pointers
			//
			data = this->buff[this->tail];
            this->tail++;
            this->tail = this->tail & this->buff_size;

//            printf("head: %d tail: %d full: %d\n",this->tail,this->tail,this->full);

            // write our new updated position
            this->buff[TAIL_POINTER] = this->tail;

            this->bytes_read++;
            return data;

		}
	}

	//
	return -1;
}

void stream::print_state( )
{
	printf("head : %d tail : %d read : %d wrote : %d \n",this->head,this->tail,this->bytes_read,this->bytes_written);
	if(this->direction == TX )
	{
		printf("IsDone: %d \n",XExample_tx_piped64_IsDone(this->axi_config_tx));
		printf("Input %X \n",XExample_tx_piped64_Get_input_r( this->axi_config_tx ));
	}
	else if(this->direction == RX)
	{
		printf("IsDone: %d \n",XExample_rx_piped64_IsDone(this->axi_config_rx));
		printf("Output %X \n",XExample_rx_piped64_Get_output_r( this->axi_config_rx ));
	}
}
