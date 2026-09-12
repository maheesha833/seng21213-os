/* =============================================================================
 * SENG21213-OS :: Kernel threads
 * File   : kernel/thread.c
 * L10 §2 — threads share the process address space but have their own stack
 * ============================================================================*/
#include "thread.h"
#include "scheduler.h"

extern void thread_entry_stub(void);

static uint8_t thread_stacks[MAX_PROCS][STACK_SIZE] __attribute__((aligned(16)));
static int     next_thread_slot = 0;

/* thread_create(fn, arg, name)
 *   Creates a kernel thread that starts executing fn(arg).
 *   Uses the same PCB table as create_process() but with a separate stack
 *   pool so that create_process's slot <-> stack mapping is not disturbed.
 */
pcb_t *thread_create(void (*fn)(void *), void *arg, const char *name) {
    pcb_t *p = 0;
    process_alloc_stack(&p, name);
    if (!p) return 0;

    int tslot = next_thread_slot++;
    if (tslot >= MAX_PROCS) return 0;

    /* Build initial stack. switch_context's popad restores 8 GP regs;
     * then `ret` jumps to thread_entry_stub, which pops fn and arg. */
    uint32_t *sp = (uint32_t *)&thread_stacks[tslot][STACK_SIZE];
    *--sp = (uint32_t)arg;                 /* arg  (popped by stub) */
    *--sp = (uint32_t)fn;                  /* fn   (popped by stub) */
    *--sp = (uint32_t)thread_entry_stub;   /* ret target */
    for (int k = 0; k < 8; k++) *--sp = 0; /* popad filler */

    p->esp = (uint32_t)sp;
    p->eip = (uint32_t)thread_entry_stub;
    return p;
}

void thread_exit(void) {
    pcb_t *cur = get_current();
    if (cur) cur->state = PROC_ZOMBIE;
    for (;;) yield();
}
