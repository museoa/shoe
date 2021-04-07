/* bif.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Implements the built-in functions (BIF:s) environment.
 */

#ifndef __BIF_H__
#define __BIF_H__

#define SPECIFY_RESULT(_type, _u, _value)                                  \
        do {                                                               \
          result->_u = (_value);                                           \
          result->type = (_type);                                          \
	} while(0)

#define BIF_RESULT_CHAR(x)          SPECIFY_RESULT(T_CHAR, u.integer, (x))
#define BIF_RESULT_SMALL_INTEGER(x)                                        \
        SPECIFY_RESULT(T_SMALL_INTEGER, u.integer, (x))
#define BIF_RESULT_BIG_INTEGER(x)   SPECIFY_RESULT(T_BIG_INTEGER, u.big, (x))
#define BIF_RESULT_REAL(x)          SPECIFY_RESULT(T_REAL, u.real, (x))
#define BIF_RESULT_STRING(x)        SPECIFY_RESULT(T_STRING, u.str, (x))
#define BIF_RESULT_SYMBOL(x)        SPECIFY_RESULT(T_SYMBOL, u.str, (x))
#define BIF_RESULT_MAPPING(x)       SPECIFY_RESULT(T_MAPPING, u.map, (x))
#define BIF_RESULT_VECTOR(x)        SPECIFY_RESULT(T_VECTOR, u.vec, (x))
#define BIF_RESULT_UNDEFINED()      (result->type = T_UNDEFINED)
#define BIF_RESULT_NIL()            (result->type = T_NIL)
#define BIF_RESULT_TRUE()           (result->type = T_TRUE)
#define BIF_RESULT_FALSE()          (result->type = T_FALSE)

#define BIF_RESULT_REDUCED_INTEGER(big, i)                                 \
        do {                                                               \
          i = mpz_get_si(big->u.integer);                                  \
	  if(mpz_cmp_si(big->u.integer, i) == 0)                           \
	    BIF_RESULT_SMALL_INTEGER(i);                                   \
	  else                                                             \
	    BIF_RESULT_BIG_INTEGER(big);                                   \
        } while(0)

#define IS_REDUCABLE_INTEGER(big, i)                                       \
        (i = mpz_get_si(big->u.integer), (mpz_cmp_si(big->u.integer, i) == 0))

#define BIF_MAP_TEXT_WITH_SMALL_INTEGER(map, s, i)                         \
        do {                                                               \
          /* Note: These are not reached by the garb! */                   \
          struct svalue key, value;                                        \
                                                                           \
          key.u.str = str_allocate_text(&process->str_heap, s);            \
          key.type = T_STRING;                                             \
          value.u.integer = i;                                             \
          value.type = T_SMALL_INTEGER;                                    \
                                                                           \
          map_set(map, &key, &value);                                      \
        } while(0)

#define BIF_MAP_TEXT_WITH_UNDEFINED(map, s)                                \
        do {                                                               \
          /* Note: These are not reached by the garb! */                   \
          struct svalue key, value;                                        \
                                                                           \
          key.u.str = str_allocate_text(&process->str_heap, s);            \
          key.type = T_STRING;                                             \
          value.type = T_UNDEFINED;                                        \
                                                                           \
          map_set(map, &key, &value);                                      \
        } while(0)

typedef void (bif_f)(struct process *process,
		     struct svalue *result, struct svalue *args);

struct bif
{
  char *name;
  bif_f *bif;
};

extern struct bif bifs[];

#define BIF_DECLARE(bif_name)                                              \
        void bif_name(struct process *process,                             \
		      struct svalue *result, struct svalue *args)

BIF_DECLARE(bif_booleanp);
BIF_DECLARE(bif_charp);
BIF_DECLARE(bif_nullp);
BIF_DECLARE(bif_numberp);
BIF_DECLARE(bif_pairp);
BIF_DECLARE(bif_portp);
BIF_DECLARE(bif_procedurep);
BIF_DECLARE(bif_stringp);
BIF_DECLARE(bif_symbolp);
BIF_DECLARE(bif_vector);

BIF_DECLARE(bif_eq);

BIF_DECLARE(bif_cons);
BIF_DECLARE(bif_car);
BIF_DECLARE(bif_cdr);
BIF_DECLARE(bif_list);
BIF_DECLARE(bif_set_car);
BIF_DECLARE(bif_set_cdr);

BIF_DECLARE(bif_char_to_integer);
BIF_DECLARE(bif_char_less_equal);
BIF_DECLARE(bif_char_less_than);
BIF_DECLARE(bif_char_equal);
BIF_DECLARE(bif_char_greater_equal);
BIF_DECLARE(bif_char_greater_than);

BIF_DECLARE(bif_integer_to_char);

BIF_DECLARE(bif_make_string);
BIF_DECLARE(bif_string);
BIF_DECLARE(bif_string_to_symbol);
BIF_DECLARE(bif_string_append);
BIF_DECLARE(bif_string_copy);
BIF_DECLARE(bif_string_fill);
BIF_DECLARE(bif_string_length);
BIF_DECLARE(bif_string_ref);
BIF_DECLARE(bif_string_set);
BIF_DECLARE(bif_string_less_equal);
BIF_DECLARE(bif_string_less_than);
BIF_DECLARE(bif_string_equal);
BIF_DECLARE(bif_string_greater_equal);
BIF_DECLARE(bif_string_greater_than);
BIF_DECLARE(bif_substring);

BIF_DECLARE(bif_symbol_to_string);

BIF_DECLARE(bif_make_vector);
BIF_DECLARE(bif_vector_length);
BIF_DECLARE(bif_vector_ref);
BIF_DECLARE(bif_vector_set);

BIF_DECLARE(bif_make_mapping);
BIF_DECLARE(bif_mapping_length);
BIF_DECLARE(bif_mapping_ref);
BIF_DECLARE(bif_mapping_set);
BIF_DECLARE(bif_mapping_remove);

BIF_DECLARE(bif_display);
BIF_DECLARE(bif_newline);

BIF_DECLARE(bif_lexer);
BIF_DECLARE(bif_integer_to_4bytes);

BIF_DECLARE(bif_number_to_string);
BIF_DECLARE(bif_plus);
BIF_DECLARE(bif_less);
BIF_DECLARE(bif_minus);
BIF_DECLARE(bif_times);

BIF_DECLARE(bif_fatal_error);

#endif /* __BIF_H__ */
