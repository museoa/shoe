/* args.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * This module takes care of argument checks.
 */

#include "types.h"

#define ARGS_ERROR(args)                                                   \
        do {                                                               \
          args_error args;                                                 \
          return;                                                          \
        } while(0)

#define ARGS_GET(args)                                                     \
        do {                                                               \
          if(!args_get args)                                               \
             return;                                                       \
        } while(0)

#define RANGE_CHECK(process, name, ref, length)                            \
        do {                                                               \
          if(ref < 0 || length <= ref)                                     \
          {                                                                \
            if(length)                                                     \
              ARGS_ERROR((process, name, "Index %d out of range 0 - %d.",  \
	                ref, length-1));                                   \
            else                                                           \
              ARGS_ERROR((process, name, "Index %d out of empty range.",   \
			 ref));                                            \
          }                                                                \
	} while(0)

INT args_error(struct process *process, char *name, char *fmt, ...);
INT args_get(struct process *process, char *name,
	     struct svalue *args, char *fmt, ...);
