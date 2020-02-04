# AXI_Benchmarks
Benchmarks for sending data from PS to PL and vice versa

# Todos

* [x] Get basic loop back working for baremetal
* [x] Test cache coherency for baremetal (confirmed for HPC0 and HPC1 ACE is a different story ...)
* [x] get OCM working for baremetal
* [ ] Add AXU PMU for performance measurements
* [ ] Create read write API's 
* [ ] Add pictures 
* [ ] Tidy up readme making how to steps clear

# Initial loop back test

the loopback_bsp.cpp and loobpack HLS code will pair together to run a simple loop back test that can demonstrate the following:
	Cache Coherency
	Handling PL-PS Interrupts
	Minimal Execution to kick off the FPGA


While the loopback test is simple in nature it is a good starting point and explains some concepts that are otherwise neglected or not explained well in the documentation.

The following code should be run with the HPC0 or HPC1 ports as these have the ability to snoop the caches of the PS since they pass through CCI.

Some information of the code that is helpful for understanding how and why it works.

```c
	// Below are C generated drivers provided by Vivado HLS. When you choose the 
	// s_axilite interface you will get a set of drivers to communicate to the PL
	// depending on the name of your application it may be called something slightly different rather than
	// XExample. the IsReady queries the PL to see if it can accept input
	// Set_input() tells the PL where to read from memory this is important. it sets the ptr thatI have called in my application 'input' 
	// Set_ouput() does the same.
	// the IsDone() function allows you to poll your PL to see when it has exited the HLS function you created
	// the start function tells the PL to begin running
	// more information on the generated API's can be found here: 
	// https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_4/ug902-vivado-high-level-synthesis.pdf page 503
    printf("XExample_IsReady() %d\n",XExample_IsReady(AXI_Config));
    XExample_Set_input_r(AXI_Config,(u32)input);
    printf("XExample_IsReady() %d\n",XExample_IsDone(AXI_Config));
    XExample_Set_output_r(AXI_Config,(u32)output);
    XExample_Start(AXI_Config);
```

```c
    // sets the addresses to outershareable so the PL can see the data
    Xil_SetTlbAttributes(input, 0x605);
    Xil_SetTlbAttributes(output, 0x605);
    // enables snooping for the HPC0 and HPC1 ports
    Xil_Out32(0xFD6E4000,0x1);
    dmb();
    // More information can be found here:
    // https://www.xilinx.com/support/answers/69446.html
    // https://community.arm.com/developer/ip-products/processors/f/cortex-a-forum/3238/the-exact-definition-of-outer-and-inner-in-armv7
```

```c
void example(volatile int *input, volatile int* output){

// this pragma defines a master interface to be created the offset slave is important because it allows us to control from the PS where these pointers point at
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem1
// The s-axilite interface allows us to create a port which can set the pointers, start the PL query the PL and have some interaction with it
//
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

// More information can be found here:
// https://www.xilinx.com/html_docs/xilinx2017_4/sdaccel_doc/jit1504034365862.html
// https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/m-axi-interfaces-optional-depth-Offset-types/m-p/1069538#M19564
// 
```

## Running the Test.

TODO explain this better / more thoroughly 

Open Vivado HLS use the HLS code and export the RTL in HLS. This will synthesize and create the IP block that you need for vivado.
Open Vivado, add a new IP repository and point it towards your HLS project.
Expose a HPC0 or HPC1 port on the PS
Run the automation wizard

Important! Double click on the HLS IP and set the Cache bits high as seen in image 


Important! Double click on the PS IP block and enable CCI for the port exposed see image.

Generate Bit stream
Export hardware
Launch SDK
Copy my code over make any changes (your driver generated code may be different)

Run and check output

TODO image of my Vivado System 


## How to use OCM 

Rather simple but worth giving a shout out since it took me a bit to do. in the address editor you can drag and drop the OCM port into the non excluded segment. I tested with pointers defined like this. 

TODO add screen shot of before and after OCM 


```c 
#define OCM_BUFF1 (0xFFFC0000)
#define OCM_BUFF2 (0xFFFD0000)

volatile int* ocm_buff1 = (int*)OCM_BUFF1;
volatile int* ocm_buff2 = (int*)OCM_BUFF2;
```

# References
 I used many references including the forums... here are some that I found helpful

 https://www.xilinx.com/html_docs/xilinx2017_4/sdaccel_doc/jit1504034365862.html
 https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/m-axi-interfaces-optional-depth-Offset-types/m-p/1069538#M19564
 https://www.xilinx.com/support/answers/69446.html
 https://community.arm.com/developer/ip-products/processors/f/cortex-a-forum/3238/the-exact-definition-of-outer-and-inner-in-armv7
 https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_4/ug902-vivado-high-level-synthesis.pdf
 https://www.xilinx.com/support/documentation/user_guides/ug1085-zynq-ultrascale-trm.pdf
 https://www.zynq-mpsoc-book.com/wp-content/uploads/2019/04/MPSoC_ebook_web_v1.0.pdf
 http://infocenter.arm.com/help/topic/com.arm.doc.ddi0470i/DDI0470I_cci400_r1p3_trm.pdf