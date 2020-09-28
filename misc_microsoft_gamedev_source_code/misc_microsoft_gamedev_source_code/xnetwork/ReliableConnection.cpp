//==============================================================================
// ReliableConnection.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "ReliableConnection.h"
#include "SocksGameSocket.h"
#include "SocksReliableSocket.h"
#include "SocksListener.h"
#include "SocksHelper.h"

#ifdef XBOX
int BReliableConnection::mNumActiveConnections = 0;
BReliableConnection* BReliableConnection::mpActiveConnections[BReliableConnection::MaxActiveConnections];
#endif

//==============================================================================
// Defines
static char connectSignature[] = "refactormyfacade";
const long cConnectTimeout = 10000;

//==============================================================================
// BReliableConnection::BReliableConnection
//==============================================================================
BReliableConnection::BReliableConnection(BSocket *socket) :
   mState(cDisconnected),
   mDirectGameSocket(0),
   mTranslatedGameSocket(0),
   mpObserver(0),
   mDirectGameSocketConnectTimer(0),
   mTranslatedGameSocketConnectTimer(0),
   mConnectTimer(0),
#ifndef XBOX   
   mWindow(0),
#endif   
   mDisableNotifications(false),
   mPrimary(true),   
   mSuppliedSocket(0),
   mConnectedSocket(0),
   mInServiceCB(false)
#ifdef XBOX
   ,mTimerInterval(0),
   mTimerLastTick(0)
#endif   
{   
   memset(&mRemoteAddress, 0, sizeof(SOCKADDR_IN));
   memset(&mTranslatedRemoteAddress, 0, sizeof(SOCKADDR_IN));

   if (socket)
   {
      mSuppliedSocket = socket;
      mConnectedSocket = mSuppliedSocket;      
      socket->setObserver(this);
      mState = cConnected;
   }   

#ifdef XBOX   
   if (mNumActiveConnections < MaxActiveConnections)
      mpActiveConnections[mNumActiveConnections++] = this;
#endif      
      
} // BReliableConnection::BReliableConnection

//==============================================================================
// 
void BReliableConnection::destroyUnconnectedSockets(void)
{
   mDisableNotifications = true;

   if (mDirectGameSocket && (mDirectGameSocket != mConnectedSocket))
   {
      mDirectGameSocket->dispose();
      mDirectGameSocket = 0;
   }
   if (mTranslatedGameSocket && (mTranslatedGameSocket != mConnectedSocket))
   {
      mTranslatedGameSocket->dispose();
      mTranslatedGameSocket = 0;
   }
   if (mSuppliedSocket && (mSuppliedSocket!= mConnectedSocket))
   {
      mSuppliedSocket->dispose();
      mSuppliedSocket = 0;
   }

   //SleepEx(0, true);

   mDisableNotifications = false;
}

//==============================================================================
// BReliableConnection::~BReliableConnection
//==============================================================================
BReliableConnection::~BReliableConnection(void)
{
#ifndef XBOX
   if (mWindow != NULL)
   {
      DestroyWindow (mWindow);
      mWindow = NULL;
   }   
#endif   

   mConnectedSocket = 0;
   destroyUnconnectedSockets();

#ifdef XBOX   
   int i;
   for (i = 0; i < mNumActiveConnections; i++)
   {  
      if (mpActiveConnections[i] == this)
      {
         mpActiveConnections[i] = mpActiveConnections[mNumActiveConnections - 1];
         break;
      }
   }

   BASSERT(i != mNumActiveConnections);
   mNumActiveConnections--;
#endif      

} // BReliableConnection::~BReliableConnection

#ifdef XBOX    
void BReliableConnection::tickActiveConnections(void)
{
   for (int i = 0; i < mNumActiveConnections; i++)
      mpActiveConnections[i]->checkForEvents();
}

void BReliableConnection::checkForEvents(void)
{
   if (mTimerInterval)
   {
      const DWORD curTickCount = GetTickCount();

      if (mTimerInterval)
      {
         if ((curTickCount - mTimerLastTick) >= mTimerInterval)
         {
            mTimerLastTick = curTickCount;
            tic(cConnectTimer);
         }
      }         
   }         
}
#endif

