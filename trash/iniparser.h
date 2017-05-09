//------------------------------------------------------------------------------
// Module for translating ini file sections to blocks of linked lists.
//------------------------------------------------------------------------------

#ifndef _iniparser_h
#define _iniparser_h
//------------------------------------------------------------------------------
#include "utils.h"
//------------------------------------------------------------------------------
// Structures.

struct t_key_value_pair
{
   char *key;
   char *value;
};
//------------------------------------------------------------------------------
struct t_key_value_list_item
{
   struct t_key_value_pair       pair;
   struct t_key_value_list_item  *next;
};
//------------------------------------------------------------------------------
struct t_key_value_list
{
   struct t_key_value_list_item  *first;
   struct t_key_value_list_item  *cur;
};
//------------------------------------------------------------------------------
struct t_key_value_block
{
   char                          *block_name;
   struct t_base                 *db;
   struct t_key_value_list       pairs;
//   struct t_common_list          pairs;
   struct t_key_value_block      *next;
};
//------------------------------------------------------------------------------
struct t_base
{
   int                           items_cnt;
//   struct t_common_list          blocks;
   struct t_key_value_block      *first;
   struct t_key_value_block      *cur;
};

//------------------------------------------------------------------------------
// Kay-value pair iterator callback.
typedef int (*f_pair_interpreter)(struct t_key_value_pair *kvp,
                                  void *user_data);
//------------------------------------------------------------------------------
// Functions.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Load base from INI file.
struct t_base *db_load_base(const char *ini_file);
int db_destroy(struct t_base *db);
//------------------------------------------------------------------------------
// Return value in the first pair in block with key == key_name. Or NULL,
// if not found.
__inline char *db_get_value(
   const struct t_key_value_block   *kv_block,
   const char                       *key_name
);
//------------------------------------------------------------------------------
// Return first pair in block with key == key_name. Or NULL, if not found.
struct t_key_value_pair *db_get_value_pair(
   const struct t_key_value_block   *kv_block,
   const char                       *key_name
);
//------------------------------------------------------------------------------
// Return first block, if block_name is NULL or block, which name is block_name.
__inline struct t_key_value_block *db_get_block_by_name(
   const struct t_base  *db,
   const char           *block_name
);
//------------------------------------------------------------------------------
// Return first finded block, where key == key_name and value == key_value.
// block_name, key_name or key_value may be NULL.
struct t_key_value_block *db_get_block_by_value(
   const struct t_base  *db,
   const char           *block_name,
   const char           *key_name,
   const char           *key_value
);
//------------------------------------------------------------------------------
// Return next block or NULL.
__inline struct t_key_value_block *db_next_block(const struct t_key_value_block *kvb);
//------------------------------------------------------------------------------
// Return next list item or NULL.
__inline struct t_key_value_list_item *db_next_li(
   const struct t_key_value_list_item *li);
//------------------------------------------------------------------------------
// Iterate records in block. Return 0, if ok or result < 0, if fail.
// If result of fpi < 0, then exit.
int db_iterate_block(const struct t_key_value_block *kvb,
                     f_pair_interpreter fpi, void* user_data);
#endif
//------------------------------------------------------------------------------
