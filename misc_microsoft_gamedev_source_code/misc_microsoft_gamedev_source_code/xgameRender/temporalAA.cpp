// File: temporalAA.cpp
// rg [6/15/06] - This experiment has been abandoned.
#include "xgameRender.h"
#include "temporalAA.h"

#include "asyncFileManager.h"
#include "math\plane.h"

#include "render.h"
#include "renderDraw.h"
#include "vertexTypes.h"

#include "deviceStateDumper.h"

BTemporalAAManager gTemporalAAManager;

BTemporalAAManager::BTemporalAAManager() : 
   BEventReceiver(),
   mWidth(0), 
   mHeight(0),
   mWorkerFrameIndex(0),
   mCurLerpFactor(.5f)
{
   Utils::ClearObj(mpTextures);
   
   for (uint i = 0; i < cNumFrames; i++)
   {
      mViewMatrices[i].setIdentity();
      mProjMatrices[i].setIdentity();
   }
}
   
BTemporalAAManager::~BTemporalAAManager()
{
}
   
void BTemporalAAManager::init(uint width, uint height)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(gRenderThread.getInitialized());
   
   deinit();
   
   mWidth = width;
   mHeight = height;
   mWorkerFrameIndex = 0;
   
   for (uint i = 0; i < cNumFrames; i++)
   {
      mViewMatrices[i].setIdentity();
      mProjMatrices[i].setIdentity();
   }
   
   mCurLerpFactor = .5f;
                           
   eventReceiverInit(cThreadIndexRender);
   commandListenerInit();
                           
   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();
   pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
   pPacket->setFilename("temporalAA\\temporalAA.bin");
   pPacket->setReceiverHandle(mEventHandle);
   pPacket->setDiscardOnClose(true);
               
   gAsyncFileManager.submitRequest(pPacket);
}
   
void BTemporalAAManager::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   if (!mWidth)
      return;
      
   gRenderThread.blockUntilGPUIdle();         
                     
   commandListenerDeinit();
   eventReceiverDeinit();
   
   mWidth = 0;
   mHeight = 0;
   mWorkerFrameIndex = 0;
   
   Utils::ClearObj(mpTextures);

   for (uint i = 0; i < cNumFrames; i++)
   {
      mViewMatrices[i].setIdentity();
      mProjMatrices[i].setIdentity();
   }
         
   gRenderThread.blockUntilGPUIdle();               
}
      
      
int ray3Plane(
              float& t,
              const Plane& plane,
              const BVec3& dir,
              const BVec3& origin,
              bool backside,
              float maxDist,
              float parallelEps)
{
   const float dirAlong = plane.n * dir;

   if (fabs(dirAlong) <= parallelEps)
      return -1;//PARALLEL;

   const float posAlong = plane.n * origin;

   t = (plane.d - posAlong);

   if ((!backside) && (t > 0.0f))
      return 0;//FAILURE;

   t = t / dirAlong;
   if (fabs(t) > maxDist)
      return 0;//FAILURE;

   return 1;
}
   
