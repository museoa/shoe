/* map.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements mapping operations.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME "map"

#include "types.h"

#include "err.h"
#include "mem.h"
#include "map.h"
#include "str.h"
#include "process.h"
#include "svalue.h"

#define MAP_MINIMUM_GC_SIZE 256
#define MAP_GC_RATIO          2

#define NEW_INDEX_SIZE  8
#define AVG_LINK_LENGTH 4
#define MIN_LINK_LENGTH 1

#define check_resize(map)                                                  \
        do {                                                               \
          if(map->size*2 > NEW_INDEX_SIZE &&                               \
	     map->used < map->hash_size * MIN_LINK_LENGTH)                 \
	    map_resize(map, map->size/2);                                  \
	} while(0)

#define MAP_HEAP_SIZE(map)                                                 \
        (sizeof(struct map_table) +                                        \
	 sizeof(struct map_entry) * (map)->size +                          \
	 sizeof(struct map_entry *) * (map)->hash_size)

void map_create(struct process *process)
{
  struct map_heap *heap;

  heap = &process->map_heap;
  
  heap->entries = 0;
  heap->gc_entries = MAP_MINIMUM_GC_SIZE;
  heap->process = process;
  
  heap->first = 0;
}

void map_destroy(struct map_heap *heap)
{
  struct map *map, *next;

  map = heap->first;
  while(map)
  {
    next = map->next;

#if MODULE_DEBUG
    heap->entries -= map->size;
#endif /* MODULE_DEBUG */
      
    mem_free(map->table);
    mem_free(map);
    
    map = next;
  }

#if MODULE_DEBUG
  if(heap->entries)
    err_fatal("*** Allocated mapping svalues = %d ***", heap->entries);
#endif /* MODULE_DEBUG */
}

INT map_debug_memory(struct map_heap *heap)
{
  struct map *map, *next;
  INT memory = 0;

  for(map = heap->first; map; map = map->next)
    memory += sizeof(struct map) + MAP_HEAP_SIZE(map);

  return memory;
}

INT map_debug_objects(struct map_heap *heap)
{
  struct map *map, *next;
  INT n = 0;

  for(map = heap->first; map; map = map->next)
    n++;

  return n;
}

static void map_allocate_heap(struct map_heap *heap, struct map *map,INT size)
{
  struct map_table *table;
  INT i;
  
  map->size = size;
  map->hash_size = map->size / AVG_LINK_LENGTH + 1;
  map->used = 0;
  map->heap = heap;
  
  if(heap->gc_entries < heap->entries)
  {
    DEB(("Garbage collect mappings (%d < %d).",
	 heap->gc_entries, heap->entries));
    heap->process->gc = 1;
  }

  heap->entries += size;
  
  table = mem_allocate(MAP_HEAP_SIZE(map));
  map->table = table;
  
  /* Link up free list. */
  table->free_list = (struct map_entry *) &table[1];
  for(i = 0; i < map->size-1; i++)
    table->free_list[i].next = &table->free_list[i+1];
  table->free_list[i].next = 0;

  /* Zerofy hash table. */
  table->hash = (struct map_entry **) &table->free_list[map->size];
  for(i = 0; i < map->hash_size; i++)
    table->hash[i] = 0;
}

static void map_resize(struct map *map, INT new_size)
{
  struct map_entry *entry, *next, *prev, *new_entry;
  struct map_table *table, *new_table;
  struct map new_map;
  INT i, hash;

  DEB(("Resize mapping heap from %d to %d.", map->size, new_size));
    
  map_allocate_heap(map->heap, &new_map, new_size);
  new_map.used = map->used;
  new_map.next = map->next;

  new_table = new_map.table;
  table = map->table;
  
  /* Keep the internal order in order to maintain high-speed lookups. */
  for(i = 0; i < map->hash_size; i++)
  {
    /* Reverse chain. */
    entry = table->hash[i];
    prev = 0;
    while(entry)
    {
      next = entry->next;
      entry->next = prev;
      prev = entry;
      entry = next;
    }
    entry = prev;
    
    /* Copy reversed chain. */
    while(entry)
    {
      hash = svalue_hash(&entry->key) % new_map.hash_size;
      
      new_entry = new_table->free_list;
      new_table->free_list = new_entry->next;

      new_entry->key = entry->key;
      new_entry->value = entry->value;
      new_entry->next = new_table->hash[hash];
      new_table->hash[hash] = new_entry;
      
      entry = entry->next;
    }
  }
  
  /* Destroy old table and establish new one. */
  new_map.heap->entries -= map->size;
  mem_free(map->table);
  *map = new_map;
}

