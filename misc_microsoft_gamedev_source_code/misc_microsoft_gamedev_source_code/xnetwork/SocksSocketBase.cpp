//#include <map>
#include "precompiled.h"
#include "SocksSocketBase.h"

//xsystem
#include "timelineprofiler.h"


#define DllInstance GetModuleHandle (NULL)         // hack-o-tron!

static const CHAR SOCKS_WINDOW_CLASS_NAME [] = "SocksAsyncSocketClass";
static const CHAR SOCKS_WINDOW_NAME [] = "SocksAsyncSocket";


#ifndef XBOX
BSocksWindowClass gSocksWindowClass;
#endif


#ifdef XBOX
int BSocksSocketBase::mNumActiveSockets = 0;
BSocksSocketBase* BSocksSocketBase::mpActiveSockets[BSocksSocketBase::MaxActiveSockets];
#endif

//
// BSocksSocketBase
//

#ifdef XBOX
void BSocksSocketBase::tickActiveSockets(void)
{
   int i;
   for (i = 0; i < mNumActiveSockets; i++)
      mpActiveSockets[i]->checkForEvents(true);
}
#endif

BSocksSocketBase::BSocksSocketBase (IN BSocket::BObserver * Observer) :
   BSocket (Observer),
   mSocket (INVALID_SOCKET)
#ifdef XBOX
   ,mEventMask(0),
   mEventHandle(WSA_INVALID_EVENT),
   mSendMessages(false)
#endif
{
#ifdef XBOX
   for (int i = 0; i < MaxTimers; i++)
   {
      mTimers[i].mInterval = 0;
      mTimers[i].mLastTick = 0;
   }
#else
   mWindow = NULL;
#endif   
}

HRESULT BSocksSocketBase::asyncSelect (
   IN DWORD EventMask)
{   
#ifndef XBOX
   if (mWindow == NULL)
   {
      mWindow = CreateWindowExA (
         0,             // extended styles
      //   (PCSTR) g_ResolveWindowClass.GetClassAtom(), // window class
         SOCKS_WINDOW_CLASS_NAME,
         SOCKS_WINDOW_NAME,
         0,             // style
         0, 0,          // position
         0, 0,          // size
         NULL,          // parent window
         NULL,          // menu handle
         DllInstance,   // DLL resource instance handle
         this);         // creation instance parameter

      if (mWindow == NULL)
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksSocketBase: failed to create message routing window -- socket will NOT work");
         nlogError (cTransportNL, Result);
         return Result;
      }
   }

   if (mSocket != INVALID_SOCKET)
   {
      if (WSAAsyncSelect (mSocket, mWindow, SOCKS_WM_NETWORK_EVENT, EventMask) == SOCKET_ERROR)
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksSocketBase: WSAAsyncSelect failed, will not be able to receive network events");
         nlogError (cTransportNL, Result);
         return Result;
      }
   }
#else
   int i;
   for (i = 0; i < mNumActiveSockets; i++)
      if (mpActiveSockets[i] == this)
         break;
   
   if (i == mNumActiveSockets)
   {
      BASSERT(mNumActiveSockets < MaxActiveSockets);
      mpActiveSockets[mNumActiveSockets++] = this;
   }         
      
   if (mSocket != INVALID_SOCKET)
   {
      if (WSA_INVALID_EVENT == mEventHandle)
      {
         mEventHandle = WSACreateEvent();
         
         if (WSA_INVALID_EVENT == mEventHandle)
         {
            HRESULT Result = GetLastResult();
            nlog (cTransportNL, "BSocksSocketBase: WSACreateEvent failed");
            nlogError (cTransportNL, Result);
            return Result;  
         }
         
         if (WSAEventSelect(mSocket, mEventHandle, FD_CLOSE) == SOCKET_ERROR)
         {
            HRESULT Result = GetLastResult();
            nlog (cTransportNL, "BSocksSocketBase: WSAEventSelect failed, will not be able to receive network events");
            nlogError (cTransportNL, Result);
            return Result;  
         }
      }

#if 0
      // Set socket to nonblocking.
      DWORD ioctl_opt = 1;                   
      if (0 != ioctlsocket(mSocket, FIONBIO, &ioctl_opt))
      {
         HRESULT Result = GetLastResult();
         nlog (cTransportNL, "BSocksSocketBase: ioctlsocket() select failed");
         nlogError (cTransportNL, Result);
         return Result;
      }
#endif
   } 
   
   mSendMessages = true;      
   
   mEventMask = EventMask;
#endif   

   return S_OK;
}

