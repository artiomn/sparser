//
// INI file to memory database.
//

//------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "iniparser.h"
#include "ini.h"
#include "utils.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Base operation functions.
//------------------------------------------------------------------------------

static void block_destructor(void *kvb)
{
   assert(kvb != NULL);

   cl_destroy(((struct t_key_value_block*)kvb)->pairs);
   dfree(((struct t_key_value_block*)kvb)->block_name);
   dfree(kvb);
}
//------------------------------------------------------------------------------
static void pair_destructor(void *kvp)
{
   assert(kvp != NULL);

   dfree(((struct t_key_value_pair*)kvp)->key);
   dfree(((struct t_key_value_pair*)kvp)->value);
   dfree(kvp);
}
//------------------------------------------------------------------------------
static struct t_key_value_pair *db_add_pair(
   struct t_key_value_block   *kvb,
   const char                 *key_name,
   const char                 *value
)
// Allocate memory in kpv->key and kvp->value. Memory must be freed
// somewhere.
{
   struct t_key_value_pair* kvp;

   assert(kvb        != NULL);
   assert(key_name   != NULL);

   kvp = (struct t_key_value_pair*)dmalloc(sizeof(struct t_key_value_pair));
   if (!kvp) return(NULL);

   if (cl_push(kvb->pairs, kvp) != clec_noerr)
   {
      dfree(kvp);
      return(NULL);
   }

   kvp->key    = (char*)strdup(key_name);
   kvp->value  = (char*)strdup(value);

   return(kvp);
}
//------------------------------------------------------------------------------
static
struct t_key_value_block *db_add_kv_block(struct t_base  *db,
                                          const char     *block_name)
{
   struct t_key_value_block* kvb;

   assert(db != NULL);

   if (!block_name) return(NULL);

   kvb = (struct t_key_value_block*)dmalloc(sizeof(struct t_key_value_block));

   if (!kvb) return(NULL);
   if ((kvb->pairs = cl_create_defd(pair_destructor)) == NULL)
   {
      dfree(kvb);
      return(NULL);
   }

   if (cl_push(db->blocks, kvb) != clec_noerr)
   {
      dfree(kvb->pairs);
      dfree(kvb);
      return(NULL);
   }

   kvb->block_name   = (char*)strdup(block_name);
   kvb->db           = db;
   db->items_cnt++;

   return(kvb);
}
//------------------------------------------------------------------------------
static struct t_base *db_create()
{
   struct t_base *result = (struct t_base*)dmalloc(sizeof(struct t_base));

   if (result)
   {
      if ((result->blocks = cl_create_defd(block_destructor)) == NULL)
      {
         dfree(result);
         return(NULL);
      }
      result->items_cnt = 0;
   }
   return(result);
}
//------------------------------------------------------------------------------
bool db_destroy(struct t_base *db)
{
   if (!(db && db->blocks)) return(false);

   cl_destroy(db->blocks);
   dfree(db);

   return(true);
}
//------------------------------------------------------------------------------
// Struct for handler and db_load.
struct t_db_ab
{
   struct t_base            *db;
   struct t_key_value_block *cur_kvb;
};
//------------------------------------------------------------------------------
static
int ini_handler(void *user, const char *section, const char *name,
                const char *value)
