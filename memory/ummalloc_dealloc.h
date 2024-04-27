#pragma once
#include "ummalloc_data.h"

/**
 * @brief Free a chunk whose both previous
 * and next chunks are not free.
 * 
 * 
 * @param pack Chunk to be freed.
 * @attention
 * PREV_INUSE of next chunk should have been set to 0.
 * THIS_INUSE of next chunk should have been set to 1.
 * PREV_INUSE of this chunk should have been set to 1.
 * THIS_INUSE of this chunk should have been set to 0.
 */
static inline void free_chunk(struct pack *pack) {
    size_t size = pack_size(pack);
    size_t index = 0;
    if (size <= 256) {
        IMPOSSIBLE(size <= 32);
        index = (size - 1) / 16;
    } else if (size > 4096) {
        index = size < 6144 ? 48 : (size - 1) / 4096 + 48;
    } else {
        index = size <= 640 ? (size + 1535) / 64 : 34 + (size - 513) / 256;
    }

    struct node *list = &slots[index];
    struct node *node = (struct node *)pack->data;
    list_push(list, node);

    bitmap |= 1ull << index;
}
