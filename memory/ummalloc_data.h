#pragma once
/**
 * Global memory layout as below:
 * 
 * ------------------------     <--- High address of the heap
 *  ......
 * Unmapped memory
 * 
 * ------------------------     <--- base pointer, aligned to 4096 bytes
 * custom pack (8 bytes)
 * - prev: -1 as invalid
 * - size: 0  as useless
 * - flag: 2  as NOW_INUSE
 * ------------------------     <--- base pointer - 8 bytes
 * 
 * Memory using by user
 *  ......
 * 
 * ------------------------     <--- Low address of the heap
 * 
 * Slot memory layout:
 *  - Slot 00 ~ 00: Reserved for extremely large memory.
 *  - Slot 01 ~ 01: { 32 }                                  Fixed size.
 *  - Slot 02 ~ 31: [48    , 512   ]   step 16 bytes.       Fixed size.
 *  - Slot 32 ~ 33: {576   , 640   }   step 64 bytes.       Dynamic size.
 *  - Slot 34 ~ 47: [768   , 4096  ]   step 256 bytes.      Dynamic size.
 *  - Slot 48 ~ 48: { 6144 }                                Dynamic size.
 *  - Slot 49 ~ 63: [8191  , 65536 ]   step 4096 bytes.     Dynamic size.
 * 
 * Summary:
 *  - Extreme:  [0x00, 0x01)    # Not implemented yet.
 *  - Fast:     [0x01, 0x02)
 *  - Tiny:     [0x02, 0x20)
 *  - Middle:   [0x20, 0x30)
 *  - Huge:     [0x30, 0x40)
*/

#include "user/user.h"
#include "ummalloc_decl.h"

/**
 * @brief Calculate the ceil of log2(x).
 * @example
 *  63 -> 6, 64 -> 6, 65 -> 7, 0 -> 64
*/
static inline size_t log2_ceil64(size_t x) {
    x = x - 1;
    size_t t = 0, p = 0;

    p = (x >> 32 != 0) << 5;
    t += p; x >>= p;

    p = (x >> 16 != 0) << 4;
    t += p; x >>= p;

    p = (x >> 8  != 0) << 3;
    t += p; x >>= p;

    p = (x >> 4  != 0) << 2;
    t += p; x >>= p;

    p = (x >> 2  != 0) << 1;
    t += p; x >>= p;

    // 0 -> 0 ; 1 -> 1 ; 2 -> 2; 3 -> 2
    return t + x + (x < 3) - 1;
}


/**
 * @brief Calculate the floor of log2(x).
 * @example
 *  1 -> 0, 2 -> 1, 3 -> 1, 4 -> 2
*/
static inline size_t log2_floor64(size_t x) {
    return log2_ceil64(x + 1) - 1;
}

struct node *base;      // Base address of the heap.
uint64_t    bitmap;     // Bitmap for all slots.
struct node fast_full;  // Fast slot for those full.
struct node slots[64];  // 64 slots for different size.

static void *malloc_brk(size_t);
static void  free_chunk(struct pack *);
static struct pack *try_merge_prev(struct pack * __restrict);
static struct pack *try_merge_next(struct pack * __restrict);

static inline uint64_t next_free(size_t size) {
    uint64_t mask = bitmap;
    uint64_t temp = -1;
    mask &= temp << (size + 1);
    return mask & (-mask);
}

static inline void bitmap_set(size_t index) { bitmap |= 1ull << index; }
static inline void bitmap_clr(size_t index) { bitmap &= ~(1ull << index); }

/** Safely remove a node from the list. */
static inline void
safe_remove(struct node *__restrict node, size_t index) {
    struct node *prev = node->prev;
    struct node *next = node->next;
    node_link(prev, next);
    if (prev == next) bitmap_clr(index);
}

static inline size_t get_index(size_t size) {
    size_t index = 0;
    if (size <= 256) {
        IMPOSSIBLE(size <= 32);
        index = (size - 1) / 16;
    } else if (size > 4096) {
        index = size < 6144 ? 48 : (size - 1) / 4096 + 48;
    } else {
        index = size <= 640 ? (size + 1535) / 64 : 34 + (size - 513) / 256;
    }
    return index;
}
