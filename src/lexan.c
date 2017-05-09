//------------------------------------------------------------------------------
#include <ctype.h>
#include <string.h>
//------------------------------------------------------------------------------
#include "lexan.h"
#include "lexan_funcs.h"
#include "utils.h"
//------------------------------------------------------------------------------
typedef enum
{
   lxt_unknown, lxt_eol,
   lxt_bracket, lxt_op, lxt_name, lxt_function, lxt_number, lxt_comma
} t_lexeme_type;
//------------------------------------------------------------------------------
typedef enum
{
   op_unknown,
   // Max. prio.
   op_mul,     op_div,     op_mod,
   op_plus,    op_minus,
   op_lshift,  op_rshift,
   op_lt,      op_gt,      op_ge,   op_le,
   op_eq,      op_ne,
   op_bw_and,
   op_bw_xor,
   op_bw_or,
   op_log_and,
   // Min. prio.
   op_log_or,
   op_un_bw_not, op_un_log_not, op_size, op_offset, op_exists
} t_operation;
//------------------------------------------------------------------------------
typedef enum { bt_left, bt_right } t_bracket_type;
//------------------------------------------------------------------------------
typedef enum { nmt_unknown, nmt_field, nmt_metafield, nmt_op, nmt_function }
   t_name_type;
//------------------------------------------------------------------------------
const t_operation op_so    = op_size;
const t_operation op_ofs   = op_offset;
const t_operation op_exs   = op_exists;
//------------------------------------------------------------------------------
#define MAX_PRIO 0
//------------------------------------------------------------------------------
struct op_action
{
   int            prio;
//   t_operation    op;
//   f_op_function  func;
}
static operations[] =
{
   [op_unknown]   = { MAX_PRIO     },
   [op_mul]       = { MAX_PRIO     },
   [op_div]       = { MAX_PRIO     },
   [op_mod]       = { MAX_PRIO     },
   [op_plus]      = { MAX_PRIO + 1 },
   [op_minus]     = { MAX_PRIO + 1 },
   [op_lshift]    = { MAX_PRIO + 2 },
   [op_rshift]    = { MAX_PRIO + 2 },
   [op_lt]        = { MAX_PRIO + 3 },
   [op_gt]        = { MAX_PRIO + 3 },
   [op_ge]        = { MAX_PRIO + 3 },
   [op_le]        = { MAX_PRIO + 3 },
   [op_eq]        = { MAX_PRIO + 4 },
   [op_ne]        = { MAX_PRIO + 4 },
   [op_bw_and]    = { MAX_PRIO + 5 },
   [op_bw_xor]    = { MAX_PRIO + 6 },
   [op_bw_or]     = { MAX_PRIO + 7 },
   [op_log_and]   = { MAX_PRIO + 8 },
   [op_log_or]    = { MAX_PRIO + 9 },
};
//------------------------------------------------------------------------------
#define MIN_PRIO \
         (operations[sizeof(operations) / sizeof(struct op_action) - 1].prio)
//------------------------------------------------------------------------------
struct t_name
// Name tables.
{
   char           *value;
   t_name_type    nt;
   void           *ndata;
   unsigned int   val_len;
}
// Predefined names.
static name_table_fixed[] =
{
   { "sizeof", nmt_op,         (void*)&op_so,   0 },
   { "offset", nmt_op,         (void*)&op_ofs,  0 },
   { "exists", nmt_op,         (void*)&op_exs,  0 },
   { "abs",    nmt_function,   &la_abs,         0 },
   #if HAVE_POW == 1
   { "pow",    nmt_function,   &la_pow,         0 },
   #endif
   { "max",    nmt_function,   &la_max,         0 },
   { "min",    nmt_function,   &la_min,         0 },
   { "time",   nmt_function,   &la_time,        0 },
   { NULL,     nmt_unknown,    NULL,            0 }
},
// Dynamic name table.
*name_table_dynamic;
//------------------------------------------------------------------------------
struct t_lexeme
{
   t_lexeme_type type;
   union
   {
      long int          num;
      struct t_name     name;
      t_operation       op;
      t_bracket_type    brk_type;
   } data;
};
//------------------------------------------------------------------------------
static
struct t_name *lookup_name_in_table(const struct t_name *name,
                                    struct t_name table[])
