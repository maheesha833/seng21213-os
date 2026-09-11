#include "idt.h"
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;
extern void isr_irq0(void);
static void idt_load(void) {
    __asm__ __volatile__("lidt (%0)" : : "r"(&idt_ptr) : "memory");
}
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (uint16_t)(handler & 0xFFFF);
    idt[num].base_high = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[num].selector  = sel;
    idt[num].zero      = 0;
    idt[num].flags     = flags;
}
void idt_init(void) {
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].base_low = 0; idt[i].base_high = 0;
        idt[i].selector = 0; idt[i].zero = 0; idt[i].flags = 0;
    }
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;
    idt_set_gate(0x20, (uint32_t)isr_irq0, 0x08, 0x8E);
    idt_load();
}
