/* big.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements arbitrary precision integer operations.
 */

#ifndef __BIG_H__
#define __BIG_H__

#if defined(HAVE_GMP2_GMP_H) && defined(HAVE_LIBGMP2)
#  define USE_BIG_INTEGERS
#  define USE_GMP2
#else
#  if defined(HAVE_GMP_H) && defined(HAVE_LIBGMP)
#    define USE_BIG_INTEGERS
#    define USE_GMP1
#  endif /* HAVE_GMP_H && HAVE_LIBGMP */
#endif /* HAVE_GMP2_GMP_H && HAVE_LIBGMP2 */

#ifdef USE_BIG_INTEGERS

#ifdef USE_GMP2
#  include <gmp2/gmp.h>
#else
#  ifdef USE_GMP1
#    include <gmp.h>
#  endif /* USE_GMP1 */
#endif /* USE_GMP2 */

#include "types.h"
#include "str.h"

#define BIG_MARK(big)      ((big)->mark = 1)
#define BIG_UNMARK(big)    ((big)->mark = 0)
#define BIG_IS_MARKED(big) ((big)->mark)

struct big
{
  BYTE mark;
  struct big *next;

  union
  {
    mpz_t integer;
  } u;
};

struct big_heap
{
  INT gc_size, size;

  struct process *process;
  
  struct big *first;
};

void big_init(void);

void big_create(struct process *process);
void big_destroy(struct big_heap *heap);

#define big_debug_objects(heap) ((heap)->size)

struct big *big_allocate_integer(struct big_heap *heap, INT z);
struct big *big_allocate_integer_text(struct big_heap *heap,char *s,INT base);

struct big *big_copy_integer(struct big_heap *heap, struct big *big);

struct str *big_integer_to_string(struct str_heap *str_heap,
				  struct big *big, INT base);
REAL big_integer_to_float(struct big *big);
struct big *big_float_to_integer(struct big_heap *heap, REAL f);

INT big_equal_integer(struct big *a, struct big *b);

void big_sweep(struct big_heap *heap);

#endif /* USE_BIG_INTEGERS */

#endif /* __BIG_H__ */