// Return structure woth name description or NULL, if name not found.
{
   register int i;

   assert(table   != NULL);
   assert(name    != NULL);

   for (i = 0; /*(i < (sizeof(table) / sizeof(struct t_name))) && */table[i].value; i++)
   {
      if (streqn(name->value, table[i].value, name->val_len)) return(&table[i]);
   }
   return(NULL);
}

//------------------------------------------------------------------------------
// Lexical analyzer.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
static __inline bool is_opsymbol(const char c)
{
   switch (c)
   {
      case '/':
      case '*':
      case '%':
      case '<':
      case '>':
      case '=':
      case '!':
      case '&':
      case '^':
      case '|':
      case '+':
      case '-':
      case '~':
         return(true);
      break;
   }
   return(false);
}
//------------------------------------------------------------------------------
static char *la_get_op(char *expr, struct t_lexeme *op)
// Return operation in op parameter.
// Return value - next symbol after operation, or error symbol position.
// If error occured, op->type == lxt_unknown (and op->data.op == op_unknown).
{
   char *result = expr;

   assert(result  != NULL);
   assert(op      != NULL);

   op->type    = lxt_op;
   op->data.op = op_unknown;

   switch (*result)
   {
      case '*':
         op->data.op = op_mul;
      break;
      case '/':
         op->data.op = op_div;
      break;
      case '%':
         op->data.op = op_mod;
      break;
      case '+':
         op->data.op = op_plus;
      break;
      case '-':
         op->data.op = op_minus;
      break;
      case '^':
         op->data.op = op_bw_xor;
      break;
      case '~':
         op->data.op = op_un_bw_not;
      break;
      case '<':
         result++;
         if (*result == '\0')
         {
            op->data.op = op_lt;
            return(result);
         }
         switch (*result)
         {
            case '<':
               op->data.op = op_lshift;
            break;
            case '=':
               op->data.op = op_le;
            break;
            default:
               op->data.op = op_lt;
               return(result);
         }
      break;
      case '>':
         result++;
         if (*result == '\0')
         {
            op->data.op = op_gt;
            return(result);
         }
         switch (*result)
         {
            case '>':
               op->data.op = op_rshift;
            break;
            case '=':
               op->data.op = op_ge;
            break;
            default:
               op->data.op = op_gt;
               return(result);
         }
      break;
      case '&':
         result++;
         if (*result != '&')
         {
            op->data.op = op_bw_and;
            return(result);
         }
         else op->data.op = op_log_and;
      break;
      case '|':
         result++;
         if (*result != '|')
         {
            op->data.op = op_bw_or;
            return(result);
         }
         else op->data.op = op_log_or;
      break;
      case '!':
         result++;
         if (*result != '=')
         {
            op->data.op = op_un_log_not;
            return(result);
         }
         else op->data.op = op_ne;
      break;
      case '=':
         result++;
         if (*result != '=')
         {
            op->data.op = op_unknown;
            op->type    = lxt_unknown;
            return(NULL);
         }
         op->data.op = op_eq;
      break;

      default:
         op->data.op = op_unknown;
         op->type    = lxt_unknown;
         return(result);
   }
   return(++result);
}
//------------------------------------------------------------------------------
static char *la_get_number(char *expr, struct t_lexeme *num)
// Return number in num->data.num.
// Return value - next position after number, if ok or error position, if error.
// If error is occured, op->type == lxt_unknown,
{
   char *result = expr;

   assert(num     != NULL);
   assert(result  != NULL);

   num->type = lxt_number;

   if (*expr == '0')
   // Hex, bin or octal.
   {
      if (!(++expr) || (*expr == '\0'))
      {
         num->data.num = 0;
         result++;
      }
      else if (*expr == 'x')
      // Hex.
      {
         if (!(++expr) || !isxdigit(*expr))
         {
            num->type = lxt_unknown;
            return(expr);
         }
         num->data.num = strtol(expr, &result, 16);
      }
      else if (*expr == 'b')
      // Bin.
      {
         if (!(++expr) || !((*expr == '1') || (*expr == '0')))
         {
            num->type = lxt_unknown;
            return(expr);
         }
         num->data.num = strtol(expr, &result, 2);
      }
      // Octal.
      else if (isoctal(*expr)) num->data.num = strtol(expr, &result, 8);
      else
      {
         num->data.num = 0;
         result++;
      }
   }
   else if (isdigit(*expr))
   // Decimal.
   {
      num->data.num = strtol(expr, &result, 10);
   }
   else num->type = lxt_unknown;

   return(result);
}
//------------------------------------------------------------------------------
static char *la_get_name(char *expr, struct t_lexeme *name)
// Return string in name->data.name or operation, if name is operation.
{
   register char  *result = expr;
   unsigned int   st_len;
   bool           is_mf;
   struct t_name  *lookup_name;


   assert(result  != NULL);
   assert(name    != NULL);

   name->type = lxt_unknown;

//   if (!expr) return(NULL);
   is_mf = is_metafield_fs(*expr);
   if (!(is_first_namesym(*expr) || is_mf)) return(expr);
   name->type = lxt_name;
   while (++result && (*result != '\0') && is_namesym(*result)) ;
   st_len = result - expr;
   if (st_len)
   {
      name->data.name.value   = expr;  //(char*)strndup(expr, st_len);
      name->data.name.val_len = st_len;
      if (is_mf)
      // Name is metafield.
      {
         name->type           = lxt_name;
         name->data.name.nt   = nmt_metafield;
      }
      else if ((lookup_name =
                lookup_name_in_table(&(name->data.name), name_table_fixed)) != NULL)
      {
         // Name is operation.
         if (lookup_name->nt == nmt_op)
         {
            //dfree(name->data.name.value);
            name->type     = lxt_op;
            name->data.op  = *(t_operation*)(lookup_name->ndata);
         }
         // Name is a function or identifier (field).
         else
         {
            name->data.name.nt      = lookup_name->nt;
            name->data.name.ndata   = lookup_name->ndata;

            if (name->data.name.nt == nmt_function) name->type = lxt_function;
         }
      }
      else
      // Name was not found in lookup table.
      {
         name->type              = lxt_name;
         name->data.name.nt      = nmt_field;
      }
   }
   return(result);
}
//------------------------------------------------------------------------------
static char *la_get_bracket(char *expr, struct t_lexeme *bt)
// Return bracket in bt->data.brk_type.
// Return value - next position after bracket, if ok or error position, if error.
// If error is occured, bt->type == lxt_unknown,
{
   assert(expr != NULL);
   assert(bt   != NULL);

   if (is_left_bracket(*expr))
   {
      bt->data.brk_type = bt_left;
   }
   else if (is_right_bracket(*expr))
   {
      bt->data.brk_type = bt_right;
   }
   else
   {  
      bt->type = lxt_unknown;
      return(expr);
   }

   bt->type = lxt_bracket;

   return(++expr);
}
//------------------------------------------------------------------------------
static __inline char *la_get_comma(char *expr, struct t_lexeme *comma)
{
   assert(expr    != NULL);
   assert(comma   != NULL);

   if (!iscomma(*expr))
   {
      comma->type = lxt_unknown;
      return(expr);
   }

   comma->type = lxt_comma;
   return(++expr);
   
}
//------------------------------------------------------------------------------
static __inline char *la_get_eol(char *expr, struct t_lexeme *eol)
{
   assert(eol != NULL);

   if (!expr || iseol(*expr))
   {
      eol->type = lxt_eol;
      return(++expr);
   }

   eol->type = lxt_unknown;
   return(expr);
}
//------------------------------------------------------------------------------
static char *la_get_token(char *expr, struct t_lexeme *lexeme)
// Fill lexeme structure.
{
   register char *result = skip_spaces(expr);

   if (!result || iseol(*result))
   {
      result = la_get_eol(result, lexeme);
   }
   else if (isbracket(*result))
   {
      result = la_get_bracket(result, lexeme);
   }
   else if (isdigit(*result))
   // Number (octal, decimal, hex or binary).
   // Number always starts from decimal digit.
   {
      result = la_get_number(result, lexeme);
   }
   else if (is_opsymbol(*result))
   {
      result = la_get_op(result, lexeme);
   }
   else if (is_first_namesym(*result) || is_metafield_fs(*result))
   // Identifier, operator word, function or metafield.
   {
      result = la_get_name(result, lexeme);
   }
   else if (iscomma(*result))
   {
      result = la_get_comma(result, lexeme);
   }
   else
   // 
   {
      assert(lexeme != NULL);
      lexeme->type = lxt_unknown;
   }

   return(result);
}

