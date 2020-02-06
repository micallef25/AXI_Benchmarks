#ifndef BENCH_HELPERS
#define BENCH_HELPERS


enum MEMORY_ENUM{
	DDR = 0, // data stored in DDR
	OCM = 1, // use OCM 
	L2 = 2,
	INVALID_MEMORY,
}MEMORY_TYPE;

enum PORT_SETTING{
	HPC0 = 0,
	HPC1,
	HP1,
	HP2,
	HP3,
	HP4,
	ACE,
	ACP,
	INVALID_PORT,
}



#endif