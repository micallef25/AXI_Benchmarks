
//#define COMPILE_LOOPBACK_TEST

#ifdef COMPILE_LOOPBACK_TEST

#include <stdio.h>
#include "platform.h"
#include "xexample.h"
#include <stdlib.h>
#include "xil_cache.h"
#include "xil_mmu.h"
#include "xpseudo_asm.h"

#include "xil_types.h"

#include "xparameters.h"

#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"

#define PSU_OCM_RAM_0   (0xFFFC0000)
#define OCM_BUFF1 PSU_OCM_RAM_0
#define OCM_BUFF2 (0xFFFD0000)

volatile int* ocm_buff1 = (int*)OCM_BUFF1;
volatile int* ocm_buff2 = (int*)OCM_BUFF2;

void delay(int delay)
{
    for(int i = 0; i < delay; i++)
    {
        // spin
    }
}

volatile int count = 0;
XScuGic InterruptController; /* Instance of the Interrupt Controller */
static XScuGic_Config *GicConfig;/* The configuration parameters of the controller */
XExample* AXI_Config;

void XTmrCtr_InterruptHandler()
{
    printf("XExample_IsDone() %d\n",XExample_IsDone(AXI_Config));
    printf("XExample_InterruptGetStatus() %d\n",XExample_InterruptGetStatus(AXI_Config));
    XExample_InterruptClear(AXI_Config,XExample_InterruptGetStatus(AXI_Config));
    count = 10;
}

int SetUpInterruptSystem(XScuGic *XScuGicInstancePtr)
{
    /*
    * Connect the interrupt controller interrupt handler to the hardware
    * interrupt handling logic in the ARM processor.
    */
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler) XScuGic_InterruptHandler,XScuGicInstancePtr);
    /*
    * Enable interrupts in the ARM
    */
    Xil_ExceptionEnable();
    return XST_SUCCESS;
}

int ScuGicInterrupt_Init(u16 DeviceId, XExample* AXI_Config)
{
int Status;
/*
* Initialize the interrupt controller driver so that it is ready to
* use.
* */
    GicConfig = XScuGic_LookupConfig(DeviceId);
    if (NULL == GicConfig) {
    return XST_FAILURE;
    }
    Status = XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS)
    {
        return XST_FAILURE;
    }
/*
* Setup the Interrupt System
* */
    Status = SetUpInterruptSystem(&InterruptController);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }
/*
* Connect a device driver handler that will be called when an
* interrupt for the device occurs, the device driver handler performs
* the specific interrupt processing for the device
*/
    Status = XScuGic_Connect(&InterruptController,XPAR_FABRIC_EXAMPLE_0_INTERRUPT_INTR,(Xil_ExceptionHandler)XTmrCtr_InterruptHandler,(void *)AXI_Config);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }
/*
* Enable the interrupt for the device and then cause (simulate) an
* interrupt so the handlers will be called
*/
    XScuGic_Enable(&InterruptController, XPAR_FABRIC_EXAMPLE_0_INTERRUPT_INTR);
    return XST_SUCCESS;
}

int main()
{
    //
    printf("Starting Loop Back Test \n");

    //
    init_platform();

    AXI_Config = malloc(sizeof(XExample));
    if(AXI_Config == NULL)
    {
        printf("malloc not ok\n");
        return 0;
    }

    int rval = XExample_Initialize(AXI_Config, XPAR_EXAMPLE_0_DEVICE_ID);
    if(rval != XST_SUCCESS)
    {
        printf("error in init\n");
        return 0;
    }

    // enables interrupts from the PL
    ScuGicInterrupt_Init(XPAR_EXAMPLE_0_DEVICE_ID,AXI_Config);
    XExample_InterruptGlobalEnable(AXI_Config);
    XExample_InterruptEnable(AXI_Config,1);

    volatile int* input = malloc(sizeof(int)* 100);
    if(input == NULL)
    {
        printf("malloc not ok\n");
        return 0;
    }

    volatile int* output = malloc(sizeof(int)* 100);
    if(output == NULL)
    {
        printf("malloc not ok\n");
        return 0;
    }

    // clear our memory, this should also cause for this array to go into the cache
    // thus we will be testing coherency among PL and PS
    for(int i = 0; i < 100; i++)
    {
        input[i] = 0;
    }


    // slave 3 from CCI man page. This enables snooping from the HPC0 and 1 ports
    Xil_Out32(0xFD6E4000,0x1);
    dmb();

    // mark our memory regions as outer shareable which means it will not live in L1 but L2
    //
    Xil_SetTlbAttributes(input, 0x605);
    Xil_SetTlbAttributes(output, 0x605);


    // point the input ptr to a memory address
    printf("XExample_IsReady() %d\n",XExample_IsReady(AXI_Config));
    XExample_Set_input_r(AXI_Config,(u32)input);
    delay(1000);

    // point the output to a memory address
    printf("XExample_IsReady() %d\n",XExample_IsReady(AXI_Config));
    XExample_Set_output_r(AXI_Config,(u32)output);
    delay(1000);

    // write some unique value
    for(int i = 0; i < 100; i++)
    {
        input[i] = i;
    }

    // tell the PL to start running
    XExample_Start(AXI_Config);

    // wait a bit, you can quiry the isdone port and should see the fpga still spinning
    delay(1000);

    // write the magic flag
    input[99] = 0xdeadbeef;

    // delays are technically not necessary as I get an interrupt when the PL is done
    delay(10000);

    // wait for interrupt to occur print results
    while(count != 10)
    {
        // spin
    }


    for(int i = 0; i < 100; i++)
    {
        printf("out[%d] %x\n",i,output[i]);
    }

    printf("cleaning up\n");
    free(input);
    free(output);

    cleanup_platform();
    return 0;
}


#endif
