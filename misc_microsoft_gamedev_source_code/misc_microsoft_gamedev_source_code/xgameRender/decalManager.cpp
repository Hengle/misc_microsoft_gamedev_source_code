//============================================================================
//
//  File: decalManager.cpp
//
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "decalManager.h"
#include "flashgateway.h"

//============================================================================
// Globals
//============================================================================
BDecalManager gDecalManager;

static const float cDefaultZBias = -0.00175000005f;

//============================================================================
// BDecalManager::BDecalManager
//============================================================================
BDecalManager::BDecalManager() :
   mRenderBegunDrawing(false),
   mNonConformZBias(cDefaultZBias),
   mConformZBias(cDefaultZBias)
{
   ASSERT_THREAD(cThreadIndexSim);
}

//============================================================================
// BDecalManager::~BDecalManager
//============================================================================
BDecalManager::~BDecalManager()
{
   ASSERT_THREAD(cThreadIndexSim);
}

//============================================================================
// BDecalManager::init
//============================================================================
void BDecalManager::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mEventHandle != cInvalidEventReceiverHandle)
      return;
   
   commandListenerInit();
   
   eventReceiverInit();
   
   mNonConformZBias = cDefaultZBias;
   mConformZBias = cDefaultZBias;
}

//============================================================================
// BDecalManager::deinit
//============================================================================
void BDecalManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);  
   
   if (mEventHandle == cInvalidEventReceiverHandle)
      return;
      
   commandListenerDeinit();
   
   eventReceiverDeinit();
}

//============================================================================
// BDecalManager::createDecal
//============================================================================
BDecalHandle BDecalManager::createDecal(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   int index = mSimDecalAttribs.allocIndex(false);
   if (index < 0)
      return cInvalidDecalHandle;

   BDecalAttribs* pDecal = getDecal(index);
   pDecal->clear();
   pDecal->incUseCount();  
   
   // By default the decal is disabled now.
   pDecal->setEnabled(false); 
   
   return static_cast<BDecalHandle>(index);
}

//============================================================================
// BDecalManager::destroyDecal
//============================================================================
void BDecalManager::destroyDecal(BDecalHandle handle)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   BDEBUG_ASSERT(handle != cInvalidDecalHandle);
   
   mSimDecalAttribs.freeIndex(handle);
}

//============================================================================
// BDecalManager::getDecal
//============================================================================
BDecalAttribs* BDecalManager::getDecal(BDecalHandle handle)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   BDEBUG_ASSERT(handle != cInvalidDecalHandle);
   
   BDEBUG_ASSERT(mSimDecalAttribs.isItemAllocated(handle));
   
   return static_cast<BDecalAttribs*>(mSimDecalAttribs.getItem(handle));
}
//============================================================================
// BDecalManager::destroyAllDecals
//============================================================================
void BDecalManager::destroyAllDecals()
{
   ASSERT_THREAD(cThreadIndexSim);
   mSimDecalAttribs.freeAll();
}
//============================================================================
// BDecalManager::destroyDecalsWithCatagoryIndex
//============================================================================
void BDecalManager::destroyDecalsWithCatagoryIndex(uint catIndex)
{
   ASSERT_THREAD(cThreadIndexSim);
   for (uint i = 0; i < mRenderDecalAttribs.getHighwaterMark(); i++)
   {
      if (!mRenderDecalAttribs.isItemAllocated(i))
         continue;

//-- FIXING PREFIX BUG ID 6412
      const BDecalAttribs& decalAttribs = *static_cast<BDecalAttribs*>(mRenderDecalAttribs.getItem(i));
//--

      if (!decalAttribs.getEnabled())
         continue;
      if(decalAttribs.getCategoryIndex()==catIndex)
         destroyDecal(static_cast<BDecalHandle>(i));
   }
}
//============================================================================
// BDecalManager::holdUntilFadeDecal
//============================================================================
void BDecalManager::holdUntilFadeDecal(BDecalHandle handle, double curGameTime, float holdUntilFadeTime, float fadeOutTime, int fadeDirection, eFadeType fadeType)
{
   ASSERT_THREAD(cThreadIndexSim);

   BDEBUG_ASSERT(handle != cInvalidDecalHandle);

   BDEBUG_ASSERT(mSimDecalAttribs.isItemAllocated(handle));

   BDEBUG_ASSERT((fadeDirection == -1) || (fadeDirection == 1));
   BDEBUG_ASSERT(fadeOutTime > 0.0f);


   BBeginFadeDecalData* pData = static_cast<BBeginFadeDecalData*>(gRenderThread.submitCommandBegin(mCommandHandle, cRCBeginFade, sizeof(BBeginFadeDecalData)));

   pData->mHandle = handle;
   pData->mStartTime = curGameTime;
   pData->mTotalTime = fadeOutTime;
   pData->mUseCount = getDecal(handle)->getUseCount();
   pData->mFadeDirection = (char)fadeDirection;
   pData->mFadeType = fadeType;
   pData->mHoldTime = holdUntilFadeTime;

   gRenderThread.submitCommandEnd(sizeof(BBeginFadeDecalData));
}

