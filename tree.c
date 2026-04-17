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
    TempEntry temp[MAX_TREE_ENTRIES];
    int temp_count = 0;

    size_t prefix_len = strlen(prefix);

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
            size_t dir_len = slash - remaining;

            char dirname[256];
            strncpy(dirname, remaining, dir_len);
            dirname[dir_len] = '\0';

            int found = 0;
            for (int j = 0; j < temp_count; j++) {
                if (strcmp(temp[j].name, dirname) == 0) {
                    found = 1;
                    break;
                }
            }

            if (!found) {
                strcpy(temp[temp_count].name, dirname);
                temp[temp_count].is_dir = 1;
                temp_count++;
            }
        }
    }

    // Process directories recursively
    for (int i = 0; i < temp_count; i++) {
        char new_prefix[512];
        snprintf(new_prefix, sizeof(new_prefix), "%s%s/", prefix, temp[i].name);

        ObjectID sub_id;
        if (build_tree(index, new_prefix, &sub_id) != 0)
            return -1;

        TreeEntry *te = &tree.entries[tree.count++];
        te->mode = MODE_DIR;
        strcpy(te->name, temp[i].name);
        te->hash = sub_id;
    }

    // Serialize + write
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
    return -1;
}
