#include "tree.h"
#include "index.h"
#include "pes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tree_from_index(ObjectID *out_id) {
    Index index;

    if (index_load(&index) != 0)
        return -1;

    // empty index → valid empty tree
    if (index.count == 0) {
        return object_write(OBJ_TREE, "", 0, out_id);
    }

    char buffer[4096];
    int offset = 0;

    for (int i = 0; i < index.count; i++) {
        IndexEntry *e = &index.entries[i];

        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&e->hash, hex);

        offset += snprintf(buffer + offset,
            sizeof(buffer) - offset,
            "%o %s %s\n",
            e->mode,
            e->path,
            hex);
    }

    if (object_write(OBJ_TREE, buffer, offset, out_id) != 0)
        return -1;

    return 0;
}
