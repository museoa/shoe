/* mat.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements the built-in mathematical functions (BIF:s).
 */

#include "types.h"

#include "bif.h"

INT mat_character_is_within_range(INT c, INT base);
INT mat_character_to_digit(INT c);
char mat_digit_to_character(INT d);

INT mat_make_integer(struct process *process,
		     char *s, INT length, struct svalue *val);
INT mat_make_float(struct process *process,
		   char *s, INT length, struct svalue *val);
INT mat_make_number(struct process *process,
		    char *s, INT length, struct svalue *val);

struct str *mat_number_to_string(struct process *process, struct svalue *val,
				 INT base);

BIF_DECLARE(bif_number_to_string);

BIF_DECLARE(bif_zerop);
BIF_DECLARE(bif_positivep);
BIF_DECLARE(bif_negativep);

BIF_DECLARE(bif_is_odd);
BIF_DECLARE(bif_is_even);

BIF_DECLARE(bif_abs);

BIF_DECLARE(bif_floor);
BIF_DECLARE(bif_ceiling);
BIF_DECLARE(bif_truncate);
BIF_DECLARE(bif_round);

BIF_DECLARE(bif_less);

BIF_DECLARE(bif_inexact_to_exact);
BIF_DECLARE(bif_exact_to_inexact);

BIF_DECLARE(bif_plus);
BIF_DECLARE(bif_minus);
BIF_DECLARE(bif_times);
BIF_DECLARE(bif_division);

/* NOTE: These functions assume some properties of the CPU. */

#define MAT_SIGN(x) ((x)<0 ? -1 : 1)

#define MAT_MULTIPLY(r, a, b, tmp)                                         \
 (((tmp)=(a),(r)=(a)*(b)),(((b)&&(r)/(b)!=(tmp)?((r)=(tmp)),0:1)))

#define MAT_DIVIDE(r, a, b, tmp)                                           \
 (((tmp)=(a),(r)=(a)/(b)),(((r)*(b)!=(tmp)?((r)=(tmp)),0:1)))

#define MAT_ADD(r, a, b, tmp)                                              \
 (MAT_SIGN(a)==MAT_SIGN(b)?                                                \
  (((tmp)=(a),(r)=(a)+(b)),MAT_SIGN(r)!=MAT_SIGN(tmp)?                     \
   ((r)=tmp),0:1):((r)=(a)+(b),1))

#define MAT_SUB(r, a, b, tmp)                                              \
 MAT_ADD((r), (a), -(b), (tmp))

#define MAT_NEG(r, n)                                                      \
 ((MAT_SIGN(n)!=MAT_SIGN(-(n))) ? (((r)=-(n)),1) : 0)
