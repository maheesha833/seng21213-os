BITS 32
section .text
global switch_context
switch_context:
    pushad
    mov eax, [esp + 36]
    mov edx, [esp + 40]
    mov [eax], esp
    mov esp, edx
    popad
    ret
