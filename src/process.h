/* process.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This is for processes.
 */

#ifndef __PROCESS_H__
#define __PROCESS_H__

struct process;

#include "types.h"

#include "big.h"
#include "map.h"
#include "pair.h"
#include "str.h"
#include "vec.h"

#define N_REGISTERS 5

#define REGISTER_ARGL 0
#define REGISTER_CONT 1
#define REGISTER_ENV  2
#define REGISTER_PROC 3
#define REGISTER_VAL  4

#define N_TRAPS 8

#define TRAP_ERROR 0

struct process
{
  /* Process identification. */
  INT pid;

  /* Invocation parameters. */
  struct invocation *invocation;

  /* Registers. */
  struct svalue reg[N_REGISTERS];
  struct svalue stack;

  /* Jump label. */
  INT label;

  /* Error management. */
  struct svalue error;
  
  /* Assorted heaps. */
  struct pair_heap pair_heap;
  struct map_heap map_heap;
  struct str_heap str_heap;
  struct vec_heap vec_heap;

#ifdef USE_BIG_INTEGERS
  struct big_heap big_heap;
#endif /* USE_BIG_INTEGERS */
  
  /* The program code. */
  UBYTE *pc;
  struct svalue program;

  /* Traps. */
  struct svalue trap[N_TRAPS];

  /* Garbage collector.  Set this flag to start a collection. */
  INT gc;
  
  /* Debug information. */
  INT trace;
};

void process_create(struct process *process, struct invocation *invocation);
void process_destroy(struct process *process);

#endif /* __PROCESS_H__ */
