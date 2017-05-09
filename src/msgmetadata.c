//------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
//------------------------------------------------------------------------------
#include "msgmetadata.h"
#include "utils.h"
#include "crc.h"
//------------------------------------------------------------------------------
#if DEBUG_LEVEL >= 2
static int nc = 0;
#endif
//------------------------------------------------------------------------------
static
struct t_message_tree_node *create_node(const t_node_type node_type)
// Return created node or NULL.
// Node and node data must be destroyed somewhere.
{
   struct t_message_tree_node *node;
   void                       *data_addr  = NULL;
   size_t                     data_sz     = sizeof(struct t_message_tree_node);

   if ((node = dmalloc(data_sz)) == NULL) return(NULL);
   memzero(node, data_sz);

   switch (node_type)
   {
      case nt_field:
         data_sz = sizeof(struct t_message_field_md);
      break;
      case nt_metafield:
         data_sz = sizeof(struct t_metafield);
      break;
      case nt_link:
         data_sz = sizeof(struct t_link_data);
      break;
      default:
         return(NULL);
      break;
   }

   node->type = node_type;

   if ((data_addr = dmalloc(data_sz)) == NULL)
   {
      dfree(node);
      return(NULL);
   }

   memzero(data_addr, data_sz);

   switch (node_type)
   {
      case nt_field:
         node->field = (struct t_message_field_md *)data_addr;
      break;
      case nt_metafield:
         node->metafield = (struct t_metafield *)data_addr;
      break;
      case nt_link:
         node->link = (struct t_link_data *)data_addr;
      break;
      default:
         assert("Unknown node type" && false);
   }

   #if DEBUG_LEVEL >= 3
   fprintf(stderr, "Message node number %d created...\n", ++nc);
   #endif

   return(node);
}
//------------------------------------------------------------------------------
static
bool free_node(struct t_message_tree_node *node)
// Destroy node, correctly created with create_node().
// Return true, if ok or false, if error.
{
   if (!node) return(false);

   switch (node->type)
   {
      case nt_field:
         dfree(node->field);
      break;
      case nt_metafield:
         dfree(node->metafield);
      break;
      case nt_link:
         dfree(node->link);
      break;
      default:
         return(false);
      break;
   }
   dfree(node);
   #if DEBUG_LEVEL >= 3
   fprintf(stderr, "Message node number %d destroyed...\n", nc--);
   #endif
   return(true);
}
//------------------------------------------------------------------------------
static
struct t_message_tree_node *add_node(struct t_message_field_tree  *msg_tree,
                                     struct t_message_tree_node   *node)
