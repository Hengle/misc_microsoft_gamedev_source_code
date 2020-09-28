//==============================================================================
// SocksHelper.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _SocksHelper_H_
#define _SocksHelper_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BSocksSocketBase;

//==============================================================================
// Const declarations


//==============================================================================
class BSocksHelper
{
   public:
      class BAddressNotify
      {
         public:
            virtual void addressResult(long addressOut) = 0;
      };

      static HRESULT             socksStartup();
      static HRESULT             socksCleanup();
      static void                socksTickActiveSockets();

      static HRESULT             socksCatchError(int value);
      static HRESULT             socksGetAddress(const char *addressIn, BAddressNotify* notify);
      static HRESULT             socksGetAddress(long *addressOut, const char *addressIn);
      static HRESULT             socksGetName(long addressIn, char *addressOut, long len);
      static HRESULT             socksGetHostName(char *name, long len);

      static SOCKADDR_IN         &getLocalIP(void);
      static DWORD               getLocalIP(const SOCKADDR_IN& remoteRoutingAddress, SOCKADDR_IN *address, bool interfaceQuery);
      static char                *addressToString(const SOCKADDR_IN &address);
      static SOCKADDR_IN         stringToAddress(const char *string);
      static HRESULT             addressesToString(const SOCKADDR_IN &address1, const SOCKADDR_IN &address2, char stringOut[100]);
      static HRESULT             stringToAddresses(const char *string, SOCKADDR_IN *address1, SOCKADDR_IN *address2);      
      static bool                isValidExternalIP(const SOCKADDR_IN& address); 
      static void                pumpMessages(void);

      static DWORD               getFirstInterfaceIP(SOCKADDR_IN *address);
      
      static void                addSocket(BSocksSocketBase *pSocket);
      static void                removeSocket(BSocksSocketBase *pSocket);
      static void                serviceSockets(void);

#ifndef XBOX
      static LRESULT WINAPI      WindowProcedure(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
#endif      

   private:
      static BAddressNotify      *mNotify;
      static HANDLE              mGetHostHandle;
#ifndef XBOX      
      static HWND                mWindow;
#endif      
      static BDynamicSimArray<BSocksSocketBase*>  mSockets;
      BSocksHelper() {}
}; // BSocksHelper

//==============================================================================
#endif // _SocksHelper_H_

//==============================================================================
// eof: SocksHelper.h
//==============================================================================
