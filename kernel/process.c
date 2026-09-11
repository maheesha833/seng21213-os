#include "process.h"
static pcb_t   table[MAX_PROCS];
static uint8_t stacks[MAX_PROCS][STACK_SIZE] __attribute__((aligned(16)));
static int     next_pid = 1;
static pcb_t  *current  = 0;
static pcb_t  *list     = 0;

void process_init(void) {
    for (int i = 0; i < MAX_PROCS; i++) table[i].pid = 0;
    next_pid = 1; current = 0; list = 0;
}
pcb_t *get_current(void)     { return current; }
void   set_current(pcb_t *p) { current = p; }
pcb_t *get_proc_list(void)   { return list; }

const char *state_str(proc_state_t s) {
    switch (s) {
        case PROC_READY:   return "READY";
        case PROC_RUNNING: return "RUNNING";
        case PROC_BLOCKED: return "BLOCKED";
        case PROC_ZOMBIE:  return "ZOMBIE";
        default:           return "?";
    }
}

pcb_t *create_process(void (*entry)(void), const char *name) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCS; i++)
        if (table[i].pid == 0) { slot = i; break; }
    if (slot < 0) return 0;
    pcb_t *p = &table[slot];
    p->pid   = next_pid++;
    p->state = PROC_READY;
    p->eip   = (uint32_t)entry;
    int i = 0;
    while (name[i] && i < 31) { p->name[i] = name[i]; i++; }
    p->name[i] = '\0';
    uint32_t *sp = (uint32_t *)&stacks[slot][STACK_SIZE];
    *--sp = (uint32_t)entry;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    p->esp = (uint32_t)sp;
    p->next = 0;
    if (!list) list = p;
    else { pcb_t *t = list; while (t->next) t = t->next; t->next = p; }
    return p;
}