// Add node, as brother or child of a current node.
// Return added node or NULL.
{
   struct t_message_tree_node *prev_node;
   bool                       add_child = false;

   assert(msg_tree != NULL);

   if (node == NULL) return(false);

   // Test cur_node. Not root_node.
   if (!msg_tree->cur_node)
   {
      msg_tree->root = msg_tree->cur_node = node;
   }
   else
   {
      add_child = ((msg_tree->cur_node->type == nt_link) &&
                   (msg_tree->cur_node->link->link_type != lt_end));

      if (node->type == nt_link)
      // Node for addition is a link.
      {
         switch (node->link->link_type)
         {
            case lt_if:
            case lt_repeat:
               add_child = add_child && (msg_tree->cur_node->link->child == NULL);
            break;
            case lt_else:
            case lt_elif:
               if ((!add_child) ||
                   (msg_tree->cur_node->type == nt_link) &&
                   ((msg_tree->cur_node->link->link_type != lt_if) &&
                    (msg_tree->cur_node->link->link_type != lt_elif)))
               {
                  if (msg_tree->cur_node->parent && (msg_tree->cur_node->parent->type == nt_link))
                  {
                     if ((msg_tree->cur_node->parent->link->link_type != lt_if) &&
                         (msg_tree->cur_node->parent->link->link_type != lt_elif)) return(NULL);

                     msg_tree->cur_node = msg_tree->cur_node->parent;
                  }
                  else return(NULL);
               }

               add_child = false;
            break;
            case lt_end:
               if ((!add_child) && (msg_tree->cur_node->parent))
                 msg_tree->cur_node = msg_tree->cur_node->parent;
               if (!(msg_tree->cur_node && (msg_tree->cur_node->type == nt_link) &&
                     ((msg_tree->cur_node->link->link_type == lt_if) ||
                     (msg_tree->cur_node->link->link_type == lt_elif) ||
                     (msg_tree->cur_node->link->link_type == lt_repeat))))
                  return(NULL);

               add_child = false;
            break;
            default:
               assert("Unknown node type!" && false);
            break;
         }
      }
      else
      // Node for addition is a field.
      {
         add_child = add_child && (msg_tree->cur_node->link->child == NULL);
      }

      prev_node = msg_tree->cur_node;

      if (add_child)
      {
         msg_tree->cur_node->link->child  = node;
         msg_tree->cur_node               = node;
         node->parent                     = prev_node;
      }
      else
      {
         msg_tree->cur_node->next         = node;
         msg_tree->cur_node               = node;
         node->parent                     = prev_node->parent;
      }
   }

   // Node was successfully added.
   assert(msg_tree->cur_node != NULL);
   msg_tree->cur_node->next = NULL;

   return(msg_tree->cur_node);
}
//------------------------------------------------------------------------------
static void link_destructor(void *link)
{
   free_node((struct t_message_tree_node*)link);
}
//------------------------------------------------------------------------------
static bool free_fields_tree(struct t_message_metadata *mmd)
{
   struct t_message_tree_node *node = NULL;
   struct t_common_list       *links;

   assert(mmd != NULL);

   if ((links = cl_create_defd(link_destructor)) == NULL) return(false);

   assert(mmd->fields && mmd->fields->root);

   node = mmd->fields->root;

   // Destroy fields and accumulate links in the list.
   while (node)
   {
      mmd->fields->cur_node = node;
      if ((node->type == nt_link) && (node->link->child))
      {
         if (cl_push(links, node) != clec_noerr)
         {
            cl_destroy(links);
            return(false);
         };
         node = node->link->child;
      }
      else
      {
         node = (node->next) ? node->next : ((node->parent) ? node->parent->next : NULL);
         free_node(mmd->fields->cur_node);
      }

   }

   // Destroy links.
   cl_destroy(links);

   dfree(mmd->fields);
   mmd->fields = NULL;

   return(true);
}
//------------------------------------------------------------------------------
bool destroy_messages_db(struct t_base          *messages,
                         struct t_common_list   *md_cache)
{
   struct t_message_metadata *data;
   t_cl_error err;

   if (!messages) return(false);

   data = cl_pop(md_cache, &err);
   while (data && (err == clec_noerr))
   {
      if (err != clec_noerr) break;
      free_fields_tree(data);
      dfree(data);
      data = cl_pop(md_cache, &err);
   }
   cl_destroy(md_cache);

