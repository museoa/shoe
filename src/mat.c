/* mat.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements the built-in mathematical functions (BIF:s).
 */

#include "types.h"

#ifdef HAVE_MATH_H
#include <math.h>
#endif /* HAVE_MATH_H */
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif /* HAVE_STDIO_H */

#include "port.h"

#include "process.h"
#include "args.h"
#include "bif.h"
#include "err.h"
#include "mat.h"
#include "mem.h"
#include "str.h"

INT mat_character_is_within_range(INT c, INT base)
{
  return (('0' <= c && c < '0'+MIN(base, 10)) ||
	  ('a' <= c && c < 'a'+base-10) ||
	  ('A' <= c && c < 'A'+base-10));
}

INT mat_character_to_digit(INT c)
{
  if('0' <= c && c <= '9')
    return c - '0';
  
  if('a' <= c && c <= 'f')
    return c - 'a' + 10;
  
  if('A' <= c && c <= 'F')
    return c - 'A' + 10;

  if(c == '#')
    /* This funky sharp is used in the floating point notation. */
    return 0;

  err_fatal("mat_digit_to_character: Strange character (%c).", c);
  
  return 0;
}

char mat_digit_to_character(INT d)
{
  if(0 <= d && d <= 9)
    return '0' + d;
  
  if(10 <= d && d <= 15)
    return 'a' + d - 10;
  
  err_fatal("mat_digit_to_character: Strange digit (%c).", d);
  
  return 0;
}

INT mat_make_integer(struct process *process,
		     char *s, INT length, struct svalue *val)
{
  INT overflow;
  INT base = 10;
  INT sign = 1;
  INT i = 0;
  char *t;

  val->type = T_UNDEFINED;
  
  /* Check for prefix specifications. */
  while(length >= 2  && *s == '#')
  {
    length--, s++;
    switch(*s)
    {
      /* We only support exact numbers here. */
    case 'e': case 'E': break;

      /* Base. */
    case 'b': case 'B': base =  2; break;
    case 'o': case 'O': base =  8; break;
    case 'd': case 'D': base = 10; break;
    case 'x': case 'X': base = 16; break;
    default:
      return 0;
    }
    length--, s++;
  }
  
  /* Check for signs of a sign. */
  if(length >= 1 && (*s == '-' || *s == '+'))
  {
    if(*s == '-')
      sign = -1;
    length--, s++;
  }

  /* Check that there is space for all the digits... */
  if(length <= 0)
    return 0;
  
  /* Read all digits. */
  for(t = s; length > 0; length--, s++)
    if(mat_character_is_within_range(*s, base))
    {
#ifdef USE_BIG_INTEGERS
      if(!MAT_MULTIPLY(i, i, base, overflow))
      {
	/* Try big integers instead. */
	char *m;
	
	m = mem_duplicate(t, length+1 + (s-t));
	m[length + (s-t)] = '\0';
	
	val->u.big = big_allocate_integer_text(&process->big_heap, m, base);
	
	mem_free(m);

	if(!val->u.big)
	  return 0;
	
	if(sign < 0)
	  mpz_neg(val->u.big->u.integer, val->u.big->u.integer);

	if(IS_REDUCABLE_INTEGER(val->u.big, i))
	{
	  val->u.integer = i;
	  val->type = T_SMALL_INTEGER;
	}
	else
	  val->type = T_BIG_INTEGER;
	
	return 1;
      } else
	i += mat_character_to_digit(*s);
#else
      i = i*base + mat_character_to_digit(*s);
#endif /* USE_BIG_INTEGERS */
    } else
      return 0;

  /* It's "safe" to convert something positive to
     something negative, but not the other way around. */
  val->u.integer = i * sign;
  val->type = T_SMALL_INTEGER;
  
  return 1;
}

