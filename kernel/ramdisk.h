#ifndef RAMDISK_H
#define RAMDISK_H
#include "../include/types.h"

#define RAMDISK_BLOCK_SIZE 4096
#define RAMDISK_BLOCKS     256    /* 1 MB total */

void     ramdisk_read (uint32_t block, void *buf);
void     ramdisk_write(uint32_t block, const void *buf);
uint8_t *ramdisk_direct(uint32_t block);   /* in-place pointer for block ops */

#endif
