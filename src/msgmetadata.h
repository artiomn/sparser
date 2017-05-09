//------------------------------------------------------------------------------
// Common logic for work with messages metadata.
//------------------------------------------------------------------------------

#ifndef _msgmetadata_h
#define _msgmetadata_h
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
//------------------------------------------------------------------------------
#include "common.h"
#include "iniparser.h"
#include "raw_message.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
typedef enum { mof_unknown = -1, mof_prn, mof_raw, mof_csv } t_msg_out_fmt;
//------------------------------------------------------------------------------
typedef enum { nt_field, nt_link, nt_metafield } t_node_type;
//------------------------------------------------------------------------------
typedef enum {
               lt_undef = -1,
               lt_if = 0, lt_elif, lt_else,
               lt_repeat, lt_end
             } t_link_type;
//------------------------------------------------------------------------------
typedef enum { mft_ptr, mft_data } t_mf_type;
//------------------------------------------------------------------------------
// Message field metadata.
struct t_message_field_md
{
   float                size;
   char                 out_spec;
   // Repeating field in the end of message.
   bool                 is_repeat;
   char                 *name;
   char                 *descr;
};
//------------------------------------------------------------------------------
struct t_message_tree_node;
//------------------------------------------------------------------------------
struct t_link_data
{
   t_link_type                link_type;
   // Expression for this link. If true - link accepted.
   char                       *expr;
   struct t_message_tree_node *child;
};
//------------------------------------------------------------------------------
struct t_header_field
{
   char  *name;
   char  *descr;
   char  out_spec;
};
//------------------------------------------------------------------------------
struct t_header_metadata
{
   struct t_common_list *fields_md;
   unsigned int         max_desc_len;
};
//------------------------------------------------------------------------------
struct t_metafield
{
   t_mf_type   type;
   uint16      size;
   char        *name;
   union
   {
      uint32   data;
      uint8    *data_ptr;
   } value;
};
//------------------------------------------------------------------------------
struct t_message_tree_node
{
   t_node_type                   type;
   union
   {
      // For type == nt_field.
      struct t_message_field_md  *field;
      // For type == nt_link.
      struct t_link_data         *link;
      // For type == nt_metafield.
      struct t_metafield         *metafield;
   };

   // If NULL, node is the last node in the list.
   struct t_message_tree_node    *next;
   struct t_message_tree_node    *parent;
};
//------------------------------------------------------------------------------
struct t_message_field_tree
{
   // Summ of field sizes.
   uint16                        fields_size;

   struct t_message_tree_node    *root;
   struct t_message_tree_node    *cur_node;
};

//------------------------------------------------------------------------------
// Message metadata table.
//------------------------------------------------------------------------------

struct t_message_metadata
{
   t_message_type                message_type;
   char                          *message_descr;
   bool                          is_compound;

   // Maximum description length.
   unsigned short int            msg_out_max_dl;
   // Summary size of fixed-size message fields.
   unsigned short                min_data_sz;

   // Fields list for the current message of this type.
   struct t_message_field_tree   *fields;
};
//------------------------------------------------------------------------------
struct t_header_metadata   *init_header_fields_db(const char *filename);
bool                       destroy_header_fields_db(struct t_header_metadata *hmd);
//------------------------------------------------------------------------------
struct t_base              *init_messages_db(const char *filename,
                                             struct t_common_list **md_cache);
bool                       destroy_messages_db(struct t_base *messages,
                                               struct t_common_list *md_cache);
struct t_message_metadata  *get_message_md_by_type(const t_message_type type,
                                                   struct t_common_list *md_cache);
//------------------------------------------------------------------------------
__inline struct t_message_metadata *get_message_metadata(struct t_message_raw *msg,
                                                         struct t_common_list *md_cache);
//------------------------------------------------------------------------------
// DEBUG: Print field tree for the message of given type. If ok, return 0.
bool print_msg_tree(const t_message_type msg_type,
                    struct t_common_list *md_cache);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
