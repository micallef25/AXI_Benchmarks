#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "../Overlays/stream.h"


int run_benchmark( stream::memory_t memory, stream::axi_port_t port );
int run_benchmark_memory( stream::memory_t memory, stream::axi_port_t port );
int run_benchmark_ps_ps_flow( stream::memory_t memory, stream::axi_port_t port );
int run_benchmark_ps_pl_flow( stream::memory_t memory, stream::axi_port_t port );

#endif
