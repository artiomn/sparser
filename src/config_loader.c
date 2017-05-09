//------------------------------------------------------------------------------
#include <string.h>
#include <libgen.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "config_loader.h"
#include "iniparser.h"
#include "utils.h"
//------------------------------------------------------------------------------
static
bool set_file(char **result_path, char *conf_dir, char *value, char *key_name)
{
   // 2 - PATH_DELIM length + '\0' length.
   register size_t slen = strlen(conf_dir) + strlen(value) + 2;

   assert(result_path   != NULL);
   assert(key_name      != NULL);

   if (*result_path)
   {
      fprintf(stderr, _("Warning: \"%s\" is duplicated (used first value)!\n"), key_name);
      return(true);
   }

//   if ((*result_path = strdup(value)) == NULL) return(false);
   if (value && (value[0] == PATH_DELIM[0]))
   {
      if ((*result_path = strdup(value)) == NULL) return(false);
      return(true);
   }
   if ((*result_path = malloc(slen)) == NULL) return(false);
   if (snprintf(*result_path, slen, "%s%s%s",
               conf_dir, PATH_DELIM, value) < 0) return(false);
   return(true);
}
//------------------------------------------------------------------------------
static
bool config_iterator(struct t_key_value_pair *kvp, void *cnf)
{
   struct t_config *conf = (struct t_config*)cnf;

   assert(kvp  != NULL);
   assert(conf != NULL);

   if (strceq(kvp->key, "header_file"))
   {
      if (!set_file(&conf->header_file, conf->config_dir, kvp->value, kvp->key))
         return(false);
   }
   else if (strceq(kvp->key, "messages_file"))
   {
      if (!set_file(&conf->messages_file, conf->config_dir, kvp->value, kvp->key))
         return(false);
   }
   else if (strceq(kvp->key, "stations_file"))
   {
      if (!set_file(&conf->stations_file, conf->config_dir, kvp->value, kvp->key))
         return(false);
   }
   else if (strceq(kvp->key, "filters_file"))
   {
      if (!set_file(&conf->filters_file, conf->config_dir, kvp->value, kvp->key))
         return(false);
   }
   else if (strceq(kvp->key, "from"))
   {
      if (!conf->src) conf->src = strdup(kvp->value);
   }
   else if (strceq(kvp->key, "to"))
   {
      if (!conf->dst) conf->dst = strdup(kvp->value);
   }
   else if (strceq(kvp->key, "out_format"))
   {
      if (conf->out_format == mof_unknown)
      {
         if ((conf->out_format = get_out_format(kvp->value)) == mof_unknown)
         {
            fprintf(stderr, _("Warning: unknown output format '%s' (default format will be used).\n"), kvp->value);
            conf->out_format = get_out_format(DEF_MSG_OUTTYPE);
         }
      }
   }
   else
   {
      fprintf(stderr, _("Error: unknown configuration parameter \"%s\"!\n"), kvp->key);
      return(false);
   }
   return(true);
}
//------------------------------------------------------------------------------
static
bool configdb_iterator(struct t_key_value_block *kvb, void *cnf)
{
   if (!(kvb && strceq(kvb->block_name, "config"))) return(true);
   return(db_iterate_block(kvb, config_iterator, cnf));
}
//------------------------------------------------------------------------------
bool load_config(struct t_config *config,
                 char            *config_file)
{
   struct t_base *cnf_db;
   char          *dn;

   if (!config) return(false);

   cnf_db = db_load_base(config_file);

   if (!cnf_db) return(false);

   if ((dn = strdup(config_file)) == NULL)
   {
      db_destroy(cnf_db);
      return(false);
   }

   config->config_dir = (char*)strdup(dirname(dn));
   dfree(dn);
   if (!config->config_dir)
   {
      db_destroy(cnf_db);
      return(false);
   }

   if (!db_iterate_base(cnf_db, configdb_iterator, config))
   {
      db_destroy(cnf_db);
      return(false);
   }

   if (!config->header_file)     config->header_file     = strdup(MSGHEADER_FILE);
   if (!config->messages_file)   config->messages_file   = strdup(MESSAGES_FILE);
   if (!config->stations_file)   config->stations_file   = strdup(STATIONS_FILE);
   if (!config->filters_file)    config->filters_file    = strdup(FILTERS_FILE);
   if (!config->src)             config->src             = strdup(DEF_IN);
   if (!config->dst)             config->dst             = strdup(DEF_OUT);

   if (config->out_format == mof_unknown)
      config->out_format = get_out_format(DEF_MSG_OUTTYPE);

   assert(config->out_format != mof_unknown);

   db_destroy(cnf_db);
   return(true);
}
//------------------------------------------------------------------------------
bool destroy_config(struct t_config *config)
{
   if (!config) return(false);

   dfree(config->config_dir);
   dfree(config->header_file);
   dfree(config->messages_file);
   dfree(config->stations_file);
   dfree(config->filters_file);
   dfree(config->src);
   dfree(config->dst);
   return(true);
}