INT mat_make_float(struct process *process,
		   char *s, INT length, struct svalue *val)
{
  INT got_mantissa = 0, got_dot = 0;
  INT exponent = 0;
  INT base = 10;
  INT sign = 1;
  
  double d = 0.0;
  
  val->type = T_UNDEFINED;
  
  /* Check for prefix specifications. */
  while(length >= 2  && *s == '#')
  {
    length--, s++;
    switch(*s)
    {
      /* We only support inexact numbers here. */
    case 'i': case 'I': break;

      /* Base. */
    case 'b': case 'B': base =  2; break;
    case 'o': case 'O': base =  8; break;
    case 'd': case 'D': base = 10; break;
    case 'x': case 'X': base = 16; break;
    default:
      return 0;
    }
    length--, s++;
  }
  
  /* Check for signs of a sign. */
  if(length >= 1 && (*s == '-' || *s == '+'))
  {
    if(*s == '-')
      sign = -1;
    length--, s++;
  }

  /* Read mantissa. */
  for(; length > 0; length--, s++)
    if((mat_character_is_within_range(*s, base) ||
	(got_mantissa && *s == '#')) &&
       !(length > 1 && (s[1] == '+' || s[1] == '-')))
    {
      got_mantissa = 1;

      /* FIXME: Does this work??  Nope, it does not... */
      if(d*base != d*base)
	exponent++;
      else
	d = d*base + mat_character_to_digit(*s);

      if(got_dot)
	exponent--;
    }
    else if(!got_dot && *s == '.')
      got_dot = 1;
    else
      break;

  if(!got_mantissa)
    return 0;

  /* Check for any exponent. */
  if(length >= 1 && (*s == 'e' || *s == 'E' ||
		     *s == 's' || *s == 'S' ||
		     *s == 'f' || *s == 'F' ||
		     *s == 'd' || *s == 'D' ||
		     *s == 'l' || *s == 'L'))
  {
    INT e = 0, esign = 1, got_exp = 0, overflow;

    s++, length--;   /* Consume exponent token. */
    
    /* Check for signs of a sign. */
    if(length >= 1 && (*s == '-' || *s == '+'))
    {
      if(*s == '-')
	esign = -1;
      length--, s++;
    }

    for(; length > 0; length--, s++)
      if(mat_character_is_within_range(*s, base) || *s == '#')
      {
	if(!MAT_MULTIPLY(e, e, base, overflow))
	  return 0;   /* Jeez, exponent overflow. */
	
	got_exp = 1;
	e += mat_character_to_digit(*s);
      }
      else
	return 0;

    if(!got_exp)
      return 0;
    
    e *= esign;
    if(!MAT_ADD(exponent, exponent, e, overflow))
      return 0;
  }

  /* Everything should be consumed by now. */
  if(length)
    return 0;

  /* Compose the actual floating point number. */
  if(d != 0.0)
    d *= pow((double)base, (double)exponent);

  /* FIXME: Check overflow? */

  val->u.real = (REAL)(sign*d);
  val->type = T_REAL;
  
  return 1;
}

INT mat_make_number(struct process *process,
		    char *s, INT length, struct svalue *val)
{
  return mat_make_integer(process, s, length, val) ||
         mat_make_float(process, s, length, val);
}

struct str *mat_number_to_string(struct process *process, struct svalue *val,
				 INT base)
{
  char buffer[256], *p; /* This makes us able to do 254 bit binary numbers. */
  struct str *raw;
  INT i, neg = 0;
  
  switch(val->type)
  {
  case T_SMALL_INTEGER:
    i = val->u.integer;
    
    if(i < 0)
    {
      neg = 1;
      i = -i;
    }
    
    for(p = buffer; i || p == buffer; i /= base)
      *p++ = mat_digit_to_character(i % base);

    if(neg)
      *p++ = '-';
      
    raw = str_allocate_raw(p-buffer);

    /* Copy backwards. */
    for(i = 0; i < raw->length; i++)
      raw->s[i] = *--p;
    
    return str_commit_raw(&process->str_heap, raw);

#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    return big_integer_to_string(&process->str_heap, val->u.big, base);
#endif /* USE_BIG_INTEGERS */

  case T_REAL:
    /* Maybe a bit heuristical, but it seems to work... */
    sprintf(buffer, "%g", (double)val->u.real);

    /* Check if we have a dot, exponent or something else
       that makes us recognize this is a floating point number.*/
    for(i = 0, p = buffer; *p; p++)
      if((*p < '0' || '9' < *p) && *p != '-')
	i = 1;
    
    if(!i)
    {
      /* Add a dot if necessary. */
      *p++ = '.';
      *p++ = '0';
      *p++ = '\0';
    }
    
    return str_allocate_text(&process->str_heap, buffer);

  default:
    err_fatal("mat_number_to_string: Unknown type (%d).", val->type);
  }
  
  return 0;   /* Makes gcc happy. */
}