//============================================================================
// BDecalManager::fadeDecal
//============================================================================
void BDecalManager::fadeDecal(BDecalHandle handle, double curGameTime, float fadeTotalTime, int fadeDirection, eFadeType fadeType)
{
   ASSERT_THREAD(cThreadIndexSim);

   BDEBUG_ASSERT(handle != cInvalidDecalHandle);

   BDEBUG_ASSERT(mSimDecalAttribs.isItemAllocated(handle));
   
   BDEBUG_ASSERT((fadeDirection == -1) || (fadeDirection == 1));
   BDEBUG_ASSERT(fadeTotalTime > 0.0f);
   
   //BDecalAttribs& decalAttribs = *getDecal(handle);
   //if (decalAttribs.getBlendMode() == BDecalAttribs::cBlendOpaque)
   //   decalAttribs.setBlendMode(BDecalAttribs::cBlendOver);
   
   BBeginFadeDecalData* pData = static_cast<BBeginFadeDecalData*>(gRenderThread.submitCommandBegin(mCommandHandle, cRCBeginFade, sizeof(BBeginFadeDecalData)));
   
   pData->mHandle = handle;
   pData->mStartTime = curGameTime;
   pData->mTotalTime = fadeTotalTime;
   pData->mUseCount = getDecal(handle)->getUseCount();
   pData->mFadeDirection = (char)fadeDirection;
   pData->mFadeType = fadeType;
   pData->mHoldTime = 0;
   
   gRenderThread.submitCommandEnd(sizeof(BBeginFadeDecalData));
}

//============================================================================
// BDecalManager::updateRenderThread
//============================================================================
void BDecalManager::updateRenderThread(double gameTime)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   SCOPEDSAMPLE(DecalManagerUpdateRenderThread);
            
   uchar* pData = static_cast<uchar*>(gRenderThread.allocateFrameStorage(mSimDecalAttribs.getSerializeSize()));
   
   mSimDecalAttribs.serialize(pData);
      
   gRenderThread.submitCommand(mCommandHandle, cRCUpdate, BUpdateData(gameTime, pData));

   updateSingleFrameDecals();
}

//============================================================================
// BDecalManager::updateSingleFrameDecals()
//============================================================================
void BDecalManager::updateSingleFrameDecals()
{
   ASSERT_THREAD(cThreadIndexSim);

   for (uint i = 0; i < mSimDecalAttribs.getHighwaterMark(); i++)
   {
      if (!mSimDecalAttribs.isItemAllocated(i))
         continue;

      BDecalAttribs& decalAttribs = *static_cast<BDecalAttribs*>(mSimDecalAttribs.getItem(i));

      if (!decalAttribs.getEnabled())
         continue;

      if (decalAttribs.getRenderOneFrame())
      {
         decalAttribs.setEnabled(false);
      }
   }
}

