/* svalue.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements svalue operations.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME "svalue"

#include "types.h"

#include "err.h"
#include "big.h"
#include "str.h"

char *svalue_describe(INT type)
{
  switch(type)
  {
  case T_UNDEFINED:     return "#undefined";
  case T_REAL:	        return "float";
  case T_SMALL_INTEGER: return "integer";
  case T_BIG_INTEGER:   return "big-integer";
  case T_LABEL:         return "#label";
  case T_NIL:           return "nil";
  case T_TRUE:          return "true";
  case T_FALSE:         return "false";
  case T_BIF:           return "#bif";
  case T_CHAR:          return "character";
  case T_FREE:          return "#free";
  case T_PORT:          return "port";
  case T_VECTOR:        return "vector";
  case T_MAPPING:       return "mapping";
  case T_PAIR:          return "pair";
  case T_SYMBOL:        return "symbol";
  case T_STRING:        return "string";
  case T_LAMBDA:        return "#lambda";
  case T_CONTINUATION:  return "#continuation";
    
  default:
    err_fatal("svalue_describe: Unknown type (%d).", type);
  }
  
  return 0;
}

INT svalue_hash(struct svalue *value)
{
  switch(value->type)
  {
  case T_CHAR:
  case T_SMALL_INTEGER:
    return value->u.integer;

  case T_STRING:
  case T_SYMBOL:
    return str_hash(value->u.str);

  case T_PAIR:
    return 0;   /* FIXME: Bad, but... */
    
  case T_TRUE:
  case T_FALSE:
  case T_REAL:
  case T_BIG_INTEGER:
    return 0;   /* FIXME: Bad, but... */
    
  default:
    err_fatal("svalue_hash: Unknown type (%d).", value->type);
  }
  
  return 0;
}

#define EQ(_type, _u)                                                      \
        case _type: if(a->_u == b->_u) return 1; return 0

INT svalue_eq(struct svalue *a, struct svalue *b)
{
  if(a->type == b->type)
    switch(a->type)
    {
      EQ(T_BIF,     u.bif);
      EQ(T_SMALL_INTEGER, u.integer);
      EQ(T_CHAR,    u.integer);
      EQ(T_LAMBDA,  u.pair);
      EQ(T_MAPPING, u.map);
      EQ(T_PAIR,    u.pair);
      EQ(T_REAL,    u.real);
      EQ(T_SYMBOL,  u.str);
      EQ(T_STRING,  u.str);
      EQ(T_VECTOR,  u.vec);
      
#ifdef USE_BIG_INTEGERS
    case T_BIG_INTEGER:
      return big_equal_integer(a->u.big, b->u.big);
#endif /* USE_BIG_INTEGERS */
      
    case T_NIL:
    case T_TRUE:
    case T_FALSE:
    case T_UNDEFINED:
      return 1;
      
    default:
      err_fatal("svalue_equal: Unknown type (%d).", a->type);
    }
  
  return 0;
}

INT svalue_compare(struct svalue *a, struct svalue *b)
{
  if(a->type != b->type)
    return a->type < b->type ? -1 : 1;

  if(svalue_eq(a, b))
    return 0;
  
  return 0;   /* FIXME. */
}
