//==============================================================================
// SocksHelper.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "SocksHelper.h"
#include "Socket.h"
#include "SocksSocketBase.h"
#include "winsockinc.h"
#include "ReliableConnection.h"
//#include "UDP360Socket.h"
#include "UDP360Connection.h"

#ifndef XBOX
   #pragma warning(disable: 4995)
   #include <ws2tcpip.h>
   #pragma warning(default: 4995)
#endif   
#include "SocksMapper.h"

#ifdef XBOX
   #define MAXGETHOSTSTRUCT (1024)
#endif

//==============================================================================
// Defines
static const CHAR SH_WINDOW_CLASS_NAME [] = "SocketHelperAsyncClass";
static const CHAR SH_WINDOW_NAME [] = "SocketHelperAsync";
static char sHostBuffer[MAXGETHOSTSTRUCT]; 

#define WM_ASYNCNAMERESOLVE WM_USER + 100

BDynamicSimArray<BSocksSocketBase*> BSocksHelper::mSockets;
BSocksHelper::BAddressNotify* BSocksHelper::mNotify = NULL;

#ifndef XBOX
HWND BSocksHelper::mWindow = NULL;
#endif

HANDLE BSocksHelper::mGetHostHandle = NULL;

//==============================================================================
// BSocksHelper::socksStartup
//==============================================================================
HRESULT BSocksHelper::socksStartup(void)
{
   if(!networkStartup())
   {
      nlog(cTransportNL, "Failed to initialize WinSock");
      return E_FAIL;
   }
   else
      return S_OK;
}

//==============================================================================
// BSocksHelper::socksCleanup
//==============================================================================
HRESULT BSocksHelper::socksCleanup(void)
{
   BSocksMapper::destroyInstance();

   networkCleanup();

   mSockets.clear();

#ifndef XBOX
   if (mWindow != NULL)
   {
      DestroyWindow (mWindow);
      mWindow = NULL;
   }
#endif
   
   mNotify = NULL;

   return S_OK;
}

//==============================================================================
// socksCatchError
//==============================================================================
long BSocksHelper::socksCatchError(int value)
{
   //
   // All WinSock error codes are valid Win32 error codes.
   // They are small-positive-integers (based at 10000), so they must be encoded in HRESULT form.
   //

   if (value == SOCKET_ERROR)   
   {
      return GetLastResult();
   }
   else
   {
      return S_OK;
   }
}

//==============================================================================
// BSocksHelper::socksGetAddress
//==============================================================================
HRESULT BSocksHelper::socksGetAddress(const char *addressIn, BAddressNotify* notify)
{
   if (mNotify != NULL || notify == NULL)
      return E_FAIL;

#ifdef XBOX
   // rg [6/21/05] - Untested!
   XNDNS* pXNDNS = NULL;
   
   int result = XNetDnsLookup(addressIn, NULL, &pXNDNS);
   
   if (0 != result)
      return E_FAIL;
   
   if (0 != pXNDNS->iStatus)
   {
      XNetDnsRelease(pXNDNS);      
      return E_FAIL;
   }
 
   notify->addressResult(static_cast<long>(pXNDNS->aina[0].S_un.S_addr));
      
   XNetDnsRelease(pXNDNS);      
#else
   mNotify = notify;

   if (mWindow == NULL)
   {
      mWindow = CreateWindowExA (
         0,             // extended styles
         //   (PCSTR) g_ResolveWindowClass.GetClassAtom(), // window class
         SH_WINDOW_CLASS_NAME,
         SH_WINDOW_NAME,
         0,             // style
         0, 0,          // position
         0, 0,          // size
         NULL,          // parent window
         NULL,          // menu handle
         DllInstance,   // DLL resource instance handle
         NULL);         // creation instance parameter

      if (mWindow == NULL)
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksSocketBase: failed to create message routing window -- socket will NOT work");
         return Result;
      }
   }

   ZeroMemory(sHostBuffer, MAXGETHOSTSTRUCT);
   mGetHostHandle = WSAAsyncGetHostByName(mWindow, WM_ASYNCNAMERESOLVE, addressIn, sHostBuffer, MAXGETHOSTSTRUCT);
   if (mGetHostHandle == 0)
   {
      mNotify = NULL;
      return E_FAIL;
   }
