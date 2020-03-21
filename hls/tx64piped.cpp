#include <stdio.h>
#include <stdint.h>
#include <hls_stream.h>


#define DEBUG1 100

enum meta_data_t{
	RAW_BUFFER_SIZE = 258,
	HEAD_POINTER = 257,
	TAIL_POINTER = 256,
	BUFFER_SIZE = 256,
	MASK = 255,
};


void example_tx_piped64( volatile uint64_t* input, hls::stream<uint64_t> &fifo )
{

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem_read

#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS INTERFACE axis port=fifo bundle=stream_out
#pragma HLS STREAM depth=100 variable=fifo

	uint8_t stream_head=0;
	uint8_t stream_tail=0;
	uint64_t data[BUFFER_SIZE];

	int16_t bytes_read = 0;
	uint8_t cycles=0;
	while(1)
		{
			//
			// not allowed to read when the pointers are equal and the full bit is zero
			// can read when pointers are equal and fullness is set this means writer is much faster
			//
			stream_head = input[HEAD_POINTER];

			if( stream_tail == stream_head )
			{
				for(int i = 0; i < 10; i++)
				{
					cycles++;
				}
	        }
			else
			{

				//
				// no wrap around
				if(stream_head > stream_tail)
				{
					bytes_read = (stream_head - stream_tail);
				}
				//
				// head has wrapped around but tail has not tail = 90
				// head = 10
				else
				{
					bytes_read = ((BUFFER_SIZE - stream_tail) + stream_head );
				}

				//input[DEBUG1] = bytes_read;

				// burst read
				gmem_read:for(int h = 0; h < bytes_read; h++)
				{
#pragma HLS pipeline II=1
					// read from global mem and into bft
					data[h]  = input[stream_tail];  //grab a byte from gmem

		            stream_tail++;
		            stream_tail = stream_tail & MASK;
				}

				fifo_write:for(int h = 0; h < bytes_read; h++)
				{
#pragma HLS pipeline II=1
					//
					fifo.write(data[h]);
				}



	            // write our new updated position
	            input[TAIL_POINTER] = stream_tail;
			}
		}// while 1
 }


