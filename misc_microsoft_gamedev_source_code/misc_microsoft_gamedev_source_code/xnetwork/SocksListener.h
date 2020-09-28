//==============================================================================
// SocksListener.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _SocksListener_H_
#define _SocksListener_H_

//==============================================================================
// Includes

#include "Listener.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations



//==============================================================================
class BSocksReliableListener : public BReliableListener
{
   public:
      // Constructors
      BSocksReliableListener (
         IN BAcceptor * acceptor = 0);

      // Destructors
      virtual ~BSocksReliableListener(void);

      // Functions      
      virtual HRESULT listen (
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL,
         IN LONG Backlog = cDefaultBacklog);

      virtual HRESULT stopListening (void);

      // Variables

   public:

#ifndef XBOX
      static LRESULT WINAPI WindowProcedure (
         IN HWND Window,
         IN UINT Message,
         IN WPARAM Parameter1,
         IN LPARAM Parameter2);
#endif         

   private:

#ifndef XBOX
      enum { WM_NETWORK_EVENT = WM_USER };
#endif      
      
      void onNetworkEvent (
         IN DWORD Event,
         IN DWORD Status);

      HRESULT listenInternal (
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL,
         IN LONG Backlog);

   private:

      //
      // Used for message routing (not UI)
      //

      HWND mWindow;

      SOCKET mSocket;

}; // BSocksListener


//==============================================================================
#endif // _SocksListener_H_

//==============================================================================
// eof: SocksListener.h
//==============================================================================