//------------------------------------------------------------------------------
// Syntax analyzer.
//------------------------------------------------------------------------------

static
long int sa_ph_expression(char                           **expr,
                          struct t_full_msgdata          *fmd,
                          const bool                     f_skip,
                          t_lex_ecode                    *e_code);
//------------------------------------------------------------------------------
static
long int sa_term(char                           **expr,
                 const int                      prio,
                 struct t_full_msgdata          *fmd,
                 const bool                     f_skip,
                 t_lex_ecode                    *e_code);
//------------------------------------------------------------------------------
static
long int make_operation(const long int arg0, const long int arg1,
                        const t_operation op,
                        t_lex_ecode *e_code)
// Make binary operation with code op: arg0 op arg1.
// Return calculated value.
{
   long int result = 0;

   assert(e_code != NULL);

   if (*e_code != lxec_noerror) return(result);

   switch (op)
   {
      case op_mul:
         result = arg0 * arg1;
      break;
      case op_div:
         result = arg0 / arg1;
      break;
      case op_mod:
         result = arg0 % arg1;
      break;
      case op_plus:
         result = arg0 + arg1;
      break;
      case op_minus:
         result = arg0 - arg1;
      break;
      case op_lshift:
         result = arg0 << arg1;
      break;
      case op_rshift:
         result = arg0 >> arg1;
      break;
      case op_lt:
         result = arg0 < arg1;
      break;
      case op_gt:
         result = arg0 > arg1;
      break;
      case op_ge:
         result = arg0 >= arg1;
      break;
      case op_le:
         result = arg0 <= arg1;
      break;
      case op_eq:
         result = arg0 == arg1;
      break;
      case op_ne:
         result = arg0 != arg1;
      break;
      case op_bw_and:
         result = arg0 & arg1;
      break;
      case op_bw_xor:
         result = arg0 ^ arg1;
      break;
      case op_bw_or:
         result = arg0 | arg1;
      break;
      case op_log_and:
         result = arg0 && arg1;
      break;
      case op_log_or:
         result = arg0 || arg1;
      break;
/*      case op_size:
      break;
      case op_offset:
      break;*/
      default:
         *e_code = lxec_unknown_op;
   }
   return(result);
}
//------------------------------------------------------------------------------
static
long int calc_op_value(char                           **expr,
                       const int                      prio,
                       struct t_full_msgdata          *fmd,
                       const bool                     f_skip,
                       const t_operation              op,
                       const long int                 left_op,
                       t_lex_ecode                    *e_code)
