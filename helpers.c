#include "types.h"
#include <stdlib.h>
#include <string.h>

FILE *_open_disk() {
    FILE *disk = fopen("disk.fs", "r+b");
    return disk;
}

struct Block _load_block(FILE *disk) {
    struct Block block;
    fread(&block, sizeof(struct Block), 1, disk);
    return block;
}

struct INode _load_inode(FILE *disk) {
    struct INode inode;
    fread(&inode, sizeof(struct INode), 1, disk);
    return inode;
}

struct SuperBlock _load_super_block(FILE *disk) {
    rewind(disk);
    struct SuperBlock sblock;
    fread(&sblock, sizeof(struct SuperBlock), 1, disk);
    return sblock;
}

size_type _block_index(size_type block_offset) {
    return (block_offset - HEADER_SIZE) / BLOCK_SIZE;
}

size_type _taken_inodes(struct SuperBlock *sblock) {
    return sblock->inode_count - sblock->free_inode_count;
}

size_type _first_free_block(FILE *disk, struct SuperBlock *sblock) {
    size_type free_block_offset = sblock->first_block_offset;
    fseek(disk, free_block_offset, SEEK_SET);

    do {
        struct Block block = _load_block(disk);
        if (block.next_block_offset == 0) {
            break;
        }

        free_block_offset += BLOCK_SIZE;
    } while (true);

    fseek(disk, free_block_offset, SEEK_SET);
    return free_block_offset;
}

size_type _next_free_block(FILE *disk) {
    fseek(disk, (long)sizeof(struct Block), SEEK_CUR);
    size_type free_block_offset = ftell(disk);

    do {
        struct Block block = _load_block(disk);
        if (block.next_block_offset == 0) {
            break;
        }

        free_block_offset += BLOCK_SIZE;
    } while (true);

    fseek(disk, -(long)sizeof(struct Block), SEEK_CUR);
    return free_block_offset;
}

size_type _first_free_inode(FILE *disk, struct SuperBlock *sblock) {
    size_type free_inode_offset = sblock->first_inode_offset;
    fseek(disk, free_inode_offset, SEEK_SET);

    do {
        struct INode inode = _load_inode(disk);
        if (inode.first_block_offset == 0) {
            break;
        }

        free_inode_offset += sizeof(struct INode);
    } while (true);

    fseek(disk, free_inode_offset, SEEK_SET);
    return free_inode_offset;
}

void _write_block(FILE *disk, struct Block *block) {
    fwrite(block, sizeof(struct Block), 1, disk);
}

void _write_inode(FILE *disk, struct INode *inode) {
    fwrite(inode, sizeof(struct INode), 1, disk);
}

void _write_super_block(FILE *disk, struct SuperBlock *sblock) {
    rewind(disk);
    fwrite(sblock, sizeof(struct SuperBlock), 1, disk);
}

void _form_super_block(FILE *disk, size_type size) {
    struct SuperBlock sblock;

    sblock.size = size;
    sblock.first_inode_offset = sizeof(struct SuperBlock);
    sblock.inode_count = MAX_INODE_COUNT;
    sblock.free_inode_count = sblock.inode_count;

    size_type possible_blocks = (size - HEADER_SIZE) / BLOCK_SIZE;

    sblock.first_block_offset = HEADER_SIZE;
    sblock.block_count = possible_blocks;
    sblock.free_block_count = sblock.block_count;

    _write_super_block(disk, &sblock);
    _form_inodes(disk);
    _form_blocks(disk, possible_blocks);

    size_type remaining_bytes =
        sblock.size - (HEADER_SIZE + possible_blocks * BLOCK_SIZE);
    _remaining_bytes(disk, remaining_bytes);
}

void _form_inodes(FILE *disk) {
    struct INode empty_inode = {
        .file_name = {'\0'}, .file_size = 0, .first_block_offset = 0};

    for (size_type i = 0; i < MAX_INODE_COUNT; ++i) {
        _write_inode(disk, &empty_inode);
    }
}

void _form_blocks(FILE *disk, size_type block_count) {
    struct Block empty_block = {.data = {0}, .next_block_offset = 0};

    for (size_type i = 0; i < block_count; ++i) {
        _write_block(disk, &empty_block);
    }
}

void _remaining_bytes(FILE *disk, size_type byte_count) {
    unsigned char *bytes = malloc(byte_count);
    memset(bytes, 0, byte_count);
    fwrite(bytes, sizeof(unsigned char), byte_count, disk);
    free(bytes);
}

size_type _free_blocks(FILE *disk) {
    size_type freed_block_counter = 0;

    do {
        struct Block current_block = _load_block(disk);
        size_type next_block_offset = current_block.next_block_offset;
        fseek(disk, -(long)BLOCK_SIZE, SEEK_CUR);
        struct Block empty_block = {0};
        fwrite(&empty_block, BLOCK_SIZE, 1, disk);
        ++freed_block_counter;

        if (next_block_offset != (size_type)(-1)) {
            fseek(disk, next_block_offset, SEEK_SET);
        } else {
            break;
        }
    } while (true);

    return freed_block_counter;
}
