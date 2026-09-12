/* =============================================================================
 * SENG21213-OS :: Physical Memory Manager (bitmap allocator)
 * File   : kernel/pmm.c
 *
 * L11 §3 — 1 bit per 4 KB physical frame. 1 = used, 0 = free.
 *
 * Reads the E820 map from 0x8000 if the bootloader placed it there
 * (magic 'SENG' at 0x7F04). Otherwise falls back to the QEMU default:
 * 32 MB usable starting at 1 MB, first 1 MB reserved.
 * ============================================================================*/
#include "pmm.h"

#define FRAME_SIZE  PMM_FRAME_SIZE
#define MAX_FRAMES  PMM_MAX_FRAMES
#define BITMAP_SIZE (MAX_FRAMES / 8)

/* E820 record layout */
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;      /* 1 = usable RAM */
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;

#define E820_ADDR       0x8000
#define E820_COUNT_ADDR 0x7F00
#define E820_MAGIC_ADDR 0x7F04
#define E820_MAGIC_VAL  0x53454E47u   /* 'SENG' */

static uint8_t  bitmap[BITMAP_SIZE];
static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;

static inline int frame_used(uint32_t f) {
    return (bitmap[f >> 3] & (1u << (f & 7))) != 0;
}
static inline void mark_used(uint32_t f) {
    bitmap[f >> 3] |= (uint8_t)(1u << (f & 7));
}
static inline void mark_free(uint32_t f) {
    bitmap[f >> 3] &= (uint8_t)~(1u << (f & 7));
}

void pmm_init(void) {
    /* Start: every frame marked used. */
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) bitmap[i] = 0xFF;
    total_frames = 0;
    used_frames  = 0;

    volatile uint32_t *magic = (volatile uint32_t *)E820_MAGIC_ADDR;
    volatile uint16_t *count = (volatile uint16_t *)E820_COUNT_ADDR;
    e820_entry_t      *e820  = (e820_entry_t *)E820_ADDR;

    if (*magic == E820_MAGIC_VAL) {
        /* ---- Parse E820 ---- */
        uint16_t n = *count;
        if (n > 32) n = 32;
        for (uint16_t i = 0; i < n; i++) {
            if (e820[i].type != 1) continue;   /* 1 = usable */
            uint64_t base = e820[i].base;
            uint64_t len  = e820[i].length;
            uint64_t first = (base + FRAME_SIZE - 1) / FRAME_SIZE;
            uint64_t last  = (base + len) / FRAME_SIZE;
            if (last > MAX_FRAMES) last = MAX_FRAMES;
            if (first >= last) continue;
            if (last > total_frames) total_frames = (uint32_t)last;
            for (uint64_t f = first; f < last; f++) mark_free((uint32_t)f);
        }
    }

    /* ---- Fallback: 32 MB at 1 MB (QEMU default) ---- */
    if (total_frames == 0) {
        total_frames = (0x100000 + 32 * 1024 * 1024) / FRAME_SIZE;
        if (total_frames > MAX_FRAMES) total_frames = MAX_FRAMES;
        for (uint32_t f = 0x100000 / FRAME_SIZE; f < total_frames; f++) mark_free(f);
    }

    /* ---- Reserve the first 1 MB (BIOS, IVT, VGA, boot sector, kernel) ---- */
    for (uint32_t f = 0; f < (1u << 20) / FRAME_SIZE && f < total_frames; f++) {
        mark_used(f);
    }

    /* Count used frames. */
    used_frames = 0;
    for (uint32_t f = 0; f < total_frames; f++) {
        if (frame_used(f)) used_frames++;
    }
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t f = 0; f < total_frames; f++) {
        if (!frame_used(f)) {
            mark_used(f);
            used_frames++;
            return f * FRAME_SIZE;
        }
    }
    return 0;
}

void pmm_free_frame(uint32_t paddr) {
    if (paddr & (FRAME_SIZE - 1)) return;   /* not frame-aligned */
    uint32_t f = paddr / FRAME_SIZE;
    if (f >= total_frames) return;
    if (frame_used(f)) { mark_free(f); used_frames--; }
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames; }
uint32_t pmm_free_frames(void)  { return total_frames - used_frames; }
