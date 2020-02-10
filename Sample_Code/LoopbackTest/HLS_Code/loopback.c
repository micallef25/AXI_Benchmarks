/*
* Basic loop back example
*/

//#define COMPILE_LOOPBACK_TEST

#ifdef COMPILE_LOOPBACK_TEST

void example(volatile int *input, volatile int* output)
{

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem1

#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control


	int data = 0x00;
	int i = 0;
	while(data != 0xdeadbeef)
	{
		data = input[i]; // read from global memory
		output[i] = data; // write to output
		i++;
		if(i == 100)
		{
			i = 0;
		}
	}
 }


#endif
