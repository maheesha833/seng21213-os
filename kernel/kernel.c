#include "vga.h"
#include "keyboard.h"
#include "process.h"
#include "scheduler.h"
#include "../include/types.h"

static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_ps(void);

static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}
static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}
static size_t k_strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static const char *k_ltrim(const char *s) { while (*s == ' ') s++; return s; }

static void proc_a(void) {
    for (;;) {
        vga_putchar('A');
        for (volatile int i = 0; i < 20000000; i++) {}
        yield();
    }
}
static void proc_b(void) {
    for (;;) {
        vga_putchar('B');
        for (volatile int i = 0; i < 40000000; i++) {}
        yield();
    }
}

static void cmd_ps(void) {
    vga_puts("\n  PID   STATE     NAME\n");
    vga_puts("  ----  --------  --------\n");
    pcb_t *p = get_proc_list();
    while (p) {
        vga_printf("  %u     %s     %s\n", p->pid, state_str(p->state), p->name);
        p = p->next;
    }
    vga_puts("\n");
}

static void print_splash(void) {
    vga_clear(VGA_BLACK);
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);
    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems", VGA_YELLOW, VGA_BLACK);
    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 1: Process Scheduler", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_set_cursor(3, 2);
    vga_puts_color("  Department of Software Engineering", VGA_LIGHT_GREY, VGA_BLACK);
    vga_set_cursor(4, 2);
    vga_puts_color("  Type 'help' or 'ps' to begin.", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 | PIT: 100Hz | Round-Robin", VGA_DARK_GREY, VGA_BLACK);
    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  proc_a and proc_b print A/B at different rates via preemption.\n\n");
}

static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  --------------------------------\n");
    vga_puts("  help   - Show this help message\n");
    vga_puts("  clear  - Clear the screen\n");
    vga_puts("  about  - About this OS\n");
    vga_puts("  echo   - Echo text\n");
    vga_puts("  mem    - Memory map (stub)\n");
    vga_puts("  ps     - [L09] List processes\n\n");
}
static void cmd_clear(void) { vga_clear(VGA_BLACK); }
static void cmd_about(void) {
    vga_puts("\n  SENG21213-OS - x86 i686, freestanding C, QEMU\n\n");
}
static void cmd_echo(const char *args) { vga_puts("  "); vga_puts(args); vga_puts("\n"); }
static void cmd_mem(void) {
    vga_puts("\n  Memory Map (stub - PMM in L11)\n");
    vga_puts("  0x00000000 - 0x000FFFFF  reserved\n");
    vga_puts("  0x00100000 - 0x00EFFFFF  extended\n\n");
}

static char shell_buf[256];
static char prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' or 'ps'.\n", VGA_LIGHT_GREEN, VGA_BLACK);
    for (;;) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "about") == 0) { cmd_about(); continue; }
        if (k_strcmp(cmd, "mem")   == 0) { cmd_mem();   continue; }
        if (k_strcmp(cmd, "ps")    == 0) { cmd_ps();    continue; }
        if (k_strncmp(cmd, "echo ", 5) == 0) { cmd_echo(k_ltrim(cmd + 5)); continue; }
        vga_puts_color("  Unknown command. Try 'help'.\n", VGA_LIGHT_RED, VGA_BLACK);
    }
}

void kernel_main(void) {
    vga_init();
    kb_init();
    print_splash();

    process_init();
    create_process(proc_a, "proc_a");
    create_process(proc_b, "proc_b");

    static pcb_t shell_pcb;
    shell_pcb.pid   = 100;
    shell_pcb.state = PROC_RUNNING;
    const char *n = "shell"; int i = 0;
    while (n[i]) { shell_pcb.name[i] = n[i]; i++; }
    shell_pcb.name[i] = 0;
    shell_pcb.next = 0;
    {
        pcb_t *t = get_proc_list();
        while (t->next) t = t->next;
        t->next = &shell_pcb;
    }
    set_current(&shell_pcb);

    scheduler_init();

    shell_run();
    for (;;) __asm__ __volatile__("hlt");
}
