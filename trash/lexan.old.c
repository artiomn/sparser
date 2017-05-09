//------------------------------------------------------------------------------
#include <ctype.h>
#include <string.h>
//------------------------------------------------------------------------------
#include "lexan.h"
#include "common.h"
#include "utils.h"
//------------------------------------------------------------------------------
typedef enum
{
   lxt_unknown, lxt_bracket, lxt_op, lxt_name, lxt_function, lxt_number
} t_lexeme_type;
//------------------------------------------------------------------------------
typedef enum
{
   op_unknown,
   op_mul, op_div, op_mod, op_plus, op_minus, op_lshift, op_rshift, op_lt, op_gt,
   op_ge, op_le, op_eq, op_ne, op_bw_and, op_bw_xor, op_bw_or, op_log_and,
   op_log_or, /*op_un_minus,*/ op_un_bw_not, op_un_log_not, op_size, op_offset
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
struct t_name
// Names tables.
{
   char        *value;
   t_name_type nt;
   void        *ndata;
}
// Predefined names.
name_table_fixed[] =
{
   { "sizeof", nt_op, (void*)&op_so },
   { "offset", nt_op, (void*)&op_ofs },
   { NULL, nt_unknown }
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
      if (streq(name, table[i].value)) return(&table[i]);
   return(NULL);
}

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
      name->data.name.value = malloc(st_len + 1);
      strncpy(name->data.name.value, expr, st_len);
      // May be 2 '\0' symbols in the end.
      name->data.name.value[st_len] = '\0';

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
         else name->data.name.nt = lookup_name->nt;
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
char *ph_expression(char *expr, long int *result);
//------------------------------------------------------------------------------
long int term0(char *expr, char **result)
// Mul, div, mod ('*', '/', '%').
// op - operation type.
// Return calculated value.
// Result - position after right operand.
{
   char              *cur_spos   = expr;
   char              *last       = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = ph_expression(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if (lex.type == lxt_op)
   {
      switch (lex.data.op)
      {
         case op_mul:
            cur_spos =  term0(cur_spos, &right_op);
            *result  *= right_op;
         break;
         case op_div:
            cur_spos =  term0(cur_spos, &right_op);
            *result  /= right_op;
         break;
         case op_mod:
            cur_spos =  term0(cur_spos, &right_op);
            *result  %= right_op;
         break;
         default:
//            cur_spos = ph_expression(last, result);
            return(expr);
      }
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term1(char *expr, long int *result)
// Plus or minus ('+', '-').
// op - operation type.
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term0(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);
   printf("Term1: result = %d, type = %d, op = %d\n", *result, lex.type, lex.data.op);

   if (lex.type == lxt_op)
   {
      switch (lex.data.op)
      {
         case op_plus:
            cur_spos =  term1(cur_spos, &right_op);
            *result  += right_op;
         break;
         case op_minus:
            cur_spos =  term1(cur_spos, &right_op);
            printf("Term1 (minus): result = %d, right_op = %d\n", *result, right_op);
            *result  -= right_op;
            printf("Term1 (after minus): result = %d\n", *result);
         break;
         default:
//            cur_spos = term0(expr, result);
            return(expr);
      }
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term2(char *expr, long int *result)
// Shifts ('<<' or '>>').
// op - operation type.
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term1(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if (lex.type == lxt_op)
   {
      switch (lex.data.op)
      {
         case op_lshift:
            cur_spos =     term2(cur_spos, &right_op);
            *result  <<=   right_op;
         break;
         case op_rshift:
            cur_spos =     term2(cur_spos, &right_op);
            *result  >>=   right_op;
         break;
         default:
//            cur_spos = term1(expr, result);
            return(expr);
      }
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term3(char *expr, long int *result)
// Comparison ('>=', '<=', '<', '>').
// op - operation type.
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term2(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if (lex.type == lxt_op)
   {
      switch (lex.data.op)
      {
         case op_gt:
            cur_spos = term3(cur_spos, &right_op);
            *result  = *result > right_op;
         break;
         case op_lt:
            cur_spos = term3(cur_spos, &right_op);
            *result  = *result < right_op;
         break;
         case op_le:
            cur_spos = term3(cur_spos, &right_op);
            *result  = *result <= right_op;
         break;
         case op_ge:
            cur_spos = term3(cur_spos, &right_op);
            *result  = *result >= right_op;
         break;
         default:
//            cur_spos = term2(expr, result);
            return(expr);
      }
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term4(char *expr, long int *result)
// Comparison ('==' or '!=').
// op - operation type.
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term3(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if (lex.type == lxt_op)
   {
      switch (lex.data.op)
      {
         case op_eq:
            cur_spos = term4(cur_spos, &right_op);
            *result  = *result == right_op;
         break;
         case op_ne:
            cur_spos = term4(cur_spos, &right_op);
            *result  = *result != right_op;
         break;
         default:
//            cur_spos = term3(expr, result);
            return(expr);
      }
   }
//   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term5(char *expr, long int *result)
// Bitwise AND ('&').
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term4(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if ((lex.type == lxt_op) && (lex.data.op == op_bw_and))
   {
      cur_spos =  term5(cur_spos, &right_op);
      *result  &= right_op;
   }
   else return(expr);
//   else cur_spos = term4(expr, result);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term6(char *expr, long int *result)
// Bitwise XOR ('^').
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term5(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if ((lex.type == lxt_op) && (lex.data.op == op_bw_xor))
   {
      cur_spos =  term6(cur_spos, &right_op);
      *result  ^= right_op;
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term7(char *expr, long int *result)
// Bitwise OR ('|').
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term6(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if ((lex.type == lxt_op) && (lex.data.op == op_bw_or))
   {
      cur_spos =  term7(cur_spos, &right_op);
      *result  |= right_op;
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term8(char *expr, long int *result)
// Logical AND ('&&').
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term7(cur_spos, result);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if ((lex.type == lxt_op) && (lex.data.op == op_log_and))
   {
      cur_spos = term8(cur_spos, &right_op);
      *result  = *result && right_op;
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *term9(char *expr, long int *result)
// Logical OR ('||').
// Return calculated value in 'result'.
// Return value: position after right operand.
{
   char              *cur_spos   = expr;
   long int          right_op    = 0;
   struct t_lexeme   lex;

   cur_spos = term8(cur_spos, result);
//   printf("X = %d\nPOS: %s\n", *result, cur_spos);
   expr     = cur_spos;
   cur_spos = get_token(cur_spos, &lex);

   if ((lex.type == lxt_op) && (lex.data.op == op_log_or))
   {
      cur_spos = term9(cur_spos, &right_op);
      *result  = *result || right_op;
   }
   else return(expr);

   return(cur_spos);
}
//------------------------------------------------------------------------------
char *ph_expression(char *expr, long int *result)
{
   struct t_lexeme lex;
   char *cur_spos    = expr;
   char *last_spos   = expr;
   long int right_op = 0;

   if (((cur_spos = get_token(cur_spos, &lex)) != NULL))
   {
//              printf("S: %s\nL: %d\n", cur_spos, lex.type);
      switch (lex.type)
      {
// printf("BRK: %d\n", lex.type);
         case lxt_bracket:
            
            if (lex.data.brk_type == bt_left)
            {
               cur_spos = term9(cur_spos, result);
//               printf("S: %s\n", cur_spos);
               cur_spos = get_token(cur_spos, &lex);
/*               if ((lex.type != lxt_bracket) || (lex.data.brk_type != bt_right))
               {
                  fprintf(stderr, "Unclosed bracket!\n");
                  return(NULL);
               }*/
            }
            else
            {
               fprintf(stderr, "Unexpected bracket!\n");
               return(NULL);
            }
         break;
         case lxt_op:
            switch (lex.data.op)
            {
               case op_un_bw_not:
                  cur_spos = ph_expression(cur_spos, result);
                  *result  = ~ *result;
               break;
               case op_un_log_not:
                  cur_spos = ph_expression(cur_spos, result);
                  *result  = ! *result;
               break;
               case op_minus:
               printf("Minus\n");
                  cur_spos = ph_expression(cur_spos, result);
                  *result  = - *result;
               break;
               case op_size:
               break;
               case op_offset:
                  fprintf(stderr, "Field offset was not realized!\n");
               break;
               case op_unknown:
                  fprintf(stderr, "Error: unknown operation!\n");
                  return(NULL);
               break;
               default:
//                  cur_spos = term9(last_spos, result);
            }
         break;
         case lxt_name:
            printf("Ident: %s\n", lex.data.name.value);
         break;
         case lxt_function:
            printf("Function: %s()\n", lex.data.name.value);
         break;
         case lxt_number:
            *result = lex.data.num;
//           if (cur_spos && (*cur_spos != '\0')) cur_spos = term9(expr, result);
         break;
         case lxt_unknown:
            fprintf(stderr, "Error: unknown lexeme!\n");
            return(NULL);
         default:
      }
      last_spos = cur_spos;
   }
   return(cur_spos);
}
//------------------------------------------------------------------------------
long int analyse(const char                       *expr,
            const struct t_message_metadata  *mmd,
            long int                         *result)
{
/*   struct t_lexeme op;
   long int r = 0;*/
//   term9(expr, result);
   char *cur_spos = expr;
   
//   while (((cur_spos = term9(cur_spos, result)) != NULL) && (*cur_spos != '\0'))
   {
      term9(expr, result);
   };
//   *result = -1;
   return(0);
}
//------------------------------------------------------------------------------
