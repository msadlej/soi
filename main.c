#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void do_create(int argc, char *argv[]) {
    if (argc != 1) {
        printf("usage: create <size in bytes>\n");
        return;
    }

    size_type disk_size = 0;
    if (sscanf(argv[0], "%u", &disk_size) != 1) {
        printf("create: invalid size argument\n");
    }

    if (_create(disk_size) != 0) {
        printf("create: unable to create disk\n");
    } else {
        printf("create: disk has been created\n");
    }
}

static void do_copy_to(int argc, char *argv[]) {
    if (argc != 1) {
        printf("usage: copy_to <file name>\n");
        return;
    }

    FILE *disk = _open_disk();
    if (disk == NULL) {
        return;
    }
    char *file_name = argv[0];
    FILE *file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("copy_to: unable to open file \"%s\"\n", file_name);
    } else {
        if (_copy_to(disk, file, file_name) != 0) {
            printf("copy_to: unable to copy file \"%s\" to disk\n", file_name);
        } else {
            printf("copy_to: file \"%s\" has been copied to disk\n", file_name);
        }
        fclose(file);
    }

    fclose(disk);
}

static void do_copy_from(int argc, char *argv[]) {
    if (argc != 1) {
        printf("usage: copy_from <file name>\n");
        return;
    }

    FILE *disk = _open_disk();
    if (disk == NULL) {
        return;
    }
    char *file_name = argv[0];

    if (_copy_from(disk, file_name) != 0) {
        printf("copy_from: unable to copy file \"%s\" from disk\n", file_name);
    } else {
        printf("copy_from: file \"%s\" has been copied from disk\n", file_name);
    }

    fclose(disk);
}

static void do_list(int argc, char *argv[]) {
    if (argc != 0) {
        printf("usage: list\n");
        return;
    }

    FILE *disk = _open_disk();
    if (disk == NULL) {
        return;
    }

    if (_list(disk) != 0) {
        printf("list: unable to list disk\n");
    }

    fclose(disk);
}

static void do_remove(int argc, char *argv[]) {
    if (argc != 1) {
        printf("usage: remove <file_name>\n");
        return;
    }

    char *file_name = argv[0];
    FILE *disk = _open_disk();
    if (disk == NULL) {
        return;
    }

    if (_remove(disk, file_name) != 0) {
        printf("remove: unable to remove file \"%s\" from disk\n", file_name);
    } else {
        printf("remove: file \"%s\" has been removed from disk\n", file_name);
    }

    fclose(disk);
}

static void do_map(int argc, char *argv[]) {
    if (argc != 0) {
        printf("usage: map\n");
        return;
    }

    FILE *disk = _open_disk();
    if (disk == NULL) {
        return;
    }

    if (_map(disk) != 0) {
        printf("map: unable to map disk\n");
    }

    fclose(disk);
}

static void do_delete(int argc, char *argv[]) {
    if (argc != 0) {
        printf("usage: delete\n");
        return;
    }

    if (_delete() != 0) {
        printf("delete: unable to delete disk\n");
    } else {
        printf("delete: disk has been deleted\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("usage: ./main <option> [arguments]\n");
        return 0;
    }

    const char *option = argv[1];

    if (strcmp(option, "create") == 0) {
        do_create(argc - 2, argv + 2);
    } else if (strcmp(option, "copy_to") == 0) {
        do_copy_to(argc - 2, argv + 2);
    } else if (strcmp(option, "copy_from") == 0) {
        do_copy_from(argc - 2, argv + 2);
    } else if (strcmp(option, "list") == 0) {
        do_list(argc - 2, argv + 2);
    } else if (strcmp(option, "remove") == 0) {
        do_remove(argc - 2, argv + 2);
    } else if (strcmp(option, "map") == 0) {
        do_map(argc - 2, argv + 2);
    }

    else if (strcmp(option, "delete") == 0) {
        do_delete(argc - 2, argv + 2);
    } else {
        printf("Unrecognized option \"%s\".\n", option);
    }
}
