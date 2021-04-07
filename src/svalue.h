/* svalue.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements svalue operations.
 */

#ifndef __SVALUE_H__
#define __SVALUE_H__

#include "types.h"

char *svalue_describe(INT type);
INT svalue_hash(struct svalue *value);
INT svalue_eq(struct svalue *a, struct svalue *b);
INT svalue_compare(struct svalue *a, struct svalue *b);

#endif /* __SVALUE_H__ */