   return(db_destroy(messages));
}
//------------------------------------------------------------------------------
static bool msg_iterator(struct t_key_value_pair *kvp, void *user_data)
{
   struct t_message_metadata     *mmd           =
      (struct t_message_metadata*)user_data;
   struct t_message_field_tree   *msg_tree;
   struct t_message_tree_node    *node;
   char                          first_sym;
   char                          *comma_pos     = NULL;
   unsigned int                  descr_len      = 0;
   struct t_message_field_md     *mf_md;

   assert(mmd != NULL);
   assert(kvp != NULL);

   msg_tree    = mmd->fields;
   first_sym   = tolower(kvp->key[0]);

   if (is_metafield_fs(first_sym))
   // Metafield.
   {
      if (!add_node(msg_tree, node = create_node(nt_metafield)))
      {
         free_node(node);
         fprintf(stderr, _("Error: memory allocation for the message metafield (message type 0x%04X)!\n"),
                 mmd->message_type);
         return(false);
      }

      assert(node->metafield != NULL);

      node->metafield->type             = mft_ptr;
      node->metafield->size             = strlen_utf8(kvp->value);
      node->metafield->name             = kvp->key;
      node->metafield->value.data_ptr   = kvp->value;
   }
   else if (is_control_fs(first_sym))
   // Control keyword.
   {
      if ((node = create_node(nt_link)) == NULL)
      {
         fprintf(stderr, _("Error: memory allocation for the message link (message type 0x%04X)!\n"),
                 mmd->message_type);
         return(false);
      }

      assert(node->link != NULL);

      if (strceq(kvp->key, "@if"))
      {
         node->link->link_type = lt_if;
      }
      else if (strceq(kvp->key, "@elif"))
      {
         node->link->link_type = lt_elif;
      }
      else if (strceq(kvp->key, "@else"))
      {
//            fprintf(stderr, "Messages metadata parsing error: @else without preceding @if or @elif!\n");
         node->link->link_type = lt_else;
      }
      else if (strceq(kvp->key, "@end"))
      {
         node->link->link_type = lt_end;
      }
      else if (strceq(kvp->key, "@repeat"))
      {
         node->link->link_type = lt_repeat;
      }
      else
      {
         fprintf(stderr, _("Error: unknown control keyword \"%s\" in message type 0x%04X!\n"),
                 kvp->key, mmd->message_type);
         free_node(node);
         return(false);
      }

      node->link->expr = kvp->value;
      if (!add_node(msg_tree, node))
      {
         fprintf(stderr, _("Error: can't add node \"%s\" [value = \"%s\", message type 0x%04X]!\n"),
                 kvp->key, kvp->value, mmd->message_type);
         free_node(node);
         return(false);
      }
   }
   else if (is_first_namesym(first_sym))
   // Message field.
   {
      if (!add_node(msg_tree, node = create_node(nt_field)))
      {
         free_node(node);
         fprintf(stderr, _("Error: memory allocation for the message field (message type 0x%04X)!\n"),
                 mmd->message_type);
         return(false);
      }

      assert(node->field != NULL);

      mf_md       = node->field;
      mf_md->name = kvp->key;

      if ((comma_pos = (char*)strchr(kvp->value, ',')) == NULL)
      {
         fprintf(stderr, _("Error: parsing size of field \"%s\" (message type 0x%04X)!\n"),
                 kvp->key, mmd->message_type);
         return(false);
      }
      comma_pos = skip_spaces(comma_pos + 1);

      if (*(kvp->value) == '*')
      {
         mf_md->is_repeat  = true;
         mf_md->size       = get_num(kvp->value + 1);
      }
      else
      {
         mf_md->is_repeat  = false;
         mf_md->size       = get_num(kvp->value);
      }

      if (mf_md->size >= 1)
         mmd->min_data_sz += mf_md->size;

      /* Old piece of code for the old messages.ini format (with C-types).
      if ((comma_pos = (char*)strchr(comma_pos, ',')) == NULL)
      {
         fprintf(stderr, "Parsing C-type of field %s error!\n", kvp->key);
         return(-3);
      }
 
      comma_pos += 1;
       */
      mf_md->out_spec   = tolower(*comma_pos);
      comma_pos = skip_spaces(comma_pos + 1);
      if (!(comma_pos && (*comma_pos == ',')))
      {
         fprintf(stderr, _("Error: parsing output specification of field \"%s\" (message type 0x%04X)!\n"),
                 kvp->key, mmd->message_type);
         return(false);
      }
      mf_md->descr      = comma_pos + 1;
      descr_len         = strlen_utf8(comma_pos + 1);

      if (descr_len > mmd->msg_out_max_dl) mmd->msg_out_max_dl = descr_len;
     
   }
   else
   {
      fprintf(stderr, _("Error: incorrect identifier name \"%s\" in message type 0x%04X\n"),
              kvp->key, mmd->message_type);
      return(false);
   }
 
   return(true);
}
//------------------------------------------------------------------------------
static bool msgdb_iterator(struct t_key_value_block *kvb, void *msg_db)
{
   struct t_message_metadata *mmd =
         (struct t_message_metadata*)dmalloc(sizeof(struct t_message_metadata));

   assert(kvb != NULL);

   if (mmd == NULL) return(false);

   memzero(mmd, sizeof(struct t_message_metadata));

   if (((mmd->fields = dmalloc(sizeof(struct t_message_field_tree))) == NULL) ||
       (cl_push(msg_db, mmd) != clec_noerr))
   {
      dfree(mmd);
      return(false);
   }

   memzero(mmd->fields, sizeof(struct t_message_field_tree));
   mmd->message_type = get_num(kvb->block_name);
   if (!db_iterate_block(kvb, msg_iterator, mmd))
   {
//      mmd->message_type = 0;
      cl_pop(msg_db, NULL);
      free_fields_tree(mmd);
      dfree(mmd);
      return(false);
   }

   return(true);
}
//------------------------------------------------------------------------------
static bool mfs_iterator(struct t_key_value_pair *kvp, void *mfs_db)
{
   struct t_header_field   *mf_md = dmalloc(sizeof(struct t_header_field));
   unsigned int            descr_len;
   char                    *spos;

   assert(kvp     != NULL);
   assert(mfs_db  != NULL);

   if (!mf_md) return(false);

   spos = kvp->value;
   assert(spos != NULL);

   memzero(mf_md, sizeof(struct t_header_field));
   mf_md->out_spec = tolower(*spos);
   spos = skip_spaces(spos + 1);

   if (!iscomma(*spos))
   {
      fprintf(stderr, _("Error: parsing output specification of header field \"%s\"!\n"), kvp->key);
      return(false);
   }


   if (((mf_md->name = strdup(kvp->key)) == NULL) ||
       ((mf_md->descr = strdup(spos + 1)) == NULL))
   {
      dfree(mf_md->name);
      dfree(mf_md->descr);
   }
   if (cl_push(((struct t_header_metadata*)mfs_db)->fields_md, mf_md) != clec_noerr)
   {
      dfree(mf_md);
      return(false);
   }

   if (mf_md->out_spec !=  'n')
   // Only visible fields, used to calculate max descr length for visual alignment.
   {
      descr_len = strlen_utf8(mf_md->descr);
      if (((struct t_header_metadata*)mfs_db)->max_desc_len < descr_len)
         ((struct t_header_metadata*)mfs_db)->max_desc_len = descr_len;
   }
      
   return(true);
}
//------------------------------------------------------------------------------
static bool mfsdb_iterator(struct t_key_value_block *kvb, void *mfs_db)
{
   t_cl_error err;
   struct t_header_field *mf_md;

   assert(mfs_db != NULL);

   if (strceq(kvb->block_name, "metafields"))
   {
      if (!db_iterate_block(kvb, mfs_iterator, mfs_db))
      {
         mf_md = cl_pop(((struct t_header_metadata*)mfs_db)->fields_md, &err);
         while (mf_md && (err == clec_noerr))
         {
            dfree(mf_md->name);
            dfree(mf_md->descr);
            dfree(mf_md);
            mf_md = cl_pop(((struct t_header_metadata*)mfs_db)->fields_md, &err);
         }
      }
      return(false);
   }
   return(true);
}
//------------------------------------------------------------------------------
void header_field_destructor(void *hf)
{
   dfree(((struct t_header_field *)hf)->name);
   dfree(((struct t_header_field *)hf)->descr);
   dfree(hf);
}
//------------------------------------------------------------------------------
struct t_header_metadata *init_header_fields_db(const char *filename)
{
   struct t_header_metadata   *result;
   struct t_base              *mf_db = db_load_base(filename);

