//------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "filters.h"
#include "network.h"
#include "lexan.h"
//------------------------------------------------------------------------------
static
bool destroy_io_point(struct t_io_point *io_point)
{
   bool result;

   assert(io_point != NULL);

   result = uninit_io_point((struct t_io_point*)io_point);
   dfree(io_point);
   return(result);
}
//------------------------------------------------------------------------------
static void io_point_destructor(void *io_point)
{
   destroy_io_point((struct t_io_point*)io_point);
}
//------------------------------------------------------------------------------
static void action_destructor(void *action)
{
   dfree((struct t_filter_action*)action);
}
//------------------------------------------------------------------------------
static void filter_destructor(void *filter)
{
   assert(filter != NULL);

   cl_destroy(((struct t_filter*)filter)->actions);
   cl_destroy(((struct t_filter*)filter)->sources);
   cl_destroy(((struct t_filter*)filter)->dests);
   dfree(filter);
}
//------------------------------------------------------------------------------
static struct t_filter *add_filter(struct t_filters_base *fdb)
{
   struct t_filter *filter = cl_new_item(fdb->filters, sizeof(struct t_filter));

   if (!filter) return(NULL);

   assert(fdb != NULL);

   filter->out_fmt = fdb->def_out_fmt;
   filter->enabled = true;

   return(filter);
}
//------------------------------------------------------------------------------
static
struct t_filter_action *add_filter_action(struct t_common_list *actions,
                                          struct t_filter_action* action)
{
   assert(action != NULL);

   t_cl_error err = cl_push(actions, action);
   if (err != clec_noerr) return(NULL);
   return(action);
}
//------------------------------------------------------------------------------
static
struct t_io_point *add_io_point(struct t_common_list *iops,
                                struct t_io_point *iop)