//============================================================================
// BDecalManager::renderTickDecal
//============================================================================
void BDecalManager::renderTickDecal(BDecalHandle handle, BDecalAttribs& attribs, BDecalState& state)
{  
   if (state.mFading)
   {
      //double mStartTime;
      //float mTotalTime;
      
      float t = 0;
      if(state.mHoldingUntilFade)
      {
         if(mRenderGameTime - state.mFadeStartTime > state.mHoldUntilFadeDuration)
         {
            state.mHoldingUntilFade = false;
            state.mFadeStartTime = mRenderGameTime;
        
         }
            return;
      }
      else
      {
         t = Math::Clamp<float>(float((mRenderGameTime - state.mFadeStartTime) / state.mFadeTotalTime), 0.0f, 1.0f);
      }
      
      if (state.mFadeDirection < 0)
         t = 1.0f - t;
      
      uint alpha = Math::FloatToIntRound(t * 255.0f);
      
      bool doneFading = false;
      if ( ((state.mFadeDirection < 0) && (alpha == 0)) ||
           ((state.mFadeDirection > 0) && (alpha == 255)) )
         doneFading = true;
      
      uint origAlpha = attribs.getAlpha();
      if (origAlpha != 255)
         alpha = (alpha * origAlpha + 128) / 255;
      
      attribs.setAlpha(alpha);
      
      if (doneFading)
      {
         if (state.mFadeType != cFTNormal) 
         {
            if (!state.mSentDoneFadingEvent)
            {
               gEventDispatcher.send(mEventHandle, mEventHandle, cECDoneFading, handle, state.mFadeType, NULL, BEventDispatcher::cSendSynchronousDispatch);
               state.mSentDoneFadingEvent = true;
            }
         }
         else
         {
            state.mFading = false;
         }
      }         
   }
}

//============================================================================
// BDecalManager::BDecalAttribNormalKeySorter::operator()
//============================================================================
bool BDecalManager::BDecalAttribNormalKeySorter::operator() (uint i, uint j) const 
{ 
   const BDecalAttribs& lhs = *static_cast<BDecalAttribs*>(mDecalManager.mRenderDecalAttribs.getItem(i));
   const BDecalAttribs& rhs = *static_cast<BDecalAttribs*>(mDecalManager.mRenderDecalAttribs.getItem(j));
   return lhs.keyCompare(rhs);
}

//============================================================================
// BDecalManager::BDecalAttribAlphaKeySorter::operator()
//============================================================================
bool BDecalManager::BDecalAttribAlphaKeySorter::operator() (uint i, uint j) const
{ 
   const BDecalAttribs& lhs = *static_cast<BDecalAttribs*>(mDecalManager.mRenderDecalAttribs.getItem(i));
   const BDecalAttribs& rhs = *static_cast<BDecalAttribs*>(mDecalManager.mRenderDecalAttribs.getItem(j));

   const BVec4& camPos = gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4();

   const BVec3& lPos = lhs.getPos();
   const BVec3& rPos = rhs.getPos();

   float lDist2 = Math::Sqr(lPos[0] - camPos[0]) + Math::Sqr(lPos[1] - camPos[1]) + Math::Sqr(lPos[2] - camPos[2]);
   float rDist2 = Math::Sqr(rPos[0] - camPos[0]) + Math::Sqr(rPos[1] - camPos[1]) + Math::Sqr(rPos[2] - camPos[2]);

   return lDist2 > rDist2;
}

//============================================================================
// BDecalManager::renderUpdate
//============================================================================
void BDecalManager::renderUpdate(const BUpdateData* pUpdateData)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(DecalManagerRenderUpdate);
         
   mRenderGameTime = pUpdateData->mGameTime;
   
   mRenderDecalAttribs.deserialize(pUpdateData->mpDecalAttribs);

   if (mRenderDecalAttribs.getHighwaterMark() > mRenderDecalState.getSize())
      mRenderDecalState.resize(mRenderDecalAttribs.getHighwaterMark());
   
   mRenderValidNormalDecals.resize(0);
   mRenderValidAlphaDecals.resize(0);
   mRenderValidStencilDecals.resize(0);
   
   for (uint i = 0; i < mRenderDecalAttribs.getHighwaterMark(); i++)
   {
      if (!mRenderDecalAttribs.isItemAllocated(i))
         continue;
      
      BDecalAttribs& decalAttribs = *static_cast<BDecalAttribs*>(mRenderDecalAttribs.getItem(i));
            
      if (!decalAttribs.getEnabled())
         continue;
      
      BDecalState& decalState = mRenderDecalState[i];
      
      if (decalAttribs.getUseCount() != decalState.mUseCount)
      {
         decalState.mUseCount = decalAttribs.getUseCount();
         decalState.clear();
      }
      
      if (decalAttribs.getBlendMode() == BDecalAttribs::cBlendStencil)
         mRenderValidStencilDecals.pushBack(static_cast<ushort>(i));
      else if (decalAttribs.getBlendMode() == BDecalAttribs::cBlendOver)
         mRenderValidAlphaDecals.pushBack(static_cast<ushort>(i));
      else  
         mRenderValidNormalDecals.pushBack(static_cast<ushort>(i));
      
      renderTickDecal(static_cast<BDecalHandle>(i), decalAttribs, decalState);      
   }

   if (mRenderValidStencilDecals.size())
      std::sort(mRenderValidStencilDecals.begin(), mRenderValidStencilDecals.end(), BDecalAttribAlphaKeySorter(*this));
   
   if (mRenderValidAlphaDecals.size())
      std::sort(mRenderValidAlphaDecals.begin(), mRenderValidAlphaDecals.end(), BDecalAttribAlphaKeySorter(*this));
      
   if (mRenderValidNormalDecals.size())
      std::sort(mRenderValidNormalDecals.begin(), mRenderValidNormalDecals.end(), BDecalAttribNormalKeySorter(*this));
}

