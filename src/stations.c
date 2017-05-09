//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
//------------------------------------------------------------------------------
#include "stations.h"
#include "utils.h"
//------------------------------------------------------------------------------
static
struct t_station *add_station(struct t_common_list *sdb)
{
   assert(sdb != NULL);
   return(cl_new_item(sdb, sizeof(struct t_station)));
}
//------------------------------------------------------------------------------
static
bool station_iterator(struct t_key_value_pair *kvp, void *station_ptr)
{
   struct t_station *station = (struct t_station*)station_ptr;

   assert(station != NULL);
   assert(kvp     != NULL);

   if (strceq(kvp->key, "address"))
   {
      if (station->addr_count >= MAX_ST_ADDRS)
      {
         fprintf(stderr, _("Error: station adresses limit exceeds [station: %s, max: %d]!\n"),
                 station->name, MAX_ST_ADDRS);
         return(false);
      }
      station->addr[station->addr_count++] = get_num(kvp->value);
   }
   else if (strceq(kvp->key, "name"))
   {
      if (station->name)
      {
         fprintf(stderr, _("Warning: station name will be replaced [%s -> %s]!\n"),
                 station->name, kvp->value);
         dfree(station->name);
      }
      if ((station->name = strdup(kvp->value)) == NULL) return(false);
   }
   else
   {
      fprintf(stderr, _("Error: unknown station description field [%s]!\n"),
              kvp->key);
      return(false);
   }
   return(true);
}
//------------------------------------------------------------------------------
static
bool stationdb_iterator(struct t_key_value_block *kvb, void *stations)
{
   struct t_common_list    *sdb = (struct t_common_list*)stations;
   struct t_station        *station;

   assert(kvb  != NULL);
   assert(sdb  != NULL);

   if (!strceq(kvb->block_name, "station")) return(true);

   if ((station = add_station(sdb)) == NULL) return(false);

   if (!db_iterate_block(kvb, station_iterator, station))
   {
      return(false);
   }

   return(true);
}
//------------------------------------------------------------------------------
static void station_destructor(void *station)
{
   dfree(((struct t_station*)station)->name);
   dfree(station);
}
//------------------------------------------------------------------------------
struct t_common_list *init_stations_db(const char *filename)
{
   struct t_base        *sdb  = db_load_base(filename);
   struct t_common_list *st_list;

   if (!sdb) return(NULL);

   if ((st_list = cl_create_defd(station_destructor)) == NULL)
   {
      db_destroy(sdb);
      return(NULL);
   }

   if (!db_iterate_base(sdb, stationdb_iterator, st_list))
   {
      db_destroy(sdb);
      destroy_stations_db(st_list);
      return(NULL);
   }

   db_destroy(sdb);
   return(st_list);
}
//------------------------------------------------------------------------------
bool destroy_stations_db(struct t_common_list *stations)
{
   struct t_station  *station;
   t_cl_error        err;

   return(cl_destroy(stations));
}
//------------------------------------------------------------------------------
static bool st_ns_handler(void *st, void *addr_ptr)
{
   register int               i;
   register uint16            addr0;
   register uint16            st_addr;
   register struct t_station  *station = ((struct t_station*)st);

   assert(station    != NULL);
   assert(addr_ptr   != NULL);

   addr0 = *(uint16*)addr_ptr;
   addr0 = (addr0 & 0xf000) ? addr0 & 0xfff0 : addr0;

   for (i = 0; i < station->addr_count; i++)
   {
      st_addr = station->addr[i];
      if (st_addr & 0xf000) st_addr = st_addr & 0xfff0;
      if (st_addr == addr0) return(false);
   }


   return(true);
}
//------------------------------------------------------------------------------
char *get_station_name_by_addr(struct t_common_list   *stations,
                               const uint16           addr)
{
   struct t_station* station = cl_find(stations, st_ns_handler, &addr);

   if (!station) return(_("unknown"));
   return(station->name);
}
//------------------------------------------------------------------------------

