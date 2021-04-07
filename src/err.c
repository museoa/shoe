/* err.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Error messages.
 */

#include "types.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif /* HAVE_STDIO_H */
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif /* HAVE_STDARG_H */

#include "big.h"
#include "str.h"
#include "vec.h"
#include "map.h"
#include "mat.h"
#include "process.h"

void err_fatal(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Fatal error -- ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void err_fatal_perror(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Fatal -- ");
  vfprintf(stderr, fmt, ap);
  perror("");
  va_end(ap);
  exit(1);
}

void err_printf(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

void err_debug(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Debug -- ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

void err_debugnn(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Debug -- ");
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

void err_print_binary(char *s, INT length)
{
  char *e = s + length;
  
  while(s < e)
    switch(*(s++))
    {
    case '\0': fprintf(stderr, "\\0");  break;
    case '\t': fprintf(stderr, "\\t");  break;
    case '\n': fprintf(stderr, "\\n");  break;
    case '\r': fprintf(stderr, "\\r");  break;
    case '\\': fprintf(stderr, "\\\\"); break;
      
    default:
      fprintf(stderr, "%c", *(s-1));
    }
}

void err_print_string(struct str *str)
{
  char *e, *s;

  s = str->s;
  e = s + str->length;
  
#if 0
  err_printf("\"");
#endif
  while(s < e)
    switch(*(s++))
    {
#if 0
    case '\0': fprintf(stderr, "\\0");  break;
    case '\t': fprintf(stderr, "\\t");  break;
    case '\n': fprintf(stderr, "\\n");  break;
    case '\r': fprintf(stderr, "\\r");  break;
    case '\"': fprintf(stderr, "\\\""); break;
    case '\\': fprintf(stderr, "\\\\"); break;
#endif 
    default:
      fprintf(stderr, "%c", *(s-1));
    }
#if 0
    err_printf("\"");
#endif 
}

void err_print_symbol(struct str *str)
{
  char *e, *s;

  s = str->s;
  e = s + str->length;
  
  while(s < e)
    switch(*(s++))
    {
    case '\0': fprintf(stderr, "\\0");  break;
    case '\t': fprintf(stderr, "\\t");  break;
    case '\n': fprintf(stderr, "\\n");  break;
    case '\r': fprintf(stderr, "\\r");  break;
    case '\\': fprintf(stderr, "\\\\"); break;
    case '\"': fprintf(stderr, "\\\""); break;
    case '(':  fprintf(stderr, "\\(");  break;
    case ')':  fprintf(stderr, "\\)");  break;
    case '#':  fprintf(stderr, "\\#");  break;
      
    default:
      fprintf(stderr, "%c", *(s-1));
    }
  /* fprintf(stderr, "[%08p]", str); */
}

/*
 * Extended error messages.
 */
void err_fatal_line(INT line_number, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "%s:%d: Fatal -- ", "<unknown>", line_number);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void err_fatal_file_line(char *filename, INT line_number, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "%s:%d: Fatal -- ", filename, line_number);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void err_fatal_internal(char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Fatal internal errror -- ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

void err_debug_line(INT line_number, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "%s:%d: Debug -- ", "<unknown>", line_number);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

void err_runtime_file_line(char *filename, INT line_number, char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "%s:%d: ", filename, line_number);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

void err_display(struct process *process, struct svalue *value)
{
  INT i, n;
  
  switch(value->type)
  {
  case T_UNDEFINED:
    err_printf("#undefined");
    break;
    
  case T_SMALL_INTEGER:
    err_printf("%d", value->u.integer);
    break;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    err_printf("%s", big_integer_to_string(&process->str_heap,
					   value->u.big, 10)->s);
    break;
#endif /* USE_BIG_INTEGERS */
    
  case T_REAL:
    err_printf("%s", mat_number_to_string(process, value, 10)->s);
    break;
    
  case T_NIL:
    err_printf("()");
    break;
    
  case T_TRUE:
    err_printf("#t");
    break;
    
  case T_FALSE:
    err_printf("#f");
    break;
    
  case T_LABEL:
    err_printf("#label");
    break;
    
  case T_LAMBDA:
    /* err_printf("#lambda%d", CAR(value->u.pair).u.label); */
    err_printf("#lambda");
    break;
    
  case T_CONTINUATION:
    err_printf("#continuation");
    break;
    
  case T_BIF:
    err_printf("#bif");
    break;
    
  case T_SYMBOL:
    err_print_symbol(value->u.str);
    break;
    
  case T_STRING:
    err_print_string(value->u.str);
    break;
    
  case T_CHAR:
    switch(value->u.integer)
    {
    case ' ':
      err_printf("#\\space");
      break;
    case '\n':
      err_printf("#\\newline");
      break;
    default:
      err_printf("#\\%c", value->u.integer);
    }
    break;
    
  case T_PAIR:
    err_printf("(");
    
    err_display(process, &CAR(value->u.pair));
    
    value = &CDR(value->u.pair);
    while(IS_PAIR(*value))
    {
      err_printf(" ");
      err_display(process, &CAR(value->u.pair));
      value = &CDR(value->u.pair);
    }
    
    if(IS_NOT_NIL(*value))
    {
      err_printf(" . ");
      err_display(process, value);
    }
    
    err_printf(")");
    break;
    
  case T_VECTOR:
    err_printf("#(");

    n = value->u.vec->length;
    for(i = 0; i < n; i++)
    {
      if(i)
	err_printf(" ");
      err_display(process, &value->u.vec->v[i]);
    }
    
    err_printf(")");
    break;
    
  case T_MAPPING:
  {
    struct map_entry *entry;
    struct map_table *table;
    struct map *map;

    map = value->u.map;
    table = map->table;

    if(!map->used)
    {
      err_printf("%()");
      break;
    }
    
    err_printf(map->used == 1 ? "%( ;; %d element." : "%( ;; %d elements.",
	       map->used);
    
    for(i = 0; i < map->hash_size; i++)
	for(entry = table->hash[i]; entry; entry = entry->next)
	{
	  err_printf("\n  ");
	  err_display(process, &entry->key);
	  err_printf(" : ");
	  err_display(process, &entry->value);
	}
    
    err_printf(")");
    break;
  }

  case T_FREE:
    err_fatal("Trying to display a FREED type (garb failure)!");
  
  default:
    if(value->type >= 0)
      err_fatal("Unknown display type = %d", value->type);

    err_printf("#garb");
  }
}
