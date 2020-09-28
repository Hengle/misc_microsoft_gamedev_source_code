//============================================================================
// File: debugConnection.h
//============================================================================
#pragma once
#include "containers\queue.h"
#include "threading\eventDispatcher.h"
#include "threading\lockFreeQueue.h"
#include "threading\win32WaitableTimer.h"
#include "xfs.h"
#ifndef BUILD_FINAL
   #ifdef XBOX
   #include <xbdm.h>
   #endif // XBOX
#endif

enum eCommands;

//============================================================================
// class BDebugConnection
//============================================================================
class BDebugConnection : public BEventReceiver
{
public:
   BDebugConnection();
   ~BDebugConnection();

   void init(const char* pServerIP);

   void deinit(void);

   bool sendOutput(const char* pMsg);

   // true if a command is available
   bool receiveCommand(BString& command);
   
   enum eState
   {
      cStateInvalid,
      cStateConnecting,
      cStateConnected,
      cStateFailed
   };
   
   eState getState(void) const { return mState; }

private:
   BEventReceiverHandle mSimEventHandle;
   eState mState;
   volatile LONG mShuttingDown;
   
   BAllocFixed mPayloadAllocator;
   
   SOCKET mSocket;
   
   BWin32Event mSocketEvent;
   
   uint mSendBufferLen;
   enum { cSendBufferSize = 1024 };
   BYTE mSendBuffer[cSendBufferSize];
   
   uint mRecvBufferLen;
   enum { cRecvBufferSize = 1024 };
   BYTE mRecvBuffer[cRecvBufferSize];
      
   uint mRecvPacketOfs;
   uint mRecvPacketLen;
         
   WSAOVERLAPPED mRecvOverlapped;

#ifndef BUILD_FINAL
   #ifdef XBOX
   PDMN_SESSION mpDMSession;
   #endif // XBOX
#endif   

   enum cEventClass
   {
      cECInit = cEventClassFirstUser,
      cECDeinit,
      cECSendOutput,
      cECReceiveSocketData,
      
      cECReceiveCommand,
      cECUpdateState,
      
      cECDebugText
   };
   
   enum { cTextBufSize = 256 };
   
   class BDebugConnectionPayload : public BEventPayload
   {
   public:
      BDebugConnectionPayload() : mpPayloadAllocator(NULL)
      { 
      }
      
      void setAllocator(BAllocFixed* pPayloadAllocator) 
      {
         mpPayloadAllocator = pPayloadAllocator;
      }
            
      void setString(const char* pStr) { mBuf.set(pStr); }
      const char* getString(void) const { return mBuf.c_str(); }
      
//      void setID(uint id) { mID = id; }
      
      virtual void deleteThis(bool delivered)
      {
         if (mpPayloadAllocator)
         {
//            trace("unlocking 0x%08X, 0x%08X", this, mID);
            mpPayloadAllocator->unlock(this);
         }
//         mID |= 0xFE000000;
      }

   private:
      BFixedString<cTextBufSize> mBuf;
      BAllocFixed* mpPayloadAllocator;
//      uint mID;
      
      BDebugConnectionPayload(const BDebugConnectionPayload&);
      BDebugConnectionPayload& operator= (const BDebugConnectionPayload&);
      
      virtual ~BDebugConnectionPayload() { }
   };
   
   enum { cCommandQueueSize = 256 };
   BQueue<BDebugConnectionPayload*> mCommandQueue;
      
   enum { cNotifyTextQueueSize = 256 };
   typedef BLockFreeQueue<BFixedString128> BStringQueue;
   BStringQueue* mpNotifyTextQueue;
   
   BWin32WaitableTimer mNotifyTextTimer;
   
   BDebugConnectionPayload* constructPayload(const char* pStr);
   
   bool processInit(const BEvent& event);
   void processDeinit(const BEvent& event);
   bool sendHelloPacket(void);
   bool sendOutputPacket(const char* pStr);
   bool processSendOutput(const BDebugConnectionPayload* pPayload);
   void closeConnection(void);
   bool processReceiveSocketData(const BEvent& event);
   
   bool processPacket(eCommands func);
   bool emptyRecvBuffer(void);
   bool initiateOverlappedRecv(void);
   
   bool recvData(BYTE* pBuffer, int offset, int size);
   bool recvBYTE(BYTE& data);
   bool recvWORD(WORD& data);
   bool recvDWORD(DWORD& data);
   bool receiveString(char* pBuf, uint bufLen);
   
   void sendBegin(BYTE func);
   void sendAddBYTE(BYTE val);
   void sendAddWORD(WORD val);
   void sendAddDWORD(DWORD val);
   void sendAddString(const char* str);
   bool sendEnd(void);
   
   static DWORD debugStrHandler(ULONG dwNotification, DWORD dwParam);
         
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

extern BDebugConnection gDebugConnection;