/* kernel.c
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 */

#define MODULE_DEBUG 1

#include <stdio.h>

#include "types.h"

#include "exit.h"
#include "err.h"
#include "mem.h"

#include "invocation.h"
#include "process.h"
#include "pair.h"
#include "garb.h"
#include "bif.h"
#include "big.h"
#include "str.h"

#include "kernel.h"
#include "lexer.h"

#include "instructions.h"

#define NEXT goto next

#define PEEK1(pc)                                                          \
          ((INT)*(pc))

#define PEEK2(pc)                                                          \
          (((INT)((UBYTE)(pc)[0])<< 8) |                                   \
            (INT)((UBYTE)(pc)[1]))

#define PEEK3(pc)                                                          \
          (((INT)((UBYTE)(pc)[0])<<16) |                                   \
           ((INT)((UBYTE)(pc)[1])<< 8) |                                   \
            (INT)((UBYTE)(pc)[2]))

#define PEEK4(pc)                                                          \
          (((INT)((UBYTE)(pc)[0])<<24) |                                   \
           ((INT)((UBYTE)(pc)[1])<<16) |                                   \
           ((INT)((UBYTE)(pc)[2])<< 8) |                                   \
            (INT)((UBYTE)(pc)[3]))

#define FETCH1(pc) (*pc++)

#define FETCH4(pc)                                                         \
        (pc += 4,                                                          \
         ((INT)((UBYTE)(pc)[-4])<<24) |                                    \
         ((INT)((UBYTE)(pc)[-3])<<16) |                                    \
         ((INT)((UBYTE)(pc)[-2])<< 8) |                                    \
          (INT)((UBYTE)(pc)[-1]))

#define CONS(r, a, b)                                                      \
        (r).u.pair = pair_cons(process, &(a), &(b));                       \
        (r).type = T_PAIR

#define LIST(r, a)                                                         \
        (r).u.pair = pair_list(process, &(a));                             \
        (r).type = T_PAIR

#define PROGRAM_NAME 0
#define PROGRAM_CODE 1

#define GOTO(x)                                                            \
        pc = x + (UBYTE*)process->program.u.vec->v[PROGRAM_CODE].u.str->s

#define JUMP(r)                                                            \
        do {                                                               \
          struct svalue tmp = (r);                                         \
          if(tmp.type == T_UNDEFINED)                                      \
            goto quit;                                                     \
          process->program.u.vec = tmp.u.vec;                              \
          GOTO(tmp.aux);                                                   \
        } while(0)

#define REG      process->reg[reg]
#define REG2     process->reg[reg2]
#define REG3     process->reg[reg3]
#define REG_ARGL process->reg[REGISTER_ARGL]
#define REG_CONT process->reg[REGISTER_CONT]
#define REG_ENV  process->reg[REGISTER_ENV]
#define REG_PROC process->reg[REGISTER_PROC]
#define REG_VAL  process->reg[REGISTER_VAL]
#define STACK    process->stack
#define ERROR    process->error

#define CHECK_MEMORY                                                       \
        if(process->gc || process->pair_heap.reduce)                       \
          garb_and_reduce(process)

#define env_lookup(env, m, n)                                              \
        while(m-- > 0)                                                     \
          env = &CAR(env->u.pair);                                         \
        env = &CDR(env->u.pair);                                           \
        while(n-- > 0)                                                     \
          env = &CDR(env->u.pair)

static void env_get(struct svalue *result, INT m, INT n, struct svalue *env)
{
  env_lookup(env, m, n);
  
  *result = CAR(env->u.pair);
}

static void env_set(struct svalue *value, INT m, INT n, struct svalue *env)
{
  env_lookup(env, m, n);

  CAR(env->u.pair) = *value;
}

static void env_extend(struct process *process, INT n, struct svalue *env)
{
  struct svalue u;
  
  while(IS_PAIR(CDR(env->u.pair)))
    env = &CDR(env->u.pair);

  u.type = T_UNDEFINED;
  while(n-- > 0)
  {
    env = &CDR(env->u.pair);
    LIST(*env, u);
  }
}

