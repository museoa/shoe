/* map.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements mapping operations.
 */

#ifndef __MAP_H__
#define __MAP_H__

#include "types.h"

#define MAP_MARK(map)                                                      \
        do {                                                               \
          if((map)->size >= 0)                                             \
	    (map)->size = -(map)->size - 1;                                \
        } while(0)
     
#define MAP_UNMARK(map)                                                    \
	((map)->size = -(map)->size - 1)

#define MAP_IS_MARKED(map)                                                 \
        ((map)->size < 0)

struct map
{
  INT size;
  INT hash_size;

  INT used;
  
  struct map *next;
  
  struct map_table *table;
  
  struct map_heap *heap;
};

struct map_entry
{
  struct svalue key;
  struct svalue value;
  struct map_entry *next;
};

struct map_table
{
  struct map_entry **hash;
  struct map_entry *free_list;
};

struct map_heap
{
  INT gc_entries, entries;

  struct process *process;
  
  struct map *first;
};

void map_create(struct process *process);
void map_destroy(struct map_heap *heap);

INT map_debug_memory(struct map_heap *heap);
INT map_debug_objects(struct map_heap *heap);

struct map *map_allocate(struct map_heap *heap);
void map_set(struct map *map, struct svalue *key, struct svalue *value);
struct svalue *map_get(struct map *map, struct svalue *key);
void map_remove(struct map *map, struct svalue *key);

void map_free(struct map_heap *heap, struct svalue *key);

INT map_compare(struct map *a, struct map *b);

#define map_equal(a, b) ((a) == (b))
#define map_hash(map)   ((INT)(map))

void map_sweep(struct map_heap *heap);

#endif /* __MAP_H__ */
