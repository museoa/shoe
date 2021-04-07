/* instructions.h
 *
 * COPYRIGHT (c) 1999 by Fredrik Noring.
 *
 * Instruction mappings.
 */

#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#define I_exit                 	0
#define I_save                 	1
#define I_restore              	2
#define I_list                 	3
#define I_cons                 	4
#define I_apply_bif            	5
#define I_apply_lambda         	6
#define I_assign               	7
#define I_assign_nil           	8
#define I_assign_true          	9
#define I_assign_false         10
#define I_assign_small_integer 11
#define I_assign_string        12
#define I_assign_symbol        13
#define I_assign_bif           14
#define I_assign_lambda        15
#define I_assign_label         16
#define I_call_cc              17
#define I_branch_bif           18
#define I_goto                 19
#define I_jump                 20
#define I_branch               21
#define I_lambda               22
/* #define I_define               23 */
#define I_get                  24
#define I_set                  25
#define I_env_extend           26
#define I_assign_undefined     27
#define I_assign_character     28
#define I_assign_float         29
#define I_load_env             30
#define I_assign_big_integer   31

#endif /* __INSTRUCTIONS_H__ */
