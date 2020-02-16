/******************************************************************************
*
* Copyright (C) 2012 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/****************************************************************************/
/**
*
* @file xaxipmon_polled_example.c
*
* This file contains a design example showing how to use the driver APIs of the
* AXI Performance Monitor driver in poll mode.
*
*
* @note
*
* Global Clock Counter and Metric Counters are enabled. The Application
* for which Metrics need to be computed should be run and then the Metrics
* collected.
*
*
* <pre>
*
* MODIFICATION HISTORY:
*
* Ver   Who    Date     Changes
* ----- -----  -------- -----------------------------------------------------
* 1.00a bss    02/29/12 First release
* 2.00a bss    06/23/12 Updated to support v2_00a version of IP.
* 3.00a bss    09/03/12 Deleted XAxiPmon_SetAgent API to support
*						v2_01a version of IP.
* 3.01a bss	   10/25/12 Deleted XAxiPmon_EnableCountersData API to support
*						new version of IP.
* 6.5   ms    01/23/17 Modified xil_printf statement in main function to
*                      ensure that "Successfully ran" and "Failed" strings are
*                      available in all examples. This is a fix for CR-965028.
* </pre>
*
*****************************************************************************/

/***************************** Include Files ********************************/

#include "xaxipmon.h"
#include "xparameters.h"
#include "xstatus.h"
#include "stdio.h"

/************************** Constant Definitions ****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define AXIPMON_DEVICE_ID 	XPAR_AXIPMON_0_DEVICE_ID


/**************************** Type Definitions ******************************/


/***************** Macros (Inline Functions) Definitions ********************/

/************************** Function Prototypes *****************************/

// int AxiPmonPolledExample(u16 AxiPmonDeviceId, u32 *Metrics, u32 *ClkCntHigh,
// 							     u32 *ClkCntLow);

/************************** Variable Definitions ****************************/

static XAxiPmon AxiPmonInst;      /* System Monitor driver instance */

/****************************************************************************/
/**
*
* This function runs a test on the AXI Performance Monitor device using the
* driver APIs.
* This function does the following tasks:
*	- Initiate the AXI Performance Monitor device driver instance
*	- Run self-test on the device
*	- Sets Metric Selector to select Slave Agent Number 1 and first set of
*	Metrics
*	- Enables Global Clock Counter and Metric Counters
*	- Calls Application for which Metrics need to be computed
*	- Disables Global Clock Counter and Metric Counters
*	- Read Global Clock Counter and Metric Counter 0
*
* @param	AxiPmonDeviceId is the XPAR_<AXIPMON_instance>_DEVICE_ID value
*		from xparameters.h.
* @param	Metrics is an user referece variable in which computed metrics
*			will be filled
* @param	ClkCntHigh is an user referece variable in which Higher 64 bits
*			of Global Clock Counter are filled
* @param	ClkCntLow is an user referece variable in which Lower 64 bits
*			of Global Clock Counter are filled
*
* @return
*		- XST_SUCCESS if the example has completed successfully.
*		- XST_FAILURE if the example has failed.
*
* @note   	None
*
****************************************************************************/
int Setup_AxiPmon( int slot )
{
	int Status;
	XAxiPmon_Config *ConfigPtr;
	u8 SlotId = 0x0;

	u8 Metric =0;
	//u16 Range2 = 0x20;	/* Range 2 - 16 */
	//u16 Range1 = 0x20;	/* Range 1 - 8 */
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;
	u16 AxiPmonDeviceId = AXIPMON_DEVICE_ID;
	/*
	 * Initialize the AxiPmon driver.
	 */
	ConfigPtr = XAxiPmon_LookupConfig(AxiPmonDeviceId);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}

	XAxiPmon_CfgInitialize(AxiPmonInstPtr, ConfigPtr,
				ConfigPtr->BaseAddress);

	/*
	 * Self Test the System Monitor/ADC device
	 */
	Status = XAxiPmon_SelfTest(AxiPmonInstPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Select Agent and required set of Metrics for a Counter.
	 * We can select another agent,Metrics for another counter by
	 * calling below function again.
	 */

	if( slot == 1){
	//
	// read from memory calcs
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 0, XAPM_METRIC_SET_5,XAPM_METRIC_COUNTER_0);
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 0, XAPM_METRIC_SET_1,XAPM_METRIC_COUNTER_1);

	//
	// write to memory
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 1, XAPM_METRIC_SET_6,XAPM_METRIC_COUNTER_2);
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 1, XAPM_METRIC_SET_0,XAPM_METRIC_COUNTER_3);
	}

	if(slot == 0){
	// read from memory calcs
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 2, XAPM_METRIC_SET_5,XAPM_METRIC_COUNTER_0);
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 2, XAPM_METRIC_SET_1,XAPM_METRIC_COUNTER_1);
//
//	//
//	// write to memory
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 3, XAPM_METRIC_SET_6,XAPM_METRIC_COUNTER_2);
	XAxiPmon_SetMetrics(AxiPmonInstPtr, 3, XAPM_METRIC_SET_0,XAPM_METRIC_COUNTER_3);
	}


