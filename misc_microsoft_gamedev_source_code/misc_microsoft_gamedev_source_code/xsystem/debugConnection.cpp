//============================================================================
// File: debugConnection.cpp
//============================================================================
#include "xsystem.h"
#include "debugConnection.h"
#include "threading\interlocked.h"
#ifdef XBOX
   #include "xbdm.h"
#endif // XBOX

//============================================================================
// Globals
//============================================================================
BDebugConnection gDebugConnection;

//============================================================================
// Constants
//============================================================================
static const WORD cConnectionPort = 1001;

static const BYTE cCommandPrefix = '@';
static const BYTE cCommandSuffix = '#';

enum eCommands
{
   // Send to server
   cFuncHello,
   cFuncOutput,   
   cFuncGoodbye,
      
   // Receive from server
   cFuncCommand   
};

//============================================================================
// BDebugConnection::BDebugConnection
//============================================================================
BDebugConnection::BDebugConnection() :
   BEventReceiver(),
   mSimEventHandle(cInvalidEventReceiverHandle),
   mSocket(INVALID_SOCKET),
   mShuttingDown(FALSE),
   mCommandQueue(cCommandQueueSize),
   mpNotifyTextQueue(NULL),
   mState(cStateInvalid),
   mSendBufferLen(0),
   mRecvBufferLen(0),
   mRecvPacketOfs(0),
   mRecvPacketLen(0)
#ifndef BUILD_FINAL
   #ifdef XBOX
   ,mpDMSession(NULL)
   #endif // XBOX
#endif
{
   Utils::ClearObj(mRecvOverlapped);
   
   mPayloadAllocator.init(sizeof(BDebugConnectionPayload), 128, 128, true);
}

//============================================================================
// BDebugConnection::~BDebugConnection
//============================================================================
BDebugConnection::~BDebugConnection()
{
}

//============================================================================
// BDebugConnection::~BDebugConnection
//============================================================================
DWORD BDebugConnection::debugStrHandler(ULONG dwNotification, DWORD dwParam)
{
#ifdef XBOX
   if ((dwNotification != DM_DEBUGSTR) || (!dwParam))
      return TRUE;
   
   // You cannot set a breakpoint in here!
   // DO NOT do anything fancy here!
   // This code may be called from the context of the Guide, and from any thread!
//-- FIXING PREFIX BUG ID 411
   const PDMN_DEBUGSTR p = (PDMN_DEBUGSTR)dwParam;
//--

   if ((p->String) && (p->Length) && (gDebugConnection.mpNotifyTextQueue))
   {
      BFixedString128 buf;
      buf.setBuf(p->String, p->Length);
      gDebugConnection.mpNotifyTextQueue->enqueue(buf);
   }
#endif // XBOX
   return TRUE;
}   
   
//============================================================================
// BDebugConnection::init
//============================================================================
void BDebugConnection::init(const char* pServerIP)
{
   BDEBUG_ASSERT(pServerIP);
   
   deinit();
   
   mSimEventHandle = gEventDispatcher.addClient(this, cThreadIndexSim);
   
   eventReceiverInit(cThreadIndexMisc);
         
   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = mEventHandle;
   handleEvent.mToHandle = mEventHandle;
   handleEvent.mEventClass = cECDebugText;
   gEventDispatcher.registerHandleWithEvent(mNotifyTextTimer.getHandle(), handleEvent);
   
   mNotifyTextTimer.set(200);
         
   BDebugConnectionPayload* pPayload = constructPayload(pServerIP);
   BVERIFY(pPayload);
   
   gEventDispatcher.send(mEventHandle, mEventHandle, cECInit, 0, 0, pPayload);
   
   mState = cStateConnecting;
}

