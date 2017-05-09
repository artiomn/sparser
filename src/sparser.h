//------------------------------------------------------------------------------
// Main module header file.
//------------------------------------------------------------------------------

#ifndef _sparser_h
#define _sparser_h
//------------------------------------------------------------------------------
#include "common.h"
#include "msgparser.h"
#include "stations.h"
#include "utils.h"
#include "iniparser.h"
#include "filters.h"
#include "config_loader.h"
//------------------------------------------------------------------------------
#include "lexan.h"
//------------------------------------------------------------------------------
#define BUILD_DATE "08.08.13 16:06:57"
//------------------------------------------------------------------------------
struct t_configuration
{
   struct t_config            config;
   struct t_parser_metadata   pmd;
   struct t_base              *messages;
   struct t_base              *filters_db;

   struct t_message_raw       *msg;
   char                       *start_dir;
   bool                       network_ready;
   bool                       exec_status;

};
//------------------------------------------------------------------------------
extern struct t_configuration config;
//------------------------------------------------------------------------------
#endif

