//------------------------------------------------------------------------------
// Message parser.
//------------------------------------------------------------------------------

#ifndef _msgparser_h
#define _msgparser_h
//------------------------------------------------------------------------------
#include "common.h"
#include "raw_message.h"
#include "msgmetadata.h"
#include "filters.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
// Parsed message fields.
struct t_message_fli
{
   struct t_message_field_md  *field_md;
   struct t_common_list       *responses;

   unsigned int               real_size;
   uint8                      *field_data;
};
//------------------------------------------------------------------------------
struct t_response
{
   char  *descr;
   int   error_level;
};
//------------------------------------------------------------------------------
struct t_message
{
   struct t_message_metadata     *mmd;
   struct t_message_raw          *raw_message;
   // Responses descr.
//   struct t_common_list          *responses;

   struct t_io_point             *out_point;
   struct t_common_list          *fields;

   t_msg_out_fmt                 out_fmt;
   bool                          out_only_headers;

   // Summary error level.
   int                           error_level;
   uint8                         *pointer;
   char                          *remainder_descr;
   uint16                        remainder_len;
   uint16                        crc_real;
};
//------------------------------------------------------------------------------
struct t_parser_metadata
{
   struct t_common_list       *md_cache;
   struct t_header_metadata   *header_fields_md;
   struct t_filters_base      *filters;
   struct t_common_list       *stations;
   struct t_io_point          *in_point;
   t_msg_out_fmt              def_out_fmt;
};
//------------------------------------------------------------------------------
struct t_full_msgdata
{
   struct t_message           *msg;
   struct t_parser_metadata   *pmd;
};
//------------------------------------------------------------------------------
struct t_message_fli *get_field_by_name(const char       *name,
                                        struct t_message *msg);
//------------------------------------------------------------------------------
// Return metafield in mf, for msg with name == mf->name.
// If ok, return true.
bool get_metafield_by_name(struct t_full_msgdata   *fmd,
                           struct t_metafield      *mf);
//------------------------------------------------------------------------------
__inline bool parse_raw_message(const struct t_message_raw     *raw_msg,
                                struct t_parser_metadata       *pmd);
//------------------------------------------------------------------------------
bool process_messages(struct t_message_raw      **raw_buffer,
                      struct t_parser_metadata  *pmd);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