//============================================================================
// BDebugConnection::deinit
//============================================================================
void BDebugConnection::deinit(void)
{
   if (mEventHandle == cInvalidEventReceiverHandle)
      return;
                  
   if (gEventDispatcher.getThreadId(cThreadIndexMisc) == 0)      
      return;
   
   Sync::InterlockedExchangeExport(&mShuttingDown, TRUE);
   
   mNotifyTextTimer.cancel();   
   
   gEventDispatcher.sleep(250);
      
   gEventDispatcher.deregisterHandle(mNotifyTextTimer.getHandle(), cThreadIndexMisc);
            
   gEventDispatcher.send(mEventHandle, mEventHandle, cECDeinit, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);
   
   eventReceiverDeinit();
   
   if (cInvalidEventReceiverHandle != mSimEventHandle)
   {
      gEventDispatcher.removeClientImmediate(mSimEventHandle);
      
      mSimEventHandle = cInvalidEventReceiverHandle;
   }
         
   if (mpNotifyTextQueue)
   {
      BAlignedAlloc::Delete(mpNotifyTextQueue, gPrimaryHeap);
      mpNotifyTextQueue = NULL;
   }
            
   mCommandQueue.clear();
   
   gEventDispatcher.sleep(250);
   
   mPayloadAllocator.unlockAll();
   
   Sync::InterlockedExchangeExport(&mShuttingDown, FALSE);
}

//============================================================================
// BDebugConnection::sendOutput
//============================================================================
bool BDebugConnection::sendOutput(const char* pMsg)
{
   if ((mShuttingDown) || (mEventHandle == cInvalidEventReceiverHandle))
      return false;

   if (gEventDispatcher.getThreadId(cThreadIndexMisc) == 0)      
      return false;
      
   if (!pMsg)
      return false;
      
   BDebugConnectionPayload* pPayload = constructPayload(pMsg);
   if (!pPayload)
      return false;

   gEventDispatcher.send(mEventHandle, mEventHandle, cECSendOutput, 0, 0, pPayload);
   
   return true;
}

//============================================================================
// BDebugConnection::receiveCommand
//============================================================================
bool BDebugConnection::receiveCommand(BString& command)
{
   if ((mShuttingDown) || (mEventHandle == cInvalidEventReceiverHandle))
      return false;
      
   ASSERT_THREAD(cThreadIndexSim);
      
   BDebugConnectionPayload* pPayload = NULL;      
   if (0 == mCommandQueue.popFront(1, &pPayload))
      return false;
   
   command.set(pPayload->getString());
   
   pPayload->deleteThis(true);
   
   return true;
}

// static volatile LONG gNextPayloadID;

//============================================================================
// BDebugConnection::constructPayload
//============================================================================
BDebugConnection::BDebugConnectionPayload* BDebugConnection::constructPayload(const char* pStr)
{
   // DO NOT do anything fancy here!
   if (mShuttingDown)
      return NULL;
      
   BDebugConnectionPayload* p = (BDebugConnectionPayload*)mPayloadAllocator.lock();
   if (!p)
      return NULL;
   Utils::ConstructInPlace(p);
   p->setAllocator(&mPayloadAllocator);
   p->setString(pStr);
   
//   uint id = InterlockedIncrement(&gNextPayloadID);         
//   p->setID(id);
//   trace("constructing 0x%08X, 0x%08X", p, id);
   
   return p;
}

