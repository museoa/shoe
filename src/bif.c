/* bif.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements the built-in functions (BIF:s) environment.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME "bif"

#include "types.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif /* HAVE_STDIO_H */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include "args.h"
#include "bif.h"
#include "deb.h"
#include "err.h"
#include "mat.h"
#include "mem.h"
#include "str.h"
#include "garb.h"
#include "lexer.h"
#include "process.h"
#include "version.h"
#include "svalue.h"
#include "invocation.h"

#define CONS(r, a, b)                                                      \
        do {                                                               \
          (r)->u.pair = pair_cons(process, (a), (b));                      \
          (r)->type = T_PAIR;                                              \
	} while(0)

#define LIST(r, a)                                                         \
        do {                                                               \
          (r)->u.pair = pair_list(process, (a));                           \
          (r)->type = T_PAIR;                                              \
	} while(0)

/* MISCELLANEOUS *************************************************************/

BIF_DECLARE(bif_invocation_arguments)
{
  struct vec *vec;
  INT i;

  ARGS_GET((process, "invocation-arguments", args, ""));
  
  vec = vec_allocate(&process->vec_heap, process->invocation->argc);

  for(i = 0; i < vec->length; i++)
  {
    vec->v[i].type = T_STRING;
    vec->v[i].u.str =
      str_allocate_text(&process->str_heap, process->invocation->argv[i]);
  }
  
  BIF_RESULT_VECTOR(vec);
}

BIF_DECLARE(bif_shoe_version)
{
  char buffer[128];
  struct str *str;

  ARGS_GET((process, "shoe-version", args, ""));

  sprintf(buffer, "Shoe v%d.%d release %d",
	  SHOE_MAJOR_VERSION,
	  SHOE_MINOR_VERSION,
	  SHOE_BUILD_VERSION);

  str = str_allocate_text(&process->str_heap, buffer);
  
  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_gc)
{
  ARGS_GET((process, "gc", args, ""));

  BIF_RESULT_UNDEFINED();

  garb(process);
}

/*
 * Object predicates.
 */

#define BIF_PREDICATE(bif_name, name, PREDICATE)                          \
        BIF_DECLARE(bif_name)                                             \
        {                                                                 \
          struct svalue *val;                                             \
                                                                          \
          ARGS_GET((process, name, args, "%*", &val));                    \
                                                                          \
          if(PREDICATE(*val))                                             \
            BIF_RESULT_TRUE();                                            \
          else                                                            \
            BIF_RESULT_FALSE();                                           \
        }

BIF_PREDICATE(bif_booleanp,       "boolean?",       IS_BOOLEAN)
BIF_PREDICATE(bif_charp,          "char?",          IS_CHAR)
BIF_PREDICATE(bif_nullp,          "null?",          IS_NULL)
BIF_PREDICATE(bif_exactp,         "exact?",         IS_EXACT)
BIF_PREDICATE(bif_inexactp,       "inexact?",       IS_INEXACT)
BIF_PREDICATE(bif_numberp,        "number?",        IS_NUMBER)
BIF_PREDICATE(bif_integerp,       "integer?",       IS_INTEGER)
BIF_PREDICATE(bif_small_integerp, "small-integer?", IS_SMALL_INTEGER)
BIF_PREDICATE(bif_big_integerp,   "big-integer?",   IS_BIG_INTEGER)
BIF_PREDICATE(bif_realp,          "real?",          IS_REAL)
BIF_PREDICATE(bif_pairp,          "pair?",          IS_PAIR)
BIF_PREDICATE(bif_portp,          "port?",          IS_PORT)
BIF_PREDICATE(bif_procedurep,     "procedure?",     IS_PROCEDURE)
BIF_PREDICATE(bif_stringp,        "string?",        IS_STRING)
BIF_PREDICATE(bif_symbolp,        "symbol?",        IS_SYMBOL)
BIF_PREDICATE(bif_mappingp,       "mapping?",       IS_MAPPING)
BIF_PREDICATE(bif_vectorp,        "vector?",        IS_VECTOR)

/*
 * Equivalence predicates.
 */
     
BIF_DECLARE(bif_eq)
{
  struct svalue *a, *b;

  ARGS_GET((process, "char->integer", args, "%*%*", &a, &b));

  if(svalue_eq(a, b))
    BIF_RESULT_TRUE();
  else
    BIF_RESULT_FALSE();
}
     
/*
 * Characters.
 */

BIF_DECLARE(bif_char_to_integer)
{
  INT c;

  ARGS_GET((process, "char->integer", args, "%c", &c));
  
  BIF_RESULT_SMALL_INTEGER(c);
}     

#define CHAR_COMPARISON(bif_name, function_name, PREDICATE)                \
        BIF_DECLARE(bif_name)                                              \
        {                                                                  \
          INT n, next, prev = 0;  /* Set to zero because Gcc is stupid. */ \
          struct pair *p;                                                  \
                                                                           \
	  BIF_RESULT_TRUE();                                               \
                                                                           \
	  if(IS_NOT_PAIR(*args))                                           \
	    return;                                                        \
	  p = args->u.pair;                                                \
	                                                                   \
	  for(n = 1; ; n++)                                                \
	  {                                                                \
	    if(IS_NOT_CHAR(CAR(p)))                                        \
              err_fatal(function_name": Argument %d not char.");           \
	                                                                   \
            next = CAR(p).u.integer;                                       \
	                                                                   \
            if(2 <= n && !(prev PREDICATE next))                           \
            {                                                              \
	      BIF_RESULT_FALSE();                                          \
	      return;                                                      \
	    }                                                              \
	                                                                   \
	    prev = next;                                                   \
	                                                                   \
	    if(IS_NOT_PAIR(CDR(p)))                                        \
	      return;                                                      \
	    p = CDR(p).u.pair;                                             \
	  }                                                                \
	}

CHAR_COMPARISON(bif_char_less_equal,    "char<=?", <=)
CHAR_COMPARISON(bif_char_less_than,     "char<?",  <)
CHAR_COMPARISON(bif_char_equal,         "char=?",  ==)
CHAR_COMPARISON(bif_char_greater_equal, "char>=?", >=)
CHAR_COMPARISON(bif_char_greater_than,  "char>?",  >)

/*
 * List.
 */

BIF_DECLARE(bif_append)
{
  struct pair *p, *s, *d = 0;
  INT n;

  BIF_RESULT_NIL();
  
  if(IS_PAIR(*args))
  {
    p = args->u.pair;

    for(n = 1; ; n++)
    {
      if(IS_NOT_PAIR(CDR(p)))
      {
	if(d)
	  CDR(d) = CAR(p);
	else
	  *result = CAR(p);
	return;
      }
      
      if(IS_NOT_NIL(CAR(p)))
      {
	if(IS_NOT_PAIR(CAR(p)))
	  err_fatal("append: Argument %d not list.", n);

	if(d)
	{
	  s = CAR(p).u.pair;
	copy:
	  for(;;)
	  {
	    LIST(&CDR(d), &CAR(s));
	    d = CDR(d).u.pair;
	    
	    if(IS_NOT_PAIR(CDR(s)))
	      break;
	    s = CDR(s).u.pair;
	  }
	}
	else
	{
	  s = CAR(p).u.pair;
	  
	  LIST(result, &CAR(s));
	  d = result->u.pair;
	  
	  if(IS_PAIR(CDR(s)))
	  {
	    s = CDR(s).u.pair;
	    goto copy;
	  }
	}
	
	if(IS_NOT_NIL(CDR(s)))
	  err_fatal("append: Argument %d not proper list.", n);
      }
      
      p = CDR(p).u.pair;
    }
  }
}

BIF_DECLARE(bif_reverse)
{
  struct svalue *l;

  ARGS_GET((process, "reverse", args, "%*", &l));
  
  for(BIF_RESULT_NIL(); IS_PAIR(*l); l = &CDR(l->u.pair))
    CONS(result, &CAR(l->u.pair), result);
  
  if(IS_NOT_NIL(*l))
    err_fatal("reverse: Argument 1 not a proper list.");
}

