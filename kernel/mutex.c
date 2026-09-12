/* L10 §3 — Blocking mutex with FIFO waiter queue */
#include "mutex.h"
#include "scheduler.h"
#include "io.h"

void mutex_init(mutex_t *m) { m->locked = 0; m->waiters = 0; }

void mutex_lock(mutex_t *m) {
    cli();
    if (!m->locked) {
        m->locked = 1;
        sti();
        return;
    }
    /* Block: enqueue current process */
    pcb_t *cur = get_current();
    cur->state     = PROC_BLOCKED;
    cur->wait_next = m->waiters;
    m->waiters     = cur;
    sti();
    yield();
    /* On wake: ownership has been handed to us by mutex_unlock() */
}

void mutex_unlock(mutex_t *m) {
    cli();
    if (m->waiters) {
        pcb_t *w = m->waiters;
        m->waiters = w->wait_next;
        w->wait_next = 0;
        w->state = PROC_READY;
        /* lock ownership handed off — m->locked stays 1 */
    } else {
        m->locked = 0;
    }
    sti();
}
