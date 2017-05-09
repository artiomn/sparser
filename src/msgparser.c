//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "msgparser.h"
#include "msgloader.h"
#include "msgviewer.h"
#include "actions.h"
#include "lexan.h"
#include "utils.h"
#include "crc.h"
//------------------------------------------------------------------------------
static
bool mdi_compound(struct t_full_msgdata *fmd);
//------------------------------------------------------------------------------
static void response_destructor(void *resp)
{
   assert(resp != NULL);

   dfree(((struct t_response*)resp)->descr);
   dfree(resp);
}
//------------------------------------------------------------------------------
static void field_destructor(void *field)
{
   assert(field != NULL);

   cl_destroy(((struct t_message_fli *)field)->responses);
   dfree(field);
}
//------------------------------------------------------------------------------
static
struct t_common_list *clear_fields_list(struct t_common_list *mfl)
// Remove field items without removing list structure.
{
   struct t_message_fli *data;
   struct t_response    *resp;
   t_cl_error           err;

   while ((data = cl_pop(mfl, &err)) != NULL)
   {
      cl_destroy(data->responses);
      dfree(data);
      if (err != clec_noerr) break;
   }

   return(mfl);
}
//------------------------------------------------------------------------------
static __inline bool msg_free_list(struct t_common_list *mfl)
// Free message list. Data will be destroyed. 
// Return true, if ok or false, if error.
{
   cl_destroy(mfl);
   return(true);
}
//------------------------------------------------------------------------------
static
__inline struct t_message_fli *add_message_field(struct t_common_list *mfl)
{
   assert(mfl != NULL);

   return(cl_new_item(mfl, sizeof(struct t_message_fli)));
}
//------------------------------------------------------------------------------
static
__inline struct t_response *add_message_resp(struct t_common_list *mrs)
{
   assert(mrs != NULL);

   return(cl_new_item(mrs, sizeof(struct t_response)));
}
//------------------------------------------------------------------------------
static
__inline struct t_response *add_field_resp(struct t_full_msgdata  *fmd,
                                           char                   *resp_str)
{
   char                       *st_pos;
   char                       *spos;
   long                       el;
   struct t_response          *resp;
   struct t_message_fli       *fli;
   struct t_la_control_block  la_cb;
   bool                       add_flag = true;

   assert(fmd        != NULL);
   assert(fmd->msg   != NULL);
   assert(resp_str   != NULL);

   spos = resp_str;

   // Get field.
   if ((spos = strchr(spos, ',')) == NULL)
   {
      fprintf(stderr, _("Error: no field name in response \"%s\"!\n"), resp_str);
      return(NULL);
   }

   st_pos = strndup(resp_str, (size_t)(spos - resp_str));
   rstrip(st_pos);

   if (is_metafield_fs(*st_pos) && strceq(st_pos, "$remainder"))
   {
   }  
   else if ((fli = get_field_by_name(st_pos, fmd->msg)) == NULL)
   {
      fprintf(stderr, _("Error: can't find field with name \"%s\"!\n"), st_pos);
      dfree(st_pos);
      return(NULL);
   }

   dfree(st_pos);
   st_pos = skip_spaces(spos + 1);
   if ((spos = strchr(st_pos, ',')) == NULL)
   {
      fprintf(stderr, _("Error: can't find condition expr in response \"%s\"!\n"), resp_str);
      return(NULL);
   }

   if ((el = (size_t)(spos - st_pos)) > 0) st_pos = strndup(st_pos, el);
   else st_pos = NULL;

   // Test response for the expression.
   if (fmd->msg->mmd && st_pos)
   {
      rstrip(st_pos);
      add_flag = (bool)lxn_analyse(lxn_init_param(st_pos, fmd, &la_cb));
      if (la_cb.e_code != lxec_noerror)
      {
         lxn_print_if_err(&la_cb);
         add_flag = false;
         dfree(st_pos);
         return(NULL);
      }
   }

   dfree(st_pos);

   st_pos = skip_spaces(spos + 1);
   if ((spos = strchr(st_pos, ',')) == NULL)
   {
      fprintf(stderr, _("Error: can't find error level in the response \"%s\"!\n"), resp_str);
      return(NULL);
   }

   el = strtol(st_pos, &st_pos, 0);

   if (!add_flag) return(NULL);

   // Add to list.
   if (!fli->responses) fli->responses = cl_create_defd(response_destructor);
   if (!fli->responses) return(NULL);

   spos = ++st_pos;
   if ((st_pos = strdup(spos)) == NULL) return(NULL);

   resp = (struct t_response*)cl_new_item(fli->responses, sizeof(struct t_response));

   if (resp == NULL)
   {
      dfree(st_pos);
      return(NULL);
   }

   resp->descr       = st_pos;
   resp->error_level = el;

   return(resp);
}
//------------------------------------------------------------------------------
static
struct t_message_tree_node *get_next_node(struct t_message_tree_node *node,
                                          int                        *level)
{
   if (!node) return(NULL);
   assert(level   != NULL);

   if (node->next) return(node->next);
   else if (node->parent)
   {
      (*level)--;
      return(node->parent->next);
   }
   return(NULL);
}
//------------------------------------------------------------------------------
static bool get_parsed_message(struct t_full_msgdata *fmd)
// Message parser. Create and return structure with message fields list.
// Get message data and message metadata in msg.
// Create message fields list in msg->fields. If msg->field is not NULL,
// it's using without creating new list.
// Fields list must be destroyed somewhere.
// If error, return false, if ok, return true.
// If message hasn't fields, msg->fields->first == NULL.
{
// TODO: decomposite.
   register struct t_message_tree_node *node       = NULL;
   struct t_message                    *msg        = NULL;
   struct t_common_list                *fields;
   struct t_message_fli                *msg_field  = NULL;
   struct t_response                   *resp       = NULL;

   struct t_common_list                *lnk_stack        = NULL;
   struct t_la_control_block           la_cb;

   typedef enum { ds_undef, ds_true, ds_false } t_data_state;
   struct t_link_md
   {
      struct t_message_tree_node *node;
      int                        level;
      // Expression fields.
      union
      {
         t_data_state   ds;
         long           rep_counter;
      };
   } *lmd;

   int               level          = 0;
   uint16            data_sz; 
   uint8             *data_end;
   long int          expr_fields;

   t_cl_error        ce;

   uint16            len;
   uint16            data_remainder = 0;

   bool              err_flag       = false;
   bool              nxt_flag       = true;


   assert(fmd        != NULL);
   assert(fmd->msg   != NULL);

   msg = fmd->msg;

   if (!msg->mmd)          return(false);
   if (!msg->raw_message)  return(false);

   fields         = msg->fields;
   data_sz        = message_data_len(msg->raw_message);
   msg->pointer   = (uint8*)&(msg->raw_message->data);
   data_end       = msg->pointer + data_sz;

   if (fields) clear_fields_list(fields);
   else if ((fields = cl_create_defd(field_destructor)) == NULL) return(false);

//   if (msg->responses) clear_fields_list(fields);
//   else if ((msg->responses = cl_create_defd(field_destructor)) == NULL) return(false);

   msg->fields = fields;
   node        = msg->mmd->fields->root;

   while (node && !err_flag)
   {

      switch (node->type)
      {
         case nt_link:
            nxt_flag = true;

            if (!lnk_stack)
            {
               if ((lnk_stack = cl_create_def()) == NULL)
               {
                  err_flag = true;
                  continue;
               }
            }
            switch (node->link->link_type)
            {
               case lt_if:
                  expr_fields = lxn_analyse(lxn_init_param(node->link->expr, fmd, &la_cb));

                  if (la_cb.e_code != lxec_noerror)
                  {
                     lxn_print_if_err(&la_cb);
                     err_flag = true;
                     continue;
                  }

                  if ((lmd = cl_new_item(lnk_stack, sizeof(struct t_link_md))) == NULL)
                  {
                     err_flag = true;
                     continue;
                  }

                  lmd->level  = level;
                  lmd->node   = node;

                  if (expr_fields != 0)
                  {
                     lmd->ds     = ds_true;
                     node        = node->link->child;
                     level++;
                     continue;
                  }
                  else lmd->ds = ds_false;
               break;
               case lt_repeat:
                  if (!(node->link->expr && strlen(node->link->expr) > 0))
                  {
                     fprintf(stderr, "Error: @repeat without expression [type 0x%04X]!\n",
                             msg->mmd->message_type);
                     err_flag = true;
                     continue;
                  }

                  if ((lmd = cl_new_item(lnk_stack, sizeof(struct t_link_md))) == NULL)
                  {
                     err_flag = true;
                     continue;
                  }

                  lmd->node         = node;
                  lmd->rep_counter  = (*(node->link->expr) == '*') ? -1 : get_num(node->link->expr);
                  node              = node->link->child;
                  lmd->level        = level++;
               break;
               case lt_elif:
/*                  if (cl_is_empty(lnk_stack))
                  {
                     fprintf(stderr, "Error: @elif without preceding @if!\n");
                     err_flag = true;
                     continue;
                  }*/
                  lmd = (struct t_link_md *)cl_pick(lnk_stack, &ce);
                  if (!((ce == clec_noerr) && (lmd && (lmd->ds != ds_undef) && (lmd->level == level))))
                     err_flag = true;

                  assert(lmd->node->type == nt_link);

                  if (!err_flag &&
                      !((lmd->node->link->link_type == lt_if) || (lmd->node->link->link_type == lt_elif)))
                     err_flag = true;

                  if (err_flag)
                  {
                     fprintf(stderr, _("Error: @elif without preceding @if [type 0x%04X]!\n"),
                             msg->mmd->message_type);
                     continue;
                  }

                  lmd->node = node;

                  if (lmd->ds == ds_false)
                  {
                     expr_fields = lxn_analyse(lxn_init_param(node->link->expr, fmd, &la_cb));
                     if (la_cb.e_code != lxec_noerror)
                     {
                        lxn_print_if_err(&la_cb);
                        err_flag = true;
                        continue;
                     }

                     if (expr_fields != 0)
                     {
                        lmd->ds     = ds_true;
                        node        = node->link->child;
                        level++;
                        continue;
                     }
                  }
               break;
               case lt_else:
                  lmd = (struct t_link_md *)cl_pick(lnk_stack, &ce);
                  if (!((ce == clec_noerr) && (lmd && (lmd->ds != ds_undef) && (lmd->level == level))))
                     err_flag = true;

                  assert(lmd->node->type == nt_link);

                  if (!err_flag &&
                      !((lmd->node->link->link_type == lt_if) || (lmd->node->link->link_type == lt_elif)))
                     err_flag = true;

                  if (err_flag)
                  {
                     lmd->ds = ds_undef;
                     fprintf(stderr, _("Error: @else without preceding @if or @elif [type 0x%04X]!\n"),
                             msg->mmd->message_type);
                     err_flag = true;
                     continue;
                  }

                  lmd->node   = node;
                  lmd->ds     = ds_undef;

                  if (lmd->ds == ds_false)
                  {
                     node = node->link->child;
                     level++;
                     lmd->ds = ds_undef;
                     continue;
                  }
               break;
               case lt_end:
                  lmd = (struct t_link_md*)cl_pick(lnk_stack, &ce);

                  if (!((ce == clec_noerr) && lmd && (lmd->level == level)))
                  {
                     fprintf(stderr, _("Error: @end without preceding condition [type 0x%04X]!\n"),
                             msg->mmd->message_type);
                     err_flag = true;
                     continue;
                  }

                  assert(lmd->node->type == nt_link);
                  if ((lmd->node->link->link_type == lt_if) ||
                      (lmd->node->link->link_type == lt_elif) ||
                      (lmd->node->link->link_type == lt_else))
                  {
                     lmd = (struct t_link_md*)cl_pop(lnk_stack, &ce);
                     assert(ce == clec_noerr);
                     dfree(lmd);
                  }
                  else if (lmd->node->link->link_type == lt_repeat)
                  {
                     if ((lmd->rep_counter == -1) && (msg->pointer < data_end))
                     {
                        level++;
                        node     = lmd->node->link->child;
                        nxt_flag = false;
                     }
                     else if (lmd->rep_counter-- > 0)
                     {
                        level++;
                        node     = lmd->node->link->child;
                        nxt_flag = false;
                     }
                     else
                     {
                        lmd = (struct t_link_md*)cl_pop(lnk_stack, &ce);
                        assert(ce == clec_noerr);
                        dfree(lmd);
                     }
                  }
                  else
                  {
                     err_flag = true;
                     continue;
                  }

               break;
               case lt_undef:
               default:
                  fprintf(stderr, _("Error: link type \"%d\" is not defined [message type 0x%04X]!\n"),
                          node->link->link_type, msg->mmd->message_type);
                  err_flag = true;
                  continue;
               break;
            }

         break;
         case nt_metafield:
            nxt_flag = true;

            if (strceq(node->metafield->name, "$is_compound"))
               msg->mmd->is_compound = (bool)(atoi(node->metafield->value.data_ptr));
            else if (strceq(node->metafield->name, "$msg_descr"))
               msg->mmd->message_descr = node->metafield->value.data_ptr;
            else if (strceq(node->metafield->name, "$remainder_descr"))
               msg->remainder_descr = node->metafield->value.data_ptr;
/*            else if (strceq(node->metafield->name, "$response_descr"))
            {
               if ((resp = add_message_resp(msg->responses)) == NULL)
               {
                  fprintf(stderr, _("Error: can't add response record [message type 0x%04X]!\n"),
                          msg->mmd->message_type);
                  err_flag = true;
                  continue;
               }

               resp->descr       = node->metafield->value.data_ptr;
               resp->error_level += cur_err_level;
               msg->error_level  += cur_err_level;
            }*/
            else if (strceq(node->metafield->name, "$response"))
            {
               if ((resp = add_field_resp(fmd, node->metafield->value.data_ptr)) == NULL)
               {
//                  fprintf(stderr, _("Error: can't add response record [message type 0x%04X]!\n"),
//                          msg->mmd->message_type);
//                  err_flag = true;
//                  continue;
               }
               else msg->error_level += resp->error_level;
            }
            else if (strceq(node->metafield->name, "$error_level"))
            {
               msg->error_level = atoi(node->metafield->value.data_ptr);
            }
            /*else
            {
               fprintf(stderr, "Unknown metafield %s in message type 0x%04X\n", kvp->key, mmd->message_type);
               return(-1);
            }*/
         break;
         case nt_field:
            // Data size control. Summary fields size <= data size.
            msg_field  = NULL;
            if (msg->pointer < data_end)
            {
               // Calculating data length for non-fixed length field.
               if (node->field->size < 1)
               {
                  if (data_remainder == 0) data_remainder = data_end - msg->pointer;
                  len = ((float)data_remainder) * node->field->size;
               }
               else len = node->field->size;

               if ((msg->pointer + len) > data_end) continue;

               if ((msg_field = add_message_field(fields)) == NULL)
               {
                  err_flag = true;
                  continue;
               }

               msg_field->field_md     = node->field;
               msg_field->field_data   = msg->pointer;
               msg_field->real_size    = len;

               msg->pointer += len;
            }
            
            // In the semantic-correct message descripiton file,
            // repeated field can occured only in the end of message fields description.
            nxt_flag =  !(msg_field && msg_field->field_md->is_repeat);
         break;
         default:
            fprintf(stderr, _("Error: unknown node type [%d] on level %d\n"), node->type, level);
            err_flag = true;
            continue;
      }

      if (err_flag) break;
      if (!nxt_flag) continue;

      node = get_next_node(node, &level);
   }

   while ((lmd = cl_pop(lnk_stack, &ce)) != NULL)
   {
      fprintf(stderr, _("Error: need @end (message type = 0x%04X)!\n"),
              msg->mmd->message_type);
      err_flag = true;
      if (ce != clec_noerr) break;
      dfree(lmd);
   }
   cl_destroy(lnk_stack);

   if (err_flag)
   {
      msg_free_list(msg->fields);
//      msg_free_list(msg->responses);
      msg->fields = NULL;
   }
   else
   {
      msg->remainder_len = message_data_len(msg->raw_message) -
                           (msg->pointer - (uint8*)&(msg->raw_message->data));
   }

   return(!err_flag);
}
//------------------------------------------------------------------------------
static bool fli_ns_handler(void *fli, void *name)
{
   assert(fli  != NULL);
   assert(name != NULL);

   if (strceq(((struct t_message_fli*)(fli))->field_md->name,
              (char*)name)) return(false);
   return(true);
}
//------------------------------------------------------------------------------
struct t_message_fli *get_field_by_name(const char       *name,
                                        struct t_message *msg)
{
   assert(msg != NULL);