BIF_DECLARE(bif_length)
{
  struct svalue *l;
  INT length;

  ARGS_GET((process, "length", args, "%*", &l));
  
  for(length = 0; IS_PAIR(*l); l = &CDR(l->u.pair))
    length++;
  
  if(IS_NOT_NIL(*l))
    err_fatal("length: Argument 1 not a proper list.");
  
  BIF_RESULT_SMALL_INTEGER(length);
}

BIF_DECLARE(bif_list_tail)
{
  struct svalue *l;
  INT n, k, ref;

  ARGS_GET((process, "list-tail", args, "%*%i", &l, &ref));
  
  for(n = 0, k = ref; IS_PAIR(*l) && k > 0; l = &CDR(l->u.pair))
    n++, k--;
  
  if(k)
  {
    if(k < 0)
      err_fatal("list-tail: Index %d out of range.", ref);
    else if(IS_NOT_NIL(*l))
      err_fatal("list-tail: Argument 1 not a proper list.");
    else
      err_fatal("list-tail: Index %d out of range 0 - %d.", ref, n);
  }
  
  *result = *l;
}

#define LIST_FIRST_REST(bif_name, name, list_f)                            \
        BIF_DECLARE(bif_name)                                              \
        {                                                                  \
          struct svalue *l, *r;                                            \
          INT n, k;                                                        \
                                                                           \
          ARGS_GET((process, name, args, "%*", &l));                       \
                                                                           \
          if(IS_NIL(*l))                                                   \
          {                                                                \
            BIF_RESULT_NIL();                                              \
            return;                                                        \
          }                                                                \
                                                                           \
          if(IS_NOT_PAIR(*l))                                              \
            err_fatal(name": Argument 1 is not a list.");                  \
                                                                           \
          if(IS_NOT_PAIR(CAR(l->u.pair)))                                  \
            err_fatal(name": Argument 1 is not a list of lists.");         \
                                                                           \
          LIST(result, &list_f(CAR(l->u.pair).u.pair));                    \
                                                                           \
          r = result;                                                      \
          for(l = &CDR(l->u.pair); IS_PAIR(*l); l = &CDR(l->u.pair))       \
            if(IS_PAIR(CAR(l->u.pair)))                                    \
            {                                                              \
              LIST(&CDR(r->u.pair), &list_f(CAR(l->u.pair).u.pair));       \
              r = &CDR(r->u.pair);                                         \
            }                                                              \
            else                                                           \
              err_fatal(name": Argument 1 is not a list of pairs.");       \
                                                                           \
          if(IS_NOT_NIL(*l))                                               \
            err_fatal(name": Argument 1 is not a list of pairs.");         \
	}

LIST_FIRST_REST(bif_list_first, "list-first", CAR)
LIST_FIRST_REST(bif_list_rest,  "list-rest",  CDR)

BIF_DECLARE(bif_list_copy)
{
  struct svalue *l;
  struct pair *p;

  ARGS_GET((process, "list-copy", args, "%*", &l));

  BIF_RESULT_NIL();
  
  if(IS_PAIR(*l))
  {
    LIST(result, &CAR(l->u.pair));

    if(IS_PAIR(CDR(l->u.pair)))
    {
      p = result->u.pair;
      l = &CDR(l->u.pair);
      for(; IS_PAIR(CDR(l->u.pair)); l = &CDR(l->u.pair), p = CDR(p).u.pair)
	LIST(&CDR(p), &CAR(l->u.pair));
      
      LIST(&CDR(p), &CAR(l->u.pair));
    }
    
    if(IS_NOT_NIL(CDR(l->u.pair)))
      err_fatal("list-copy: Argument 1 is not a proper list.");
  } else if(IS_NOT_NIL(*l))
    err_fatal("list-copy: Argument 1 not a list.");
}

BIF_DECLARE(bif_list_ref)
{
  struct svalue *l;
  INT n, k, ref;

  ARGS_GET((process, "list-ref", args, "%*%i", &l, &ref));
  
  for(n = 0, k = ref; IS_PAIR(*l) && k > 0; l = &CDR(l->u.pair))
    n++, k--;
  
  if(k || IS_NOT_PAIR(*l))
  {
    if(k < 0)
      err_fatal("list-ref: Index %d out of range.", ref);
    else if(IS_NOT_NIL(*l))
      err_fatal("list-ref: Argument 1 not a proper list.");
    else if(!n)
      err_fatal("list-ref: Index %d out of empty range.", ref);
    else
      err_fatal("list-ref: Index %d out of range 0 - %d.", ref, n-1);
  }
  
  *result = CAR(l->u.pair);
}

BIF_DECLARE(bif_listp)
{
  struct svalue *l;

  ARGS_GET((process, "list?", args, "%*", &l));
  
  for(; IS_PAIR(*l); l = &CDR(l->u.pair))
    ;
  
  if(IS_NIL(*l))
    BIF_RESULT_TRUE();
  else
    BIF_RESULT_FALSE();
}

BIF_DECLARE(bif_memq)
{
  struct svalue *key, *l;

  ARGS_GET((process, "memq", args, "%*%*", &key, &l));
  
  for(; IS_PAIR(*l); l = &CDR(l->u.pair))
    if(svalue_eq(key, &CAR(l->u.pair)))
    {
      *result = *l;
      return;
    }
  
  if(IS_NOT_NIL(*l))
    err_fatal("memq: Argument 1 is not a proper list.");

  BIF_RESULT_FALSE();
}

BIF_DECLARE(bif_cons)
     /* Note: This function is especially optimised to save one pair. */
{
  struct pair *p;

  if(IS_NOT_PAIR(*args))
    err_fatal("cons: Wrong number of arguments.");
  p = args->u.pair;
  
  result->u.pair = p;
  
  if(IS_NOT_PAIR(CDR(p)))
    err_fatal("cons: Wrong number of arguments.");
  p = CDR(p).u.pair;

  CDR(result->u.pair) = CAR(p);
  result->type = T_PAIR;
  
  if(IS_NOT_NIL(CDR(p)))
    err_fatal("cons: Wrong number of arguments.");
}

BIF_DECLARE(bif_car)
{
  struct pair *p;

  ARGS_GET((process, "car", args, "%p", &p));

  *result = CAR(p);
}

BIF_DECLARE(bif_cdr)
{
  struct pair *p;

  ARGS_GET((process, "cdr", args, "%p", &p));
  
  *result = CDR(p);
}

BIF_DECLARE(bif_list)
{
  *result = *args;
}

BIF_DECLARE(bif_list_to_mapping)
{
  struct svalue *l;
  struct map *map;
  struct pair *p;
  INT i;

  ARGS_GET((process, "list->mapping", args, "%*", &l));

  map = map_allocate(&process->map_heap);

  BIF_RESULT_MAPPING(map);
  
  if(IS_PAIR(*l))
  {
    for(i = 0; ; l = &CDR(l->u.pair), i++)
    {
      if(IS_NOT_PAIR(CAR(l->u.pair)))
	err_fatal("list->mapping: Element with "
		  "index %d does not have a value.", i);
      
      p = CAR(l->u.pair).u.pair;
      
      if(IS_NOT_PAIR(CDR(p)))
	err_fatal("list->mapping: Element with "
		  "index %d does not have a value.", i);
      
      if(IS_NOT_NIL(CDR(CDR(p).u.pair)))
	err_fatal("list->mapping: Element with "
		  "index %d has trailing garbage.", i);

      map_set(map, &CAR(p), &CAR(CDR(p).u.pair));

      if(IS_NOT_PAIR(CDR(l->u.pair)))
	break;
    }
    
    if(IS_NOT_NIL(CDR(l->u.pair)))
      err_fatal("list->mapping: Argument 1 is not a proper list.");
  }
  else if(IS_NOT_NIL(*l))
    err_fatal("list->mapping: Argument 1 is not a list.");
}