//============================================================================
// BDebugConnection::processInit
//============================================================================
bool BDebugConnection::processInit(const BEvent& event)
{
   SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (s == INVALID_SOCKET)
      return false;

   SOCKADDR_IN sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_port = htons(cConnectionPort);
   sa.sin_addr.s_addr = inet_addr(((BDebugConnectionPayload*)event.mpPayload)->getString());

   if (connect(s, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR)
   {
      int error = WSAGetLastError();
      error;
//      trace("BDebugConnection::processInit: connect error: %d", error);
      closesocket(s);
      return false;
   }
   
   mSocketEvent.reset();
   
   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = mEventHandle;
   handleEvent.mToHandle = mEventHandle;
   handleEvent.mEventClass = cECReceiveSocketData;
   gEventDispatcher.registerHandleWithEvent(mSocketEvent, handleEvent);
                  
   if (mSocket != INVALID_SOCKET)      
      closesocket(mSocket);
      
   mSocket = s;
   
   mRecvBufferLen = 0;
   mRecvPacketOfs = 0;
   mRecvPacketLen = 0;
      
   if (!initiateOverlappedRecv())
   {
      closeConnection();
      return false;
   }
         
   if (!sendHelloPacket())
   {
      closeConnection();
      return false;
   }
   
   return true;
}

//============================================================================
// BDebugConnection::sendHelloPacket
//============================================================================
bool BDebugConnection::sendHelloPacket(void)
{
   char buf[256];

#ifdef BUILD_FINAL
   strcpy_s(buf, sizeof(buf), "?");
#else   
   DWORD size = sizeof(buf);
   #ifdef XBOX
   HRESULT hres = DmGetXboxName(buf, &size);
   if (FAILED(hres))
      strcpy_s(buf, sizeof(buf), "?");
   #endif // XBOX
#endif      

   sendBegin(cFuncHello);

   sendAddString(buf);

   return sendEnd();
}   

//============================================================================
// BDebugConnection::sendOutputPacket
//============================================================================
bool BDebugConnection::sendOutputPacket(const char* pStr)
{
   BDEBUG_ASSERT(pStr);

   if (mSocket == INVALID_SOCKET)
      return false;

   sendBegin(cFuncOutput);

   sendAddString(pStr);

   return sendEnd();
}

//============================================================================
// BDebugConnection::processDeinit
//============================================================================
void BDebugConnection::processDeinit(const BEvent& event)
{
   gEventDispatcher.deregisterHandle(mSocketEvent, cThreadIndexMisc);
   
   if (mSocket != INVALID_SOCKET)
   {
      sendBegin(cFuncGoodbye);
      sendEnd();
   
      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
   }
}

//============================================================================
// BDebugConnection::processSendOutput
//============================================================================
bool BDebugConnection::processSendOutput(const BDebugConnectionPayload* pPayload)
{
   return sendOutputPacket(pPayload->getString());
}

//============================================================================
// BDebugConnection::closeConnection
//============================================================================
void BDebugConnection::closeConnection(void)
{
//   trace("BDebugConnection::closeConnection");
   
   if (mSocket == INVALID_SOCKET)
      return;
      
   gEventDispatcher.send(mEventHandle, mSimEventHandle, cECUpdateState, cStateFailed);         
   
   gEventDispatcher.deregisterHandle(mSocketEvent, cThreadIndexMisc);
   
   if (mSocket != INVALID_SOCKET)
   {
      closesocket(mSocket);
      mSocket = INVALID_SOCKET;
   }
}

//============================================================================
// BDebugConnection::processPacket
//============================================================================
bool BDebugConnection::processPacket(eCommands func)
{
   switch (func)
   {
      case cFuncCommand:
      {
         BFixedString<1024> buf;
         if (!receiveString(buf.getPtr(), buf.getBufSize()))
            return false;

         BDebugConnectionPayload* pPayload = constructPayload(buf.getPtr());
                                    
         if (pPayload)
            gEventDispatcher.send(mEventHandle, mSimEventHandle, cECReceiveCommand, 0, 0, pPayload);

         break;
      }
      default:
      {
         return false;
      }
   }  
   
   return true;    
}

//============================================================================
// BDebugConnection::emptyRecvBuffer
//============================================================================
bool BDebugConnection::emptyRecvBuffer(void)
{
   uint curOfs = 0;
   
   while ((mRecvBufferLen - curOfs) > 4)
   {
      const uint prefix = mRecvBuffer[curOfs];
      const eCommands func = (eCommands)mRecvBuffer[curOfs + 1];
      const uint len = mRecvBuffer[curOfs + 2] | (mRecvBuffer[curOfs + 3] << 8);
      
      if (prefix != cCommandPrefix)
         return false;
      
      if ((mRecvBufferLen - curOfs) >= len)
      {
         mRecvPacketOfs = curOfs + 4;
         mRecvPacketLen = len - 4;
         
         if (!processPacket(func))
            return false;
         
         BYTE suffix;
         if (!recvBYTE(suffix))
            return false;
         
         if (suffix != cCommandSuffix)
            return false;
                     
         curOfs += len;
      }
      else
         break;
   }
   
   if (curOfs > 0)
   {
      const uint bytesLeft = mRecvBufferLen - curOfs;
      if (bytesLeft)
         memmove(mRecvBuffer, mRecvBuffer + curOfs, bytesLeft);
      mRecvBufferLen = bytesLeft;         
   }
   
   return true;
}

//============================================================================
// BDebugConnection::initiateOverlappedRecv
//============================================================================
bool BDebugConnection::initiateOverlappedRecv(void)
{
   for ( ; ; )
   {
      Utils::ClearObj(mRecvOverlapped);
      mRecvOverlapped.hEvent = mSocketEvent.getHandle();

      WSABUF buffer;
      buffer.len = cRecvBufferSize - mRecvBufferLen;
      buffer.buf = (char*)mRecvBuffer + mRecvBufferLen;

      DWORD recvBytesRead = 0;
      DWORD recvFlags = 0;

      int result = WSARecv(mSocket, &buffer, 1, &recvBytesRead, &recvFlags, &mRecvOverlapped, NULL);

      if (result == SOCKET_ERROR)
      {
         if (GetLastError() == WSA_IO_PENDING)
            break;
         return false;
      }
      
      if (!recvBytesRead)
      {
         // Connection was gracefully closed.
         return false;
      }

      mRecvBufferLen += recvBytesRead;

      if (!emptyRecvBuffer())
         return false;
   }      

   return true;      
}


//============================================================================
// BDebugConnection::processReceiveSocketData
//============================================================================
bool BDebugConnection::processReceiveSocketData(const BEvent& event)
{
   if (mSocket == INVALID_SOCKET)
      return false;

   DWORD bytesTransferred = 0;
   const BOOL result = GetOverlappedResult((HANDLE)mSocket, &mRecvOverlapped, &bytesTransferred, FALSE);
   
   if (!result)
   {
      if (GetLastError() == ERROR_IO_INCOMPLETE)
         return true;
      
      return false;
   }
   
   mRecvBufferLen += bytesTransferred;
   BDEBUG_ASSERT(mRecvBufferLen <= cRecvBufferSize);
   
   if (!emptyRecvBuffer())
      return false;
      
   if (!initiateOverlappedRecv())
      return false;
   
   return true;
}

//============================================================================
// BDebugConnection::receiveEvent
//============================================================================
bool BDebugConnection::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (threadIndex)
   {
      case cThreadIndexSim:
      {
         switch (event.mEventClass)
         {
            case cECReceiveCommand:
            {
               if (mCommandQueue.pushBack((BDebugConnectionPayload*)event.mpPayload))
                  return true;
               break;
            }
            case cECUpdateState:
            {
               mState = (eState)event.mPrivateData;
               break;
            }
         }
         break;
      }
      case cThreadIndexMisc:
      {
         switch (event.mEventClass)
         {
            case cEventClassThreadIsTerminating:
            {
               closeConnection();
               
               gEventDispatcher.removeClientImmediate(mEventHandle);
               mEventHandle = cInvalidEventReceiverHandle;
               
               break;
            }
            case cECInit:
            {
               if (processInit(event))
                  gEventDispatcher.send(mEventHandle, mSimEventHandle, cECUpdateState, cStateConnected);
               else
                  gEventDispatcher.send(mEventHandle, mSimEventHandle, cECUpdateState, cStateFailed);
               break;
            }
            case cECDeinit:
            {
               processDeinit(event);
               gEventDispatcher.send(mEventHandle, mSimEventHandle, cECUpdateState, cStateInvalid);
               break;
            }
            case cECSendOutput:
            {
               BDebugConnectionPayload* pPayload = (BDebugConnectionPayload*)event.mpPayload;
               if (!processSendOutput(pPayload))
                  closeConnection();
               break;
            }
            case cECReceiveSocketData:
            {  
               if (!processReceiveSocketData(event))
                  closeConnection();
               break;
            }
            case cECDebugText:
            {
               for ( ; ; )
               {
                  if ((mShuttingDown) || (!mpNotifyTextQueue))
                     break;
                                                         
                  BFixedString128 str;
                  const BOOL result = mpNotifyTextQueue->dequeue(str);
                  if (!result)
                     break;
                  
                  if (!sendOutputPacket(str))
                     closeConnection();
               }
               
               break;
            }
         }
         break;
      }
   }
   
   return false;
}

