#include "types.h"
#include <stdlib.h>
#include <string.h>

int _create(size_type size) {
    if (size < MIN_DISK_SIZE) {
        return -1;
    }

    FILE *disk = fopen("disk.fs", "wb+");
    if (disk == NULL) {
        return -1;
    }

    _form_super_block(disk, size);
    fclose(disk);

    return 0;
}

int _copy_to(FILE *disk, FILE *file, char *file_name) {
    struct SuperBlock sblock = _load_super_block(disk);

    if (sblock.free_inode_count == 0) {
        return -1;
    }

    fseek(file, 0, SEEK_END);
    size_type file_size = ftell(file);
    rewind(file);

    size_type required_full_blocks = (file_size / MAX_BLOCK_DATA);
    size_type required_partial_block =
        (file_size % MAX_BLOCK_DATA != 0 ? 1 : 0);
    size_type required_blocks = required_full_blocks + required_partial_block;

    if (sblock.free_block_count < required_blocks) {
        return -1;
    }

    size_type first_free_block_offset = _first_free_block(disk, &sblock);
    _first_free_inode(disk, &sblock);
    struct INode new_inode = {.file_name = {'\0'},
                              .file_size = file_size,
                              .first_block_offset = first_free_block_offset};

    if (strlen(file_name) > sizeof(new_inode.file_name) - 1) {
        return -1;
    }

    strcpy(new_inode.file_name, file_name);
    _write_inode(disk, &new_inode);
    fseek(disk, first_free_block_offset, SEEK_SET);
    size_type current_block_offset = first_free_block_offset;

    for (size_type i = 0; i < required_full_blocks; ++i) {
        struct Block block;
        fread(block.data, sizeof(unsigned char), MAX_BLOCK_DATA, file);

        if (required_partial_block == 0 && i == required_blocks - 1) {
            block.next_block_offset = (size_type)(-1);
        } else {
            block.next_block_offset = _next_free_block(disk);
        }

        fseek(disk, current_block_offset, SEEK_SET);
        _write_block(disk, &block);

        if (block.next_block_offset != current_block_offset + HEADER_SIZE) {
            fseek(disk, block.next_block_offset, SEEK_SET);
        }

        current_block_offset = block.next_block_offset;
    }

    if (required_partial_block) {
        size_type remaining_bytes = file_size % MAX_BLOCK_DATA;
        struct Block block;
        fread(block.data, sizeof(unsigned char), remaining_bytes, file);
        memset(block.data + remaining_bytes, 0,
               MAX_BLOCK_DATA - remaining_bytes);
        block.next_block_offset = (size_type)(-1);
        _write_block(disk, &block);
    }

    --sblock.free_inode_count;
    sblock.free_block_count -= required_blocks;
    _write_super_block(disk, &sblock);

    return 0;
}

int _copy_from(FILE *disk, char *file_name) {
    struct SuperBlock sblock = _load_super_block(disk);
    fseek(disk, sblock.first_inode_offset, SEEK_SET);

    for (size_type i = 0; i < sblock.inode_count; ++i) {
        struct INode inode = _load_inode(disk);

        if (strcmp(inode.file_name, file_name) == 0) {
            FILE *file = fopen(file_name, "wb");

            if (file == NULL) {
                return -1;
            }

            size_type first_block_offset = inode.first_block_offset;
            size_type file_size = inode.file_size;
            fseek(disk, first_block_offset, SEEK_SET);
            size_type current_block_offset = first_block_offset;

            do {
                struct Block current_block = _load_block(disk);
                size_type next_block_offset = current_block.next_block_offset;

                size_type current_block_size;
                if (file_size > MAX_BLOCK_DATA) {
                    current_block_size = MAX_BLOCK_DATA;
                } else {
                    current_block_size = file_size;
                }
                fwrite(current_block.data, sizeof(unsigned char),
                       current_block_size, file);

                if (next_block_offset != (size_type)(-1)) {
                    file_size -= current_block_size;

                    if (next_block_offset !=
                        current_block_offset + BLOCK_SIZE) {
                        fseek(disk, next_block_offset, SEEK_SET);
                    }

                    current_block_offset = next_block_offset;
                } else {
                    break;
                }
            } while (true);

            fclose(file);
            return 0;
        }
    }

    return -1;
}

int _list(FILE *disk) {
    struct SuperBlock sblock = _load_super_block(disk);
    size_type taken_inodes = _taken_inodes(&sblock);

    if (taken_inodes == 0) {
        puts("disk is empty.");
        return 0;
    }

    fseek(disk, sblock.first_inode_offset, SEEK_SET);
    printf("File name                       | Size in bytes | disk offset\n");
    printf("--------------------------------+---------------+-------------\n");

    for (size_type i = 0; i < sblock.inode_count; ++i) {
        struct INode inode = _load_inode(disk);

        if (inode.first_block_offset != 0) {
            printf("%-32s| %-14d| %-12d\n", inode.file_name, inode.file_size,
                   inode.first_block_offset);
        }
    }

    return 0;
}

