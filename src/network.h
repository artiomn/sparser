//------------------------------------------------------------------------------
// Networking.
//------------------------------------------------------------------------------

#ifndef _network_h
#define _network_h
//------------------------------------------------------------------------------
#ifdef _WIN32
   #include <winsock2.h>
#else
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
#endif
//------------------------------------------------------------------------------
#include "common.h"
//------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//------------------------------------------------------------------------------
bool init_network();
FILE *fopen_def_sock(char *subopt_str, struct sockaddr_in *m_addr);
__inline bool destr_network();
int open_dgram_socket(struct sockaddr_in *sa, const bool for_read);
__inline FILE *fopen_dgram_socket(struct sockaddr_in *sa, const char *mode);
// Crossplatform closesocket().
__inline int cp_sockclose(int sock);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
#endif