// Add IO point to list.
{
   assert(iop != NULL);

   t_cl_error err = cl_push(iops, iop);
   #if DEBUG_LEVEL >= 2
   if (err != clec_noerr) fprintf(stderr, "Error: IO point can't be add [code: %d]\n", err);
   #endif
   if (err != clec_noerr) return(NULL);
   return(iop);
}
//------------------------------------------------------------------------------
bool init_io_point(struct t_io_point *iop, const bool input)
// Initialize IO point: open files, network connections, set std stream
// descriptors.
{

   assert(iop != NULL);

   switch (iop->type)
   {
      case tiop_net:
//         if ((iop->net.sock = open_dgram_socket(&(iop->net.m_addr))) < 0)
//            return(false);
         if ((iop->fs = fopen_dgram_socket(&(iop->net.m_addr), (input) ? "rb" : "wb" )) == NULL)
            return(false);
      break;
      case tiop_file:
         #if DEBUG_LEVEL >= 3
         fprintf(stderr, "Opening file: %s, mode: %s\n", iop->file.filename, (input) ? "rb" : "wb");
         #endif
         if ((iop->fs = fopen(iop->file.filename, (input) ? "rb" : "wb" )) == NULL)
            return(false);
         #if DEBUG_LEVEL >= 3
         fprintf(stderr, "File was opened sucsessfully [descr: %p].\n", iop->fs);
         #endif
      break;
      case tiop_stream:
         switch(iop->stream.type)
         {
            case tios_stdin:
               iop->fs = stdin;
            break;
            case tios_stdout:
               iop->fs = stdout;
            break;
            case tios_stderr:
               iop->fs = stderr;
            break;
            case tios_unknown:
            default:
               fprintf(stderr, _("Error: unknown stream type!\n"));
               return(false);
         }
      case tiop_none:
      break;
      default:
         assert("Unknown iop type" && false);

   }

   return(true);
}
//------------------------------------------------------------------------------
bool uninit_io_point(struct t_io_point *iop)
{
   assert(iop != NULL);

   switch (iop->type)
   {
      case tiop_net:
         #if (DEBUG_LEVEL >= 3)
         fprintf(stderr, "Closing socket...\n");
         #endif
         assert(iop->fs != NULL);
         //return(cp_sockclose(iop->net.sock) >= 0);
         return(fclose(iop->fs) >= 0);
      break;
      case tiop_file:
         #if (DEBUG_LEVEL >= 3)
         fprintf(stderr, "Closing file %s...\n", iop->file.filename);
         #endif
         assert(iop->fs != NULL);
         // Allocated by realpath().
         dfree(iop->file.filename);
         return(fclose(iop->fs) >= 0);
      break;
      case tiop_stream:
         assert(iop->fs != NULL);
         return(true);
      break;
      case tiop_none:
         return(true);
      break;
      default:
         fprintf(stderr, "Type: %d\n", iop->type);
         assert("Unknown iop type" && false);
   }
   return(false);
}
//------------------------------------------------------------------------------
bool is_iopoints_eq(struct t_io_point *iop0, struct t_io_point *iop1)
{
   if (!(iop0 && iop1)) return(false);

   if (iop0->type != iop1->type) return(false);
   switch (iop0->type)
   {
      case tiop_net:
         // Equal, if memcmp() == 0.
         return(memcmp(&(iop0->net.m_addr), &(iop1->net.m_addr),
                       sizeof(struct sockaddr_in)) == 0);
      break;
      case tiop_file:
         return(streq(iop0->file.filename, iop1->file.filename));
      break;
      case tiop_stream:
      case tiop_none:
         return(iop0->stream.type == iop1->stream.type);
      break;
      default:
         assert("Unknown iop type" && false);
   }
   return(false);
}
//------------------------------------------------------------------------------
bool io_point_in_list(struct t_common_list *io_points, struct t_io_point *iop)
{
   struct t_io_point *cur_iop;
   t_cl_error        err;

   cur_iop = cl_cur_to_first(io_points, &err);
   while (cur_iop && (err == clec_noerr))
   {
      if (is_iopoints_eq(iop, cur_iop)) return(true);
      cur_iop = cl_next(io_points, &err);
   }
   return(false);
}
//------------------------------------------------------------------------------
static bool io_point_iterator(void *io_point, void *user_iop)
{
   struct t_io_point *iop  = (struct t_io_point*)io_point,
                     *uiop = (struct t_io_point*)user_iop;

   return(!is_iopoints_eq(iop, uiop));
}
//------------------------------------------------------------------------------
static
struct t_io_point *find_io_point(struct t_common_list *iops,
                                 const struct t_io_point *iop)
{
   return(cl_find(iops, io_point_iterator, iop));
}
//------------------------------------------------------------------------------
static
bool iop_setup_file_params(struct t_io_point *iop, char *param_string,
                           const char *def_name)
{
   char *spos     = param_string;
   char *filename = NULL;
   FILE *fs;

   spos = skip_spaces(spos);
   if (!spos || (*spos == '\0'))
   {
      // Def filename.
      if (filename == NULL)
      {
         fprintf(stderr, _("Error: default filename is not specified!\n"));
         return(false);
      }
      filename = def_name;
   }
   else if ((*spos != ',') || (*(spos + 1) == '\0'))
   {
      fprintf(stderr, _("Error: filename is not specified!\n"));
      return(false);
   }
   else
   {
      spos     = skip_spaces(spos + 1);
      filename = spos;
   }

   // It's ugly, but it works.
   if (((fs = fopen(filename, "rb")) == NULL) &&
       ((fs = fopen(filename, "wb")) == NULL))
   // If file doesn't exists, realpath return error.
   {
      fprintf(stderr, _("Error: can't open file \"%s\"!\n"), filename);
      return(false);
   }
   if (fclose(fs) != 0) return(false);
   assert(iop != NULL);
   // Absolute pathname. Based on GNU LIB in M$ Windows.
   iop->file.filename = realpath(filename, NULL);
   return(true);
}
//------------------------------------------------------------------------------
static
bool iop_setup_net_params(struct t_io_point *iop, char *param_string,
                          struct sockaddr_in *defaults)
{
   char *port_delim;
   char *spos           = param_string;
   int   port           = (defaults) ? defaults->sin_port : -1;
   bool def_addr        = true;

