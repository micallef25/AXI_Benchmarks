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

void example_rx_piped64(volatile uint64_t *output, hls::stream<uint64_t> &fifo)
{

#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem_out

#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

	#pragma HLS INTERFACE axis port=fifo bundle=stream_in
	#pragma HLS STREAM depth=100 variable=fifo

	uint8_t stream_head=0;
	uint8_t stream_tail=0;
	uint64_t data[BUFFER_SIZE];

	int16_t bytes_to_write=0;
	uint8_t cycles=0;
while(1)
	{
		//
		stream_tail = output[TAIL_POINTER];

		//
		// Not allowed to write when stream is full or the next update will collide
		// always leave 1 space
		if( ((stream_head + 1)  == stream_tail) || (( (stream_head +1) & MASK) ==  stream_tail ) )
		{
			// can not write so we refresh our status and check again
			//
			for(int i = 0; i < 10; i++)
			{
				cycles++;
			}
        }
        else
        {

        	//
			// head has wrapped around but tail has not tail = 90
			// head = 10
			if( (stream_head+1) < stream_tail)
			{
				bytes_to_write = stream_tail - (stream_head+1);
			}
			//
			// typical case where head is leading
			else
			{
				bytes_to_write = BUFFER_SIZE - (stream_head+1) + stream_tail;
			}

        	//output[DEBUG1] = bytes_to_write;

        	fifo_read:for(int h=0; h < bytes_to_write; h++)
        	{
#pragma HLS pipeline II=1
        		if (fifo.read_nb(data[h])) {
        		    // read successful
        		} else {
        		    // read failed no data in the stream
        			bytes_to_write = h;
        			break;
        		}
        	}
        	// burst write our data
        	gmem_write:for(int h=0; h < bytes_to_write; h++)
        	{
#pragma HLS pipeline II=1
        		// write to gmem from app stream
        		output[stream_head] = data[h];

        		stream_head++;  //incrementthe tail
        		stream_head = stream_head & MASK;
        	}

            //
            // update state
        	output[HEAD_POINTER] = stream_head;
        }
	}
}
