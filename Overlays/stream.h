#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include "xexample_tx.h"
#include "xexample_rx.h"


typedef struct packet_type{
	uint32_t data;
	uint8_t msg_counter;
}packet_t;


enum direction_type{
	TX = 0,
	RX = 1,
};

enum memory_type{
	DDR = 0, // data stored in DDR
	OCM = 1, // use OCM 
	CACHE = 2,
	INVALID_MEMORY,
};

enum axi_port_type{
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

typedef stream_t{

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


}stream_t;



	// creation and deletion of streams
	stream_id_t stream_create( uint32_t buff_size, direction_type direction  );
	stream_destroy( stream_id_t );


	//stream initialization
	int stream_init( axi_port_type port, stream_id_t id );

//	uint32_t read();
	// int init_tx( axi_port_type port, uint32_t ps_id );
	// int init_rx( axi_port_type port, uint32_t ps_id );
//	void write( uint32_t );

	// name changers
	axi_port_type port_str_to_type( char* port );
	const char* port_type_to_str( axi_port_type port );
	memory_type mem_str_to_type( char* str );
	const char* mem_type_to_str( memory_type memory );

	// read and write 
	uint32_t simple_read();
	void simple_write( uint32_t data );

	// polling
	u32 is_stream_done();

	// start streams
	void start_tx();
	void start_rx();


#endif
