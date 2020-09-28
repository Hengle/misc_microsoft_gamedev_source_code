//==============================================================================
//
// viewportManager.cpp
//
// Copyright (c) 2008 Ensemble Studios
//
//==============================================================================
#include "xgameRender.h"
#include "viewportManager.h"
#include "renderDraw.h"
#include "renderThread.h"

BViewportManager gViewportManager;

static void setD3DViewport(D3DVIEWPORT9& viewport, uint x, uint y, uint width, uint height)
{
   viewport.X = x;
   viewport.Y = y;
   viewport.Width = width;
   viewport.Height = height;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;
}

BViewportManager::BViewportManager() :
   mSplitScreen(false),
   mVerticalSplit(false),
   mNumViewports(0),
   mpSplitscreenTex(NULL)
{
   Utils::ClearObj(mSceneViewports);
   Utils::ClearObj(mUIViewports);
   Utils::ClearObj(mViewportUserIndices);
   Utils::ClearObj(mBackBufferViewport);
   Utils::ClearObj(mMasterViewport);
   Utils::Set(mCurrentViewport, mCurrentViewport + cMaxThreads, -1);
   Utils::Set(mCurrentUser, mCurrentUser + cMaxThreads, -1);
}

BViewportManager::~BViewportManager()
{
}

void BViewportManager::init()
{
   ASSERT_MAIN_THREAD
   
   deinit();
   
   mSplitScreen = false;
   mVerticalSplit = false;
   mNumViewports = 1;
   
   D3DSURFACE_DESC backBufDesc;
   BD3D::mpDevBackBuffer->GetDesc(&backBufDesc);
   
   const uint backBufWidth = backBufDesc.Width;
   const uint backBufHeight = backBufDesc.Height;
         
   setD3DViewport(mBackBufferViewport, 0, 0, backBufWidth, backBufHeight);
   setD3DViewport(mMasterViewport, 0, 0, backBufWidth, backBufHeight);
   
   setD3DViewport(mSceneViewports[0], 0, 0, backBufWidth, backBufHeight);
   mUIViewports[0] = mSceneViewports[0];
      
   Utils::Set(mCurrentViewport, mCurrentViewport + cMaxThreads, -1);
   Utils::Set(mCurrentUser, mCurrentUser + cMaxThreads, -1);
   
   mViewportUserIndices[0] = 0;
   mViewportUserIndices[1] = 1;
   
   for (uint j = 0; j < cMaxThreads; j++)
      for (uint i = 0; i < cMaxViewports; i++)
         mViewParams[j][i].clear();
}

void BViewportManager::deinit()
{
   ASSERT_MAIN_THREAD
   
   gRenderThread.blockUntilGPUIdle();
   
   if (mpSplitscreenTex)
   {
      mpSplitscreenTex->Release();
      mpSplitscreenTex = NULL;
   }
}

