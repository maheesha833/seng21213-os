/* SENG21213-OS :: RAM disk file system — L12 §2–3 */
#include "fs.h"
#include "ramdisk.h"

static fs_superblock_t sb;
static fs_inode_t      inodes[FS_MAX_INODES];
static uint8_t         block_bitmap[FS_BLOCKS / 8];
static fs_dirent_t     dir_cache[FS_MAX_FILES];

/* ---- bitmap helpers ---- */
static int  blk_used(uint32_t b)  { return (block_bitmap[b >> 3] >> (b & 7)) & 1; }
static void blk_mark(uint32_t b)  { block_bitmap[b >> 3] |= (uint8_t)(1u << (b & 7)); }
static void blk_clear(uint32_t b) { block_bitmap[b >> 3] &= (uint8_t)~(1u << (b & 7)); }

static int alloc_block(void) {
    for (uint32_t b = sb.data_start; b < FS_BLOCKS; b++)
        if (!blk_used(b)) { blk_mark(b); return (int)b; }
    return -1;
}

static int alloc_inode(void) {
    for (int i = 1; i < FS_MAX_INODES; i++) {
        if (!inodes[i].used) {
            inodes[i].used = 1;
            inodes[i].size = 0;
            for (int k = 0; k < FS_DIRECT_BLOCKS; k++) inodes[i].blocks[k] = 0;
            return i;
        }
    }
    return -1;
}

/* ---- directory load/save (mirrors block 1) ---- */
static void dir_save(void) {
    uint8_t *blk = ramdisk_direct(1);
    for (uint32_t i = 0; i < sizeof(dir_cache); i++)
        blk[i] = ((uint8_t *)dir_cache)[i];
}
static void dir_load(void) {
    uint8_t *blk = ramdisk_direct(1);
    for (uint32_t i = 0; i < sizeof(dir_cache); i++)
        ((uint8_t *)dir_cache)[i] = blk[i];
}

