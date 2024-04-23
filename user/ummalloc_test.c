#include "kernel/types.h"

//

#include "kernel/fcntl.h"
#include "ummalloc.h"
#include "user/user.h"
typedef enum { ALLOC, FREE, REALLOC } op_t;

char fgetc(int fd) {
  char ret;
  read(fd, &ret, sizeof(char));
  return ret;
}

int fgetint(int fd) {
  int ret = 0;
  char c = fgetc(fd);
  while (c < '0' || c > '9') c = fgetc(fd);
  do {
    ret = ret * 10 + (c - '0');
    c = fgetc(fd);
  } while ('0' <= c && c <= '9');
  return ret;
}

op_t fgetop(int fd) {
  char c = fgetc(fd);
  while (1) {
    switch (c) {
      case 'a':
        return ALLOC;
      case 'f':
        return FREE;
      case 'r':
        return REALLOC;
      default:
        c = fgetc(fd);
    }
  }
}

void sys_err(char* msg) {
  printf("sys_error : %s\n", msg);
  exit(2);
}

void lib_err(char* msg) {
  printf("lib_error : %s\n", msg);
  exit(3);
}

struct range_t {
  void* lo;
  void* hi;
  struct range_t* nxt;
};

struct range_t* range_head;
struct range_t* range_spc;
struct range_t* range_free_head;
void* begin_heap_top;

void init_range(int num_ids) {
  range_spc = malloc((num_ids + 1) * sizeof(struct range_t));
  range_head = range_spc;
  range_head->nxt = 0;
  range_free_head = range_spc + 1;
  for (int i = 1; i < num_ids; ++i) {
    range_spc[i].nxt = range_spc + i + 1;
  }
}

void add_range(void* lo, uint size) {
  void* hi = lo + size;
  if (!(lo >= begin_heap_top)) {
    lib_err("alloc not in heap");
  }
  // Check vaild
  struct range_t* curr;
  for (curr = range_head; curr->nxt; curr = curr->nxt) {
    if (!(hi <= curr->nxt->lo || curr->nxt->hi <= lo)) lib_err("alloc conflict");
  }

  // Add into linked list
  struct range_t* new = range_free_head;
  range_free_head = new->nxt;
  curr->nxt = new;
  new->lo = lo;
  new->hi = hi;
  new->nxt = 0;
}

void rm_range(void* lo) {
  struct range_t* curr;
  for (curr = range_head; curr->nxt; curr = curr->nxt) {
    if (curr->nxt->lo == lo) break;
  }
  struct range_t* rm = curr->nxt;
  if (rm == 0) sys_err("range to free not found");
  curr->nxt = rm->nxt;
  rm->nxt = range_free_head;
  range_free_head = rm;
}

void memcheck(void* mem, int ch, uint size) {
  char* curr = (char*)mem;
  int i;
  for (i = 0; i < size; i++) {
    if (*curr != (char)ch) lib_err("realloc: data not preserved");
    curr++;
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(2, "Usage: ummalloc_test tracefile\n");
    exit(1);
  }
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) sys_err("open trace fail");
  int num_ids = fgetint(fd);
  int num_ops = fgetint(fd);
  void** ptr = malloc(num_ids * sizeof(void*));
  int* ptr_size = malloc(num_ids * sizeof(int));
  init_range(num_ids);
  int max_total_size = 0;
  int total_size = 0;
  uint begin_clk = getclk();
  begin_heap_top = sbrk(0);
  if (mm_init() == -1) lib_err("mm_init");
  for (int i = 0; i < num_ops; ++i) {
    op_t op = fgetop(fd);
    int id, size;
    switch (op) {
      case ALLOC:
        id = fgetint(fd);
        size = fgetint(fd);
        ptr[id] = mm_malloc(size);
        if (ptr[id] == 0) lib_err("mm_malloc");
        if (size) add_range(ptr[id], size);
        ptr_size[id] = size;
        total_size += size;
        break;
      case FREE:
        id = fgetint(fd);
        mm_free(ptr[id]);
        if (ptr_size[id]) rm_range(ptr[id]);
        total_size -= ptr_size[id];
        break;
      case REALLOC:
        id = fgetint(fd);
        size = fgetint(fd);
        void* old_ptr = ptr[id];
        uint min_size = size < ptr_size[id] ? size : ptr_size[id];
        memset(old_ptr, i & 0xFF, min_size);
        ptr[id] = mm_realloc(old_ptr, size);
        if (size && ptr[id] == 0) {
          printf("heap used : %d bytes\n", (void*)sbrk(0) - begin_heap_top);
          lib_err("realloc");
        }
        memcheck(ptr[id], i & 0xFF, min_size);
        total_size += size - ptr_size[id];
        if (ptr_size[id]) rm_range(old_ptr);
        if (size) add_range(ptr[id], size);
        ptr_size[id] = size;
        break;
    }
    if (max_total_size < total_size) max_total_size = total_size;
  }
  uint finish_clk = getclk();
  void* finish_heap_top = sbrk(0);
  printf("heap used : %d bytes\n", finish_heap_top - begin_heap_top);
  printf("time : %l\n", finish_clk - begin_clk);
  exit(0);
}