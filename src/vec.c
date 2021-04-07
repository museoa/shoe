/* vec.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements vector operations.
 */

#define MODULE_DEBUG 1
#define MODULE_NAME "vec"

#include "types.h"

#include "err.h"
#include "mem.h"
#include "vec.h"
#include "process.h"

#define VEC_MINIMUM_GC_SIZE 256
#define VEC_GC_RATIO          2

void vec_create(struct process *process)
{
  struct vec_heap *heap;

  heap = &process->vec_heap;
  
  heap->entries = 0;
  heap->gc_entries = VEC_MINIMUM_GC_SIZE;
  heap->process = process;
  
  heap->first = 0;
}

void vec_destroy(struct vec_heap *heap)
{
  struct vec *vec, *next;

  for(vec = heap->first; vec; vec = next)
  {
#if MODULE_DEBUG
    heap->entries -= vec->length;
#endif /* MODULE_DEBUG */
    
    next = vec->next;
    mem_free(vec);
  }

#if MODULE_DEBUG
  if(heap->entries)
    err_fatal("*** Allocated vector svalues = %d ***", heap->entries);
#endif /* MODULE_DEBUG */
}

INT vec_debug_memory(struct vec_heap *heap)
{
  struct vec *vec, *next;
  INT memory = 0;

  for(vec = heap->first; vec; vec = vec->next)
    memory += sizeof(struct vec) + sizeof(struct svalue) * (vec->length-1);

  return memory;
}

INT vec_debug_objects(struct vec_heap *heap)
{
  struct vec *vec, *next;
  INT n = 0;

  for(vec = heap->first; vec; vec = vec->next)
    n++;

  return n;
}

struct vec *vec_allocate(struct vec_heap *heap, INT length)
{
  struct svalue *svalue;
  struct vec *vec;
  INT i, size;

  if(heap->gc_entries < heap->entries)
    heap->process->gc = 1;
  
  vec = mem_allocate(sizeof(struct vec) + sizeof(struct svalue) * (length-1));
  vec->length = length;

  svalue = vec->v;
  for(i = 0; i < length; i++)
    svalue++->type = T_UNDEFINED;

  heap->entries += length;
  vec->next = heap->first;
  heap->first = vec;
  
  return vec;
}

struct vec *vec_copy(struct vec_heap *heap, struct vec *vec)
{
  struct vec *new_vec;
  INT size;

  size = sizeof(struct vec) + sizeof(struct svalue) * (vec->length-1);
  new_vec = mem_allocate(size);
  mem_copy(new_vec, vec, size);

  heap->entries += vec->length;
  
  return new_vec;
}

struct vec *vec_append(struct vec_heap *heap, struct vec *a, struct vec *b)
{
  struct vec *vec;
  INT length;

  if(a->length == 0)
    return vec_copy(heap, b);
  if(b->length == 0)
    return vec_copy(heap, a);

  length = a->length + b->length;
  vec = mem_allocate(sizeof(struct vec) + sizeof(struct svalue) * (length-1));
  vec->length = length;
  
  heap->entries += length;
      
  mem_copy(vec->v, a->v, sizeof(struct svalue) * a->length);
  mem_copy(vec->v + sizeof(struct svalue) * a->length, b->v,
	   sizeof(struct svalue) * b->length);
  
  vec->next = heap->first;
  heap->first = vec;
  
  return vec;
}

void vec_sweep(struct vec_heap *heap)
{
  struct vec *vec, *next, **prev;
  INT i;

  prev = &heap->first;
  vec = heap->first;
  
  while(vec)
  {
    next = vec->next;
    if(VEC_IS_MARKED(vec))
    {
      *prev = vec;
      prev = &vec->next;
      VEC_UNMARK(vec);

      for(i = 0; i < vec->length; i++)
	UNMARK(&vec->v[i]);
    }
    else
    {
      heap->entries -= vec->length;
      mem_free(vec);
    }
    vec = next;
  }
  
  *prev = 0;
  
  heap->gc_entries = VEC_GC_RATIO * MAX(heap->entries, VEC_MINIMUM_GC_SIZE);
}
