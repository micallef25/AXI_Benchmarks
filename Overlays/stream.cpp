#include <stdint.h>

// TODO first create the generic API's showing that it works
// then we can complicate and do per port design
//
//


InterruptHandler()
{

}


stream::stream( HARDWARE, DRIECTION )
{
	ptr = 0;
}

void stream::init()
{
	//some set up may be for just running in software 
	// need to check output if the CPU needs to register this in the TLB
	// may need to create alot of memory up front so that in PS to PS we can share pointers

	// point FPGA to buffer
	Xexample_Set()

	// tell it to restart immediately
	Xexample_autorestart()

	//
	SetupInterrupts()

	// mark as outershareable
	Xil_TlbSetAttribute()

	// set CCI to snoop
	Xu32()
}

// could potentially make a table of pointers that is unique to each port
//
uint32_t stream::read()
{
	// spin until a new message has arrived
	while(1)
	{

		volatile uint64_t data = buffer[ptr];
	
		// msg is not ready, delay
		if(data == msg_counter)
		{
			for(int i = 0; i < DELAY; i++)
			{
				//delay this could be optmized out with a better compiler whic we want to avoid
			}
		}
		else
		{
			ptr++;
			ptr = ptr % BUFF_SIZE;
			return (data & ~msg_counter);
		}
	}
}

void stream::write( uint32_t data )
{
	// append out message counter and write to memory
	buffer[ptr] = data | msg_counter;
	ptr++;
	ptr = ptr % BUFF_SIZE;
	// need to handle waiting to make sure sure that data is not overwritten


}