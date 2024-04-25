#include "kernel/types.h"
#include "user/user.h"
#include "ummalloc.h"
#include "ummalloc_decl.h"
#include "ummalloc_data.h"

static inline void *
malloc_tiny(size_t size) {
    size_t index = size <= 32 ? 1 : (size - 1) / 16;
    size = (index + 1) * 16; // Align to 16 bytes.

    void *data = tiny_allocate(index, size);
    if (data != (void *)0) return data;

    return next_allocate(index, size);
}

static inline void *
malloc_middle(size_t size) {
    size_t index = size <= 640 ? (size + 1535) / 64 : 34 + (size - 513) / 256;

    void *data = middle_allocate(index, size, 4);
    if (data != (void *)0) return data;

    return next_allocate(index, size);
}

void *malloc_huge(size_t size) {
    size_t index = size < 6144 ? 48 : (size - 1) / 4096 + 48;

    void *data = huge_allocate(index, size, 8);
    if (data != (void *)0) return data;

    return next_allocate(index, size);
}

int mm_init(void) {
    mm_list_init();
    mm_align_init();
    return base == 0 ? -1 : 0;
}

void *mm_malloc(uint size) {
    size = ALIGN(size) + sizeof(struct pack);
    if (size <= 512) {
        return malloc_tiny(size);
    } else if (size > 4096) {
        return malloc_huge(size);
    } else {
        return malloc_middle(size);
    }
}

void mm_free(void *) {}

void *mm_realloc(void *ptr, uint size) {
    if (size == 0) return mm_free(ptr), (void *)0;
    if (ptr == 0) return mm_malloc(size);
    return 0;
}
