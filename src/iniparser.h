//------------------------------------------------------------------------------
// Module for translating ini file sections to blocks of linked lists.
//------------------------------------------------------------------------------

#ifndef _iniparser_h
#define _iniparser_h
//------------------------------------------------------------------------------
#include "common.h"
#include "utils.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
// Structures.

struct t_key_value_pair
{
   char *key;
   char *value;
};
//------------------------------------------------------------------------------
struct t_key_value_block
{
   char                          *block_name;
   struct t_base                 *db;
   struct t_common_list          *pairs;
};
//------------------------------------------------------------------------------
struct t_base
{
   int                           items_cnt;
   struct t_common_list          *blocks;
};
//------------------------------------------------------------------------------
// Key-value pairs block callback.
typedef bool (*f_block_interpreter)(struct t_key_value_block *kvb,
                                  void *user_data);
//------------------------------------------------------------------------------
// Kay-value pair iterator callback.
typedef bool (*f_pair_interpreter)(struct t_key_value_pair *kvp,
                                  void *user_data);
//------------------------------------------------------------------------------
// Functions.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Load base from INI file.
struct t_base *db_load_base(const char *ini_file);
bool db_destroy(struct t_base *db);
//------------------------------------------------------------------------------
// Return value in the first pair in block with key == key_name. Or NULL,
// if not found.
__inline char *db_get_value(
   const struct t_key_value_block   *kv_block,
   const char                       *key_name
);
//------------------------------------------------------------------------------
// Return first pair in block with key == key_name. Or NULL, if not found.
__inline struct t_key_value_pair *db_get_value_pair(
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
__inline struct t_key_value_block *db_get_block_by_value(
   const struct t_base  *db,
   const char           *block_name,
   const char           *key_name,
   const char           *key_value
);
//------------------------------------------------------------------------------
__inline bool db_iterate_base(const struct t_base *db,
                             f_block_interpreter fbi, void *user_data);
//------------------------------------------------------------------------------
// Iterate records in block. Return 0, if ok or result < 0, if fail.
// If result of fpi < 0, then exit.
__inline bool db_iterate_block(const struct t_key_value_block *kvb,
                     f_pair_interpreter fpi, void* user_data);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
