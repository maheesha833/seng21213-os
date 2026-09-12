; SENG21213-OS :: Context switch + thread entry stub
; L09 §3, L10 §2
BITS 32
section .text

extern thread_exit
global switch_context
global thread_entry_stub

; void switch_context(uint32_t *old_esp, uint32_t new_esp)
switch_context:
    pushad
    mov eax, [esp + 36]
    mov edx, [esp + 40]
    mov [eax], esp
    mov esp, edx
    popad
    ret

; First-time entry for a new thread.
; Stack layout when reached (after popad;ret in switch_context):
;   [esp+0] = fn  (fn address)
;   [esp+4] = arg
thread_entry_stub:
    pop eax                 ; eax = fn
    pop ebx                 ; ebx = arg
    push ebx                ; push arg (cdecl)
    call eax                ; fn(arg)
    add esp, 4              ; cdecl cleanup
    jmp thread_exit         ; never returns
