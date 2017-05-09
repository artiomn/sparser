//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef _WIN32
   #include <fcntl.h>
#endif
//------------------------------------------------------------------------------
#include "network.h"
#include "utils.h"
//------------------------------------------------------------------------------
__inline bool init_network()
// Initialize network subsytem.
{
   #ifdef _WIN32
   // Need min. 500 bytes, but sizeof(WSADATA) == 400 bytes.
   char lpWSAData[1024];
   // Ws2_32.dll initialization.
   if (WSAStartup(MAKEWORD(2, 2), (WSADATA*)&lpWSAData[0]) != 0)
   {
      fprintf(stderr, _("Error: WSAStartup %d\n"), WSAGetLastError());
      return(false);
   }
   #endif
   return(true);
}
//------------------------------------------------------------------------------
__inline bool destr_network()
{
   #ifdef _WIN32
   return(WSACleanup() == 0);
   #endif
   return(true);
}
//------------------------------------------------------------------------------
int open_dgram_socket(struct sockaddr_in *sa, const bool for_read)
{
   int sock;

   #ifdef _WIN32
   sock = WSASocket(PF_INET, SOCK_DGRAM, 0, NULL, 0, 0);
   #else
   sock = socket(PF_INET, SOCK_DGRAM, 0);
   #endif

   if (sock < 0)
   {
      fprintf(stderr, _("Error: socket creating [%s]!\n"), strerror(errno));
      return(-1);
   }

   if (for_read)
   {
      if (bind(sock, (struct sockaddr*)sa, sizeof(struct sockaddr)) < 0)
      {
         fprintf(stderr, _("Error: binding [%s]!\n"), strerror(errno));
         return(-2);
      }
   }
   else
   {
      if (connect(sock, (struct sockaddr*)sa, sizeof(struct sockaddr)) < 0)
      {
         fprintf(stderr, _("Error: connecting [%s]!\n"), strerror(errno));
         return(-3);
      }
   }
   return(sock);
}
//------------------------------------------------------------------------------
#if _WIN32 && !(defined(_fdopen) || defined(HAVE__FDOPEN))
   #define _fdopen fdopen
#endif
//------------------------------------------------------------------------------
__inline FILE *fopen_dgram_socket(struct sockaddr_in *sa, const char *mode)
{

   uint8 rw_mode = (strchr(mode, 'r') != NULL) ? 1 : 0;

   rw_mode = (strchr(mode, 'w') != NULL) ? rw_mode | 2 : rw_mode;
   switch (rw_mode)
   {
      case 1:
         #if defined(_WIN32) && (HAVE__OPEN_OSFHANDLE == 1)
         return(_fdopen(_open_osfhandle(open_dgram_socket(sa, true),
                                        _O_BINARY|_O_SEQUENTIAL|_O_RDONLY), mode));
         #else
         return(fdopen(open_dgram_socket(sa, true), mode));
         #endif
      break;
      case 2:
         #if defined(_WIN32) && (HAVE__OPEN_OSFHANDLE == 1)
         return(_fdopen(_open_osfhandle(open_dgram_socket(sa, false),
                                        _O_BINARY|_O_WRONLY), mode));
         #else
         return(fdopen(open_dgram_socket(sa, false), mode));
         #endif
      break;
      default:
      ;
   }
   return(NULL);
}
//------------------------------------------------------------------------------
__inline int cp_sockclose(int sock)
{
   #ifdef _WIN32
   return(closesocket(sock));
   #else
   return(close(sock));
   #endif
}
//------------------------------------------------------------------------------
