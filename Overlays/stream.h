#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include "xexample_tx.h"
#include "xexample_rx.h"


class stream{

typedef struct packet_type{
	uint32_t data;
	uint8_t msg_counter;
}packet_t;


public:

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

#define MAX_NUM_PORTS INVALID_PORT
#define MAX_NUM_MEMORIES INVALID_MEMORY



	stream( uint32_t buff_size );
	~stream();
//	uint32_t read();
	int init_tx( axi_port_type port, uint32_t ps_id );
	int init_rx( axi_port_type port, uint32_t ps_id );
//	void write( uint32_t );

	axi_port_type port_str_to_type( char* port );
	const char* port_type_to_str( axi_port_type port );
	memory_type mem_str_to_type( char* str );
	const char* mem_type_to_str( memory_type memory );

	uint32_t simple_read();
	void simple_write( uint32_t data );

	u32 is_stream_done();
	void start_tx();
	void start_rx();

protected:
	// table to store read and write pointers
	// uint32_t** buffer_table[2];

	// volatile uint32_t* in_buff;
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
};


#endif
