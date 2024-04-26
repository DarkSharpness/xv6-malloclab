#include "kernel/types.h"
#include "user/user.h"
#include "ummalloc.h"
#include "ummalloc_impl.h"

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
