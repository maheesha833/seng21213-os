BITS 32
section .text
extern irq0_handler
global isr_irq0
isr_irq0:
    pusha
    call irq0_handler
    popa
    iret
