/* SENG21213-OS :: 1 MB RAM disk — L12 §1 */
#include "ramdisk.h"

static uint8_t ramdisk[RAMDISK_BLOCKS * RAMDISK_BLOCK_SIZE]
    __attribute__((aligned(RAMDISK_BLOCK_SIZE)));

void ramdisk_read(uint32_t block, void *buf) {
    if (block >= RAMDISK_BLOCKS) return;
    const uint8_t *src = &ramdisk[block * RAMDISK_BLOCK_SIZE];
    uint8_t *dst = (uint8_t *)buf;
    for (uint32_t i = 0; i < RAMDISK_BLOCK_SIZE; i++) dst[i] = src[i];
}

void ramdisk_write(uint32_t block, const void *buf) {
    if (block >= RAMDISK_BLOCKS) return;
    uint8_t *dst = &ramdisk[block * RAMDISK_BLOCK_SIZE];
    const uint8_t *src = (const uint8_t *)buf;
    for (uint32_t i = 0; i < RAMDISK_BLOCK_SIZE; i++) dst[i] = src[i];
}

uint8_t *ramdisk_direct(uint32_t block) {
    if (block >= RAMDISK_BLOCKS) return 0;
    return &ramdisk[block * RAMDISK_BLOCK_SIZE];
}
