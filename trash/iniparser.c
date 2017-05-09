//
// INI file to memory database.
//

//------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "ini.h"
#include "iniparser.h"
#include "utils.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Base operation functions.
//------------------------------------------------------------------------------

struct t_key_value_list_item *db_add_pair(
   struct t_key_value_block   *kvb,
   const char                 *key_name,
   const char                 *value
)
{
   if (!kvb->pairs.cur)
   {
      kvb->pairs.cur          =
         (struct t_key_value_list_item*)malloc(sizeof(struct t_key_value_list_item));
      kvb->pairs.first        = kvb->pairs.cur;
   }
   else
   {
      kvb->pairs.cur->next    = 
         (struct t_key_value_list_item*)malloc(sizeof(struct t_key_value_list_item));
      kvb->pairs.cur          = kvb->pairs.cur->next;
   }

   if (kvb->pairs.cur)
   {
      kvb->pairs.cur->pair.key      = (char*)strdup(key_name);
      kvb->pairs.cur->pair.value    = (char*)strdup(value);
      kvb->pairs.cur->next          = NULL;
   }
   else fprintf(stderr, "Can't allocate memory for database record [%s]!\n", strerror(errno));

   return(kvb->pairs.cur);
}
//------------------------------------------------------------------------------
struct t_key_value_block *db_add_kv_block
(
   struct t_base  *db,
   const char     *block_name
)
{
   if (!db->first)
   {
      db->cur        =
         (struct t_key_value_block*)malloc(sizeof(struct t_key_value_block));
      db->first      = db->cur;
   }
   else
   {
      db->cur->next  =
         (struct t_key_value_block*)malloc(sizeof(struct t_key_value_block));
      db->cur        = db->cur->next;
   }

   if (db->cur)
   {
      db->cur->block_name  = (char*)strdup(block_name);
      db->cur->db          = db;
      db->cur->next        = NULL; 
      db->cur->pairs.first =
      db->cur->pairs.cur   = NULL;
      db->items_cnt++;
   }
   else fprintf(stderr, "Can't allocate memory for database block [%s]!\n", strerror(errno));

   return(db->cur);
}
//------------------------------------------------------------------------------
struct t_base *db_create()
{
   struct t_base *result = (struct t_base*)malloc(sizeof(struct t_base));
   if (result) bzero(result, sizeof(struct t_base));
   return(result);
}
//------------------------------------------------------------------------------
int db_destroy(struct t_base *db)
{
   db->cur = db->first;
   
   while (db->cur)
   {
      free(db->first->block_name);
      db->first->pairs.cur = db->first->pairs.first;

      while (db->first->pairs.cur)
      {
         free(db->first->pairs.first->pair.key);
         free(db->first->pairs.first->pair.value);
         db->first->pairs.cur = db->first->pairs.first->next;
         free(db->first->pairs.first);
         db->first->pairs.first = db->first->pairs.cur;
      }

      db->cur = db->first->next;
      free(db->first);
      db->first = db->cur;
   }
   free(db);
   return(0);
}
//------------------------------------------------------------------------------
// Struct for handler and db_load.
struct t_db_ab
{
   struct t_base            *db;
   struct t_key_value_block *cur_kvb;
};
//------------------------------------------------------------------------------
int ini_handler(
   void*       user,
   const char* section,
   const char* name,
   const char* value
)
// Handler for ini_parse.
{
   struct t_db_ab* dbab = (struct t_db_ab*)user;

   if ((name == NULL) && (value == NULL))
   // Section. ini.c was changed by me.
   {
      if ((dbab->cur_kvb = db_add_kv_block(dbab->db, section)) == NULL) return(0);
   }
   else
   // key=value pair.
   {
      db_add_pair(dbab->cur_kvb, name, value);
   }

   return(1);
}
//------------------------------------------------------------------------------
struct t_base *db_load_base(const char *ini_file)
// Load base from INI file.
{
   struct t_db_ab dbab = { NULL, NULL };

   if (!(dbab.db = db_create())) return(NULL);

   if (ini_parse(ini_file, ini_handler, &dbab) != 0)
   {
      fprintf(stderr, "Can't load ini file [%s]!\n", strerror(errno));
      db_destroy(dbab.db);
      return NULL;
   }
   return(dbab.db);
}
//------------------------------------------------------------------------------
__inline char *db_get_value(
   const struct t_key_value_block   *kv_block,
   const char                       *key_name
)
{
   struct t_key_value_pair *kvp = db_get_value_pair(kv_block, key_name);

   if (!kvp) return (NULL);
   return(kvp->value);
}
//------------------------------------------------------------------------------
struct t_key_value_pair *db_get_value_pair(
   const struct t_key_value_block   *kv_block,
   const char                       *key_name
)
// Return first pair in block with key == key_name. Or NULL, if not found.
{
   struct t_key_value_list_item *kvli = kv_block->pairs.first;
   while (kvli)
   {
      if (strcasecmp(kvli->pair.key, key_name) == 0) return(&kvli->pair);
      kvli = kvli->next;
   }
   return(NULL);
}
//------------------------------------------------------------------------------
__inline struct t_key_value_block *db_get_block_by_name(
   const struct t_base  *db,
   const char           *block_name
)
{
   register struct t_key_value_block *kvb  = db->first;

   if (block_name == NULL) return(kvb);

   while (kvb)
   {
      if (strceq(kvb->block_name, block_name)) return(kvb);
      kvb = kvb->next;
   }

   return(NULL);
}
//------------------------------------------------------------------------------
struct t_key_value_block *db_get_block_by_value(
   const struct t_base  *db,
   const char           *block_name,
   const char           *key_name,
   const char           *key_value
)
// Return first finded block, where key == key_name and value == key_value.
// block_name, key_name or key_value may be NULL.
{
   struct t_key_value_block      *kvb  = db->first;
   struct t_key_value_list_item  *kvli = NULL; 

   while (kvb)
   {
      if ((block_name == NULL) ||
         (strceq(kvb->block_name, block_name)))
      {
         kvli = kvb->pairs.first; 

         while (kvli)
         {
            if (
                  (key_name == NULL || strceq(kvli->pair.key, key_name)) &&
                  (key_value == NULL || strceq(kvli->pair.value, key_value))
               ) return(kvb);
            kvli = kvli->next;
         }
      }

      kvb = kvb->next;
   }

   return(NULL);
  
}
//------------------------------------------------------------------------------
__inline struct t_key_value_block *db_next_block(const struct t_key_value_block *kvb)
{
   return((kvb) ? kvb->next : NULL);
}
//------------------------------------------------------------------------------
__inline struct t_key_value_list_item *db_next_li(
   const struct t_key_value_list_item *li)
{
   return((li) ? li->next : NULL);
}
//------------------------------------------------------------------------------
int db_iterate_block(const struct t_key_value_block *kvb,
                     f_pair_interpreter fpi, void *user_data)
{
   struct t_key_value_list_item  *kvli;
   int                           fpi_res = 0;
   
   if ((kvli = kvb->pairs.first) == NULL) return(-1);
   do
   {
      if (fpi) fpi_res = fpi(&kvli->pair, user_data);
   }
   while (((kvli = kvli->next) != NULL) && (fpi_res >=0));
   return(fpi_res);
}
//------------------------------------------------------------------------------
int db_print(const struct t_base *db)
// Print database to the console.
{
   struct t_key_value_block *kvb = db->first;
   struct t_key_value_list_item *kvli = NULL;
   
   if (!kvb) return(-1);

   do
   {
      printf("[%s]\n", kvb->block_name);
      if ((kvli = kvb->pairs.first) == NULL) continue;
      do
      {
         printf("%s = %s\n", kvli->pair.key, kvli->pair.value);
      }
      while ((kvli = kvli->next) != NULL);
      printf("\n");
   }
   while ((kvb = kvb->next) != NULL);

   return(0);
}
//------------------------------------------------------------------------------