BIF_DECLARE(bif_list_to_vector)
{
  struct svalue *l, *t;
  INT i, length = 0;
  struct vec *vec;

  ARGS_GET((process, "list->vector", args, "%*", &l));

  if(IS_PAIR(*l))
  {
    for(length = 1, t = l; IS_PAIR(CDR(t->u.pair)); length++)
      t = &CDR(t->u.pair);
    
    if(IS_NOT_NIL(CDR(t->u.pair)))
      err_fatal("list->vector: Argument 1 is not a proper list.");
  }
  else if(IS_NOT_NIL(*l))
    err_fatal("list->vector: Argument 1 not a list.");
  
  vec = vec_allocate(&process->vec_heap, length);
  
  for(i = 0; i < length; i++)
  {
    vec->v[i] = CAR(l->u.pair);
    l = &CDR(l->u.pair);
  }
  
  BIF_RESULT_VECTOR(vec);
}

BIF_DECLARE(bif_list_to_string)
{
  struct svalue *l, *t;
  INT i, length = 0;
  struct str *str;

  ARGS_GET((process, "list->string", args, "%*", &l));

  if(IS_PAIR(*l))
  {
    for(length = 1, t = l; IS_PAIR(CDR(t->u.pair)); length++)
    {
      if(IS_NOT_CHAR(CAR(t->u.pair)) && IS_NOT_SMALL_INTEGER(CAR(t->u.pair)))
	err_fatal("list->string: Argument %d is not a character "
		  "or an integer.", length);
      t = &CDR(t->u.pair);
    }

    if(IS_NOT_CHAR(CAR(t->u.pair)) && IS_NOT_SMALL_INTEGER(CAR(t->u.pair)))
      err_fatal("list->string: Argument %d is not a character "
		"or an integer.", length);
    
    if(IS_NOT_NIL(CDR(t->u.pair)))
      err_fatal("list->string: Argument 1 is not a proper list.");
    
    if(IS_NOT_NIL(CDR(t->u.pair)))
      err_fatal("list->string: Argument 1 is not a proper list.");
  }
  else if(IS_NOT_NIL(*l))
    err_fatal("list->string: Argument 1 not a list.");
  
  str = str_allocate_raw(length);
  
  for(i = 0; i < length; i++)
  {
    str->s[i] = (UBYTE)CAR(l->u.pair).u.integer;
    l = &CDR(l->u.pair);
  }

  str = str_commit_raw(&process->str_heap, str);
  
  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_set_car)
{
  struct svalue *value;
  struct pair *p;

  ARGS_GET((process, "set-car!", args, "%p%*", &p, &value));
  
  CAR(p) = *value;
  
  BIF_RESULT_UNDEFINED();
}

BIF_DECLARE(bif_set_cdr)
{
  struct svalue *value;
  struct pair *p;

  ARGS_GET((process, "set-cdr!", args, "%p%*", &p, &value));
  
  CDR(p) = *value;

  BIF_RESULT_UNDEFINED();
}

/*
 * Characters.
 */

BIF_DECLARE(bif_integer_to_char)
{
  INT i;

  ARGS_GET((process, "integer->char", args, "%i", &i));
  
  BIF_RESULT_CHAR(i);
}     

/*
 * String operations.
 */

BIF_DECLARE(bif_make_string)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  struct pair *p;
  INT c, i, k;

  if(IS_NOT_PAIR(*args))
    err_fatal("make-string: Wrong number of arguments.");
  p = args->u.pair;
  
  if(IS_NOT_SMALL_INTEGER(CAR(p)))
    err_fatal("make-string: Argument 1 not integer.");
  k = CAR(p).u.integer;
  if(k < 0)
    err_fatal("make-string: Cannot make a string with negative length.");
  
  if(IS_PAIR(CDR(p)))
  {
    p = CDR(p).u.pair;
    if(IS_NOT_CHAR(CAR(p)))
      err_fatal("make-string: Argument 2 not character.");
    c = CAR(p).u.integer;
  } else
    c = ' ';

  if(IS_NOT_NIL(CDR(p)))
    err_fatal("make-string: Wrong number of arguments.");
  
  str = str_allocate_raw(k);

  for(i = 0; i < k; i++)
    str->s[i] = c;
  
  BIF_RESULT_STRING(str_commit_raw(&process->str_heap, str));
}

BIF_DECLARE(bif_string)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  struct pair *p;
  INT i, n = 0;
  
  if(IS_PAIR(*args))
  {
    p = args->u.pair;
    
    for(n = 1; ; n++)
    {
      if(IS_NOT_CHAR(CAR(p)))
	err_fatal("string: Argument %d not character.", n);
  
      if(IS_NOT_PAIR(CDR(p)))
	break;
      p = CDR(p).u.pair;
    }
  }
  
  str = str_allocate_raw(n);

  if(n > 0)
  {
    p = args->u.pair;
    for(i = 0; i < n; i++)
    {
      str->s[i] = CAR(p).u.integer;
      p = CDR(p).u.pair;
    }
  }

  BIF_RESULT_STRING(str_commit_raw(&process->str_heap, str));
}

BIF_DECLARE(bif_string_to_list)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  struct svalue c;
  
  ARGS_GET((process, "string->list", args, "%s", &str));

  c.type = T_CHAR;
  
  if(str->length)
  {
    INT i = str->length-1;
    
    c.u.integer = str->s[i--];
    LIST(result, &c);
    
    for( ; 0 <= i; i--)
    {
      c.u.integer = str->s[i];
      CONS(result, &c, result);
    }
  } else
    BIF_RESULT_NIL();
}

BIF_DECLARE(bif_string_to_symbol)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  
  ARGS_GET((process, "string->symbol", args, "%s", &str));
  
  BIF_RESULT_SYMBOL(str);
}

BIF_DECLARE(bif_unquote_string)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  
  ARGS_GET((process, "unquote-string", args, "%s", &str));

  str = str_allocate_escaped(&process->str_heap, str->s, str->length);
  
  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_string_append)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  struct pair *p;
  INT n;
    
  if(IS_NOT_PAIR(*args))
  {
    BIF_RESULT_STRING(str_allocate(&process->str_heap, "", 0));
    return;
  }
  p = args->u.pair;
  
  if(IS_STRING(CAR(p)))
  {
    if(IS_NOT_PAIR(CDR(p)))
    {
      BIF_RESULT_STRING(CAR(p).u.str);
      return;
    }
    
    str = str_copy_raw(&process->str_heap, CAR(p).u.str);
    p = CDR(p).u.pair;
    
    for(n = 2; ; n++)
    {
      if(IS_NOT_STRING(CAR(p)))
	err_fatal("string-append: Argument %d not string.", n);
      
      str = str_append_raw(str, CAR(p).u.str->s, CAR(p).u.str->length);
      
      if(IS_NOT_PAIR(CDR(p)))
	break;
      p = CDR(p).u.pair;
    }
    
    BIF_RESULT_STRING(str_commit_raw(&process->str_heap, str));
  }
  else if(IS_NIL(CAR(p)))
  {
    BIF_RESULT_STRING(str_allocate(&process->str_heap, "", 0));
  }
  else if(IS_PAIR(CAR(p)))
  {
    struct svalue *l;
    struct str *str2;

    for(n = 0, l = &CAR(p); IS_PAIR(*l); l = &CDR(l->u.pair))
      if(IS_STRING(CAR(l->u.pair)))
	n += CAR(l->u.pair).u.str->length;
      else
	err_fatal("string-append: Argument 1 is not a list of strings.");
    
    str = str_allocate_raw(n);
    
    for(n = 0, l = &CAR(p); IS_PAIR(*l); l = &CDR(l->u.pair))
    {
      str2 = CAR(l->u.pair).u.str;
      mem_copy(&str->s[n], str2->s, str2->length);
      n += str2->length;
    }
    
    BIF_RESULT_STRING(str_commit_raw(&process->str_heap, str));
  }
  else
    err_fatal("string-append: Argument 1 not string.");
}

BIF_DECLARE(bif_string_copy)
{
  struct str *str;

  ARGS_GET((process, "string-copy", args, "%s", &str));
  
  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_string_fill)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  INT c, i;

  ARGS_GET((process, "string-fill", args, "%s%c", &str, &c));
  
  str = str_copy_raw(&process->str_heap, str);
  for(i = 0; i < str->length; i++)
    str->s[i] = c;
  
  BIF_RESULT_STRING(str_commit_raw(&process->str_heap, str));
}

