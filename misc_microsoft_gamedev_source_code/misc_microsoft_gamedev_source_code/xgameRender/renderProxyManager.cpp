//============================================================================
//
// File: renderProxyManager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
// Experimental/Unused
//============================================================================
#include "xgameRender.h"
#include "renderProxyManager.h"

//============================================================================
// Globals
//============================================================================
BRenderProxyManager gRenderProxyManager;

//============================================================================
// BRenderProxyManager::BRenderProxyManager
//============================================================================
BRenderProxyManager::BRenderProxyManager() :
   BEventReceiver(),
   mInitialized(false)
{
}

//============================================================================
// BRenderProxyManager::~BRenderProxyManager
//============================================================================
BRenderProxyManager::~BRenderProxyManager()
{
}

//============================================================================
// BRenderProxyManager::init
//============================================================================
void BRenderProxyManager::init(void)
{
   ASSERT_MAIN_THREAD
   if (mInitialized)
      return;
   
   eventReceiverInit();
   commandListenerInit();
   
   mInitialized = true;
}

//============================================================================
// BRenderProxyManager::deinit
//============================================================================
void BRenderProxyManager::deinit(void)
{
   ASSERT_MAIN_THREAD
   if (!mInitialized)
      return;
      
   commandListenerDeinit();
   eventReceiverDeinit();
   
   mInitialized = true;
}

//============================================================================
// BRenderProxyManager::beginFrame
//============================================================================
void BRenderProxyManager::beginFrame(void)
{
   BDEBUG_ASSERT(mInitialized);
   ASSERT_MAIN_THREAD
}

//============================================================================
// BRenderProxyManager::add
//============================================================================
void BRenderProxyManager::add(IRenderProxy* pProxy)
{
   BDEBUG_ASSERT(mInitialized && pProxy);
   ASSERT_MAIN_THREAD
   
   serializeProxy(pProxy, IRenderProxy::cSRTAdd, cRCAdd);
}

//============================================================================
// BRenderProxyManager::remove
//============================================================================
void BRenderProxyManager::remove(IRenderProxy* pProxy)
{
   BDEBUG_ASSERT(mInitialized && pProxy);
   ASSERT_MAIN_THREAD
   
   BDEBUG_ASSERT(pProxy->getSimState() == cRPSValid);
   
   pProxy->setSimState(cRPSDeleting);
   
   gRenderThread.submitCommand(*this, cRCRemove, sizeof(IRenderProxy**), &pProxy);
}

//============================================================================
// BRenderProxyManager::tick
//============================================================================
void BRenderProxyManager::tick(IRenderProxy* pProxy)
{
   BDEBUG_ASSERT(mInitialized && pProxy);
   ASSERT_MAIN_THREAD
   
   BDEBUG_ASSERT(pProxy->getSimState() == cRPSValid);
   
   gRenderThread.submitCommand(*this, cRCTick, sizeof(IRenderProxy**), &pProxy);
}

//============================================================================
// BRenderProxyManager::update
//============================================================================
void BRenderProxyManager::update(IRenderProxy* pProxy)
{
   BDEBUG_ASSERT(mInitialized && pProxy);
   ASSERT_MAIN_THREAD

   serializeProxy(pProxy, IRenderProxy::cSRTUpdate, cRCUpdate);
}

//============================================================================
// BRenderProxyManager::serializeProxy
//============================================================================
void BRenderProxyManager::serializeProxy(IRenderProxy* pProxy, IRenderProxy::eSerializeRequestType requestType, DWORD renderCommand)
{
   uint size = 0;
   pProxy->simGetSerializeSize(requestType, size);

   // Extra 2 DWORD's for header, then up to 3 DWORD's for 16 byte alignment.
   const uint totalSize = size + sizeof(DWORD) * 2 + (size ? sizeof(DWORD) * 3 : 0);
   DWORD* pDst = (DWORD*)gRenderThread.submitCommandBegin(*this, renderCommand, totalSize);
   
   pDst[0] = (DWORD)pProxy;
   pDst[1] = size;
   
   if (size)
   {
      // The most DWORD's that can be skipped here are 3.
      DWORD* pAlignedDst = Utils::AlignUp(pDst + sizeof(DWORD) * 2, 16);

      pProxy->simSerializeState(pAlignedDst, size);

      BDEBUG_ASSERT( (((BYTE*)pAlignedDst + size) - (BYTE*)pDst) <= (int)totalSize );
   }

   gRenderThread.submitCommandEnd(totalSize);
}

