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
 * 
 * Slot memory layout:
 *  - Slot 00 ~ 00: Reserved for extremely large memory.
 *  - Slot 01 ~ 31: [32    , 512   ]   step 16 bytes.      Fixed size.
 *  - Slot 32 ~ 33: {576   , 640   }   step 64 bytes.      Dynamic size.
 *  - Slot 34 ~ 47: [768   , 4096  ]   step 256 bytes.     Dynamic size.
 *  - Slot 48 ~ 48: { 6144 }                               Dynamic size.
 *  - Slot 49 ~ 63: [8191  , 65536 ]   step 4096 bytes.    Dynamic size.
*/

#include "ummalloc_decl.h"

/**
 * @param x A positive integer no greater than 65536.
 * @return the ceil of log2(x).
 */
static inline
size_t log2_ceil16(uint16_t x) {
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

/**
 * @param x A positive integer no greater than max_uint32.
 * @return the ceil of log2(x).
 */
static inline
size_t log2_ceil32(uint32_t x) {
    x = x - 1;

    size_t t = 0, p = 0;

    p = (x >= 65536) << 4;
    t += p; x >>= p;

    p = (x >= 256) << 3;
    t += p; x >>= p;

    p = (x >= 16) << 2;
    t += p; x >>= p;

    p = (x >= 4) << 1;
    t += p; x >>= p;

    // 0 -> 0 ; 1 -> 1 ; 2 -> 2; 3 -> 2
    return t + x + (x < 3) - 1;
}

/**
 * @return the ceil of log2(x).
 */
static inline
size_t log2_ceil64(size_t x) {
    x = x - 1;

    size_t t = 0, p = 0;

    p = (x >= 4294967296) << 5;
    t += p; x >>= p;

    p = (x >= 65536) << 4;
    t += p; x >>= p;

    p = (x >= 256) << 3;
    t += p; x >>= p;

    p = (x >= 16) << 2;
    t += p; x >>= p;

    p = (x >= 4) << 1;
    t += p; x >>= p;

    // 0 -> 0 ; 1 -> 1 ; 2 -> 2; 3 -> 2
    return t + x + (x < 3) - 1;
}

struct node *base; // Base address of the heap.
size_t       rest; // Remaining size of the heap.

/* Align the top to 4096 and align the base to 8. */
static inline
void mm_align_init(void) {
    size_t heap = (size_t)sbrk(PAGE_SIZE);
    size_t temp = heap % PAGE_SIZE;
    size_t size = PAGE_SIZE;

    if (temp != 0) {
        size += PAGE_SIZE - temp;
        sbrk(PAGE_SIZE - temp);
    }

    size_t top = heap + size;
    size_t low = ALIGN(heap) + sizeof(struct pack);

    base = (struct node *)low;
    rest = top - low;

    struct node *node = (struct node *)base;
    pack_set_meta(list_pack(node), PREV_INUSE);
}

/* Allocate memory from the brk */
static inline
void *mm_alloc_brk(size_t size) {
    struct pack *pack = list_pack(base);

    size_t heap = (size_t)(base);
    base = (struct node *)(heap + size);

    if (rest < size) { // Enlarge the heap.
        size_t page = size - rest;
        page = (page + PAGE_SIZE - 1) / PAGE_SIZE;
        rest += page * PAGE_SIZE - size;

        void *addr = sbrk(page * PAGE_SIZE);
        if (addr == (void *)-1) return 0; // Out of memory.
    } else {
        rest -= size;
    }

    pack_set_size(pack, size);
    struct pack *next = pack_next(pack);
    pack_set_prev(next, size);
    pack_set_info(next, 0, PREV_INUSE);

    return (void *)heap;
}

uint64_t    bitmap;     // Bitmap for all slots.
struct node slots[64];  // 64 slots for different size.

static inline
uint64_t next_free(size_t size) {
    uint64_t mask = bitmap;
    uint64_t temp = -1;
    mask &= temp << (size + 1);
    return mask & (-mask);
}

static inline void
bitmap_set(size_t index) { bitmap |= 1 << index; }

static inline void
bitmap_clr(size_t index) { bitmap &= ~(1 << index); }

/* Safely remove the first node from the list. */
static inline struct node *
list_extract(size_t index) {
    struct node *list = &slots[index];
    struct node *node = list_pop(list);
    if (list_empty(list)) bitmap_clr(index);
    return node;
}


static inline void * 
pack_allocate(struct pack *pack, size_t size) {
    struct pack *next = pack_next(pack);
    pack_set_prev(next, size);
    pack_set_info(next, 0, PREV_INUSE);
    return pack->data;
}

static inline size_t
pack_try_split(struct pack *, size_t size, size_t ) {
    /// TODO: Split the pack into two parts if necessary.
    return size;
}

static inline void *
split_allocate(size_t index, size_t need) {
    struct node *node = list_extract(index);
    struct pack *pack = list_pack(node);
    size_t size = pack_size(pack);
    size = pack_try_split(pack, size, need);
    return pack_allocate(pack, size);
}

static inline void *
tiny_allocate(size_t index, size_t need) {
    if (list_empty(slots + index)) return (void *)0;
    struct node *node = list_extract(index);
    struct pack *pack = list_pack(node);
    return pack_allocate(pack, need);
}

static inline void *
middle_allocate(size_t index, size_t need, size_t iteration) {
    struct node *list = slots + index;
    struct node *head = list->next;
    while (iteration-- != 0 && head != list) {
        struct pack *pack = list_pack(head);
        size_t size = pack_size(pack);

        if (size >= need) {
            struct node *prev = head->prev;
            struct node *next = head->next;
            node_link(prev, next);
            if (prev == next) bitmap_clr(index);
            return pack_allocate(pack, size);
        }

        head = head->next;
    }

    return (void *)0;
}

static inline void *
next_allocate(size_t index, size_t size) {
    uint64_t lowbit = next_free(index);
    if (lowbit == 0) return mm_alloc_brk(size);
    size_t position = log2_ceil64(lowbit);
    return split_allocate(position, size);
}

static inline void *
huge_allocate(size_t index, size_t size, size_t iteration) {
    return mm_alloc_brk(size);
}

/* Initialize all the lists. */
static inline
void mm_list_init(void) {
    for (size_t i = 0; i < 64; i++) list_init(&slots[i]);
}
