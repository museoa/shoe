/* err.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Error messages.
 */

#if MODULE_DEBUG
#  define DEB(x) err_debug x
#else
#  define DEB(x)
#endif /* MODULE_DEBUG */

#define ERR(msg) err_fatal msg

void err_fatal(char *fmt, ...);
void err_printf(char *fmt, ...);
void err_debug(char *fmt, ...);
void err_debugnn(char *fmt, ...);

void err_print_binary(char *s, INT length);
void err_print_string(char *s, INT length);
void err_print_symbol(struct str *str);

void err_fatal_line(INT line_number, char *fmt, ...);
void err_fatal_file_line(char *filename, INT line_number,
				char *fmt, ...);
void err_fatal_perror(char *fmt, ...);
void err_fatal_internal(char *fmt, ...);
void err_debug_line(INT line_number, char *fmt, ...);

void err_runtime_file_line(char *filename, INT line_number, char *fmt, ...);

void err_display(struct process *process, struct svalue *value);
