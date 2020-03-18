#include <stdio.h>
#include <stdint.h>
#include <hls_stream.h>

//#define DONE ( 0x2000000000 ) // 1 << 32
//#define PRESENCE_BIT ( 0x100000000 ) // 1 << 32
//#define MASK32 (0xFFFFFFFF) // (1 << 32) - 1

#define DEBUG1 100
#define DEBUG2 101

enum meta_data_t{
	RAW_BUFFER_SIZE = 258,
	HEAD_POINTER = 257,
	TAIL_POINTER = 256,
	BUFFER_SIZE = 256,
	MASK = 255,
};

//
//void read_operator_piped( volatile uint64_t* input, hls::stream<uint64_t> &fifo)
//{
//#pragma HLS INLINE
//
//	uint8_t stream_ptr=0;
//	uint64_t data = 0;
//	uint64_t done = 0;
//	read_loop:for(int i = 0; i < 100; i++)
//	{
//#pragma HLS pipeline II=1
////#pragma HLS dependence variable=data inter false
////#pragma HLS dependence variable=done inter false
//		data = input[stream_ptr]; // read from gmem
//		stream_ptr++;
//		stream_ptr = stream_ptr % BUFFER_SIZE;
//		fifo.write(data);
////		done = data;
//	}
//}
//
//void write_operator_piped( volatile uint64_t* input, hls::stream<uint64_t> &fifo, uint32_t bftstream[100])
//{
//#pragma HLS INLINE
//
//	uint64_t data = 0x00;
//	uint8_t stream_ptr = 0;
//
//	uint64_t done = 0;
//	write_loop:for(int i = 0; i < 100; i++)
//	{
//#pragma HLS pipeline II=1
////#pragma HLS dependence variable=data inter false
////#pragma HLS dependence variable=stream_ptr inter false
//
//		data = fifo.read(); // read from global memory
//
//		// bit is high meaning that the data is ready to be consumed
//		if( (data & PRESENCE_BIT) == PRESENCE_BIT )
//		{
//			input[stream_ptr] = 0; // clear presence bit in shared memory
//			bftstream[stream_ptr] = (data & MASK32); // write to BFT
//			stream_ptr++;
//			stream_ptr = stream_ptr % BUFFER_SIZE;
//		}
////		done = data;
//	}
//}
//
void example_tx_piped64( volatile uint64_t* input,/*uint32_t instream[BUFFER_SIZE]*/ hls::stream<uint64_t> &fifo )
{

#pragma HLS INTERFACE m_axi num_read_outstanding=128 max_read_burst_length=128 port=input offset=slave bundle=gmem_read

#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

//#pragma HLS INTERFACE axis port=outstream bundle=stream_out
#pragma HLS INTERFACE axis port=fifo bundle=stream_out
#pragma HLS STREAM depth=100 variable=fifo

	uint8_t stream_head=0;
	uint8_t stream_tail=0;
	uint64_t data[BUFFER_SIZE];
	uint16_t i=0;
	uint8_t bytes_read = 0;
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
//				for(int i = 0; i < 10; i++)
//				{
//					cycles++;
//				}
	        }
			else
			{

			    //
				// compute how many bytes we can read
				int bytes_read = (stream_head - stream_tail);

				//
				// if the amount is negative this is the case where our head ptr has wrapped around
				// but tail pointer has not
				if(bytes_read < 0){
					bytes_read = ((BUFFER_SIZE - stream_tail) + stream_head );
				}

//				//
//				// no wrap around
//				if(stream_head > stream_tail)
//				{
//					bytes_read = (stream_head - stream_tail);
//				}
//				//
//				// head has wrapped around but tail has not tail = 90
//				// head = 10
//				else
//				{
//					bytes_read = ((BUFFER_SIZE - stream_tail) + stream_head );
//				}

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
//#pragma HLS pipeline II=1
					//
					fifo.write(data[h]);
				}



	            // write our new updated position
	            input[TAIL_POINTER] = stream_tail;
			}
		}// while 1
 }