//============================================================================
// class BCommandPayload
//============================================================================
class BCommandPayload : public BEventPayload
{
public:
   BCommandPayload(const void* pData, uint dataLen) : mDataLen(dataLen)
   {
      if (!dataLen)
         mpData = NULL;
      else
      {
         mpData = BAlignedAlloc::NewArray<uchar>(dataLen, 16, gRenderHeap);
         memcpy(mpData, pData, dataLen);
      }
   }

   virtual ~BCommandPayload()
   {
      BAlignedAlloc::DeleteArray(mpData, gRenderHeap);
   }

   virtual void deleteThis(bool delivered) { BAlignedAlloc::Delete(this, gRenderHeap); }
   
   uint getDataLen(void) const { return mDataLen; }
   uchar* getData(void) const { return mpData; }

private:
   uint mDataLen;
   uchar* mpData;
};

//============================================================================
// BRenderProxyManager::sendCommand
//============================================================================
void BRenderProxyManager::sendCommand(IRenderProxy* pProxy, DWORD command, const void* pData, uint dataLen)
{
   BDEBUG_ASSERT(mInitialized && pProxy);
               
   if (gRenderThread.isRenderThread())
   {
      BDEBUG_ASSERT(pProxy->getRenderState() == cRPSValid);
      
      BCommandPayload* pPayload = NULL;
      if (dataLen)
         pPayload = BAlignedAlloc::New<BCommandPayload>(4, gRenderHeap, pData, dataLen);
      
      gEventDispatcher.send(mEventHandle, mEventHandle, cECCommand, (DWORD)pProxy, command, pPayload);
   }
   else
   {
      ASSERT_MAIN_THREAD
      BDEBUG_ASSERT(pProxy->getSimState() == cRPSValid);
      
      // Extra 3 DWORD's for header, then up to 3 DWORD's for 16 byte alignment.
      const DWORD totalSize = dataLen + sizeof(DWORD) * 3 + (dataLen ? sizeof(DWORD) * 3 : 0);
      DWORD* pDst = (DWORD*)gRenderThread.submitCommandBegin(*this, cRCCommand, totalSize);

      pDst[0] = (DWORD)pProxy;
      pDst[1] = dataLen;
      pDst[2] = command;

      if (dataLen)
      {
         // The most DWORD's that can be skipped here are 3.
         DWORD* pAlignedDst = Utils::AlignUp(pDst + sizeof(DWORD) * 3, 16);
         
         memcpy(pAlignedDst, pData, dataLen);
         
         BDEBUG_ASSERT( (((BYTE*)pAlignedDst + dataLen) - (BYTE*)pDst) <= (int)totalSize );
      }

      gRenderThread.submitCommandEnd(totalSize);
   }
}

//============================================================================
// BRenderProxyManager::clear
//============================================================================
void BRenderProxyManager::clear(void)
{
   BDEBUG_ASSERT(mInitialized);
}

//============================================================================
// BRenderProxyManager::receiveEvent
//============================================================================
bool BRenderProxyManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   BDEBUG_ASSERT(mInitialized);  
   ASSERT_MAIN_THREAD
   
   switch (event.mEventClass)
   {
      case cECRemoveReply:
      {
         IRenderProxy* pProxy = (IRenderProxy*)(event.mPrivateData);
         
         BDEBUG_ASSERT(pProxy && pProxy->getSimState() == cRPSDeleting);
         
         pProxy->setSimState(cRPSRemoved);
         pProxy->simDeleteThis();
         
         break;
      }
      case cECCommand:
      {
         IRenderProxy* pProxy = reinterpret_cast<IRenderProxy*>(event.mPrivateData);
         const DWORD command = event.mPrivateData2;
//-- FIXING PREFIX BUG ID 6371
         const BCommandPayload* pPayload = reinterpret_cast<BCommandPayload*>(event.mpPayload);
//--
                  
         BDEBUG_ASSERT(pProxy && (pProxy->getSimState() == cRPSValid));
         pProxy->simReceiveCommandFromRender(command, pPayload ? pPayload->getData() : NULL, pPayload ? pPayload->getDataLen() : 0);
               
         break;
      }
   }
   
   return false;
}