   assert(iop != NULL);

   spos = skip_spaces(spos);
   if (spos == '\0')
   {
      // Def addr and port.
   }
   else if ((*spos != ',') || (*(spos + 1) == '\0'))
   {
      fprintf(stderr, _("Error: address is not defined!\n"));
      return(false);
   }
   else
   {
      spos        = skip_spaces(spos + 1);
      port_delim  = strchr(spos, ':');
      if (port_delim == NULL)
      {
         // Def port.
         if (!defaults)
         {
            fprintf(stderr, _("Error: port is not specified!\n"));
            return(false);
         }
         def_addr = false;
      }
      else if (spos - port_delim == 0)
      {
         // Def addr.
         if ((++port_delim == NULL) || (*port_delim == '\0'))
         {
            fprintf(stderr, _("Error: port is not specified!\n"));
            return(false);
         }
         port = get_num(port_delim);
      }
      else
      {
         def_addr    = false;
         *port_delim = '\0';
         port_delim  = skip_spaces(port_delim + 1);

         if ((port_delim == NULL) || (*port_delim == '\0'))
         {
            fprintf(stderr, _("Error: port is not specified!\n"));
            return(false);
         }
         port = get_num(port_delim);
      }
   }

   iop->net.m_addr.sin_family = (defaults) ? defaults->sin_family : AF_INET;
   iop->net.m_addr.sin_port   = htons(port);
   if (!def_addr)
   {
      if (!inet_pton(iop->net.m_addr.sin_family, rstrip(spos), &(iop->net.m_addr.sin_addr)))
      {
         fprintf(stderr, _("Error: address conversion error!\n"));
         return(false);
      }
   }
   else
   {
      if (!defaults)
      {
         fprintf(stderr, _("Error: default address is not defined!\n"));
         return(false);
      }
      memcpy(&(iop->net.m_addr.sin_addr), &(defaults->sin_addr), sizeof(struct in_addr));
   }

   return(true);
}
//------------------------------------------------------------------------------
static
struct t_io_point *parse_io_point_params(struct t_filters_base *fdb,
                                         const bool            input,
                                         char                  *param_string)
