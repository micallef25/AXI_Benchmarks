//#define COMPILE_STREAM_TEST

#ifdef COMPILE_STREAM_TEST

#include <stdio.h>
#include <hls_stream.h>

void example_rx(volatile int *outbuff,int instream[100] /*hls::stream<int> stream_in*/){

#pragma HLS INTERFACE m_axi port=outbuff offset=slave bundle=gmem0
//#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem1

#pragma HLS INTERFACE s_axilite port=outbuff bundle=control
//#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control


#pragma HLS INTERFACE axis port=instream bundle=outstream
//#pragma HLS INTERFACE axis port=inbuff bundle=instream


	int data = 0x00;
	int i = 0;
	while(data != 0xdeadbeef)
	{
		//data = input[i]; // read from global memory
		data = instream[i]; // write to stream
		//data = inbuff[i]; // read from stream
//		data = stream_in.read();
		outbuff[i] = data; // write to output
		i++;
		if(i == 100)
		{
			i = 0;
		}
	}
 }

#endif
