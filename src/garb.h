/* garb.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This is the garbage collector module.
 */

#ifndef __GARB_H__
#define __GARB_H__

#include "types.h"

void garb(struct process *process);
void garb_and_reduce(struct process *process);

#endif /* __GARB_H__ */