// Parse io point parameters string.
// Find io point or create another.
// Note: allocated result is added to list and need to free somewhere.
// Filename also need to deallocated.
{
   struct t_io_point    *result  = NULL;
   struct t_io_point    iop;
   struct t_common_list *iops    = (input) ? fdb->sources : fdb->dests;
   char                 *spos    = param_string;
   int                  len      = 0;

   assert(fdb != NULL);

   memzero(&iop, sizeof(struct t_io_point));
   if (strceqn(spos, "file", len = strlen("file")))
   {
      iop.type          = tiop_file;
   }
   else if (strceqn(spos, "network", len = strlen("network")))
   {
      iop.type          = tiop_net;
   }
   else if (input && strceq(spos, "stdin"))
   {
      iop.type          = tiop_stream;
      iop.stream.type   = tios_stdin;
      len               = strlen("stdin");
   }
   else if (!input && strceq(spos, "stdout"))
   {
      iop.type          = tiop_stream;
      iop.stream.type   = tios_stdout;
      len               = strlen("stdout");
   }
   else if (!input && strceq(spos, "stderr"))
   {
      iop.type          = tiop_stream;
      iop.stream.type   = tios_stderr;
      len               = strlen("stdout");
   }
   else if (strceq(spos, "none"))
   {
      iop.type          = tiop_none;
      len               = strlen("none");
   }
   else
   {
      fprintf(stderr, _("Error: unknown %s specification in \"%s\"!\n"),
         (input) ? "input source" : "output destination", param_string);

      return(NULL);
   }

   switch (iop.type)
   {
      case tiop_file:
         if (!iop_setup_file_params(&iop, spos + len,
                                    (input) ?
                                    (fdb->def_in && (fdb->def_in->type == tiop_file) ? fdb->def_in->file.filename : NULL) :
                                    (fdb->def_out && (fdb->def_out->type == tiop_file) ? fdb->def_out->file.filename : NULL)))
            return(NULL);
      break;
      case tiop_net:
         if (!iop_setup_net_params(&iop, spos + len,
                                   (input) ?
                                   (fdb->def_in && (fdb->def_in->type == tiop_net) ? &(fdb->def_in->net.m_addr) : NULL) :
                                   (fdb->def_out && (fdb->def_out->type == tiop_net) ? &(fdb->def_out->net.m_addr) : NULL)))
            return(NULL);
      break;
      case tiop_stream:
         if (*(spos + len) != '\0')
         {
            fprintf(stderr, _("Error: trailing characters after point specification \"%s\"!\n"),
                    param_string);
            return(NULL);
         }
         if (input && (iop.stream.type != tios_stdin))
         {
            fprintf(stderr, _("Error: not allowed input source!\n"));
            return(NULL);
         }
         if (!input && (iop.stream.type == tios_stdin))
         {
            fprintf(stderr, _("Error: STDIN is an incorrect destination!\n"));
            return(NULL);
         }
      break;
      case tiop_none:
         if (*(spos + len) != '\0')
         {
            fprintf(stderr, _("Error: trailing characters after point specification \"%s\"!\n"),
                    param_string);
            return(NULL);
         }
      break;
      default:
         assert("Impossible error" && false);
         return(NULL);
   }

   result = find_io_point(iops, &iop);
   if (!result)
   {
      result = dmalloc(sizeof(struct t_io_point));
      if (!result) return(NULL);
      memcpy(result, &iop, sizeof(struct t_io_point));

      if (!add_io_point(iops, result))
      {
         dfree(result);
         result = NULL;
      }
      else if (!init_io_point(result, input))
      {
         dfree(result);
         result = NULL;
      }
   }

   return(result);
}
//------------------------------------------------------------------------------
static
struct t_filter_action *parse_action_params(char *param_string)
{
   struct t_filter_action  *result  = NULL;
   t_fltaction_type        act_type = fat_unknown;
   char                    *spos    = param_string;
   int                     len      = 0;

   if (strceqn(spos, "system", len = strlen("system")))
   {
      act_type = fat_system;
   }
   else if (strceqn(spos, "message", len = strlen("message")))
   {
      act_type = fat_msg;
   }
/* Nafig.
    else if (strceqn(spos, "write", len = strlen("write")))
   {
      act_type = fat_write;
   }
*/   
   else if (strceq(spos, "none"))
   {
      act_type = fat_none;
      len = strlen("none");
   }
   else
   {
      fprintf(stderr, _("Error: unknown action type in \"%s\"!\n"),
              param_string);
      return(NULL);
   }

   spos = skip_spaces(spos + len);
   switch (act_type)
   {
      case fat_system:
      case fat_msg:
         if (!(spos && (*spos == ',')))
         {
            fprintf(stderr, _("Error: incorrect action specification \"%s\"!\n"),
                    param_string);
            return(NULL);
         }
         spos = skip_spaces(spos + 1);
         if (!(spos && *spos != '\0'))
         {
            fprintf(stderr, _("Error: action needs parameter [%s]!\n"),
                    param_string);
            return(NULL);
         }
      break;
      case fat_write:
         return(NULL);
      break;
      case fat_none:
         if (spos && (*spos != '\0'))
         {
            fprintf(stderr, _("Error: trailing characters after action specification \"%s\"!\n"),
                    param_string);
            return(NULL);
         }
      break;
      default:
         assert("Impossible error" && false);
         return(NULL);
   }

/*   result = find_action(actions, &action);
/   if (!result)
   {

      if (!add_filter_action(filter->actions, result))
      {
         dfree(result);
         result = NULL;
      }
   }
*/   
   result = dmalloc(sizeof(struct t_filter_action));
   if (!result) return(NULL);