#endif   

   return S_OK;
}

//==============================================================================
// BSocksHelper::socksGetAddress
//==============================================================================
HRESULT BSocksHelper::socksGetAddress(long *addressOut, const char *addressIn)
{
   char addressStr[1024];

   DWORD time = timeGetTime();
   nlog(cTransportNL, "BSocksHelper::socksGetAddress -- ENTER: %s", addressIn);

   if (!addressOut)
      return E_INVALIDARG;

   if (!_strcmpi(addressIn, "127.0.0.1") || !_strcmpi(addressIn, "localhost"))
   {      
      HRESULT Result = socksGetHostName(addressStr, sizeof(addressStr));
      if (FAILED (Result))
      {
         nlog(cTransportNL, "BSocksHelper::socksGetAddress -- EXIT 1: %d", timeGetTime() - time);
         return Result;
      }
   }
   else
      // FIXME: Max address length of 1023?
      StringCchCopyNExA(addressStr, sizeof(addressStr), addressIn, 1023, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);

   struct hostent * he;
   
   long addr = inet_addr(addressStr);
   if (addr != INADDR_NONE)
   {
      *addressOut = addr;
      nlog(cTransportNL, "BSocksHelper::socksGetAddress -- EXIT 2: %d", timeGetTime() - time);
      return S_OK;
   }

   he = gethostbyname(addressStr);
   if (he == NULL)
   {
      HRESULT Result = GetLastResult();
      nlog(cTransportNL, "BSocksHelper: failed to resolve DNS name '%s' to IP address", addressStr);

      nlog(cTransportNL, "BSocksHelper::socksGetAddress -- EXIT 3: %d", timeGetTime() - time);
      return Result;
   }

   *addressOut= *(long *) he -> h_addr_list [0];

   nlog(cTransportNL, "BSocksHelper::socksGetAddress -- EXIT 4: %d", timeGetTime() - time);
   return S_OK;
}