BIF_DECLARE(bif_string_length)
{
  struct str *str;

  ARGS_GET((process, "string-length", args, "%w", &str));

  BIF_RESULT_SMALL_INTEGER(str->length);
}

BIF_DECLARE(bif_string_ref)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  INT ref;

  ARGS_GET((process, "string-ref", args, "%s%i", &str, &ref));
  
  if(ref < 0 || str->length <= ref)
  {
    if(str->length)
      err_fatal("string-ref: Index out of range 0 - %d.", str->length-1);
    else
      err_fatal("string-ref: Index out of empty range.");
  }
  
  BIF_RESULT_CHAR((UBYTE)str->s[ref]);
}

BIF_DECLARE(bif_string_set)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  INT ref, c;

  ARGS_GET((process, "string-set", args, "%s%i%c", &str, &ref, &c));
  
  if(ref < 0 || str->length <= ref)
    err_fatal("string-set: Index out of range 0 - %d.", str->length-1);

  str = str_copy_raw(&process->str_heap, str);
  str->s[ref] = c;
  
  BIF_RESULT_STRING(str_commit_raw(&process->str_heap, str));
}

     /* FIXME: Wide-strings. */
#define STRING_COMPARISON(bif_name, function_name, PREDICATE)              \
        BIF_DECLARE(bif_name)                                              \
        {                                                                  \
          struct str *prev = 0, *next = 0;                                 \
          struct pair *p;                                                  \
          INT n;                                                           \
                                                                           \
	  BIF_RESULT_TRUE();                                               \
                                                                           \
	  if(IS_NOT_PAIR(*args))                                           \
	    return;                                                        \
	  p = args->u.pair;                                                \
	                                                                   \
	  for(n = 1; ; n++)                                                \
	  {                                                                \
	    if(IS_NOT_STRING(CAR(p)))                                      \
              err_fatal(function_name": Argument %d not string.");         \
	                                                                   \
            next = CAR(p).u.str;                                           \
	                                                                   \
            if(2 <= n && !(str_compare(prev, next) PREDICATE 0))           \
            {                                                              \
	      BIF_RESULT_FALSE();                                          \
	      return;                                                      \
	    }                                                              \
	                                                                   \
	    prev = next;                                                   \
	                                                                   \
	    if(IS_NOT_PAIR(CDR(p)))                                        \
	      return;                                                      \
	    p = CDR(p).u.pair;                                             \
	  }                                                                \
	}

STRING_COMPARISON(bif_string_less_equal,    "string<=?", <=)
STRING_COMPARISON(bif_string_less_than,     "string<?",  <)
STRING_COMPARISON(bif_string_equal,         "string=?",  ==)
STRING_COMPARISON(bif_string_greater_equal, "string>=?", >=)
STRING_COMPARISON(bif_string_greater_than,  "string>?",  >)

BIF_DECLARE(bif_substring)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  INT s_i, e_i;

  ARGS_GET((process, "substring", args, "%s%i%i", &str, &s_i, &e_i));
  
  if(s_i < 0 || str->length < s_i)
    err_fatal("substring: Start index %d out of range 0 - %d.",
	      s_i, str->length);
  
  if(e_i < 0 || str->length < e_i)
    err_fatal("substring: End index %d out of range 0 - %d.",
	      e_i, str->length);
  
  if(e_i < s_i)
    err_fatal("substring: End index %d ends before start index %d.",
	      e_i, s_i);

  BIF_RESULT_STRING(str_allocate(&process->str_heap, &str->s[s_i], e_i - s_i));
}

/* Wide-string and unicode related functions.  Most of
   this is originally written for Pike © Fredrik Hübinette. */

