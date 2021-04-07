/* vec.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements vector operations.
 */

#ifndef __VEC_H__
#define __VEC_H__

#include "types.h"

#define VEC_MARK(vec)                                                      \
        do {                                                               \
          if((vec)->length >= 0)                                           \
	    (vec)->length = -(vec)->length - 1;                            \
        } while(0)
     
#define VEC_UNMARK(vec)                                                    \
	((vec)->length = -(vec)->length - 1)

#define VEC_IS_MARKED(vec)                                                 \
        ((vec)->length < 0)

struct vec
{
  INT length;
  struct vec *next;
  
  struct svalue v[1];
};

struct vec_heap
{
  INT gc_entries, entries;

  struct process *process;
  
  struct vec *first;
};

void vec_create(struct process *process);
void vec_destroy(struct vec_heap *heap);

INT vec_debug_memory(struct vec_heap *heap);
INT vec_debug_objects(struct vec_heap *heap);

struct vec *vec_allocate(struct vec_heap *heap, INT length);
struct vec *vec_copy(struct vec_heap *heap, struct vec *vec);
struct vec *vec_append(struct vec_heap *heap, struct vec *a, struct vec *b);

void vec_sweep(struct vec_heap *heap);

#endif /* __VEC_H__ */
