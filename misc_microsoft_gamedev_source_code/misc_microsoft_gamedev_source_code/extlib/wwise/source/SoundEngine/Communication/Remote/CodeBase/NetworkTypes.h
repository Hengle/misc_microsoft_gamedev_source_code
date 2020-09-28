/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/


#pragma once

#ifdef WIN32
typedef sockaddr_in SocketAddrType;
#endif

#ifdef XBOX360
typedef sockaddr_in SocketAddrType;
#endif

#ifdef __PPU__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

typedef sockaddr_in SocketAddrType;

#define SOCKET int
#define INVALID_SOCKET 0xffffffff
#define SOCKET_ERROR -1

#define SD_BOTH SHUT_RDWR
#define SD_RECEIVE SHUT_RD
#define SD_SEND SHUT_WR

/*
 * Windows Sockets definitions of regular Berkeley error constants
 */
#define WSAEWOULDBLOCK          (35)
#define WSAEINPROGRESS          (36)
#define WSAEALREADY             (37)
#define WSAENOTSOCK             (38)
#define WSAEDESTADDRREQ         (39)
#define WSAEMSGSIZE             (40)
#define WSAEPROTOTYPE           (41)
#define WSAENOPROTOOPT          (42)
#define WSAEPROTONOSUPPORT      (43)
#define WSAESOCKTNOSUPPORT      (44)
#define WSAEOPNOTSUPP           (45)
#define WSAEPFNOSUPPORT         (46)
#define WSAEAFNOSUPPORT         (47)
#define WSAEADDRINUSE           (48)
#define WSAEADDRNOTAVAIL        (49)
#define WSAENETDOWN             (50)
#define WSAENETUNREACH          (51)
#define WSAENETRESET            (52)
#define WSAECONNABORTED         (53)
#define WSAECONNRESET           (54)
#define WSAENOBUFS              (55)
#define WSAEISCONN              (56)
#define WSAENOTCONN             (57)
#define WSAESHUTDOWN            (58)
#define WSAETOOMANYREFS         (59)
#define WSAETIMEDOUT            (60)
#define WSAECONNREFUSED         (61)
#define WSAELOOP                (62)
#define WSAENAMETOOLONG         (63)
#define WSAEHOSTDOWN            (64)
#define WSAEHOSTUNREACH         (65)
#define WSAENOTEMPTY            (66)
#define WSAEPROCLIM             (67)
#define WSAEUSERS               (68)
#define WSAEDQUOT               (69)
#define WSAESTALE               (70)
#define WSAEREMOTE              (71)


typedef int BOOL;
#define FALSE 0
#define TRUE 1

#endif // __PPU__

#ifdef RVL_OS

#include <revolution/os.h>
#include <revolution/soex.h>

typedef SOSockAddrIn SocketAddrType;

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define INADDR_ANY SO_INADDR_ANY

// Socket creation
#define AF_INET SO_PF_INET
#define SOCK_DGRAM SO_SOCK_DGRAM
#define SOCK_STREAM SO_SOCK_STREAM
#define IPPROTO_TCP 0
#define IPPROTO_UDP 0

// Socket options
#define SOL_SOCKET SO_SOL_SOCKET
#define SO_REUSEADDR SO_SO_REUSEADDR

// Socket shutdown
#define SD_BOTH SO_SHUT_RDWR
#define SD_RECEIVE SO_SHUT_RD
#define SD_SEND SO_SHUT_WR

#endif // RVL_OS
