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

}