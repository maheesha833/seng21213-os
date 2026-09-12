#ifndef THREAD_H
#define THREAD_H
#include "process.h"

pcb_t *thread_create(void (*fn)(void *), void *arg, const char *name);
void   thread_exit(void);

#endif
