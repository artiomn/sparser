//------------------------------------------------------------------------------
// Common functions.
//------------------------------------------------------------------------------

#ifndef _utils_h
#define _utils_h
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
//------------------------------------------------------------------------------
#include "common.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
struct t_common_li
{
   void                 *data_ptr;
   bool                 in_use;
   struct t_common_li   *prev;
   struct t_common_li   *next;
};
//------------------------------------------------------------------------------
typedef void (*f_cl_destructor)(void *data);
//------------------------------------------------------------------------------
struct t_common_list
{
   struct t_common_li   *first;
   struct t_common_li   *last;
   struct t_common_li   *cur;
   // Items, which allocated with malloc().
   unsigned             allocated_items_count;
   // Items with data.
   unsigned             current_items_count;
   unsigned             items_in_block;
   // Item destructor.
   f_cl_destructor      cli_destructor;
};

//------------------------------------------------------------------------------
// Common list.
//------------------------------------------------------------------------------

// Allocation block in elements.
#define LIST_ITEMS_IN_BLOCK 16
//------------------------------------------------------------------------------
#if !defined(_P)
   #if (defined(HAVE_NGETTEXT) || defined(ngettext))
      #define _P(singular, plural, n) ngettext(singular, plural, n)
   #else
      #define _P(singular, plural, n) char *(void*)(singular, plural, n)
   #endif
#endif
#if !defined(HAVE__)
// gettext() replacement.
__inline char *_(const char *msgid);
#endif
//------------------------------------------------------------------------------
typedef enum { clec_noerr, clec_nolist, clec_nomem, clec_empty, clec_last_item,
               clec_noiterf, clec_iterf, clec_cycle } t_cl_error;
//------------------------------------------------------------------------------
typedef bool (*f_cl_iterhandler)(void *data, void *user);
//------------------------------------------------------------------------------
struct t_common_list *cl_create(const unsigned int items_in_block,
                                f_cl_destructor item_destructor);
//------------------------------------------------------------------------------
#define cl_create_def() cl_create(LIST_ITEMS_IN_BLOCK, NULL)
#define cl_create_defd(destructor) cl_create(LIST_ITEMS_IN_BLOCK, destructor)
bool cl_destroy(struct t_common_list *list);
__inline t_cl_error cl_insert(struct t_common_list *list, void *data_ptr);
__inline void *cl_remove(struct t_common_list *list, t_cl_error *err);
__inline t_cl_error cl_push(struct t_common_list *list, void *data_ptr);
__inline void *cl_pick(const struct t_common_list *list, t_cl_error *err);
__inline void *cl_pick_top(const struct t_common_list *list, t_cl_error *err);
__inline void *cl_pop(struct t_common_list *list, t_cl_error *err);
__inline t_cl_error cl_chg_top(struct t_common_list *list, void *data_ptr);
__inline void *cl_iterate(struct t_common_list *list,
                          bool (*handler)(void *data, void *user),
                          void *user,
                          t_cl_error *err);
__inline void *cl_riterate(struct t_common_list *list,
                           bool (*handler)(void *data, void *user),
                           void *user,
                           t_cl_error *err);
__inline t_cl_error cl_testerr(struct t_common_list *list);
__inline bool cl_is_empty(const struct t_common_list *list);
__inline void *cl_cur_to_first(struct t_common_list *list, t_cl_error *err);
__inline void *cl_cur_to_last(struct t_common_list *list, t_cl_error *err);
__inline void *cl_pick_next(struct t_common_list *list, t_cl_error *err);
__inline void *cl_next(struct t_common_list *list, t_cl_error *err);
__inline bool cl_have_next(struct t_common_list *list);
__inline bool cl_have_prev(struct t_common_list *list);
__inline void *cl_new_item(struct t_common_list *cl, const size_t item_sz);
__inline void *cl_find(struct t_common_list  *cl,
                       f_cl_iterhandler      search_func,
                       const void            *data);