//============================================================================
// BDecalManager::renderBeginFade
//============================================================================
void BDecalManager::renderBeginFade(const BBeginFadeDecalData& data)
{
   // ajl fixme 12/17/07 - temp hack to debug help debug crash on this line below: "if (data.mUseCount != decalState.mUseCount)"
   #ifndef BUILD_FINAL
      static const BBeginFadeDecalData* pDebugFadeData=NULL;
      static BDecalState* pDebugDecalState=NULL;
      static BDecalHandle debugDecalHandle=cInvalidDecalHandle;
      static bool debugDecalResized=false;
   #endif

   BDecalHandle handle = data.mHandle;

   #ifndef  BUILD_FINAL
      pDebugFadeData=&data;
      debugDecalHandle=handle;
      debugDecalResized=false;
   #endif

   if (static_cast<uint>(handle + 1) > mRenderDecalState.getSize())
   {
      #ifndef  BUILD_FINAL
        debugDecalResized=true;
      #endif
      mRenderDecalState.resize(handle + 1);
   }
   
   BDecalState& decalState = mRenderDecalState[handle];

   #ifndef  BUILD_FINAL
      pDebugDecalState=&decalState;
   #endif

   if (data.mUseCount != decalState.mUseCount)
   {
      decalState.mUseCount = data.mUseCount;
      decalState.clear();
   }
      
   decalState.mFading = true;
   decalState.mFadeStartTime = data.mStartTime;
   decalState.mFadeTotalTime = data.mTotalTime;
   decalState.mFadeDirection = data.mFadeDirection;
   decalState.mFadeType = data.mFadeType;
   decalState.mSentDoneFadingEvent = false;
   decalState.mHoldingUntilFade = data.mHoldTime!=0;
   decalState.mHoldUntilFadeDuration = data.mHoldTime;
}

//============================================================================
// BDecalManager::receiveEvent
//============================================================================
bool BDecalManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   switch (event.mEventClass)
   {
      case cECDoneFading:
      {
         BDecalHandle decalHandle = static_cast<BDecalHandle>(event.mPrivateData);
         if (event.mPrivateData2 == cFTDestroyWhenDone)
            destroyDecal(decalHandle);
         else if (event.mPrivateData2 == cFTDisableWhenDone)
            getDecal(decalHandle)->setEnabled(false);
            
         break;
      }
   }
   
   return false;
}

//============================================================================
// BDecalManager::initDeviceData
//============================================================================
void BDecalManager::initDeviceData(void)
{
}

//============================================================================
// BDecalManager::frameBegin
//============================================================================
void BDecalManager::frameBegin(void)
{
}

//============================================================================
// BDecalManager::processCommand
//============================================================================
void BDecalManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cRCUpdate:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BUpdateData));
         renderUpdate(reinterpret_cast<const BUpdateData*>(pData));
         break;
      }
      case cRCBeginFade:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BBeginFadeDecalData));
         renderBeginFade(*reinterpret_cast<const BBeginFadeDecalData*>(pData));
         break;
      }
   }
}

//============================================================================
// BDecalManager::frameEnd
//============================================================================
void BDecalManager::frameEnd(void)
{
}