//============================================================================
// BDebugConnection::recvData
//============================================================================
bool BDebugConnection::recvData(BYTE* pBuffer, int offset, int size)
{
   if (size > (int)mRecvPacketLen)
      return false;
   
   memcpy(pBuffer, mRecvBuffer + mRecvPacketOfs, size);
   mRecvPacketLen -= size;
   mRecvPacketOfs += size;
   
   return true;
}

//============================================================================
// BDebugConnection::recvBYTE
//============================================================================
bool BDebugConnection::recvBYTE(BYTE& data)
{
   if(!recvData(&data, 0, sizeof(data)))
      return false;
   return true;
}

//============================================================================
// BDebugConnection::recvWORD
//============================================================================
bool BDebugConnection::recvWORD(WORD& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchWords(&data, 1);
#endif
   return true;
}

//============================================================================
// BDebugConnection::recvDWORD
//============================================================================
bool BDebugConnection::recvDWORD(DWORD& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchDWords(&data, 1);
#endif
   return true;
}

//============================================================================
// BDebugConnection::receiveString
//============================================================================
bool BDebugConnection::receiveString(char* pBuf, uint bufLen)
{
   WORD len;
   if (!recvWORD(len))
      return false;
   
   if (len >= bufLen)
      return false;

   if (!recvData((BYTE*)pBuf, 0, len))
      return false;
    
   pBuf[len] = '\0';
   return true;
}

