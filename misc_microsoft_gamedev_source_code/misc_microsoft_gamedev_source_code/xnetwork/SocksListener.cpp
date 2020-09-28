//==============================================================================
// SocksListener.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "SocksListener.h"
#include "SocksReliableSocket.h"

//==============================================================================
// Defines


#define DllInstance GetModuleHandle (NULL)         // hack-o-tron!

#ifndef XBOX
class BSocksReliableListenerWindowClass
{
private:

   ATOM mWindowClassAtom;

public:

   ATOM getWindowClassAtom (void) const
   {
      return mWindowClassAtom;
   }

   BSocksReliableListenerWindowClass (void)
   {
      WNDCLASSEXA WindowClass;

      ZeroMemory (&WindowClass, sizeof WindowClass);

      WindowClass.lpszClassName = "ReliableListenerWindowClass";
      WindowClass.cbSize = sizeof WindowClass;
      WindowClass.hInstance = DllInstance;
      WindowClass.lpfnWndProc = BSocksReliableListener::WindowProcedure;

      mWindowClassAtom = RegisterClassExA (&WindowClass);

      if (mWindowClassAtom == 0)
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksReliableListenerWindowClass: failed to register window class -- listener will not work");
         nlogError (cTransportNL, Result);
      }
   }

   ~BSocksReliableListenerWindowClass (void)
   {
      if (mWindowClassAtom != 0)
      {
         UnregisterClass (reinterpret_cast <PCSTR> (mWindowClassAtom), DllInstance);
      }
   }
};

BSocksReliableListenerWindowClass gSocksReliableListenerWindowClass;
#endif


//==============================================================================
// BSocksReliableListener::BSocksReliableListener
//==============================================================================
BSocksReliableListener::BSocksReliableListener(BAcceptor *acceptor) :
   BReliableListener(acceptor),
   mSocket (INVALID_SOCKET)
{

} // BSocksReliableListener::BSocksReliableListener

//==============================================================================
// BSocksReliableListener::~BSocksReliableListener
//==============================================================================
BSocksReliableListener::~BSocksReliableListener(void)
{
   HRESULT hr = stopListening();
   BASSERT(SUCCEEDED(hr));
} // BSocksReliableListener::~BSocksReliableListener

//==============================================================================
// BSocksReliableListener::listen
//==============================================================================


HRESULT BSocksReliableListener::listen (
   IN CONST SOCKADDR_IN * LocalAddress OPTIONAL,
   IN LONG Backlog)
{
   HRESULT Result;

   Result = listenInternal (LocalAddress, Backlog);
   if (FAILED (Result))
   {
      HRESULT hr = stopListening();
      BASSERT(SUCCEEDED(hr));
   }

   return Result;
}

HRESULT BSocksReliableListener::listenInternal (
   IN CONST SOCKADDR_IN * LocalAddress OPTIONAL,
   IN LONG Backlog)
{
   HRESULT Result;

   // create a socket to listen on

#ifdef XBOX
   // rg [6/22/05] - Sockets are created with the overlapped attribute set by default on Xenon.
   mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
   mSocket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
#endif   
   if (mSocket == INVALID_SOCKET)
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: failed to create listener socket");
      nlogError (cTransportNL, Result);
      return Result;
   }

   //
   // Ask for SO_REUSEADDR to prevent problems with app restarting quickly.
   //

   BOOL OptionValue = TRUE;
   if (setsockopt (mSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast <char *> (&OptionValue), sizeof OptionValue) == SOCKET_ERROR)
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: failed to enable SO_REUSEADDR on socket, continuing anyway");
      nlogError (cTransportNL, Result);
   }

   //
   // Bind the socket.
   //

   SOCKADDR_IN AnyAddress;
   if (LocalAddress == NULL)
   {
      AnyAddress.sin_family = AF_INET;
      AnyAddress.sin_port = 0;
      AnyAddress.sin_addr.s_addr = 0;
      LocalAddress = &AnyAddress;
   }

   if (bind (mSocket, reinterpret_cast <CONST SOCKADDR *> (LocalAddress), sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: failed to bind socket, port 0x%04x %u",
         htons (LocalAddress -> sin_port),
         htons (LocalAddress -> sin_port));
      nlogError (cTransportNL, Result);
      return Result;
   }

   //
   // Listen on the socket
   //

   if (::listen (mSocket, Backlog) == SOCKET_ERROR)
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: failed to listen on socket");
      nlogError (cTransportNL, Result);
      return Result;
   }

#ifndef XBOX
   //
   // Create the window, for message routing
   //

   mWindow = CreateWindowEx (
      0,                // ex style
      reinterpret_cast <LPCSTR> (gSocksReliableListenerWindowClass.getWindowClassAtom()),
      "ReliableListenerWindow",  // title
      0,                      // style (not visible)
      0, 0, 0, 0,             // window geometry
      NULL,                   // parent window
      NULL,                   // menu
      DllInstance,            // DLL instance
      this);                  // creation parameter

   if (mWindow == NULL)
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: failed to create messaging window");
      nlogError (cTransportNL, Result);
      return Result;
   }

   //
   // Set up the asynchronous accept.
   // This API call puts mSocket into non-blocking mode.
   //

   if (WSAAsyncSelect (mSocket, mWindow, WM_NETWORK_EVENT, FD_ACCEPT) == SOCKET_ERROR)
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: async select failed");
      nlogError (cTransportNL, Result);
      return Result;
   }