//============================================================================
// BDecalManager::deinitDeviceData
//============================================================================
void BDecalManager::deinitDeviceData(void)
{
}

//============================================================================
// BDecalManager::renderDrawDecalsBegin
//============================================================================
void BDecalManager::renderDrawDecalsBegin(void)
{
}

//============================================================================
// BDecalManager::renderDrawDecalsEnd
//============================================================================
void BDecalManager::renderDrawDecalsEnd(void)
{
}

//============================================================================
// BDecalManager::renderDrawDecalsBeginTile
//============================================================================
void BDecalManager::renderDrawDecalsBeginTile(uint tileIndex)
{
   BDEBUG_ASSERT(!mRenderBegunDrawing);
   mRenderBegunDrawing = true;
   tileIndex;

//		depthBias	-0.00175000005	float
//   static float depthBias = -0.000150000007;
//   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));

   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 1);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKOFFSETS, D3DALPHATOMASK_DITHERED);
   
   gRenderDraw.unsetTextures();
   
#define SET_SS(s, v) BD3D::mpDev->SetSamplerState(0, s, v); 
   SET_SS(D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP)
   SET_SS(D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP)
   SET_SS(D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP)
   SET_SS(D3DSAMP_MAGFILTER, D3DTEXF_LINEAR)
   SET_SS(D3DSAMP_MINFILTER, D3DTEXF_LINEAR)
   SET_SS(D3DSAMP_MIPFILTER, D3DTEXF_POINT)
   SET_SS(D3DSAMP_MIPMAPLODBIAS, 0)
   SET_SS(D3DSAMP_MAXMIPLEVEL, 0)
   SET_SS(D3DSAMP_MAXANISOTROPY, 1)
   SET_SS(D3DSAMP_MAGFILTERZ, D3DTEXF_POINT)
   SET_SS(D3DSAMP_MINFILTERZ, D3DTEXF_POINT)
   SET_SS(D3DSAMP_SEPARATEZFILTERENABLE, FALSE)
#undef SET_SS   
}

//============================================================================
// BDecalManager::renderDrawDecalsEndTile
//============================================================================
void BDecalManager::renderDrawDecalsEndTile(uint tileIndex)
{
   BDEBUG_ASSERT(mRenderBegunDrawing);
   tileIndex;
         
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 0);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
   
   mRenderBegunDrawing = false;
}

