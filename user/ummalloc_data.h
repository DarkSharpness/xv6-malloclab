/**
 * Global memory layout as below:
 * 
 *  ......
 * Unmapped memory
 * 
 * ------------------------     <--- base pointer + rest (4096 bytes aligned)
 * 
 * Allocated, yet unused
 * 
 * ------------------------     <--- base pointer here (8 bytes aligned)
 * 
 * custom pack (8 bytes)
 * - prev: -1 as invalid
 * - size: 0  as useless
 * 
 * ------------------------     <--- base pointer - 8 bytes
 * 
 * Memory using by user
 *  ......
 * 
*/

#ifndef _UMMALLOC_DATA_H
#define _UMMALLOC_DATA_H

#include "ummalloc_decl.h"

/**
 * @param x A positive integer no greater than 65536.
 * @return the ceil of log2(x).
 */
static inline size_t log2_ceil(uint32_t x) {
    x = x - 1;

    size_t t = 0, p = 0;

    p = (x >= 256) << 3;
    t += p; x >>= p;

    p = (x >= 16) << 2;
    t += p; x >>= p;

    p = (x >= 4) << 1;
    t += p; x >>= p;

    // 0 -> 0 ; 1 -> 1 ; 2 -> 2; 3 -> 2
    return t + x + (x < 3) - 1;
}

static const uint32_t SLOT_SIZE[] = {
    32, 48, 64, 80, 96, 112, 128,
    256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
    65536, // At most uint16_t.
};

static struct node slots[sizeof(SLOT_SIZE) / sizeof(SLOT_SIZE[0])];

static void *   base; // Base address of the heap.
static size_t   rest; // Remaining size of the heap.

/* Align the top to 4096 and align the base to 8. */
static inline void mm_align_init(void) {
    size_t heap = (size_t)sbrk(PAGE_SIZE);
    size_t temp = heap % PAGE_SIZE;
    size_t size = PAGE_SIZE;

    if (temp != 0) {
        size += PAGE_SIZE - temp;
        sbrk(PAGE_SIZE - temp);
    }

    size_t top = heap + size;
    size_t low = ALIGN(heap) + sizeof(struct pack);

    base = (void *)low;
    rest = top - low;

    pack_set_meta(list_pack(base), PREV_INUSE);
}

/* Initialize all the lists. */
static inline void mm_list_init(void) {
    const int size = sizeof(SLOT_SIZE) / sizeof(SLOT_SIZE[0]);
    for (int i = 0 ; i < size ; ++i) list_init(&slots[i]);
}

/* Allocate memory from the brk */
static inline void *mm_alloc_brk(size_t size) {
    struct pack *pack = list_pack(base);

    size_t heap = (size_t)(base);
    base = (void *)(heap + size);

    if (rest < size) { // Enlarge the heap.
        size_t page = size - rest;
        page = (page + PAGE_SIZE - 1) / PAGE_SIZE;
        rest += page * PAGE_SIZE - size;

        void *addr = sbrk(page * PAGE_SIZE);
        if (addr == (void *)-1) return 0; // Out of memory.
    } else {
        rest -= size;
    }

    struct next *next = pack_prev(pack);

    pack_set_size(pack, size);
    pack_set_prev(next, size);
    pack_set_info(next, 0, PREV_INUSE);

    return heap;
}

/* Return the list of a given size. */
static inline struct node *get_list(size_t size) {
    if (size <= 128) {
        if (size <= SLOT_SIZE[0]) return &slots[0];
        size_t index = (size - 17) / 16;
        return &slots[index];
    } else if (size >= 65536) {
        size_t index = sizeof(SLOT_SIZE) / sizeof(SLOT_SIZE[0]) - 1;
        return &slots[index];
    } else {
        size_t index = log2_ceil(size) - 1;
        return &slots[index];
    }
}


#endif
