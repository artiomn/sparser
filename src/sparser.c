//------------------------------------------------------------------------------
// TRAKT message parser by Artiom N.(cl)2012 
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <locale.h>
//------------------------------------------------------------------------------
#include "sparser.h"
#include "raw_message.h"
#include "msgloader.h"
#include "network.h"
//------------------------------------------------------------------------------
struct t_configuration config;
//------------------------------------------------------------------------------
void print_version(const char *p_name)
{
   printf(_("%s version: %s (build at %s)\n"), p_name, VERSION, BUILD_DATE);
   return;
}
//------------------------------------------------------------------------------
void usage(const char *p_name)
{
   printf("%s\n%s - %s\n%s\n\n", LINE_E, p_name, TITLE, LINE_E);
   print_version(p_name);
   printf(_("\nUsage:\n"
            "%s [options]\n\n"), p_name);
   printf(_("Options:\n\n"));
   printf(_("-f<parameters> \t\t- input source specification:\n"));
   printf(_("network[,address:port]\t- read data from UDP with specified address and port.\n"));
   printf(_("file,<filename> \t- read data from file.\n"));
   printf(_("stdin \t\t\t- read data from STDIN.\n"));
   printf(_("none \t\t\t- don't read data by default.\n"));
   printf(_("Default: \t\t  %s\n\n"), DEF_IN);
   printf(_("-t<parameters> \t\t- destination specification:\n"));
   printf(_("network[,address:port]\t- write data to UDP with specified address and port.\n"));
   printf(_("file,<filename> \t- write data to file.\n"));
   printf(_("stdout \t\t\t- write data to STDOUT.\n"));
   printf(_("stderr \t\t\t- write data to STDERR.\n"));
   printf(_("none \t\t\t- don't write data by default.\n"));
   printf(_("Default: \t\t  %s\n\n"), DEF_OUT);
   printf(_("\nOther options:\n"));
   printf(_("-o<out_format>\t - default output format (default: %s).\n"), DEF_MSG_OUTTYPE);
   printf(_("-m<messages> \t - messages metadata file (default: \"%s\").\n"),
          MESSAGES_FILE);
   printf(_("-r<header> \t - header metadata file (default: \"%s\").\n"),
          MSGHEADER_FILE);
   printf(_("-s<stations> \t - stations database file (default: \"%s\").\n"),
          STATIONS_FILE);
   printf(_("-c<config_file>\t - configuration file (default: \"%s\").\n"),
          DEFCONF_FILE);
   printf(_("-l<filters> \t - filters file (default \"%s\").\n"),
          FILTERS_FILE);
   printf(_("-h \t\t - print this help.\n\n"));
   printf(_("-v \t\t - print version.\n\n"));
   //printf("%s\n", LINE_E, TITLE, LINE_E);
   return;
}
//------------------------------------------------------------------------------
void exitf()
{
   printf(_("%s\nExiting...\n"), LINE_E);

   destr_network();

//   free_raw_message(config.msg);
   dfree(config.start_dir);
   dfree(config.msg);
   destroy_config(&(config.config));
   destroy_stations_db(config.pmd.stations);
   destroy_filters_db(config.filters_db, config.pmd.filters);
   destroy_messages_db(config.messages, config.pmd.md_cache);
//   destroy_header_fields_db(config.pmd.header_fields_md);
}
//------------------------------------------------------------------------------
void s_handler(int s)
{
   config.exec_status = false;
   exit(EXIT_SUCCESS);
}
//------------------------------------------------------------------------------
bool start_network(struct t_configuration *conf, const char *def_opt)
{
   if (!conf->network_ready)
   {
      printf(_("Network initialization...\n"));
      if (!init_network())
      {
         fprintf(stderr, _("Error: network initialization failed!\n"));
         return(false);
      }
      conf->network_ready = true;
      printf(_("Success.\n"));
   }
   return(true);
}
//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
   int   opt            = 0;
   char  *config_file   = NULL;
//   long  max_path       = 255;
//   char  *cnf_dir;

