#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(uint)))

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) { return 0; }

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(uint size) {
  int newsize = ALIGN(size + SIZE_T_SIZE);
  void *p = sbrk(newsize);
  if (p == (void *)-1)
    return 0;
  else {
    *(uint *)p = size;
    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, uint size) {
  void *oldptr = ptr;
  void *newptr;
  uint copySize;

  if (size == 0) {
    free(ptr);
    return 0;
  }
  
  newptr = mm_malloc(size);
  if (newptr == 0) return 0;
  copySize = *(uint *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize) copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}
