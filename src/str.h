/* str.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements string operations.
 */

#ifndef __STR_H__
#define __STR_H__

#include "types.h"

#define STR0(str) ((WCHAR0*)(str)->s)
#define STR1(str) ((WCHAR1*)(str)->s)
#define STR2(str) ((WCHAR2*)(str)->s)

#define STR_INDEX(str, i)                                                  \
        ((str)->shift == 0 ? ((WCHAR0*)(str)->s)[i] :                      \
	 ((str)->shift == 1 ? ((WCHAR1*)(str)->s)[i] :                     \
	  ((WCHAR2*)(str)->s)[i]))

#define STR_INDEX_SET(str, i, c)                                           \
        ((str)->shift == 0 ? (((WCHAR0*)(str)->s)[i] = (c)):               \
	 ((str)->shift == 1 ? (((WCHAR1*)(str)->s)[i] = (c)):              \
	  (((WCHAR2*)(str)->s)[i] = (c))))

#define STR_MARK(str)                                                      \
        do {                                                               \
          if((str)->length >= 0)                                           \
	    (str)->length = -(str)->length - 1;                            \
        } while(0)
     
#define STR_UNMARK(str)                                                    \
	((str)->length = -(str)->length - 1)

#define STR_IS_MARKED(str)                                                 \
        ((str)->length < 0)

struct str
{
  INT length;
  INT shift;
  INT hash;
  
  char s[1];
};

struct str_heap
{
  INT size;
  INT hash_size;

  INT used;
  
  INT gc_memory, memory;

  struct process *process;
  
  struct str_table *table;
};

void str_create(struct process *process);
void str_destroy(struct str_heap *heap);

#define str_debug_objects(heap) ((heap)->size)
#define str_debug_memory(heap) ((heap)->memory)

struct str *str_allocate_wide(struct str_heap *heap, char *s, INT length,
			      INT shift);
struct str *str_allocate(struct str_heap *heap, char *s, INT length);
struct str *str_allocate_escaped(struct str_heap *heap, char *s, INT length);
struct str *str_allocate_text(struct str_heap *heap, char *s);
struct str *str_allocate_raw_wide(INT length, INT shift);
struct str *str_allocate_raw(INT length);
struct str *str_copy_raw(struct str_heap *heap, struct str *str);
struct str *str_append_raw(struct str *str, char *s, INT length);
struct str *str_commit_raw(struct str_heap *heap, struct str *str);

void str_free(struct str_heap *heap, struct str *str);
void str_free_raw(struct str *str);

INT str_compare(struct str *a, struct str *b);

#define str_equal(a, b) ((a) == (b))
#define str_hash(str)   ((str)->hash)

void str_sweep(struct str_heap *heap);

#endif /* __STR_H__ */