//============================================================================
// BDebugConnection::sendBegin
//============================================================================
void BDebugConnection::sendBegin(BYTE func)
{
   mSendBufferLen = 0;
   sendAddBYTE(cCommandPrefix);
   sendAddBYTE(func);
   sendAddWORD(0);
}

//============================================================================
// BDebugConnection::sendAddBYTE
//============================================================================
void BDebugConnection::sendAddBYTE(BYTE val)
{
   BDEBUG_ASSERT(mSendBufferLen + sizeof(BYTE) <= cSendBufferSize);
   mSendBuffer[mSendBufferLen] = val;
   mSendBufferLen++;
}

//============================================================================
// BDebugConnection::sendAddWORD
//============================================================================
void BDebugConnection::sendAddWORD(WORD val)
{
   BDEBUG_ASSERT(mSendBufferLen + sizeof(WORD) <= cSendBufferSize);
   const BYTE* const ptr=reinterpret_cast<const BYTE*>(&val);
#ifdef XBOX
   mSendBuffer[mSendBufferLen] = ptr[1];
   mSendBuffer[mSendBufferLen+1] = ptr[0];
#else
   mSendBuffer[mSendBufferLen] = ptr[0];
   mSendBuffer[mSendBufferLen+1] = ptr[1];
#endif
   mSendBufferLen += 2;
}

//============================================================================
// BDebugConnection::sendAddDWORD
//============================================================================
void BDebugConnection::sendAddDWORD(DWORD val)
{
   BDEBUG_ASSERT(mSendBufferLen + sizeof(DWORD) <= cSendBufferSize);
   const BYTE* const ptr=reinterpret_cast<const BYTE*>(&val);
#ifdef XBOX
   mSendBuffer[mSendBufferLen] = ptr[3];
   mSendBuffer[mSendBufferLen+1] = ptr[2];
   mSendBuffer[mSendBufferLen+2] = ptr[1];
   mSendBuffer[mSendBufferLen+3] = ptr[0];
#else
   mSendBuffer[mSendBufferLen] = ptr[0];
   mSendBuffer[mSendBufferLen+1] = ptr[1];
   mSendBuffer[mSendBufferLen+2] = ptr[2];
   mSendBuffer[mSendBufferLen+3] = ptr[3];
#endif
   mSendBufferLen += 4;
}

//============================================================================
// BDebugConnection::sendAddString
//============================================================================
void BDebugConnection::sendAddString(const char* str)
{
   WORD len=0;
   len=(WORD)strlen(str);
   BDEBUG_ASSERT(mSendBufferLen + sizeof(WORD) + len <= cSendBufferSize);
   sendAddWORD(len);
   BYTE* bufferPtr = mSendBuffer + mSendBufferLen;
   for(int i = 0; i < len; i++)
      bufferPtr[i] = str[i];
   mSendBufferLen += len;
}

//============================================================================
// BDebugConnection::sendEnd
//============================================================================
bool BDebugConnection::sendEnd(void)
{
   sendAddBYTE(cCommandSuffix);
   BDEBUG_ASSERT(mSendBufferLen >= 5);
   mSendBuffer[2] = (BYTE)(mSendBufferLen & 0xFF);
   mSendBuffer[3] = (BYTE)(mSendBufferLen >> 8); 
   int retval=send(mSocket, reinterpret_cast<const char*>(mSendBuffer), mSendBufferLen, 0);
   if(retval==0 || retval==SOCKET_ERROR)
   {
//      trace("socket send error %d", WSAGetLastError());
      return false;
   }
   mSendBufferLen = 0;
   return true;
}
