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

/** @return the ceil of log2(x). */
static inline size_t
log2_ceil64(size_t x) {
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

struct node *base; // Base address of the heap.
uint64_t    bitmap;     // Bitmap for all slots.
struct node slots[64];  // 64 slots for different size.

static void *malloc_brk(size_t);
static void  free_chunk(struct pack *);
