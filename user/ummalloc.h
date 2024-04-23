extern int mm_init(void);
extern void *mm_malloc(uint size);
extern void mm_free(void *ptr);
extern void *mm_realloc(void *ptr, uint size);