#ifdef XBOX
void BSocksSocketBase::checkForEvents(bool tickTimers)
{
   if (!mSendMessages)
      return;
         
   const bool read = (0 != (mEventMask & FD_READ));
   const bool write = (0 != (mEventMask & FD_WRITE));
   const bool close = (0 != (mEventMask & FD_CLOSE));
   
   if (mSocket != INVALID_SOCKET)
   {
      if (close)
      {
         BASSERT(WSA_INVALID_EVENT != mEventHandle);
         
         const DWORD waitResult = WaitForSingleObject(mEventHandle, 0);
         if (WAIT_OBJECT_0 == waitResult)
         {
            WSAResetEvent(mEventHandle);
            networkClose (S_OK);
         }
      }         
   }

   if (mSocket != INVALID_SOCKET)      
   {
      if (read || write)
      {
         //for ( ; ; )
         //{
            const timeval timeout = { 0, 0 };   

            fd_set read_fd;
            FD_ZERO(&read_fd);
            if (read)
               FD_SET(mSocket, &read_fd);
               
            fd_set write_fd;
            FD_ZERO(&write_fd);
            if (write)
               FD_SET(mSocket, &write_fd);

            int res = SOCKET_ERROR;

            {
               SCOPEDSAMPLE(BSocksSocketBase_checkForEvents_select)

               res = select(0, read ? &read_fd : NULL, write ? &write_fd : NULL, NULL, &timeout);
            }

            //bool tryAgain = false;
            long count=0;

            if (SOCKET_ERROR != res)
            {
               if ((read) && (FD_ISSET(mSocket, &read_fd)))
               {
                  while (recvReady() && ++count < 5) {}
                  //tryAgain = true;
               }
               if ((write) && (mSocket != INVALID_SOCKET) && (FD_ISSET(mSocket, &write_fd)))
                  sendReady();
            }

         //   if (!tryAgain)
         //      break;
         //}
      }
   }      

   if (tickTimers)
   {
      const DWORD curTickCount = GetTickCount();

      for (DWORD i = 0; i < MaxTimers; i++)
      {
         if (mTimers[i].mInterval)
         {
            if ((curTickCount - mTimers[i].mLastTick) >= mTimers[i].mInterval)
            {
               mTimers[i].mLastTick = curTickCount;
               tic(i + 1);
            }
         }
      }

      // tick the socket so flush any buffers we have
      // be careful because the previous timer checks may have already ticked the socket
      // hopefully, the previous tick will have added to the existing buffers and
      // then we can flush them here
      tic(0);
   }
}
#endif

BSocksSocketBase::~BSocksSocketBase (void)
{
#ifdef XBOX
   if (WSA_INVALID_EVENT != mEventHandle)
   {  
      if (INVALID_SOCKET != mSocket)
         WSAEventSelect(mSocket, mEventHandle, 0);
      WSACloseEvent(mEventHandle);
      mEventHandle = WSA_INVALID_EVENT;
   }
   
   removeFromActiveSockets();
#endif   
}

#ifdef XBOX
void BSocksSocketBase::removeFromActiveSockets(void)
{
   int i;
   for (i = 0; i < mNumActiveSockets; i++)
   {  
      if (mpActiveSockets[i] == this)
      {
         mpActiveSockets[i] = mpActiveSockets[mNumActiveSockets - 1];
         break;
      }
   }

   if (i < mNumActiveSockets)
      mNumActiveSockets--;
}
#endif

HRESULT BSocksSocketBase::dispose(void)
{
   //
   // We expect the derived destructor to close the socket.
   //

   BASSERT (mSocket == INVALID_SOCKET);

#ifdef XBOX
   checkForEvents(false);
   mSendMessages = false;
      
   removeFromActiveSockets();
#else
   if (mWindow != NULL)
   {
      DestroyWindow (mWindow);
      //SleepEx(0, true);
      mWindow = NULL;
   }
#endif   

   BASSERT (mSocket == INVALID_SOCKET);   

   return BSocket::dispose();
}

#ifndef XBOX
// static
LRESULT BSocksSocketBase::WindowProcedure (
   IN HWND Window,
   IN UINT Message,
   IN WPARAM Parameter1,
   IN LPARAM Parameter2)
{
   if (Message == WM_CREATE)
   {
      SetWindowLong (Window, GWL_USERDATA,
         reinterpret_cast <LONG> (reinterpret_cast <LPCREATESTRUCT> (Parameter2) -> lpCreateParams));
   
      return 0;
   }

   if (Message == WM_DESTROY)
   {      
      SetWindowLong (Window, GWL_USERDATA, 0);     
   }

   BSocksSocketBase * Instance = reinterpret_cast <BSocksSocketBase *> (GetWindowLong (Window, GWL_USERDATA));
   if (Instance == NULL)
   {
      return DefWindowProc (Window, Message, Parameter1, Parameter2);
   }


   switch (Message)
   {         
#if 0
   case SOCKS_WM_RESOLUTION_COMPLETE:
      Instance -> resolveComplete (WSAGETASYNCBUFLEN (Parameter2), WSAGETASYNCERROR (Parameter2));
      break;
#endif

   case SOCKS_WM_NETWORK_EVENT:
      Instance -> networkEvent (WSAGETSELECTEVENT (Parameter2), WSAGETSELECTERROR (Parameter2));
      break;

   case WM_TIMER:
      Instance -> tic ( Parameter1 );
      break;

   default:
      return DefWindowProc (Window, Message, Parameter1, Parameter2);
   }

   return 0;
}
#endif

