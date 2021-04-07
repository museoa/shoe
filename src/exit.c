/* exit.c
 *
 * COPYRIGHT (c) 1998 by Fredrik Noring.
 *
 * This is the exit module.
 */

#define MODULE_DEBUG 0
#define MODULE_NAME  "exit"

#define MAX_REGISTRATIONS 32

#include "types.h"

#include "err.h"
#include "exit.h"

struct registrations {
  char *name;
  void (*f)(void);
};

static INT rp;
static struct registrations registrations[MAX_REGISTRATIONS];

void exit_init(void)
{
  rp = 0;
}

void exit_exit(void)
{
  while(rp--) {
    DEB(("Calling exit function \"%s\".", registrations[rp].name));
    registrations[rp].f();
  }
}

void exit_register(char *name, void (*f)(void))
{
  if(rp >= MAX_REGISTRATIONS)
    ERR(("Max number of registrations used up (callee is \"%s\").", name));
  
  DEB(("Registered function \"%s\".", name));
  registrations[rp].name = name;
  registrations[rp++].f = f;
}