void BTemporalAAManager::begin(uint frameIndex, const BMatrix44& view, const BMatrix44& projection, float& l, float& t, float& r, float& b)
{
   ASSERT_MAIN_THREAD
   
   l = 0.0f;
   t = 0.0f;
   r = 1.0f;
   b = 1.0f;
   
   const uint curIndex = frameIndex & 1;
   const uint prevIndex = (frameIndex & 1) ^ 1;
   
   mViewMatrices[curIndex] = view;
   mProjMatrices[curIndex] = projection;

   BMatrix44 curWorldToProj(mViewMatrices[curIndex] * mProjMatrices[curIndex]);
   BMatrix44 curProjToWorld(curWorldToProj.inverse());
   BMatrix44 curWorldToScreen(curWorldToProj * gRenderDraw.getWorkerSceneMatrixTracker().getMatrix44(cMTProjToScreen));
   BMatrix44 curViewToWorld(mViewMatrices[curIndex].inverse());
   BVec4 curWorldCameraPos(curViewToWorld.getTranslate());

   BMatrix44 prevWorldToScreen(mViewMatrices[prevIndex] * mProjMatrices[prevIndex] * gRenderDraw.getWorkerSceneMatrixTracker().getMatrix44(cMTProjToScreen));         

   BVec4 worldRayOrg(curWorldCameraPos);
   BVec4 worldRayDir(BVec4(0,0,1,0) * curViewToWorld);
   worldRayDir.normalize();

   Plane groundPlane(0.0f, 1.0f, 0.0f, 0.0f);

   float dist;   
   int result = ray3Plane(
      dist,
      groundPlane,
      worldRayDir,
      worldRayOrg,
      true,
      1e+30f,
      0.0f);

   float dx = 0.0f;
   float dy = 0.0f;                  

   if (result == 1)
   {
      BVec4 worldPos(worldRayOrg + worldRayDir*dist);

      BVec4 curScreenPos((worldPos * curWorldToScreen).project());
      BVec4 prevScreenPos((worldPos * prevWorldToScreen).project());

      dx = prevScreenPos[0] - curScreenPos[0];
      dy = prevScreenPos[1] - curScreenPos[1];

      //dx = curScreenPos[0] - prevScreenPos[0];
      //dy = curScreenPos[1] - prevScreenPos[1];      
   }

   float motionVectorLen = dx*dx+dy*dy;

   if (motionVectorLen > 4.0f)
      mCurLerpFactor = Math::Min(mCurLerpFactor + 1.0f/5.0f, 1.0f);
   else if (motionVectorLen < 1.0f)
      mCurLerpFactor = Math::Max(mCurLerpFactor - 1.0f/5.0f, .5f);
   
   if (frameIndex & 1)
   {
      float s = Math::Lerp(0.5f, 0.0f, (mCurLerpFactor - .5f) / .5f);
      l += s / mWidth;
      t += s / mHeight;
      r += s / mWidth;
      b += s / mHeight;
   }
   
   BBeginData* p = reinterpret_cast<BBeginData*>(gRenderThread.submitCommandBegin(mCommandHandle, cTAACommandBegin, sizeof(BBeginData)));
   p->mFrameIndex = frameIndex;
   //p->mView = view;
   //p->mProj = projection;
   //p->mLerpFactor = mCurLerpFactor;
   gRenderThread.submitCommandEnd(sizeof(BBeginData));
}
   
void BTemporalAAManager::resolve(void)
{
   ASSERT_MAIN_THREAD   

   gRenderThread.submitCommand(mCommandHandle, cTAACommandResolve);
}
   
void BTemporalAAManager::filter(uint x, uint y)
{  
   ASSERT_MAIN_THREAD
   
   BFilterData* pFilterData = reinterpret_cast<BFilterData*>(gRenderThread.submitCommandBegin(mCommandHandle, cTAACommandFilter, sizeof(BFilterData)));
   pFilterData->mX = (WORD)x;
   pFilterData->mY = (WORD)y;
   pFilterData->mLerpFactor = mCurLerpFactor;
   gRenderThread.submitCommandEnd(sizeof(BFilterData));
}
            
bool BTemporalAAManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
   
   switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
      {
         BAsyncFileManager::BRequestPacket* pPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket*>(event.mpPayload);
         
         if (!pPacket->getSucceeded())
         {
            trace("BTemporalAA::receiveEvent: Async load of file %s failed", pPacket->getFilename().c_str());
         }
         else
         {
            HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, pPacket->getData());
            if (FAILED(hres))
            {
               trace("BTemporalAA::receiveEvent: Effect creation of file %s failed", pPacket->getFilename().c_str());
            }
         }
         
         break;
      }
      case cEventClassClientRemove:
      {
         mEffect.clear();
         
         break;
      }
   }
   
   return false;
}
   
void BTemporalAAManager::initDeviceData(void)
{
   for (uint i = 0; i < cNumFrames; i++)
   {
      BDEBUG_ASSERT(NULL == mpTextures[i]);
      
      HRESULT hres = gRenderDraw.createTexture(mWidth, mHeight, 1, 0, D3DFMT_A8R8G8B8, 0, &mpTextures[i], NULL);
      if (FAILED(hres))
      {
         BFATAL_FAIL("BTemporalAAManager::initDeviceData: CreateTexture() failed");
      }
      
      D3DLOCKED_RECT lockedRect;
      mpTextures[i]->LockRect(0, &lockedRect, NULL, 0);
      
      memset(lockedRect.pBits, 0, lockedRect.Pitch * mHeight);
      
      mpTextures[i]->UnlockRect(0);
   }
}

void BTemporalAAManager::frameBegin(void)
{
}
   