__inline void *cl_rfind(struct t_common_list  *cl,
                        f_cl_iterhandler      search_func,
                        const void            *data);
// if error, return(-1).
__inline int cl_items_count(struct t_common_list *cl);
//__inline t_cl_error cl_prev(const struct t_common_list *list, void **data_ptr);

//------------------------------------------------------------------------------
// Other functions.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//fprintf_conv(FILE *fs, const char format, ...);
//------------------------------------------------------------------------------
// Version of strncpy that ensures dest (size bytes) is null-terminated.
char* strncpy0(char* dest, const char* src, size_t size);
int strlen_utf8(const char *s);
__inline bool is_first_namesym(const char c);
__inline bool is_namesym(const char c);
__inline bool iscomma(const char c);
__inline bool iseol(const char c);
__inline bool is_control_fs(const char c);
__inline bool is_metafield_fs(const char c);
__inline bool isoctal(const char c);
__inline bool is_left_bracket(const char c);
__inline bool is_right_bracket(const char c);
__inline bool isbracket(const char c);
// Return pointer to first non-whitespace char in given string.
// Return NULL, if error or end of s.
__inline char* skip_spaces(char *s);
// Strip whitespace chars off end of given string, in place.
// Return s.
__inline char* rstrip(char* s);
// Read text and return number.
float          get_num(const char *str);
__inline void  fprint_bin(FILE *fs, const unsigned char *data,
                         const unsigned int data_sz, const int bc);
__inline void  fprint_rbin(FILE *fs, const unsigned char *data,
                         const unsigned int data_sz, const int bc);
__inline void  print_bin(const unsigned char *data, const unsigned int data_sz,
                         const int bc);
// Print symbol c for count times.
__inline void  fprint_sym(FILE *fs, const char c, const unsigned int count);
__inline void  print_sym(const char c, const unsigned int count);

void           fprint_hexdump(FILE *fs, const unsigned char *data,
                             const long data_sz, const int bc);
void           fprint_rhexdump(FILE *fs, const unsigned char *data,
                               const long data_sz, const int bc);
__inline void  print_hexdump(const unsigned char *data, const long data_sz,
                             const int bc);
//------------------------------------------------------------------------------
void print_err(const char *msg);
//------------------------------------------------------------------------------
// Print data by specification.
// Specs:
//    x - hex, p - hex dump, d - date, t - time, 
//    u - unsigned int, i - integer,
//    b - binary, c - character, n - invisible.
void fprint_by_spec(FILE *fs, uint8 *data_ptr, const unsigned data_sz,
                    const char spec,
                    const int dump_lim);
//------------------------------------------------------------------------------
char *sprintf_by_spec(uint8 *data_ptr, const unsigned data_sz,
                      const char spec);
//------------------------------------------------------------------------------
#if !(defined(min) || defined(HAVE_MIN))
   #define min(a, b) ((a <= b) ? a : b)
#endif
#if !(defined(max) || defined(HAVE_MAX))
   #define max(a, b) ((a >= b) ? a : b)
#endif
//------------------------------------------------------------------------------
#if !(defined(memzero) || defined(HAVE_MEMZERO))
   // bzero() is deprecated.
   #define memzero(dest, size) memset(dest, 0, size)
#endif
//------------------------------------------------------------------------------
#define streq(s1, s2) (strcoll(s1, s2) == 0)
#define streqn(s1, s2, sz) (strncmp(s1, s2, sz) == 0)
#if !(defined(HAVE_STRCASECMP) && defined(HAVE_STRNCASECMP))
   #error "strcasecmp() or strncasecmp() was not found!"
#endif
#define strceq(s1, s2) (strcasecmp(s1, s2) == 0)
#define strceqn(s1, s2, sz) (strncasecmp(s1, s2, sz) == 0)
//------------------------------------------------------------------------------
#if DEBUG_LEVEL >= 3
void *dmalloc(size_t size);
void dfree(void *ptr);
#else
   #define dmalloc malloc
   #define dfree   free
#endif
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