BIF_DECLARE(bif_number_to_string)
{
  struct svalue *val;
  struct str *str;
  INT base;

  ARGS_GET((process, "number->string", args, "%*?%i", &val, &base));

  if(!base)
    base = 10;
  else if(base < 2 || 16 < base)
    err_fatal("number->string: Base %d is out of range 2 - 16.", base);
  
  str = mat_number_to_string(process, val, base);
  
  BIF_RESULT_STRING(str);
}

BIF_DECLARE(bif_less)
{
  INT n, a, b = 0;   /* Set to zero because Gcc is stupid. */
  struct pair *p;

  BIF_RESULT_TRUE();

  if(IS_NOT_PAIR(*args))
    return;

  p = args->u.pair;
  
  for(n = 0; ; n++)
  {
    if(IS_NOT_SMALL_INTEGER(CAR(p)))
      err_fatal("< on non-integers.");

    a = CAR(p).u.integer;
    if(n) {
      if(a <= b) {
	BIF_RESULT_FALSE();
	return;
      }
    } else
      b = a;
    
    if(IS_NOT_PAIR(CDR(p)))
      return;
    p = CDR(p).u.pair;
  }
}

/*
 * Predicates.
 */

BIF_DECLARE(bif_zerop)
{
  struct svalue *val;

  ARGS_GET((process, "zero?", args, "%r", &val));

  BIF_RESULT_FALSE();
  switch(val->type)
  {
  case T_SMALL_INTEGER:
    if(val->u.integer == 0)
      BIF_RESULT_TRUE();
    break;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    /* Cannot be zero. */
    break;
#endif /* USE_BIG_INTEGERS */
    
  case T_REAL:
    if(val->u.real == 0.0)
      BIF_RESULT_TRUE();
    break;
    
  default:
    err_fatal("zero?: Unknown type of number (%d).", val->type);
  }
}     

BIF_DECLARE(bif_positivep)
{
  struct svalue *val;

  ARGS_GET((process, "positive?", args, "%r", &val));

  BIF_RESULT_FALSE();
  switch(val->type)
  {
  case T_SMALL_INTEGER:
    if(val->u.integer >= 0) BIF_RESULT_TRUE();
    break;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    if(mpz_sgn(val->u.big->u.integer) >= 0)
      BIF_RESULT_TRUE();
    break;
#endif /* USE_BIG_INTEGERS */
    
  case T_REAL:
    if(val->u.real >= 0.0)
      BIF_RESULT_TRUE();
    break;
    
  default:
    err_fatal("positive?: Unknown type of number (%d).", val->type);
  }
}     

BIF_DECLARE(bif_negativep)
{
  struct svalue *val;

  ARGS_GET((process, "negative?", args, "%r", &val));

  BIF_RESULT_FALSE();
  switch(val->type)
  {
  case T_SMALL_INTEGER:
    if(val->u.integer < 0)
      BIF_RESULT_TRUE();
    break;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    if(mpz_sgn(val->u.big->u.integer) < 0)
      BIF_RESULT_TRUE();
    break;
#endif /* USE_BIG_INTEGERS */
    
  case T_REAL:
    if(val->u.real < 0.0)
      BIF_RESULT_TRUE();
    break;
    
  default:
    err_fatal("negative?: Unknown type of number (%d).", val->type);
  }
}     

#define MAT_FLOAT_MODULO(f, m) ((f) - (m) * floor((f) / (m)))

BIF_DECLARE(bif_is_odd)
{
  struct svalue *val;

  ARGS_GET((process, "odd?", args, "%r", &val));

  BIF_RESULT_FALSE();
  switch(val->type)
  {
  case T_SMALL_INTEGER:
    if(val->u.integer & 1)
      BIF_RESULT_TRUE();
    break;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    if(mpz_scan1(val->u.big->u.integer, 0) == 0)
      BIF_RESULT_TRUE();
    break;
#endif /* USE_BIG_INTEGERS */
    
  case T_REAL:
    if(MAT_FLOAT_MODULO(val->u.real, 2.0)==1.0)
      BIF_RESULT_TRUE();
    break;
    
  default:
    err_fatal("odd?: Unknown type of number (%d).", val->type);
  }
}     

