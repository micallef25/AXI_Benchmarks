#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include "xexample_tx.h"
#include "xexample_rx.h"


typedef struct packet_type{
	uint32_t data;
	uint8_t msg_counter;
}packet_t;


enum direction_t{
	TX = 0,
	RX = 1,
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
	MAX_NUMBER_OF_STREAMS,
};

typedef enum direction_t direction_type;
typedef enum memory_t memory_type;
typedef enum stream_id_t stream_id_type;
typedef enum axi_port_t axi_port_type;

typedef struct stream_t{

	volatile uint32_t* buff;

	// could move to "port object" later
	//axi_config;

	//
	uint32_t buff_size;
	
	// place in buffer
	uint8_t ptr;

	// 
	uint8_t msg_counter;

	//
	XExample_tx* axi_config_tx;
	XExample_rx* axi_config_rx;

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

	// polling
	uint32_t is_stream_done( stream_id_type stream_id );

	// start streams
	void start_stream( stream_id_type stream_id );


#endif
