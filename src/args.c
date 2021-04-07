/* args.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This module takes care of argument checks.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME  "args"

#include "types.h"

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif /* HAVE_STDARG_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif /* HAVE_STDIO_H */

#include "process.h"
#include "svalue.h"

#include "args.h"
#include "err.h"
#include "vec.h"

#define MAX_ERROR_BUFFER 4096 /* FIXME: Dynamically allocated. */

INT args_error(struct process *process, char *name, char *fmt, ...)
{
  char buffer[MAX_ERROR_BUFFER];
  va_list ap;

  sprintf(buffer, "%s -- ", name);
  
  va_start(ap, fmt);
  vsprintf(buffer + strlen(buffer), fmt, ap);
  va_end(ap);

  process->error.type = T_STRING;
  process->error.u.str = str_allocate_text(&process->str_heap, buffer);
  
  return 0;
}

#define CHECK_TYPE(arg, t)                                                 \
     if(arg && !clr && (arg)->type != (t))                                 \
       return args_error(process, name,                                    \
                         "Argument %d expected %s not %s.",                \
			 n+1, svalue_describe(t), svalue_describe((arg)->type))

#define CHECK_MULTITYPE(arg, t, tdesc)                                     \
     if(arg && !clr && !t(*(arg)))                                         \
       return args_error(process, name,                                    \
                         "Argument %d expected %s not %s.",                \
			 n+1, tdesc, svalue_describe((arg)->type))

#define CHECK_WIDE(arg, t)                                                 \
     if(arg && !clr && ((arg)->type == (t) && (arg)->u.str->shift))        \
       return args_error(process, name,                                    \
                         "Argument %d expected %s not widestring.",        \
			 n+1, svalue_describe(t))

#define ASSIGN(val)        (clr || !arg ? 0 : (val))

static INT va_args_get(struct process *process, char *name,
		       struct svalue *args, char *fmt, va_list ap)
{
  INT n = 0, opt = 0, clr = 0;
  struct svalue *arg = 0;
  struct pair *p;

  if(IS_NIL(*args) && !*fmt)
    return 1;
  if(!*fmt)
    return args_error(process, name, "Expected zero number of arguments.");
  if(*fmt == '?')
    opt = 1;
  if(IS_NOT_PAIR(*args) && !opt)
    return args_error(process, name, "Too few arguments.");
  
  for(p = (opt ? 0 : args->u.pair); *fmt; n++)
  {
    switch(*fmt++)
    {
    case '?':
      opt = 1;
      continue;
      
    case '%':
      break;
      
    default:
      err_fatal("Unknown format specification '%c' to args_get.", *--fmt);
    }

    if(p)
      arg = &CAR(p);

    switch(*fmt++)
    {
    case '*':   /* Svalue. */
      *va_arg(ap, struct svalue **) = ASSIGN(arg);
      break;
      
    case 'i':   /* Small integer. */
      CHECK_TYPE(arg, T_SMALL_INTEGER);
      *va_arg(ap, INT *) = ASSIGN(arg->u.integer);
      break;
      
    case 'f':   /* Float. */
      CHECK_TYPE(arg, T_REAL);
      *va_arg(ap, REAL *) = ASSIGN(arg->u.real);
      break;
      
    case 'r':   /* Real. */
      CHECK_MULTITYPE(arg, IS_REAL, "number");
      *va_arg(ap, struct svalue **) = ASSIGN(arg);
      break;
      
    case 'I':   /* Generic integer. */
      CHECK_MULTITYPE(arg, IS_INTEGER, "integer");
      *va_arg(ap, struct svalue **) = ASSIGN(arg);
      break;
      
    case 'c':   /* Character. */
      CHECK_TYPE(arg, T_CHAR);
      *va_arg(ap, INT *) = ASSIGN(arg->u.integer);
      break;
      
    case 'p':   /* Pair. */
      CHECK_TYPE(arg, T_PAIR);
      *va_arg(ap, struct pair **) = ASSIGN(arg->u.pair);
      break;
      
    case 's':   /* String. */
      CHECK_WIDE(arg, T_STRING);
    case 'w':
      CHECK_TYPE(arg, T_STRING);
      *va_arg(ap, struct str **) = ASSIGN(arg->u.str);
      break;
      
    case 'S':   /* Symbol. */
      CHECK_WIDE(arg, T_SYMBOL);
    case 'W':
      CHECK_TYPE(arg, T_SYMBOL);
      *va_arg(ap, struct str **) = ASSIGN(arg->u.str);
      break;
      
    case 'v':   /* Vector. */
      CHECK_TYPE(arg, T_VECTOR);
      *va_arg(ap, struct vec **) = ASSIGN(arg->u.vec);
      break;
      
    case 'm':   /* Mapping. */
      CHECK_TYPE(arg, T_MAPPING);
      *va_arg(ap, struct map **) = ASSIGN(arg->u.map);
      break;
      
    default:
      err_fatal("Unknown character '%c' int args_get format string.", *--fmt);
    }

    if(!p || IS_NOT_PAIR(CDR(p)))
    {
      if(opt || *fmt == '?')
	clr = opt = 1;
      else
	return *fmt ? args_error(process, name, "Too few arguments.") : 1;
    }
    else
      p = CDR(p).u.pair;
  }

  return clr ? 1 : args_error(process, name, "Too many arguments.");
}

INT args_get(struct process *process, char *name,
	     struct svalue *args, char *fmt, ...)
{
  va_list ap;
  INT r;
  
  va_start(ap, fmt);
  r = va_args_get(process, name, args, fmt, ap);
  va_end(ap);
  
  return r;
}
