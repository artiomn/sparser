//------------------------------------------------------------------------------
// TRAKT messages parser configuration.
//------------------------------------------------------------------------------

#ifndef _conf_h
#define _conf_h
//------------------------------------------------------------------------------
#include "config.h"
//------------------------------------------------------------------------------
#ifdef _WIN32
   #define DEF_CHARSET "CP866"
#else
   #define DEF_CHARSET "UTF-8"
#endif
#define DEF_TZ "Europe/Moscow"
//------------------------------------------------------------------------------
#define DEF_IN "network,0.0.0.0:7001"
#define DEF_OUT "stdout"
//------------------------------------------------------------------------------
#define TITLE "Artiom N. messages decoder for 'TRAKT' dispatcher centralization"
#define LINE_E "=========================================================================="
#define LINE_D "--------------------------------------------------------------------------"
//------------------------------------------------------------------------------
#define CONF_DIR        "conf"
#if (FILE_SYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR != 0) && !defined(PATH_DELIM)
   #define PATH_DELIM "\\"
#elif !defined(PATH_DELIM)
   #define PATH_DELIM "/"
#endif
#define DEFCONF_FILE    CONF_DIR PATH_DELIM "config.ini"
#define FILTERS_FILE    CONF_DIR PATH_DELIM "filters.ini"
#define MESSAGES_FILE   CONF_DIR PATH_DELIM "messages.ini"
#define STATIONS_FILE   CONF_DIR PATH_DELIM "stations.ini"
#define MSGHEADER_FILE  CONF_DIR PATH_DELIM "header.ini"
//------------------------------------------------------------------------------
#define DEF_MSG_OUTTYPE "prn"
//------------------------------------------------------------------------------
//#define DEBUG_LEVEL 2
//------------------------------------------------------------------------------
#endif
