/* mem.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Memory management.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME "mem"

#include "types.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include "err.h"
#include "exit.h"
#include "mem.h"

static INT sizeof_allocations;

static void mem_exit(void)
{
  if(sizeof_allocations)
    err_fatal("*** Allocated memory = %d ***", sizeof_allocations);
}

void mem_init(void)
{
  sizeof_allocations = 0;
  
  EXIT_REGISTER(mem_exit);
}

void* mem_allocate_zeroed(INT amount)
{
  void *ptr;

  sizeof_allocations++;
  
  ptr = calloc(amount, 1);
  if(ptr == 0)
    err_fatal("Out of memory.");
  return ptr;
}

void* mem_allocate(INT amount)
{
  return mem_allocate_zeroed(amount);
}

void *mem_reallocate(void *ptr, INT amount)
{
  if(!ptr && amount)
    sizeof_allocations++;
  if(ptr && !amount)
    sizeof_allocations--;
  
  ptr = realloc(ptr, amount);
  
  if(ptr == 0 && amount > 0)
    err_fatal("Out of memory.");
  return ptr;
}

void *mem_duplicate(void *ptr, INT amount)
{
  void *r;
  
  r = mem_allocate_zeroed(amount);   /* FIXME: Does not need to be zeroed. */
  mem_copy(r, ptr, amount);

  return r;
}

void mem_free(void *ptr)
{
  sizeof_allocations--;
  free(ptr);
}

#ifndef HAVE_MEMCMP
INT mem_equal(void *_a, void *_b, INT length)
{
  BYTE *a = _a;
  BYTE *b = _b;
  while(length--)
    if(*a++ != *b++)
      return 0;
  return 1;
}
#endif

#ifndef HAVE_MEMCPY
void *mem_copy(void *_dst, void *_src, INT length)
{
  BYTE *dst = _dst;
  BYTE *src = _src;
  
  while(length--)
    *dst++ = *src++;
  
  return _dst;
}
#endif

#ifndef HAVE_MEMSET
void *mem_set(void *_dst, BYTE x, INT length)
{
  BYTE *dst = _dst;
  
  while(length--)
    *dst++ = x;
  
  return _dst;
}
#endif