// Get right operator and calculate expression or skip right expression.
{
   long int result      = 0;
   long int right_op;
   bool     skip_flag   = false;

   if ((op == op_log_and) && (left_op == 0))
   {
      skip_flag   = true;
      result      = 0;
   }
   else if ((op == op_log_or) && (left_op != 0))
   {
      skip_flag   = true;
      result      = 1;
   }
   
   right_op = (prio < MAX_PRIO) ? sa_ph_expression(expr, fmd, skip_flag, e_code) :
               sa_term(expr, prio, fmd, skip_flag, e_code);

   if (!skip_flag) result = make_operation(left_op, right_op, op, e_code);

   return(result);
}
//------------------------------------------------------------------------------
static
long int sa_term(char                           **expr,
                 const int                      prio,
                 struct t_full_msgdata          *fmd,
                 const bool                     f_skip,
                 t_lex_ecode                    *e_code)
// Get terms from operation priority 'prio'.
// 'prio'   - start operation priority.
// 'expr'   - address of expression string.
// *expr will be changed to the last untouched symbol.
// Return calculated value.
{
   char              *last_spos;
   int               high_prio   = prio - 1;
   register long int result      = 0;
   struct t_lexeme   lex;

   assert(e_code  != NULL);
   assert(expr    != NULL);

   last_spos = *expr;

   result =
      (high_prio < MAX_PRIO) ? sa_ph_expression(expr, fmd, f_skip, e_code) :
                      sa_term(expr, high_prio, fmd, f_skip, e_code);

   if (*e_code != lxec_noerror) return(result);

   while (true)
   {
      last_spos   = *expr;
      *expr       = la_get_token(*expr, &lex);

      if (lex.type == lxt_unknown)
      {
         *e_code  = lxec_unknown_lexeme;
         // Unknown lexeme is next.
         *expr    = skip_spaces(last_spos) + 1;
         return(result);
      }

      if ((lex.type == lxt_op) && (operations[lex.data.op].prio == prio))
      {
         result = calc_op_value(expr, high_prio, fmd, f_skip, lex.data.op, result, e_code);
/*         right_op = (high_prio < MAX_PRIO) ? sa_ph_expression(expr, msg, e_code) :
                     sa_term(expr, high_prio, msg, e_code);

         result = make_operation(result, right_op, lex.data.op, e_code);*/
         if (*e_code != lxec_noerror) return(result);
      }
      else
      {
         *expr = last_spos;
         return(result);
      }
   }

   return(result);
}
//------------------------------------------------------------------------------
static
long int sa_get_field_value(const struct t_name             *name,
                            struct t_full_msgdata           *fmd,
                            t_lex_ecode                     *e_code)
{
   long int             result = 0;
   char                 *nm;
   struct t_message_fli *fl;
   struct t_metafield   mfl;

   assert(e_code  != NULL);
   assert(name    != NULL);
   assert(fmd     != NULL);

   if (*e_code != lxec_noerror) return(result);
   // Need to control for free() and duplicated free().
   nm = strndup(name->value, name->val_len);

   switch (name->nt)
   {
      case nmt_field:
         if (!nm || (fl = get_field_by_name(nm, fmd->msg)) == NULL)
         {
            dfree(nm);
            *e_code = lxec_field_not_found;
            return(result);
         }
         if (memcpy(&result, fl->field_data, min(sizeof(long int), fl->real_size)) == NULL)
         {
            *e_code = lxec_general;
            return(result);
         }
      break;
      case nmt_metafield:
         mfl.name = nm;
         if (!(mfl.name && get_metafield_by_name(fmd, &mfl)))
         {
            dfree(nm);
            *e_code = lxec_field_not_found;
            return(result);
         }
         if (mfl.type == mft_data)
         {
            result = mfl.value.data;
         }
         else if (mfl.type == mft_ptr)
         {
            *e_code = lxec_not_field;
            //memcpy(&result, mfl.value.data_ptr, min(sizeof(long int), mfl.size));
         }
     
      break;
      default:
         *e_code = lxec_not_field;
   }
   dfree(nm);

   return(result);
}
//------------------------------------------------------------------------------
static
long int sa_ph_un_op(char                    **expr,
                     const t_operation       op,
                     const struct t_message  *msg,
                     const bool              f_skip,
                     t_lex_ecode             *e_code)
