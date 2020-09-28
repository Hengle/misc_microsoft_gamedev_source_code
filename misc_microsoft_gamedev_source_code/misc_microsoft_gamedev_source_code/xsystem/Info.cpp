#include "xsystem.h"
#pragma warning(disable: 4995)
#ifndef XBOX
   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif
//#pragma warning(default: 4995)
#include "info.h"

BInfo::BInfo() :
   m_lCount(-1)
{
}

HRESULT BInfo::initialize(void)
{
   //FIXME - May need replacement for Xbox
#ifdef XBOX
   return S_OK;
#else
   // open a socket descriptor
   SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
   unsigned long ulBytesReturned = 0;

   if (sd == SOCKET_ERROR)
      goto failure;

   // make the ioctl call for the interface list
   if (WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &m_InterfaceList, 
      sizeof(m_InterfaceList), &ulBytesReturned, 0, 0) == SOCKET_ERROR)
   {
      goto failure;
   }
   
   closesocket(sd);

   m_lCount = ulBytesReturned / sizeof(INTERFACE_INFO);

   return S_OK;

failure:
   if (sd != SOCKET_ERROR)
      closesocket(sd);

   int err = WSAGetLastError();
   return HRESULT_FROM_WIN32(err);
#endif
}

long BInfo::getInterfaceCount(void)
{
#ifdef XBOX
   return 0;
#else
   return m_lCount;
#endif
}

DWORD BInfo::getInterfaceAddress(long lIfIndex)
{
#ifdef XBOX
   return 0;
#else
   sockaddr_in* pAddress = 0;
   pAddress = (sockaddr_in *) &(m_InterfaceList[lIfIndex].iiAddress);
   return ntohl(pAddress->sin_addr.s_addr);
#endif
}

DWORD BInfo::getInterfaceBroadcastAddress(long lIfIndex)
{
#ifdef XBOX
   return 0;
#else
   sockaddr_in* pAddress = 0;
   pAddress = (sockaddr_in *) &(m_InterfaceList[lIfIndex].iiBroadcastAddress);
   return ntohl(pAddress->sin_addr.s_addr);
#endif
}

DWORD BInfo::getInterfaceNetmask(long lIfIndex)
{
#ifdef XBOX
   return 0;
#else
   sockaddr_in* pAddress = 0;
   pAddress = (sockaddr_in *) &(m_InterfaceList[lIfIndex].iiNetmask);
   return ntohl(pAddress->sin_addr.s_addr);
#endif
}

bool BInfo::isUp(long lIfIndex)
{
#ifdef XBOX
   return false;
#else
   return (m_InterfaceList[lIfIndex].iiFlags & IFF_UP) ? true : false;
#endif
}

bool BInfo::isBroadcastSupported(long lIfIndex)
{
#ifdef XBOX
   return false;
#else
   return (m_InterfaceList[lIfIndex].iiFlags & IFF_BROADCAST) ? true : false;
#endif
}

bool BInfo::isLoopback(long lIfIndex)
{
#ifdef XBOX
   return false;
#else
   return (m_InterfaceList[lIfIndex].iiFlags & IFF_LOOPBACK) ? true : false;
#endif
}

bool BInfo::isPPP(long lIfIndex)
{
#ifdef XBOX
   return false;
#else
   return (m_InterfaceList[lIfIndex].iiFlags & IFF_POINTTOPOINT) ? true : false;
#endif
}