//============================================================================
// BRenderProxyManager::initDeviceData
//============================================================================
void BRenderProxyManager::initDeviceData(void)
{
   BDEBUG_ASSERT(mInitialized);
}

//============================================================================
// BRenderProxyManager::frameBegin
//============================================================================
void BRenderProxyManager::frameBegin(void)
{
   BDEBUG_ASSERT(mInitialized);
}

//============================================================================
// BRenderProxyManager::processCommand
//============================================================================
void BRenderProxyManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   BDEBUG_ASSERT(mInitialized);
   
   BDEBUG_ASSERT(header.mLen >= sizeof(DWORD));
   IRenderProxy* pProxy = *(IRenderProxy**)pData;
   BDEBUG_ASSERT(pProxy);
   
   switch (header.mType)
   {
      case cRCAdd:
      {
         BDEBUG_ASSERT(pProxy->getRenderState() == cRPSInvalid);
         pProxy->setRenderState(cRPSValid);
         
         const uint size = *reinterpret_cast<const DWORD*>(pData + sizeof(DWORD));

         if (size)
         {
            const uchar* pAlignedData = Utils::AlignUp(pData + sizeof(DWORD) * 2, 16);

            pProxy->renderAdd(pAlignedData, size);
         }
         else
            pProxy->renderAdd(NULL, 0);

         break;
      }
      case cRCRemove:
      {
         BDEBUG_ASSERT(pProxy->getRenderState() == cRPSValid);
                  
         pProxy->renderRemove();
         
         pProxy->setRenderState(cRPSRemoved);
         
         gEventDispatcher.send(mEventHandle, mEventHandle, cECRemoveReply, (DWORD)pProxy);
         
         break;
      }
      case cRCTick:
      {
         BDEBUG_ASSERT(pProxy->getRenderState() == cRPSValid);
         
         pProxy->renderTick();
         break;
      }
      case cRCUpdate:
      {
         BDEBUG_ASSERT(pProxy->getRenderState() == cRPSValid);
         
         const uint size = *reinterpret_cast<const DWORD*>(pData + sizeof(DWORD));
         
         if (size)
         {
            const uchar* pAlignedData = Utils::AlignUp(pData + sizeof(DWORD) * 2, 16);
            
            pProxy->renderUpdateState(pAlignedData, size);
         }
         else
            pProxy->renderUpdateState(NULL, 0);
                  
         break;
      }
      case cRCCommand:
      {
         BDEBUG_ASSERT(pProxy->getRenderState() == cRPSValid);
         
         const uint dataLen = *reinterpret_cast<const DWORD*>(pData + sizeof(DWORD));
         const uint command = *reinterpret_cast<const DWORD*>(pData + sizeof(DWORD) * 2);

         if (dataLen)
         {
            const uchar* pAlignedData = Utils::AlignUp(pData + sizeof(DWORD) * 3, 16);

            pProxy->renderReceiveCommandFromSim(command, pAlignedData, dataLen);
         }
         else
            pProxy->renderReceiveCommandFromSim(command, NULL, 0);

         break;
      }
   }
}

//============================================================================
// BRenderProxyManager::frameEnd
//============================================================================
void BRenderProxyManager::frameEnd(void)
{
   BDEBUG_ASSERT(mInitialized);
}

//============================================================================
// BRenderProxyManager::deinitDeviceData
//============================================================================
void BRenderProxyManager::deinitDeviceData(void)
{
   BDEBUG_ASSERT(mInitialized);
}