BIF_DECLARE(bif_string_to_unicode)
{
  struct str *str, *raw;
  INT i;

  ARGS_GET((process, "string->unicode", args, "%w", &str));

  switch(str->shift)
  {
  case 0:
    /* 8-bit characters. */
    raw = str_allocate_raw_wide(str->length, 1);

    /* This is done automatically, but... */
    mem_zero(raw->s, str->length<<1);   /* Clear the upper (and lower) byte. */
    
    for(i = str->length; i--;)
      raw->s[i*2 + 1] = str->s[i];
    
    str = str_commit_raw(&process->str_heap, str);
    break;
    
  case 1:
    /* 16-bit characters. */
    
    /* FIXME: Should we check for 0xfffe & 0xffff here too? */
    raw = str_allocate_raw_wide(str->length, 1);
    
#if (SHOE_BYTEORDER == 4321)
    
    /* Big endian. */
    mem_copy(raw->s, str->s, str->length<<1);
    
#else
    
    /* Little endian. */
    {
      WCHAR1 *str1 = STR1(str);
      
      for(i = str->length; i--;)
      {
	UINT32 c = str1[i];
	raw->s[i * 2 + 1] = c & 0xff;
	raw->s[i * 2] = c >> 8;
      }
    }
    
#endif   /* SHOE_BYTEORDER == 4321 */
    
    str = str_commit_raw(&process->str_heap, str);
    break;
    
  case 2:
    /* 32-bit characters--is someone writing in Klingon? */
    
    {
      WCHAR2 *str2 = STR2(str);
      INT j, length;
      
      length = str->length*2;
      
      /* Check how many extra wide characters there are. */
      for(i = str->length; i--;)
      {
	if(str2[i] > 0xfffd)
	{
	  if(str2[i] < 0x10000)
	    /* 0xfffe: Byte-order detection illegal character.
	       0xffff: Illegal character. */
	    err_fatal("string-to-unicode: Illegal character 0x%04x (index %d)"
		      " is not a Unicode character.", str2[i], i);
	  
	  if(str2[i] > 0x10ffff)
	    err_fatal("string-to-unicode: Character 0x%08x (index %d) "
		      "is out of range (0x00000000 - 0x0010ffff).",str2[i],i);
	  
	  /* Extra wide characters take two unicode characters in space.
	     i.e. one unicode character extra. */
	  length += 2;
	}
      }
      
      raw = str_allocate_raw_wide(length, 1);
      j = length;
      
      for(i = str->length; i--;)
      {
	UINT32 c = str2[i];

	j -= 2;

	if(c > 0xffff)
	{
	  /* Use surrogates. */
	  c -= 0x10000;
	  
	  raw->s[j + 1] = c & 0xff;
	  raw->s[j] = 0xdc | ((c >> 8) & 0x03);
	  j -= 2;
	  c >>= 10;
	  c |= 0xd800;
	}
	
	raw->s[j + 1] = c & 0xff;
	raw->s[j] = c >> 8;
      }
      
      str = str_commit_raw(&process->str_heap, raw);
    }
    break;
    
  default:
    err_fatal("string-to-unicode: Bad string shift (%d).\n", str->shift);
  }

  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_unicode_to_string)
{
  struct str *str;
  
#if (SHOE_BYTEORDER != 4321)
  
  /* Little endian. */
  struct str *raw;
  INT i, length;
  WCHAR1 *str1;
  
#endif /* SHOE_BYTEORDER == 4321 */
  
  ARGS_GET((process, "unicode->string", args, "%s", &str));
  
  if(str->length & 1)
    err_fatal("str_unicode_to_string: String length is odd.");

  /* FIXME: In the future add support for decoding of surrogates? */

#if (SHOE_BYTEORDER == 4321)
  
  /* Big endian. */
  str = str_allocate_wide(&process->str_heap, str->s, str->length, 1);
  
#else
  
  /* Little endian. */
  length = str->length/2;
  raw = str_allocate_raw_wide(length, 1);
  str1 = STR1(raw);

  for(i = length; i--;)
    str1[i] = (((UCHAR*)str->s)[i*2] << 8) + ((UCHAR*)str->s)[i*2 + 1];
  
  str = str_commit_raw(&process->str_heap, raw);
  
#endif /* SHOE_BYTEORDER == 4321 */

  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_utf8_to_string)
{
  INT length = 0, shift = 0, i, j, extended = 0;
  struct str *str, *raw;

  ARGS_GET((process, "utf8->string", args, "%w?%i", &str, &extended));

  for(i = 0; i < str->length; i++)
  {
    UINT c = ((UCHAR*)str->s)[i];
    length++;
    
    if(c & 0x80)
    {
      INT cont = 0;
      
      if((c & 0xc0) == 0x80)
	err_fatal("utf8-to-string: Unexpected continuation block 0x%02x "
		  "at index %d.\n", c, i);

      if((c & 0xe0) == 0xc0)
      {
	/* 11-bit. */
	cont = 1;
	if(c & 0x1c)
	  if(shift < 1)
	    shift = 1;
      }
      else if((c & 0xf0) == 0xe0)
      {
	/* 16-bit. */
	cont = 2;
	if(shift < 1)
	  shift = 1;
      }
      else
      {
	shift = 2;
	if((c & 0xf8) == 0xf0)
	  /* 21-bit. */
	  cont = 3;
	else if((c & 0xfc) == 0xf8)
	  /* 26-bit. */
	  cont = 4;
	else if((c & 0xfe) == 0xfc)
	  /* 31-bit. */
	  cont = 5;
	else if(c == 0xfe)
	{
	  /* 36-bit. */
	  if(!extended)
	    err_fatal("utf8-to-string: Character 0xfe at index %d when "
		      "not in extended mode.\n", i);
	  
	  cont = 6;
	}
	else
	  err_fatal("utf8-to-string(): Unexpected character 0xff at "
		    "index %d.\n", i);
      }
      
      while(cont--)
      {
	i++;
	
	if(i >= str->length)
	  err_fatal("utf8-to-string: Truncated UTF8 sequence.\n");
	
	c = ((UCHAR*)(str->s))[i];
	if((c & 0xc0) != 0x80)
	  err_fatal("utf8-to-string: Expected continuation character "
		    "at index %d (got 0x%02x).\n", i, c);
      }
    }
  }
  
  if(length == str->length)
  {
    /* 7-bit in == 7-bit out. */
    BIF_RESULT_STRING(str);
    return;
  }

  raw = str_allocate_raw_wide(length, shift);
  
  for(j = i = 0; i < str->length; i++)
  {
    UINT c = ((UCHAR*)str->s)[i];

    if(c & 0x80)
    {
      int cont = 0;

      /* NOTE: The tests aren't as paranoid here, since we've
       * already tested the string above.
       */
      if((c & 0xe0) == 0xc0)
      {
	/* 11-bit. */
	cont = 1;
	c &= 0x1f;
      }
      else if((c & 0xf0) == 0xe0)
      {
	/* 16-bit. */
	cont = 2;
	c &= 0x0f;
      }
      else if((c & 0xf8) == 0xf0)
      {
	/* 21-bit. */
	cont = 3;
	c &= 0x07;
      }
      else if((c & 0xfc) == 0xf8)
      {
	/* 26-bit. */
	cont = 4;
	c &= 0x03;
      }
      else if((c & 0xfe) == 0xfc)
      {
	/* 31-bit. */
	cont = 5;
	c &= 0x01;
      }
      else
      {
	/* 36-bit. */
	cont = 6;
	c = 0;
      }
      
      while(cont--)
      {
	UINT32 c2 = ((UCHAR*)(str->s))[++i] & 0x3f;
	c = (c << 6) | c2;
      }
    }
    
    STR_INDEX_SET(raw, j, c);
    j++;
  }
  
  str = str_commit_raw(&process->str_heap, raw);

  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_string_to_utf8)
{
  INT extended = 0, i, j, length;
  struct str *raw, *str;

  ARGS_GET((process, "string->utf8", args, "%w?%i", &str, &extended));

  length = str->length;

  for(i = 0; i < str->length; i++)
  {
    UINT32 c = STR_INDEX(str, i);
    
    if(c & ~0x7f)
    {
      /* 8-bit or more. */
      length++;
      if(c & ~0x7ff)
      {
	/* 12-bit or more. */
	length++;
	if(c & ~0xffff)
	{
	  /* 17-bit or more. */
	  length++;
	  if(c & ~0x1fffff)
	  {
	    /* 22-bit or more. */
	    length++;
	    if(c & ~0x3ffffff)
	    {
	      /* 27-bit or more. */
	      length++;
	      if(c & ~0x7fffffff)
	      {
		/* 32-bit or more. */
		if(!extended)
		  err_fatal("string-to-utf8: Value 0x%08x (index %d) "
			    "is larger than 31 bits.\n", c, i);
		length++;
		
		/* FIXME: Needs fixing when we get 64-bit characters... */
	      }
	    }
	  }
	}
      }
    }
  }
  
  if(length == str->length)
  {
    /* 7-bit string--already valid utf8. */
    BIF_RESULT_STRING(str);
    return;
  }
  
  raw = str_allocate_raw(length);

  for(i = j = 0; i < str->length; i++)
  {
    UINT32 c = STR_INDEX(str, i);
    
    if(!(c & ~0x7f))
    {
      /* 7-bit. */
      raw->s[j++] = c;
    }
    else if(!(c & ~0x7ff))
    {
      /* 11-bit. */
      raw->s[j++] = 0xc0 | (c >> 6);
      raw->s[j++] = 0x80 | (c & 0x3f);
    }
    else if(!(c & ~0xffff))
    {
      /* 16-bit. */
      raw->s[j++] = 0xe0 | (c >> 12);
      raw->s[j++] = 0x80 | ((c >> 6) & 0x3f);
      raw->s[j++] = 0x80 | (c & 0x3f);
    }
    else if(!(c & ~0x1fffff))
    {
      /* 21-bit. */
      raw->s[j++] = 0xf0 | (c >> 18);
      raw->s[j++] = 0x80 | ((c >> 12) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 6) & 0x3f);
      raw->s[j++] = 0x80 | (c & 0x3f);
    }
    else if(!(c & ~0x3ffffff))
    {
      /* 26-bit. */
      raw->s[j++] = 0xf8 | (c >> 24);
      raw->s[j++] = 0x80 | ((c >> 18) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 12) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 6) & 0x3f);
      raw->s[j++] = 0x80 | (c & 0x3f);
    }
    else if(!(c & ~0x7fffffff))
    {
      /* 31-bit. */
      raw->s[j++] = 0xfc | (c >> 30);
      raw->s[j++] = 0x80 | ((c >> 24) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 18) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 12) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 6) & 0x3f);
      raw->s[j++] = 0x80 | (c & 0x3f);
    }
    else
    {
      /* This and onwards is extended utf8 encoding, 32--36-bit. */
      raw->s[j++] = (char)0xfe;
      raw->s[j++] = 0x80 | ((c >> 30) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 24) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 18) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 12) & 0x3f);
      raw->s[j++] = 0x80 | ((c >> 6) & 0x3f);
      raw->s[j++] = 0x80 | (c & 0x3f);
    }
  }

  str = str_commit_raw(&process->str_heap, raw);

  BIF_RESULT_STRING(str);
}

/*
 * Symbols.
 */

BIF_DECLARE(bif_symbol_to_string)
{
  struct str *sym;
  
  ARGS_GET((process, "symbol->string", args, "%S", &sym));

  BIF_RESULT_STRING(sym);
}

BIF_DECLARE(bif_unquote_symbol)
     /* FIXME: Wide-strings. */
{
  struct str *str;
  
  ARGS_GET((process, "unquote-symbol", args, "%S", &str));

  str = str_allocate_escaped(&process->str_heap, str->s, str->length);
  
  BIF_RESULT_SYMBOL(str);
}

/*
 * Vectors.
 */

BIF_DECLARE(bif_make_vector)
{
  struct svalue *val, elm;
  struct vec *vec;
  INT i, k;

  ARGS_GET((process, "make-vector", args, "%i?%*", &k, &val));
  
  if(val)
    elm = *val;
  else
    elm.type = T_UNDEFINED;

  if(k < 0)
    ARGS_ERROR((process, "make-vector", "Length cannot be negative."));
  
  vec = vec_allocate(&process->vec_heap, k);
  
  for(i = 0; i < k; i++)
    vec->v[i] = elm;
  
  BIF_RESULT_VECTOR(vec);
}

