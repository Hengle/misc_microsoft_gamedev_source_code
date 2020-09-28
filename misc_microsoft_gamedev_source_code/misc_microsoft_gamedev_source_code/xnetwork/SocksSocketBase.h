#pragma once

#include "Socket.h"

class BSocksSocketBase :
   public BSocket
{
public:
   virtual HRESULT dispose(void);

#ifdef XBOX   
   static void tickActiveSockets(void);
#endif   

protected:

   virtual ~BSocksSocketBase (void);
   SOCKET      mSocket;
   //
   // Used for asynchronous message routing (no UI).
   //
protected:

#ifndef XBOX
   HWND        mWindow;
#endif   

protected:

#ifndef XBOX
   enum
   {
      SOCKS_WM_NETWORK_EVENT = WM_USER,    // for use with WSAAsyncSelect
      SOCKS_WM_RESOLUTION_COMPLETE
   };
#endif   

   BSocksSocketBase (
      IN BSocket::BObserver * Observer);  

#ifndef XBOX
   friend class BSocksWindowClass;
   static LRESULT WINAPI WindowProcedure (
      IN HWND Window,
      IN UINT Message,
      IN WPARAM Parameter1,
      IN LPARAM Parameter2);
#endif      

   //
   // Creates the message routing window (if necessary)
   //
   HRESULT asyncSelect (
      IN DWORD EventMask);

   //
   // Destroys the message routing window.
   //
   void asyncStop (void);

   virtual void networkEvent (
      IN DWORD Event,
      IN DWORD Status);

   virtual BOOL recvReady (void);
   virtual void sendReady (void);

   HRESULT startTimer ( DWORD timerID, DWORD interval );

   void stopTimer ( DWORD timerID );

   virtual void tic (DWORD timerID);
   virtual void networkClose (IN HRESULT Result);

#ifndef XBOX
   virtual LRESULT onWindowMessage (
      IN HWND Window,
      IN UINT Message,
      IN WPARAM Parameter1,
      IN LPARAM Parameter2);
#endif      

#if 0
   virtual void resolveComplete (
      IN DWORD BufferLength,
      IN DWORD Status);
#endif

#ifdef XBOX
   static int mNumActiveSockets;
   enum { MaxActiveSockets = 32 };
   static BSocksSocketBase* mpActiveSockets[MaxActiveSockets];
         
   DWORD mEventMask;
   
   struct BTimer
   {
      DWORD mInterval;
      DWORD mLastTick;   
   };
    
   enum { MaxTimers = 2 };
   BTimer mTimers[MaxTimers];
   
   WSAEVENT mEventHandle;
   bool mSendMessages;

   void checkForEvents( bool tickTimers = true);
   void removeFromActiveSockets(void);
#endif

};

#ifndef XBOX
class BSocksWindowClass
{
private:

   ATOM m_ClassAtom;

public:

   BSocksWindowClass (void);
   ~BSocksWindowClass (void);  

   ATOM GetClassAtom (void) const;
};
#endif