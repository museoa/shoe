/* lexer.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME "lexer"

#include "lexer.h"

#include "err.h"
#include "mat.h"
#include "mem.h"
#include "big.h"
#include "str.h"
#include "vec.h"

#include "process.h"

#define WHITESPACE   ' ': case '\t': case '\n': case '\r'
#define ONECHARACTER '(': case ')':  case '\'': case '`': case ',': case '@'
#define DIGIT        '0': case '1': case '2': case '3': case '4': \
                     case '5': case '6': case '7': case '8': case '9'
#define ESCAPE       '\\'
#define STRING       '"'

#define ADD_TOKEN(_type, _u, _value)                                       \
        do {                                                               \
          if(vec)                                                          \
          {                                                                \
	    vec->v[p].type = _type;                                        \
	    vec->v[p++]._u = _value;                                       \
	    vec->v[p].type = T_SMALL_INTEGER;                              \
	    vec->v[p++].u.integer = t_row;                                 \
	    vec->v[p].type = T_SMALL_INTEGER;                              \
	    vec->v[p++].u.integer = t_col;                                 \
          }                                                                \
          else                                                             \
            p++;                                                           \
        } while(0)

#define ADD_SVALUE_TOKEN(_svalue)                                          \
        do {                                                               \
          if(vec)                                                          \
          {                                                                \
	    vec->v[p++] = _svalue;                                         \
	    vec->v[p].type = T_SMALL_INTEGER;                              \
	    vec->v[p++].u.integer = t_row;                                 \
	    vec->v[p].type = T_SMALL_INTEGER;                              \
	    vec->v[p++].u.integer = t_col;                                 \
          }                                                                \
          else                                                             \
            p++;                                                           \
        } while(0)

#define STEP                                                               \
        if(*s == '\n')                                                     \
        {                                                                  \
          row++;                                                           \
          col = 0;                                                         \
        }                                                                  \
        else if(*s == '\t')                                                \
          col += 8;                                                        \
        else                                                               \
          col++;                                                           \
        s++

INT try_make_sharp(char *s, INT length, struct svalue *value)
{
  if(length < 1 || *s != '#')
    return 0;
  length--, s++;

  /* Check for #t. */
  if(length == 1 && (*s == 't' || *s == 'T'))
  {
    value->type = T_TRUE;
    return 1;
  }
  
  /* Check for #f. */
  if(length == 1 && (*s == 'f' || *s == 'F'))
  {
    value->type = T_FALSE;
    return 1;
  }

  /* Check for character codes. */
  if(length >= 2 && *s == '\\')
  {
    length--, s++;
    if(length == 1)
    {
      value->type = T_CHAR;
      value->u.integer = *s;
      return 1;
    }
    /* FIXME: #\newline, #\space etc. */
  }
  
  return 0;
}

INT lexer(struct process *process, struct str *str, struct vec *vec)
{
  INT p = 0, state = 0, col = 0, row = 1;
  INT t_col = 0, t_row = 0;   /* Set to zero because Gcc is stupid. */
  char *e, *s, *t = 0;   /* Set to zero because Gcc is stupid. */

  struct svalue sharp, number;

  s = str->s;
  e = s + str->length;

  while(s < e)
  {
    
    switch(state)
    {
    case 0:
      /* Whitespace state. */
      switch(*s)
      {
      case WHITESPACE:
	STEP;
	break;

      case ';':
	STEP;
	state = 1;
	break;
	
      case '(':
      case ')':
      case '\'':
      case '`':
      case ',':
	t_row = row;
	t_col = col;
	if(s[1] == '@')
	{
	  ADD_TOKEN(T_SYMBOL, u.str, str_allocate(&process->str_heap, s, 2));
	  STEP;
	}
	else
	{
	  ADD_TOKEN(T_SYMBOL, u.str, str_allocate(&process->str_heap, s, 1));
	}
	STEP;
	break;

      case STRING:
	STEP;   /* Skip `"'. */
	t_row = row;
	t_col = col;
	t = s;
	state = 4;
	break;
	
      default:
	t_row = row;
	t_col = col;
	t = s;
	state = 2;
      }
      break;

      /* Comment state. */
    case 1:
      if(*s == '\n')
	state = 0;
      STEP;
      break;

      /* Identifier and number states. */
    case 2:
      switch(*s)
      {
      case WHITESPACE:
      case '(':
      case ')':
      case ';':
      complete_identifier:
	if(*t == '#' && *s == '(' && s-t == 1)
	{
	  STEP;
	}
	else if(*t == '%' && *s == '(' && s-t == 1)
	{
	  STEP;
	}
	if(mat_make_number(process, t, s-t, &number))
	  ADD_SVALUE_TOKEN(number);
	else if(try_make_sharp(t, s-t, &sharp))
	  ADD_SVALUE_TOKEN(sharp);
	else
	  ADD_TOKEN(T_SYMBOL, u.str, str_allocate(&process->str_heap, t, s-t));
	state = 0;
	break;

      case ESCAPE:
	STEP;
	state = 3;
	break;
	
      default:
	STEP;
      }
      break;

    case 3:
      STEP;
      state = 2;
      break;
      
      /* String states. */
    case 4:
      switch(*s)
      {
      case STRING:
	ADD_TOKEN(T_STRING, u.str,
		  str_allocate_escaped(&process->str_heap, t, s-t));
	state = 0;
	STEP;
	break;

      case ESCAPE:
	STEP;
	state = 5;
	break;

      default:
	STEP;
      }
      break;

    case 5:
      STEP;
      state = 4;
      break;

    default:
      err_fatal("Unknown lexer state %d.", state);
    }
  }

  /* Complete last token. */
  switch(state)
  {
    case 2:
      goto complete_identifier;
      
    case 4:
      err_fatal("End of file in string.");
      
    case 3:
    case 5:
      err_fatal("End of file in quote.");
  }
  
  return p;
}

struct vec* lex_read(struct process *process, struct str *str)
{
  struct vec *vec;
  
  vec = vec_allocate(&process->vec_heap, 3 * lexer(process, str, 0));
  lexer(process, str, vec);
  
  return vec;
}