   result->type      = act_type;
   result->params    = spos;

   return(result);
}
//-------------------------------------------------------------------------------
__inline t_msg_out_fmt get_out_format(const char *fmt_str)
{
      if (!fmt_str) return(mof_unknown);

      if (strceq(fmt_str, "csv")) return(mof_csv);
      if (strceq(fmt_str, "prn")) return(mof_prn);
      if (strceq(fmt_str, "raw")) return(mof_raw);

//      fprintf(stderr, "Error: unknown output format '%s'!\n", fmt_str);
      return(mof_unknown);
}
//-------------------------------------------------------------------------------
static
bool filter_iterator(struct t_key_value_pair *kvp, void *filters_db)
{
   t_cl_error              err;
   struct t_filters_base   *fdb     = (struct t_filters_base*)filters_db;
   struct t_filter         *filter; 
   struct t_io_point       *iop;
   struct t_filter_action  *action;


   assert(kvp != NULL);
   assert(fdb != NULL);

   filter = (struct t_filter*)cl_pick_top(fdb->filters, &err);
   if (err != clec_noerr)
   {
      fprintf(stderr, _("Error: pick filter from base!\n"));
      return(false);
   }

   assert(filter != NULL);

   if (strceq(kvp->key, "msg_type"))
   {
      filter->msg_type = get_num(kvp->value);
   }
   else if (strceq(kvp->key, "enabled"))
   {
      filter->enabled = (bool)atoi(kvp->value);
   }
   else if (strceq(kvp->key, "expr"))
   {
      filter->expr = kvp->value;
   }
   else if (strceq(kvp->key, "action"))
   {
      if ((action = parse_action_params(kvp->value)) == NULL) return(false);
      if (!filter->actions)
      {
         if ((filter->actions = cl_create_defd(action_destructor)) == NULL)
         {
            fprintf(stderr, _("Error: can't create actions list!\n"));
            return(false);
         }
      }
      if (!add_filter_action(filter->actions, action)) return(false);
   }
   else if (strceq(kvp->key, "from"))
   {
      if ((iop = parse_io_point_params(fdb, true, kvp->value)) == NULL) return(false);

      if (!filter->sources)
      {
         if ((filter->sources = cl_create_def()) == NULL)
         {
            fprintf(stderr, "Error: can't create sources list!\n");
            return(false);
         }
      }
      if (!add_io_point(filter->sources, iop)) return(false);
   }
   else if (strceq(kvp->key, "to"))
   {
      if ((iop = parse_io_point_params(fdb, false, kvp->value)) == NULL) return(false);
      // Create destinations list only, when the first destination is appeared.
      if (!filter->dests)
      {
         if ((filter->dests = cl_create_def()) == NULL)
         {
            fprintf(stderr, _("Can't create destinations list!\n"));
            return(false);
         }
      }
      if (!add_io_point(filter->dests, iop)) return(false);
   }
   else if (strceq(kvp->key, "out_format"))
   {
      if ((filter->out_fmt = get_out_format(kvp->value)) == mof_unknown)
      {
         fprintf(stderr, _("Error: unknown output format \"%s\"!\n"), kvp->value);
         return(false);
      }
   }
   else if (strceq(kvp->key, "descr"))
   {
      filter->descr = kvp->value;
   }
   else if (strceq(kvp->key, "only_headers"))
   {
      filter->f_only_header = (bool)atoi(kvp->value);
   }
   else
   {
      fprintf(stderr, _("Error: unknown filter parameter \"%s\"!\n"), kvp->key);
      return(false);
   }
   return(true);
}
//------------------------------------------------------------------------------
static
bool filterdb_iterator(struct t_key_value_block *kvb, void *filters_db)
{
   struct t_filters_base   *fdb = (struct t_filters_base*)filters_db;
//   struct t_filter         *filter;

   assert(kvb != NULL);
   assert(fdb != NULL);

   if (!(kvb && strceq(kvb->block_name, "filter"))) return(true);

   if (add_filter(fdb) == NULL) return(false);
   if (!db_iterate_block(kvb, filter_iterator, filters_db))
   {
      // Will be freed in destroy_filters_db().
//      dfree(filter);
      return(false);
   }

   return(true);
}
//------------------------------------------------------------------------------
struct t_base  *load_filters_db(const char            *filename,
                                struct t_filters_base **filters_base,
                                char                  *def_in,
                                char                  *def_out,
                                t_msg_out_fmt         def_out_fmt)
{
   struct t_base *filters = db_load_base(filename);

   if (!(filters && filters_base)) return(NULL);
   if ((*filters_base = init_filters_db(filters, def_in,
                                        def_out, def_out_fmt)) == NULL)
   {
      // Called in init_filters_db() via destroy_filters_db().
//      db_destroy(filters);
      return(NULL);
   }
   return(filters);
}
//------------------------------------------------------------------------------
static
struct t_io_point *init_def_iop(const struct t_io_point *src_iop)
// Allocate default IO point.
// Memory must be freed somewhere.
{
   struct t_io_point *iop = dmalloc(sizeof(struct t_io_point));

   if (!iop) return(NULL);
   if (src_iop) memcpy(iop, src_iop, sizeof(struct t_io_point));
   else iop->type = tiop_unknown;

   return(iop);
}
//------------------------------------------------------------------------------
struct t_filters_base *init_filters_db(struct t_base     *filters,
                                       char              *def_in,
                                       char              *def_out,
                                       t_msg_out_fmt     def_out_fmt)
{
   struct t_filters_base *fl_base;

   assert(filters != NULL);

   if ((fl_base =
        (struct t_filters_base*)dmalloc(sizeof(struct t_filters_base))) == NULL)
      return(NULL);

   memzero(fl_base, sizeof(struct t_filters_base));

   fl_base->sources     = cl_create_defd(io_point_destructor);
   fl_base->dests       = cl_create_defd(io_point_destructor);
   fl_base->filters     = cl_create_defd(filter_destructor);
   fl_base->def_out_fmt = def_out_fmt;

   if (!(fl_base->sources && fl_base->dests && fl_base->filters && def_in && def_out))
   {
      destroy_filters_db(filters, fl_base);
      return(NULL);
   }

   if ((fl_base->def_in = parse_io_point_params(fl_base, true, def_in)) == NULL)
   {
      destroy_filters_db(filters, fl_base);
      return(NULL);
   }
   if ((fl_base->def_out = parse_io_point_params(fl_base, false, def_out)) == NULL)
   {
      destroy_filters_db(filters, fl_base);
      return(NULL);
   }
   if (!db_iterate_base(filters, filterdb_iterator, fl_base))
   {
      destroy_filters_db(filters, fl_base);
      return(NULL);
   }
   return(fl_base);
}
//------------------------------------------------------------------------------
bool destroy_filters_db(struct t_base *filters,
                        struct t_filters_base *filters_base)
{
   t_cl_error        err;
   struct t_filter*  filter;

