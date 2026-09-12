#ifndef PROCESS_H
#define PROCESS_H
#include "../include/types.h"
#define MAX_PROCS  16
#define STACK_SIZE 4096

typedef enum {
    PROC_READY = 0, PROC_RUNNING = 1, PROC_BLOCKED = 2, PROC_ZOMBIE = 3
} proc_state_t;

typedef struct pcb {
    uint32_t     pid;
    proc_state_t state;
    uint32_t     esp;
    uint32_t     eip;
    char         name[32];
    struct pcb  *next;
    struct pcb  *wait_next;
} pcb_t;

void   process_init(void);
pcb_t *create_process(void (*entry)(void), const char *name);
pcb_t *process_alloc_stack(pcb_t **out_pcb, const char *name);
void   process_add_to_list(pcb_t *p);      /* NEW */
pcb_t *get_current(void);
void   set_current(pcb_t *p);
pcb_t *get_proc_list(void);
const char *state_str(proc_state_t s);
#endif