void BTemporalAAManager::workerBegin(const BRenderCommandHeader& header, const uchar* pData)
{
   BDEBUG_ASSERT(header.mLen == sizeof(BBeginData));
   const BBeginData* pBeginData = reinterpret_cast<const BBeginData*>(pData);
   
   mWorkerFrameIndex = pBeginData->mFrameIndex;
   //mViewMatrices[mWorkerFrameIndex & 1] = pBeginData->mView;
   //mProjMatrices[mWorkerFrameIndex & 1] = pBeginData->mProj;
}
   
void BTemporalAAManager::workerResolve(const BRenderCommandHeader& header, const uchar* pData)
{
   SCOPEDSAMPLE(BTemporalAAManager_workerResolve);
   
   BD3D::mpDev->Resolve(
      D3DRESOLVE_RENDERTARGET0,
      NULL,
      mpTextures[mWorkerFrameIndex & 1],
      NULL,
      0,
      0,
      NULL,
      0.0f,
      0,
      NULL);
}

void BTemporalAAManager::workerFilter(const BRenderCommandHeader& header, const uchar* pData)
{
   const BFilterData* pFilterData = reinterpret_cast<const BFilterData*>(pData);
   
   if (!mEffect.getEffect())
      return;
      
   mEffect.updateIntrinsicParams();      
      
   SCOPEDSAMPLE(BTemporalAAManager_workerFilter);
      
   const uint curIndex = mWorkerFrameIndex & 1;
   const uint prevIndex = (mWorkerFrameIndex & 1) ^ 1;
   
   float lerpFactor = pFilterData->mLerpFactor;
   float z = 0.0f;
   // gPrevFrame is always bilinear sampled.
   if (curIndex)
   {
      lerpFactor = 1.0f - lerpFactor;
      mEffect("gCurFrame") = mpTextures[prevIndex];
      mEffect("gPrevFrame") = mpTextures[curIndex];
      z = 0.0f;
   }            
   else
   {
      mEffect("gCurFrame") = mpTextures[curIndex];
      mEffect("gPrevFrame") = mpTextures[prevIndex];
      z = 1.0f;
   }
      
   mEffect("gBlendParams") = BVec4(0.0f, 0.0f, z, lerpFactor);
   
   BFXLEffectTechnique& technique = mEffect.getTechniqueFromIndex(0);
   
   technique.beginRestoreDefaultState();
   
   technique.beginPass();
   
   BD3D::mpDev->SetVertexDeclaration(BTLVertex::msVertexDecl);
   
   BTLVertex verts[3];
   
   verts[0].x = -.5f;
   verts[0].y = -.5f;
   verts[0].z = 0.0f;
   verts[0].rhw = 1.0f;
   verts[0].diffuse = 0xFFFFFFFF;
   verts[0].specular = 0xFFFFFFFF;
   verts[0].tu = 0.0f;
   verts[0].tv = 0.0f;
   
   verts[1].x = mWidth-.5f;
   verts[1].y = -.5f;
   verts[1].z = 0.0f;
   verts[1].rhw = 1.0f;
   verts[1].diffuse = 0xFFFFFFFF;
   verts[1].specular = 0xFFFFFFFF;
   verts[1].tu = 1.0f;
   verts[1].tv = 0.0f;
   
   verts[2].x = -.5f;
   verts[2].y = mHeight-.5f;
   verts[2].z = 0.0f;
   verts[2].rhw = 1.0f;
   verts[2].diffuse = 0xFFFFFFFF;
   verts[2].specular = 0xFFFFFFFF;
   verts[2].tu = 0.0f;
   verts[2].tv = 1.0f;
   
   technique.commit();
                                    
   BD3D::mpDev->DrawVerticesUP(D3DPT_RECTLIST, 3, verts, sizeof(verts[0]));
         
   BD3D::mpDev->SetVertexDeclaration(NULL);
   
   technique.endPass();
   
   technique.end();
}

void BTemporalAAManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cTAACommandBegin:
      {
         workerBegin(header, pData);     
         break;
      }
      case cTAACommandResolve:
      {
         workerResolve(header, pData);
         break;
      }
      case cTAACommandFilter:
      {
         workerFilter(header, pData);
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
      }
   }
}

void BTemporalAAManager::frameEnd(void)
{
}

void BTemporalAAManager::deinitDeviceData(void)
{
   for (uint i = 0; i < cNumFrames; i++)
   {
      gRenderDraw.releaseD3DResource(mpTextures[i]);
      mpTextures[i] = NULL;
   }
}
