#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>

#include "xexample_tx_piped64.h"
#include "xexample_rx_piped64.h"


class stream{

public:


	//
	// enums / typedefs / macros
	enum direction_t
	{
		TX = 0,
		RX = 1,
	};

	enum memory_t
	{
		DDR = 0, // data stored in DDR
		OCM = 1, // use OCM 
		CACHE = 2,
		INVALID_MEMORY,
	};

	enum axi_port_t
	{
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

	enum meta_data_t
	{
		POW_2 = 256,
		RAW_BUFFER_SIZE = POW_2+2,
		HEAD_POINTER = POW_2+1,
		TAIL_POINTER = POW_2,
		MASK = POW_2-1,
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


	//
	// function defintions

	stream( stream_id_t id, direction_t direction, memory_t mem, axi_port_t port );
	~stream();

	// creation and deletion of streams
	// int stream_create( direction_type direction,memory_type mem, axi_port_type port  );
	// void stream_destroy();

	//stream initialization
	int stream_init();

	// name changers
	axi_port_t port_str_to_type( char* port );
	const char* port_type_to_str( axi_port_t port );
	memory_t mem_str_to_type( char* str );
	const char* mem_type_to_str( memory_t memory );

	// read and write for measuring performance
	uint32_t simple_read();
	void simple_write( uint32_t data );

	// circular buffer implementation for sending data to PL
	void write( uint64_t data );
	uint64_t read();

	// polling
	uint32_t is_stream_done();

	// start streams
	void start_stream();

	void print_state();

	//
	// variables

private:

	//
	// enums / typedefs / macros


	//
	// function defintions


	// private variables
	volatile uint64_t* buff;

	uint16_t ptr;

	//
	uint64_t buff_size;

	// head and tail pointers
	uint16_t head;
	uint16_t tail;

	//
	uint32_t bytes_read;
	uint32_t bytes_written;

	//
	XExample_tx_piped64* axi_config_tx;
	XExample_rx_piped64* axi_config_rx;

	axi_port_t port;
	memory_t memory;
	direction_t direction;

	uint32_t bsp_id;

};

#endif