//	XAxiPmon_StartCounters(AxiPmonInstPtr, 1000);


//	XAxiPmon_GetMetrics(AxiPmonInstPtr,XAPM_METRIC_COUNTER_0,&Metric,&SlotId);
//	printf("---slot %d\n",SlotId);
//	printf("Metric %d\n",Metric);
//	XAxiPmon_GetMetrics(AxiPmonInstPtr,XAPM_METRIC_COUNTER_1,&Metric,&SlotId);
//	printf("---slot %d\n",SlotId);
//	printf("Metric %d\n",Metric);
//	XAxiPmon_GetMetrics(AxiPmonInstPtr,XAPM_METRIC_COUNTER_2,&Metric,&SlotId);
//	printf("---slot %d\n",SlotId);
//	printf("Metric %d\n",Metric);
//	XAxiPmon_GetMetrics(AxiPmonInstPtr,XAPM_METRIC_COUNTER_3,&Metric,&SlotId);
//	printf("---slot %d\n",SlotId);
//	printf("Metric %d\n",Metric);
	/*
	 * Set Incrementer Ranges
	 */
//	XAxiPmon_SetIncrementerRange(AxiPmonInstPtr, XAPM_INCREMENTER_0,Range2, Range1);
//	XAxiPmon_SetIncrementerRange(AxiPmonInstPtr, XAPM_INCREMENTER_1,Range2, Range1);
//	XAxiPmon_SetIncrementerRange(AxiPmonInstPtr, XAPM_INCREMENTER_2,Range2, Range1);
//	XAxiPmon_SetIncrementerRange(AxiPmonInstPtr, XAPM_INCREMENTER_3,Range2, Range1);
	//XAxiPmon_SetIncrementerRange(AxiPmonInstPtr, XAPM_INCREMENTER_1,Range2, Range1);
	/*
	 * Enable Metric Counters.
	 */
	XAxiPmon_EnableMetricsCounter(AxiPmonInstPtr);

	/*
	 * Enable Global Clock Counter Register.
	 */
	XAxiPmon_EnableGlobalClkCounter(AxiPmonInstPtr);

	XAxiPmon_StartCounters(AxiPmonInstPtr, 0x64);

	return XST_SUCCESS;

}

int Shutdown_AxiPmon(u32 *Metrics, u32 *ClkCntHigh,u32 *ClkCntLow)
{
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;
	u8 SlotId = 0x0;
	u8 Metric =0;
	/*
	 * Disable Global Clock Counter Register.
	 */

	XAxiPmon_DisableGlobalClkCounter(AxiPmonInstPtr);

	/*
	 * Disable Metric Counters.
	 */
	XAxiPmon_DisableMetricsCounter(AxiPmonInstPtr);

//	/* Get Metric Counter 0  */
//	XAxiPmon_GetMetrics(AxiPmonInstPtr,XAPM_METRIC_COUNTER_1,&Metric,&SlotId);
////	printf("slot %d\n",SlotId);
////	printf("Metric %d\n",Metric);
//	XAxiPmon_GetMetrics(AxiPmonInstPtr,XAPM_METRIC_COUNTER_0,&Metric,&SlotId);
////	printf("Metric %d\n",Metric);
////	printf("slot %d\n",SlotId);
//	/* Get Metric Counter 0  */
//	*Metrics = XAxiPmon_GetMetricCounter(AxiPmonInstPtr,XAPM_METRIC_COUNTER_0);
//
////	printf("Latency %lu\n",*Metrics);
//
//	/* Get Metric Counter 1  */
//	*Metrics = XAxiPmon_GetMetricCounter(AxiPmonInstPtr,XAPM_METRIC_COUNTER_1);
//
////	printf("Bytes_Read %lu\n",*Metrics);

	/* Get Global Clock Cycles Count in ClkCntHigh,ClkCntLow */
	XAxiPmon_GetGlobalClkCounter(AxiPmonInstPtr, ClkCntHigh, ClkCntLow);


	return XST_SUCCESS;
}

u32 query_metric( u32 metric )
{
	XAxiPmon *AxiPmonInstPtr = &AxiPmonInst;
	return XAxiPmon_GetMetricCounter(AxiPmonInstPtr,metric);
}
