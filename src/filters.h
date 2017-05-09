//------------------------------------------------------------------------------
// Message filters and separators.
//------------------------------------------------------------------------------

#ifndef _filters_h
#define _filters_h
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
   #include <winsock2.h>
#else
   #include <sys/socket.h>
   #include <arpa/inet.h>
   #include <netinet/in.h>
#endif
//------------------------------------------------------------------------------
#include "common.h"
#include "utils.h"
#include "msgmetadata.h"
#include "iniparser.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
// IO stream type.
typedef enum { tios_unknown = -1, tios_stdin, tios_stdout, tios_stderr } t_ios_type;
// IO point type.
typedef enum { tiop_unknown = -1, tiop_net, tiop_file, tiop_stream,
               tiop_none } t_iop_type;
//------------------------------------------------------------------------------
struct t_io_point
{
   t_iop_type type;

   union
   {
      struct
      {
         struct sockaddr_in   m_addr;
//         int                  sock;
      } net;
      struct
      {
         char *filename;
      } file;
      struct
      {
         t_ios_type  type;
      } stream;
   };
   FILE *fs;
//   uint16 hash;
};
//------------------------------------------------------------------------------
typedef enum { fat_unknown = -1, fat_none, fat_system, fat_msg, fat_write }
               t_fltaction_type;
//------------------------------------------------------------------------------
struct t_filter_action
{
   t_fltaction_type  type;
   char              *params;
};
//------------------------------------------------------------------------------
struct t_filter
{
   t_message_type          msg_type;
   char                    *expr;
   bool                    enabled;

   t_msg_out_fmt           out_fmt;
   char                    *descr;
   bool                    f_only_header;
   // Actions is local for the filter.
   struct t_common_list    *actions;
   // List of addresses in global (per base) io points lists.
   struct t_common_list    *sources;
   struct t_common_list    *dests;
};
//------------------------------------------------------------------------------
struct t_filters_base
{
   struct t_common_list *sources;
   struct t_common_list *dests;

   struct t_common_list *filters;
   struct t_io_point    *def_in;
   struct t_io_point    *def_out;
   t_msg_out_fmt        def_out_fmt;
};
//------------------------------------------------------------------------------
struct t_base  *load_filters_db(const char            *filename,
                                struct t_filters_base **filters_base,
                                char                  *def_in,
                                char                  *def_out,
                                t_msg_out_fmt         def_out_fmt);

struct t_filters_base *init_filters_db(struct t_base     *filters,
                                       char              *def_in,
                                       char              *def_out,
                                       t_msg_out_fmt     def_out_fmt);

bool                    destroy_filters_db(struct t_base         *filters,
                                           struct t_filters_base *filters_base);
//------------------------------------------------------------------------------
// For debug.
void dump_io_point(struct t_io_point *iop);
bool dump_filters_db(struct t_filters_base *fdb);
//------------------------------------------------------------------------------
bool get_filters_by_msgtype(struct t_filters_base *fdb,
                            const t_message_type msg_type,
                            bool (*filter_iter)(struct t_filter *filter, void *user),
                            void *user_data);
//------------------------------------------------------------------------------
bool init_io_point(struct t_io_point *iop, const bool input);
bool uninit_io_point(struct t_io_point *iop);
bool is_iopoints_eq(struct t_io_point *iop0, struct t_io_point *iop1);
bool io_point_in_list(struct t_common_list *io_points, struct t_io_point *iop);
//------------------------------------------------------------------------------
// Convert text format representation (csv, raw, prn) to the enum.
__inline t_msg_out_fmt get_out_format(const char *fmt_str);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

