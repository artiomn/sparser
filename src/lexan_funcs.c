//------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
//------------------------------------------------------------------------------
#include "lexan_funcs.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Functions for calculated expressions.
long int la_abs(struct t_func_arg *fa, t_lex_ecode *e_code)
{
   struct t_func_arg *a       = fa;
   long int          result   = 0;
   int               argc     = 0;

   assert(e_code != NULL);

   while (a)
   {
      result = a->value;

      if ((++argc) > 1)
      {
         *e_code = lxec_farg_ofw;
         break;
      }

      a = a->next;
   }

   if (argc < 1) *e_code = lxec_farg_ufw;
   else
   {
      result = abs(result);
   }
   return(result);
}
//------------------------------------------------------------------------------
#if HAVE_POW == 1
long int la_pow(struct t_func_arg *fa, t_lex_ecode *e_code)
{
   struct t_func_arg *a       = fa;
   long int          result   = 0;
   long int          factor   = 0;
   int               argc     = 0;

   assert(e_code != NULL);

   while (a)
   {

      if (argc == 0)
      {
         result = a->value;
      }
      else if (argc == 1)
      {
         factor = a->value;
      }
      else
      {
         *e_code = lxec_farg_ofw;
         break;
      }
      argc++;
      a = a->next;
   }

   if (argc < 2) *e_code = lxec_farg_ufw;
   else
   {
      result = pow(result, factor);
   }
   return(result);
}
#endif
//------------------------------------------------------------------------------
long int la_min(struct t_func_arg *fa, t_lex_ecode *e_code)
{
   struct t_func_arg *a       = fa;
   long int          result   = 0;
   int               argc     = 0;

   assert(e_code != NULL);

   while (a)
   {
      if (argc == 0) result = a->value;
      else result = (a->value < result) ? a->value : result;
      argc++;
      a = a->next;
   }

   if (argc < 1) *e_code = lxec_farg_ufw;
   return(result);
}
//------------------------------------------------------------------------------
long int la_max(struct t_func_arg *fa, t_lex_ecode *e_code)
{
   struct t_func_arg *a       = fa;
   long int          result   = 0;
   int               argc     = 0;

   assert(e_code != NULL);

   while (a)
   {
      if (argc == 0) result = a->value;
      else result = (a->value > result) ? a->value : result;
      argc++;
      a = a->next;
   }

   if (argc < 1) *e_code = lxec_farg_ufw;
   return(result);
}
//------------------------------------------------------------------------------
long int la_time(struct t_func_arg *fa, t_lex_ecode *e_code)
{
   struct t_func_arg *a       = fa;
   long int          result   = 0;
   int               argc     = 0;
   struct tm         *ts      = NULL;
   time_t            cur_time = 0;

   assert(e_code != NULL);

   #ifndef _WIN32
   if ((ts = dmalloc(sizeof(struct tm))) == NULL)
   {
      *e_code = lxec_func_ie;
      return(result);
   }
   #endif

/*   if (!time(&cur_time))
   {
      *e_code = lxec_func_ie;
      dfree(ts);
      return(result);
   }*/

   #ifdef _WIN32
   // On Windows gmtime is threadsafe.
   if (ts = localtime(&cur_time) == NULL)
   #else
   if (localtime_r(&cur_time, ts) == NULL)
   #endif
   {
      *e_code = lxec_func_ie;
      dfree(ts);
      return(result);
   }

   assert(ts != NULL);

   while (a)
   {
      switch (argc)
      {
         case 0:
            // Year, since 1900.
            ts->tm_year  = a->value - 1900;
            // 2 digit years, since 2000.
            if (ts->tm_year < 0) ts->tm_year = a->value + 100;
         break;
         case 1:
            // Month in range 0..11.
            ts->tm_mon   = a->value - 1;
         break;
         case 2:
            // 1..31
            ts->tm_mday  = a->value;
         break;
         case 3:
            // 0..23
            ts->tm_hour  = a->value;
         break;
         case 4:
            ts->tm_min   = a->value;
         break;
         case 5:
            ts->tm_sec   = a->value;
         break;
      }
      argc++;
      a = a->next;
   }

   // Ignored by mktime.
   //ts->tm_wday = 0;
   //ts->tm_yday = 0;

   if (argc < 0) *e_code = lxec_farg_ufw;
   else if (argc > 6) *e_code = lxec_farg_ofw;
   tzset();
   result = mktime(ts);
//   fprintf(stderr, "y: %d, m: %d, d: %d, result: %d %s",
//           ts->tm_year, ts->tm_mon, ts->tm_mday, result, asctime(ts));
   dfree(ts);
   return(result);
}

//------------------------------------------------------------------------------
// Service functions.
//------------------------------------------------------------------------------

bool add_farg_list(struct t_func_arg **arg_list, const long int arg)
// 
{
   struct t_func_arg *a = dmalloc(sizeof(struct t_func_arg)), *cur_node = NULL;

   assert(arg_list != NULL);

   if (!a) return(false);

   a->next  = NULL;
   a->value = arg;

   if (!(*arg_list))
   {
      *arg_list = a;
      return(true);
   }
   else
   {
      for (cur_node = *arg_list; (cur_node->next != NULL); cur_node = cur_node->next);
   }

   cur_node->next = a;
   return(true);
}
//------------------------------------------------------------------------------
void free_farg_list(struct t_func_arg **arg_list)
{
   struct t_func_arg *a;

   assert(arg_list != NULL);

   a = *arg_list;

   while (a)
   {
      a = a->next;
      dfree(*arg_list);
      *arg_list = a;
   }
   *arg_list = NULL;
}
//------------------------------------------------------------------------------

