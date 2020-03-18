#include <stdio.h>
#include <stdint.h>
#include <hls_stream.h>

//#define BUFFER_SIZE 97
//#define HEAD_POINTER 99
//#define TAIL_POINTER 98
//#define FULL_BIT 97
#define DEBUG1 100
#define DEBUG2 101

enum meta_data_t{
	RAW_BUFFER_SIZE = 258,
	HEAD_POINTER = 257,
	TAIL_POINTER = 256,
	BUFFER_SIZE = 256,
	MASK = 255,
};

//void read_operator( volatile uint64_t* output, hls::stream<uint64_t> &fifo)
//{
//#pragma HLS INLINE
//
//	uint8_t read_stream_ptr=0;
//	uint64_t read_data = 0;
//	uint64_t done = 0;
//	read_loop:for(int i = 0; i < 100; i++)
//	{
//#pragma HLS pipeline II=1
////#pragma HLS dependence variable=read_data inter false
////#pragma HLS dependence variable=done inter false
//		read_data = output[read_stream_ptr]; // read from gmem
//		read_stream_ptr++;
//		read_stream_ptr = read_stream_ptr % BUFFER_SIZE;
//		fifo.write(read_data);
////		done = read_data;
//	} // read operator
//}
//
//void write_operator( volatile uint64_t* output, hls::stream<uint64_t> &fifo, uint32_t bftstream[100])
//{
//#pragma HLS INLINE
//
//	uint8_t write_stream_ptr=0;
//	uint64_t write_data = 0;
//	uint64_t done = 0;
//	uint8_t delay = 0;
//
//	write_loop:for(int i = 0; i < 100; i++)
//	{
//#pragma HLS pipeline II=1
////#pragma HLS dependence variable=done inter false
////#pragma HLS dependence variable=write_stream_ptr inter false
//
//		write_data = fifo.read(); // read from global memory
////		done = write_data;
//		// bit is high meaning that the data is ready to be consumed
//		if( (write_data & PRESENCE_BIT) == PRESENCE_BIT )
//		{
//			delay++;
//		}
//		else
//		{
//			uint32_t bft_data = bftstream[write_stream_ptr]; // read from application stream
//
//			// set the bit and write the data to global memory
//			output[write_stream_ptr] = bft_data | PRESENCE_BIT;
//			write_stream_ptr++;
//			write_stream_ptr = write_stream_ptr % BUFFER_SIZE;
//		}
//	} // write operator
//
//}

void example_rx_piped64(volatile uint64_t *output,/*uint32_t instream[BUFFER_SIZE]*/ hls::stream<uint64_t> &fifo)
{

#pragma HLS INTERFACE m_axi num_write_outstanding=128 max_write_burst_length=128 port=output offset=slave bundle=gmem_out

#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

//#pragma HLS INTERFACE axis port=instream bundle=stream_in
	#pragma HLS INTERFACE axis port=fifo bundle=stream_in
	#pragma HLS STREAM depth=100 variable=fifo

	uint8_t stream_head=0;
	uint8_t stream_tail=0;
	uint64_t data[BUFFER_SIZE];
	uint16_t i=0;
	uint8_t bytes_to_write=0;
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
//			for(int i = 0; i < 10; i++)
//			{
//				cycles++;
//			}
        }
        else
        {
        	//
        	// compute how many bytes it is safe to write
        	int bytes_to_write = stream_tail - (stream_head+1);

        	//
        	// if bytes a negative occurs this is the case where head is 90 and tail has wrapped around
        	// recompute
        	if(bytes_to_write < 0){
        		bytes_to_write = BUFFER_SIZE - (stream_head+1) + stream_tail;
        	}

        	//
			// head has wrapped around but tail has not tail = 90
			// head = 10
//			if( (stream_head+1) < stream_tail)
//			{
//				bytes_to_write = stream_tail - (stream_head+1);
//			}
//			//
//			// typical case where head is leading
//			else
//			{
//				bytes_to_write = BUFFER_SIZE - (stream_head+1) + stream_tail;
//			}

        	//output[DEBUG1] = bytes_to_write;

        	fifo_read:for(int h=0; h < bytes_to_write; h++)
        	{
//#pragma HLS pipeline II=1
        		// write to gmem from app stream
//				data = instream[stream_head];  //grab a byte from the buffer
        		data[h] = fifo.read();
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
