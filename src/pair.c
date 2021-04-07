/* pair.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This is the pair module.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME  "pair"

#include "types.h"

#include "err.h"
#include "mem.h"

#include "process.h"
#include "garb.h"
#include "pair.h"

#define Ki *1024
#define Mi *(1024 Ki)
#define Gi *(1024 Mi)

#define NEW_HEAP_SIZE  (32 Ki)

#define ALLOCATION_RATIO   2
#define REALLOCATION_RATIO 3
#define DEALLOCATION_RATIO 4

static void pair_allocate_heap(struct pair_heap *heap, INT size)
{
  struct pair_section *section;
  struct pair *old_free_list;
  INT i;
  
  section = mem_allocate(sizeof(struct pair_section) +
			 sizeof(struct pair) * (size-1));
  section->next = heap->first;
  heap->first = section;
  section->size = size;

  heap->size += size;
  
  old_free_list = heap->free_list;
  heap->free_list = section->heap;
  for(i = 0; i < section->size-1; i++)
  {
    section->heap[i].car.type = T_FREE;
    section->heap[i].car.u.pair = &section->heap[i+1];
  }
  section->heap[i].car.type = T_FREE;
  section->heap[i].car.u.pair = old_free_list;
  
  DEB(("Pairs allocated: %d (totals: %d)", size, heap->size));
}

void pair_create(struct pair_heap *heap)
{
  heap->size = 0;
  heap->used = 0;
  heap->first = 0;
  heap->free_list = 0;

  heap->reduce = 0;
    
  pair_allocate_heap(heap, NEW_HEAP_SIZE);
}

void pair_destroy(struct pair_heap *heap)
{
  struct pair_section *section, *next;

  for(section = heap->first; section; section = next)
  {
    next = section->next;
    mem_free(section);
  }
}

INT pair_debug_objects(struct pair_heap *heap)
{
  struct pair_section *section;
  INT size = 0;
  
  for(section = heap->first; section; section = section->next)
    size += section->size;

  return size;
}

INT pair_debug_objects_used(struct pair_heap *heap)
{
  struct pair *pair;
  INT n = 0;
  
  for(pair = heap->free_list; pair; pair = pair->car.u.pair)
    n++;

  return pair_debug_objects(heap) - n;
}

static struct pair *pair_void(struct process *process)
{
  struct pair *pair;
  
  if(!process->pair_heap.free_list)
    garb(process);
  
  pair = process->pair_heap.free_list;

  if(!pair)
    err_fatal("Pair heap full.");
  
  process->pair_heap.free_list = pair->car.u.pair;
  process->pair_heap.used++;

  return pair;
}

struct pair *pair_cons(struct process *process,
		       struct svalue *car, struct svalue *cdr)
{
  struct pair *pair = pair_void(process);
  
  pair->car = *car;
  pair->cdr = *cdr;

  return pair;
}

struct pair *pair_list(struct process *process, struct svalue *car)
{
  struct pair *pair = pair_void(process);

  pair->car = *car;
  pair->cdr.type = T_NIL;

  return pair;
}

struct pair *pair_nil(struct process *process)
{
  struct pair *pair = pair_void(process);
  
  pair->car.type = T_NIL;
  pair->cdr.type = T_NIL;

  return pair;
}

void pair_sweep(struct pair_heap *heap)
{
  struct pair_section *section;
  INT i;
  
#if MODULE_DEBUG
  INT n = heap->used;
#endif

  for(section = heap->first; section; section = section->next)
    for(i = 0; i < section->size; i++)
      if(IS_MARKED(&section->heap[i].car))
      {
	UNMARK(&section->heap[i].car);
	UNMARK(&section->heap[i].cdr);
      }
      else if(section->heap[i].car.type != T_FREE)
      {
	section->heap[i].car.type = T_FREE;
	section->heap[i].car.u.pair = heap->free_list;
	heap->free_list = &section->heap[i];
	heap->used--;
      }

  DEB(("Pairs reclaimed: %d of total %d with %d in use",
       n - heap->used, heap->size, heap->used));
  
  /* Allocation and deallocation policies. */
  if(heap->size < ALLOCATION_RATIO*heap->used)
    pair_allocate_heap(heap, MIN(heap->size, 512 Ki));
  
  if(NEW_HEAP_SIZE < heap->size && DEALLOCATION_RATIO*heap->used < heap->size)
    heap->reduce = 1;
  else
    heap->reduce = 0;
}

void pair_allocate_reduced(struct pair_heap *heap)
{
  heap->free_list = 0;
  pair_allocate_heap(heap, MAX(NEW_HEAP_SIZE, REALLOCATION_RATIO*heap->used));
}

void pair_reduce(struct pair_heap *heap)
{
  struct pair_section *section, *next;

  section = heap->first->next;
  heap->first->next = 0;
  
  for(; section; section = next)
  {
    next = section->next;
    heap->size -= section->size;
    mem_free(section);
  }
  
  DEB(("Pairs reduced: total of %d with %d in use", heap->size, heap->used));
}