#else
   // Set socket to nonblocking.
   DWORD ioctl_opt = 1;                   
   if (0 != ioctlsocket(mSocket, FIONBIO, &ioctl_opt))
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: ioctlsocket() select failed");
      nlogError (cTransportNL, Result);
      return Result;
   }
#endif   

   //
   // Print the actual address bound to debugger
   //

   SOCKADDR_IN SocketAddress;
   INT SocketAddressLength = sizeof SocketAddress;
   if (getsockname (mSocket, reinterpret_cast <SOCKADDR *> (&SocketAddress), &SocketAddressLength) == SOCKET_ERROR)
   {
      Result = GetLastResult();
      nlog (cTransportNL, "BSocksReliableListener: listener is ready, but failed to query local address after bind");
      nlogError (cTransportNL, Result);
   }
   else
   {
      nlog (cTransportNL, "BSocksReliableListener: listener is ready, requested port 0x%04x %u, actual port 0x%04x %u",
         htons (LocalAddress -> sin_port),
         htons (LocalAddress -> sin_port),
         htons (SocketAddress.sin_port),
         htons (SocketAddress.sin_port));
   }

   return S_OK;
}

//==============================================================================
// BSocksReliableListener::stopListening
//==============================================================================
HRESULT BSocksReliableListener::stopListening(void)
{
#ifndef XBOX
   if (mWindow != NULL)
   {
      DestroyWindow (mWindow);
      mWindow = NULL;
   }
#endif   

   if (mSocket != INVALID_SOCKET)
   {
      closesocket (mSocket);
      mSocket = INVALID_SOCKET;
   }

   return S_OK;
}


#ifndef XBOX
// static
LRESULT BSocksReliableListener::WindowProcedure (
   IN HWND Window,
   IN UINT Message,
   IN WPARAM Parameter1,
   IN LPARAM Parameter2)
{
   BSocksReliableListener * Instance = reinterpret_cast <BSocksReliableListener *> (GetWindowLong (Window, GWL_USERDATA));

   switch (Message)
   {
   case WM_CREATE:
      SetWindowLong (Window, GWL_USERDATA, reinterpret_cast <LONG> (reinterpret_cast <LPCREATESTRUCT> (Parameter2) -> lpCreateParams));
      break;

   case WM_NETWORK_EVENT:
      if (Instance != NULL)
      {
         Instance -> onNetworkEvent (WSAGETSELECTEVENT (Parameter2), WSAGETSELECTERROR (Parameter2));
      }
      break;

   default:
      return DefWindowProc (Window, Message, Parameter1, Parameter2);
   }

   return 0;
}
#endif

void BSocksReliableListener::onNetworkEvent (
   IN DWORD Event,
   IN DWORD Status)
{
   UNREFERENCED_PARAMETER (Status);

   if (Event != FD_ACCEPT)
   {
      nlog (cTransportNL, "BSocksReliableListener: received event (0x%08x) other than FD_ACCEPT?!", Event);
      return;
   }

   //
   // Try to accept a client.
   //

   SOCKADDR_IN SocketAddress;
   INT SocketAddressLength = sizeof SocketAddress;

   SOCKET ClientSocket = ::accept (mSocket, reinterpret_cast <SOCKADDR *> (&SocketAddress), &SocketAddressLength);
   if (ClientSocket == SOCKET_ERROR)
   {
      //
      // Hmmmmmm, odd.  WinSock says ya got a client, then it says ya don't.
      //

      nlog(cTransportNL, "BSocksReliableListener: received FD_ACCEPT, and tried to accept client, but failed");
      return;
   }

   if (getAcceptor() == NULL)
   {
      nlog(cTransportNL,"BSocksReliableListener: received client connection, but no acceptor is set");
      closesocket (ClientSocket);
      return;
   }

   SOCKADDR_IN RemoteAddress;
   INT RemoteAddressLength = sizeof RemoteAddress;

   if (getpeername (ClientSocket, reinterpret_cast <SOCKADDR *> (&RemoteAddress), &RemoteAddressLength) == SOCKET_ERROR)
   {
      nlog(cTransportNL, "BSocksReliableListener: received client connection, but could not query remote address");
      closesocket (ClientSocket);
      return;
   }


   if (!getAcceptor() -> canAccept (&RemoteAddress))
   {
      nlog(cTransportNL, "BSocksReliableListener: received client connection, but upper layers don't want it");
      closesocket (ClientSocket);
      return;
   }
   
   BSocksReliableSocket * ClientSocketObject = new BSocksReliableSocket (NULL);
   HRESULT Result = ClientSocketObject -> createFromAccept (ClientSocket);
   if (FAILED (Result))
   {
      ClientSocketObject->dispose();
      closesocket (ClientSocket);
      return;
   }

   nlog(cTransportNL, "BSocksReliableListener: received client connection, submitting to observer interface");

   getAcceptor() -> accepted (ClientSocketObject);
}


//==============================================================================
// BSocksReliableListener::
//==============================================================================



//==============================================================================
// eof: SocksListener.cpp
//==============================================================================
