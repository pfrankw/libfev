#ifndef FEV_TIME_H
#define FEV_TIME_H

#include <stdint.h>

// Returns the clock, in milliseconds
uint64_t fev_clock();
void fev_sleep_usec(uint64_t usec);


#endif