// Prefix operator with parenthesis.
{
   struct t_lexeme      lex;
   struct t_message_fli *fl;
   char                 *nm;
//   char                 *last_spos;
   long int             result  = 0;
   
   assert(e_code  != NULL);
   assert(expr    != NULL);
   // Used.
   assert(msg     != NULL);

   if (*e_code != lxec_noerror) return(result);

//   last_spos = *expr;

   *expr = la_get_token(*expr, &lex);

   if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_left))
   {
      *e_code  = lxec_no_oph;
      return(result);
   }

   *expr = la_get_token(*expr, &lex);
   if ((lex.type != lxt_name) || (lex.data.name.nt != nmt_field))
   {
      *e_code  = lxec_not_field;
      return(result);
   }


   if (!f_skip)
   {
      // Need to control for free() and duplicated free().
      if ((nm = strndup(lex.data.name.value, lex.data.name.val_len)) == NULL)
      {
         *e_code  = lxec_general;
         return(result);
      }
      fl = get_field_by_name(nm, msg);

      switch (op)
      {
         case op_size:
            if (!fl)
            {
               *e_code = lxec_field_not_found;
               dfree(nm);
               return(result);
            }

            result = fl->real_size;
         break;
         case op_offset:
            if (!fl)
            {
               *e_code = lxec_field_not_found;
               dfree(nm);
               return(result);
            }

            result = fl->field_data - (uint8*)&(msg->raw_message->data);
         break;
         case op_exists:
            result = (fl != NULL);
         break;
         default:
            *e_code = lxec_unknown_op;
      }
   }

   dfree(nm);

