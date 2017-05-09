//------------------------------------------------------------------------------
// Lexical analyser predefined functions.
//------------------------------------------------------------------------------

#ifndef _lexan_funcs_h
#define _lexan_funcs_h
//------------------------------------------------------------------------------
#include "common.h"
#include "lexan.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
struct t_func_arg
{
   long int          value;
   struct t_func_arg *next;
};
//------------------------------------------------------------------------------
typedef long int (*f_func)(struct t_func_arg *args, t_lex_ecode *e_code);
//------------------------------------------------------------------------------
// Functions.
long int la_abs(struct t_func_arg *fa, t_lex_ecode *e_code);
#if HAVE_POW == 1
long int la_pow(struct t_func_arg *fa, t_lex_ecode *e_code);
#endif
long int la_min(struct t_func_arg *fa, t_lex_ecode *e_code);
long int la_max(struct t_func_arg *fa, t_lex_ecode *e_code);
long int la_time(struct t_func_arg *fa, t_lex_ecode *e_code);
//------------------------------------------------------------------------------
// Service functions.
bool  add_farg_list(struct t_func_arg **arg_list, const long int arg);
void  free_farg_list(struct t_func_arg **arg_list);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