BIF_DECLARE(bif_vector)
{
  struct vec *vec;
  struct pair *p;
  INT i, n = 0;
  
  if(IS_PAIR(*args))
  {
    p = args->u.pair;
    
    for(n = 1; IS_PAIR(CDR(p)); n++)
      p = CDR(p).u.pair;
  }
  
  vec = vec_allocate(&process->vec_heap, n);

  if(n > 0)
  {
    p = args->u.pair;
    for(i = 0; i < n; i++)
    {
      vec->v[i] = CAR(p);
      p = CDR(p).u.pair;
    }
  }

  BIF_RESULT_VECTOR(vec);
}

BIF_DECLARE(bif_vector_to_list)
{
  struct vec *vec;
  INT i;
  
  ARGS_GET((process, "vector->list", args, "%v", &vec));

  BIF_RESULT_NIL();
  
  for(i = vec->length-1 ; 0 <= i; i--)
    CONS(result, &vec->v[i], result);
}

BIF_DECLARE(bif_vector_copy)
{
  struct vec *vec1, *vec2;
  INT i;
  
  ARGS_GET((process, "vector->copy", args, "%v", &vec1));

  vec2 = vec_allocate(&process->vec_heap, vec1->length);
  
  BIF_RESULT_VECTOR(vec2);
  
  for(i = 0 ; i < vec1->length; i++)
    vec2->v[i] = vec1->v[i];
}

BIF_DECLARE(bif_vector_fill)
{
  struct svalue *value;
  struct vec *vec;
  INT i;
  
  ARGS_GET((process, "vector->fill", args, "%v%*", &vec, &value));

  BIF_RESULT_VECTOR(vec);
  
  for(i = 0 ; i < vec->length; i++)
    vec->v[i] = *value;
}

BIF_DECLARE(bif_vector_length)
{
  struct vec *vec;

  ARGS_GET((process, "vector-length", args, "%v", &vec));

  BIF_RESULT_SMALL_INTEGER(vec->length);
}

BIF_DECLARE(bif_vector_ref)
{
  struct vec *vec;
  INT ref;

  ARGS_GET((process, "vector-ref", args, "%v%i", &vec, &ref));
  RANGE_CHECK(process, "vector-ref", ref, vec->length);
  
  *result = vec->v[ref];
}

BIF_DECLARE(bif_vector_set)
{
  struct svalue *val;
  struct vec *vec;
  INT ref;

  ARGS_GET((process, "vector-set!", args, "%v%i%*", &vec, &ref, &val));
  RANGE_CHECK(process, "vector-set!", ref, vec->length);
  
  vec->v[ref] = *val;
  
  BIF_RESULT_UNDEFINED();
}

/*
 * Mappings.
 */

BIF_DECLARE(bif_make_mapping)
{
  struct vec *keys, *vals;
  struct map *map;
  INT i;

  ARGS_GET((process, "make-mapping", args, "?%v%v", &keys, &vals));

  if(keys && !vals)
    err_fatal("make-mapping: Too few arguments.");
  
  if(keys && keys->length != vals->length)
    err_fatal("make-mapping: Array sizes are not equal (%d is not %d).",
	      keys->length, vals->length);

  map = map_allocate(&process->map_heap);

  if(keys)
    for(i = 0; i < keys->length; i++)
      map_set(map, &keys->v[i], &vals->v[i]);
  
  BIF_RESULT_MAPPING(map);
}

BIF_DECLARE(bif_mapping_length)
{
  struct map *map;

  ARGS_GET((process, "mapping-length", args, "%m", &map));
  
  BIF_RESULT_SMALL_INTEGER(map->used);
}

BIF_DECLARE(bif_mapping_to_list)
{
  struct map_entry *entry;
  struct map_table *table;
  struct map *map;
  INT i;
  
  ARGS_GET((process, "mapping->list", args, "%m", &map));
  
  BIF_RESULT_NIL();
  
  if(map->used)
    for(i = 0, table = map->table; i < map->hash_size; i++)
      for(entry = table->hash[i]; entry; entry = entry->next)
      {
	CONS(result, result, result);
	LIST(&CAR(result->u.pair), &entry->value);
	CONS(&CAR(result->u.pair), &entry->key, &CAR(result->u.pair));
      }
}

BIF_DECLARE(bif_mapping_copy)
{
  struct map_entry *entry;
  struct map_table *table;
  struct map *map1, *map2;
  INT i;
  
  ARGS_GET((process, "mapping-copy", args, "%m", &map1));

  map2 = map_allocate(&process->map_heap);

  BIF_RESULT_MAPPING(map2);
  
  if(map1->used)
    for(i = 0, table = map1->table; i < map1->hash_size; i++)
      for(entry = table->hash[i]; entry; entry = entry->next)
	map_set(map2, &entry->key, &entry->value);
}

BIF_DECLARE(bif_mapping_keys)
{
  struct map_entry *entry;
  struct map_table *table;
  struct map *map;
  struct vec *vec;
  INT i, n;
  
  ARGS_GET((process, "mapping-keys", args, "%m", &map));

  vec = vec_allocate(&process->vec_heap, map->used);

  BIF_RESULT_VECTOR(vec);
  
  if(map->used)
    for(i = n = 0, table = map->table; i < map->hash_size; i++)
      for(entry = table->hash[i]; entry; entry = entry->next)
	vec->v[n++] = entry->key;
}

BIF_DECLARE(bif_mapping_values)
{
  struct map_entry *entry;
  struct map_table *table;
  struct map *map;
  struct vec *vec;
  INT i, n;
  
  ARGS_GET((process, "mapping-values", args, "%m", &map));

  vec = vec_allocate(&process->vec_heap, map->used);

  BIF_RESULT_VECTOR(vec);
  
  if(map->used)
    for(i = n = 0, table = map->table; i < map->hash_size; i++)
      for(entry = table->hash[i]; entry; entry = entry->next)
	vec->v[n++] = entry->value;
}

BIF_DECLARE(bif_mapping_ref)
{
  struct svalue *ref;
  struct map *map;

  ARGS_GET((process, "mapping-ref", args, "%m%*", &map, &ref));

  ref = map_get(map, ref);
  
  if(ref)
    *result = *ref;
  else
    BIF_RESULT_FALSE();
}

BIF_DECLARE(bif_mapping_set)
{
  struct svalue *ref, *val;
  struct map *map;

  ARGS_GET((process, "mapping-set!", args, "%m%*%*", &map, &ref, &val));
  
  map_set(map, ref, val);
  
  BIF_RESULT_UNDEFINED();
}

BIF_DECLARE(bif_mapping_remove)
{
  struct svalue *ref;
  struct map *map;

  ARGS_GET((process, "mapping-remove!", args, "%m%*", &map, &ref));
  
  map_remove(map, ref);
  
  BIF_RESULT_UNDEFINED();
}

/*
 * Output.
 */

BIF_DECLARE(bif_display)
{
  struct svalue *val;
  
  ARGS_GET((process, "display", args, "%*", &val));

  err_display(process, val);
  
  BIF_RESULT_UNDEFINED();
}

BIF_DECLARE(bif_newline)
{
  err_printf("\n");
  
  BIF_RESULT_UNDEFINED();
}

/*
 * Experimental.
 */

BIF_DECLARE(bif_has_big_integers)
{
  ARGS_GET((process, "has-big-integers?", args, ""));

#ifdef USE_BIG_INTEGERS
  BIF_RESULT_TRUE();
#else
  BIF_RESULT_FALSE();
#endif /* USE_BIG_INTEGERS */
}

BIF_DECLARE(bif_compiler_bifs)
{
  struct svalue key, val;
  struct map *map;
  INT i;
  
  ARGS_GET((process, "compiler-bifs", args, ""));

  map = map_allocate(&process->map_heap);
  key.type = T_SYMBOL;
  val.type = T_SMALL_INTEGER;
  
  for(i = 0; bifs[i].name; i++)
  {
    key.u.str = str_allocate_text(&process->str_heap, bifs[i].name);
    val.u.integer = i;
    
    map_set(map, &key, &val);
  }
  
  BIF_RESULT_MAPPING(map);
}
  