//============================================================================
// BDecalManager::renderDrawSortedDecals
//============================================================================
void BDecalManager::renderDrawSortedDecals(BDynamicRenderArray<ushort>& sortedIndices, uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming, bool categoryEarlyOut, int pass)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mRenderBegunDrawing);
   tileIndex;

   BManagedTextureHandle curTextureHandle = cInvalidManagedTextureHandle;
   BManagedTextureHandle curStencilTextureHandle = cInvalidManagedTextureHandle;
   bool curConformToTerrain = false;
   int curTessLevel = -1;
   int firstDecalIndex = -1;
   int lastDecalIndex = -1;
   int curBlendMode = -1;
   int curFlashMovieIndex = -1;

   for (uint sortedDecalIndex = 0; sortedDecalIndex < sortedIndices.getSize(); sortedDecalIndex++)
   {
      const uint decalIndex = sortedIndices[sortedDecalIndex];

      BDEBUG_ASSERT(mRenderDecalAttribs.isItemAllocated(decalIndex));

      const BDecalAttribs& decalAttribs = *static_cast<const BDecalAttribs*>(mRenderDecalAttribs.getItem(decalIndex));

      BDEBUG_ASSERT(decalAttribs.getEnabled());
         
      if (filter != cRFAll)
      {
         const bool conformToTerrain = decalAttribs.getConformToTerrain();
         if (filter == cRFConformToTerrain)
         {
            if (!conformToTerrain)
               continue;
         }
         else if (filter == cRFNonConformToTerrain)
         {
            if (conformToTerrain)
               continue;
         }
      }
      
      if (categoryIndex != -1)
      {
         if ((categoryEarlyOut) && ((int)decalAttribs.getCategoryIndex() > (int)categoryIndex))
            break;
         else if ((int)decalAttribs.getCategoryIndex() != (int)categoryIndex)
            continue;
      }

      if ( (curBlendMode            != decalAttribs.getBlendMode())         ||
           (curTextureHandle        != decalAttribs.getTextureHandle())     ||
           (curStencilTextureHandle != decalAttribs.getStencilTextureHandle()) ||
           (curFlashMovieIndex      != decalAttribs.getFlashMovieIndex())   ||
           (curConformToTerrain     != decalAttribs.getConformToTerrain())  ||
           (curTessLevel            != (int)decalAttribs.getTessLevel()) )
      {
         if (firstDecalIndex != -1)
         {
            if (curBlendMode == BDecalAttribs::cBlendStencil)
               renderDrawDecalRangeStencil(pass, sortedIndices, firstDecalIndex, lastDecalIndex, stencilTestWhenConforming);
            else
               renderDrawDecalRange(sortedIndices, firstDecalIndex, lastDecalIndex, stencilTestWhenConforming);
         }

         curBlendMode         = decalAttribs.getBlendMode();
         curTextureHandle     = decalAttribs.getTextureHandle();
         curConformToTerrain  = decalAttribs.getConformToTerrain();
         curTessLevel         = decalAttribs.getTessLevel();
         curFlashMovieIndex   = decalAttribs.getFlashMovieIndex();

         firstDecalIndex = sortedDecalIndex;   
         lastDecalIndex = sortedDecalIndex;
      }  
      else 
      {
         BDEBUG_ASSERT(firstDecalIndex != -1);
         lastDecalIndex = sortedDecalIndex;
      }
   }

   if (firstDecalIndex != -1)
   {
      if (curBlendMode == BDecalAttribs::cBlendStencil)
      {
         renderDrawDecalRangeStencil(pass, sortedIndices, firstDecalIndex, lastDecalIndex, stencilTestWhenConforming);
      }
      else
         renderDrawDecalRange(sortedIndices, firstDecalIndex, lastDecalIndex, stencilTestWhenConforming);
   }
}

//============================================================================
// BDecalManager::renderDrawDecals
//============================================================================
void BDecalManager::renderDrawNormalDecals(uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mRenderBegunDrawing);
   
   SCOPEDSAMPLE(DrawNormalDecals);
   
   renderDrawSortedDecals(mRenderValidNormalDecals, tileIndex, categoryIndex, filter, stencilTestWhenConforming, true, -1);
}

//============================================================================
// BDecalManager::renderDrawAlphaDecals
//============================================================================
void BDecalManager::renderDrawAlphaDecals(uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mRenderBegunDrawing);
   
   SCOPEDSAMPLE(DrawAlphaDecals);
   
   renderDrawSortedDecals(mRenderValidAlphaDecals, tileIndex, categoryIndex, filter, stencilTestWhenConforming, false, -1);
}

//============================================================================
// BDecalManager::renderDrawStencilDecals
//============================================================================
void BDecalManager::renderDrawStencilDecals(uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mRenderBegunDrawing);

   // Early out if nothing to do
   if(mRenderValidStencilDecals.getSize() == 0)
      return;

   SCOPEDSAMPLE(DrawStencilDecals);

   gRenderDraw.clear(D3DCLEAR_STENCIL);

   gRenderDraw.pushRenderState(D3DRS_COLORWRITEENABLE);
   
   renderDrawSortedDecals(mRenderValidStencilDecals, tileIndex, categoryIndex, filter, stencilTestWhenConforming, false, 0);
   renderDrawSortedDecals(mRenderValidStencilDecals, tileIndex, categoryIndex, filter, stencilTestWhenConforming, false, 1);

   gRenderDraw.popRenderState(D3DRS_COLORWRITEENABLE);
   
   BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
   BD3D::mpDev->SetRenderState(D3DRS_STENCILREF, 0);
   BD3D::mpDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
   BD3D::mpDev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
}

