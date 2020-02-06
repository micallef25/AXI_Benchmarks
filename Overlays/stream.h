#include <stdint.h>
#include "stream.h"



class stream{

enum memory_type{
	DDR = 0, // data stored in DDR
	OCM = 1, // use OCM 
	L2 = 2,
	INVALID_MEMORY,
}memory_t;

enum port_type{
	HPC0 = 0,
	HPC1,
	HP1,
	HP2,
	HP3,
	HP4,
	ACE,
	ACP,
	INVALID_PORT,
}port_t;


typedef struct packet_type{
	uint32_t data;
	uint8_t msg_counter;
}packet_t;


public:
	read();
	init();
	write();

protected:
	volatile uint32_t buffer[BUFF_SIZE];
	
	// place in buffer
	uint8_t ptr;

	// 
	uint8_t msg_counter;
};