BIF_DECLARE(bif_is_even)
{
  struct svalue *val;

  ARGS_GET((process, "even?", args, "%r", &val));

  BIF_RESULT_FALSE();
  switch(val->type)
  {
  case T_SMALL_INTEGER:
    if(!(val->u.integer & 1))
      BIF_RESULT_TRUE();
    break;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    if(mpz_scan0(val->u.big->u.integer, 0) == 0)
      BIF_RESULT_TRUE();
    break;
#endif /* USE_BIG_INTEGERS */
    
  case T_REAL:
    if(MAT_FLOAT_MODULO(val->u.real, 2.0)==0.0)
      BIF_RESULT_TRUE();
    break;
    
  default:
    err_fatal("even?: Unknown type of number (%d).", val->type);
  }
}     

BIF_DECLARE(bif_abs)
{
  struct svalue *val;
  struct big *big;
  INT i;

  ARGS_GET((process, "abs", args, "%r", &val));

  switch(val->type)
  {
  case T_SMALL_INTEGER:
    i = val->u.integer;
    
    if(i >= 0)
    {
      BIF_RESULT_SMALL_INTEGER(i);
      return;
    }

#ifdef USE_BIG_INTEGERS
    if(MAT_NEG(i, i))
    {
      BIF_RESULT_SMALL_INTEGER(i);
      return;
    }

    big = big_allocate_integer(&process->big_heap, i);
    mpz_neg(big->u.integer, big->u.integer);
    BIF_RESULT_REDUCED_INTEGER(big, i);
#else
    BIF_RESULT_SMALL_INTEGER(-i);
#endif /* USE_BIG_INTEGERS */
    return;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    big = big_copy_integer(&process->big_heap, val->u.big);
    mpz_abs(big->u.integer, big->u.integer);
    BIF_RESULT_REDUCED_INTEGER(big, i);
    return;
#endif /* USE_BIG_INTEGERS */
    
  case T_REAL:
    BIF_RESULT_REAL(fabs(val->u.real));
    return;
    
  default:
    err_fatal("abs: Unknown type of number (%d).", val->type);
  }
}     

#define BIF_FLOAT_OPERATION(bif_name, name, f)                             \
        BIF_DECLARE(bif_name)                                              \
        {                                                                  \
          struct svalue *val;                                              \
                                                                           \
          ARGS_GET((process, name, args, "%r", &val));                     \
                                                                           \
          if(val->type == T_REAL)                                          \
          {                                                                \
            BIF_RESULT_REAL(f);                                            \
            return;                                                        \
          }                                                                \
                                                                           \
          *result = *val;                                                  \
        }

#define MAT_TRUNCATE(x) (((x) > 0.0 ? floor : ceil)(x))
#define MAT_ROUND(x)    MAT_TRUNCATE((x)+((x) > 0.0 ? 0.5 : -0.5))

BIF_FLOAT_OPERATION(bif_floor   , "floor",    floor((double)val->u.real))
BIF_FLOAT_OPERATION(bif_ceiling,  "ceiling",  ceil((double)val->u.real))
BIF_FLOAT_OPERATION(bif_truncate, "truncate", MAT_TRUNCATE(val->u.real))
BIF_FLOAT_OPERATION(bif_round,    "round",    MAT_ROUND(val->u.real))

BIF_DECLARE(bif_exact_to_inexact)
{
  struct svalue *val;

  ARGS_GET((process, "exact->inexact", args, "%I", &val));

  switch(val->type)
  {
  case T_SMALL_INTEGER:
    BIF_RESULT_REAL((REAL)val->u.integer);
    return;
    
#ifdef USE_BIG_INTEGERS
  case T_BIG_INTEGER:
    BIF_RESULT_REAL((REAL)big_integer_to_float(val->u.big));
    return;
#endif /* USE_BIG_INTEGERS */
    
  default:
    err_fatal("exact->inexact: Unknown type of number (%d).", val->type);
  }
}

BIF_DECLARE(bif_inexact_to_exact)
{
  struct big *big;
  REAL f;
  INT i;

  ARGS_GET((process, "inexact->exact", args, "%f", &f));

#ifdef USE_BIG_INTEGERS
  if(fabs(f) <= MAXINT)
    BIF_RESULT_SMALL_INTEGER((INT)f);
  else
  {
    big = big_float_to_integer(&process->big_heap, f);
    BIF_RESULT_REDUCED_INTEGER(big, i);
  }
#else
  BIF_RESULT_SMALL_INTEGER((INT)f);
#endif /* USE_BIG_INTEGERS */
}

/*
 * Addition.
 */