void kernel(struct process *process)
{
  UBYTE *pc;
  INT instr, i, reg, reg2, reg3;

  pc = process->pc;

  /* swap_in(process); */
  
  next:
  
  instr = FETCH1(pc);
  
  if(process->trace)
    err_debug("instr [%p] = %d", pc, instr);
  
  switch(instr)
  {
  case I_exit:
  quit:
    /* err_display(&REG_VAL);
       err_printf("\n"); */
    
    /* swap_out(process); */
    return;
    
  case I_save:
    reg = FETCH1(pc);
    CONS(STACK, REG, STACK);
    NEXT;
    
  case I_restore:
    reg = FETCH1(pc);
    REG = CAR(STACK.u.pair);
    STACK = CDR(STACK.u.pair);
    NEXT;
    
  case I_cons:
    reg = FETCH1(pc);
    reg2 = FETCH1(pc);
    reg3 = FETCH1(pc);
    CONS(REG, REG2, REG3);
    NEXT;
    
  case I_list:
    reg = FETCH1(pc);
    reg2 = FETCH1(pc);
    LIST(REG, REG2);
    NEXT;
    
  case I_apply_bif:
    CHECK_MEMORY;
    reg = FETCH1(pc);

    if(IS_NOT_BIF(REG_PROC))
      err_fatal("Application not BIF.");
    
    REG_PROC.u.bif(process, &REG, &REG_ARGL);
    
    if(ERROR.type != T_UNDEFINED)
    {
      struct svalue t;

      t = process->trap[TRAP_ERROR];
      
      if(IS_CONTINUATION(t))
      {
	REG_VAL = ERROR;
	ERROR.type = T_UNDEFINED;
	
	STACK = CAR(t.u.pair);
	t = CDR(t.u.pair);
	REG_PROC = CAR(t.u.pair);
	t = CDR(t.u.pair);
	REG_ENV = CAR(t.u.pair);
	t = CDR(t.u.pair);
	REG_CONT = CAR(t.u.pair);
	t = CDR(t.u.pair);
	REG_ARGL = CAR(t.u.pair);
	t = CDR(t.u.pair);
	JUMP(t);
      } else
	err_fatal("Trap vector %d is not a continuation.", 0);
    }
    NEXT;
    
  case I_apply_lambda:
    CHECK_MEMORY;
    if(IS_CONTINUATION(REG_PROC))
    {
      struct svalue t = REG_PROC, u = REG_ARGL;
      STACK = CAR(t.u.pair);
      t = CDR(t.u.pair);
      REG_PROC = CAR(t.u.pair);
      t = CDR(t.u.pair);
      REG_ENV = CAR(t.u.pair);
      t = CDR(t.u.pair);
      REG_CONT = CAR(t.u.pair);
      t = CDR(t.u.pair);
      REG_ARGL = CAR(t.u.pair);
      t = CDR(t.u.pair);
      if(IS_NOT_PAIR(u))
	err_fatal("Too few arguments to continuation.");
      REG_VAL = CAR(u.u.pair);
      if(IS_NOT_NIL(CDR(u.u.pair)))
	err_fatal("Too many arguments to continuation.");
      JUMP(t);
    }
    else if(IS_LAMBDA(REG_PROC))
    {
      JUMP(CAR(REG_PROC.u.pair));
    }
    else
    {
      err_display(process, &REG_PROC);
      err_fatal("Application not lambda.");
    }
    NEXT;

  case I_assign:
    reg = FETCH1(pc);
    reg2 = FETCH1(pc);
    REG = REG2;
    NEXT;
    
  case I_assign_nil:
    reg = FETCH1(pc);
    REG.type = T_NIL;
    NEXT;
    
  case I_assign_true:
    reg = FETCH1(pc);
    REG.type = T_TRUE;
    NEXT;
    
  case I_assign_false:
    reg = FETCH1(pc);
    REG.type = T_FALSE;
    NEXT;
    
  case I_assign_small_integer:
    reg = FETCH1(pc);
    for(i = reg2 = 0; i < (INT)sizeof(INT); i++)
      reg2 = (reg2<<8) | (UBYTE)(*pc++);
    REG.u.integer = reg2;
    REG.type = T_SMALL_INTEGER;
    NEXT;
    
#ifdef USE_BIG_INTEGERS
  case I_assign_big_integer:
    reg = FETCH1(pc);
    reg2 = FETCH4(pc);
    REG.u.big = big_allocate_integer_text(&process->big_heap, (char*)pc, 16);
    pc += reg2;
    REG.type = T_BIG_INTEGER;
    NEXT;
#endif /* USE_BIG_INTEGERS */
    
  case I_assign_float:
    /* FIXME: Use "proper" encoding. */
  {
    union
    {
      REAL r;
      UBYTE b[sizeof(REAL)];
    } u;
    INT i;
    
    reg = FETCH1(pc);
    for(i = 0; i < (INT)sizeof(REAL); i++)
      u.b[i] = *pc++;
    REG.u.real = u.r;
    REG.type = T_REAL;
    NEXT;
  }
    
  case I_assign_character:
    reg = FETCH1(pc);
    reg2 = FETCH4(pc);
    REG.u.integer = reg2;
    REG.type = T_CHAR;
    NEXT;
    
  case I_assign_string:
    reg = FETCH1(pc);
    reg2 = FETCH4(pc);
    REG.u.str = str_allocate(&process->str_heap, (char*)pc, reg2);
    REG.type = T_STRING;
    pc += reg2;
    NEXT;
    
  case I_assign_symbol:
    reg = FETCH1(pc);
    reg2 = FETCH4(pc);
    REG.u.str = str_allocate(&process->str_heap, (char*)pc, reg2);
    REG.type = T_SYMBOL;
    pc += reg2;
    NEXT;
    
  case I_assign_bif:
    reg = FETCH1(pc);
    reg2 = FETCH4(pc);
    REG.u.bif = bifs[reg2].bif;
    REG.type = T_BIF;
    if(!REG.u.bif)
      err_fatal("BIF %d not implemented.", reg2);
    NEXT;
    
  case I_assign_lambda:
    reg = FETCH4(pc);
    reg2 = FETCH1(pc);
    REG2.type = T_LABEL;
    REG2.aux = reg;
    REG2.u.vec = process->program.u.vec;
    REG2.u.pair = pair_cons(process, &REG2, &REG_ENV);
    REG2.type = T_LAMBDA;
    NEXT;
    
  case I_assign_label:
    reg = FETCH4(pc);
    REG_CONT.aux = reg;
    REG_CONT.u.vec = process->program.u.vec;
    REG_CONT.type = T_LABEL;
    NEXT;
    
  case I_assign_undefined:
    reg = FETCH1(pc);
    REG.type = T_UNDEFINED;
    NEXT;
    
  case I_call_cc:
    reg = FETCH4(pc);
    REG_ARGL.type = T_LABEL;
    REG_ARGL.aux = reg;
    REG_ARGL.u.vec = process->program.u.vec;
    CONS(REG_ARGL, REG_ARGL, REG_ARGL);
    CONS(REG_ARGL, REG_CONT, REG_ARGL);
    CONS(REG_ARGL, REG_ENV, REG_ARGL);
    CONS(REG_ARGL, REG_PROC, REG_ARGL);
    CONS(REG_ARGL, STACK, REG_ARGL);
    REG_ARGL.type = T_CONTINUATION;
    LIST(REG_ARGL, REG_ARGL);
    NEXT;
    
  case I_branch_bif:
    reg = FETCH4(pc);
    if(IS_BIF(REG_PROC))
      GOTO(reg);
    NEXT;
    
  case I_goto:
    reg = FETCH4(pc);
    GOTO(reg);
    NEXT;
    
  case I_jump:
    JUMP(REG_CONT);
    NEXT;
    
  case I_branch:
    reg = FETCH4(pc);
    if(IS_FALSE(REG_VAL))
      GOTO(reg);
    NEXT;

  case I_lambda:
    reg = FETCH4(pc);
    REG_ENV = CDR(REG_PROC.u.pair);
    if(reg < 0)
    {
      struct svalue *t = &REG_ARGL;
      INT i = reg;
      
      while(++i)
	t = &CDR(t->u.pair);
      LIST(*t, *t);
    }
    else
    {
      struct svalue *t = &REG_ARGL;
      INT i;
      
      for(i = 0; i < reg && IS_PAIR(*t); i++)
	t = &CDR(t->u.pair);
      
      if(IS_NOT_NIL(*t))
	err_fatal("Too many arguments to function.");
      
      if(i != reg)
	err_fatal("Too few arguments to function.");
    }
    CONS(REG_ENV, REG_ENV, REG_ARGL);

    /* Fall through. */
    
  case I_env_extend:
    reg2 = FETCH4(pc);
    env_extend(process, reg2, &REG_ENV);
    NEXT;

  case I_get:
    reg = FETCH1(pc);
    reg2 = FETCH4(pc);
    reg3 = FETCH4(pc);
    env_get(&REG, reg2, reg3, &REG_ENV);
    NEXT;

  case I_set:
    reg = FETCH1(pc);
    reg2 = FETCH4(pc);
    reg3 = FETCH4(pc);
    env_set(&REG, reg2, reg3, &REG_ENV);
    REG.type = T_UNDEFINED;
    NEXT;

  case I_load_env:
    REG_ENV = process->program.u.vec->v[2];
    NEXT;

  default:
    err_fatal("Unknown instruction: %d.", instr);
  }
}

int main(int argc, char **argv)
{
  struct invocation invocation;
  struct process process;

  exit_init();
  mem_init();
  
#ifdef USE_BIG_INTEGERS
  big_init();
#endif /* USE_BIG_INTEGERS */

  invocation.argc = argc;
  invocation.argv = argv;

  process_create(&process, &invocation);
  kernel(&process);
  process_destroy(&process);
  
  exit_exit();

  return 0;
}