//============================================================================
// BDecalManager::renderDrawDecalRangeStencil
//============================================================================
void BDecalManager::renderDrawDecalRangeStencil(int pass, const BDynamicRenderArray<ushort>& sortedIndices, int firstSortedDecalIndex, int lastSortedDecalIndex, bool stencilTestWhenConforming)
{
   SCOPEDSAMPLE(BDecalManager_renderDrawDecalRangeStencil);
   const uint numDecals = lastSortedDecalIndex - firstSortedDecalIndex + 1;

   const uint firstDecalIndex = sortedIndices[firstSortedDecalIndex];
   const BDecalAttribs& firstDecalAttribs = *static_cast<const BDecalAttribs*>(mRenderDecalAttribs.getItem(firstDecalIndex));
   
   if (pass == 0)
   {
      BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILREF, 1);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
      
      BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
     
      BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
      BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
      BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);

      //BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

      const BManagedTextureHandle textureHandle = firstDecalAttribs.getStencilTextureHandle();
      gRenderDraw.setTextureByHandle(0, textureHandle, cDefaultTextureWhite);
   }
   else
   {
      BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILREF, 0);
      BD3D::mpDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);

      BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);

      static BOOL zWriteEnable = FALSE;
      BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);

      // if we have an index to a flash movie but did not get a texture then bail
      int flashMovieIndex = firstDecalAttribs.getFlashMovieIndex();
      if (flashMovieIndex != -1)
      {
         IDirect3DTexture9* pTexture = gFlashGateway.getTexture(flashMovieIndex);
         if(pTexture == NULL)
            return;

         gRenderDraw.setTexture(0, pTexture);
      }
      else
      {
         //CLM fix for assert if this handle is not valid.
         const BManagedTextureHandle textureHandle = firstDecalAttribs.getTextureHandle();
         if(!gD3DTextureManager.isValidManagedTextureHandle(textureHandle))
            gRenderDraw.setTextureByHandle(0, cInvalidManagedTextureHandle, cDefaultTextureWhite);
         else
            gRenderDraw.setTextureByHandle(0, textureHandle, cDefaultTextureWhite);
      }
   }
      
   const bool conformToTerrain = firstDecalAttribs.getConformToTerrain();
   const int tessLevel = firstDecalAttribs.getTessLevel();

   /*
   if (stencilTestWhenConforming)
   {
      BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, conformToTerrain ? TRUE : FALSE);
   }
   */

   float depthBias = conformToTerrain ? mConformZBias : mNonConformZBias;;
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));      

   mRenderPatchInstances.resize(numDecals);

   for (uint i = 0; i < numDecals; i++)
   {
      const uint decalIndex = sortedIndices[firstSortedDecalIndex + i];
      const BDecalAttribs& decalAttribs = *static_cast<const BDecalAttribs*>(mRenderDecalAttribs.getItem(decalIndex));
      //const BDecalState& decalState = mRenderDecalState[decalIndex];

      BTerrainHeightField::BPatchInstance& patch = mRenderPatchInstances[i];

      patch.mWorldPos = decalAttribs.getPos();
      BVec3 forward(decalAttribs.getForward());

      BVec3 right(forward % BVec3(0.0f, 1.0f, 0.0f));

      forward *= decalAttribs.getSizeZ();
      right *= decalAttribs.getSizeX();

      patch.mForwardX = XMConvertFloatToHalf(forward[0]);
      patch.mForwardY = XMConvertFloatToHalf(forward[1]);
      patch.mForwardZ = XMConvertFloatToHalf(forward[2]);

      patch.mRightX = XMConvertFloatToHalf(right[0]);
      patch.mRightY = XMConvertFloatToHalf(right[1]);
      patch.mRightZ = XMConvertFloatToHalf(right[2]);
      patch.mYOffset = decalAttribs.mYOffset.getBits();
      patch.mIntensity = decalAttribs.mIntensity.getBits();

      patch.mU = decalAttribs.mU.getBits();
      patch.mV = decalAttribs.mV.getBits();
      patch.mWidth = decalAttribs.mUWidth.getBits();
      patch.mHeight = decalAttribs.mVHeight.getBits();

      patch.mColor = decalAttribs.mColor;
   }

   const float yLowRange = 10.0f;
   const float yHighRange = 10.0f;

   gTerrainHeightField.renderPatches(mRenderPatchInstances.getPtr(), numDecals, float(tessLevel), yLowRange, yHighRange, conformToTerrain, BTerrainHeightField::eUnlitDecalPatch);
}

