#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>

#define IP_32
#ifdef IP_32
#include "xexample_tx_piped64.h"
#include "xexample_rx_piped64.h"
#else
#include "xexample_tx_128.h"
#include "xexample_rx_128.h"
#endif

//#define BUFFER_SIZE 258 // needs to be power of 2 + 2
//#define HEAD_POINTER 256
//#define TAIL_POINTER 257
//#define FULL_BIT 97

typedef struct packet_type{
	uint32_t data;
	uint8_t msg_counter;
}packet_t;


enum direction_t{
	TX = 0,
	RX = 1,
};

enum meta_data_t{
	RAW_BUFFER_SIZE = 258,
	HEAD_POINTER = 257,
	TAIL_POINTER = 256,
	MASK = 255,
};

enum memory_t{
	DDR = 0, // data stored in DDR
	OCM = 1, // use OCM 
	CACHE = 2,
	INVALID_MEMORY,
};

enum axi_port_t{
	HPC0 = 0,
	HPC1,
	HP0,
	HP1,
	HP2,
	HP3,
	ACE,
	ACP,
	INVALID_PORT,
};

enum stream_id_t{
	STREAM_ID_0 = 0,
	STREAM_ID_1,
	STREAM_ID_2,
	STREAM_ID_3,
	STREAM_ID_4,
	STREAM_ID_5,
	MAX_NUMBER_OF_STREAMS,
};

typedef enum direction_t direction_type;
typedef enum memory_t memory_type;
typedef enum stream_id_t stream_id_type;
typedef enum axi_port_t axi_port_type;
typedef enum meta_data_t meta_data_type;

typedef struct stream_t{

	volatile uint64_t* buff;

	// could move to "port object" later
	//axi_config;

	//
	uint64_t buff_size;
	
	// place in buffer
	uint8_t ptr;

	// head and tail pointers
	volatile uint64_t head;
	volatile uint64_t tail;

	//
	uint32_t bytes_read;
	uint32_t bytes_written;

	// 
	uint8_t msg_counter;

	//
#ifdef IP_32
	XExample_tx_piped64* axi_config_tx;
	XExample_rx_piped64* axi_config_rx;
#else
	XExample_tx_128* axi_config_tx;
	XExample_rx_128* axi_config_rx;
#endif

	axi_port_type port;
	memory_type memory;
	direction_type direction;

	uint32_t X_ID;


}stream_t;

	void clear_streams();

	// creation and deletion of streams
	int stream_create( stream_id_type stream_id, uint32_t buff_size, direction_type direction,memory_type mem, axi_port_type port  );
	void stream_destroy( stream_id_type stream_id );

	//stream initialization
	int stream_init( stream_id_type stream_id );

	// name changers
	axi_port_type port_str_to_type( char* port );
	const char* port_type_to_str( axi_port_type port );
	memory_type mem_str_to_type( char* str );
	const char* mem_type_to_str( memory_type memory );

	// read and write 
	uint32_t simple_read( stream_id_type stream_id );
	void simple_write( stream_id_type stream_id, uint32_t data );

	//
	void presence_write( stream_id_type stream_id, uint32_t data );
	uint32_t presence_read( stream_id_type stream_id );

	//
	void circular_write( stream_id_type stream_id, uint64_t data );
	uint64_t circular_read( stream_id_type stream_id );

	//
	uint32_t burst_read( stream_id_type stream_id,uint64_t* data_out, uint32_t length );
	uint32_t burst_write( stream_id_type stream_id, uint64_t* data, uint32_t length );

	//
	uint32_t burst_read_single( stream_id_type stream_id,uint64_t* data_out );
	uint32_t burst_write_single( stream_id_type stream_id, uint64_t* data, uint32_t length );

	// polling
	uint32_t is_stream_done( stream_id_type stream_id );

	// start streams
	void start_stream( stream_id_type stream_id );

	void print_state( stream_id_type stream_id );


#endif
