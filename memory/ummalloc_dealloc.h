#pragma once
#include "ummalloc_data.h"

/**
 * @brief Free a chunk whose both prev and next chunks are not free.
 * @param pack Chunk to be freed.
 * @attention
 * PREV_INUSE of next chunk should have been set to 0.
 * THIS_INUSE of next chunk should have been set to 1.
 * PREV_INUSE of this chunk should have been set to 1.
 * THIS_INUSE of this chunk should have been set to 0.
 */
static inline void free_chunk(struct pack *pack) {
    IMPOSSIBLE(pack_meta(pack)            != PREV_INUSE);
    IMPOSSIBLE(pack_meta(pack_next(pack)) != THIS_INUSE);

    size_t size = pack_size(pack);
    size_t index = get_index(size);

    struct node *list = &slots[index];
    struct node *node = (struct node *)pack->data;
    list_push(list, node);

    bitmap |= 1ull << index;
}

static inline void
try_safe_remove(struct node *node, struct pack *pack) {
    struct node *prev = node->prev;
    struct node *next = node->next;
    node_link(prev, next);
    if (prev == next)
        bitmap_clr(get_index(pack_size(pack)));
}


/**
 * @brief Try to merge a chunk with its previous chunk.
 * @param pack Chunk to be merged.
 * @return Pack of the merged chunk.
 * @attention First, the pack must be out of any list.
 * In addition, next chunk of current chunk must be in use.
 * Also, bit flags of the merged chunk will be unchanged.
 */
static inline struct pack *
try_merge_prev(struct pack * __restrict pack) {
    enum Meta meta = pack_meta(pack);
    if (meta & PREV_INUSE) return pack;

    size_t size = pack_size(pack);
    struct pack *prev = pack_prev(pack);

    struct node *node = (struct node *)prev->data;

    try_safe_remove(node, prev);
    prev_add_size(prev, size);
    struct pack *next = pack_next(prev);
    pack_set_prev(next, pack_size(prev));

    return prev;
}
