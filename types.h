#ifndef TYPES_H
#define TYPES_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t size_type;

#define BLOCK_SIZE ((size_type)4096)
#define MAX_INODE_COUNT ((size_type)32)
#define MAX_FILENAME_LENGTH ((size_type)32)
#define HEADER_SIZE                                                            \
    ((size_type)(sizeof(struct SuperBlock) +                                   \
                 MAX_INODE_COUNT * sizeof(struct INode)))
#define MIN_DISK_SIZE ((size_type)(HEADER_SIZE + BLOCK_SIZE * MAX_INODE_COUNT))
#define MAX_BLOCK_DATA ((size_type)(BLOCK_SIZE - sizeof(size_type)))

struct Block {
    unsigned char data[MAX_BLOCK_DATA];
    size_type next_block_offset;
};

struct INode {
    char file_name[MAX_FILENAME_LENGTH];
    size_type file_size;
    size_type first_block_offset;
};

struct SuperBlock {
    size_type size;

    size_type first_inode_offset;
    size_type inode_count;
    size_type free_inode_count;

    size_type first_block_offset;
    size_type block_count;
    size_type free_block_count;
};

FILE *_open_disk();
struct Block _load_block(FILE *);
struct INode _load_inode(FILE *);
struct SuperBlock _load_super_block(FILE *);
size_type _block_index(size_type);
size_type _taken_inodes(struct SuperBlock *);
size_type _first_free_block(FILE *, struct SuperBlock *);
size_type _next_free_block(FILE *);
size_type _first_free_inode(FILE *, struct SuperBlock *);
void _write_block(FILE *, struct Block *);
void _write_inode(FILE *, struct INode *);
void _write_super_block(FILE *, struct SuperBlock *);
void _form_super_block(FILE *, size_type);
void _form_inodes(FILE *);
void _form_blocks(FILE *, size_type);
void _remaining_bytes(FILE *, size_type);
size_type _free_blocks(FILE *);

int _create(size_type);
int _copy_to(FILE *, FILE *, char *);
int _copy_from(FILE *, char *);
int _list(FILE *);
int _remove(FILE *, char *);
int _map(FILE *);
int _delete();

#endif
