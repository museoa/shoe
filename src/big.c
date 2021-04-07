/* big.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements arbitrary precision integer operations.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME "big"

#include "types.h"

#include "err.h"
#include "mem.h"
#include "big.h"
#include "str.h"
#include "process.h"

#ifdef USE_BIG_INTEGERS

#define BIG_MINIMUM_GC_SIZE 64
#define BIG_GC_RATIO         2

#define BIG_ALLOCATE(heap, b)                                              \
        do {                                                               \
          if(heap->gc_size < heap->size)                                   \
            heap->process->gc = 1;                                         \
                                                                           \
          b = mem_allocate(sizeof(struct big));                            \
          BIG_UNMARK(b);                                                   \
                                                                           \
          heap->size++;                                                    \
          b->next = heap->first;                                           \
          heap->first = b;                                                 \
        } while(0)

static void *big_mem_allocate(size_t amount)
{
  return mem_allocate(amount);
}

static void *big_mem_reallocate(void *ptr, size_t old_amount, size_t amount)
{
  return mem_reallocate(ptr, amount);
}

static void big_mem_free(void *ptr, size_t amount)
{
  mem_free(ptr);
}

void big_init(void)
{
  mp_set_memory_functions(big_mem_allocate, big_mem_reallocate, big_mem_free);
}

void big_create(struct process *process)
{
  struct big_heap *heap;

  heap = &process->big_heap;
  
  heap->size = 0;
  heap->gc_size = BIG_MINIMUM_GC_SIZE;
  heap->process = process;
  
  heap->first = 0;
}

void big_destroy(struct big_heap *heap)
{
  struct big *big, *next;

  for(big = heap->first; big; big = next)
  {
    next = big->next;
    mpz_clear(big->u.integer);
    mem_free(big);
  }
}

struct big *big_allocate_integer(struct big_heap *heap, INT z)
{
  struct big *big;

  BIG_ALLOCATE(heap, big);
  
  mpz_init_set_si(big->u.integer, z);
  
  return big;
}

struct big *big_copy_integer(struct big_heap *heap, struct big *big)
{
  struct big *big2;
  
  BIG_ALLOCATE(heap, big2);
  
  mpz_init_set(big2->u.integer, big->u.integer);
  
  return big2;
}

struct big *big_allocate_integer_text(struct big_heap *heap, char *s,INT base)
{
  struct big *big;
  
  BIG_ALLOCATE(heap, big);
  
  if(mpz_init_set_str(big->u.integer, s, base) == -1)
    return 0;
  
  return big;
}

struct str *big_integer_to_string(struct str_heap *str_heap,
				  struct big *big, INT base)
{
  struct str *str;
  INT length;

  length = mpz_sizeinbase(big->u.integer, base) +
	   (mpz_sgn(big->u.integer) < 0 ? 1 : 0);
  str = str_allocate_raw(length);
  
  /* It writes an extra \0, but that's OK. */
  mpz_get_str(str->s, base, big->u.integer);

  if(str->s[str->length])
    err_fatal("big_integer_to_string: Buffer overflow.");
  
  return str_commit_raw(str_heap, str);
}

struct big *big_float_to_integer(struct big_heap *heap, REAL f)
{
  struct big *big;
  
  BIG_ALLOCATE(heap, big);

  mpz_init_set_d(big->u.integer, (double)f);
    
  return big;
}

REAL big_integer_to_float(struct big *big)
{
  REAL f = 0.0;
  char *s, *t;
  
  t = s = mpz_get_str(0, 10, big->u.integer);
  if(*s == '-')
    s++;
  while(*s)
    f = f*10.0 + (REAL)(*s++ - '0');
  if(*t == '-')
    f = -f;
  mem_free(t);

  return f;
}

INT big_equal_integer(struct big *a, struct big *b)
{
  return mpz_cmp(a->u.integer, b->u.integer) == 0;
}

void big_sweep(struct big_heap *heap)
{
  struct big *big, *next, **prev;

  for(big = heap->first, prev = &heap->first; big; big = next)
  {
    next = big->next;
    
    if(BIG_IS_MARKED(big))
    {
      *prev = big;
      prev = &big->next;
      BIG_UNMARK(big);
    }
    else
    {
      heap->size--;
      mpz_clear(big->u.integer);
      mem_free(big);
    }
  }
  
  heap->gc_size = BIG_GC_RATIO * MAX(heap->size, BIG_MINIMUM_GC_SIZE);
  
  *prev = 0;
}

#endif /* USE_BIG_INTEGERS */