BIF_DECLARE(bif_plus)
{
  INT i = 0, use_float = 0;
  struct svalue *val;
  struct pair *p;
  REAL f = 0.0;

#ifdef USE_BIG_INTEGERS
  INT use_big = 0, overflow;
  struct big *big = 0;
#endif /* USE_BIG_INTEGERS */
  
  if(IS_NIL(*args))
  {
    BIF_RESULT_SMALL_INTEGER(i);
    return;
  }

  p = args->u.pair;
  for(;;)
  {
    val = &CAR(p);
    
#ifdef USE_BIG_INTEGERS
  allocate_big:
    if(use_big && !big)
      big = big_allocate_integer(&process->big_heap, i);
#endif /* USE_BIG_INTEGERS */
    
    switch(val->type)
    {
    case T_SMALL_INTEGER:
      if(use_float)
	f += (REAL)val->u.integer;
#ifdef USE_BIG_INTEGERS
      else if(use_big)
      {
	if(val->u.integer > 0)
	  mpz_add_ui(big->u.integer, big->u.integer, val->u.integer);
	else if(val->u.integer < 0)
	  mpz_sub_ui(big->u.integer, big->u.integer, -val->u.integer);
      }
      else if(!MAT_ADD(i, i, val->u.integer, overflow))
      {
	use_big = 1;
	goto allocate_big;
      }
#else
      else
	i += val->u.integer;
#endif /* USE_BIG_INTEGERS */
      break;
    
#ifdef USE_BIG_INTEGERS
    case T_BIG_INTEGER:
      if(use_float)
	f += big_integer_to_float(val->u.big);
      else if(use_big)
	mpz_add(big->u.integer, big->u.integer, val->u.big->u.integer);
      else
      {
	use_big = 1;
	goto allocate_big;
      }
      break;
#endif /* USE_BIG_INTEGERS */

    case T_REAL:
      if(!use_float)
      {
#ifdef USE_BIG_INTEGERS
	if(use_big)
	  f = big_integer_to_float(big);
	else 
#endif /* USE_BIG_INTEGERS */
	  f = (REAL)i;
	
	use_float = 1;
      }

      f += val->u.real;
      break;
      
    default:
      err_fatal("+ on non-numbers.");
    }

    if(IS_NOT_PAIR(CDR(p)))
      break;
    p = CDR(p).u.pair;
  }

  if(use_float)
    BIF_RESULT_REAL(f);
#ifdef USE_BIG_INTEGERS
  else if(use_big)
    BIF_RESULT_REDUCED_INTEGER(big, i);
#endif /* USE_BIG_INTEGERS */
  else
    BIF_RESULT_SMALL_INTEGER(i);
}

/*
 * Subtraction.
 */

BIF_DECLARE(bif_minus)
{
  INT i = 0, k, use_float = 0;
  struct svalue *val;
  struct pair *p;
  REAL f = 0.0;

#ifdef USE_BIG_INTEGERS
  INT use_big = 0, overflow;
  struct big *big = 0;
#endif /* USE_BIG_INTEGERS */
  
  if(IS_NIL(*args))
  {
    BIF_RESULT_SMALL_INTEGER(i);
    return;
  }

  p = args->u.pair;
  for(k = 0; ; k++)
  {
    val = &CAR(p);

    if(k == 1)
    {
      if(use_float)
	f = -f;
#ifdef USE_BIG_INTEGERS
      else if(use_big)
	mpz_neg(big->u.integer, big->u.integer);
      else if(!MAT_NEG(i, i))
      {
	big = big_allocate_integer(&process->big_heap, i);
	mpz_neg(big->u.integer, big->u.integer);
	use_big = 1;
      }
#else
      else
	i = -i;
#endif /* USE_BIG_INTEGERS */
    }
    
#ifdef USE_BIG_INTEGERS
  allocate_big:
    if(use_big && !big)
      big = big_allocate_integer(&process->big_heap, i);
#endif /* USE_BIG_INTEGERS */
    
    switch(val->type)
    {
    case T_SMALL_INTEGER:
      if(use_float)
	f -= (REAL)val->u.integer;
#ifdef USE_BIG_INTEGERS
      else if(use_big)
      {
	if(val->u.integer > 0)
	  mpz_sub_ui(big->u.integer, big->u.integer, val->u.integer);
	else if(val->u.integer < 0)
	  mpz_add_ui(big->u.integer, big->u.integer, -val->u.integer);
      }
      else if(!MAT_SUB(i, i, val->u.integer, overflow))
      {
	use_big = 1;
	goto allocate_big;
      }
#else
      else
	i -= val->u.integer;
#endif /* USE_BIG_INTEGERS */
      break;
    
#ifdef USE_BIG_INTEGERS
    case T_BIG_INTEGER:
      if(use_float)
	f -= big_integer_to_float(val->u.big);
      else if(use_big)
	mpz_sub(big->u.integer, big->u.integer, val->u.big->u.integer);
      else
      {
	use_big = 1;
	goto allocate_big;
      }
      break;
#endif /* USE_BIG_INTEGERS */

    case T_REAL:
      if(!use_float)
      {
#ifdef USE_BIG_INTEGERS
	if(use_big)
	  f = big_integer_to_float(big);
	else
#endif /* USE_BIG_INTEGERS */
	  f = (REAL)i;

	use_float = 1;
      }

      f -= val->u.real;
      break;
      
    default:
      err_fatal("- on non-numbers.");
    }

    if(IS_NOT_PAIR(CDR(p)))
      break;
    p = CDR(p).u.pair;
  }

  if(use_float)
    BIF_RESULT_REAL(f);
#ifdef USE_BIG_INTEGERS
  else if(use_big)
    BIF_RESULT_REDUCED_INTEGER(big, i);
#endif /* USE_BIG_INTEGERS */
  else
    BIF_RESULT_SMALL_INTEGER(i);
}

