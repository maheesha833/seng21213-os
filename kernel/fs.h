#ifndef FS_H
#define FS_H
#include "../include/types.h"

#define FS_MAGIC         0x53454E47u
#define FS_BLOCK_SIZE    4096
#define FS_BLOCKS        256
#define FS_MAX_INODES    128
#define FS_MAX_FILES     128
#define FS_MAX_NAME      28
#define FS_DIRECT_BLOCKS 8
#define FS_INDIRECT_PER_BLOCK (FS_BLOCK_SIZE / 4)
#define FS_DATA_START    3

typedef struct {
    uint32_t magic;
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t data_start;
} fs_superblock_t;

typedef struct {
    uint32_t size;
    uint32_t blocks[FS_DIRECT_BLOCKS];
    uint32_t indirect;         /* Extension: L12 section 2 */
    uint8_t  used;
    uint8_t  pad[3];
} fs_inode_t;

typedef struct {
    char     name[FS_MAX_NAME];
    uint32_t inode;
} fs_dirent_t;

void     fs_init(void);
void     fs_format(void);
int      fs_create(const char *name);
int      fs_open(const char *name);
int      fs_read(int ino, void *buf, uint32_t maxlen);
int      fs_write(int ino, const void *buf, uint32_t len);
int      fs_unlink(const char *name);
uint32_t fs_size(int ino);
int      fs_list(char names[][FS_MAX_NAME], int max);

#endif