//==============================================================================
// BSocksHelper::socksGetName
//==============================================================================
HRESULT BSocksHelper::socksGetName(long addressIn, char *addressOut, long len)
{
   BASSERT(len > 0);
   struct in_addr in;
    
   in.s_addr = addressIn;
   char *strAddr = inet_ntoa(in);
   if (strAddr == NULL)
      return E_FAIL;

   StringCchCopyNExA (addressOut, len, strAddr, len - 1, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
   addressOut[len - 1] = '\0';

   return S_OK;
}

//==============================================================================
// BSocksHelper::socksGetHostName
//==============================================================================
long BSocksHelper::socksGetHostName(char *name, long len)
{
    return socksCatchError(gethostname(name, len));
}

//==============================================================================
// BSocksHelper::getLocalIP
//==============================================================================
SOCKADDR_IN &BSocksHelper::getLocalIP(void)
{
   static SOCKADDR_IN localAddress;
   memset(&localAddress, 0, sizeof(localAddress));
   // default local address
   localAddress.sin_addr.S_un.S_addr = INADDR_ANY;
   localAddress.sin_port = 0;
   localAddress.sin_family = AF_INET;
   
#ifdef XBOX
   XNADDR xna;
   if (getXNAddr(xna))
      localAddress.sin_addr = xna.ina;
#else
   SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
   if (sd != SOCKET_ERROR) 
   {
      INTERFACE_INFO InterfaceList[20];
      unsigned long nBytesReturned;
      if (WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
         sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR) 
      {
         nlog(cTransportNL, "BSocksHelper::getLocalIP -- WSAIoctl error: %x", WSAGetLastError());
         return localAddress;
      }

      long count = nBytesReturned / sizeof(INTERFACE_INFO);
      for (long idx=0; idx<count; idx++)
      {
         // ignore down interfaces
         if ((InterfaceList[idx].iiFlags & IFF_UP) == 0)
            continue;

         // ignore loopback interfaces
         if (InterfaceList[idx].iiFlags & IFF_LOOPBACK)
            continue;

         // ignore BS addresses
         if (InterfaceList[idx].iiAddress.AddressIn.sin_addr.S_un.S_addr == 0)
            continue;

         // looks like a good one, so go with that
         localAddress.sin_addr.S_un.S_addr = InterfaceList[idx].iiAddress.AddressIn.sin_addr.S_un.S_addr;
         break;
      }
      closesocket(sd);
   }

   // set up the local address
   // let's do a routing interface query to eso.com to look it up
   //   SOCKADDR_IN routingAddress;
   //   long addressOut = INADDR_ANY;
   //   memset(&routingAddress, 0, sizeof(routingAddress));      
   //   BSocksHelper::socksGetAddress(&addressOut, "eso.com"); 
   //   routingAddress.sin_family = AF_INET;
   //   routingAddress.sin_port = ntohs(2300);
   //   routingAddress.sin_addr.S_un.S_addr = addressOut;
   // get real local address if possible
   //   BSocksHelper::getLocalIP(routingAddress, &localAddress, true);

   // return it
#endif   
   return localAddress;
}

//==============================================================================
// BSocksHelper::getLocalIP
//==============================================================================
DWORD BSocksHelper::getLocalIP(const SOCKADDR_IN &remoteRoutingAddress, SOCKADDR_IN *address, bool interfaceQuery)
{
   if (!interfaceQuery)
      return getFirstInterfaceIP(address);

#ifdef XBOX
   // rg [6/22/05] - FIXME, incomplete!
   return getFirstInterfaceIP(address);
#else
   SOCKET udpSocket = WSASocket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);

   if (udpSocket == INVALID_SOCKET)
      return getFirstInterfaceIP(address);

   const DWORD BufferLength = 0x400;
   LPBYTE Buffer = new BYTE [BufferLength];

   if (Buffer == NULL)
   {
      closesocket (udpSocket);
      return getFirstInterfaceIP(address);
   }

   DWORD BytesTransferred;
   if (WSAIoctl (udpSocket, SIO_ROUTING_INTERFACE_QUERY, (void *)&remoteRoutingAddress, sizeof(remoteRoutingAddress), Buffer, BufferLength, &BytesTransferred, NULL, NULL) == SOCKET_ERROR)
   {
      delete [] Buffer;
      closesocket (udpSocket);
      HRESULT result = WSAGetLastError();
      nlog(cTransportNL, "BSocksHelper::getLocalIP -- routing interface query error: %x", result);
      return result;
   }

   closesocket (udpSocket);

   if (BytesTransferred == 0)
   {
      nlog(cTransportNL, "BSocksHelper::getLocalIP -- 0 bytes transferred.");
      delete [] Buffer;
      return getFirstInterfaceIP(address);
   }


   SOCKADDR_IN * SocketAddress = reinterpret_cast <SOCKADDR_IN *> (Buffer);

   if (SocketAddress)
   {
      nlog(cTransportNL, "BSocksHelper::getLocalIP -- address: %hu.%hu.%hu.%hu", SocketAddress->sin_addr.S_un.S_un_b.s_b1,
                                                                    SocketAddress->sin_addr.S_un.S_un_b.s_b2,
                                                                    SocketAddress->sin_addr.S_un.S_un_b.s_b3,
                                                                    SocketAddress->sin_addr.S_un.S_un_b.s_b4 );
      DWORD tempAddress = inet_addr("127.0.0.1");
      DWORD invalidAddress = inet_addr("0.0.0.0");
      // is the address invalid (127.0.0.1)? if so, use the first interface IP
      if (
            (memcmp(&tempAddress, &SocketAddress->sin_addr.S_un.S_addr, sizeof(tempAddress)) == 0) ||
            (memcmp(&invalidAddress, &SocketAddress->sin_addr.S_un.S_addr, sizeof(invalidAddress)) == 0) ||
            (SocketAddress->sin_family != AF_INET)
         )
      {
         nlog(cTransportNL, "BSocksHelper::getLocalIP -- invalid address.");
         delete [] Buffer;
         return getFirstInterfaceIP(address);
      }

      memcpy(address, SocketAddress, sizeof(SOCKADDR_IN));
      delete [] Buffer;
      return NOERROR;
   }
   else   
   {
      delete [] Buffer;
      return getFirstInterfaceIP(address);
   }
#endif   
}