BIF_DECLARE(bif_compiler_instructions)
{
  struct svalue key, val;
  struct map *map;
  INT i;
  
  ARGS_GET((process, "compiler-instructions", args, ""));

  map = map_allocate(&process->map_heap);
  key.type = T_SYMBOL;
  val.type = T_SMALL_INTEGER;
  
  for(i = 0; bifs[i].name; i++)
  {
    key.u.str = str_allocate_text(&process->str_heap, bifs[i].name);
    val.u.integer = i;
    
    map_set(map, &key, &val);
  }
  
  BIF_RESULT_MAPPING(map);
}

BIF_DECLARE(bif_lexer)
{
  struct str *str;
  
  ARGS_GET((process, "lexer", args, "%s", &str));
  
  BIF_RESULT_VECTOR(lex_read(process, str));
}

static char to_hex(char c)
{
  return c < 10 ? c + '0' : c + 'a' - 10;
}

BIF_DECLARE(bif_compiler_program_to_c)
{
  struct str *str, *raw;
  INT i, j = 0;
  
  ARGS_GET((process, "compiler-program->c", args, "%s", &str));

  if(str->length <= 0)
  {
    str = str_allocate(&process->str_heap, "", 0);
    BIF_RESULT_STRING(str);
    return;
  }
  
  raw = str_allocate_raw(str->length*5-1);
  
  raw->s[j++] = '0';
  raw->s[j++] = 'x';
  raw->s[j++] = to_hex((str->s[0] >> 4) & 0xf);
  raw->s[j++] = to_hex(str->s[0] & 0xf);
  for(i = 1; i < str->length; i++)
  {
    raw->s[j++] = ',';
    raw->s[j++] = '0';
    raw->s[j++] = 'x';
    raw->s[j++] = to_hex((str->s[i] >> 4) & 0xf);
    raw->s[j++] = to_hex(str->s[i] & 0xf);
  }
  
  str = str_commit_raw(&process->str_heap, raw);
  
  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_trap_ref)
{
  INT i;
  
  ARGS_GET((process, "trap-ref", args, "%i", &i));

  if(i < 0 || N_TRAPS <= i)
    err_fatal("trap-ref: Trap %d is not within range 0 - %d.", i, N_TRAPS-1);
  
  *result = process->trap[i];
}

BIF_DECLARE(bif_trap_set)
{
  struct svalue *v;
  INT i;
  
  ARGS_GET((process, "trap-set!", args, "%i%*", &i, &v));

  if(i < 0 || N_TRAPS <= i)
    err_fatal("trap-set!: Trap %d is not within range 0 - %d.", i, N_TRAPS-1);
  
  process->trap[i] = *v;
  BIF_RESULT_UNDEFINED();
}

BIF_DECLARE(bif_load_program)
{
  struct svalue env;
  struct vec *vec;
  
  ARGS_GET((process, "load-program", args, "%v", &vec));

  result->u.vec = vec;
  result->aux = 0;
  result->type = T_LABEL;

  env.type = T_NIL;
  
  CONS(result, result, &env);
  result->type = T_LAMBDA;
}

BIF_DECLARE(bif_small_integer_size)
{
  ARGS_GET((process, "small-integer-size", args, ""));

  BIF_RESULT_SMALL_INTEGER((INT)sizeof(INT));
}

BIF_DECLARE(bif_float_size)
{
  ARGS_GET((process, "float-size", args, ""));

  BIF_RESULT_SMALL_INTEGER((INT)sizeof(REAL));
}

BIF_DECLARE(bif_small_integer_to_bytes)
{
  struct svalue b;
  INT i, j;

  ARGS_GET((process, "integer->bytes", args, "%i", &j));

  BIF_RESULT_NIL();
  b.type = T_SMALL_INTEGER;
  for(i = (INT)sizeof(INT)-1; 0 <= i; i--, j >>= 8)
  {
    b.u.integer = j & 0xff;
    CONS(result, &b, result);
  }
}

BIF_DECLARE(bif_float_to_bytes)
{
  /* FIXME: Use "proper" encoding. */
  
  struct svalue b;
  union
  {
    REAL r;
    UBYTE b[sizeof(REAL)];
  } u;
  INT i;

  ARGS_GET((process, "float->bytes", args, "%f", &u.r));

  BIF_RESULT_NIL();
  b.type = T_SMALL_INTEGER;
  for(i = (INT)sizeof(REAL)-1; 0 <= i; i--)
  {
    b.u.integer = u.b[i];
    CONS(result, &b, result);
  }
}

#define READ_FILE_BUFFER_SIZE 4096

BIF_DECLARE(bif_read_binary)
{
  char buffer[READ_FILE_BUFFER_SIZE];
  struct str *str = 0;
  INT n;

  ARGS_GET((process, "read-binary", args, ""));

  n = read(STDIN_FILENO, buffer, READ_FILE_BUFFER_SIZE);

  if(n < 0)
  {
    BIF_RESULT_FALSE();
    return;
  }

  str = str_allocate(&process->str_heap, buffer, n);

  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_read_binary_file)
{
  char buffer[READ_FILE_BUFFER_SIZE];
  struct str *str = 0, *filename;
  int fd;
  INT n;

  ARGS_GET((process, "read-binary-file", args, "%s", &filename));

  fd = open(filename->s, O_RDONLY);
  
  BIF_RESULT_FALSE();
  
  if(fd == -1)
    return;

  for(;;)
  {
    n = read(fd, buffer, READ_FILE_BUFFER_SIZE);

    if(n == 0)
      break;
    
    if(n < 0)
    {
      close(fd);
      
      if(str)
	str_free_raw(str);
      
      return;
    }

    if(str)
      str = str_append_raw(str, buffer, n);
    else {
      str = str_allocate_raw(n);
      mem_copy(str->s, buffer, n);
    }
  }

  close(fd);
      
  if(str)
    str = str_commit_raw(&process->str_heap, str);
  else
    str = str_allocate(&process->str_heap, "", 0);

  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_write_binary_file)
{
  struct str *str, *filename;
  INT n, t;
  int fd;

  ARGS_GET((process, "write-binary-file", args, "%s%s", &filename, &str));

  fd = open(filename->s, O_CREAT | O_TRUNC | O_WRONLY, 0666);
  
  BIF_RESULT_FALSE();
  
  if(fd == -1)
    return;

  for(t = 0; t < str->length; t += n)
  {
    n = write(fd, str->s + t, str->length - t);

    if(n <= 0)
    {
      close(fd);
      return;
    }
  }

  close(fd);
      
  BIF_RESULT_TRUE();
}

/* ERRORS ********************************************************************/

BIF_DECLARE(bif_fatal_error)
{
  struct pair *p;

  BIF_RESULT_UNDEFINED();
  
  err_printf("Error -- ");
  if(IS_PAIR(*args))
  {
    p = args->u.pair;

    for(;;)
    {
      err_display(process, &CAR(p));
  
      if(IS_NOT_PAIR(CDR(p)))
	break;
      err_printf(" ");
      p = CDR(p).u.pair;
    }
  }
  
  err_printf("\n");
  exit(1);
}

