#ifndef AXI_PMON_H
#define AXI_PMON_H

#include "xparameters.h"
#include "xstatus.h"
#include "xaxipmon.h"

int Setup_AxiPmon( int slot );
int Shutdown_AxiPmon(u32 *Metrics, u32 *ClkCntHigh,u32 *ClkCntLow);
u32 query_metric( u32 metric );

#define TOTAL_WRITES			XAPM_METRIC_SET_0 	/**< Write Transaction Count */
#define TOTAL_READS				XAPM_METRIC_SET_1 	/**< Read Transaction Count */
#define BYTES_WRITTEN			XAPM_METRIC_SET_2	/**< Write Byte Count */
#define BYTES_READ				XAPM_METRIC_SET_3 	/**< Read Byte Count */
#define WRITE_BEAT_COUNT		XAPM_METRIC_SET_4 	/**< Write Beat Count */
#define READ_LATENCY		XAPM_METRIC_SET_5	/**< Total Read Latency */
#define WRITE_LATENCY		XAPM_METRIC_SET_6 	/**< Total Write Latency */

#endif
