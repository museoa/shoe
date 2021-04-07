/* kernel.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements the kernel functions.
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "types.h"

#define swap_in(p)                                                         \
        argl = p->argl;                                                    \
        cont = p->cont;                                                    \
        env = p->env;                                                      \
        proc = p->proc;                                                    \
        stack = p->stack;                                                  \
        val = p->val;                                                      \
        label = p->label

#define swap_out(p)                                                        \
        p->argl = argl;                                                    \
        p->cont = cont;                                                    \
        p->env = env;                                                      \
        p->proc = proc;                                                    \
        p->stack = stack;                                                  \
        p->val = val;                                                      \
        p->label = label

#define garbcheck()                                                        \
        if(is_pair_heap_full(process))                                     \
          do {                                                             \
            swap_out(process);                                             \
            garb(process);                                                 \
            swap_in(process);                                              \
          } while(0)

#define kernel_error(msg)                                                  \
        {                                                                  \
          val.type = T_STRING;                                             \
          val.u.str = str_allocate_text(&process->str_heap, msg);          \
          trap(0, val);                                                    \
        }

#define assign(r, x)                                                       \
        r = x

#define assign_nil(r)                                                      \
        r.type = T_NIL

#define assign_integer(r, i)                                               \
        r.type = T_INTEGER;                                                \
        r.u.integer = i

#define assign_label(r, l)                                                 \
        r.type = T_LABEL;                                                  \
        r.u.label = l
#define assign_lambda(r, l, e)                                             \
        garbcheck();                                                       \
        assign_label(r, l);                                                \
        r.u.lambda = pair_cons(process, &r, &e);                           \
        r.type = T_LAMBDA;                                                 \

#define assign_bif(r, b)                                                   \
        r.type = T_BIF;                                                    \
        r.u.bif = b

#define assign_char(r, c)                                                  \
        r.type = T_CHAR;                                                   \
        r.u.integer = c

#define assign_true(r, b)                                                  \
        r.type = T_TRUE

#define assign_false(r, b)                                                 \
        r.type = T_FALSE

#define assign_string(r, s)                                                \
        r.type = T_STRING;                                                 \
        r.u.str = str_allocate_text(&process->str_heap, s)

#define assign_symbol(r, s)                                                \
        r.type = T_SYMBOL;                                                 \
        r.u.str = str_allocate_text(&process->str_heap, s)

#define list(r, a)                                                         \
        garbcheck();                                                       \
        r.u.pair = pair_list(process, &a);                                 \
        r.type = T_PAIR

#define cons(r, a, b)                                                      \
        garbcheck();                                                       \
        r.u.pair = pair_cons(process, &a, &b);                             \
        r.type = T_PAIR

#define jump(r)                                                            \
        label = r.u.label;                                                 \
        continue

#define branch(l, p)                                                       \
        if(p)                                                              \
          goto l

#define lambda(e, p, a, n)                                                 \
        e = CDR(p.u.lambda);                                               \
        if(n < 0)                                                          \
        {                                                                  \
          INT i = n;                                                       \
	  struct svalue *t = &a;                                           \
          while(++i)                                                       \
            t = &CDR(t->u.pair);                                           \
          garbcheck();                                                     \
          t->u.pair = pair_list(process, t);                               \
          t->type = T_PAIR;                                                \
	}                                                                  \
        cons(e, e, a)

#define call_cc(r, l)                                                      \
        r.type = T_LABEL;                                                  \
        r.u.label = l;                                                     \
        cons(r, argl, r);                                                  \
        cons(r, cont, r);                                                  \
        cons(r, env, r);                                                   \
        cons(r, proc, r);                                                  \
        cons(r, stack, r);                                                 \
        r.type = T_CONTINUATION

#define apply_lambda(l, a)                                                 \
        if(IS_CONTINUATION(l))                                             \
        {                                                                  \
	  struct svalue t = l, u = a;                                      \
          stack = CAR(t.u.pair);                                           \
          t = CDR(t.u.pair);                                               \
          proc = CAR(t.u.pair);                                            \
          t = CDR(t.u.pair);                                               \
          env = CAR(t.u.pair);                                             \
          t = CDR(t.u.pair);                                               \
          cont = CAR(t.u.pair);                                            \
          t = CDR(t.u.pair);                                               \
          argl = CAR(t.u.pair);                                            \
          t = CDR(t.u.pair);                                               \
          if(IS_NOT_PAIR(u))                                               \
            kernel_error("To few arguments to continuation.");             \
          val = CAR(u.u.pair);                                             \
          if(IS_NOT_NIL(CDR(u.u.pair)))                                    \
            kernel_error("To many arguments to continuation.");            \
          jump(t);                                                         \
        }                                                                  \
        else if(IS_LAMBDA(l))                                              \
        {                                                                  \
          jump(CAR(l.u.lambda));                                           \
        }                                                                  \
        else                                                               \
          kernel_error("Application not lambda.")

#define trap(n, x)                                                         \
        trap_get(&proc, n, &env);                                          \
        if(IS_CONTINUATION(proc))                                          \
        {                                                                  \
	  struct svalue t = proc;                                          \
          val = x;                                                         \
          stack = CAR(t.u.pair);                                           \
          t = CDR(t.u.pair);                                               \
          proc = CAR(t.u.pair);                                            \
          t = CDR(t.u.pair);                                               \
          env = CAR(t.u.pair);                                             \
          t = CDR(t.u.pair);                                               \
          cont = CAR(t.u.pair);                                            \
          t = CDR(t.u.pair);                                               \
          argl = CAR(t.u.pair);                                            \
          t = CDR(t.u.pair);                                               \
          jump(t);                                                         \
        } else                                                             \
          err_fatal("Trap vector %d is not a continuation.", n)

#define apply_bif(v, f, a)                                                 \
        if(IS_NOT_BIF(f))                                                  \
          kernel_error("Application not BIF.");                            \
        f.u.bif(process, &v, &a);                                          \
        if(process->error.type != T_UNDEFINED)                             \
        {                                                                  \
          v = process->error;                                              \
          process->error.type = T_UNDEFINED;                               \
          trap(0, v);                                                      \
        } else do { } while(0)

#define copy_list(r, l)                                                    \
        r = l

#define save(r)                                                            \
        cons(stack, r, stack)

#define restore(r)                                                         \
        r = CAR(stack.u.pair);                                             \
        stack = CDR(stack.u.pair)

#define is_bif(x)   IS_BIF(x)

#define is_false(x) IS_FALSE(x)

#define get(r, m, n, e)                                                    \
        env_get(&r, m, n, &e)

#define set(r, m, n, e)                                                    \
        env_set(&r, m, n, &e)

#define define(r, e)                                                       \
        env_define(process, &r, &e)

#endif /* __KERNEL_H__ */
