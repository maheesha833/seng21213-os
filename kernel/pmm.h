#ifndef PMM_H
#define PMM_H
#include "../include/types.h"

#define PMM_FRAME_SIZE  4096
#define PMM_MAX_FRAMES  (512 * 1024)   /* up to 2 GB of physical RAM */

void     pmm_init(void);
uint32_t pmm_alloc_frame(void);   /* physical address, 0 = failure */
void     pmm_free_frame(uint32_t paddr);
uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);

#endif
