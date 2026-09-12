/* =============================================================================
 * SENG21213-OS :: Main Kernel (Stage 0 + 1 + 2 + 3)
 * ============================================================================*/
#include "vga.h"
#include "keyboard.h"
#include "process.h"
#include "scheduler.h"
#include "thread.h"
#include "mutex.h"
#include "semaphore.h"
#include "pmm.h"
#include "../include/types.h"

static void cmd_help(void);
static void cmd_clear(void);
static void cmd_about(void);
static void cmd_echo(const char *args);
static void cmd_mem(void);
static void cmd_ps(void);
static void cmd_race(void);
static void cmd_pc(void);
static void cmd_meminfo(void);
static void cmd_memtest(void);

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

/* --- L10 demos --- */
static volatile int myglobal;
static mutex_t      glock;

static void racer_unsafe(void *arg) {
    (void)arg;
    for (int i = 0; i < 50000; i++) {
        int tmp = myglobal;
        yield();
        myglobal = tmp + 1;
    }
}
static void racer_safe(void *arg) {
    (void)arg;
    for (int i = 0; i < 50000; i++) {
        mutex_lock(&glock);
        int tmp = myglobal;
        yield();
        myglobal = tmp + 1;
        mutex_unlock(&glock);
    }
}
static void cmd_race(void) {
    vga_puts("\n  Race condition demo (2 threads x 50000 increments)...\n");
    myglobal = 0;
    pcb_t *a = thread_create(racer_unsafe, 0, "racer_u1");
    pcb_t *b = thread_create(racer_unsafe, 0, "racer_u2");
    while (a->state != PROC_ZOMBIE || b->state != PROC_ZOMBIE) yield();
    vga_printf("  Without mutex: myglobal = %d  (expected 100000)\n", myglobal);

    mutex_init(&glock);
    myglobal = 0;
    a = thread_create(racer_safe, 0, "racer_s1");
    b = thread_create(racer_safe, 0, "racer_s2");
    while (a->state != PROC_ZOMBIE || b->state != PROC_ZOMBIE) yield();
    vga_printf("  With mutex:    myglobal = %d  (expected 100000)\n\n", myglobal);
}

#define BUF_N 8
static int          buf[BUF_N];
static int          buf_head, buf_tail;
static semaphore_t  sem_empty, sem_full, sem_mutex;

static void producer(void *arg) {
    (void)arg;
    for (int i = 0; i < 1000; i++) {
        sem_wait(&sem_empty);
        sem_wait(&sem_mutex);
        buf[buf_head] = i;
        buf_head = (buf_head + 1) % BUF_N;
        sem_signal(&sem_mutex);
        sem_signal(&sem_full);
    }
}
static void consumer(void *arg) {
    (void)arg;
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sem_wait(&sem_full);
        sem_wait(&sem_mutex);
        sum += buf[buf_tail];
        buf_tail = (buf_tail + 1) % BUF_N;
        sem_signal(&sem_mutex);
        sem_signal(&sem_empty);
    }
    vga_printf("\n  Producer-consumer: sum = %d  (expected 499500)\n", sum);
}
static void cmd_pc(void) {
    buf_head = buf_tail = 0;
    sem_init(&sem_empty, BUF_N);
    sem_init(&sem_full,  0);
    sem_init(&sem_mutex, 1);
    vga_puts("\n  Bounded-buffer producer-consumer (1000 items, buffer 8)...\n");
    pcb_t *pr = thread_create(producer, 0, "producer");
    pcb_t *co = thread_create(consumer, 0, "consumer");
    while (pr->state != PROC_ZOMBIE || co->state != PROC_ZOMBIE) yield();
    vga_puts("  Done.\n\n");
}

/* --- L11 PMM --- */
static void cmd_meminfo(void) {
    uint32_t t = pmm_total_frames();
    uint32_t u = pmm_used_frames();
    uint32_t f = pmm_free_frames();
    vga_puts_color("\n  Physical Memory Manager (L11)\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  --------------------------------\n");
    vga_printf("  Total : %u MB  (%u frames of 4 KB)\n", t * 4 / 1024, t);
    vga_printf("  Used  : %u MB  (%u frames)\n",         u * 4 / 1024, u);
    vga_printf("  Free  : %u MB  (%u frames)\n\n",        f * 4 / 1024, f);
}