/*
 * Multiplication.
 */

BIF_DECLARE(bif_times)
{
  INT i = 1, use_float = 0;
  struct svalue *val;
  struct pair *p;
  REAL f = 1.0;

#ifdef USE_BIG_INTEGERS
  INT use_big = 0, overflow;
  struct big *big = 0;
#endif /* USE_BIG_INTEGERS */
  
  if(IS_NIL(*args))
  {
    BIF_RESULT_SMALL_INTEGER(i);
    return;
  }

  p = args->u.pair;
  for(;;)
  {
    val = &CAR(p);
    
#ifdef USE_BIG_INTEGERS
  allocate_big:
    if(use_big && !big)
      big = big_allocate_integer(&process->big_heap, i);
#endif /* USE_BIG_INTEGERS */
    
    switch(val->type)
    {
    case T_SMALL_INTEGER:
      if(use_float)
	f *= (REAL)val->u.integer;
#ifdef USE_BIG_INTEGERS
      else if(use_big)
      {
	if(val->u.integer > 0)
	  mpz_mul_ui(big->u.integer, big->u.integer, val->u.integer);
	else if(val->u.integer < 0)
	{
	  mpz_mul_ui(big->u.integer, big->u.integer, -val->u.integer);
	  mpz_neg(big->u.integer, big->u.integer);
	}
	else
	{
	  use_big = 0;
	  big = 0;
	  i = 0;
	}
      }
      else if(!MAT_MULTIPLY(i, i, val->u.integer, overflow))
      {
	use_big = 1;
	goto allocate_big;
      }
#else
      else
	i *= val->u.integer;
#endif /* USE_BIG_INTEGERS */
      break;

#ifdef USE_BIG_INTEGERS
    case T_BIG_INTEGER:
      if(use_float)
	f *= big_integer_to_float(val->u.big);
      else if(use_big)
	mpz_mul(big->u.integer, big->u.integer, val->u.big->u.integer);
      else if(i)
      {
	use_big = 1;
	goto allocate_big;
      }
      break;
#endif /* USE_BIG_INTEGERS */

    case T_REAL:
      if(!use_float)
      {
#ifdef USE_BIG_INTEGERS
	if(use_big)
	  f = big_integer_to_float(big);
	else
#endif /* USE_BIG_INTEGERS */
	  f = (REAL)i;
	
	use_float = 1;
      }

      f *= val->u.real;
      break;
      
    default:
      err_fatal("* on non-numbers.");
    }

    if(IS_NOT_PAIR(CDR(p)))
      break;
    p = CDR(p).u.pair;
  }

  if(use_float)
    BIF_RESULT_REAL(f);
#ifdef USE_BIG_INTEGERS
  else if(use_big)
    BIF_RESULT_REDUCED_INTEGER(big, i);
#endif /* USE_BIG_INTEGERS */
  else
    BIF_RESULT_SMALL_INTEGER(i);
}

/*
 * Division.
 */

