/* str.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements string operations.  A neat thing with the heap
 * is that it can resize itself dynamically--both shrink and
 * grow as needed.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME "str"

#include "types.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include "err.h"
#include "mem.h"
#include "str.h"
#include "process.h"

#define STR_MINIMUM_GC_SIZE (256*1024)
#define STR_GC_RATIO                 2

struct str_entry
{
  struct str *str;
  struct str_entry *next;
};

struct str_table
{
  struct str_entry **hash;
  struct str_entry *free_list;
};

#define NEW_INDEX_SIZE  32
#define AVG_LINK_LENGTH  4
#define MIN_LINK_LENGTH  1

#define check_resize(heap)                                                 \
        do {                                                               \
          if(heap->size*2 > NEW_INDEX_SIZE &&                              \
	     heap->used < heap->hash_size * MIN_LINK_LENGTH)               \
	    str_resize(heap, heap->size/2);                                \
	} while(0)

static void str_allocate_heap(struct str_heap *heap, INT size)
{
  struct str_table *table;
  INT i;
  
  heap->size = size;
  heap->hash_size = heap->size / AVG_LINK_LENGTH + 1;
  heap->used = 0;
  
  table = mem_allocate(sizeof(struct str_table) +
		       sizeof(struct str_entry) * heap->size +
		       sizeof(struct str_entry *) * heap->hash_size);
  heap->table = table;
  
  /* Link up free list. */
  table->free_list = (struct str_entry *) &table[1];
  for(i = 0; i < heap->size-1; i++)
    table->free_list[i].next = &table->free_list[i+1];
  table->free_list[i].next = 0;

  /* Zerofy hash table. */
  table->hash = (struct str_entry **) &table->free_list[heap->size];
  for(i = 0; i < heap->hash_size; i++)
    table->hash[i] = 0;
}

void str_create(struct process *process)
{
  struct str_heap *heap;

  heap = &process->str_heap;
  
  heap->memory = 0;
  heap->gc_memory = STR_MINIMUM_GC_SIZE;
  heap->process = process;

  str_allocate_heap(heap, NEW_INDEX_SIZE);
}

void str_destroy(struct str_heap *heap)
{
  struct str_table *table;
  struct str_entry *entry;
  INT i;
  
  table = heap->table;
  
  for(i = 0; i < heap->hash_size; i++)
    for(entry = table->hash[i]; entry; entry = entry->next)
      mem_free(entry->str);

  mem_free(table);
}

static void str_resize(struct str_heap *heap, INT new_size)
{
  struct str_entry *entry, *next, *prev, *new_entry;
  struct str_table *table, *new_table;
  struct str_heap new_heap;
  INT i, hash;

  DEB(("Resize string heap from %d to %d.", heap->size, new_size));
    
  str_allocate_heap(&new_heap, new_size);
  new_heap.gc_memory = heap->gc_memory;
  new_heap.memory = heap->memory;
  new_heap.process = heap->process;
  new_heap.used = heap->used;

  new_table = new_heap.table;
  table = heap->table;
  
  /* Keep the internal order in order to maintain high-speed lookups. */
  for(i = 0; i < heap->hash_size; i++)
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
      hash = entry->str->hash % new_heap.hash_size;
      
      new_entry = new_table->free_list;
      new_table->free_list = new_entry->next;

      new_entry->str = entry->str;
      new_entry->next = new_table->hash[hash];
      new_table->hash[hash] = new_entry;
      
      entry = entry->next;
    }
  }
  
  /* Destroy old heap and establish new one. */
  mem_free(heap->table);
  *heap = new_heap;
}

/* Based on P.J. Weinberger's hash function, slightly
 * improved.  This one also computes the hash function
 * backwards in order to avoid problems with common
 * beginnings, and, takes the length of the string into
 * account.
 */

static INT str_hash_raw(char *s, INT length, INT shift)
{
  UINT h;
  char *b;

  if(!length)
    return 0;

  length <<= shift;
  
  b = s + length-1;
  h = *s++ ^ length;
  length = (length >> 1) + 1;
  length = MIN(length, 42<<shift);
  for( ; length > 1; length--) {
    h ^= (h<<4) + *s++;
    h &= 0x7fffffff;
    h ^= (h<<4) + *b--;
    h &= 0x7fffffff;
  }

  return h;
}

struct str *str_allocate_wide(struct str_heap *heap, char *s, INT length,
			      INT shift)
{
  struct str_entry *entry, **prev;
  struct str_table *table;
  INT hash, rhash, size;
  struct str *str;

#if MODULE_DEBUG  
  if(length < 0)
    err_fatal("String cannot be of negative length.");
#endif
  
  /* Check if the string already exists. */
  rhash = str_hash_raw(s, length, shift);
  hash = rhash % heap->hash_size;

  table = heap->table;
  prev = &table->hash[hash];
  entry = table->hash[hash];

  while(entry)
  {
    if(length == entry->str->length &&
       shift == entry->str->shift &&
       mem_equal(s, entry->str->s, length<<shift))
    {
      /* Reorder heap in order to improve speed next time. */
      *prev = entry->next;
      entry->next = table->hash[hash];
      table->hash[hash] = entry;
      
      return entry->str;
    }
    prev = &entry->next;
    entry = entry->next;
  }
  
  /* Otherwise, create it and add it to the pool. */
  if(table->free_list == 0)
  {
    str_resize(heap, 2*heap->size + 2);
    hash = rhash % heap->hash_size;
    table = heap->table;
  }

  if(heap->gc_memory < heap->memory)
    heap->process->gc = 1;
  
  size = sizeof(struct str) + (length << shift);
  
  heap->memory += size;
  
  str = mem_allocate(size);
  str->length = length;
  str->shift = shift;
  str->hash = rhash;
  mem_copy(str->s, s, length<<shift);
  str->s[length<<shift] = '\0';

  entry = table->free_list;
  table->free_list = entry->next;
  entry->next = table->hash[hash];
  table->hash[hash] = entry;
  
  entry->str = str;
  
  heap->used++;

  return str;
}