//==============================================================================
//
HRESULT BReliableConnection::send (
   const void* Buffer,
   const long Length,
   const DWORD flags)
{
   if (mState != cConnected)
   {
      nlog(cReliableConnNL, "BReliableConnection::send -- not connected.");
      return E_FAIL;
   }

   BASSERT(mConnectedSocket!=NULL);
   if (!mConnectedSocket)
   {
      nlog(cReliableConnNL, "BReliableConnection::send -- no connected socket.");
      return E_FAIL;
   }

   BSendBuffer *buf;
   HRESULT hr = mConnectedSocket->sendAllocateBuffer(Length, &buf);
   if (FAILED(hr))
   {
      nlog(cReliableConnNL, "BReliableConnection::send -- can't allocate buffer size %d", Length);
      return hr;
   }

   XMemCpy(buf->Buffer, Buffer, Length);
   buf->Length = Length;
   buf->Flags = flags;

   hr = mConnectedSocket->send(buf);
   if (FAILED(hr))
   {
      nlog(cReliableConnNL, "BReliableConnection::send -- mConnectedSocket failed send hr 0x%x", hr);
      return hr;
   }

   return S_OK;   
}

// BReliableConnection::send

//==============================================================================
//
HRESULT BReliableConnection::sendTo (
   IN CONST VOID * Buffer,
   IN ULONG Length,
   IN CONST SOCKADDR_IN * RemoteAddress)
{
   if (mState != cConnected)
      return E_FAIL;

   BASSERT(mConnectedSocket!=NULL);

   BSendBuffer *buf;
   HRESULT hr = mConnectedSocket->sendAllocateBuffer(Length, &buf);
   if (FAILED(hr))
      return hr;

   CopyMemory(buf->Buffer, Buffer, Length);
   buf->Length = Length;

   hr = mConnectedSocket->sendTo(buf, RemoteAddress);
   if (FAILED(hr))
      return hr;

   return S_OK;
}


//==============================================================================
//
HRESULT BReliableConnection::sendPacket(BPacket &packet)
{  
   if (mState != cConnected)
      return E_FAIL;

   BSerialBuffer sb;
   packet.serialize(sb);

   return send(sb.getBuffer(), sb.getBufferSize());
}
// BReliableConnection::sendPacket

//==============================================================================
//
HRESULT BReliableConnection::sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress)
{   
   BSerialBuffer sb;
   packet.serialize(sb);

   return sendTo(sb.getBuffer(), sb.getBufferSize(), RemoteAddress);
}
// BReliableConnection::sendPacket

#define DllInstance GetModuleHandle (NULL)         // hack-o-tron!

#ifndef XBOX
//==============================================================================
static const CHAR WINDOW_CLASS_NAME [] = "ReliableConnectionClass";
static const CHAR WINDOW_NAME [] = "ReliableConnection";

//==============================================================================
class BReliableConnectionWindowClass
{
private:

   ATOM m_ClassAtom;

public:

   BReliableConnectionWindowClass (void)
   {
      WNDCLASSEXA WindowClass;

      ZeroMemory (&WindowClass, sizeof WindowClass);

      WindowClass.cbSize = sizeof WindowClass;
      WindowClass.lpfnWndProc = BReliableConnection::WindowProcedure;
      WindowClass.lpszClassName = WINDOW_CLASS_NAME;
      WindowClass.hInstance = DllInstance;

      m_ClassAtom = RegisterClassExA (&WindowClass);

      if (m_ClassAtom == 0)
      {
         HRESULT Result = GetLastResult();
         nlog (cReliableConnNL, "failed to register DNS resolution window class");
         nlogError (cReliableConnNL, Result);
      }

   }

   ~BReliableConnectionWindowClass (void)
   {
      if (m_ClassAtom != 0)
      {
         UnregisterClass ((PCSTR) m_ClassAtom, DllInstance);
      }
   }

   ATOM GetClassAtom (void) const { return m_ClassAtom; }

};

