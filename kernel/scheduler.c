/* SENG21213-OS :: Round-robin scheduler — L09 §3–4
 * Fixed: disable IRQs during the critical context-switch section
 * to prevent reentrant scheduler_tick() calls. */
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
    cli_();                              /* critical section start */
    pcb_t *cur = get_current();
    if (!cur) { sti_(); return; }
    pcb_t *next = pick_next(cur);
    if (!next) { sti_(); return; }

    if (cur->state == PROC_RUNNING) cur->state = PROC_READY;
    next->state = PROC_RUNNING;
    set_current(next);

    switch_context(&cur->esp, next->esp);

    /* Resumed: re-enable IRQs on our stack */
    sti_();
}

void yield(void) { scheduler_tick(); }

void irq0_handler(void) {
    pic_send_eoi(0);
    scheduler_tick();
}