BIF_DECLARE(bif_division)
{
  INT i = 1, use_float = 0, overflow;
  struct svalue *val;
  struct pair *p;
  REAL f = 1.0;

#ifdef USE_BIG_INTEGERS
  INT use_big = 0;
  struct big *big = 0;
  mpz_t tmp;
#endif /* USE_BIG_INTEGERS */
  
  if(IS_NIL(*args))
  {
    BIF_RESULT_SMALL_INTEGER(i);
    return;
  }

#ifdef USE_BIG_INTEGERS
  mpz_init(tmp);
#endif /* USE_BIG_INTEGERS */
  
  /* Load the first argument ... */
  p = args->u.pair;
  if(IS_PAIR(CDR(p)))
  {
    val = &CAR(p);
    switch(val->type)
    {
    case T_SMALL_INTEGER:
      i = val->u.integer;
      break;
      
#ifdef USE_BIG_INTEGERS
    case T_BIG_INTEGER:
      big = big_copy_integer(&process->big_heap, val->u.big);
      use_big = 1;
      break;
#endif /* USE_BIG_INTEGERS */
      
    case T_REAL:
      f = val->u.real;
      use_float = 1;
      break;
      
    default:
      err_fatal("/ on non-numbers.");
    }
    
    p = CDR(p).u.pair;
  }

  /* ... and compute the rest below. */
    for(;;)
    {
      val = &CAR(p);

    change_type:
#ifdef USE_BIG_INTEGERS
      if(use_big && !big)
	big = big_allocate_integer(&process->big_heap, i);
#endif /* USE_BIG_INTEGERS */

      switch(val->type)
      {
      case T_SMALL_INTEGER:
	if(val->u.integer == 0)
	  err_fatal("/: Division by zero.");
	
	if(use_float)
	  f /= (REAL)val->u.integer;
#ifdef USE_BIG_INTEGERS
	else if(use_big)
	{
	  mpz_set_ui(tmp, ABS(val->u.integer));
	  mpz_mod(tmp, big->u.integer, tmp);
	  if(mpz_sgn(tmp) != 0)
	  {
	    f = big_integer_to_float(val->u.big);
	    use_float = 1;
	    goto change_type;
	  }
	  
	  mpz_set_ui(tmp, ABS(val->u.integer));
	  mpz_divexact(big->u.integer, big->u.integer, tmp);
	  if(val->u.integer < 0)
	    mpz_neg(big->u.integer, big->u.integer);
	}
#endif /* USE_BIG_INTEGERS */
	else if(!MAT_DIVIDE(i, i, val->u.integer, overflow))
	{
	  f = (REAL)i;
	  use_float = 1;
	  goto change_type;
	}
	break;
	
#ifdef USE_BIG_INTEGERS
      case T_BIG_INTEGER:
	if(mpz_sgn(val->u.big->u.integer) == 0)
	  err_fatal("/: Division by zero.");
	
	if(use_float)
	  f /= big_integer_to_float(val->u.big);
	else if(use_big)
	{
	  mpz_mod(tmp, big->u.integer, val->u.big->u.integer);
	  if(mpz_sgn(tmp) != 0)
	  {
	    f = big_integer_to_float(big);
	    use_float = 1;
	    goto change_type;
	  }
	  
	  mpz_divexact(big->u.integer, big->u.integer, val->u.big->u.integer);
	}
	else
	{
	  use_big = 1;
	  goto change_type;
	}
	break;
#endif /* USE_BIG_INTEGERS */
	
      case T_REAL:
	if(val->u.real == 0.0)
	  err_fatal("/: Division by zero.");

	if(!use_float)
	{
#ifdef USE_BIG_INTEGERS
	  if(use_big)
	    f = big_integer_to_float(big);
	  else
#endif /* USE_BIG_INTEGERS */
	    f = (REAL)i;
	  
	  use_float = 1;
	}
	
	f /= val->u.real;
	break;
	
      default:
	err_fatal("/ on non-numbers.");
      }
      
      if(IS_NOT_PAIR(CDR(p)))
	break;
      p = CDR(p).u.pair;
    }
    
#ifdef USE_BIG_INTEGERS
  mpz_clear(tmp);
#endif /* USE_BIG_INTEGERS */
  
  if(use_float)
    BIF_RESULT_REAL(f);
#ifdef USE_BIG_INTEGERS
  else if(use_big)
    BIF_RESULT_REDUCED_INTEGER(big, i);
#endif /* USE_BIG_INTEGERS */
  else
    BIF_RESULT_SMALL_INTEGER(i);
}
