/* types.h
 *
 * Copyright (c) 1999 by Fredrik Noring.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#include "universe.h"

#define REAL  float
#define BYTE  signed char
#define UCHAR unsigned char
#define CHAR  signed char
#define UBYTE unsigned char
#define INT   signed int
#define UINT  unsigned int

#define INT8 BYTE
#define INT16 signed short
#define INT32 signed int

#define UINT8 UBYTE
#define UINT16 unsigned short
#define UINT32 unsigned int

#define WCHAR0 INT8
#define WCHAR1 INT16
#define WCHAR2 INT32

#define MAXINT ((((1 << (8*(sizeof(INT)-1)+6))-1)<<1)+1)

/* Non-referencing. */
#define T_UNDEFINED     0
#define T_REAL          1
#define T_SMALL_INTEGER 2
#define T_LABEL         3
#define T_NIL           4
#define T_TRUE          5
#define T_FALSE         6
#define T_BIF           7
#define T_CHAR          8
#define T_FREE          9
#define T_PORT         10
#define T_VECTOR       11
#define T_MAPPING      12
#define T_BIG_INTEGER  13
/* Referencing. */
#define REFERENCING    20
#define T_PAIR         20
#define T_SYMBOL       21
#define T_STRING       22
#define T_LAMBDA       23
#define T_CONTINUATION 24

struct svalue;
struct process;

typedef void (bif)(struct process *process, struct svalue *, struct svalue *);

union anything
{
  struct pair *pair;
  struct big *big;
  struct map *map;
  struct str *str;
  struct vec *vec;
  INT integer;
  REAL real;
  bif *bif;
};

struct svalue
{
  INT type:8;
  INT aux:24;
  union anything u;
};

struct pair
{
  struct svalue car;
  struct svalue cdr;
};

#define MARK(s)      ((s)->type |= 0x80)
#define UNMARK(s)    ((s)->type &= 0x7f)
#define IS_MARKED(s) ((s)->type & 0x80)

#define IS_NUMBER(x)            ((x).type == T_SMALL_INTEGER ||            \
			 	 (x).type == T_BIG_INTEGER ||              \
			 	 (x).type == T_REAL)
#define IS_NOT_NUMBER(x)        ((x).type != T_SMALL_INTEGER &&            \
			 	 (x).type != T_BIG_INTEGER &&              \
			 	 (x).type != T_REAL)

#define IS_REAL(x)              IS_NUMBER(x)
#define IS_NOT_REAL(x)          IS_NOT_NUMBER(x)

#define IS_EXACT(x)             IS_INTEGER(x)
#define IS_NOT_EXACT(x)         IS_NOT_INTEGER(x)

#define IS_INEXACT(x)           IS_FLOAT(x)
#define IS_NOT_INEXACT(x)       IS_NOT_FLOAT(x)

#define IS_INTEGER(x)           ((x).type == T_SMALL_INTEGER ||            \
			 	 (x).type == T_BIG_INTEGER)
#define IS_NOT_INTEGER(x)       ((x).type != T_SMALL_INTEGER &&            \
			 	 (x).type != T_BIG_INTEGER)

#define IS_SMALL_INTEGER(x)     ((x).type == T_SMALL_INTEGER)
#define IS_NOT_SMALL_INTEGER(x) ((x).type != T_SMALL_INTEGER)

#define IS_BIG_INTEGER(x)       ((x).type == T_BIG_INTEGER)
#define IS_NOT_BIG_INTEGER(x)   ((x).type != T_BIG_INTEGER)

#define IS_FLOAT(x)             ((x).type == T_REAL)
#define IS_NOT_FLOAT(x)         ((x).type != T_REAL)

#define IS_TRUE(x)              ((x).type == T_TRUE)
#define IS_FALSE(x)             ((x).type == T_FALSE)

#define IS_PAIR(x)              ((x).type == T_PAIR)
#define IS_NOT_PAIR(x)          ((x).type != T_PAIR)

#define IS_NIL(x)               ((x).type == T_NIL)
#define IS_NOT_NIL(x)           ((x).type != T_NIL)

#define IS_BIF(x)               ((x).type == T_BIF)
#define IS_NOT_BIF(x)           ((x).type != T_BIF)

#define IS_LAMBDA(x)            ((x).type == T_LAMBDA)
#define IS_NOT_LAMBDA(x)        ((x).type != T_LAMBDA)

#define IS_CONTINUATION(x)      ((x).type == T_CONTINUATION)
#define IS_NOT_CONTINUATION(x)  ((x).type != T_CONTINUATION)

#define IS_SYMBOL(x)            ((x).type == T_SYMBOL)
#define IS_NOT_SYMBOL(x)        ((x).type != T_SYMBOL)

#define IS_STRING(x)            ((x).type == T_STRING)
#define IS_NOT_STRING(x)        ((x).type != T_STRING)

#define IS_CHAR(x)              ((x).type == T_CHAR)
#define IS_NOT_CHAR(x)          ((x).type != T_CHAR)

#define IS_NULL(x)              ((x).type == T_NIL)
#define IS_NOT_NULL(x)          ((x).type != T_NIL)

#define IS_PORT(x)              ((x).type == T_PORT)
#define IS_NOT_PORT(x)          ((x).type != T_PORT)

#define IS_VECTOR(x)            ((x).type == T_VECTOR)
#define IS_NOT_VECTOR(x)        ((x).type != T_VECTOR)

#define IS_MAPPING(x)           ((x).type == T_MAPPING)
#define IS_NOT_MAPPING(x)       ((x).type != T_MAPPING)

#define IS_PROCEDURE(x)         ((x).type == T_BIF ||                      \
			 	 (x).type == T_LAMBDA ||                   \
				 (x).type == T_CONTINUATION)
#define IS_NOT_PROCEDURE(x)     ((x).type != T_BIF &&                      \
				 (x).type != T_LAMBDA &&                   \
				 (x).type != T_CONTINUATION)

#define IS_BOOLEAN(x)           ((x).type == T_TRUE || (x).type == T_FALSE)
#define IS_NOT_BOOLEAN(x)       ((x).type != T_TRUE && (x).type != T_FALSE)

#define CAR(p)                  ((p)->car)
#define CDR(p)                  ((p)->cdr)

#define ABS(n)   ((n) <  0  ?-(n) : (n))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#endif /* __TYPES_H__ */
