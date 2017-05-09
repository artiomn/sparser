#include <stdio.h>
#include "../utils.h"

bool iter_handler(void *data, void *user)
{
   printf("Iter handler\n");
   printf("data = %i, user = %c\n", (int)data, (char)user);
   return(true);
}

main()
{
   struct t_common_list *cl = cl_create(5);
   t_cl_error err = false;
   int opt = 0;
   
   for (opt = 0; opt < 15; opt++)
      cl_push(cl, opt + 1);
   for (opt = 0; opt < 21; opt++)
   {printf("data: %d, icount: %d ", cl_pop(cl, &err), cl->current_items_count, err); printf("errr: %d\n", err); }
   for (opt = 0; opt < 19; opt++)
      cl_push(cl, 5);
   for (opt = 0; opt < 21; opt++)
   {
      printf("AF %d\n", cl->allocated_items_count);
      printf("%d - lc: %d ", cl_pop(cl, &err), cl->current_items_count, err);
      printf("e %d\n", err);
   }
   for (opt = 0; opt < 15; opt++)
      cl_push(cl, opt + 1);
   for (opt = 0; opt < 21; opt++)
   {
      printf("data: %d, icount: %d ", cl_pop(cl, &err), cl->current_items_count, err);
      printf("errr: %d\n", err);
   }
   printf("Pushing (cl: %p)...\n", cl);
   for (opt = 0; opt < 15; opt++)
      cl_push(cl, opt + 1);
   for (opt = 0; opt < 19; opt++)
      cl_push(cl, 5);
   printf("Iterator testing (cl: %p)...\n", cl);
   opt = cl_iterate(cl, &iter_handler, 'u',  NULL);
   printf("Iter res: %i\n", opt);
   for (opt = 0; opt < 21; opt++)
   {
      printf("AF %d\n", cl->allocated_items_count);
      printf("%d - lc: %d ", cl_pop(cl, &err), cl->current_items_count, err);
      printf("e %d\n", err);
   }
   for (opt = 0; opt < 21; opt++)
   {
      printf("data: %d, icount: %d ", cl_pop(cl, &err), cl->current_items_count, err);
      printf("errr: %d\n", err);
   }
   cl_destroy(cl);
   printf("Ok\n");
   exit(0);   
}
