/* =============================================================================
 * SENG21213-OS :: Linker Script
 * File   : linker.ld
 *
 * Places the kernel binary starting at physical address 0x10000.
 * Uses explicit PHDRS to give .text/.rodata R+X and .data/.bss R+W
 * permissions, silencing the modern-ld "LOAD segment with RWX permissions"
 * warning.
 * ============================================================================*/

OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);   /* R + X */
    data PT_LOAD FLAGS(6);   /* R + W */
}

SECTIONS {
    . = 0x10000;

    .text : {
        *(.text)
    } :text

    .rodata : {
        *(.rodata*)
    } :text

    .data : {
        *(.data)
    } :data

    .bss : {
        *(COMMON)
        *(.bss)
    } :data

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}