   if (!(filters && filters_base)) return(false);

   cl_destroy(filters_base->filters);
   cl_destroy(filters_base->sources);
   cl_destroy(filters_base->dests);
   dfree(filters_base);
   db_destroy(filters);

   return(true);
}
//------------------------------------------------------------------------------
bool get_filters_by_msgtype(struct t_filters_base *fdb,
                            const t_message_type msg_type,
                            bool (*filter_iter)(struct t_filter *filter, void *user),
                            void *user_data)
{
   struct t_filter   *filter;
   int               f_count = 0;
   t_cl_error        err;

   assert(fdb != NULL);

   filter = cl_cur_to_first(fdb->filters, &err);
   while (filter && (err == clec_noerr))
   {
      if (filter->msg_type == msg_type)
      {
         f_count++;
         if (!filter_iter(filter, user_data)) break;
      }
      filter = cl_next(fdb->filters, &err);
   }
   
   return((err == clec_noerr) || (err == clec_last_item) && f_count);
}
//------------------------------------------------------------------------------
void dump_io_point(struct t_io_point *iop)
{
   char addr[32];

   assert(iop != NULL);

   printf("IOP [%p]: ", iop);
   switch (iop->type)
   {
      case tiop_file:
         printf("file '%s' ", iop->file.filename);
      break;
      case tiop_stream:
         printf("stream ");
         switch (iop->stream.type)
         {
            case tios_stdin:
               printf("STDIN ");
            break;
            case tios_stdout:
               printf("STDOUT ");
            break;
            case tios_stderr:
               printf("STDERR ");
            break;
            case tios_unknown:
               printf("UNKNOWN ");
            break;
            default:
               printf("TYPE_ERR! ");
         }
      break;
      case tiop_net:
         printf("network %s:%d ",
                inet_ntop(iop->net.m_addr.sin_family, &(iop->net.m_addr.sin_addr),
                          addr, 32),
                ntohs(iop->net.m_addr.sin_port));
      break;
      default:
         printf("with unknown type (%d) ", iop->type);
   }
   printf("[fs = %p, fd: %d]\n", iop->fs, (iop->fs) ? fileno(iop->fs) : -1);
}
//------------------------------------------------------------------------------
void dump_io_points_list(struct t_common_list *list)
{
   struct t_io_point *iop;
   t_cl_error        err;

   iop = cl_cur_to_first(list, &err);
   if (!iop) return;
   while (iop && (err == clec_noerr))
   {
      dump_io_point(iop);
      iop = cl_next(list, &err);
   }
}
//------------------------------------------------------------------------------
bool dump_filters_db(struct t_filters_base *fdb)
// Load message in raw_buffer.
{
   struct t_filter      *flt;
   t_cl_error           err;

   if (!fdb) return(false);

   printf("Sources:\n");
   dump_io_points_list(fdb->sources);
   printf("DEF SRC:\n");
   dump_io_point(fdb->def_in);
   printf("Destinations:\n");
   dump_io_points_list(fdb->dests);
   printf("DEF DST:\n");
   dump_io_point(fdb->def_out);
   printf("Filters:\n");
   flt = cl_cur_to_first(fdb->filters, &err);
   printf("----------\n");
   while (flt && (err == clec_noerr))
   {
      printf("Filter [%p]\n", flt);
      printf("Msg type:%*c0x%04X\nOut format: %*c", 8, ' ', flt->msg_type, 5, ' ');
      switch(flt->out_fmt)
      {
         case mof_prn:
            printf("mof_prn\n");
         break;
         case mof_raw:
            printf("mof_raw\n");
         break;
         case mof_csv:
            printf("mof_csv\n");
         break;
         default:
            printf("unknown [%d]\n", flt->out_fmt);
      }
      printf("Expression:%*c%s\nOnly headers:%*c%s\n",
             6, ' ', flt->expr, 4, ' ', (flt->f_only_header) ? "true" : "false");
      printf("Filter sources:\n");
      dump_io_points_list(flt->sources);
      printf("Filter destinations:\n");
      dump_io_points_list(flt->dests);
      printf("----------\n");
      flt = cl_next(fdb->filters, &err);
   }

   return((err == clec_noerr) || (err == clec_last_item));
}

