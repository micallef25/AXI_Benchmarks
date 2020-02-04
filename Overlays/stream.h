#include <stdint.h>
#include "stream.h"



class stream{

public:
	read();
	init();
	write();

protected:
	volatile uint64_t buffer[BUFF_SIZE];
	
	// place in buffer
	uint8_t ptr;

	// 
	uint8_t msg_counter;
};