struct bif bifs[] =
{ {  "invocation-arguments", bif_invocation_arguments          },
  { 		 "boolean?", bif_booleanp                      },
  { 		    "char?", bif_charp                         },
  { 		    "null?", bif_nullp                         },
  { 		  "number?", bif_numberp                       },
  { 		    "pair?", bif_pairp                         },
  { 		    "port?", bif_portp                         },
  { 	       "procedure?", bif_procedurep                    },
  { 		  "string?", bif_stringp                       },
  { 		  "symbol?", bif_symbolp                       },
  { 		  "vector?", bif_vectorp                       },
  { 	      "input-port?", 0 /*bif_input_port*/              },
  { 	     "output-port?", 0 /*bif_output_port*/             },
  { 		      "eq?", bif_eq                            },
  { 		     "cons", bif_cons                          },
  { 		      "car", bif_car                           },
  { 		      "cdr", bif_cdr                           },
  { 		     "list", bif_list                          },
  { 	     "list->vector", bif_list_to_vector                },
  { 		 "set-car!", bif_set_car                       },
  { 		 "set-cdr!", bif_set_cdr                       },
  { 	    "char->integer", bif_char_to_integer               },
  { 	       "char-ci<=?", 0 /*bif_char_ci_less_equal*/      },
  { 		"char-ci<?", 0 /*bif_char_ci_less_than*/       },
  { 		"char-ci=?", 0 /*bif_char_ci_equal*/           },
  { 	       "char-ci>=?", 0 /*bif_char_ci_greater_equal*/   },
  { 		"char-ci>?", 0 /*bif_char_ci_greater_than*/    },
  { 		  "char<=?", bif_char_less_equal               },
  { 		   "char<?", bif_char_less_than                },
  { 		   "char=?", bif_char_equal                    },
  { 		  "char>=?", bif_char_greater_equal            },
  { 		   "char>?", bif_char_greater_than             },
  { 	    "integer->char", bif_integer_to_char               },
  { 	      "make-string", bif_make_string                   },
  { 		   "string", bif_string                        },
  { 	     "string->list", bif_string_to_list                },
  { 	   "string->number", 0 /*bif_string_to_number*/        },
  { 	   "string->symbol", bif_string_to_symbol              },
  { 	    "string-append", bif_string_append                 },
  { 	     "string-ci<=?", 0 /*bif_string_ci_less_equal*/    },
  { 	      "string-ci<?", 0 /*bif_string_ci_less_than*/     },
  { 	      "string-ci=?", 0 /*bif_string_ci_equal*/         },
  { 	     "string-ci>=?", 0 /*bif_string_ci_greater_equal*/ },
  { 	      "string-ci>?", 0 /*bif_string_ci_greater_than*/  },
  { 	      "string-copy", bif_string_copy                   },
  { 	      "string-fill", bif_string_fill                   },
  { 	    "string-length", bif_string_length                 },
  { 	       "string-ref", bif_string_ref                    },
  { 	       "string-set", bif_string_set                    },
  { 		"string<=?", bif_string_less_equal             },
  { 		 "string<?", bif_string_less_than              },
  { 		 "string=?", bif_string_equal                  },
  { 		"string>=?", bif_string_greater_equal          },
  { 		 "string>?", bif_string_greater_than           },
  { 		"substring", bif_substring                     },
  { 	   "symbol->string", bif_symbol_to_string              },
  { 	      "make-vector", bif_make_vector                   },
  { 		   "vector", bif_vector                        },
  { 	    "vector-length", bif_vector_length                 },
  { 	       "vector-ref", bif_vector_ref                    },
  { 	      "vector-set!", bif_vector_set                    },
  { 	     "vector->list", bif_vector_to_list                },
  { 	     "vector-fill!", bif_vector_fill                   },
  { 	     "make-mapping", bif_make_mapping                  },
  { 	   "mapping-length", bif_mapping_length                },
  { 	      "mapping-ref", bif_mapping_ref                   },
  { 	     "mapping-set!", bif_mapping_set                   },
  { 	  "mapping-remove!", bif_mapping_remove                },
  {  "call-with-input-file", 0 /*bif_call_with_input_file*/    },
  { "call-with-output-file", 0 /*bif_with_output_file*/        },
  {    "current-input-port", 0 /*bif_current_input_port*/      },
  {   "current-output-port", 0 /*bif_current_output_port*/     },
  {  "with-input-from-file", 0 /*bif_with_input_from_file*/    },
  {   "with-output-to-file", 0 /*bif_with_output_to_file*/     },
  {       "open-input-file", 0 /*bif_open_input_file*/         },
  {      "open-output-file", 0 /*bif_open_output_file*/        },
  {      "close-input-port", 0 /*bif_close_input_port*/        },
  {     "close-output-port", 0 /*bif_close_output_port*/       },
  {                  "read", 0 /*bif_read*/                    },
  {             "read-char", 0 /*bif_read_char*/               },
  {             "peek-char", 0 /*bif_peek_char*/               },
  {           "eof-object?", 0 /*bif_eof_object*/              },
  {           "char-ready?", 0 /*bif_char_ready*/              },
  {                 "write", 0 /*bif_write*/                   },
  {               "display", bif_display                       },
  {               "newline", bif_newline                       },
  {            "write-char", 0 /*bif_write_char*/              },
  {         "compiler-bifs", bif_compiler_bifs                 },
  {                 "lexer", bif_lexer                         },
  {  "small-integer->bytes", bif_small_integer_to_bytes        },
  {      "read-binary-file", bif_read_binary_file              },
  {        "number->string", bif_number_to_string              },
  {                     "<", bif_less                          },
  {    	                "+", bif_plus                          },
  {    	                "-", bif_minus                         },
  {    	                "*", bif_times                         },
  {    	      "fatal-error", bif_fatal_error                   },
  { 	     "list->string", bif_list_to_string                },
  { 	     "load-program", bif_load_program                  },
  { 	           "append", bif_append                        },
  { 	          "reverse", bif_reverse                       },
  { 	           "length", bif_length                        },
  { 	            "list?", bif_listp                         },
  { 	             "memq", bif_memq                          },
  { 	        "list-tail", bif_list_tail                     },
  { 	         "list-ref", bif_list_ref                      },
  { "compiler-instructions", bif_compiler_instructions         },
  {        "unquote-string", bif_unquote_string                },
  {        "unquote-symbol", bif_unquote_symbol                },
  {         "mapping->list", bif_mapping_to_list               },
  {         "list->mapping", bif_list_to_mapping               },
  { 		 "mapping?", bif_mappingp                      },
  { 	     "mapping-keys", bif_mapping_keys                  },
  { 	   "mapping-values", bif_mapping_values                },
  { 	     "mapping-copy", bif_mapping_copy                  },
  { 	      "vector-copy", bif_vector_copy                   },
  { 	        "list-copy", bif_list_copy                     },
  { 	         "integer?", bif_integerp                      },
  { 	            "real?", bif_realp                         },
  { 	     "float->bytes", bif_float_to_bytes                },
  { 	         "trap-ref", bif_trap_ref                      },
  { 	        "trap-set!", bif_trap_set                      },
  { 	  "unicode->string", bif_unicode_to_string             },
  { 	  "string->unicode", bif_string_to_unicode             },
  { 	     "utf8->string", bif_utf8_to_string                },
  { 	     "string->utf8", bif_string_to_utf8                },
  { 	"write-binary-file", bif_write_binary_file             }, 
  {   "compiler-program->c", bif_compiler_program_to_c         },
  {        "small-integer?", bif_small_integerp                },
  {          "big-integer?", bif_big_integerp                  },
  {                     "/", bif_division                      },
  { 	           "exact?", bif_exactp                        },
  { 	         "inexact?", bif_inexactp                      },
  { 	            "zero?", bif_zerop                         },
  { 	        "positive?", bif_positivep                     },
  { 	        "negative?", bif_negativep                     },
  { 	             "odd?", bif_is_odd                        },
  { 	            "even?", bif_is_even                       },
  { 	              "abs", bif_abs                           },
  { 	            "floor", bif_floor                         },
  { 	          "ceiling", bif_ceiling                       },
  { 	         "truncate", bif_truncate                      },
  {	            "round", bif_round                         },
  {	   "exact->inexact", bif_exact_to_inexact              },
  {	   "inexact->exact", bif_inexact_to_exact              },
  {           "read-binary", bif_read_binary                   },
  {     "has-big-integers?", bif_has_big_integers              },
  {            "float-size", bif_float_size                    },
  {    "small-integer-size", bif_small_integer_size            },
  {          "shoe-version", bif_shoe_version                  },
  {         "debug-objects", bif_debug_objects                 },
  {          "debug-memory", bif_debug_memory                  },
  {                    "gc", bif_gc                            },
  {            "list-first", bif_list_first                    },
  {             "list-rest", bif_list_rest                     },
  {           "debug-pairs", bif_debug_pairs                   },
 {    	                  0, 0                                 } };