//==============================================================================
// BSocksHelper::addressesToString
//==============================================================================
HRESULT BSocksHelper::addressesToString(const SOCKADDR_IN &address1, const SOCKADDR_IN &address2, char stringOut[100])
{
   bsnprintf (stringOut, 99, "%hu.%hu.%hu.%hu:%hu %hu.%hu.%hu.%hu:%hu",
      address1.sin_addr.S_un.S_un_b.s_b1,
      address1.sin_addr.S_un.S_un_b.s_b2,
      address1.sin_addr.S_un.S_un_b.s_b3,
      address1.sin_addr.S_un.S_un_b.s_b4,
      address1.sin_port,
      address2.sin_addr.S_un.S_un_b.s_b1,
      address2.sin_addr.S_un.S_un_b.s_b2,
      address2.sin_addr.S_un.S_un_b.s_b3,
      address2.sin_addr.S_un.S_un_b.s_b4,
      address2.sin_port
      );

   return S_OK;
}

//==============================================================================
// BSocksHelper::stringToAddresses
//==============================================================================
HRESULT BSocksHelper::stringToAddresses(const char *string, SOCKADDR_IN *address1, SOCKADDR_IN *address2)
{  
   memset(address1, 0, sizeof(SOCKADDR_IN));
   memset(address2, 0, sizeof(SOCKADDR_IN));

   int ret = sscanf_s(string, "%hu.%hu.%hu.%hu:%hu %hu.%hu.%hu.%hu:%hu",
      &address1->sin_addr.S_un.S_un_b.s_b1,
      &address1->sin_addr.S_un.S_un_b.s_b2,
      &address1->sin_addr.S_un.S_un_b.s_b3,
      &address1->sin_addr.S_un.S_un_b.s_b4,
      &address1->sin_port,
      &address2->sin_addr.S_un.S_un_b.s_b1,
      &address2->sin_addr.S_un.S_un_b.s_b2,
      &address2->sin_addr.S_un.S_un_b.s_b3,
      &address2->sin_addr.S_un.S_un_b.s_b4,
      &address2->sin_port
      );
   ret;
   BASSERT(ret == 10);

   address1->sin_family = AF_INET;   
   address2->sin_family = AF_INET;

   return S_OK;
}

//==============================================================================
char *BSocksHelper::addressToString(const SOCKADDR_IN &address1)
{
   static char stringOut[100];
   bsnprintf (stringOut, 99, "%hu.%hu.%hu.%hu:%hu",
      address1.sin_addr.S_un.S_un_b.s_b1,
      address1.sin_addr.S_un.S_un_b.s_b2,
      address1.sin_addr.S_un.S_un_b.s_b3,
      address1.sin_addr.S_un.S_un_b.s_b4,
      address1.sin_port      
      );

   return &stringOut[0];
}

//==============================================================================
SOCKADDR_IN BSocksHelper::stringToAddress(const char *string)
{
   SOCKADDR_IN address1;
   memset(&address1, 0, sizeof(SOCKADDR_IN));   

   sscanf_s(string, "%hu.%hu.%hu.%hu:%hu",
      &address1.sin_addr.S_un.S_un_b.s_b1,
      &address1.sin_addr.S_un.S_un_b.s_b2,
      &address1.sin_addr.S_un.S_un_b.s_b3,
      &address1.sin_addr.S_un.S_un_b.s_b4,
      &address1.sin_port      
      );

   address1.sin_family = AF_INET;      

   return address1;
}

