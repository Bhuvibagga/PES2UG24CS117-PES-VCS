#include "index.h"
#include "pes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

// ─── Sorting helper ─────────────────────────

static int compare_index(const void *a, const void *b) {
    return strcmp(((IndexEntry *)a)->path,
                  ((IndexEntry *)b)->path);
}

// ─── index_load ─────────────────────────

int index_load(Index *index) {
    index->count = 0;

    FILE *f = fopen(".pes/index", "r");
    if (!f) return 0;

    while (1) {
        if (index->count >= MAX_INDEX_ENTRIES)
            break;

        IndexEntry *e = &index->entries[index->count];

        char hash_hex[HASH_HEX_SIZE + 1];

        int ret = fscanf(f, "%o %s %u %s",
                         &e->mode,
                         hash_hex,
                         &e->size,
                         e->path);

        if (ret != 4)
            break;

        if (hex_to_hash(hash_hex, &e->hash) != 0) {
            fclose(f);
            return -1;
        }

        index->count++;
    }

    fclose(f);
    return 0;
}

// ─── index_save ─────────────────────────

int index_save(const Index *index) {
    FILE *f = fopen(".pes/index", "w");
    if (!f) return -1;

    qsort((void *)index->entries,
          index->count,
          sizeof(IndexEntry),
          compare_index);

    for (int i = 0; i < index->count; i++) {
        char hash_hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&index->entries[i].hash, hash_hex);

        fprintf(f, "%o %s %u %s\n",
                index->entries[i].mode,
                hash_hex,
                index->entries[i].size,
                index->entries[i].path);
    }

    fclose(f);
    return 0;
}

// ─── index_add ─────────────────────────

int index_add(Index *index, const char *path) {
    struct stat st;

    if (stat(path, &st) != 0)
        return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    uint8_t *data = NULL;

    if (st.st_size > 0) {
        data = malloc(st.st_size);
        if (!data) {
            fclose(f);
            return -1;
        }

        if (fread(data, 1, st.st_size, f) != (size_t)st.st_size) {
            free(data);
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    ObjectID id;
    if (object_write(OBJ_BLOB, data, st.st_size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    // check if already exists
    int found = -1;
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            found = i;
            break;
        }
    }

    IndexEntry *e;

    if (found >= 0) {
        e = &index->entries[found];
    } else {
        if (index->count >= MAX_INDEX_ENTRIES)
            return -1;

        e = &index->entries[index->count++];
    }

    e->mode = (st.st_mode & S_IXUSR) ? 0100755 : 0100644;
    e->hash = id;
    e->size = st.st_size;
    strcpy(e->path, path);

    return index_save(index);
}

// ─── index_status ─────────────────────────

int index_status(const Index *index) {
    printf("Staged changes:\n");

    if (index->count == 0) {
        printf("  (nothing to show)\n");
    } else {
        for (int i = 0; i < index->count; i++) {
            printf("  staged: %s\n", index->entries[i].path);
        }
    }

    printf("\nUnstaged changes:\n");
    printf("  (nothing to show)\n");

    printf("\nUntracked files:\n");

    DIR *d = opendir(".");
    struct dirent *dir;

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;

        int found = 0;
        for (int i = 0; i < index->count; i++) {
            if (strcmp(index->entries[i].path, dir->d_name) == 0) {
                found = 1;
                break;
            }
        }

        if (!found)
            printf("  untracked: %s\n", dir->d_name);
    }

    closedir(d);
    return 0;
}
