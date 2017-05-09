//------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "raw_message.h"
#include "utils.h"
#include "crc.h"
//------------------------------------------------------------------------------
__inline int message_data_len(const struct t_message_raw *msg)
{
   return(msg->header.message_length - (message_header_size + message_crc_size));
}
//------------------------------------------------------------------------------
__inline uint16 message_crc(const struct t_message_raw *msg)
{
   return(*((uint16*)((uint8*)msg + msg->header.message_length - message_crc_size)));
}
//------------------------------------------------------------------------------
/*struct t_message_raw *create_raw_message()
{
   struct t_message_raw *msg = NULL;

   if ((msg = dmalloc(message_struct_size)) == NULL)
   {
      fprintf(stderr, "Memory allocation for the message error [%s]!", strerror(errno));
      return(NULL);
   }

   bzero(msg, message_struct_size);
   if ((msg->header = dmalloc(message_header_size)) == NULL)
   {
      dfree(msg);
      fprintf(stderr, "Memory allocation for the message header error [%s]!", strerror(errno));
      return(NULL);
   }

   return(msg);
}
//------------------------------------------------------------------------------
__inline bool free_raw_message(struct t_message_raw *msg)
{
   if (msg)
   {
      dfree(msg->data);
      dfree(msg->header);
   }
   return(true);
}
//------------------------------------------------------------------------------
*/