//==============================================================================
// BSocksHelper::isValidExternalIP
//==============================================================================
bool BSocksHelper::isValidExternalIP(const SOCKADDR_IN& address)
{
   if (address.sin_family != AF_INET)
      return false;
   
   if (
         (address.sin_addr.S_un.S_un_b.s_b1 == 127) &&
         (address.sin_addr.S_un.S_un_b.s_b2 == 0) &&
         (address.sin_addr.S_un.S_un_b.s_b3 == 0) &&
         (address.sin_addr.S_un.S_un_b.s_b4 == 1)
      )
      return false;
   else if (
         (address.sin_addr.S_un.S_un_b.s_b1 == 10)         
      )
      return false;
   else if (
         (address.sin_addr.S_un.S_un_b.s_b1 == 172) &&
         (address.sin_addr.S_un.S_un_b.s_b2 >= 16) &&
         (address.sin_addr.S_un.S_un_b.s_b2 <= 31)
      )
      return false;   
   else if (
         (address.sin_addr.S_un.S_un_b.s_b1 == 192) &&
         (address.sin_addr.S_un.S_un_b.s_b2 == 168)         
      )
      return false;
   else
      return true;
}

//==============================================================================
// BSocksHelper::pumpMessages
//==============================================================================
void BSocksHelper::pumpMessages(void)
{
#ifndef XBOX
   MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) 
   {
		if (!GetMessage(&msg, NULL, 0, 0)) 
      {
			PostQuitMessage(msg.wParam);
			return;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);		
	}
#endif	
}

//==============================================================================
// BSocksHelper::getFirstInterfaceIP
//==============================================================================
DWORD BSocksHelper::getFirstInterfaceIP(SOCKADDR_IN *address)
{
   nlog(cTransportNL, "BSocksHelper::getFirstInterfaceIP -- ENTER.");
   
   if (!address)
      return ERROR_INVALID_ADDRESS;

#ifdef XBOX
   XNADDR xna;
   if (!getXNAddr(xna))
      return static_cast<DWORD>(E_FAIL);
              
   address->sin_family = AF_INET;
   address->sin_port = 0;
   address->sin_addr = xna.ina;
#else
   SOCKET udpSocket = WSASocket (AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);

   if (udpSocket == INVALID_SOCKET)
   {
      //logErrorHR (HRESULT_FROM_WIN32 (GetLastError()), "WSASocket failed");
      return WSAGetLastError();
   }

   const DWORD BufferLength = 0x400;
   LPBYTE Buffer = new BYTE [BufferLength];

   if (Buffer == NULL)
   {
      closesocket (udpSocket);
      //logErrorHR (E_OUTOFMEMORY, "Out of memory");
      return WSAENOBUFS;
   }

   DWORD BytesTransferred;
   if (WSAIoctl (udpSocket, SIO_ADDRESS_LIST_QUERY, NULL, 0, Buffer, BufferLength, &BytesTransferred, NULL, NULL) == SOCKET_ERROR)
   {
      delete [] Buffer;
      closesocket (udpSocket);      
      //logErrorHR (HRESULT_FROM_WIN32 (GetLastError()), "WSAIOctl SIO_ADDRESS_LIST_QUERY failed");
      return WSAGetLastError();
   }


   closesocket (udpSocket);

   if (BytesTransferred == 0)
   {
      delete [] Buffer;      
      //logErrorHR (E_FAIL, "Out of memory");
      return WSAGetLastError();
   }


   SOCKET_ADDRESS_LIST * SocketAddressList = reinterpret_cast <SOCKET_ADDRESS_LIST *> (Buffer);

   //BASSERT (SocketAddressList -> iAddressCount > 0);   

   if (SocketAddressList && (SocketAddressList -> iAddressCount > 0))
   {   
      *address = * reinterpret_cast <SOCKADDR_IN *> (SocketAddressList -> Address[0].lpSockaddr);        
   }
   else
   {
      delete [] Buffer;  
      return WSAEADDRNOTAVAIL;
   }

   delete [] Buffer;
#endif

   return S_OK;
}

