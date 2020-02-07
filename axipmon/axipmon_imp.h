#ifndef AXI_PMON_H
#define AXI_PMON_H

#include "xparameters.h"
#include "xstatus.h"

int Setup_AxiPmon();
int Shutdown_AxiPmon(u32 *Metrics, u32 *ClkCntHigh,u32 *ClkCntLow);

#endif
