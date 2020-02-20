#include <stdio.h>
#include <stdint.h>

void example_rx_64(volatile uint64_t *output,uint64_t instream[100]){

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem_out

#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS INTERFACE axis port=instream bundle=stream_in


	uint64_t data = 0x00;
	int i = 0;
	while(data != 0xdeadbeef)
	{
#pragma HLS pipeline II=1
		data = instream[i]; // write to stream
		output[i] = data; // write to output
		i++;
		if(i == 100)
		{
			i = 0;
		}
	}
 }
