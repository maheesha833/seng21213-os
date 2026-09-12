/* SENG21213-OS :: Round-robin scheduler
 * Extension: sleep_ms() with sorted wake-tick list (L09 section 3) */
#include "scheduler.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"

extern void switch_context(uint32_t *old_esp, uint32_t new_esp);
void irq0_handler(void);

static inline void cli_(void) { __asm__ __volatile__("cli" ::: "memory"); }
static inline void sti_(void) { __asm__ __volatile__("sti" ::: "memory"); }

void scheduler_init(void) {
    pic_remap();
    idt_init();
    pit_init_100hz();
    __asm__ __volatile__("sti");
}

/* ---- Extension: sleep queue ---- */
static volatile uint32_t tick_count = 0;

typedef struct sleeper {
    pcb_t          *p;
    uint32_t        wake_tick;
    struct sleeper *next;
} sleeper_t;

static sleeper_t  sleepers[MAX_PROCS];
static sleeper_t *sleep_head = 0;

void sleep_ms(uint32_t ms) {
    if (ms == 0) { yield(); return; }

    cli_();
    pcb_t *cur = get_current();
    if (!cur) { sti_(); return; }

    int slot = -1;
    for (int i = 0; i < MAX_PROCS; i++) if (!sleepers[i].p) { slot = i; break; }
    if (slot < 0) { sti_(); return; }

    sleeper_t *s = &sleepers[slot];
    s->p = cur;
    s->wake_tick = tick_count + (ms + 9) / 10;
    s->next = 0;

    cur->state = PROC_BLOCKED;

    if (!sleep_head || s->wake_tick < sleep_head->wake_tick) {
        s->next = sleep_head;
        sleep_head = s;
    } else {
        sleeper_t *t = sleep_head;
        while (t->next && t->next->wake_tick <= s->wake_tick) t = t->next;
        s->next = t->next;
        t->next = s;
    }
    sti_();
    yield();
}

static void check_sleepers(void) {
    while (sleep_head && sleep_head->wake_tick <= tick_count) {
        sleeper_t *s = sleep_head;
        sleep_head = s->next;
        if (s->p && s->p->state == PROC_BLOCKED) s->p->state = PROC_READY;
        s->p = 0;
        s->next = 0;
    }
}
/* ---- end extension ---- */

static pcb_t *pick_next(pcb_t *cur) {
    if (!cur) return 0;
    pcb_t *n = cur->next;
    while (n && n->state != PROC_READY && n != cur) n = n->next;
    if (!n || n == cur) {
        n = get_proc_list();
        while (n && n->state != PROC_READY && n != cur) n = n->next;
    }
    return (n && n != cur) ? n : 0;
}

void scheduler_tick(void) {
    cli_();
    pcb_t *cur = get_current();
    if (!cur) { sti_(); return; }
    pcb_t *next = pick_next(cur);
    if (!next) { sti_(); return; }

    if (cur->state == PROC_RUNNING) cur->state = PROC_READY;
    next->state = PROC_RUNNING;
    set_current(next);

    switch_context(&cur->esp, next->esp);
    sti_();
}

void yield(void) { scheduler_tick(); }

void irq0_handler(void) {
    pic_send_eoi(0);
    tick_count++;
    check_sleepers();
    scheduler_tick();
}
