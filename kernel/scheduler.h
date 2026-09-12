#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "process.h"

void scheduler_init(void);
void scheduler_tick(void);
void yield(void);
void sleep_ms(uint32_t ms);   /* Extension: L09 section 3 */

#endif