static bool inService = false;
static BDynamicSimArray<BSocksSocketBase*>  addSockets;
static BDynamicSimArray<BSocksSocketBase*>  removeSockets;

//==============================================================================
// BSocksHelper::
//==============================================================================
void BSocksHelper::addSocket(BSocksSocketBase *pSocket)
{
   if (inService)
      addSockets.add(pSocket);
   else
      mSockets.add(pSocket);
}

//==============================================================================
// BSocksHelper::
//==============================================================================
void BSocksHelper::removeSocket(BSocksSocketBase *pSocket)
{
   if (inService)
      removeSockets.add(pSocket);
   else
      mSockets.remove(pSocket);
}

//==============================================================================
// BSocksHelper::
//==============================================================================
void BSocksHelper::serviceSockets(void)
{
   inService = true;

/*
   long count = mSockets.getNumber();
   for (long idx=0; idx<count; idx++)
      mSockets[idx]->service();

   count = removeSockets.getNumber();
   for (idx=0; idx<count; idx++)
      mSockets.remove(removeSockets[idx]);
   removeSockets.clear();

   count = addSockets.getNumber();
   for (idx=0; idx<count; idx++)
      mSockets.add(addSockets[idx]);
   addSockets.clear();
*/

   inService = false;
}

void BSocksHelper::socksTickActiveSockets()
{
#ifdef XBOX
   SCOPEDSAMPLE(BSocksHelper_socksTickActiveSockets)

   // the timeline profile uses BSocksReliableSocket
#if defined(ENABLE_TIMELINE_PROFILER)
   BSocksSocketBase::tickActiveSockets();
#endif

   //BLoopbackSocket::tickActiveSockets();
   //BReliableConnection::tickActiveConnections();
   //BUDP360Connection::tickActiveConnections();

   // BUDP360Socket still used for lan game discovery
   //BUDP360Socket::tickActiveSockets();
#endif   
}

#ifndef XBOX
//==============================================================================
// BSocksHelper::WindowProcedure
//==============================================================================
LRESULT BSocksHelper::WindowProcedure(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   if (wMsg == WM_ASYNCNAMERESOLVE)
   {
      if (mNotify)
      {
         // if there was an error, notify and bail
         if (WSAGETASYNCERROR(lParam) != 0)
         {
            mNotify->addressResult(-1);
            mNotify = NULL;
            return 0;
         }

         // pull out the result and notify
         struct hostent *he = (hostent*)sHostBuffer;
         // wtf? 0 length addresses
         if (he->h_length == 0)
            mNotify->addressResult(-1);
         else
         {
            long addressOut= *(long *)he -> h_addr_list [0];         
            mNotify->addressResult(addressOut);
         }
         mNotify = NULL;
      }
      return 0;
   }

   return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

//==============================================================================
// BSocketHelperWindowClass
//==============================================================================
class BSocksHelperWindowClass
{
private:

   ATOM m_ClassAtom;

public:

   BSocksHelperWindowClass (void)
   {
      WNDCLASSEXA WindowClass;

      ZeroMemory (&WindowClass, sizeof WindowClass);

      WindowClass.cbSize = sizeof WindowClass;
      WindowClass.lpfnWndProc = BSocksHelper::WindowProcedure;
      WindowClass.lpszClassName = SH_WINDOW_CLASS_NAME;
      WindowClass.hInstance = DllInstance;

      m_ClassAtom = RegisterClassExA (&WindowClass);

      if (m_ClassAtom == 0)
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "failed to register SocketHelper window class");
         nlogError (cTransportNL, Result);
      }
   }

   ~BSocksHelperWindowClass (void)
   {
      if (m_ClassAtom != 0)
      {
         UnregisterClass ((PCSTR) m_ClassAtom, DllInstance);
      }
   }

   ATOM GetClassAtom (void) const 
   { 
      return m_ClassAtom; 
   }
};

BSocksHelperWindowClass gSocketHelperWindowClass;
#endif

//==============================================================================
// eof: SocksHelpers.cpp
//==============================================================================
