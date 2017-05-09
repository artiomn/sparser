//------------------------------------------------------------------------------
#include <ctype.h>
#include <string.h>
//------------------------------------------------------------------------------
#include "common.h"
#include "lexan.h"
#include "lexan_funcs.h"
#include "utils.h"
//------------------------------------------------------------------------------
typedef enum
{
   lxt_unknown,
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
   op_un_bw_not, op_un_log_not, op_size, op_offset
} t_operation;
//------------------------------------------------------------------------------
typedef enum { bt_left, bt_right } t_bracket_type;
//------------------------------------------------------------------------------
typedef enum { nt_unknown, nt_ident, nt_metafield, nt_op, nt_function }
   t_name_type;
//------------------------------------------------------------------------------
const t_operation op_so    = op_size;
const t_operation op_ofs   = op_offset;
//------------------------------------------------------------------------------
typedef long int (*f_op_function)(const long int arg0, const long int arg1,
                                  const t_operation op);
//------------------------------------------------------------------------------
#define MIN_PRIO 0
//------------------------------------------------------------------------------
struct op_action
{
   int            prio;
//   t_operation    op;
   f_op_function  func;
}
operations[] =
{
   [op_mul]       = { MIN_PRIO,     NULL },
   [op_div]       = { MIN_PRIO,     NULL },
   [op_mod]       = { MIN_PRIO,     NULL },
   [op_plus]      = { MIN_PRIO + 1, NULL },
   [op_minus]     = { MIN_PRIO + 1, NULL },
   [op_lshift]    = { MIN_PRIO + 2, NULL },
   [op_rshift]    = { MIN_PRIO + 2, NULL },
   [op_lt]        = { MIN_PRIO + 3, NULL },
   [op_gt]        = { MIN_PRIO + 3, NULL },
   [op_ge]        = { MIN_PRIO + 3, NULL },
   [op_le]        = { MIN_PRIO + 3, NULL },
   [op_eq]        = { MIN_PRIO + 4, NULL },
   [op_ne]        = { MIN_PRIO + 4, NULL },
   [op_bw_and]    = { MIN_PRIO + 5, NULL },
   [op_bw_xor]    = { MIN_PRIO + 6, NULL },
   [op_bw_or]     = { MIN_PRIO + 7, NULL },
   [op_log_and]   = { MIN_PRIO + 8, NULL },
   [op_log_or]    = { MIN_PRIO + 9, NULL },
};
//------------------------------------------------------------------------------
struct t_name
// Name tables.
{
   char        *value;
   t_name_type nt;
   void        *ndata;
}
// Predefined names.
name_table_fixed[] =
{
   { "sizeof", nt_op,         (void*)&op_so     },
   { "offset", nt_op,         (void*)&op_ofs    },
   { "abs",    nt_function,   &la_abs           },
   { "pow",    nt_function,   &la_pow           },
   { NULL,     nt_unknown,    NULL              }
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
struct t_name *lookup_name_in_table(const char *name, struct t_name table[])
// Return structure woth name description or NULL, if name not found.
{
   unsigned i;

   if (!table) return(NULL);
   for (i = 0; /*(i < (sizeof(table) / sizeof(struct t_name))) && */table[i].value; i++)
   {
      if (streq(name, table[i].value)) return(&table[i]);
   }
   return(NULL);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Lexical analyzer.
//------------------------------------------------------------------------------

char* skip_spaces(char *expr)
// Skip spaces.
// Return first non space symbol, if ok or NULL, if error or end of expr.
{
   while (expr && isspace(*expr)) expr++;
   return(expr);
}
//------------------------------------------------------------------------------
__inline bool iscomma(const char c)
{
   return(c == ',');
}
//------------------------------------------------------------------------------
__inline bool isoctal(const char c)
{
   return((c >= '0') && (c <= '7'));
}
//------------------------------------------------------------------------------
__inline bool is_first_namesym(const char c)
{
   return(isalpha(c) || (c == '_'));
}
//------------------------------------------------------------------------------
__inline bool is_namesym(const char c)
{
   return(isalnum(c) || (c == '_'));
}
//------------------------------------------------------------------------------
__inline bool isbracket(const char c)
{
   return((c == '(') || (c == ')'));
}
//------------------------------------------------------------------------------
__inline bool is_metafield_fs(const char c)
{
   return(c == '$');
}
//------------------------------------------------------------------------------
__inline bool is_opsymbol(const char c)
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
char *get_op(char *expr, struct t_lexeme *op)
// Return operation in op parameter.
// Return value - next symbol after operation, or error symbol position.
// If error occured, op->type == lxt_unknown (and op->data.op == op_unknown).
{
   char *result = expr;

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
char *get_number(char *expr, struct t_lexeme *num)
// Return number in num->data.num.
// Return value - next position after number, if ok or error position, if error.
// If error is occured, op->type == lxt_unknown,
{
   char *result = expr;

   num->type = lxt_number;
   
   if (*expr == '0')
   // Hex, bin or octal.
   {
      if (!(++expr) || (*expr == '\0')) num->data.num = 0;
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
char *get_name(char *expr, struct t_lexeme *name)
// Return string in name->data.name or operation, if name is operation.
//
{
   char           *result = expr;
   bool           is_mf;
   unsigned       st_len;
   struct t_name  *lookup_name;
   
   name->type = lxt_unknown;

   if (!expr) return(NULL);
   is_mf = is_metafield_fs(*expr);
   if (!(is_first_namesym(*expr) || is_mf)) return(expr);
//   name->type = lxt_name;
   while (++result && is_namesym(*result)) ;
   st_len = result - expr;
   if (st_len)
   {
      name->data.name.value = (char*)strndup(expr, st_len);
      if (is_mf)
      // Name is metafield.
      {
         name->type           = lxt_name;
         name->data.name.nt   = nt_metafield;
      }
      else if ((lookup_name =
                lookup_name_in_table(name->data.name.value, name_table_fixed)) != NULL)
      {
         // Name is operation.
         if (lookup_name->nt == nt_op)
         {
            free(name->data.name.value);
            name->type     = lxt_op;
            name->data.op  = *(t_operation*)(lookup_name->ndata);
         }
         // Name is a function or identifier (field).
         else
         {
            name->data.name.nt      = lookup_name->nt;
            name->data.name.ndata   = lookup_name->ndata;

            if (name->data.name.nt == nt_function) name->type = lxt_function;
         }
      }
      else
      // Name was not found in lookup table.
      {
         name->type              = lxt_name;
         name->data.name.nt      = nt_ident;
      }
   }
   return(result);
}
//------------------------------------------------------------------------------
char *get_bracket(char *expr, struct t_lexeme *bt)
// Return bracket in bt->data.brk_type.
// Return value - next position after bracket, if ok or error position, if error.
// If error is occured, bt->type == lxt_unknown,
{
   if (!(expr && isbracket(*expr)))
   {
      bt->type = lxt_unknown;
      return(expr);
   }

   bt->type          = lxt_bracket;
   bt->data.brk_type = (*expr == '(') ? bt_left : bt_right;
   return(++expr);
}
//------------------------------------------------------------------------------
__inline char *get_comma(char *expr, struct t_lexeme *comma)
{
   if (!(expr && iscomma(*expr)))
   {
      comma->type = lxt_unknown;
      return(expr);
   }

   comma->type = lxt_comma;
   return(++expr);
   
}
//------------------------------------------------------------------------------
char *get_token(char *expr, struct t_lexeme *lexeme)
// Fill lexeme structure.
{
   char *result = skip_spaces(expr);
   
   if (isbracket(*result))
   {
      result = get_bracket(result, lexeme);
   }
   else if (isdigit(*result))
   // Number (octal, decimal, hex or binary).
   // Number always starts from decimal digit.
   {
      result = get_number(result, lexeme);
   }
   else if (is_opsymbol(*result))
   {
      result = get_op(result, lexeme);
   }
   else if (is_first_namesym(*result) || is_metafield_fs(*expr))
   // Identifier, operator word, function or metafield.
   {
      result = get_name(result, lexeme);
   }
   else if (iscomma(*result))
   {
      result = get_comma(result, lexeme);
   }
   else
   // 
   {
      lexeme->type = lxt_unknown;
   }
   return(result);
}

//------------------------------------------------------------------------------
// Syntax analyzer.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
long int sa_ph_expression(char **expr);
//------------------------------------------------------------------------------
/*bool sa_term_0(char **expr, long int *result, const t_operation op)
{
   switch (op)
   {
      case op_mul:
         *result *= sa_ph_expression(expr);
      break;
      case op_div:
         *result /= sa_ph_expression(expr);
      break;
      case op_mod:
         *result %= sa_ph_expression(expr);
      break;
      default:
         return(false);
   }
   return(true);
}
//------------------------------------------------------------------------------
long int sa_run_term(char **expr,
                     bool (*term_func)
                          (char **expr, long int *result, const t_operation op))
{
   
}*/
//------------------------------------------------------------------------------
long int sa_term0(char **expr)
// Mul, div, mod ('*', '/', '%').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_ph_expression(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if (lex.type == lxt_op)
      {
         switch (lex.data.op)
         {
            case op_mul:
               result *= sa_ph_expression(expr);
            break;
            case op_div:
               result /= sa_ph_expression(expr);
            break;
            case op_mod:
               result %= sa_ph_expression(expr);
            break;
            default:
               *expr = last_spos;
               return(result);
         }
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
long int sa_term1(char **expr)
// Plus or minus ('+', '-').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term0(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);
      if (lex.type == lxt_op)
      {
         switch (lex.data.op)
         {
            case op_plus:
               result += sa_term0(expr);
            break;
            case op_minus:
               result -= sa_term0(expr);
            break;
            default:
               *expr = last_spos;
               return(result);
         }
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
long int sa_term2(char **expr)
// Shifts ('<<' or '>>').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term1(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if (lex.type == lxt_op)
      {
         switch (lex.data.op)
         {
            case op_lshift:
               result <<= sa_term2(expr);
            break;
            case op_rshift:
               result >>= sa_term2(expr);
            break;
            default:
               *expr = last_spos;
               return(result);
         }
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
long int sa_term3(char **expr)
// Comparison ('>=', '<=', '<', '>').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term2(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if (lex.type == lxt_op)
      {
         switch (lex.data.op)
         {
            case op_gt:
               result = result > sa_term2(expr);
            break;
            case op_lt:
               result = result < sa_term2(expr);
            break;
            case op_le:
               result = result <= sa_term2(expr);
            break;
            case op_ge:
               result = result >= sa_term2(expr);
            break;
            default:
               *expr = last_spos;
               return(result);
         }
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
long int sa_term4(char **expr)
// Comparison ('==' or '!=').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term3(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if (lex.type == lxt_op)
      {
         switch (lex.data.op)
         {
            case op_eq:
               result = result == sa_term3(expr);
            break;
            case op_ne:
               result = result != sa_term3(expr);
            break;
            default:
               *expr = last_spos;
               return(result);
         }
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
long int sa_term5(char **expr)
// Bitwise AND ('&').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term4(expr);
   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if ((lex.type == lxt_op) && (lex.data.op == op_bw_and))
      {
         result &= sa_term4(expr);
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
long int sa_term6(char **expr)
// Bitwise XOR ('^').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term5(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if ((lex.type == lxt_op) && (lex.data.op == op_bw_xor))
      {
         result ^=  sa_term5(expr);
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
long int sa_term7(char **expr)
// Bitwise OR ('|').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term6(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if ((lex.type == lxt_op) && (lex.data.op == op_bw_or))
      {
         result |=  sa_term6(expr);
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
long int sa_term8(char **expr)
// Logical AND ('&&').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term7(expr);

   while (true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if ((lex.type == lxt_op) && (lex.data.op == op_log_and))
      {
         result = result && sa_term7(expr);
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
long int sa_term9(char **expr)
// Logical OR ('||').
// Return calculated value.
{
   char              *last_spos  = *expr;
   long int          result      = 0;
   struct t_lexeme   lex;

   result = sa_term8(expr);

   while(true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if ((lex.type == lxt_op) && (lex.data.op == op_log_or))
      {
         result = result || sa_term8(expr);
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

long int make_operation(const long int arg0, const long int arg1,
                        const t_operation op);
//------------------------------------------------------------------------------
long int sa_term(char **expr, const int prio)
{
   char              *last_spos  = *expr;
   int               high_prio   = prio - 1;
   long int          result      = 0;
   struct t_lexeme   lex;
   
   result = (high_prio >= MIN_PRIO) ? sa_term(expr, high_prio) : sa_ph_expression(expr);

   while(true)
   {
      last_spos   = *expr;
      *expr       = get_token(*expr, &lex);

      if ((lex.type == lxt_op) && (operations[lex.data.op].prio == prio))
      {
         result = make_operation(result,
                     (high_prio >= MIN_PRIO) ?
                        sa_term(expr, high_prio) : sa_ph_expression(expr),
                     lex.data.op);
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
long int make_operation(const long int arg0, const long int arg1,
                        const t_operation op)
{
   long int result;

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
   }
   return(result);
}
//------------------------------------------------------------------------------
long int sa_ph_un_op(char **expr, const t_operation op)
// Prefix operator with parenthesis.
{
   struct t_lexeme   lex;
   char              *last_spos  = *expr;
   long int          result = 0;
   
   *expr = get_token(*expr, &lex);

   if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_left))
   {
      *expr = last_spos;
      fprintf(stderr, "Error: no opening parenthesys after operator!\n");
      return(result);
   }

   *expr = get_token(*expr, &lex);
   if ((lex.type != lxt_name) || (lex.data.name.nt != nt_ident))
   {
      *expr = last_spos;
      fprintf(stderr, "Operator allowed only for the fields!\n");
      return(result);
   }

   switch (op)
   {
      case op_size:
      break;
      case op_offset:
      break;
      default:
         fprintf(stderr, "Unknown operator code!\n");
   }

   if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_right))
   {
      *expr = last_spos;
      fprintf(stderr, "Unclosed bracket after operator!\n");
      return(result);
   }
   return(result);
}
//------------------------------------------------------------------------------
long int sa_run_function(char **expr, struct t_name *f_name)
{
   struct t_lexeme   lex;
   char              *last_spos  = *expr, *lp;
   long int          result      = 0;
   struct t_func_arg *arg_list   = NULL;
   
   *expr = get_token(*expr, &lex);
   
   if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_left))
   {
      *expr = last_spos;
      fprintf(stderr, "Error: no opening parenthesys after function!\n");
      return(result);
   }

   lp    = *expr;
   *expr = get_token(*expr, &lex);

   while (true)
   {
      if ((lex.type == lxt_bracket) && (lex.data.brk_type == bt_right)) break;
      else *expr = lp;

      result = sa_term(expr, 9);

      *expr = get_token(*expr, &lex);

      // Argument.
      if (add_farg_list(&arg_list, result) < 0)
      {
         fprintf(stderr, "Argument adding error!\n");
      };

      if ((lex.type == lxt_bracket) && (lex.data.brk_type == bt_right)) break;

      if (!*expr || (**expr == '\0'))
      {
         fprintf(stderr, "No trailing bracket after function!\n");
         *expr = last_spos;
         free_farg_list(&arg_list);
         return(result);
      }
      else if (lex.type != lxt_comma)
      {
         fprintf(stderr, "Error in function arguments!\n");
         *expr = last_spos;
         free_farg_list(&arg_list);
         return(result);
      }
      lp = *expr;

   }
   // Function running.
   if (!f_name->ndata)
   {
      fprintf(stderr, "Function code was not defined!\n");
   }
   else
   {
      result = ((f_func)f_name->ndata)(arg_list);
   }
   
   free_farg_list(&arg_list);
   return(result);
}
//------------------------------------------------------------------------------
long int sa_get_field_value(const char *f_name)
{
   printf("Field: %s\n", f_name);
   return(0);
}
//------------------------------------------------------------------------------
long int sa_ph_expression(char **expr)
{
   struct t_lexeme   lex;
   char              *last_spos  = *expr;
   long int          result      = 0;

   last_spos   = *expr;
   *expr       = get_token(*expr, &lex);
   switch (lex.type)
   {
      case lxt_bracket:
         if (lex.data.brk_type == bt_left)
         {
            result   = sa_term(expr, 9);
            *expr    = get_token(*expr, &lex);

            if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_right))
            {
               fprintf(stderr, "Unclosed bracket!\n");
            }
         }
         else
         {
            fprintf(stderr, "Unexpected bracket!\n");
         }
      break;
      case lxt_op:
         switch (lex.data.op)
         {
            case op_un_bw_not:
               result   = sa_ph_expression(expr);
               result   = ~result;
            break;
            case op_un_log_not:
               result   = sa_ph_expression(expr);
               result   = !result;
            break;
            case op_minus:
               result   = sa_ph_expression(expr);
               result   = -result;
            break;
            case op_size:
            case op_offset:
               result   = sa_ph_un_op(expr, lex.data.op);
            break;
            case op_unknown:
               fprintf(stderr, "Error: unknown operation!\n");
            break;
            default:
               fprintf(stderr, "Operation not recognized!\n");
         }
      break;
      case lxt_name:
         result = sa_get_field_value(lex.data.name.value);
      break;
      case lxt_function:
         result = sa_run_function(expr, &lex.data.name);
      break;
      case lxt_number:
         result = lex.data.num;
      break;
      case lxt_unknown:
      default:
         fprintf(stderr, "Error: unknown lexeme!\n");
   }

   return(result);
}
//------------------------------------------------------------------------------
long int analyse(const char                        *expr,
                 const struct t_message_metadata   *mmd,
                 long int                          *result)
{
   char *cur_spos = expr;
   
   *result = sa_term(&cur_spos, 9);
   return(*result);
}
//------------------------------------------------------------------------------
