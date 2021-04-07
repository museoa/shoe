/* process.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This is the process module.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME  "process"

#include "types.h"

#include "err.h"
#include "mem.h"
#include "exit.h"
#include "process.h"
#include "pair.h"
#include "str.h"
#include "vec.h"
#include "invocation.h"

#include "bootstrap.h"

void process_create(struct process *process, struct invocation *invocation)
{
  INT i;
  
  /* Create heaps and indices. */
  pair_create(&process->pair_heap);
  vec_create(process);
  str_create(process);
  map_create(process);
  
#ifdef USE_BIG_INTEGERS
  big_create(process);
#endif /* USE_BIG_INTEGERS */
  
  /* Initialize all registers. */
  process->reg[REGISTER_ARGL].type = T_UNDEFINED;
  process->reg[REGISTER_CONT].type = T_UNDEFINED;
  process->reg[REGISTER_ENV].type  = T_UNDEFINED;
  process->reg[REGISTER_PROC].type = T_UNDEFINED;
  process->reg[REGISTER_VAL].type  = T_UNDEFINED;

  process->program.type = T_UNDEFINED;
  process->stack.type = T_UNDEFINED;
  process->error.type = T_UNDEFINED;

  /* Start program at jump label zero. */
  process->label = 0;
  
  /* No PIDs are invented, yet. */
  process->pid = 0;

  /* Set invocation parameters. */
  process->invocation = invocation;

  /* Reset program. */
  process->program.u.vec = vec_allocate(&process->vec_heap, 3);
  process->program.type = T_VECTOR;
  process->program.u.vec->v[1].u.str = str_allocate(&process->str_heap,
						    (char*)bootstrap_code,
						    bootstrap_size);
  process->program.u.vec->v[1].type = T_STRING;
  process->program.u.vec->v[2].u.pair = pair_nil(process);
  process->program.u.vec->v[2].type = T_PAIR;

  process->pc = (UBYTE*)process->program.u.vec->v[1].u.str->s;

  /* Traps. */
  for(i = 0; i < N_TRAPS; i++)
    process->trap[i].type = T_FALSE;
  
  /* Garbage collector. */
  process->gc = 0;
  
  /* Reset debug information. */
  process->trace = 0;
}

void process_destroy(struct process *process)
{
  /* Wipe out heaps and indices. */
#ifdef USE_BIG_INTEGERS
  big_destroy(&process->big_heap);
#endif /* USE_BIG_INTEGERS */
  
  map_destroy(&process->map_heap);
  str_destroy(&process->str_heap);
  vec_destroy(&process->vec_heap);
  pair_destroy(&process->pair_heap);
}
