//============================================================================
//
// File: renderProxyManager.h
// 
// Copyright (c) 2006 Ensemble Studios
//
// Experimental/Unused
//============================================================================
#pragma once

#include "renderThread.h"

class BRenderProxyManager;

//============================================================================
// enum eRenderProxyState
//============================================================================
enum eRenderProxyState
{
   cRPSInvalid,
   cRPSValid,
   cRPSDeleting,
   cRPSRemoved
};

//============================================================================
// class IRenderProxy
//============================================================================
class IRenderProxy
{
public:
   // Sim thread methods
   IRenderProxy() : 
      mSimState(cRPSInvalid),
      mRenderState(cRPSInvalid)
   {
   }

   virtual ~IRenderProxy()
   {
      BDEBUG_ASSERT((mSimState == cRPSRemoved) && (mRenderState == cRPSRemoved));
   }   
   
   virtual void simDeleteThis(void) = 0;
         
   enum eSerializeRequestType
   {  
      cSRTAdd,
      cSRTUpdate,
   };
   
   virtual void simGetSerializeSize(eSerializeRequestType requestType, uint& size) { size = 0; }
   virtual void simSerializeState(void* pBuf, uint bufLen) { }
   virtual void simReceiveCommandFromRender(DWORD command, const void* pData, uint dataLen) { }

   eRenderProxyState getSimState(void) const { ASSERT_MAIN_THREAD; return mSimState; }
   void setSimState(eRenderProxyState state) { ASSERT_MAIN_THREAD; mSimState = state; }
         
   // Render thread methods
   virtual void renderAdd(const void* pData, uint dataLen) { }
   virtual bool renderTick(void) { return false; }
   virtual void renderUpdateState(const void* pData, uint dataLen) { }
   virtual void renderReceiveCommandFromSim(DWORD command, const void* pData, uint dataLen) { }
   virtual void renderRemove(void) { }
         
   eRenderProxyState getRenderState(void) const { ASSERT_RENDER_THREAD; return mRenderState; }
   void setRenderState(eRenderProxyState state) { ASSERT_RENDER_THREAD; mRenderState = state; }
               
private:
   eRenderProxyState mSimState;   
   eRenderProxyState mRenderState;   
};

//============================================================================
// class BRenderProxyManager
//============================================================================
class BRenderProxyManager : public BRenderCommandListener, public BEventReceiver
{
public:
   BRenderProxyManager();
   ~BRenderProxyManager();
   
   // Sim thread only
   void init(void);
   void deinit(void);
   
   void beginFrame(void);
         
   // add() will cause the proxy object's renderAdd() virtual method to be called on the render thread.
   // You cannot delete the proxy object once it's added into the system! Call remove() or removeAll() instead.
   void add(IRenderProxy* pProxy);
   
   // remove() will cause the proxy object's renderRemove() virtual method to be called on the render thread.
   // The proxy will not actually be deleted on the sim thread until a response message is received from the render thread.
   // The proxy manager will then call simDeleteThis() on the sim thread once it's safe to delete the proxy object.
   void remove(IRenderProxy* pProxy);
   
   // tick() causes the proxy's renderTick() command to be called on the render thread.
   void tick(IRenderProxy* pProxy);
   
   // update() will call the proxy's simGetSerializeSize()/simSerializeState() methods to be called. The serialize buffer will be within the 
   // renderer's command stream. 
   // On the render thread, renderDeserializeState() will be called to deserialize the state from the command stream.
   void update(IRenderProxy* pProxy);
       
   // Sim/render threads
   // If you call this method from the sim thread, the proxy's "renderReceiveCommandFromSim" virtual method will be called.
   // If you call this method from the render thread, the proxy's "simReceiveCommandFromRender" virtual method will be called.
   // Commands sent from the render thread to the sim thread are MUCH more expensive!
   void sendCommand(IRenderProxy* pProxy, DWORD command, const void* pData = NULL, uint dataLen = 0);
   
private:
   void clear(void);
   bool mInitialized : 1;
   
   enum eRenderCommands
   {
      cRCAdd,
      cRCRemove,
      cRCTick,
      cRCUpdate,
      cRCCommand
   };
   
   enum eEventClasses
   {
      cECRemoveReply = cEventClassFirstUser,
      cECCommand
   };
         
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);   
   
   void serializeProxy(IRenderProxy* pProxy, IRenderProxy::eSerializeRequestType requestType, DWORD renderCommand);
};

//============================================================================
// Globals
//============================================================================
extern BRenderProxyManager gRenderProxyManager;
