/* lexer.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 */

#ifndef __LEXER_H__
#define __LEXER_H__

#include "types.h"

struct token
{
  INT length;
  INT type;

  union
  {
    INT i;
    char *str;
  } u;
};

INT lexer(struct process *process, struct str *str, struct vec *vec);
struct vec* lex_read(struct process *process, struct str *str);

#endif /* __LEXER_H__ */
