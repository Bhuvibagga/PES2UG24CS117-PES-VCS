#include "pes.h"
#include "index.h"
#include "commit.h"
#include "tree.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ───────── INIT ─────────
int cmd_init(int argc, char *argv[]) {
    system("mkdir -p .pes/objects");
    printf("Initialized empty PES repository in .pes/\n");
    return 0;
}

// ───────── ADD ─────────
int cmd_add(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: pes add <file>\n");
        return -1;
    }

    Index index;

    if (index_load(&index) != 0)
        return -1;

    if (index_add(&index, argv[2]) != 0)
        return -1;

    return 0;
}

// ───────── COMMIT ─────────
int cmd_commit(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[2], "-m") != 0) {
        printf("Usage: pes commit -m \"message\"\n");
        return -1;
    }

    ObjectID id;

    if (commit_create(argv[3], &id) != 0) {
        printf("error: commit failed\n");
        return -1;
    }

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&id, hex);

    printf("Committed: %s \"%s\"\n", hex, argv[3]);
    return 0;
}

// ───────── LOG CALLBACK ─────────
static void print_commit(const ObjectID *id, const Commit *commit, void *ctx) {
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);

    printf("commit %s\n", hex);
    printf("%s\n", (char *)commit);
}

// ───────── LOG ─────────
int cmd_log(int argc, char *argv[]) {
    if (commit_walk(print_commit, NULL) != 0) {
        printf("No commits yet.\n");
    }
    return 0;
}

// ───────── MAIN ─────────
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: pes <command>\n");
        return -1;
    }

    char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        return cmd_init(argc, argv);
    }
    else if (strcmp(cmd, "add") == 0) {
        return cmd_add(argc, argv);
    }
    else if (strcmp(cmd, "commit") == 0) {
        return cmd_commit(argc, argv);
    }
    else if (strcmp(cmd, "log") == 0) {
        return cmd_log(argc, argv);
    }
    else {
        printf("Unknown command: %s\n", cmd);
        return -1;
    }
}
