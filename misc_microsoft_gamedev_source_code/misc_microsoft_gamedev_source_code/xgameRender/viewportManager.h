//==============================================================================
//
// viewportManager.h
//
// Copyright (c) 2008 Ensemble Studios
//
//==============================================================================
#pragma once
#include "renderViewParams.h"

class BViewportManager
{
   BViewportManager(const BViewportManager&);
   BViewportManager& operator= (const BViewportManager&);
   
public:
   enum { cMaxViewports = 2 };
   
   BViewportManager();
   ~BViewportManager();
         
   // --- Sim thread only ---
   void init();
   void deinit();
         
   void set(bool splitscreen, bool verticalSplit, int masterViewportWidth = -1, int masterViewportHeight = -1);
         
   // --- Any thread ---
   const D3DVIEWPORT9& getBackBufferViewport() const { return mBackBufferViewport; }
   uint getBackBufferWidth() const { return mBackBufferViewport.Width; }
   uint getBackBufferHeight() const { return mBackBufferViewport.Height; }
   
   bool isSplitScreen() const { return mSplitScreen; }
   bool isVerticalSplit() const { return mVerticalSplit; }
         
   const D3DVIEWPORT9& getMasterViewport() const { return mMasterViewport; }
   uint getMasterViewportWidth() const { return mMasterViewport.Width; }
   uint getMasterViewportHeight() const { return mMasterViewport.Height; }
         
   uint getNumViewports() const { return mNumViewports; }
   
   //  The scene viewport X,Y offsets are always (0,0).
   const D3DVIEWPORT9& getSceneViewport(uint viewportIndex) const { BDEBUG_ASSERT(viewportIndex < mNumViewports); return mSceneViewports[viewportIndex]; }
   // The UI viewports may have non-zero X,Y offsets.
   const D3DVIEWPORT9& getUIViewport(uint viewportIndex) const { BDEBUG_ASSERT(viewportIndex < mNumViewports); return mUIViewports[viewportIndex]; }
   
   IDirect3DTexture9* getSplitscreenTex() const { return mpSplitscreenTex; }
   
   void setCurrentViewport(int viewportIndex);
   // Returns -1 if outside the viewport rendering loop
   int getCurrentViewport() const;
   
   bool isLastViewport() const { return (int)getCurrentViewport() == ((int)getNumViewports() - 1); }
      
   // -1 = UI, 0 = primary, 1 = secondary
   void setCurrentUser(int user);
   int getCurrentUser() const;
   
   void setViewportUserIndex(uint viewportIndex, int user) { BDEBUG_ASSERT(viewportIndex < cMaxViewports); mViewportUserIndices[viewportIndex] = user; }
   // Returns the user associated with the given viewport index.
   int getViewportUserIndex(uint viewportIndex) const { BDEBUG_ASSERT(viewportIndex < cMaxViewports); return mViewportUserIndices[viewportIndex]; }
   // Returns the viewport with the given user, or -1.
   int getUserViewportIndex(int user) const;
   
   void setViewportViewParams(uint viewportIndex, const BRenderViewParams& viewParams);
   const BRenderViewParams& getViewportViewParams(uint viewportIndex) const;
               
private:
   // Any thread
   bool                 mSplitScreen : 1;
   bool                 mVerticalSplit : 1;
      
   // The backbuffer viewport always encompasses the entire backbuffer, and will never change.
   D3DVIEWPORT9         mBackBufferViewport;
   
   // The master viewport will usually be the entire backbuffer, but may be smaller. The splitscreen viewports must fit inside the master viewport.
   D3DVIEWPORT9         mMasterViewport;
   
   // Scene viewports are used while rendering the viewport. They always have an X,Y offset of (0,0). 
   uint                 mNumViewports;
   D3DVIEWPORT9         mSceneViewports[cMaxViewports];
   // The UI viewports have proper (X,Y) offsets.
   D3DVIEWPORT9         mUIViewports[cMaxViewports];
   
   int                  mViewportUserIndices[cMaxViewports];
   
   // Sim/render thread copies
   enum { cMaxThreads = 2 };
   BRenderViewParams    mViewParams[cMaxThreads][cMaxViewports];
   int                  mCurrentViewport[cMaxThreads];
   int                  mCurrentUser[cMaxThreads];
         
   IDirect3DTexture9*   mpSplitscreenTex;   
      
   static void renderEnableSplitscreen(void* pData);
   static void renderDisableSplitscreen(void* pData);
};

extern BViewportManager gViewportManager;