   if (!name) return(NULL);
   if (!(msg && msg->fields)) return(NULL);

   return((struct t_message_fli*)cl_rfind(msg->fields, &fli_ns_handler, name));
}
//------------------------------------------------------------------------------
bool get_metafield_by_name(struct t_full_msgdata      *fmd,
                           struct t_metafield         *mf)
{
   struct t_message *msg;

   if (!(mf && mf->name && fmd && fmd->msg && fmd->pmd)) return(false);
   if (!is_metafield_fs(mf->name[0])) return(false);

   msg = fmd->msg;

   if (strceq("$msg_type", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.type);
      mf->value.data = msg->raw_message->header.type;
      return(true);
   }
   else if (strceq("$msg_code", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.type) / 2;
      mf->value.data = MESSAGE_CODE(msg->raw_message);
      return(true);
   }
   else if (strceq("$msg_version", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.type) / 2;
      mf->value.data = MESSAGE_VERSION(msg->raw_message);
      return(true);
   }
   else if (strceq("$reciever", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.reciever);
      mf->value.data = msg->raw_message->header.reciever;
      return(true);
   }
   else if (strceq("$sender", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.sender);
      mf->value.data = msg->raw_message->header.sender;
      return(true);
   }
   else if (strceq("$reciever_name", mf->name))
   {
      mf->type             = mft_ptr;
      mf->value.data_ptr   = (char*)get_station_name_by_addr(fmd->pmd->stations, msg->raw_message->header.reciever);
      // Size in bytes.
      mf->size             = (mf->value.data_ptr) ? strlen(mf->value.data_ptr) : 0;
      return(true);
   }
   else if (strceq("$sender_name", mf->name))
   {
      mf->type             = mft_ptr;
      mf->value.data_ptr   = (char*)get_station_name_by_addr(fmd->pmd->stations, msg->raw_message->header.sender);
      // Size in bytes.
      mf->size             = (mf->value.data_ptr) ? strlen(mf->value.data_ptr) : 0;
      return(true);
   }
   else if (strceq("$time", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.time);
      mf->value.data = msg->raw_message->header.time;
      return(true);
   }
   else if (strceq("$id", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.id);
      mf->value.data = msg->raw_message->header.id;
      return(true);
   }
   else if (strceq("$msg_len", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.length);
      mf->value.data = msg->raw_message->header.length;
      return(true);
   }
   else if (strceq("$msg_descr", mf->name))
   {
      mf->type             = mft_ptr;
      if (msg->mmd)
      {
         // Not strlen_utf8, because need size in bytes, but not in symbols.
         mf->size             = (msg->mmd->message_descr) ?
                                 strlen(msg->mmd->message_descr) : 0;
         mf->value.data_ptr   = msg->mmd->message_descr;
      }
      else
      {
         // Size in bytes, not in symbols (not strlen_utf8()).
         mf->size             = strlen(_("unknown message"));
         mf->value.data_ptr   = _("unknown message");
      }
      return(true);
   }
   else if (strceq("$remainder_descr", mf->name))
   {
      mf->type             = mft_ptr;
      mf->size             = (msg->remainder_descr) ?
                              strlen(msg->remainder_descr) : 0;
      mf->value.data_ptr   = msg->remainder_descr;
      return(true);
   }
   else if (strceq("$data_length", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.length);
      mf->value.data = message_data_len(msg->raw_message);
      return(true);
   }
   else if (strceq("$is_compound", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = 1;
      mf->value.data = (msg->mmd) ? msg->mmd->is_compound : 0;
      return(true);
   }
   else if (strceq("$reserved", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->raw_message->header.reserved);
      mf->value.data = msg->raw_message->header.reserved;
      return(true);
   }
   else if (strceq("$crc_is_correct", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = 1;
      mf->value.data = message_crc(msg->raw_message) == msg->crc_real;
      return(true);
   }
   else if (strceq("$crc", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(uint16);
      mf->value.data = message_crc(msg->raw_message);
      return(true);
   }
   else if (strceq("$calc_crc", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(uint16);
      mf->value.data = msg->crc_real;
      return(true);
   }
   else if (strceq("$remainder", mf->name))
   {
      mf->type             = mft_ptr;
      mf->size             = msg->remainder_len;
      mf->value.data_ptr   = msg->pointer;
      return(true);
   }
   else if (strceq("$error_level", mf->name))
   {
      mf->type       = mft_data;
      mf->size       = sizeof(msg->error_level);
      mf->value.data = msg->error_level;
      return(true);
   }
   return(false);
}
//------------------------------------------------------------------------------
static __inline bool msg_init(struct t_full_msgdata *fmd)
{
   assert(fmd        != NULL);
   assert(fmd->msg   != NULL);

   if (!get_parsed_message(fmd)) return(false);

   fmd->msg->crc_real = crc16((uint8*)(fmd->msg->raw_message),
                              fmd->msg->raw_message->header.length -
                              message_crc_size);
   return(true);
}
//------------------------------------------------------------------------------
static
__inline bool msg_out(struct t_full_msgdata *fmd)
{
   register bool result;
  
   assert(fmd        != NULL);
   assert(fmd->msg   != NULL);

   if (fmd->msg->out_point->type == tiop_none) result = true;
   else result = shm_table[fmd->msg->out_fmt](fmd);

   if (result && fmd->msg->mmd && fmd->msg->mmd->is_compound)
      result = mdi_compound(fmd);
   return(result);
}
//------------------------------------------------------------------------------
static
__inline bool msg_out_in_all_dests(struct t_full_msgdata    *fmd,
                                   struct t_filter          *filter)
{
   struct t_io_point *iop;
   t_cl_error        err;
   bool              result = true;
   bool              f_headers;
   bool              f_compound;

   assert(fmd                 != NULL);
   assert(fmd->msg            != NULL);
   assert(fmd->msg->mmd       != NULL);
   assert(fmd->pmd            != NULL);
   assert(fmd->pmd->filters   != NULL);
   assert(filter              != NULL);

   fmd->msg->out_fmt          = filter->out_fmt;
   fmd->msg->out_only_headers = filter->f_only_header;
   f_headers                  = fmd->msg->out_only_headers;
   f_compound                 = fmd->msg->mmd->is_compound;

   iop = cl_cur_to_first(filter->dests, &err);
   if (!iop)
   {
      fmd->msg->out_point = fmd->pmd->filters->def_out;
      if (!msg_out(fmd)) result = false;
   }
   else while (iop && (err == clec_noerr))
   {
      fmd->msg->out_point = iop;
      if (!msg_out(fmd))
      {
         result = false;
         break;
      }
      // Messages in this compound will be output one time.
      if (fmd->msg->mmd->is_compound)
      {
         fmd->msg->mmd->is_compound   = false;
         fmd->msg->out_only_headers   = true;
      }
      iop = cl_next(filter->dests, &err);
   }

   fmd->msg->out_only_headers   = f_headers;
   fmd->msg->mmd->is_compound   = f_compound;

   return(result);
}
//------------------------------------------------------------------------------
static
__inline bool make_single_action(struct t_full_msgdata   *fmd,
                                 struct t_filter_action  *action)
{
   assert(fmd     != NULL);
   assert(action  != NULL);

   if (action->type == fat_none) return(true);
   return(actions_table[action->type](fmd, action));
}
//------------------------------------------------------------------------------
static
bool make_actions(struct t_full_msgdata   *fmd,
                  struct t_common_list    *actions)
{
   struct t_filter_action  *action;
   t_cl_error              err;

   action = cl_cur_to_first(actions, &err);

   while (action && (err == clec_noerr))
   {
      if (!make_single_action(fmd, action)) return(false);
      action = cl_next(actions, &err);
   }

   return(true);
}
//------------------------------------------------------------------------------
static __inline bool msg_uninit(struct t_message *msg)
// Clears in get_parsed_message().
{
   bool result;

   assert(msg != NULL);

   result      = msg_free_list(msg->fields);
   msg->fields = NULL;

   return(result);
}
//------------------------------------------------------------------------------
static
bool parse_message(struct t_full_msgdata *fmd)
// Call common message parser, which make message fields list, then
// Call message representer.
// It's called recursively in compound messages.
{
// TODO: Rewrite it!
   struct t_filter            *filter;
   struct t_la_control_block  la_cb;
   struct t_parser_metadata   *pmd;
   struct t_message           *msg;
   bool                       break_flag  = false;
   bool                       out_flag    = false;
   bool                       init_flag   = false;
   bool                       expr_flag   = false;
   bool                       ff_flag     = false;
   t_cl_error                 err;

   assert(fmd           != NULL);

   pmd = fmd->pmd;
   msg = fmd->msg;

   assert(pmd           != NULL);
   assert(pmd->filters  != NULL);
   assert(msg           != NULL);

   filter = cl_cur_to_first(pmd->filters->filters, &err);
   while (filter && (err == clec_noerr))
   {

      if (filter->enabled && (filter->msg_type == msg->raw_message->header.type))
      {

         out_flag = false;
         if (!filter->sources)
         {
            out_flag = true;
         }
         else
         {
            if (io_point_in_list(filter->sources, pmd->in_point))
            {
               out_flag = true;
               break;
            }
         }

         if (out_flag || filter->actions)
         {
            if (!msg->mmd) msg->mmd = get_message_metadata(msg->raw_message, pmd->md_cache);
            if (!init_flag && ((init_flag = msg_init(fmd)) == false)) return(false);
            if (msg->mmd && filter->expr)
            {
               expr_flag = (bool)lxn_analyse(lxn_init_param(filter->expr, fmd, &la_cb));
               if (la_cb.e_code != lxec_noerror)
               {
                  lxn_print_if_err(&la_cb);
                  out_flag = expr_flag = false;
                  break;
               }
            }
            // Message without metadata always has true expression.
            else expr_flag = true;
         }

         out_flag = expr_flag && out_flag;

         if (out_flag)
         {
            if (!ff_flag) ff_flag = true;
            if (!msg_out_in_all_dests(fmd, filter))
            {
               fprintf(stderr, "Error: can't output data for message with type [0x%04X]!\n",
                       msg->raw_message->header.type);
               break;
            }
         }

         if (filter->actions && expr_flag)
         {
            if (!make_actions(fmd, filter->actions)) break;
         }
      } // if (filter->msg_type == msg_type)

      if (break_flag) break;
      filter = cl_next(pmd->filters->filters, &err);
   }

   // Applicable filters was not found.
   if (!ff_flag)
   {
      if (!msg->mmd) msg->mmd = get_message_metadata(msg->raw_message, pmd->md_cache);

      if (msg->mmd && (!init_flag && (init_flag = msg_init(fmd)) == false)) return(false);
      msg->out_point          = pmd->filters->def_out;
      msg->out_fmt            = pmd->def_out_fmt;
      msg->out_only_headers   = false;
      if (!msg_out(fmd))
      {
         msg_uninit(msg);
         return(false);
      }
   }

   return((init_flag) ? msg_uninit(msg) : true);
}
//------------------------------------------------------------------------------
static
bool mdi_compound(struct t_full_msgdata *fmd)
// Compound messages parser.
{
   // Used for show compound message.
   register struct t_message_raw *inner_raw_msg;
   struct t_message              inner_msg;
   struct t_full_msgdata         inner_fmd;
   // I have many registers on x64.
   register uint16      i        = 0;
   register uint16      data_sz;
   register uint8       *data;
   bool                 result   = true;

   assert(fmd                    != NULL);
   assert(fmd->msg               != NULL);
   assert(fmd->msg->raw_message  != NULL);

   inner_fmd.msg  = &inner_msg;
   inner_fmd.pmd  = fmd->pmd;

   data_sz        = message_data_len(fmd->msg->raw_message);
   data           = (uint8*)&(fmd->msg->raw_message->data);

   while (i < data_sz)
   {
      inner_raw_msg = (struct t_message_raw*)(data + i);
      i += inner_raw_msg->header.length;

      // Malformed inner message length.
      if (i > data_sz)
      {
         fprintf(stderr,
                 _("Error: inner message length greater, than compound message length!\n"));
         return(false);
      }
      memzero(&inner_msg, sizeof(struct t_message));
      inner_msg.raw_message   = inner_raw_msg;
//      inner_msg.header_md     = msg->header_md;
      result                  = (parse_message(&inner_fmd)) ?
                                result : false;
      msg_free_list(inner_msg.fields);
      inner_msg.fields = NULL;
   }
   return(result);
}
//------------------------------------------------------------------------------
__inline bool parse_raw_message(const struct t_message_raw     *raw_msg,
                                struct t_parser_metadata       *pmd)
// Create full message structure for raw mesage and call parser.
{
   struct t_message        msg;
   struct t_full_msgdata   fmd;

   assert(raw_msg != NULL);
   assert(pmd     != NULL);

   memzero(&msg, sizeof(struct t_message));
   msg.raw_message   = raw_msg;
   fmd.msg           = &msg;
   fmd.pmd           = pmd;;
//   msg.header_md     = pmd->header_fields_md;
   return(parse_message(&fmd));
}
//------------------------------------------------------------------------------
bool process_messages(struct t_message_raw      **raw_buffer,
                      struct t_parser_metadata  *pmd)
// Load message in raw_buffer.
{
   struct t_io_point    *src;
   t_cl_error           err;

   if (!raw_buffer) return(false);
   src = cl_cur_to_first(pmd->filters->sources, &err);
   while (src && (err == clec_noerr))
   {

      switch (src->type)
      {
         case tiop_file:
         case tiop_stream:
            // Doesn't work under M$ windows.
            if (src->fs) *raw_buffer = load_msg_from_file(src->fs);
         break;
         case tiop_net:
            #if defined(_WIN32) && (HAVE__GET_OSFHANDLE == 1)
            if (src->fs) *raw_buffer = load_msg_from_net(_get_osfhandle(fileno(src->fs)));
            #else
            if (src->fs) *raw_buffer = load_msg_from_net(fileno(src->fs));
            #endif
         break;
         case tiop_none:
            dfree(*raw_buffer);
            *raw_buffer = NULL;
         break;
         default:
            fprintf(stderr, _("Error: unknown IO point type (%d)!\n"), src->type);
            dfree(*raw_buffer);
            return(false);
      }

      if (*raw_buffer)
      {
         #if DEBUG_LEVEL >= 3
         fprintf(stderr, "Raw buffer length: \t%d\nMessage type: \t\t0x%04X\n-----\n",
                 (*raw_buffer)->header.length, (*raw_buffer)->header.type);
         #endif
         pmd->in_point = src;
         parse_raw_message(*raw_buffer, pmd);
         dfree(*raw_buffer);
         *raw_buffer = NULL;
      }
      src = cl_next(pmd->filters->sources, &err);
   }

   return((err == clec_noerr) || (err == clec_last_item));
}
//------------------------------------------------------------------------------
