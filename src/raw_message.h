//------------------------------------------------------------------------------
// Raw message structure and functions.
//------------------------------------------------------------------------------

#ifndef _raw_message_h
#define _raw_message_h
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
//------------------------------------------------------------------------------
#include "common.h"
#include "iniparser.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
typedef uint16 t_message_type;
#pragma pack(1)
struct t_message_header
{
   uint16            length;
   t_message_type    type;
   uint16            reciever;
   uint16            sender;
   time32            time;
   uint8             id;
   uint8             reserved;
};
//------------------------------------------------------------------------------
struct t_message_raw
{
   struct t_message_header header;
   uint8                   *data;
///   uint16                  crc;
};
//------------------------------------------------------------------------------
// Restore default alignment?
#pragma pack()
//------------------------------------------------------------------------------
#define MESSAGE_CODE(msg) (msg->header.type & 0x00ff)
#define MESSAGE_VERSION(msg) (msg->header.type >> 8)
//------------------------------------------------------------------------------
#define message_header_size   sizeof(struct t_message_header)
#define message_crc_size      sizeof(uint16)
#define message_struct_size   sizeof(struct t_message_raw)
//------------------------------------------------------------------------------
__inline uint16 message_crc(const struct t_message_raw *msg);
__inline int message_data_len(const struct t_message_raw *msg);
//------------------------------------------------------------------------------
/*struct t_message_raw *create_raw_message();
//------------------------------------------------------------------------------
__inline bool free_raw_message(struct t_message_raw *msg);*/
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
