//#define COMPILE_STREAM_TEST

//#ifdef COMPILE_STREAM_TEST


#include <stdio.h>
#include <stdint.h>

void example_tx_64(volatile uint64_t* input,uint64_t outbuffer[100])
{

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem_read

#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS INTERFACE axis port=outbuffer bundle=stream_out


	uint64_t data = 0x00;
	char i = 0;
	while(data != 0xdeadbeef)
	{
#pragma HLS pipeline II=1
#pragma HLS dependence variable=data inter false
		data = input[i]; // read from global memory
		outbuffer[i] = data; // write to stream
		i++;
		if( i == 100)
		{
			i = 0;
		}
	}
 }

//#endif