   if (!mf_db) return(NULL);

   result = (struct t_header_metadata*)dmalloc(sizeof(struct t_header_metadata));

   if (result == NULL) return(NULL);
   result->max_desc_len = 0;

   if ((result->fields_md =
        cl_create(mf_db->items_cnt, header_field_destructor)) == NULL)
   {
      dfree(result);
      db_destroy(mf_db);
      return(NULL);
   }

   db_iterate_base(mf_db, mfsdb_iterator, result);

   if (cl_is_empty(result->fields_md))
   {
      cl_destroy(result->fields_md);
      dfree(result);
      db_destroy(mf_db);
      return(NULL);
   }
   db_destroy(mf_db);
   return(result);
}
//------------------------------------------------------------------------------
bool destroy_header_fields_db(struct t_header_metadata *hmd)
{
   bool result;

   if (!hmd) return(false);

   result = cl_destroy(hmd->fields_md);
   dfree(hmd);
   return(result);
}
//------------------------------------------------------------------------------
struct t_base *init_messages_db(const char            *filename,
                                struct t_common_list  **md_cache)
{
   struct t_base *messages = db_load_base(filename);

   if (!(messages && md_cache)) return(NULL);

   if ((*md_cache = cl_create(messages->items_cnt, NULL)) == NULL)
   {
      db_destroy(messages);
      return(NULL);
   }