int _remove(FILE *disk, char *file_name) {
    struct SuperBlock sblock = _load_super_block(disk);
    fseek(disk, sblock.first_inode_offset, SEEK_SET);

    for (size_type i = 0; i < sblock.inode_count; ++i) {
        struct INode inode = _load_inode(disk);

        if (strcmp(inode.file_name, file_name) == 0) {
            size_type first_block_offset = inode.first_block_offset;
            fseek(disk, -(long)sizeof(struct INode), SEEK_CUR);
            struct INode empty_inode = {0};
            fwrite(&empty_inode, sizeof(struct INode), 1, disk);
            fseek(disk, first_block_offset, SEEK_SET);
            size_type freed_blocks = _free_blocks(disk);

            sblock.free_block_count += freed_blocks;
            ++sblock.free_inode_count;
            _write_super_block(disk, &sblock);

            return 0;
        }
    }

    return -1;
}

int _map(FILE *disk) {
    struct SuperBlock sblock = _load_super_block(disk);

    printf("disk size: %u\n", sblock.size);
    printf("--------------------------------------");
    printf("Total inodes: %u\n", sblock.inode_count);
    printf("Free inodes: %u\n", sblock.free_inode_count);
    printf("Occupied inodes: %u\n",
           sblock.inode_count - sblock.free_inode_count);
    printf("First inode offset: %u\n", sblock.first_inode_offset);
    printf("--------------------------------------");
    printf("Total blocks: %u\n", sblock.block_count);
    printf("Free blocks: %u\n", sblock.free_block_count);
    printf("Occupied blocks: %u\n",
           sblock.block_count - sblock.free_block_count);
    printf("First block offset: %u\n", sblock.first_block_offset);
    printf("\n");

    printf("INode index | Offset     | Status   | File name                    "
           "\n");
    printf("------------+------------+----------+------------------------------"
           "\n");

    size_type offset = sblock.first_inode_offset;
    for (size_type i = 0; i < sblock.inode_count; ++i) {
        struct INode current_inode = _load_inode(disk);
        char *status =
            (current_inode.first_block_offset == 0 ? "Free" : "Occupied");
        printf("%-11u | %-10u | %-8s | %-32s\n", i, offset, status,
               current_inode.file_name);
        offset += sizeof(struct INode);
    }
    printf("\n");

    char **names = malloc(sizeof(char *) * sblock.inode_count);
    fseek(disk, sblock.first_inode_offset, SEEK_SET);
    char **block_names = malloc(sizeof(char *) * sblock.block_count);
    size_type inode_offset = sblock.first_inode_offset;

    for (size_type i = 0; i < sblock.inode_count; ++i) {
        fseek(disk, inode_offset, SEEK_SET);
        struct INode current_inode = _load_inode(disk);

        if (current_inode.first_block_offset == 0) {
            names[i] = NULL;
        } else {
            names[i] = malloc(sizeof(char) * MAX_FILENAME_LENGTH);
            strcpy(names[i], current_inode.file_name);
            size_type current_block_offset = current_inode.first_block_offset;

            do {
                fseek(disk, current_block_offset, SEEK_SET);
                struct Block current_block = _load_block(disk);
                size_type next_block_offset = current_block.next_block_offset;
                size_type block_index = _block_index(current_block_offset);
                block_names[block_index] = names[i];

                if (next_block_offset == (size_type)(-1)) {
                    break;
                } else {
                    current_block_offset = next_block_offset;
                }
            } while (true);
        }

        inode_offset += sizeof(struct INode);
    }

    offset = sblock.first_block_offset;
    fseek(disk, sblock.first_block_offset, SEEK_SET);
    printf("Block index | Offset     | Status   | File name                    "
           "\n");
    printf("------------+------------+----------+------------------------------"
           "\n");

    for (size_type i = 0; i < sblock.block_count; ++i) {
        struct Block current_block = _load_block(disk);
        bool is_free = (current_block.next_block_offset == 0);
        char *status = (is_free ? "Free" : "Occupied");
        char *name = (is_free ? "None" : block_names[i]);
        printf("%-11u | %-10u | %-8s | %-32s\n", i, offset, status, name);
        offset += sizeof(struct Block);
    }

    for (size_type i = sblock.inode_count; i-- > 0;) {
        free(names[i]);
    }
    free(block_names);
    free(names);

    return 0;
}

int _delete() { return remove("disk.fs"); }
