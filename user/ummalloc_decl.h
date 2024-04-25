#ifndef _UMMALLOC_DECL_H
#define _UMMALLOC_DECL_H

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef uint64_t            size_t;

struct pack {
    uint32_t prev;  // Previous chunk size
    uint32_t size;  // This chunk size
    char data[0];   // Data
};

struct node {
    struct node *prev;
    struct node *next;
};

enum Meta {
    PREV_INUSE  = 0b001,
    RESERVED    = 0b110,
    FULL_MASK   = 0b111,
};

inline void pack_add_meta(struct pack *pack, enum Meta info) {
    pack->size |= info;
}

inline void pack_set_meta(struct pack *pack, enum Meta info) {
    pack->size = (pack->size & ~FULL_MASK) | info;
}

inline void pack_set_info(struct pack *pack, uint32_t size, enum Meta info) {
    pack->size = size | info;
}

inline void pack_set_prev(struct pack *pack, uint32_t size) {
    pack->prev = size;
}

inline void pack_set_size(struct pack *pack, uint32_t size) {
    pack->size = (pack->size & FULL_MASK) | size;
}

inline uint32_t pack_size(struct pack *pack) {
    return pack->size & ~FULL_MASK;
}

inline struct pack *pack_prev(struct pack *pack) {
    uint32_t size = pack_size(pack);
    size_t   next = (size_t)pack + size;
    return (struct pack *)next;
}

inline struct pack *pack_prev(struct pack *pack) {
    uint32_t size = pack->prev;
    size_t   prev = (size_t)pack - size;
    return (struct pack *)prev;
}



inline struct pack *list_pack(struct node *node) {
    return (struct pack *)((size_t)node - sizeof(struct pack));
}

inline void list_init(struct node *list) {
    list->next = list->prev = list;
}

inline int list_empty(struct node *list) {
    return list->next == list;
}

inline void link(struct node *prev, struct node *next) {
    prev->next = next;
    next->prev = prev;
}

inline struct node *list_pop(struct node *list) {
    struct node *node = list->next;
    link(list, node->next);
    return node;
}

inline void list_push(struct node *list, struct node *node) {
    link(node, list->next);
    link(list, node);
}

#endif
