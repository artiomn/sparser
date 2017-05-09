//------------------------------------------------------------------------------
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "msgloader.h"
#include "raw_message.h"
#include "crc.h"
//------------------------------------------------------------------------------
int load_msg_from_file(struct t_message_raw *msg, FILE* fs)
{
   int      bc             = 0;
   uint16   msg_len        = 0;
   uint16   msg_data_len   = 0;
   uint8    *msg_buffer;

   if ((bc = fread(&msg_len, sizeof(msg_len), 1, fs)) < 1)
   {
      fprintf(stderr, "Message length reading error [%s]!\n", strerror(errno));
      return(-1);
   }

   if ((msg_buffer = dmalloc(msg_len)) == NULL)
   {
      fprintf(stderr, "Memory allocation for the message buffer error [%s]!\n", strerror(errno));
      return(-2);
   }

   bzero(msg_buffer, msg_len);

   // Read message.
   memcpy(msg_buffer, &msg_len, sizeof(msg_len));
   if ((bc = fread(msg_buffer + sizeof(msg_len), msg_len - sizeof(msg_len), 1, fs)) < 1)
   {
      fprintf(stderr, "Message reading error [%s]!\n", strerror(errno));
      dfree(msg_buffer);
      return(-3);
   }

   memcpy(msg->header, msg_buffer, message_header_size);
   msg_data_len = message_data_len(msg);

   if (msg_data_len > 0)
   {
      if ((msg->data = dmalloc(msg_data_len)) == NULL)
      {
         fprintf(stderr, "Memory allocation for the message data error [%s]!\n", strerror(errno));
         dfree(msg_buffer);
         return(-4);
      }
      memcpy(msg->data, msg_buffer + message_header_size, msg_data_len);
   }

   //   msg->crc = *((unsigned short int*)msg_buffer + msg_len - message_crc_size);

   memcpy(&msg->crc, msg_buffer + msg_len - message_crc_size, message_crc_size);
   dfree(msg_buffer);
   return(0);
}
//------------------------------------------------------------------------------
int load_msg_from_net(struct t_message_raw *msg, const int sock)
{
   int      bc             = 0;
   uint16   msg_len        = 0;
   uint16   msg_data_len   = 0;
   uint8    *msg_buffer;

   // Get data size.
   if ((bc = recvfrom(sock, &msg_len, sizeof(msg_len),
      MSG_NOSIGNAL | MSG_WAITALL | MSG_PEEK, NULL, NULL)) < 0)
   {
      fprintf(stderr, "Message length recieving error [%s]!\n", strerror(errno));
      return(-1);
   }

   if ((msg_buffer = dmalloc(msg_len)) == NULL)
   {
      fprintf(stderr, "Memory allocation for the message buffer error [%s]!\n", strerror(errno));
      return(-2);
   }

   bzero(msg_buffer, msg_len);

   // Read message.
   if ((bc = recvfrom(sock, msg_buffer, msg_len,
      MSG_NOSIGNAL | MSG_WAITALL, NULL, NULL)) < msg_len)
   {
      fprintf(stderr, "Packet recieving error [%s]!\n", strerror(errno));
      dfree(msg_buffer);
      return(-3);
   }

   memcpy(msg->header, msg_buffer, message_header_size);
   msg_data_len = message_data_len(msg);

   if (msg_data_len > 0)
   {
      if ((msg->data = dmalloc(msg_data_len)) == NULL)
      {
         fprintf(stderr, "Memory allocation for the message data error [%s]!\n", strerror(errno));
         dfree(msg_buffer);
         return(-4);
      }
      memcpy(msg->data, msg_buffer + message_header_size, msg_data_len);
   }

//   msg->crc = *((unsigned short int*)msg_buffer + msg_len - message_crc_size);
   memcpy(&msg->crc, msg_buffer + msg_len - message_crc_size, message_crc_size);

   dfree(msg_buffer);
   return(0);
}
//------------------------------------------------------------------------------

