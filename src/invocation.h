/* invocation.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This is the invocation module.
 */

#ifndef __INVOCATION_H__
#define __INVOCATION_H__

#include "types.h"

struct invocation
{
  INT argc;
  
  char **argv;
};

#endif /* __INVOCATION_H__ */
