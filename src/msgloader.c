//------------------------------------------------------------------------------
#ifdef _WIN32
   #include <winsock2.h>
//   #include <ws2tcpip.h>
#else
   #include <sys/socket.h>
#endif
#include <string.h>
#include <errno.h>
//------------------------------------------------------------------------------
#include "msgloader.h"
#include "raw_message.h"
#include "crc.h"
//------------------------------------------------------------------------------
struct t_message_raw *load_msg_from_file(FILE* fs)
// Allocate and return buffer.
// Buffer need to be freed in the caller.
{
   int      bc             = 0;
   uint16   msg_len        = 0;
   uint8    *msg_buffer;

   assert(fs != NULL);

   if (feof(fs)) return(NULL);

   if ((bc = fread(&msg_len, sizeof(msg_len), 1, fs)) < 1)
   {
      if (feof(fs)) return(NULL);
      fprintf(stderr, _("Error: message length reading error [%s]!\n"),
              strerror(errno));
      return(NULL);
   }

   if (msg_len < sizeof(msg_len)) return(NULL);

   if ((msg_buffer = dmalloc(msg_len)) == NULL)
   {
      fprintf(stderr, _("Error: memory allocation for the message buffer error [%s]!\n"),
              strerror(errno));
      return(NULL);
   }
   memzero(msg_buffer, msg_len);

   *((uint16*)msg_buffer) = msg_len;

   // Read message body.
   if ((bc = fread(msg_buffer + sizeof(msg_len),
                   (size_t)msg_len - sizeof(msg_len), 1, fs)) < 1)
   {
      // Return error, if full message cannot to be read.
      fprintf(stderr, _("Error: message reading error [%s]!\n"),
              strerror(errno));
      dfree(msg_buffer);
      return(NULL);
   }

   return((struct t_message_raw*)msg_buffer);
}
//------------------------------------------------------------------------------
struct t_message_raw *load_msg_from_net(const int sock)
// Allocate and return buffer.
// Buffer need to be freed in the caller.
{
   int      bc             = 0;
   uint16   msg_len        = 0;
   uint8    *msg_buffer;

   #ifndef _WIN32 
      #ifndef NL_FLAGS
         #define NL_FLAGS (MSG_NOSIGNAL | MSG_WAITALL)
      #endif
   #else
      #define NL_FLAGS 0
   #endif

   assert(sock >= 0);

   // Get data size.
   if ((bc = recvfrom(sock, (char*)&msg_len, sizeof(msg_len),
      NL_FLAGS | MSG_PEEK, NULL, NULL)) < sizeof(msg_len))
   {
      fprintf(stderr, _("Error: message length recieving error [%s]!\n"),
              strerror(errno));
      return(NULL);
   }

   if ((msg_buffer = dmalloc(msg_len)) == NULL)
   {
      fprintf(stderr, _("Error: memory allocation for the message buffer error [%s]!\n"),
              strerror(errno));
      return(NULL);
   }

   memzero(msg_buffer, msg_len);

   // Read message.
   if ((bc = recvfrom(sock, msg_buffer, msg_len,
      NL_FLAGS, NULL, NULL)) < msg_len)
   {
      fprintf(stderr, _("Error: packet recieving error [%s]!\n"),
              strerror(errno));
      dfree(msg_buffer);
      return(NULL);
   }
   #undef NL_FLAGS

   return((struct t_message_raw*)msg_buffer);
}
//------------------------------------------------------------------------------

