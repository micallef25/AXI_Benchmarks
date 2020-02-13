#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "../Overlays/stream.h"

#define NUM_ELEMENTS 4

int run_benchmark( int buffer_size, memory_type memory, int time, axi_port_type port );
int run_benchmark_memory( int buffer_size, memory_type memory, int time, axi_port_type port );

typedef struct wide_dt_struct{
    int data[NUM_ELEMENTS];
} __attribute__ ((packed, aligned(4))) wide_dt;

#endif