#ifndef XBOX
LRESULT BSocksSocketBase::onWindowMessage (
   IN HWND Window,
   IN UINT Message,
   IN WPARAM Parameter1,
   IN LPARAM Parameter2)
{
   return DefWindowProc (Window, Message, Parameter1, Parameter2);
}
#endif

void BSocksSocketBase::asyncStop (void)
{
#ifdef XBOX
    checkForEvents(false);
    mSendMessages = false;
    
    removeFromActiveSockets();
    
   if (WSA_INVALID_EVENT != mEventHandle)
   {  
      if (INVALID_SOCKET != mSocket)
         WSAEventSelect(mSocket, mEventHandle, 0);
      WSACloseEvent(mEventHandle);
      mEventHandle = WSA_INVALID_EVENT;
   }

   for (int i = 0; i < MaxTimers; i++)
   {
      mTimers[i].mInterval = 0;
      mTimers[i].mLastTick = 0;
   }

   mEventMask = 0;
#else
   if (mWindow != NULL)
   {
      WSAAsyncSelect (mSocket, mWindow, 0, 0);
      DestroyWindow (mWindow);
      //SleepEx(0, true);
      mWindow = NULL;
   }
#endif   
}


void BSocksSocketBase::networkEvent (
   IN DWORD Event,
   IN DWORD Status)
{
   switch (Event)
   {
   case FD_CLOSE:
      networkClose (HRESULT_FROM_WIN32 (Status));
      break;

   case FD_READ:
      recvReady ();
      break;

   case FD_WRITE:
      sendReady ();
      break;

#ifndef XBOX
   case FD_ADDRESS_LIST_CHANGE:
      if (getObserver())
         getObserver()->interfaceAddressChanged();
      break;
#endif

   default:
      nlog (cTransportNL, "BSocksSocketBase: received unknown network event 0x%08x %u", Event, Event);
      break;
   }
}

BOOL BSocksSocketBase::recvReady (void)
{
   return FALSE;
}

void BSocksSocketBase::sendReady (void)
{
}

HRESULT BSocksSocketBase::startTimer ( DWORD timerID, DWORD interval )
{
#ifdef XBOX
   if (!mSendMessages)
      return E_FAIL;
      
   if ((timerID < 1) || (timerID > MaxTimers))
      return E_FAIL;
   
   mTimers[timerID - 1].mInterval = interval;
   mTimers[timerID - 1].mLastTick = GetTickCount();
#else
   if (!mWindow)
      return E_FAIL;

   if (SetTimer( mWindow, timerID, interval, 0 ) == 0)
      return E_FAIL;
#endif

   return S_OK;
}

void BSocksSocketBase::stopTimer ( DWORD timerID )
{
#ifdef XBOX
   if ((timerID < 1) || (timerID > MaxTimers))
      return;

   mTimers[timerID - 1].mInterval = 0;
   mTimers[timerID - 1].mLastTick = 0;
#else
   KillTimer( mWindow, timerID );         
#endif   
}

void BSocksSocketBase::tic (DWORD timerID)
{
   UNREFERENCED_PARAMETER (timerID);
}

void BSocksSocketBase::networkClose (IN HRESULT Result)
{
   UNREFERENCED_PARAMETER (Result);
}


#ifndef XBOX
//////////////////////////////////////////////////////////////////////////////

BSocksWindowClass::BSocksWindowClass (void)
{
   WNDCLASSEXA WindowClass;

   ZeroMemory (&WindowClass, sizeof WindowClass);

   WindowClass.cbSize = sizeof WindowClass;
   WindowClass.lpfnWndProc = BSocksSocketBase::WindowProcedure;
   WindowClass.lpszClassName = SOCKS_WINDOW_CLASS_NAME;
   WindowClass.hInstance = DllInstance;

   m_ClassAtom = RegisterClassExA (&WindowClass);

   if (m_ClassAtom == 0)
   {
      HRESULT Result = GetLastResult();
      nlog (cTransportNL, "failed to register DNS resolution window class");
      nlogError (cTransportNL, Result);
   }
}

BSocksWindowClass::~BSocksWindowClass (void)
{
   if (m_ClassAtom != 0)
   {
      UnregisterClass ((PCSTR) m_ClassAtom, DllInstance);
   }
}

ATOM BSocksWindowClass::GetClassAtom (void) const 
{ 
   return m_ClassAtom; 
}
#endif