void BViewportManager::set(bool splitscreen, bool verticalSplit, int masterViewportWidth, int masterViewportHeight)
{
   D3DSURFACE_DESC backBufDesc;
   BD3D::mpDevBackBuffer->GetDesc(&backBufDesc);
         
   if (masterViewportWidth < 0) masterViewportWidth = mBackBufferViewport.Width;
   if (masterViewportHeight < 0) masterViewportHeight = mBackBufferViewport.Height;
            
   if ((mSplitScreen == splitscreen) && 
       (verticalSplit == mVerticalSplit) && 
       (mMasterViewport.Width == (uint)masterViewportWidth) && 
       (mMasterViewport.Height == (uint)masterViewportHeight) &&
       (backBufDesc.Width == mBackBufferViewport.Width) &&
       (backBufDesc.Height == mBackBufferViewport.Height))
      return;
      
   gRenderThread.blockUntilGPUIdle();

   if (mpSplitscreenTex)
   {
      mpSplitscreenTex->Release();
      mpSplitscreenTex = NULL;
   }

   setD3DViewport(mBackBufferViewport, 0, 0, backBufDesc.Width, backBufDesc.Height);
   
   BDEBUG_ASSERT(Math::IsInRange<uint>(masterViewportWidth, 1, mBackBufferViewport.Width));
   BDEBUG_ASSERT(Math::IsInRange<uint>(masterViewportHeight, 1, mBackBufferViewport.Height));
            
   setD3DViewport(mMasterViewport, 0, 0, masterViewportWidth, masterViewportHeight);
   
   if (!splitscreen)
   {
      mViewportUserIndices[0] = 0;
      mViewportUserIndices[1] = 1;

      mSplitScreen = false;
      mVerticalSplit = false;
      mNumViewports = 1;

      setD3DViewport(mSceneViewports[0], 0, 0, masterViewportWidth, masterViewportHeight);
      mUIViewports[0] = mSceneViewports[0];      
   }
   else
   {
      mViewportUserIndices[0] = 1;
      mViewportUserIndices[1] = 0;

      mSplitScreen = true;
      mVerticalSplit = verticalSplit;
      mNumViewports = 2;
                        
      if (mVerticalSplit)
      {
         setD3DViewport(mSceneViewports[0], 0, 0, masterViewportWidth / 2, masterViewportHeight);
         mSceneViewports[1] = mSceneViewports[0];
         
         mUIViewports[0] = mSceneViewports[0];
         mUIViewports[1] = mSceneViewports[0];
         
         mUIViewports[1].X += masterViewportWidth / 2;
      }
      else
      {
         setD3DViewport(mSceneViewports[0], 0, 0, masterViewportWidth, masterViewportHeight / 2);
         mSceneViewports[1] = mSceneViewports[0];
         
         mUIViewports[0] = mSceneViewports[0];
         mUIViewports[1] = mSceneViewports[0];
         
         mUIViewports[1].Y += masterViewportHeight / 2;
      }
            
      HRESULT hres = BD3D::mpDev->CreateTexture(mSceneViewports[0].Width, mSceneViewports[0].Height, 1, 0, backBufDesc.Format, 0, &mpSplitscreenTex, NULL);
      if (FAILED(hres))
      {
         BFATAL_FAIL("Out of memory");
      }
   }      
}

static inline uint getCurrentThreadIndex() 
{
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)  
      return 0;
   else
      return 1;
}

void BViewportManager::setCurrentViewport(int viewportIndex) 
{  
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
   {
      mCurrentViewport[0] = viewportIndex; 
      gRenderThread.submitDataCopy(&mCurrentViewport[1], &viewportIndex, sizeof(mCurrentViewport[1]));
   }
   else
      mCurrentViewport[1] = viewportIndex;
}

int BViewportManager::getCurrentViewport() const 
{
   return mCurrentViewport[getCurrentThreadIndex()];
}

// -1 = UI, 0 = primary, 1 = secondary
void BViewportManager::setCurrentUser(int user) 
{ 
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
   {
      mCurrentUser[0] = user; 
      gRenderThread.submitDataCopy(&mCurrentUser[1], &user, sizeof(mCurrentUser[1]));
   }   
   else
      mCurrentUser[1] = user; 
}

int BViewportManager::getCurrentUser() const 
{ 
   return mCurrentUser[getCurrentThreadIndex()];
}

int BViewportManager::getUserViewportIndex(int user) const
{
   for (uint i = 0; i < mNumViewports; i++)
      if (mViewportUserIndices[i] == user)
         return i;
         
   return -1;
}

void BViewportManager::setViewportViewParams(uint viewportIndex, const BRenderViewParams& viewParams)
{
   BDEBUG_ASSERT(viewportIndex < cMaxViewports);

   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
   {
      mViewParams[0][viewportIndex] = viewParams;
      gRenderThread.submitDataCopy(&mViewParams[1][viewportIndex], &mViewParams[0][viewportIndex], sizeof(&mViewParams[1][viewportIndex]));
   }
   else
      mViewParams[1][viewportIndex] = viewParams;
}

const BRenderViewParams& BViewportManager::getViewportViewParams(uint viewportIndex) const   
{ 
   BDEBUG_ASSERT(viewportIndex < cMaxViewports); 
   
   return mViewParams[getCurrentThreadIndex()][viewportIndex]; 
}