BReliableConnectionWindowClass gReliableConnectionWindowClass;
#endif

//==============================================================================
//
HRESULT BReliableConnection::connect(bool primary, const SOCKADDR_IN &localAddress, const SOCKADDR_IN &translatedLocalAddress, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress)
{

   if (mState != cDisconnected)
      return E_FAIL;

#ifndef XBOX      
   // First set up a window, so we can attach a timer to it  
   if (mWindow == NULL)
   {
      mWindow = CreateWindowExA (
         0,             // extended styles
      //   (PCSTR) g_ResolveWindowClass.GetClassAtom(), // window class
         WINDOW_CLASS_NAME,
         WINDOW_NAME,
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
         nlog (cReliableConnNL, "BReliableConnection::connect -- failed to create message routing window -- connection will NOT work");
         nlogError (cReliableConnNL, Result);
         return Result;
      }
   }
#endif   

   nlog(cReliableConnNL, "BReliableConnection::connect -- attempt...[p]=%s", primary?"true":"false");
   nlog(cPerfNL, "BReliableConnection::connect -- attempt");

   SOCKADDR_IN laddr;
   memset(&laddr, 0, sizeof(laddr));
   laddr.sin_family = AF_INET;      

   // if the remote guy has the same address as our local address, don't try and connect to him there
   // coz it will only loop back and screw everything up :-)
   HRESULT hr = E_FAIL;
   if (localAddress.sin_addr.S_un.S_addr != remoteAddress.sin_addr.S_un.S_addr)
   {
      mDirectGameSocket = new BSocksGameSocket(this, localAddress);
      mDirectGameSocket->setObserver(this);
      hr = mDirectGameSocket->connect(&remoteAddress, &localAddress);   
      BASSERT(SUCCEEDED(hr));
      mDirectGameSocketConnectTimer = timeGetTime();
   }

   // if address and translatedAddress don't match, then we try both
   if (
         (remoteAddress.sin_addr.S_un.S_addr != translatedRemoteAddress.sin_addr.S_un.S_addr) ||
         (remoteAddress.sin_port != translatedRemoteAddress.sin_port) ||
         (localAddress.sin_addr.S_un.S_addr != translatedLocalAddress.sin_addr.S_un.S_addr) ||
         (localAddress.sin_port != translatedLocalAddress.sin_port)
      )
   {
      mTranslatedGameSocket = new BSocksGameSocket(this, translatedLocalAddress);
      mTranslatedGameSocket->setObserver(this);
      hr = mTranslatedGameSocket->connect(&translatedRemoteAddress, &localAddress);         
      BASSERT(SUCCEEDED(hr));
      mTranslatedGameSocketConnectTimer = timeGetTime();
   }

   mPrimary = primary;
   mState = cConnecting;
   memcpy(&mRemoteAddress, &remoteAddress, sizeof(SOCKADDR_IN));
   memcpy(&mTranslatedRemoteAddress, &translatedRemoteAddress, sizeof(SOCKADDR_IN));

#ifdef XBOX
   mTimerInterval = cConnectTimerInterval;
   mTimerLastTick = GetTickCount();
#else
   SetTimer(mWindow, cConnectTimer, cConnectTimerInterval, 0);
#endif   

   mConnectTimer = timeGetTime();

   return S_OK;
}

//==============================================================================
//
void BReliableConnection::disconnect(void)
{
   if (mDisableNotifications)
      return;

   nlog(cReliableConnNL, "BReliableConnection::disconnect");
   nlog(cPerfNL, "BReliableConnection::disconnect");
   
#ifdef XBOX   
   mTimerInterval = 0;
   mTimerLastTick = 0;
#else
   KillTimer(mWindow, cConnectTimer);   
#endif
   
   mState = cDisconnected;
   mConnectedSocket = 0;
   destroyUnconnectedSockets();
   if (mpObserver)
      mpObserver->disconnected(this);

   memset(&mRemoteAddress, 0, sizeof(SOCKADDR_IN));
   memset(&mTranslatedRemoteAddress, 0, sizeof(SOCKADDR_IN));
}

