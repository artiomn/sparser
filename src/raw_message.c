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
   assert(msg != NULL);
   return(msg->header.length - (message_header_size + message_crc_size));
}
//------------------------------------------------------------------------------
__inline uint16 message_crc(const struct t_message_raw *msg)
{
   assert(msg != NULL);
   return(*((uint16*)((uint8*)msg + msg->header.length - message_crc_size)));
}
//------------------------------------------------------------------------------
