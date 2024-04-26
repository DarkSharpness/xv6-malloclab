#pragma once
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

// /* Used for optimization */
// #define IMPOSSIBLE(x) do { if(x) { __builtin_unreachable(); } } while(0)

/* USED for debug only. */
#ifndef IMPOSSIBLE
#include "user/user.h"
#define IMPOSSIBLE(x) do { if(x) { printf("\nImpossible!\n"); } exit(1); } while(0)
#endif // IMPOSSIBLE

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
    NONE_INUSE  = 0b000,
    PREV_INUSE  = 0b001,
    THIS_INUSE  = 0b010,
    BOTH_INUSE  = 0b011,
    RESERVED    = 0b100,
    FULL_MASK   = 0b111,
};

static inline void
pack_add_meta(struct pack *pack, enum Meta info) {
    pack->size |= info;
}

static inline void
pack_clr_meta(struct pack *pack, enum Meta info) {
    pack->size &= ~info;
}

static inline void
pack_set_meta(struct pack *pack, enum Meta info) {
    pack->size = (pack->size & ~FULL_MASK) | info;
}

static inline void
pack_add_info(struct pack *pack, uint32_t size, enum Meta info) {
    IMPOSSIBLE(size & FULL_MASK);
    pack->size = size | info | (pack->size & FULL_MASK);
}

static inline void
pack_set_info(struct pack *pack, uint32_t size, enum Meta info) {
    IMPOSSIBLE(size & FULL_MASK);
    pack->size = size | info;
}

static inline void
pack_set_prev(struct pack *pack, uint32_t size) {
    pack->prev = size;
}

static inline void
pack_set_size(struct pack *pack, uint32_t size) {
    IMPOSSIBLE(size & FULL_MASK);
    pack->size = (pack->size & FULL_MASK) | size;
}

static inline uint32_t
pack_size(struct pack *pack) {
    return pack->size & ~FULL_MASK;
}

static inline struct pack *
pack_next(struct pack *pack) {
    uint32_t size = pack_size(pack);
    size_t   next = (size_t)pack + size;
    return (struct pack *)next;
}

static inline struct pack *
pack_prev(struct pack *pack) {
    uint32_t size = pack->prev;
    size_t   prev = (size_t)pack - size;
    return (struct pack *)prev;
}

static inline struct pack *
list_pack(struct node *node) {
    return (struct pack *)((size_t)node - sizeof(struct pack));
}

static inline void
list_init(struct node *list) {
    list->next = list->prev = list;
}

static inline int
list_empty(struct node *list) {
    return list->next == list;
}

static inline void
node_link(struct node *prev, struct node *next) {
    prev->next = next;
    next->prev = prev;
}

static inline struct node *
list_pop(struct node *list) {
    struct node *node = list->next;
    node_link(list, node->next);
    return node;
}

static inline void
list_push(struct node *list, struct node *node) {
    node_link(node, list->next);
    node_link(list, node);
}

static inline void
list_erase(struct node *node) {
    node_link(node->prev, node->next);
}