   if (!db_iterate_base(messages, msgdb_iterator, *md_cache))
   {
      cl_destroy(*md_cache);
      *md_cache = NULL;
      db_destroy(messages);
      return(NULL);
   }
   return(messages);
}
//------------------------------------------------------------------------------
struct t_message_metadata *get_message_md_by_type(const t_message_type type,
                                                  struct t_common_list *md_cache)
{
   struct t_message_metadata  *mmd = NULL;
   t_cl_error                 err;

   if ((mmd = cl_cur_to_first(md_cache, &err)) == NULL) return(NULL);
   while (mmd && (err == clec_noerr))
   {
      if (mmd->message_type == type) return(mmd);
      mmd = cl_next(md_cache, &err);
   }
   return(NULL);
}
//------------------------------------------------------------------------------
__inline struct t_message_metadata *get_message_metadata(struct t_message_raw *msg,
                                                         struct t_common_list *md_cache)
{
   assert(msg != NULL);
   return(get_message_md_by_type(msg->header.type, md_cache));
}
//------------------------------------------------------------------------------
bool print_msg_tree(const t_message_type msg_type,
                   struct t_common_list *md_cache)
{
   struct t_message_metadata   *mmd    = get_message_md_by_type(msg_type, md_cache);
   int                        level    = 0;
   struct t_message_tree_node *node    = NULL;

   if (!mmd) return(false);
   node = mmd->fields->root;

   printf("ROOT\n");
   while (node)
   {
      switch (node->type)
      {
         case nt_link:
            printf("Link level %d, type %d, with expr: %s [addr: %p, par: %p]\n",
                   level, node->link->link_type, node->link->expr, node, node->parent);
         break;
         case nt_field:
            printf("Field level %d, name: \"%s\"\n", level, node->field->name);
         break;
         default:
            printf("Unknown node type %d (level: %d)\n", node->type, level);
      }

      if ((node->type == nt_link) && (node->link->link_type != lt_end) && (node->link->child))
      {
         level++;
         node = node->link->child;
      }
      else
      {
         if (node->next) node = node->next;
         else if (node->parent)
         {
//            if (level-- <= 0) node = NULL;
            node = node->parent->next;
            level--;
         }
         else node = NULL;
//         node = (node->next) ? node->next : ((node->parent) ? level--, node->parent->next : NULL);
      }
   }
   
   
   return(true);
}
//------------------------------------------------------------------------------