// Handler for ini_parse.
{
   struct t_db_ab *dbab = (struct t_db_ab*)user;

   if ((name == NULL) && (value == NULL))
   // Section. ini.c was changed by me.
   {
      if ((dbab->cur_kvb = db_add_kv_block(dbab->db, section)) == NULL) return(0);
   }
   else 
   // key=value pair.
   {
      if (db_add_pair(dbab->cur_kvb, name, value) == NULL) return(0);
   }

   return(1);
}
//------------------------------------------------------------------------------
struct t_base *db_load_base(const char *ini_file)
// Load base from INI file.
{
   struct t_db_ab dbab = { NULL, NULL };
   int            err_line;

   if (!(dbab.db = db_create())) return(NULL);

   if ((err_line = ini_parse(ini_file, ini_handler, &dbab)) != 0)
   {
      fprintf(stderr, _("Can't load ini file: "));
      if (err_line < 0) fprintf(stderr, "%s\n", strerror(errno));
      else fprintf(stderr, _("[error in line: %d]!\n"), err_line);
      db_destroy(dbab.db);
      return(NULL);
   }
   return(dbab.db);
}
//------------------------------------------------------------------------------
__inline char *db_get_value(const struct t_key_value_block  *kv_block,
                            const char                      *key_name)
{
   struct t_key_value_pair *kvp = db_get_value_pair(kv_block, key_name);

   if (!kvp) return (NULL);
   return(kvp->value);
}
//------------------------------------------------------------------------------
static bool gvp_handler(void *kvp, void *key_name)
{
   if (strceq(((struct t_key_value_pair*)kvp)->key, (char*)key_name))
      return(false);
   return(true);
} 
//------------------------------------------------------------------------------
__inline struct t_key_value_pair *db_get_value_pair(
   const struct t_key_value_block   *kv_block,
   const char                       *key_name
)
// Return first pair in block with key == key_name. Or NULL, if not found.
{
   return(cl_find(kv_block->pairs, &gvp_handler, key_name));
}
//------------------------------------------------------------------------------
static bool gbn_handler(void *kvb, void *block_name)
{
   if (block_name == NULL) return(false);
   if (strceq(((struct t_key_value_block*)kvb)->block_name, (char*)block_name))
      return(false);
   return(true);
} 
//------------------------------------------------------------------------------
__inline struct t_key_value_block *db_get_block_by_name(
   const struct t_base  *db,
   const char           *block_name
)
{
   assert(db != NULL);
   return(cl_find(db->blocks, &gbn_handler, block_name));
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
   struct t_key_value_block      *kvb;
   struct t_key_value_pair       *kvp;
   t_cl_error                    err;

   assert(db != NULL);

   kvb = cl_cur_to_first(db->blocks, &err);
   while (kvb && (err == clec_noerr))
   {
      if ((block_name == NULL) ||
         (strceq(kvb->block_name, block_name)))
      {
         kvp = cl_cur_to_first(kvb->pairs, &err);

         while (kvp && (err == clec_noerr))
         {
            if (
                  (key_name == NULL || strceq(kvp->key, key_name)) &&
                  (key_value == NULL || strceq(kvp->value, key_value))
               ) return(kvb);
            kvp = cl_next(kvb->pairs, &err);
         }
      }

      if ((err == clec_noerr) || (err == clec_last_item))
         kvb = cl_next(db->blocks, &err);
   }
   return(NULL);
}
//------------------------------------------------------------------------------
__inline bool db_iterate_block(const struct t_key_value_block *kvb,
                              f_pair_interpreter fpi, void *user_data)
{
   t_cl_error err;

   assert(kvb != NULL);
   assert(fpi != NULL);

   if (kvb->pairs == NULL) return(false);
   cl_iterate(kvb->pairs, (f_cl_iterhandler)fpi, user_data, &err);
   return(err == clec_noerr);
}
//------------------------------------------------------------------------------
__inline bool db_iterate_base(const struct t_base *db,
                             f_block_interpreter fbi, void *user_data)
{
   t_cl_error err;

   assert(db   != NULL);
   assert(fbi  != NULL);

   if (db->blocks == NULL) return(false);
   cl_iterate(db->blocks, (f_cl_iterhandler)fbi, user_data, &err);
   return(err == clec_noerr);
}
//------------------------------------------------------------------------------
/*int db_print(const struct t_base *db)
// Print database to the console.
{
   struct t_key_value_block   *kvb = db->blocks;
   struct t_key_value_pair    *kvp = NULL;
   
   if (!kvb) return(-1);

   do
   {
      printf("[%s]\n", kvb->data_ptr->block_name);
      if ((kvli = kvb->pairs->first) == NULL) continue;
      do
      {
         printf("%s = %s\n", kvli->pair.key, kvli->pair.value);
      }
      while ((kvli = kvli->next) != NULL);
      printf("\n");
   }
   while ((kvb = kvb->next) != NULL);

   return(0);
}*/
//------------------------------------------------------------------------------