static void cmd_memtest(void) {
    vga_puts("\n  PMM stress test: allocate 100 frames\n");
    uint32_t before = pmm_free_frames();
    uint32_t frames[100];
    for (int i = 0; i < 100; i++) {
        frames[i] = pmm_alloc_frame();
        if (!frames[i]) {
            vga_puts("  ERROR: allocation failed mid-loop\n\n");
            for (int j = 0; j < i; j++) pmm_free_frame(frames[j]);
            return;
        }
    }
    uint32_t after_alloc = pmm_free_frames();
    vga_printf("  Free before: %u, after alloc: %u (diff %d)\n",
               before, after_alloc, (int)(before - after_alloc));

    for (int i = 0; i < 100; i++) pmm_free_frame(frames[i]);
    uint32_t after_free = pmm_free_frames();
    vga_printf("  Free after free: %u\n", after_free);

    if (after_free == before) vga_puts("  [OK] No leak detected\n\n");
    else                     vga_puts("  [FAIL] Leak detected!\n\n");
}

/* --- splash / help --- */
static void print_splash(void) {
    vga_clear(VGA_BLACK);
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);
    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems", VGA_YELLOW, VGA_BLACK);
    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 3: Physical Memory Manager", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_set_cursor(3, 2);
    vga_puts_color("  Department of Software Engineering", VGA_LIGHT_GREY, VGA_BLACK);
    vga_set_cursor(4, 2);
    vga_puts_color("  Try: meminfo, memtest, ps, demo_race, demo_pc", VGA_LIGHT_GREEN, VGA_BLACK);
    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 | PIT: 100Hz | Round-Robin | Bitmap PMM", VGA_DARK_GREY, VGA_BLACK);
    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Type 'meminfo' to view total/used/free physical frames.\n");
    vga_puts("  Type 'memtest' to allocate and free 100 frames.\n\n");
}

static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  --------------------------------\n");
    vga_puts("  help       - Show this help message\n");
    vga_puts("  clear      - Clear the screen\n");
    vga_puts("  about      - About this OS\n");
    vga_puts("  echo       - Echo text\n");
    vga_puts("  mem        - Memory map (stub)\n");
    vga_puts("  ps         - [L09] List processes\n");
    vga_puts("  demo_race  - [L10] Race with/without mutex\n");
    vga_puts("  demo_pc    - [L10] Producer-consumer\n");
    vga_puts("  meminfo    - [L11] Physical memory totals\n");
    vga_puts("  memtest    - [L11] Allocate/free 100 frames\n\n");
}
static void cmd_clear(void) { vga_clear(VGA_BLACK); }
static void cmd_about(void) { vga_puts("\n  SENG21213-OS - x86 i686, freestanding C, QEMU\n\n"); }
static void cmd_echo(const char *args) { vga_puts("  "); vga_puts(args); vga_puts("\n"); }
static void cmd_mem(void) {
    vga_puts("\n  Static memory map\n");
    vga_puts("  0x00000000 - 0x000FFFFF  first 1 MB (BIOS, VGA, kernel)\n");
    vga_puts("  0x00100000 - ...         usable RAM (managed by PMM)\n");
    vga_puts("  Use 'meminfo' for live totals.\n\n");
}

static char shell_buf[256];
static char prompt[] = "ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready.\n\n", VGA_LIGHT_GREEN, VGA_BLACK);
    for (;;) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;
        if (k_strcmp(cmd, "help")      == 0) { cmd_help();    continue; }
        if (k_strcmp(cmd, "clear")     == 0) { cmd_clear();   continue; }
        if (k_strcmp(cmd, "about")     == 0) { cmd_about();   continue; }
        if (k_strcmp(cmd, "mem")       == 0) { cmd_mem();     continue; }
        if (k_strcmp(cmd, "ps")        == 0) { cmd_ps();      continue; }
        if (k_strcmp(cmd, "demo_race") == 0) { cmd_race();    continue; }
        if (k_strcmp(cmd, "demo_pc")   == 0) { cmd_pc();      continue; }
        if (k_strcmp(cmd, "meminfo")   == 0) { cmd_meminfo(); continue; }
        if (k_strcmp(cmd, "memtest")   == 0) { cmd_memtest(); continue; }
        if (k_strncmp(cmd, "echo ", 5) == 0) { cmd_echo(k_ltrim(cmd + 5)); continue; }
        vga_puts_color("  Unknown command. Try 'help'.\n", VGA_LIGHT_RED, VGA_BLACK);
    }
}

void kernel_main(void) {
    vga_init();
    kb_init();
    print_splash();

    process_init();
    pmm_init();

    static pcb_t shell_pcb;
    shell_pcb.pid       = 100;
    shell_pcb.state     = PROC_RUNNING;
    shell_pcb.esp       = 0;
    shell_pcb.eip       = 0;
    shell_pcb.next      = 0;
    shell_pcb.wait_next = 0;
    const char *n = "shell"; int i = 0;
    while (n[i]) { shell_pcb.name[i] = n[i]; i++; }
    shell_pcb.name[i] = 0;
    process_add_to_list(&shell_pcb);
    set_current(&shell_pcb);

    scheduler_init();
    shell_run();
    for (;;) __asm__ __volatile__("hlt");
}
