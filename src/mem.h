/* mem.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Memory management.
 */

#include <string.h>

void mem_init(void);

void *mem_allocate_zeroed(INT amount);
void *mem_allocate(INT amount);
void *mem_reallocate(void *ptr, INT amount);

void *mem_duplicate(void *ptr, INT amount);

void mem_free(void *ptr);

#ifdef HAVE_MEMCMP
#define mem_equal(a, b, length) (memcmp(a, b, length) == 0)
#else
INT mem_equal(void *a, void *b, INT length);
#endif

#ifdef HAVE_MEMCPY
#define mem_copy(dst, src, length) memcpy(dst, src, length)
#else
void *mem_copy(void *_dst, void *_src, INT length);
#endif

#ifdef HAVE_MEMSET
#define mem_set(dst, x, length) memset(dst, x, length)
#else
void *mem_set(void *_dst, BYTE x, INT length);
#endif

#define mem_zero(dst, length) mem_set(dst, 0, length)
