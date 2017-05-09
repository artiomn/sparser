//------------------------------------------------------------------------------
// Lexical analyser for math expressions.
//------------------------------------------------------------------------------

// Run parser:
// bool lxn_calulate(char *expr, struct t_message_metadata *mmd,
//                   long int *result)
// {
//    struct t_la_control_block pcb;
//
//    *result = lxn_analyse(lxn_init_param(expr, mmd, &pcb));
//    return(!lxn_print_if_err(&pcb));
//  }
//
//  Artiom N.

#ifndef _lexan_h
#define _lexan_h
//------------------------------------------------------------------------------
#include "common.h"
#include "msgparser.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
typedef enum { lxec_noerror, lxec_ue_eol, lxec_general, lxec_unknown_lexeme,
   lxec_unknown_op,
   lxec_ue_ph, lxec_no_oph, lxec_no_cph,
   lxec_not_field, lxec_field_not_found,
   lxec_func_nodef, lxec_func_ie,
   lxec_farg_ae, lxec_farg_ufw, lxec_farg_ofw, lxec_farg_nocomma,
   lxec_trailing_chars,
   lxec_end
} t_lex_ecode;
//------------------------------------------------------------------------------
// Parser parameters and status structure.
struct t_la_control_block
{
   char                       *pos_start;
   char                       *pos_end;
   struct t_full_msgdata      *fmd;
   t_lex_ecode                e_code;
};
//------------------------------------------------------------------------------
// Init parser control block.
struct t_la_control_block *lxn_init_param(char                       *expr,
                                          struct t_full_msgdata      *fmd,
                                          struct t_la_control_block  *param);
//------------------------------------------------------------------------------
// Parse expression and return calculated value,
// if param->e_code == lxec_noerror.
long int lxn_analyse(struct t_la_control_block *param);
//------------------------------------------------------------------------------
// Return true, if error during parsing was occured.
// False, if all is ok.
__inline bool lxn_print_if_err(const struct t_la_control_block *param);
//------------------------------------------------------------------------------
// Return text message for the error code.
char *lxn_decode_err(const t_lex_ecode e_code);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
