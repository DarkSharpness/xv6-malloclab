#include "kernel/types.h"
#include "user/user.h"
#include "ummalloc.h"
#include "ummalloc_decl.h"
#include "ummalloc_data.h"

int mm_init(void) {
    mm_list_init();
    mm_align_init();
    return base == 0 ? -1 : 0;
}

void *mm_malloc(uint size) {
    size = ALIGN(size) + sizeof(struct pack);

    // Find the corresponding free list.
    struct node *list = get_list(size);
    if (list_empty(list)) {
        return mm_alloc_brk(size);
    } else {
        // struct pack *pack = list_pack(list_pop(list));
        // struct pack *next = pack_prev(pack);
        // pack_set_prev(next, size);
        // pack_add_meta(next, PREV_INUSE);
        // return pack->data;
    }
}

void mm_free(void *ptr) {
    struct node *node = (struct node *)(ptr);
    struct pack *pack = list_pack(node);



}

void *mm_realloc(void *ptr, uint size) {
    if (size == 0) return mm_free(ptr), 0;
    if (ptr == 0) return mm_malloc(size);
    // ptr != 0 && size != 0

}