struct str *str_allocate_escaped(struct str_heap *heap, char *s, INT length)
     /* FIXME: Wide-strings. */
{
  char buffer[256], *p, *m = 0;
  INT i, new_length;
  struct str *str;

  if(length > 255)
    p = m = mem_allocate(length);
  else
    p = buffer;

  for(i = 0, new_length = length; i < length; i++, s++)
  {
    if(*s == '\\')
    {
      i++, s++;
      new_length--;
      switch(*s)
      {
      case '\\': *p++ = '\\'; break;
      case '\"': *p++ = '"';  break;
      case '0':  *p++ = '\0'; break;
      case 't':  *p++ = '\t'; break;
      case 'r':  *p++ = '\r'; break;
      case 'n':  *p++ = '\n'; break;
      default:
	*p++ = *s;
      }
    } else
      *p++ = *s;
  }
  
  str = str_allocate(heap, p-new_length, new_length);
  
  if(m)
    mem_free(m);
  
  return str;
}

struct str *str_allocate(struct str_heap *heap, char *s, INT length)
{
  return str_allocate_wide(heap, s, length, 0);
}

struct str *str_allocate_text(struct str_heap *heap, char *s)
{
  return str_allocate_wide(heap, s, strlen(s), 0);
}

struct str *str_allocate_raw_wide(INT length, INT shift)
{
  struct str *str;

  str = mem_allocate(sizeof(struct str) + (length<<shift));
  str->length = length;
  str->shift = shift;

  return str;
}

struct str *str_allocate_raw(INT length)
{
  return str_allocate_raw_wide(length, 0);
}

struct str *str_copy_raw(struct str_heap *heap, struct str *str)
{
  struct str *new_str;
  
  new_str = mem_allocate(sizeof(struct str) + (str->length<<str->shift));
  mem_copy(new_str, str, sizeof(struct str) + (str->length<<str->shift));

  return new_str;
}

struct str *str_append_raw(struct str *str, char *s, INT length)
{
  INT new_length = length + str->length;
    
  str = mem_reallocate(str, sizeof(struct str) + (new_length<<str->shift));
  mem_copy(str->s + (str->length<<str->shift), s, (length<<str->shift));
  str->length = new_length;
  
  return str;
}

struct str *str_commit_raw(struct str_heap *heap, struct str *str)
{
  struct str *new_str;

  /* FIXME: This can be optimized in order to avoid an allocation. */
  new_str = str_allocate_wide(heap, str->s, str->length, str->shift);
  mem_free(str);

  return new_str;
}

void str_free(struct str_heap *heap, struct str *str)
{
  struct str_entry *entry, **prev;
  struct str_table *table;
  INT hash;
  
  hash = str->hash;
  
  table = heap->table;
  prev = &table->hash[hash];
  entry = table->hash[hash];
  
  while(entry)
  {
    if(str_equal(str, entry->str)) {
      *prev = entry->next;
      entry->next = table->free_list;
      table->free_list = entry;
      
      heap->memory -= sizeof(struct str) + (str->length << str->shift);
      
      heap->used--;
      mem_free(str);
      
      check_resize(heap);
      return;
    }
    prev = &entry->next;
    entry = entry->next;
  }
  
  err_fatal("Unknown string ('%s')!", str->s);
}

void str_free_raw(struct str *str)
{
  mem_free(str);
}

INT str_compare(struct str *a, struct str *b)
{
  INT i, n;

  if(str_equal(a, b))
    return 0;

  n = MIN(a->length, b->length);
  
  if(a->shift == 0 && b->shift == 0)
  {
    /* Narrow case. */
    for(i = 0; i < n; i++)
    {
      if(a->s[i] < b->s[i])
	return -1;
      if(a->s[i] > b->s[i])
	return 1;
    }
  }
  else
  {
    /* Wide case. */
    for(i = 0; i < n; i++)
    {
      if(STR_INDEX(a, i) < STR_INDEX(b, i))
	return -1;
      if(STR_INDEX(a, i) > STR_INDEX(b, i))
	return 1;
    }
  }

  return a->length - b->length;
}

void str_sweep(struct str_heap *heap)
{
  struct str_entry *entry, **prev;
  struct str_table *table;
  INT i;

  table = heap->table;
  for(i = 0; i < heap->hash_size; i++)
  {
    entry = table->hash[i];
    prev = &table->hash[i];
    
    while(entry)
      if(STR_IS_MARKED(entry->str)) {
	STR_UNMARK(entry->str);
      
	prev = &entry->next;
	entry = entry->next;
      } else {
	*prev = entry->next;
	entry->next = table->free_list;
	table->free_list = entry;
	
	heap->memory -= sizeof(struct str) +
			(entry->str->length << entry->str->shift);
      
	heap->used--;
	mem_free(entry->str);
	
	entry = *prev;
      }
  }
  
  check_resize(heap);
  
  heap->gc_memory = STR_GC_RATIO * MAX(heap->memory, STR_MINIMUM_GC_SIZE);
}
