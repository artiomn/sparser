//------------------------------------------------------------------------------
#include <stdio.h>
#include "../conf.h"
#include "../iniparser.h"
//------------------------------------------------------------------------------
#define SF "../"##STATIONS_FILE
//------------------------------------------------------------------------------
int main()
{
   struct t_base *db = db_load_base(SF);

   if (db) db_print(db);
   printf("243km == %s\n", db_get_value_pair(db_get_block_by_value(db, NULL, "address", "1620"),
      "name")->value);
   printf("station == %s\n", db_get_block_by_value(db, NULL, "address", "1620")->block_name);
   // Incorrect.
   printf("NULL == %p\n", db_get_block_by_value(db, NULL, "section", "1620"));
//   ASSERT(1);
   db_destroy(db);
   exit(0);
}
//------------------------------------------------------------------------------

