/* garb.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This is the garbage collector module.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME  "garb"

#include "types.h"

#include "err.h"
#include "mem.h"

#include "big.h"
#include "map.h"
#include "process.h"
#include "pair.h"
#include "str.h"
#include "vec.h"
#include "garb.h"

/* 
 * Current scheme: mark-and-sweep using the C stack.  Not
 * really a very good idea since the stack can run out of
 * space.  We have however taken some measurements against
 * common problems with very long lists using goto.
 *
 * The garbage collector also features a stop-and-copy
 * mechanism used when the pair allocation policies believes
 * the heap contains too much unused space.  Thus, the heap
 * can both grow and shrink.
 *
 * A last remark: In the current implementation a pair is
 * not marked directly, only indirectly using the stored
 * car and cdr svalues. It is sub-optimal to mark both car
 * and cdr, but that's the way it is.
 */

static void mark(struct svalue *svalue)
{
  INT i, length, type;

 next:
  if(IS_MARKED(svalue))
    return;

  type = svalue->type;
  MARK(svalue);
  
  switch(type)
  {
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    BIG_MARK(svalue->u.big);
    return;
#endif /* USE_BIG_INTEGERS */
    
  case T_MAPPING:
  {
    struct map_table *table;
    struct map_entry *entry;
    struct map *map;

    map = svalue->u.map;
    table = map->table;
    MAP_MARK(map);

    for(i = 0; i < map->hash_size; i++)
      for(entry = table->hash[i]; entry; entry = entry->next)
      {
	mark(&entry->key);
	mark(&entry->value);
      }
    
    return;
  }
    
  case T_STRING:
  case T_SYMBOL:
    STR_MARK(svalue->u.str);
    return;
    
  case T_LABEL:
  case T_VECTOR:
    length = svalue->u.vec->length;
    VEC_MARK(svalue->u.vec);   /* The length of the vector is modified... */
    svalue = svalue->u.vec->v;
    
    for(i = 0; i < length; i++)
      mark(svalue++);
    return;
    
  case T_PAIR:
  case T_LAMBDA:
  case T_CONTINUATION:
    mark(&CAR(svalue->u.pair));
    svalue = &CDR(svalue->u.pair);
    goto next;   /* This allows us to follow arbitrary long lists. */
  }
}

/*
 * This routines slims the pair heap by copying the pairs to a new
 * heap and freeing the old one.  The routine can be optimized with
 * an order of a magnitude since we know that the old heap just
 * has been collected.
 */

#define COPY_CONS(r, a, b)                                                 \
        r = heap->free_list;                                               \
        heap->free_list = r->car.u.pair;                                   \
        CAR(r) = a;                                                        \
        CDR(r) = b

static void copy(struct pair_heap *heap, struct svalue *svalue)
{
  INT i, length, type;

 next:
  if(IS_MARKED(svalue))
    return;
  
  type = svalue->type;
  MARK(svalue);
  
  switch(type)
  {
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    BIG_MARK(svalue->u.big);
    return;
#endif /* USE_BIG_INTEGERS */
    
  case T_MAPPING:
  {
    struct map_table *table;
    struct map_entry *entry;
    struct map *map;

    map = svalue->u.map;
    table = map->table;
    MAP_MARK(map);

    for(i = 0; i < map->hash_size; i++)
      for(entry = table->hash[i]; entry; entry = entry->next)
      {
	copy(heap, &entry->key);
	copy(heap, &entry->value);
      }
    
    return;
  }
    
  case T_STRING:
  case T_SYMBOL:
    STR_MARK(svalue->u.str);
    return;
    
  case T_LABEL:
  case T_VECTOR:
    length = svalue->u.vec->length;
    VEC_MARK(svalue->u.vec);   /* The length of the vector is modified... */
    svalue = svalue->u.vec->v;
    
    for(i = 0; i < length; i++)
      copy(heap, svalue++);
    return;
    
  case T_PAIR:
  case T_LAMBDA:
  case T_CONTINUATION:
    if(IS_MARKED(&CAR(svalue->u.pair)))
      svalue->u.pair = CAR(svalue->u.pair).u.pair;
    else
    {
      struct pair *old_pair;

      old_pair = svalue->u.pair;
      COPY_CONS(svalue->u.pair, CAR(old_pair), CDR(old_pair));
      CAR(old_pair).u.pair = svalue->u.pair;
      MARK(&CAR(old_pair));
      
      copy(heap, &CAR(svalue->u.pair));
      svalue = &CDR(svalue->u.pair);
      goto next;   /* This allows us to follow arbitrary long lists. */
    }
  }
}

void garb(struct process *process)
{
  int i;

  for(i = 0; i < N_REGISTERS; i++)
  {
    mark(&process->reg[i]);
    UNMARK(&process->reg[i]);
  }
  
  for(i = 0; i < N_TRAPS; i++)
  {
    mark(&process->trap[i]);
    UNMARK(&process->trap[i]);
  }
  
  mark(&process->stack);
  UNMARK(&process->stack);

  mark(&process->error);
  UNMARK(&process->error);
  
  mark(&process->program);
  UNMARK(&process->program);
  
  pair_sweep(&process->pair_heap);
  map_sweep(&process->map_heap);
  str_sweep(&process->str_heap);
  vec_sweep(&process->vec_heap);

#ifdef USE_BIG_INTEGERS
  big_sweep(&process->big_heap);
#endif /* USE_BIG_INTEGERS */

  process->gc = 0;
}

void garb_and_reduce(struct process *process)
{
  struct pair_heap *heap;
  INT i;

  garb(process);

  heap = &process->pair_heap;
  
  if(heap->reduce)
  {
    DEB(("garb_and_reduce (used: %d, size: %d)", heap->used, heap->size));
    
    pair_allocate_reduced(heap);
    
    for(i = 0; i < N_REGISTERS; i++)
      copy(heap, &process->reg[i]);
    
    for(i = 0; i < N_TRAPS; i++)
      copy(heap, &process->trap[i]);
    
    copy(heap, &process->stack);
    copy(heap, &process->error);
    copy(heap, &process->program);

    for(i = 0; i < N_REGISTERS; i++)
      UNMARK(&process->reg[i]);
    
    for(i = 0; i < N_TRAPS; i++)
      UNMARK(&process->trap[i]);
  
    UNMARK(&process->stack);
    UNMARK(&process->error);
    UNMARK(&process->program);
    
    map_sweep(&process->map_heap);
    str_sweep(&process->str_heap);
    vec_sweep(&process->vec_heap);

#ifdef USE_BIG_INTEGERS
  big_sweep(&process->big_heap);
#endif /* USE_BIG_INTEGERS */
  
    pair_reduce(heap);
    
    pair_sweep(&process->pair_heap);
    
    DEB(("---> garb_and_reduce (used: %d, size: %d)", heap->used, heap->size));
    
    heap->reduce = 0;
  }
}
