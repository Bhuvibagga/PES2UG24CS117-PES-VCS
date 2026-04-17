#include "index.h"
#include "object.h"

// Helper structure for recursion
typedef struct {
    char name[256];
    Tree subtree;
    int is_dir;
    ObjectID hash;
    uint32_t mode;
} TempEntry;

// Recursive function to build tree
static int build_tree(Index *index, const char *prefix, ObjectID *out_id) {
    Tree tree = {0};

    size_t prefix_len = strlen(prefix);

    // Track directories
    char seen_dirs[MAX_TREE_ENTRIES][256];
    int dir_count = 0;

    for (int i = 0; i < index->count; i++) {
        IndexEntry *entry = &index->entries[i];

        if (strncmp(entry->path, prefix, prefix_len) != 0)
            continue;

        const char *remaining = entry->path + prefix_len;

        if (*remaining == '\0') continue;

        char *slash = strchr(remaining, '/');

        if (!slash) {
            // FILE
            TreeEntry *te = &tree.entries[tree.count++];
            te->mode = entry->mode;
            strcpy(te->name, remaining);
            te->hash = entry->hash;
        } else {
            // DIRECTORY
            size_t len = slash - remaining;

            char dirname[256];
            strncpy(dirname, remaining, len);
            dirname[len] = '\0';

            int exists = 0;
            for (int j = 0; j < dir_count; j++) {
                if (strcmp(seen_dirs[j], dirname) == 0) {
                    exists = 1;
                    break;
                }
            }

            if (!exists) {
                strcpy(seen_dirs[dir_count++], dirname);
            }
        }
    }

    // Recursively build subtrees
    for (int i = 0; i < dir_count; i++) {
        char new_prefix[512];
        snprintf(new_prefix, sizeof(new_prefix), "%s%s/", prefix, seen_dirs[i]);

        ObjectID sub_id;
        if (build_tree(index, new_prefix, &sub_id) != 0)
            return -1;

        TreeEntry *te = &tree.entries[tree.count++];
        te->mode = MODE_DIR;
        strcpy(te->name, seen_dirs[i]);
        te->hash = sub_id;
    }

    // Serialize and write tree
    void *data;
    size_t len;

    if (tree_serialize(&tree, &data, &len) != 0)
        return -1;

    if (object_write(OBJ_TREE, data, len, out_id) != 0) {
        free(data);
        return -1;
    }

    free(data);
    return 0;
}
int tree_from_index(ObjectID *id_out) {
    Index index;

    if (index_load(&index) != 0)
        return -1;

    Tree tree = {0};

    for (int i = 0; i < index.count; i++) {
        TreeEntry *te = &tree.entries[tree.count++];

        te->mode = index.entries[i].mode;
        strcpy(te->name, index.entries[i].path);
        te->hash = index.entries[i].hash;
    }

    void *data;
    size_t len;

    if (tree_serialize(&tree, &data, &len) != 0)
        return -1;

    if (object_write(OBJ_TREE, data, len, id_out) != 0) {
        free(data);
        return -1;
    }

    free(data);
    return 0;
}
int tree_from_index(ObjectID *id_out) {
    Index index;

    if (index_load(&index) != 0)
        return -1;

    return build_tree(&index, "", id_out);
}
