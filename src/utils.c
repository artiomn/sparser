//------------------------------------------------------------------------------
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
//------------------------------------------------------------------------------
#include "utils.h"
//------------------------------------------------------------------------------
#if DEBUG_LEVEL >= 3
static long int malloc_count = 0;
#endif
//------------------------------------------------------------------------------
int strlen_utf8(const char *s)
{
   register int i = 0, j = 0;

   assert(s != NULL);

   while (s[i])
   {
     if ((s[i] & 0xC0) != 0x80) j++;
     i++;
   }

   return(j);
}
//------------------------------------------------------------------------------
char* strncpy0(char* dest, const char* src, size_t size)
{
   assert((src != NULL) && (dest != NULL));

   strncpy(dest, src, size);
   dest[size - 1] = '\0';

   return(dest);
}
//------------------------------------------------------------------------------
__inline char* skip_spaces(char *s)
{
   while (s && (*s != '\0') && isspace(*s)) s++;
   return(s);
}
//------------------------------------------------------------------------------
__inline char* rstrip(char* s)
{
   register char *p;

   assert(s != NULL);

   p = s + strlen(s);
   while (p > s && isspace(*--p)) *p = '\0';

   return(s);
}
//------------------------------------------------------------------------------
__inline bool iseol(const char c)
{
   return((c == '\0') || (c == '\n'));
}
//------------------------------------------------------------------------------
__inline bool iscomma(const char c)
{
   return(c == ',');
}
//------------------------------------------------------------------------------
__inline bool is_metafield_fs(const char c)
{
   return(c == '$');
}
//------------------------------------------------------------------------------
__inline bool is_control_fs(const char c)
{
   return(c == '@');
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
__inline bool isoctal(const char c)
{
   return((c >= '0') && (c <= '7'));
}
//------------------------------------------------------------------------------
__inline bool is_left_bracket(const char c)
{
   return(c == '(');
}
//------------------------------------------------------------------------------
__inline bool is_right_bracket(const char c)
{
   return(c == ')');
}
//------------------------------------------------------------------------------
__inline bool isbracket(const char c)
{
   return(is_left_bracket(c) || is_right_bracket(c));
}
//------------------------------------------------------------------------------
float get_num(const char *str)
{
   char           *r_str      = str;
   bool           rrev_flag   = false;
   long int       result      = 0;

   assert(str != NULL);

   if (strstr(str, "1/") - str == 0) r_str += 2, rrev_flag = true;

   if (strstr(r_str, "0b") - r_str == 0)
   // Binary.
   {
      r_str    += 2;
      result   =  strtol(r_str, NULL, 2);
   }
   else result = strtol(r_str, NULL, 0);
   return((rrev_flag) ? 1/(float)result : (float)result);
}
//------------------------------------------------------------------------------
__inline void print_hexdump(const unsigned char *data, const long data_sz,
                            const int bc)
{
   fprint_hexdump(stdout, data, data_sz, bc);
}
//------------------------------------------------------------------------------
void fprint_hexdump(FILE *fs, const unsigned char *data, const long data_sz,
                    const int bc)
{
   register int i;
   register int wl = bc / 5;

   assert(data       != NULL);
   assert(fs         != NULL);
   assert(data_sz    >  0);

   for (i = 0; i < data_sz; i++)
   {
      fprintf(fs, "0x%02X ", data[i]);
      if (wl && (((i + 1) % wl) == 0)) fprintf(fs, "\n");
   }
   return;
}
//------------------------------------------------------------------------------
void fprint_rhexdump(FILE *fs, const unsigned char *data, const long data_sz,
                    const int bc)
{
   register int i;
   register int wl = bc / 5;

   assert(data       != NULL);
   assert(fs         != NULL);
   assert(data_sz    >  0);

   for (i = data_sz - 1; i >= 0; i--)
   {
      fprintf(fs, "0x%02X ", data[i]);
      if (wl && (((i + 1) % wl) == 0)) fprintf(fs, "\n");
   }
   return;
}
//------------------------------------------------------------------------------
static
__inline void fprint_bin_octet(FILE* fs,
                               uint8 byte,
                               char* octet_pair)
{
   register int i;

   assert(fs != NULL);

   for (i = 7; i >= 0; i--)
   {
      octet_pair[(i >= 4) ? i + 1 : i] = (byte & 0x01) ? '1' : '0';
      byte >>= 1;
   }
   
   //data++;
   fprintf(fs, "%s", octet_pair);
}
//------------------------------------------------------------------------------
static
void fprint_bin_internal(FILE* fs, const unsigned char *data,
                         const unsigned int data_sz, const int bc,
                         const bool reverse)
{
   char                    octet_pair[10];
   register int            i;
   register int            wl = bc / 10;

   assert(data       != NULL);
   assert(fs         != NULL);
   assert(data_sz    >  0);

   wl -= (wl > 1) ? 1 : 0;

   octet_pair[9] = '\0';
   octet_pair[4] = ' ';

   //while (data_sz--)
   if (reverse)
   for (i = data_sz - 1; i >= 0; i--)
   {
      fprint_bin_octet(fs, data[i], octet_pair);
      if (i > 0) fputc(' ', fs);
      //if (((data_sz + 1) % 7) == 0) putchar('\n');
      if (wl && (((i + 1) % wl) == 0)) fputc('\n', fs);
   }
   else
   for (i = 0; i < data_sz; i++)
   {
      fprint_bin_octet(fs, data[i], octet_pair);
      if (i < data_sz - 1) fputc(' ', fs);
      if (wl && (((i + 1) % wl) == 0)) fputc('\n', fs);
   }
   return;
}
/*void print_bin(unsigned long int num, short int padding)
{
   char *buf = (char*)dmalloc(sizeof(num) * sizeof(char) + 1);
   unsigned int i = 0;

   if (!buf) return;

   while ((num > 0) || padding)
   {
      buf[i] = (num & 0x1) ? '1' : '0';
//      !(i % 4) && printf(" ");
      if (num) num >>= 1;
      padding--;
      i++;
   }

   buf[i] = 0;

   while (i-- > 0) putchar(buf[i]);

   dfree(buf);
   return;
}*/
//------------------------------------------------------------------------------
__inline void print_bin(const unsigned char *data, const unsigned int data_sz,
                        const int bc)
{
   fprint_bin(stdout, data, data_sz, bc);
}
//------------------------------------------------------------------------------
__inline void fprint_rbin(FILE* fs, const unsigned char *data,
                 const unsigned int data_sz, const int bc)
{
   fprint_bin_internal(fs, data, data_sz, bc, true);
}
//------------------------------------------------------------------------------
__inline void fprint_bin(FILE* fs, const unsigned char *data,
                const unsigned int data_sz, const int bc)
{
   fprint_bin_internal(fs, data, data_sz, bc, false);
}
//------------------------------------------------------------------------------
__inline void  fprint_sym(FILE* fs, const char c, const unsigned int count)
{
/*   unsigned int i = count;
   while (i--) fputc(c, fs); */
   assert(fs != NULL);

   if (count) fprintf(fs, "%*c", count, c);
}
//------------------------------------------------------------------------------
__inline void  print_sym(const char c, const unsigned int count)
{
   fprint_sym(stdout, c, count);
}
//------------------------------------------------------------------------------
void fprint_by_spec(FILE *fs, uint8 *data_ptr, const unsigned data_sz,
                    const char spec, const int dump_lim)
{
   long long               data_value;
   register int            i;
   time_t                  tm;
   char                    ta[255];
   
   if (!data_ptr)
   {
      fprintf(fs, "NULL");
      return;
   }
   
   switch (spec)
   {
      case 'x':
//         data_value = 0;
//         memcpy(&data_value, data_ptr, data_sz);
//         fprintf(fs, "0x%0*X", (int)data_sz * 2, data_value);
         fprintf(fs, "0x");
         for (i = data_sz - 1; i >= 0; i--) fprintf(fs, "%02X", data_ptr[i]);
      break;
      case 'p':
         fprint_rhexdump(fs, data_ptr, data_sz, dump_lim);
      break;
      case 'o':
      
      break;
      case 'u':
         data_value = 0;
         memcpy(&data_value, data_ptr, min(data_sz, sizeof(long long)));
         fprintf(fs, "%u", (unsigned int)data_value);
//         fprintf(fs, "%u", *((unsigned int*)data_ptr));
      break;
      case 'i':
         data_value = 0;
         memcpy(&data_value, data_ptr, min(data_sz, sizeof(long long)));
         fprintf(fs, "%lld", (signed long long int)data_value);
      break;
      case 'b':
//         if ((len * 8) > MAX_PRN_LEN) fputc('\n', msg->mmd->out_file);
         fprintf(fs, "0b");
         fprint_rbin(fs, data_ptr, data_sz, dump_lim);
      break;
      case 'd':
         tm = *(time_t*)(time32*)data_ptr;
         strftime(ta, 255, "%x", localtime(&tm));
         fprintf(fs, "%s",  (strlen(ta)) ? ta : _("not configured"));
      break;
      case 't':
         tm = *(time_t*)(time32*)data_ptr;
         strftime(ta, 255, "%X", localtime(&tm));
         fprintf(fs, "%s",  (strlen(ta)) ? ta : _("not configured"));
      break;
      case 'c':
         for (i = 0; (i < data_sz) && (data_ptr[i] != 0); i++)
            fprintf(fs, "%c", data_ptr[i]);
      break;
      case 'n':
         // Invisible field.
      break;
      default:
         fprintf(stderr, _("Error: unknown output format specifier: \'%c\'\n"), spec);
      break;
   }
}
//------------------------------------------------------------------------------
char *sprintf_by_spec(uint8 *data_ptr, const unsigned data_sz,
                      const char spec)
// Allocate string.
// Memory must be deallocated somewhere.
{
   long long               data_value;
   register int            i;
   time_t                  tm;
   char                    ta[255];
   char                    *result = NULL;
   
   if (!data_ptr) return(strdup(""));
   
   switch (spec)
   {
      case 'x':
      case 'p':
      case 'b':
//         data_value = 0;
//         memcpy(&data_value, data_ptr, data_sz);
//         fprintf(fs, "0x%0*X", (int)data_sz * 2, data_value);

         // String length. +1 - for the '\0'.
         i = 2 + 2 * data_sz + 1;
         if ((result = dmalloc(i)) == NULL) return(NULL);
         sprintf(result, "0x");
         for (i = data_sz - 1; i >= 0; i--) sprintf(result + (data_sz - i) * 2, "%02X", data_ptr[i]);
      break;
      case 'o':
      
      break;
      case 'u':
         data_value = 0;
         memcpy(&data_value, data_ptr, min(data_sz, sizeof(long long)));
         // Digits count.
         i = log10(data_value) + 2;
         if ((result = dmalloc(i)) == NULL) return(NULL);
         sprintf(result, "%u", (unsigned int)data_value);
      break;
      case 'i':
         data_value = 0;
         memcpy(&data_value, data_ptr, min(data_sz, sizeof(long long)));
         // Digits count. 2 - '\0' and possible '-'.
         i = log10(data_value) + 3;
         if ((result = dmalloc(i)) == NULL) return(NULL);
         sprintf(result, "%lld", (signed long long int)data_value);
      break;
/*      case 'b':
//         if ((len * 8) > MAX_PRN_LEN) fputc('\n', msg->mmd->out_file);
         i = 2 + 2 * data_sz + 1;
         sprintf(result, "0b");
//         fprint_rbin(fs, data_ptr, data_sz, dump_lim);
      break;*/
      case 'd':
      case 'm':
         tm = *(time_t*)(time32*)data_ptr;
         strftime(ta, 255, "%x", localtime(&tm));
//         if ((result = dmalloc(strlen(ta) + 1)) == NULL) return(NULL);
//         sprintf(result, "%s",  (strlen(ta)) ? ta : "not configured");
         result = strdup(ta);
      break;
      case 't':
         tm = *(time_t*)(time32*)data_ptr;
         strftime(ta, 255, "%X", localtime(&tm));
         result = strdup(ta);        
      break;
      case 'c':
         result = strndup(data_ptr, data_sz);
      break;
      case 'n':
         // Invisible field.
         return(strdup(""));
      break;
      default:
         fprintf(stderr, _("Error: unknown output format specifier: \'%c\'\n"), spec);
      break;
   }
   return(result);
}
//------------------------------------------------------------------------------
#if !defined(HAVE__)
// gettext() replacement.
__inline char *_(const char *msgid)
{
   #if (defined(HAVE_GETTEXT) || defined(gettext))
      return(gettext(msgid));
   #else
      return(msgid);
   #endif
}
#endif

//------------------------------------------------------------------------------
// Common list routines.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
struct t_common_li *cl_expand(struct t_common_list *list)
// Append fiels to a list.
// It doesn't touch list->cur.
{
   assert(list != NULL);

   struct t_common_li      *li      = list->cur;
   register unsigned int   cur_inum = list->items_in_block;

   if (cur_inum)
   {
      if (!li)
      {
         if ((li = dmalloc(sizeof(struct t_common_li))) == NULL)
            return(NULL);

         li->in_use     = false;
         li->data_ptr   = NULL;
         li->prev       = li->next  = NULL;
         list->first    = list->cur = li;
         cur_inum--;
      }
      else
      while (li->next) li = li->next;

      while (cur_inum-- > 0)
      {
         if ((li->next = dmalloc(sizeof(struct t_common_li))) == NULL)
            return(NULL);
         memzero(li->next, sizeof(struct t_common_li));

         li->next->prev = li;
         li             = li->next;
      }
      list->allocated_items_count += list->items_in_block;
   }

   return(li);
}
//------------------------------------------------------------------------------
static struct t_common_li *cl_destroy_fields(struct t_common_list *list)
// Set list->cur to the last existing item or set cur and first to the NULL,
// if list no more hasn't items after destroing.
{
   struct t_common_li *li, *pli = NULL;

   assert(list != NULL);

   li = list->cur;

   if (li->prev)
   {
      list->cur         = li->prev;
      list->cur->next   = NULL;
   }
   else
   {
      list->first = list->cur = NULL;
   }

   while (li)
   {
      pli   = li;
      li    = li->next;
      dfree(pli);
      list->allocated_items_count--;
   }
   return(list->cur);
}
//------------------------------------------------------------------------------
struct t_common_list *cl_create(const unsigned int items_in_block,
                                f_cl_destructor item_destructor)
{
   register struct t_common_list *result = dmalloc(sizeof(struct t_common_list));

   if (!result) return(NULL);
   memzero(result, sizeof(struct t_common_list));

   if (items_in_block == 0)
   {
      result->items_in_block = 1;
      return(result);
   }

   result->items_in_block = items_in_block;

   if (!cl_expand(result))
   {
      cl_destroy(result);
      return(NULL);
   }

   result->cli_destructor = item_destructor;
   return(result);
}
//------------------------------------------------------------------------------
__inline t_cl_error cl_testerr(struct t_common_list *list)
{
   if (!list) return(clec_nolist);
   if (!(list->cur && list->cur->in_use)) return(clec_empty);

   return(clec_noerr);
}
//------------------------------------------------------------------------------
bool cl_destroy(struct t_common_list *list)
{
   if (!list) return(false);

   list->cur = list->first;

   while (list->cur)
   {
      if (list->cli_destructor && list->cur->in_use)
         list->cli_destructor(list->cur->data_ptr);
      if (list->cur == list->cur->next) return(false);
      list->cur = list->cur->next;
      dfree(list->first);
      list->first = list->cur;
   }

   dfree(list);

   return(true);
}
//------------------------------------------------------------------------------
__inline t_cl_error cl_insert(struct t_common_list *list,
                              void *data_ptr)
{
   if (!list) return(clec_nolist);
   if (!(list->cur || cl_expand(list))) return(clec_nomem);

/*   if (list->cur->in_use)
   {
      if (!cl_expand(list)) return(clec_nomem);
      list->cur = list->cur->next;
   }

   list->cur->in_use    = true;
   list->cur->data_ptr  = data_ptr;
   list->current_items_count++;*/

   return(clec_noerr);
}
//------------------------------------------------------------------------------
__inline void *cl_remove(struct t_common_list *list, t_cl_error *err)
{
/*   t_cl_error     e_code;
   register void  *ptr;

   if (e_code != clec_noerr)
   {
      if (err) *err = e_code;
      return(NULL);
   }

   list->cur->in_use = false;

   if (list->current_items_count > 0) list->current_items_count--;

   // Don't destroy first items_in_block items.
   if (list->items_in_block                                 &&
       (list->current_items_count >= list->items_in_block)  &&
       (list->current_items_count % list->items_in_block == 0)) cl_destroy_fields(list);
   else if (list->cur->prev)
   {
      list->last = list->cur = list->cur->prev;
   }

   if (err) *err = e_code;

   return(ptr);*/
}
//------------------------------------------------------------------------------
__inline t_cl_error cl_push(struct t_common_list *list, void *data_ptr)
{
   if (!list) return(clec_nolist);
   if (!(list->cur || cl_expand(list))) return(clec_nomem);

   while (list->cur->next)
   {
      if (!list->cur->in_use) break;
      list->cur = list->cur->next;
   }

   if (list->cur->in_use)
   {
      if (!cl_expand(list)) return(clec_nomem);
      list->cur = list->cur->next;
   }

   list->last           = list->cur;
   list->cur->in_use    = true;
   list->cur->data_ptr  = data_ptr;
   list->current_items_count++;

   return(clec_noerr);
}
//------------------------------------------------------------------------------
static
__inline struct t_common_li *cl_pick_li(const struct t_common_list *list,
                                        t_cl_error *err)
{
   register t_cl_error e_code = clec_noerr;

   if (!list) e_code = clec_nolist;
   else if (!(list->cur && list->cur->in_use)) e_code = clec_empty;

   if (err) *err = e_code;
   if (e_code != clec_noerr) return(NULL);

   return(list->cur);
}
//------------------------------------------------------------------------------
__inline void *cl_pick(const struct t_common_list *list, t_cl_error *err)
{
   struct t_common_li *cli = cl_pick_li(list, err);

   return((*err == clec_noerr) ? cli->data_ptr : NULL);
}
//------------------------------------------------------------------------------
__inline void *cl_pick_top(const struct t_common_list *list, t_cl_error *err)
{
   t_cl_error           e_code = clec_noerr;
   struct t_common_li   *cli;

   if (!list) e_code = clec_nolist;
   else
   {
      cli = list->cur;
      if (!(cli && cli->in_use)) e_code = clec_empty;
      else
      while (cli->next && cli->next->in_use)
      {
         cli = cli->next;
      }
   }

   if (err) *err = e_code;   
   if (e_code != clec_noerr) return(NULL);

   return(cli->data_ptr);
}
//------------------------------------------------------------------------------
__inline void *cl_pop(struct t_common_list *list, t_cl_error *err)
{
   t_cl_error     e_code;
   register void  *ptr = cl_cur_to_last(list, &e_code);

   if (e_code != clec_noerr)
   {
      if (err) *err = e_code;
      return(NULL);
   }

   list->cur->in_use = false;

   if (list->current_items_count > 0) list->current_items_count--;

   // Don't destroy first items_in_block items.
   if (list->items_in_block                                 &&
       (list->current_items_count >= list->items_in_block)  &&
       (list->current_items_count % list->items_in_block == 0)) cl_destroy_fields(list);
   else if (list->cur->prev)
   {
      list->last = list->cur = list->cur->prev;
   }

   if (err) *err = e_code;

   return(ptr);
}
//------------------------------------------------------------------------------
__inline t_cl_error cl_chg_top(struct t_common_list *list, void *data_ptr)
{
   t_cl_error err;

   cl_cur_to_last(list, &err);
   if (err != clec_noerr) return(err);
   list->cur->data_ptr = data_ptr;

   return(err);
}
//------------------------------------------------------------------------------
__inline bool cl_is_empty(const struct t_common_list *list)
{
   return(!list || (list->current_items_count == 0));
}
//------------------------------------------------------------------------------
static
void* cl_iterate_internal(struct t_common_list  *list,
                          f_cl_iterhandler      handler,
                          void                  *user,
                          t_cl_error            *err,
                          bool                  forward)
{
   register void  *cur_data   = NULL;
   t_cl_error     e_code      = clec_noerr;
//   struct t_common_li   *prev_li    = NULL;

   assert(handler != NULL);

   if (!list) e_code = clec_nolist;
//   if (!handler) e_code = clec_noiterf;

   if (e_code != clec_noerr)
   {
      if (err) *err = e_code;
      return(NULL);
   }
   
//   prev_li = list->cur;
   if (forward) list->cur = list->first;
   else cl_cur_to_last(list, &e_code);

   if (e_code != clec_noerr)
   {
      if (err) *err = e_code;
      return(NULL);
   }
 
   while (list->cur && list->cur->in_use)
   {
      cur_data = list->cur->data_ptr;
      if (!handler(cur_data, user))
      {
         e_code = clec_iterf;
         break;
      }
      list->cur = (forward) ? list->cur->next : list->cur->prev;
   }

   if (err) *err = e_code;

   return(cur_data);
}
//------------------------------------------------------------------------------
__inline void* cl_iterate(struct t_common_list  *list,
                          f_cl_iterhandler      handler,
                          void                  *user,
                          t_cl_error            *err)
{
   return(cl_iterate_internal(list, handler, user, err, true));
}
//------------------------------------------------------------------------------
__inline void* cl_riterate(struct t_common_list *list,
                           f_cl_iterhandler     handler,
                           void                 *user,
                           t_cl_error           *err)
{
   return(cl_iterate_internal(list, handler, user, err, false));
}
//------------------------------------------------------------------------------
__inline void *cl_cur_to_first(struct t_common_list *list, t_cl_error *err)
{
   register t_cl_error e_code = clec_noerr;

   if (!list) e_code = clec_nolist;
   else if (!(list->first && list->first->in_use)) e_code = clec_empty;

   if (err) *err = e_code;
   if (e_code != clec_noerr) return(NULL);
   list->cur = list->first;

   return(list->cur->data_ptr);
}
//------------------------------------------------------------------------------
__inline void *cl_cur_to_last(struct t_common_list *list, t_cl_error *err)
{
   t_cl_error e_code = clec_noerr;

   if (!list) e_code = clec_nolist;
   else
   {
      if (list->last && list->last->in_use) list->cur = list->last;
      else
      {
         if (!(list->cur && list->cur->in_use)) list->cur = list->first;

         if (!(list->cur && list->cur->in_use)) e_code = clec_empty;
         else
         while (list->cur->next && list->cur->next->in_use)
         {
            list->cur = list->cur->next;
         }
      }
   }

   if (err) *err = e_code;
   if (e_code != clec_noerr) return(NULL);

   return(list->cur->data_ptr);
}
//------------------------------------------------------------------------------
__inline void *cl_pick_next(struct t_common_list *list, t_cl_error *err)
{
}
//------------------------------------------------------------------------------
__inline bool cl_have_next(struct t_common_list *list)
{
   assert(list != NULL);
   return(list->cur && list->cur->next && list->cur->next->in_use);
}
//------------------------------------------------------------------------------
__inline bool cl_have_prev(struct t_common_list *list)
{
   assert(list != NULL);
   return(list->cur && list->cur->prev);
}
//------------------------------------------------------------------------------
__inline void *cl_next(struct t_common_list *list, t_cl_error *err)
{
   register t_cl_error e_code = clec_noerr;

   if (!list) e_code = clec_nolist;
   else if (!(list->cur && list->cur->in_use)) e_code = clec_empty;

   if (e_code != clec_noerr)
   {
      if (*err) *err = e_code;
      return(NULL);
   }
 
   if (list->cur->next && list->cur->next->in_use)
   {
      list->cur = list->cur->next;
      if (*err) *err = clec_noerr;
      return(list->cur->data_ptr);
   }

   if (err) *err = clec_last_item;
   return(NULL);
}
//------------------------------------------------------------------------------
__inline void *cl_new_item(struct t_common_list *cl, const size_t item_sz)
{
   void *result = dmalloc(item_sz);

   if (!result) return(NULL);

   if (cl_push(cl, result) != clec_noerr)
   {
      dfree(result);
      return(NULL);
   }

   memzero(result, item_sz);

   return(result);
}
//------------------------------------------------------------------------------
__inline void *cl_find(struct t_common_list  *cl,
                       f_cl_iterhandler      search_func,
                       const void            *data)
{
   t_cl_error err;
   void *result = cl_iterate(cl, search_func, (void*)data, &err);

   return((err != clec_iterf) ? NULL : result);
}
//------------------------------------------------------------------------------
__inline void *cl_rfind(struct t_common_list  *cl,
                       f_cl_iterhandler      search_func,
                       const void            *data)
{
   t_cl_error err;
   void *result = cl_riterate(cl, search_func, (void*)data, &err);

   return((err != clec_iterf) ? NULL : result);
}
//------------------------------------------------------------------------------
__inline int cl_items_count(struct t_common_list *cl)
{
   if (!cl) return(-1);
   return(cl->current_items_count);
}
//------------------------------------------------------------------------------
#if DEBUG_LEVEL >= 3
//------------------------------------------------------------------------------
void *dmalloc(size_t size)
{
   register void *ptr = malloc(size);
   if (ptr)
   {
      malloc_count++;
      fprintf(stderr, "Malloc at address %p (%ld bytes), counter: %ld\n",
              ptr, size, malloc_count);
   }
   else
   {
      fprintf(stderr, "Malloc error, counter: %ld!\n", malloc_count);
   }
   return(ptr);
}
//------------------------------------------------------------------------------
void dfree(void *ptr)
{
   if (ptr)
   {
      malloc_count--;
      fprintf(stderr, "Free at address %p, counter: %ld\n", ptr, malloc_count);
   }
   free(ptr);
}
//------------------------------------------------------------------------------
#endif