//============================================================================
// BDecalManager::renderDrawDecalRange
//============================================================================
void BDecalManager::renderDrawDecalRange(const BDynamicRenderArray<ushort>& sortedIndices, int firstSortedDecalIndex, int lastSortedDecalIndex, bool stencilTestWhenConforming)
{
   SCOPEDSAMPLE(BDecalManager_renderDrawDecalRange);
   const uint numDecals = lastSortedDecalIndex - firstSortedDecalIndex + 1;
   
   const uint firstDecalIndex = sortedIndices[firstSortedDecalIndex];
   const BDecalAttribs& firstDecalAttribs = *static_cast<const BDecalAttribs*>(mRenderDecalAttribs.getItem(firstDecalIndex));
         
   const BDecalAttribs::eBlendMode blendMode = firstDecalAttribs.getBlendMode();
      
   switch (blendMode)
   {  
      case BDecalAttribs::cBlendOver:
      {
         BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
         BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
         BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);

         static BOOL zWriteEnable = FALSE;
         BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);
         break;
      }
      case BDecalAttribs::cBlendAdditive:
      {
         BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
         BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
         BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
         break;
      }
      default:
      {
         BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
         BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
         BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
         break;
      }
   }
   

   // if we have an index to a flash movie but did not get a texture then bail
   int flashMovieIndex = firstDecalAttribs.getFlashMovieIndex();
   if (flashMovieIndex != -1)
   {
      SCOPEDSAMPLE(BDecalManager_renderDrawDecalRange_gFlashGatewaygetTexture);
      IDirect3DTexture9* pTexture = gFlashGateway.getTexture(flashMovieIndex);
      if(pTexture == NULL)
         return;

      gRenderDraw.setTexture(0, pTexture);
   }
   else
   {
      BManagedTextureHandle textureHandle = firstDecalAttribs.getTextureHandle();
      if(textureHandle==NULL) textureHandle = cInvalidManagedTextureHandle;
      gRenderDraw.setTextureByHandle(0, textureHandle, cDefaultTextureWhite);
   }
      
   const bool conformToTerrain = firstDecalAttribs.getConformToTerrain();
   const int tessLevel = firstDecalAttribs.getTessLevel();
   
   if (stencilTestWhenConforming)
   {
      BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, conformToTerrain ? TRUE : FALSE);
   }

   float depthBias = conformToTerrain ? mConformZBias : mNonConformZBias;;
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));      
      
   mRenderPatchInstances.resize(numDecals);
   
   for (uint i = 0; i < numDecals; i++)
   {
      const uint decalIndex = sortedIndices[firstSortedDecalIndex + i];
      const BDecalAttribs& decalAttribs = *static_cast<const BDecalAttribs*>(mRenderDecalAttribs.getItem(decalIndex));
      //const BDecalState& decalState = mRenderDecalState[decalIndex];
      
      BTerrainHeightField::BPatchInstance& patch = mRenderPatchInstances[i];
      
      patch.mWorldPos = decalAttribs.getPos();
      BVec3 forward(decalAttribs.getForward());
      
      BVec3 right(forward % BVec3(0.0f, 1.0f, 0.0f));
      
      forward *= decalAttribs.getSizeZ();
      right *= decalAttribs.getSizeX();
      
      patch.mForwardX = XMConvertFloatToHalf(forward[0]);
      patch.mForwardY = XMConvertFloatToHalf(forward[1]);
      patch.mForwardZ = XMConvertFloatToHalf(forward[2]);
      
      patch.mRightX = XMConvertFloatToHalf(right[0]);
      patch.mRightY = XMConvertFloatToHalf(right[1]);
      patch.mRightZ = XMConvertFloatToHalf(right[2]);
      patch.mYOffset = decalAttribs.mYOffset.getBits();
      patch.mIntensity = decalAttribs.mIntensity.getBits();
            
      patch.mU = decalAttribs.mU.getBits();
      patch.mV = decalAttribs.mV.getBits();
      patch.mWidth = decalAttribs.mUWidth.getBits();
      patch.mHeight = decalAttribs.mVHeight.getBits();

      patch.mColor = decalAttribs.mColor;
   }
    
   const float yLowRange = 10.0f;
   const float yHighRange = 10.0f;
   
   gTerrainHeightField.renderPatches(mRenderPatchInstances.getPtr(), numDecals, float(tessLevel), yLowRange, yHighRange, conformToTerrain, BTerrainHeightField::eUnlitDecalPatch);
}

















