/* exit.h
 *
 * COPYRIGHT (c) 1998 by Fredrik Noring.
 *
 * This is the exit module.
 */

#define EXIT_REGISTER(f) exit_register(MODULE_NAME, f)

void exit_init(void);
void exit_exit(void);
void exit_register(char *name, void (*f)(void));
