//------------------------------------------------------------------------------
// Stations information database and loader.
//------------------------------------------------------------------------------

#ifndef _stations_h
#define _stations_h
//------------------------------------------------------------------------------
#include "common.h"
#include "iniparser.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
#define MAX_ST_ADDRS 16
//------------------------------------------------------------------------------
struct t_station
{
   uint16   addr[MAX_ST_ADDRS];
   int      addr_count;
   char     *name;
};
//------------------------------------------------------------------------------
struct t_common_list *init_stations_db(const char *filename);
bool destroy_stations_db(struct t_common_list *stations);
char *get_station_name_by_addr(struct t_common_list   *stations,
                               const uint16           addr);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