//==============================================================================
//
void BReliableConnection::recvd (
   IN BSocket * Socket,
   IN const void * Buffer,
   IN DWORD Length,
   IN DWORD voiceLength,
   IN CONST SOCKADDR_IN * RemoteAddress)
{
   // if we get any data, and we're connecting, then it's the connect packet
   if (Length == sizeof(connectSignature) && Buffer && (memcmp(Buffer, connectSignature, sizeof(connectSignature)) == 0))
   {
      if (mState == cConnecting)
      {
         nlog(cReliableConnNL, "BReliableConnection::recvd -- while connecting");
         if (!mPrimary)
         {
            // else host is pinging us with connect packet, so ping him back
            mConnectedSocket = Socket;

            nlog(cReliableConnNL, "BReliableConnection::recvd -- connected mConnectedSocket[%p], sending c back to primary", this);
            nlog(cPerfNL, "BReliableConnection::recvd -- [%p] is connected", this);

            //nlog(cReliableConnNL, "BReliableConnection::recvd mConnectedSocket[%p] - %s", mConnectedSocket, getSocketName(mConnectedSocket));

            // disconnect the remaining sockets
            destroyUnconnectedSockets();
         }

         BASSERT(mConnectedSocket);

         mState = cConnected;
#ifdef XBOX
         mTimerInterval = 0;
         mTimerLastTick = 0;
#else
         KillTimer(mWindow, cConnectTimer);
#endif
         if (mpObserver)
            mpObserver->connected(this);
      }

      if (!mPrimary && mConnectedSocket)
      {
         // ping host back
         BSendBuffer *buf;
         mConnectedSocket->sendAllocateBuffer(sizeof(connectSignature), &buf);
         memcpy(buf->Buffer, connectSignature, sizeof(connectSignature)); // connect message
         buf->Length = sizeof(connectSignature);
         mConnectedSocket->send(buf);
      }

      return;
   }

   if (mState != cConnected)
   {
      nlog(cReliableConnNL, "BReliableConnection::recvd -- mState != cConnected so data just hit the floor from %s:d.", inet_ntoa(RemoteAddress->sin_addr), htons(RemoteAddress->sin_port));
      return;
   }

   BASSERT(Socket == mConnectedSocket);

   if (mpObserver)
      mpObserver->dataReceived(this, Buffer, Length, RemoteAddress);
}
           
//==============================================================================
//
void BReliableConnection::disconnected (
   IN BSocket * Socket,
   IN DWORD Status)
{
   Status;
               
   if ((Socket == mConnectedSocket) && mpObserver)   
      disconnect();
   else
   {
      if (mDirectGameSocket == Socket)
         mDirectGameSocket = 0;
      if (mTranslatedGameSocket == Socket)      
         mTranslatedGameSocket = 0;

      Socket->dispose();
   }
}

//==============================================================================
//
bool BReliableConnection::notifyUnresponsiveSocket (
         IN BSocket * Socket,
         DWORD lastRecvTime)
{
   if ((Socket == mConnectedSocket) && mpObserver)   
      return mpObserver->notifyUnresponsiveConnection(this, lastRecvTime);
   else
      return true;
}

//==============================================================================
//
bool BReliableConnection::notifyResponsiveSocket (
   IN BSocket * Socket,
   DWORD lastRecvTime)
{
   if ((Socket == mConnectedSocket) && mpObserver)   
      return mpObserver->notifyResponsiveConnection(this, lastRecvTime);
   else
      return true;
}

//==============================================================================
//
BSocket *BReliableConnection::getBestConnectedSocket(void)
{
   if (mDirectGameSocket && mDirectGameSocket->isConnected())
      return mDirectGameSocket;
   /*else if (mDirectGameSocketConnectTimer && (timeGetTime() - mDirectGameSocketConnectTimer < (DWORD)cDirectGameSocketConnectTimeout))
      return 0;*/ // DirectGameSocket and TranslatedGameSocket are either/or, so we don't have to wait between them
   else if (mTranslatedGameSocket && mTranslatedGameSocket->isConnected())
      return mTranslatedGameSocket;
   else if (mTranslatedGameSocketConnectTimer && (timeGetTime() - mTranslatedGameSocketConnectTimer < cTranslatedGameSocketConnectTimeout))
      return 0;
   return 0;
}