struct map *map_allocate(struct map_heap *heap)
{
  struct map *map;

  DEB(("Allocate mapping."));
  
  map = mem_allocate(sizeof(struct map));
  map_allocate_heap(heap, map, NEW_INDEX_SIZE);

  map->next = heap->first;
  heap->first = map;
  
  return map;
}

struct svalue *map_get(struct map *map, struct svalue *key)
{
  struct map_entry *entry, **prev;
  struct map_table *table;
  INT hash;
  
  hash = svalue_hash(key) % map->hash_size;
  
  table = map->table;
  prev = &table->hash[hash];
  entry = table->hash[hash];
  
  while(entry)
  {
    if(svalue_eq(key, &entry->key))
    {
      /* Reorder heap in order to improve speed next time. */
      *prev = entry->next;
      entry->next = table->hash[hash];
      table->hash[hash] = entry;

      return &entry->value;
    }
    prev = &entry->next;
    entry = entry->next;
  }

  return 0;
}

void map_set(struct map *map, struct svalue *key, struct svalue *value)
{
  struct map_entry *entry, **prev;
  struct map_table *table;
  INT hash;

  hash = svalue_hash(key) % map->hash_size;
  
  table = map->table;
  prev = &table->hash[hash];
  entry = table->hash[hash];
  
  while(entry)
  {
    if(svalue_eq(key, &entry->key))
    {
      /* Reorder heap in order to improve speed next time. */
      *prev = entry->next;
      entry->next = table->hash[hash];
      table->hash[hash] = entry;

      entry->key = *key;
      entry->value = *value;
      return;
    }
    prev = &entry->next;
    entry = entry->next;
  }
  
  /* Otherwise, create it and add it to the pool. */
  if(table->free_list == 0)
  {
    map_resize(map, 2*map->size + 2);
    hash = svalue_hash(key) % map->hash_size;
    table = map->table;
  }
  
  entry = table->free_list;
  table->free_list = entry->next;
  entry->next = table->hash[hash];
  table->hash[hash] = entry;
  
  entry->key = *key;
  entry->value = *value;
  
  map->used++;
}

void map_remove(struct map *map, struct svalue *key)
{
  struct map_entry *entry, **prev;
  struct map_table *table;
  INT hash;
  
  hash = svalue_hash(key) % map->hash_size;
  
  table = map->table;
  prev = &table->hash[hash];
  entry = table->hash[hash];
  
  while(entry)
  {
    if(svalue_eq(key, &entry->key)) {
      *prev = entry->next;
      entry->next = table->free_list;
      table->free_list = entry;
      
      map->used--;

      check_resize(map);
      return;
    }
    prev = &entry->next;
    entry = entry->next;
  }
}

INT map_compare(struct map *a, struct map *b)
{
  if(map_equal(a, b))
    return 0;

  return a->used < b->used ? -1 : 1;
}

void map_sweep(struct map_heap *heap)
{
  struct map *map, *next, **prev;
  struct map_table *table;
  struct map_entry *entry;
  INT i;

  prev = &heap->first;
  map = heap->first;
  while(map)
  {
    next = map->next;

    if(MAP_IS_MARKED(map))
    {
      MAP_UNMARK(map);
      
      table = map->table;
      for(i = 0; i < map->hash_size; i++)
	for(entry = table->hash[i]; entry; entry = entry->next)
	{
	  UNMARK(&entry->key);
	  UNMARK(&entry->value);
	}
      
      prev = &map->next;
    } else {
      map->heap->entries -= map->size;
      
      *prev = map->next;
      mem_free(map->table);
      mem_free(map);
    }

    map = next;
  }
  
  heap->gc_entries = MAP_GC_RATIO * MAX(heap->entries, MAP_MINIMUM_GC_SIZE);
}
