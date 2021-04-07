/* pair.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This is for the pair.
 */

#ifndef __PAIR_H__
#define __PAIR_H__

#include "types.h"

struct pair_section
{
  INT size;

  struct pair_section *next;
  
  struct pair heap[1];
};

struct pair_heap
{
  INT size;
  INT used;

  INT reduce;   /* Counter used by the heap reduce policy. */
  
  struct pair *free_list;
  struct pair_section *first;
};

#define is_pair_heap_full(p) ((p)->pair_heap.free_list == 0)

void pair_create(struct pair_heap *heap);
void pair_destroy(struct pair_heap *heap);

#define pair_debug_memory(heap)                                            \
        (sizeof(struct pair) * pair_debug_objects(heap))
#define pair_debug_memory_used(heap)                                       \
        (sizeof(struct pair) * pair_debug_objects_used(heap))
INT pair_debug_objects(struct pair_heap *heap);
INT pair_debug_objects_used(struct pair_heap *heap);

struct pair *pair_cons(struct process *process,
		       struct svalue *car, struct svalue *cdr);
struct pair *pair_list(struct process *process, struct svalue *car);
struct pair *pair_nil(struct process *process);

void pair_sweep(struct pair_heap *heap);

void pair_allocate_reduced(struct pair_heap *heap);
void pair_reduce(struct pair_heap *heap);

#endif /* __PAIR_H__ */
