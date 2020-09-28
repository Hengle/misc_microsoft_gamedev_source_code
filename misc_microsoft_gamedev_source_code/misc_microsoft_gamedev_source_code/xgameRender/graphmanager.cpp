//============================================================================
// graphmanager.h
// Ensemble Studios (C) 2007
//============================================================================
#include "xgameRender.h"
#include "graphmanager.h"
#include "graphrenderer.h"

BGraphManager gGraphManager;

//============================================================================
//============================================================================
BGraphManager::BGraphManager():
   mbInitialized(false)
{
}

//============================================================================
//============================================================================
BGraphManager::~BGraphManager()
{
}

//============================================================================
//============================================================================
bool BGraphManager::init()
{
   ASSERT_MAIN_THREAD

   if (mbInitialized)
      return true;

   if (!gGraphRenderer.init())
      return false;

   commandListenerInit();

   mbInitialized = true;
   return true;
}

//============================================================================
//============================================================================
void BGraphManager::deinit()
{
   ASSERT_MAIN_THREAD

   if (!mbInitialized)
      return;

   commandListenerDeinit();
   
   gGraphRenderer.deinit();

   mSimGraphAttribs.freeAll();

   mbInitialized = false;
}

//============================================================================
// BDecalManager::createGraph
//============================================================================
BGraphHandle BGraphManager::createGraph(void)
{
   ASSERT_THREAD(cThreadIndexSim);

   int index = mSimGraphAttribs.allocIndex(false);
   if (index < 0)
      return cInvalidGraphHandle;

   BGraphAttribs* pGraph = getGraph(index);
   pGraph->clear();   

   // By default the decal is disabled now.
   pGraph->setEnabled(false); 

   return static_cast<BGraphHandle>(index);
}

//============================================================================
// BGraphManager::destroyGraph
//============================================================================
void BGraphManager::destroyGraph(BGraphHandle handle)
{
   ASSERT_THREAD(cThreadIndexSim);

   BDEBUG_ASSERT(handle != cInvalidGraphHandle);

   mSimGraphAttribs.freeIndex(handle);
}

//============================================================================
// BGraphManager::getGraph
//============================================================================
BGraphAttribs* BGraphManager::getGraph(BGraphHandle handle)
{
   ASSERT_THREAD(cThreadIndexSim);

   BDEBUG_ASSERT(handle != cInvalidGraphHandle);

   BDEBUG_ASSERT(mSimGraphAttribs.isItemAllocated(handle));

   return static_cast<BGraphAttribs*>(mSimGraphAttribs.getItem(handle));
}
//============================================================================
// BGraphManager::destroyAllGraphs
//============================================================================
void BGraphManager::destroyAllGraphs()
{
   ASSERT_THREAD(cThreadIndexSim);
   mSimGraphAttribs.freeAll();
}

//============================================================================
// BGraphManager::updateRenderThread
//============================================================================
void BGraphManager::updateRenderThread(double gametime)
{
   ASSERT_THREAD(cThreadIndexSim);

   SCOPEDSAMPLE(GraphManagerUpdateRenderThread);

   uchar* pData = static_cast<uchar*>(gRenderThread.allocateFrameStorage(mSimGraphAttribs.getSerializeSize()));

   mSimGraphAttribs.serialize(pData);

   gRenderThread.submitCommand(mCommandHandle, BGMRUpdate, BGraphUpdateData(gametime, pData));   
}

//============================================================================
// BGraphManager::initDeviceData
//============================================================================
void BGraphManager::initDeviceData()
{
}

//============================================================================
// BGraphManager::frameBegin
//============================================================================
void BGraphManager::frameBegin()
{
}

//============================================================================
// BGraphManager::processCommand
//============================================================================
void BGraphManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case BGMRUpdate:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BGraphUpdateData));
         renderUpdate(reinterpret_cast<const BGraphUpdateData*>(pData));
         break;
      }   
   }
}

//============================================================================
// BGraphManager::frameEnd
//============================================================================
void BGraphManager::frameEnd(void)
{
}

//============================================================================
// BGraphManager::deinitDeviceData
//============================================================================
void BGraphManager::deinitDeviceData(void)
{
}

//============================================================================
// BGraphManager::renderUpdate
//============================================================================
void BGraphManager::renderUpdate(const BGraphUpdateData* pUpdateData)
{
   ASSERT_RENDER_THREAD;
   mRenderGraphAttribs.deserialize(pUpdateData->mpGraphAttribs);   

   for (uint i = 0; i < mRenderGraphAttribs.getHighwaterMark(); ++i)
   {
      if (!mRenderGraphAttribs.isItemAllocated(i))
         continue;

      BGraphAttribs& graphAttribs = *static_cast<BGraphAttribs*>(mRenderGraphAttribs.getItem(i));

      graphAttribs.update(pUpdateData->mGameTime);
   }   
}

//==============================================================================
// BGraphManager::simRenderGraphs()
//==============================================================================
void BGraphManager::simRenderGraphs()
{
   ASSERT_MAIN_THREAD   
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BGraphManager::workerRender));
}


//============================================================================
// BGraphManager::renderGraphs
//============================================================================
void BGraphManager::workerRender(void* pData)
{
   ASSERT_RENDER_THREAD
   pData;
   renderGraphs();
}

//============================================================================
// BGraphManager::renderGraphs
//============================================================================
void BGraphManager::renderGraphs()
{
   ASSERT_RENDER_THREAD
   for (uint i = 0; i < mRenderGraphAttribs.getHighwaterMark(); ++i)
   {
      if (!mRenderGraphAttribs.isItemAllocated(i))
         continue;

//-- FIXING PREFIX BUG ID 6411
      const BGraphAttribs& graphAttribs = *static_cast<BGraphAttribs*>(mRenderGraphAttribs.getItem(i));
//--

      if (graphAttribs.getEnabled())
         gGraphRenderer.render(graphAttribs);
   }
}