//   max_path = pathconf(".", _PC_PATH_MAX);

   setlocale(LC_ALL, "");
   // TODO? if ... error(EXIT_FAILURE, errno, "descr");

   if (!bindtextdomain(PACKAGE_NAME, LOCALE_DIR))
   {
      fprintf(stderr, "Error: can't bind text domain!\n");
      exit(EXIT_FAILURE);
   }
   if (!textdomain(PACKAGE_NAME))
   {
      fprintf(stderr, "Error: can't set text domain!\n");
      exit(EXIT_FAILURE);
   }
   if (!bind_textdomain_codeset(PACKAGE_NAME, DEF_CHARSET))
   {
      fprintf(stderr, "Error: can't set charset \"%s\"!\n", DEF_CHARSET);
      exit(EXIT_FAILURE);
   }

   memzero(&config, sizeof(struct t_configuration));
   config.config.out_format = mof_unknown;

   if (atexit(&exitf) != 0)
   {
      fprintf(stderr, _("Error: can't register on exit function [%s]!\n"),
              strerror(errno));
      // Not return(). exit() for compatiblity.
      //return(EXIT_FAILURE);
      exit(EXIT_FAILURE);
   }
   #ifdef SIGINT
   signal(SIGINT, &s_handler);
   #endif
   #ifdef SIGQUIT
   signal(SIGQUIT, &s_handler);
   #endif
   #ifdef SIGKILL
   signal(SIGKILL, &s_handler);
   #endif

   while ((opt = getopt(argc, argv, "f:t:l:c:m:r:s:o:hv")) != -1)
   {
      switch (opt)
      {
         case 'c':
            dfree(config_file);
            config_file = strdup(optarg);
         break;
         case 'f':
            dfree(config.config.src);
            config.config.src = strdup(optarg);
         break;
         case 't':
            dfree(config.config.dst);
            config.config.dst = strdup(optarg);
         break;
         case 'l':
            dfree(config.config.filters_file);
            config.config.filters_file = strdup(optarg);
         break;
         case 'r':
            dfree(config.config.header_file);
            config.config.header_file = strdup(optarg);
         break;
         case 'm':
            dfree(config.config.messages_file);
            config.config.messages_file = strdup(optarg);
         break;
         case 's':
            dfree(config.config.stations_file);
            config.config.stations_file = strdup(optarg);
         break;
         case 'o':
            if ((config.config.out_format = get_out_format(optarg)) == mof_unknown)
            {
               fprintf(stderr, "Error: output format \"%s\" incorrect!\n", optarg);
               exit(EXIT_FAILURE);
            }
         break;
         case 'h':
            usage(PACKAGE_NAME);
            exit(EXIT_SUCCESS);
         break;
         case 'v':
            print_version(PACKAGE_NAME);
            exit(EXIT_FAILURE);
         break;
         default:
            usage(PACKAGE_NAME);
            exit(EXIT_FAILURE);
         break;
      } // switch.
   } // while getoopt.

/*   if ((config.start_dir = dmalloc(max_path)) == NULL)
   {
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   if (!getcwd(config.start_dir, max_path))
   {
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }*/

   printf("%s\n%s\n%s\n\n", LINE_E, _(TITLE), LINE_E);
   printf(_("Initializing...\n"));

   // Russian railways is used Moscow time...
   printf(_("Setting timezone to \"%s\"...\n"), DEF_TZ);
   if (setenv("TZ", DEF_TZ, 1) < 0)
   {
      fprintf(stderr, _("Warning: timezone setting failed! Time may be incorrect!\n"));
   }
   else tzset();

   if (!config_file) config_file = strdup(DEFCONF_FILE);

/*   if (chdir(CONF_DIR) < 0)
   {
      dfree(config_file);
      fprintf(stderr, "Error: can't change directory to \"%s\" [%s]!\n", CONF_DIR, strerror(errno));
      exit(EXIT_FAILURE);
   }*/

   if (!load_config(&(config.config), config_file))
   {
      fprintf(stderr, _("Error: can't load config \"%s\"!\n"), config_file);
      dfree(config_file);
      exit(EXIT_FAILURE);
   }
   dfree(config_file);
   config.pmd.def_out_fmt = config.config.out_format;
   printf(_("Loading messages data from \"%s\"...\n"), config.config.messages_file);
   if ((config.messages =
        init_messages_db(config.config.messages_file, &config.pmd.md_cache)) == NULL)
   {
      fprintf(stderr, _("Error: can't loading messages data!\n"));
      exit(EXIT_FAILURE);
   }

   printf(_("Success: found %d message types.\n"), config.messages->items_cnt);
//   db_print(messages);
//   print_msg_tree(0x0504);

   printf(_("Loading stations data from \"%s\"...\n"), config.config.stations_file);
   if ((config.pmd.stations = init_stations_db(config.config.stations_file)) == NULL)
      fprintf(stderr, _("Warning: can't load stations data!\n"));
   else printf(_("Success: found %d stations.\n"), cl_items_count(config.pmd.stations));

   printf(_("Loading header metadata from \"%s\"...\n"), config.config.header_file);
   if ((config.pmd.header_fields_md = init_header_fields_db(config.config.header_file)) == NULL)
   {
      fprintf(stderr, _("Error: can't load header fields metadata!\n"));
      exit(EXIT_FAILURE);
   }
   printf(_("Success.\n"));

   if (!start_network(&config, NULL)) exit(EXIT_FAILURE);

   printf(_("Loading filters from \"%s\"...\n"), config.config.filters_file);
   config.filters_db = load_filters_db(config.config.filters_file, &(config.pmd.filters), //NULL, NULL);
                                       config.config.src, config.config.dst, config.pmd.def_out_fmt);
   if (!config.filters_db)
   {
      fprintf(stderr, _("Error: can't load filters!\n"));
      exit(EXIT_FAILURE);
   }
   printf(_("Success: found %d filters\n"), cl_items_count(config.pmd.filters->filters));
//   dump_filters_db(config.filters);
//   exit(0);
/*   if (chdir(config.start_dir) < 0)
   {
      fprintf(stderr, "Error!\n");
      exit(EXIT_FAILURE);
   }*/
   config.exec_status = true;
   printf(_("Decoding started...\n\n"));
   while(config.exec_status)
   {
      if (!process_messages(&config.msg, &config.pmd))
      {
         fprintf(stderr, _("Error: message processing error!\n"));
         break;
      }
      if (!config.exec_status) break;

      dfree(config.msg);
      config.msg = NULL;
   }

   exit(EXIT_SUCCESS);
}
//------------------------------------------------------------------------------