//   last_spos   = *expr;
   *expr       = la_get_token(*expr, &lex);

   if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_right))
   {
      *e_code  = lxec_no_cph;
      return(result);
   }
   return(result);
}
//------------------------------------------------------------------------------
static
long int sa_run_function(char                            **expr,
                         struct t_name                   *f_name,
                         struct t_full_msgdata           *fmd,
                         const bool                      f_skip,
                         t_lex_ecode                     *e_code)
{
   struct t_lexeme   lex;
   char              *last_spos;
   long int          result      = 0;
   struct t_func_arg *arg_list   = NULL;

   assert(e_code  != NULL);
   assert(expr    != NULL);
   assert(f_name  != NULL);
 
   if (*e_code != lxec_noerror) return(result);

   last_spos = *expr;

   *expr = la_get_token(*expr, &lex);

   if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_left))
   {
      *e_code  = lxec_no_oph;
      return(result);
   }

   last_spos   = *expr;
   *expr       = la_get_token(*expr, &lex);

   while (true)
   {
      if ((lex.type == lxt_bracket) && (lex.data.brk_type == bt_right)) break;
      else *expr = last_spos;

      result = sa_term(expr, MIN_PRIO, fmd, f_skip, e_code);
      if (*e_code != lxec_noerror) return(result);

      *expr = la_get_token(*expr, &lex);

      // Argument.
      if (!f_skip)
      {
         if (add_farg_list(&arg_list, result) == false)
         {
            *e_code = lxec_farg_ae;
            free_farg_list(&arg_list);
            return(result);
         }
      }

      if ((lex.type == lxt_bracket) && (lex.data.brk_type == bt_right)) break;

      if (!*expr || (lex.type == lxt_eol))
      {
         *e_code  = lxec_no_cph;
         free_farg_list(&arg_list);
         return(result);
      }
      else if (lex.type != lxt_comma)
      {
         *e_code  = lxec_farg_nocomma;
         free_farg_list(&arg_list);
         return(result);
      }
      last_spos = *expr;

   }
   // Function running.
   if (!f_name->ndata)
   {
      *expr    = last_spos;
      *e_code  = lxec_func_nodef;
   }
   else
   {
      if (!f_skip) result = ((f_func)f_name->ndata)(arg_list, e_code);
   }
   
   free_farg_list(&arg_list);
   return(result);
}
//------------------------------------------------------------------------------
static
long int sa_ph_expression(char                           **expr,
                          struct t_full_msgdata          *fmd,
                          const bool                     f_skip,
                          t_lex_ecode                    *e_code)
{
   struct t_lexeme   lex;
   char              *last_spos  = *expr;
   long int          result      = 0;

   assert(e_code  != NULL);
   assert(expr    != NULL);
   assert(fmd     != NULL);

   if (*e_code != lxec_noerror) return(result);

   last_spos   =  *expr;
   *expr       =  la_get_token(*expr, &lex);
   if (*e_code != lxec_noerror) return(result);


   switch (lex.type)
   {
      case lxt_bracket:
         if (lex.data.brk_type == bt_left)
         {

            result = sa_term(expr, MIN_PRIO, fmd, f_skip, e_code);
            if (*e_code == lxec_noerror)
            {
               *expr = la_get_token(*expr, &lex);
               if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_right))
               {
                  *expr    = last_spos;
                  *e_code  = lxec_no_cph;
               }
            }
         }
         else
         {
            *e_code = lxec_ue_ph;
         }
      break;
      case lxt_op:
         switch (lex.data.op)
         {
            case op_un_bw_not:
               result = sa_ph_expression(expr, fmd, f_skip, e_code);
               if (f_skip) break;
               result = ~result;
            break;
            case op_un_log_not:
               result = sa_ph_expression(expr, fmd, f_skip, e_code);
               if (f_skip) break;
               result = !result;
            break;
            case op_minus:
               result = sa_ph_expression(expr, fmd, f_skip, e_code);
               if (f_skip) break;
               result = -result;
            break;
            case op_size:
            case op_offset:
            case op_exists:
               result = sa_ph_un_op(expr, lex.data.op, fmd->msg, f_skip, e_code);
            break;
            case op_unknown:
            default:
               *e_code  = lxec_unknown_op;
//               fprintf(stderr, "Operation not recognized!\n");
         }
      break;
      case lxt_name:
         if (f_skip)
         {
            //dfree(lex.data.name.value);
            break;
         }
         result = sa_get_field_value(&lex.data.name, fmd, e_code);
         // Freed strndup allocated (in sa_get_name) memory.
         //dfree(lex.data.name.value);
      break;
      case lxt_function:
         result = sa_run_function(expr, &lex.data.name, fmd, f_skip, e_code);
      break;
      case lxt_number:
         result = lex.data.num;
      break;
      case lxt_eol:
         *e_code  = lxec_ue_eol;
      break;
      case lxt_unknown:
      default:
         *expr    = skip_spaces(last_spos) + 1;
         *e_code  = lxec_unknown_lexeme;
   }

   return(result);
}
//------------------------------------------------------------------------------
char *lxn_decode_err(const t_lex_ecode e_code)
{
   switch(e_code)
   {
      case lxec_noerror:
         return(_("no error"));
      break;
      case lxec_ue_eol:
         return(_("unexpected end of line"));
      break;
      case lxec_general:
         return(_("general error"));
      break;
      case lxec_unknown_lexeme:
         return(_("unknown lexeme"));
      break;
      case lxec_unknown_op:
         return(_("unknown operation"));
      break;
      case lxec_ue_ph:
         return(_("unexpected bracket"));
      break;
      case lxec_no_oph:
         return(_("no opening bracket"));
      break;
      case lxec_no_cph:
         return(_("no trailing bracket"));
      break;
      case lxec_not_field:
         return(_("operator argument is not a field"));
      break;
      case lxec_func_nodef:
         return(_("function code is not defined"));
      break;
      case lxec_func_ie:
         return(_("function internal error"));
      break;
      case lxec_farg_ae:
         return(_("function argument adding error"));
      break;
      case lxec_farg_ufw:
         return(_("function needs more arguments"));
      break;
      case lxec_farg_ofw:
         return(_("function needs less arguments"));
      break;
      case lxec_farg_nocomma:
         return(_("need comma between arguments"));
      break;
      case lxec_trailing_chars:
         return(_("characters after the end of expr"));
      break;
      case lxec_field_not_found:
         return(_("field was not found"));
      break;
      case lxec_end:
         return(_("end of parser error codes list [stub]"));
      break;
      default:
         return(_("unknown error"));
   }
   return(NULL);
}
//------------------------------------------------------------------------------
static
__inline void lxn_print_error(const char *st_expr, const char *cur_expr,
                              const t_lex_ecode e_code)
{
   assert(st_expr    != NULL);
   assert(cur_expr   != NULL);

   fprintf(stderr, _("Error: %s\n"), lxn_decode_err(e_code));
   fprintf(stderr, "%s\n%*c\n", st_expr, cur_expr - st_expr, '^');
}
//------------------------------------------------------------------------------
__inline bool lxn_print_if_err(const struct t_la_control_block *param)
// Return true, if error during parsing was occured.
// False, if all is ok.
{
   assert(param != NULL);

   if (param->e_code == lxec_noerror) return(false);
   lxn_print_error(param->pos_start, param->pos_end, param->e_code);
   return(true);
}
//------------------------------------------------------------------------------
__inline
struct t_la_control_block *lxn_init_param(char                       *expr,
                                          struct t_full_msgdata      *fmd,
                                          struct t_la_control_block  *param)
{
   assert(param != NULL);

   param->pos_start  = expr;
   param->fmd        = fmd;
   return(param);
}
//------------------------------------------------------------------------------
long int lxn_analyse(struct t_la_control_block *param)
// Return calculated value, if param->e_code == lxec_noerror.
{
   struct t_lexeme   lex;
   long int          result = 0;

   assert(param != NULL);

   param->pos_end = param->pos_start;
   param->e_code  = lxec_noerror;

   result         = sa_term(&param->pos_end, MIN_PRIO, param->fmd,
                            false, &param->e_code);

   if (param->e_code != lxec_noerror) return(result);

   param->pos_end = la_get_token(param->pos_end, &lex);
   if (lex.type != lxt_eol) param->e_code = lxec_trailing_chars;

   return(result);
}
//------------------------------------------------------------------------------