//==============================================================================
//
const char *BReliableConnection::getSocketName(BSocket *socket)
{
   static const char sUnknown[] = "Unknown";
   static const char sDirectGameSocket[] = "mDirectGameSocket";
   static const char sTranslatedGameSocket[] = "mTranslatedGameSocket";

   if (socket == mDirectGameSocket)
      return(sDirectGameSocket);
   else if (socket == mTranslatedGameSocket)
      return(sTranslatedGameSocket);

   return(sUnknown);
}

//==============================================================================
//
void BReliableConnection::tic (DWORD timerID)
{
   if (mInServiceCB)
      return;

   mInServiceCB = true;

   timerID;
   if (timerID == cConnectTimer)
   {
      if (mState != cConnecting)
      {
#ifdef XBOX      
         mTimerInterval = 0;
         mTimerLastTick = 0;
#else
         KillTimer(mWindow, cConnectTimer);
#endif         
         mInServiceCB = false;
         return;
      }
      //else if (mConnectTimer && (timeGetTime()-mConnectTimer) > cConnectTimeout)
      else if (mConnectTimer && (timeGetTime()-mConnectTimer) > 30*60*1000)
      {
         if (mpObserver)
            mpObserver->connectTimeout(this);

         disconnect();
         mInServiceCB = false;
         return;
      }

      if (mPrimary && (mConnectedSocket==0))
      {
         mConnectedSocket = getBestConnectedSocket();
         if (mConnectedSocket)
         {
            nlog(cReliableConnNL, "BReliableConnection::tic -- mConnectedSocket[%p] - %s", mConnectedSocket, getSocketName(mConnectedSocket));
            nlog(cReliableConnNL, "BReliableConnection::tic -- is connected, mConnectedSocket[%p] - %s", mConnectedSocket, getSocketName(mConnectedSocket));

            nlog(cPerfNL, "BReliableConnection::tic -- [%p] is connected", this);

            destroyUnconnectedSockets();
         }
      }

      if (mConnectedSocket && (mState == cConnecting))
      {
         nlog(cReliableConnNL, "BReliableConnection::tic -- sending a c to non primary side");
         BSendBuffer *buf;
         mConnectedSocket->sendAllocateBuffer(sizeof(connectSignature), &buf);
         memcpy(buf->Buffer, connectSignature, sizeof(connectSignature)); // connect message
         buf->Length = sizeof(connectSignature);
         mConnectedSocket->send(buf);
      }
   }

   mInServiceCB = false;
}

//==============================================================================
//
void BReliableConnection::connected ( IN BSocket *Socket ) 
{  
   UNREFERENCED_PARAMETER(Socket);
   
   nlog(cReliableConnNL, "BReliableConnection::connected -- socket[%p]", Socket);
   nlog(cPerfNL, "BReliableConnection::connected -- socket[%p]", Socket);
}

#ifndef XBOX
//==============================================================================
//
// static
LRESULT BReliableConnection::WindowProcedure (
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

   BReliableConnection * Instance = reinterpret_cast <BReliableConnection *> (GetWindowLong (Window, GWL_USERDATA));
   if (Instance == NULL)
   {
      return DefWindowProc (Window, Message, Parameter1, Parameter2);
   }


   switch (Message)
   {
   case WM_TIMER:
      Instance -> tic ( Parameter1 );
      break;

   default:
      return Instance -> onWindowMessage (Window, Message, Parameter1, Parameter2);
   }

   return 0;
}

//==============================================================================
//
LRESULT BReliableConnection::onWindowMessage (
   IN HWND Window,
   IN UINT Message,
   IN WPARAM Parameter1,
   IN LPARAM Parameter2)
{
   return DefWindowProc (Window, Message, Parameter1, Parameter2);
}
#endif

//==============================================================================
// BReliableConnection::
//==============================================================================



//==============================================================================
// eof: ReliableConnection.cpp
//==============================================================================
