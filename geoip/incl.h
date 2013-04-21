#ifndef GEOIP_INCL_H
#define GEOIP_INCL_H

#include <stdlib.h>
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else /* !defined(_WIN32) */ 
#include <winsock2.h> 
#include <ws2tcpip.h> 
#include <windows.h> 
#include <io.h>
#define snprintf _snprintf 
#define FILETIME_TO_USEC(ft) (((unsigned __int64) ft.dwHighDateTime << 32 | ft.dwLowDateTime) / 10) 
#endif /* !defined(_WIN32) */ 

#endif