static int str_eq(const char *a, const char *b) {
    for (int i = 0; i < FS_MAX_NAME; i++) {
        if (a[i] != b[i]) return 0;
        if (a[i] == 0) return 1;
    }
    return 1;
}
static void str_copy(char *dst, const char *src, int maxlen) {
    int i = 0;
    while (i < maxlen - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static int dir_find(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (dir_cache[i].inode == 0) continue;
        if (str_eq(dir_cache[i].name, name)) return i;
    }
    return -1;
}

/* ---- public API ---- */
void fs_format(void) {
    sb.magic       = FS_MAGIC;
    sb.block_count = FS_BLOCKS;
    sb.inode_count = FS_MAX_INODES;
    sb.data_start  = FS_DATA_START;

    /* bitmap: blocks 0,1,2 reserved (superblock, dir, bitmap) */
    for (uint32_t i = 0; i < sizeof(block_bitmap); i++) block_bitmap[i] = 0;
    blk_mark(0); blk_mark(1); blk_mark(2);

    for (int i = 0; i < FS_MAX_INODES; i++) {
        inodes[i].used = 0;
        inodes[i].size = 0;
        for (int k = 0; k < FS_DIRECT_BLOCKS; k++) inodes[i].blocks[k] = 0;
    }
    inodes[0].used = 1;   /* inode 0 reserved */

    for (int i = 0; i < FS_MAX_FILES; i++) {
        for (int k = 0; k < FS_MAX_NAME; k++) dir_cache[i].name[k] = 0;
        dir_cache[i].inode = 0;
    }

    /* write superblock + directory to disk */
    uint8_t *sbblk = ramdisk_direct(0);
    uint8_t *s = (uint8_t *)&sb;
    for (uint32_t i = 0; i < sizeof(sb); i++) sbblk[i] = s[i];
    dir_save();
}

void fs_init(void) {
    uint8_t *sbblk = ramdisk_direct(0);
    uint8_t *s = (uint8_t *)&sb;
    for (uint32_t i = 0; i < sizeof(sb); i++) s[i] = sbblk[i];

    if (sb.magic != FS_MAGIC || sb.block_count != FS_BLOCKS) {
        fs_format();
    } else {
        dir_load();
    }
}

int fs_create(const char *name) {
    if (dir_find(name) >= 0) return -1;
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (dir_cache[i].inode == 0) { slot = i; break; }
    if (slot < 0) return -1;

    int ino = alloc_inode();
    if (ino < 0) return -1;

    str_copy(dir_cache[slot].name, name, FS_MAX_NAME);
    dir_cache[slot].inode = (uint32_t)ino;
    dir_save();
    return ino;
}

int fs_open(const char *name) {
    int s = dir_find(name);
    return (s < 0) ? -1 : (int)dir_cache[s].inode;
}

int fs_write(int ino, const void *buf, uint32_t len) {
    if (ino <= 0 || ino >= FS_MAX_INODES || !inodes[ino].used) return -1;
    fs_inode_t *n = &inodes[ino];
    const uint8_t *p = (const uint8_t *)buf;

    uint32_t written = 0;
    while (written < len) {
        uint32_t blk_idx = (n->size + written) / FS_BLOCK_SIZE;
        uint32_t off     = (n->size + written) % FS_BLOCK_SIZE;
        if (blk_idx >= FS_DIRECT_BLOCKS) break;   /* file size cap */

        if (n->blocks[blk_idx] == 0) {
            int b = alloc_block();
            if (b < 0) break;
            n->blocks[blk_idx] = (uint32_t)b;
        }
        uint8_t *d = ramdisk_direct(n->blocks[blk_idx]);

        uint32_t chunk = FS_BLOCK_SIZE - off;
        if (chunk > len - written) chunk = len - written;
        for (uint32_t k = 0; k < chunk; k++) d[off + k] = p[written + k];
        written += chunk;
    }
    n->size += written;
    return (int)written;
}

int fs_read(int ino, void *buf, uint32_t maxlen) {
    if (ino <= 0 || ino >= FS_MAX_INODES || !inodes[ino].used) return -1;
    fs_inode_t *n = &inodes[ino];
    uint32_t len = n->size;
    if (len > maxlen) len = maxlen;

    uint8_t *p = (uint8_t *)buf;
    uint32_t done = 0;
    while (done < len) {
        uint32_t blk = done / FS_BLOCK_SIZE;
        uint32_t off = done % FS_BLOCK_SIZE;
        if (n->blocks[blk] == 0) break;
        uint8_t *d = ramdisk_direct(n->blocks[blk]);
        uint32_t chunk = FS_BLOCK_SIZE - off;
        if (chunk > len - done) chunk = len - done;
        for (uint32_t k = 0; k < chunk; k++) p[done + k] = d[off + k];
        done += chunk;
    }
    return (int)done;
}

int fs_unlink(const char *name) {
    int s = dir_find(name);
    if (s < 0) return -1;
    int ino = (int)dir_cache[s].inode;
    if (ino <= 0 || ino >= FS_MAX_INODES) return -1;

    fs_inode_t *n = &inodes[ino];
    for (int i = 0; i < FS_DIRECT_BLOCKS; i++) {
        if (n->blocks[i]) { blk_clear(n->blocks[i]); n->blocks[i] = 0; }
    }
    n->used = 0;
    n->size = 0;

    for (int k = 0; k < FS_MAX_NAME; k++) dir_cache[s].name[k] = 0;
    dir_cache[s].inode = 0;
    dir_save();
    return 0;
}

uint32_t fs_size(int ino) {
    if (ino <= 0 || ino >= FS_MAX_INODES || !inodes[ino].used) return 0;
    return inodes[ino].size;
}

int fs_list(char names[][FS_MAX_NAME], int max) {
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES && count < max; i++) {
        if (dir_cache[i].inode == 0) continue;
        for (int k = 0; k < FS_MAX_NAME; k++) names[count][k] = dir_cache[i].name[k];
        count++;
    }
    return count;
}
