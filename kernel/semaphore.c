/* L10 §3 — Counting semaphore */
#include "semaphore.h"
#include "scheduler.h"
#include "io.h"

void sem_init(semaphore_t *s, int count) { s->count = count; s->waiters = 0; }

void sem_wait(semaphore_t *s) {
    cli();
    if (s->count > 0) { s->count--; sti(); return; }
    pcb_t *cur = get_current();
    cur->state     = PROC_BLOCKED;
    cur->wait_next = s->waiters;
    s->waiters     = cur;
    sti();
    yield();
}

void sem_signal(semaphore_t *s) {
    cli();
    if (s->waiters) {
        pcb_t *w = s->waiters;
        s->waiters = w->wait_next;
        w->wait_next = 0;
        w->state = PROC_READY;
    } else {
        s->count++;
    }
    sti();
}
