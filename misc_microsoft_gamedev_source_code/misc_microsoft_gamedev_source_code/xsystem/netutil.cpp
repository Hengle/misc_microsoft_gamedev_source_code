//============================================================================
// netutil.cpp
//
// Copyright (c) 2005, Ensemble Studios
//============================================================================

#include "xsystem.h"
#include "netutil.h"

#ifndef XBOX
#include <WinSock2.h>
#endif

// Globals
static int gNetRefCount=0;

//==============================================================================
// networkStartup
//==============================================================================
bool networkStartup()
{
   if(gNetRefCount>0)
   {
      gNetRefCount++;
      return true;
   }

#ifdef XBOX
   XNetStartupParams xnsp;
   memset(&xnsp, 0, sizeof(xnsp));
   xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
   xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;

/*
#ifndef BUILD_FINAL
   // ajl 9/26/08 - increase max sockets to 64 (default is 32) since sound system is requesting > 32 files and that
   // causes XFS to fail.
   xnsp.cfgSockMaxStreamSockets = 102;
#endif
*/

   INT err = XNetStartup(&xnsp);
   if (0 != err)
      return false;

   if( XOnlineStartup() != ERROR_SUCCESS )
   {
	   return false;
   }
#endif

   //WORD wVersionRequested = MAKEWORD( 2, 0 );
   //WSADATA wsaData;

   //if (WSAStartup (wVersionRequested, &wsaData) != 0)
   //   return false;

   gNetRefCount++;

   return true;
}

//==============================================================================
// BSocksHelper::networkCleanup
//==============================================================================
void networkCleanup()
{
   if(gNetRefCount<1)
   {
      BASSERT(0);
      return;
   }

   gNetRefCount--;
   if(gNetRefCount>0)
      return;

#ifdef XBOX
   //WSACleanup();
   XOnlineCleanup();
#endif // XBOX
}

#ifdef XBOX
//==============================================================================
// createGUIDXbox
//==============================================================================
void createGUIDXbox(GUID* pGUID)
{
   // rg [6/21/05] - Untested!! This is quick and dirty and may not be a very good implementation. I have not researched how to gen GUID's.

   static unsigned long nextGUIDIndex;

   memset(pGUID, 0, sizeof(GUID));

   // Get MAC address
   XNADDR xna;

   getXNAddr(xna);

   memcpy(pGUID->Data4, &xna.abEnet, sizeof(xna.abEnet));

   // Get system time
   SYSTEMTIME sysTime;
   GetSystemTime(&sysTime);

   // Convert to file time
   FILETIME fileTime;
   BOOL success = SystemTimeToFileTime(&sysTime, &fileTime);
   if (success)
   {
      pGUID->Data1 = fileTime.dwHighDateTime;
      pGUID->Data2 = static_cast<unsigned short>(fileTime.dwLowDateTime);
      pGUID->Data3 = static_cast<unsigned short>(fileTime.dwLowDateTime >> 16);
   }

   pGUID->Data4[6] = static_cast<unsigned char>(nextGUIDIndex);
   pGUID->Data4[7] = static_cast<unsigned char>(nextGUIDIndex >> 8);
   nextGUIDIndex++;
}

//==============================================================================
// stringFromGUIDXbox
// ajl [6/27/05] - Untested
//==============================================================================
long stringFromGUIDXbox(GUID& rguid, WCHAR* lpsz, int cchMax)
{
   /*
   rguid.Data1; // DWORD
   rguid.Data2; // WORD
   rguid.Data3; // WORD
   rguid.Data4; // BYTE[8]

   output example: {c200e360-38c5-11ce-ae62-08002b2b79ef}
   */

   if(cchMax<39)
      return 0;

   const BYTE* const d1=(BYTE*)(&rguid.Data1);
   const BYTE* const d2=(BYTE*)(&rguid.Data1);
   const BYTE* const d3=(BYTE*)(&rguid.Data1);
   const BYTE* const d4=rguid.Data4;

   HRESULT hr=StringCchPrintfW(lpsz, cchMax, L"{%2hx%2hx%2hx%2hx-%2hx%2hx-%2hx%2hx-%2hx%2hx%2hx%2hx%2hx%2hx%2hx%2hx}", 
      (WORD)d1[0], 
      (WORD)d1[1], 
      (WORD)d1[2], 
      (WORD)d1[3], 
      (WORD)d2[0], 
      (WORD)d2[1], 
      (WORD)d3[0], 
      (WORD)d3[1], 
      (WORD)d4[0], 
      (WORD)d4[1], 
      (WORD)d4[2], 
      (WORD)d4[3], 
      (WORD)d4[4], 
      (WORD)d4[5], 
      (WORD)d4[6], 
      (WORD)d4[7]);

   if(FAILED(hr))
      return 0;
   else
      return wcslen(lpsz)+1;
}

//==============================================================================
// gethostbyname
//==============================================================================
hostent* gethostbyname(const char* name)
{
   static hostent hostEnt={0};

   XNDNS* pXNDNS = NULL;

   int result = XNetDnsLookup(name, NULL, &pXNDNS);

   if (0 != result)
      return NULL;

   for(;;)
   {
      if(pXNDNS->iStatus!=WSAEINPROGRESS)
         break;
   }

   if (0 != pXNDNS->iStatus)
   {
      XNetDnsRelease(pXNDNS);      
      return NULL;
   }

   strcpy_s(hostEnt.h_name, sizeof(hostEnt.h_name), name);

   hostEnt.h_addr_list[0][0]=pXNDNS->aina[0].S_un.S_un_b.s_b1;
   hostEnt.h_addr_list[0][1]=pXNDNS->aina[0].S_un.S_un_b.s_b2;
   hostEnt.h_addr_list[0][2]=pXNDNS->aina[0].S_un.S_un_b.s_b3;
   hostEnt.h_addr_list[0][3]=pXNDNS->aina[0].S_un.S_un_b.s_b4;

   hostEnt.h_length=4;
   hostEnt.h_addrtype=AF_INET;

   XNetDnsRelease(pXNDNS);   

   return &hostEnt;
}

//==============================================================================
// gethostname
// rg [6/22/05] - Untested and incomplete.
//==============================================================================
int gethostname(char* name, int namelen)
{
   XNADDR xna;
   if (!getXNAddr(xna))
      return SOCKET_ERROR;

   XNetInAddrToString(xna.ina, name, namelen);
   return 0;  
}

//==============================================================================
// inet_ntoa
//==============================================================================
char* inet_ntoa(struct in_addr in)
{
   static char buf[32];
   XNetInAddrToString(in, buf, sizeof(buf));
   return buf;
}

//==============================================================================
// getXNAddr
//==============================================================================
bool getXNAddr(XNADDR& a)
{
   memset(&a, 0, sizeof(a));
   DWORD dwRet = 0;
   do
   {
      dwRet = XNetGetTitleXnAddr(&a);
   } 
   while (dwRet == XNET_GET_XNADDR_PENDING);

   if(dwRet & XNET_GET_XNADDR_NONE)
      return false;

   return true;
}

#endif // XBOX
