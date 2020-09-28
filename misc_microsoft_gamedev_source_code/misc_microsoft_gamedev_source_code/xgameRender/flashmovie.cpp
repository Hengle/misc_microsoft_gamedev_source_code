//============================================================================
//
// flashmovie.cpp
//
//============================================================================
#include "xgameRender.h"
#include "renderDraw.h"
#include "gpuHeap.h"
#include "flashmovie.h"
#include "flashmoviedefinition.h"
#include "BD3D.h"
#include "flashmanager.h"
#include "color.h"
#include "Timer.h"
#include "tiledAA.h"


#include "renderToTextureXbox.h"

//============================================================================
//============================================================================
BFlashMovie::BFlashMovie():
   mpMovie(NULL),
   mpData(NULL),
   mDataIndex(-1),
   mRenderTargetHandle(cInvalidManagedTextureHandle),
   mpSavedColorSurf(NULL),
   mpSavedDepthSurf(NULL),
   mX(0),
   mY(0),
   mWidth(0),
   mHeight(0),
   mpRenderTarget(NULL),
   mpRenderTargetTexture(NULL),
   mpDepthStencil(NULL),
   mbInitialized(false)
{
   clear();
}

//============================================================================
//============================================================================
BFlashMovie::~BFlashMovie()
{   
}

//============================================================================
//============================================================================
bool BFlashMovie::init(int dataIndex, BFlashMovieDefinition* pData, bool bRenderToTexture)
{
   ASSERT_RENDER_THREAD
   if (!pData)
      return false;

   if (pData->mStatus==BFlashMovieDefinition::cStatusInitialized)
   {
      if (!pData->load())
         return false;
   }

   BDEBUG_ASSERT(pData->mStatus==BFlashMovieDefinition::cStatusLoaded);
   BDEBUG_ASSERT(pData->mpMovieDef);

   mpData = pData;
   mDataIndex = dataIndex;

   mWidth   = pData->mMovieInfo.Width;
   mHeight  = pData->mMovieInfo.Height;
      
   //-- create an instance of the movie
   GPtr<GFxMovieView>  pNewMovieView = pData->mpMovieDef->CreateInstance();
   if (!pNewMovieView)
   {
      fprintf(stderr, "Error: failed to create movie instance\n");
      return false;
   }

   //-- store the movie instance inside the flash movie
   mpMovie= pNewMovieView;
   //mpMovie->SetVerboseAction(pData->mSettings.VerboseAction);
   mpMovie->SetFSCommandHandler(&gFlashManager);

   // allow stretching of the movie to fit into the entire viewport
   mpMovie->SetViewScaleMode(GFxMovieView::SM_ShowAll);
   //mpMovie->SetViewScaleMode(GFxMovieView::SM_NoScale);
   //mpMovie->SetViewAlignment(GFxMovieView::Align_Center);

   BFlashPlayerLog* pLog = gFlashManager.getLog();
   if (!pLog)
      return false;

   mpMovie->SetLog(pLog);
   
   const BFlashPlayerSettings& settings = gFlashManager.getSettings();
   // Disable pause.
   setFlag(eFlagPaused, false);
   // Init timing for the new piece.
   mFrameCounter    = 0;
   // Time ticks: always rely on a timer
   mTimeStartTicks  = GetTickCount();
   mLastLoggedFps   = mTimeStartTicks;
   // Movie logical ticks: either timer or setting controlled
   mMovieStartTicks = (!settings.FastForward) ? mTimeStartTicks : 0;
   mMovieLastTicks  = mMovieStartTicks;

   XMCOLOR color;
   color.c = cDWORDBlackNoAlpha;
   setBackGroundColor(color);
   setBackGroundAlpha(0.0f);

   setFlag(eFlagRenderToTexture, bRenderToTexture);
   if (bRenderToTexture)
      setFlag(eFlagScaleEnable, false);

   updateViewSize();   

   mX = (BD3D::mD3DPP.BackBufferWidth-mWidth)/2;
   mY = (BD3D::mD3DPP.BackBufferHeight-mHeight)/2;

   mbInitialized = true;
   return true;
}

//============================================================================
//============================================================================
void BFlashMovie::deinit()
{
   if (mpMovie)
   {
      // Release the movie
      mpMovie->Release();
      
      // Must explicitly set the pointer to NULL, so the stupid GPtr "smart" pointer releases its reference to the movie.
      mpMovie = NULL;
   }
   
   if (mRenderTargetHandle!=cInvalidManagedTextureHandle)
   {
      gD3DTextureManager.unloadManagedTextureByHandle(mRenderTargetHandle);
   }

   releaseGPUHeapTexture();   

   if (mpRenderTarget)
      gGPUFrameHeap.releaseD3DResource(mpRenderTarget);
   mpRenderTarget = NULL;

   if (mpDepthStencil)
      gGPUFrameHeap.releaseD3DResource(mpDepthStencil);
   mpDepthStencil = NULL;

   mbInitialized = false;
}

//============================================================================
//============================================================================
void BFlashMovie::invoke(const char* method, const char* fmt, const char* value)
{
   ASSERT_RENDER_THREAD
   BASSERT(mpMovie.GetPtr() != NULL);

   if (mpMovie.GetPtr() == NULL)
      return;

   //trace("Invoke(%s, %s)", method, value);   
   //const char* pResultStr = 
   SCOPEDSAMPLE(BFlashMovie_invoke);
   
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   
   
   mpMovie->Invoke(method, NULL, fmt, value);
   //trace("Invoke Result = %s", pResultStr);

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mASInvokesTotalTime += timer.getElapsedSeconds();
   gFlashManager.mWorkerStats.mASInvokesPerFrame++;
#endif
}

//============================================================================
//============================================================================
void BFlashMovie::invoke(const char* method, const GFxValue* pArgs, int argCount)
{
   ASSERT_RENDER_THREAD
   BASSERT(mpMovie.GetPtr() != NULL);

   if (mpMovie.GetPtr() == NULL)
      return;

#if 0
   trace("Invoke(%s)", method);
   for (int i = 0; i < argCount; i++)
   {
      if (pArgs[i].GetType() == GFxValue::VT_String)
         trace("[%u] : %s", i, pArgs[i].GetString());
      else if (pArgs[i].GetType() == GFxValue::VT_StringW)
      {                  
         trace("[%u] : %S", i, pArgs[i].GetStringW());
      }
      else
         trace("[%u] : %4.3f", i, pArgs[i].GetNumber());
   }
#endif

   //const char* pResultStr = 
   SCOPEDSAMPLE(BFlashMovie_invoke);
   
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif   

   mpMovie->Invoke(method, NULL, pArgs, argCount);
   //trace("Invoke Result = %s", pResultStr);

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mASInvokesTotalTime += timer.getElapsedSeconds();
   gFlashManager.mWorkerStats.mASInvokesPerFrame++;
#endif
}

//============================================================================
//============================================================================
void BFlashMovie::setVariable(const char* variablePath, const char* value, GFxMovie::SetVarType type)
{
   ASSERT_RENDER_THREAD
   BASSERT(mpMovie.GetPtr() != NULL);

   if (mpMovie.GetPtr() == NULL)
      return;
   
   mpMovie->SetVariable(variablePath, value, type);

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mASSetVariablePerFrame++;
#endif
}

//============================================================================
//============================================================================
void BFlashMovie::setVariable(const char* variablePath, const GFxValue& value, GFxMovie::SetVarType type)
{
   ASSERT_RENDER_THREAD
   BASSERT(mpMovie.GetPtr() != NULL);

   if (mpMovie.GetPtr() == NULL)
      return;

   mpMovie->SetVariable(variablePath, value, type);

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mASSetVariablePerFrame++;
#endif
}

//============================================================================
//============================================================================
void BFlashMovie::setVariableArray(const char* variablePath, const GFxValue* pValue, int count, int startIndex, GFxMovie::SetVarType type)
{   
   ASSERT_RENDER_THREAD
   BASSERT(mpMovie.GetPtr() != NULL);

   if (mpMovie.GetPtr() == NULL)
      return;

   BASSERT(pValue!=NULL);
   mpMovie->SetVariableArray(variablePath, startIndex, pValue, count, type);   

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mASSetVariablePerFrame++;
#endif
}

//============================================================================
//============================================================================
void BFlashMovie::handleEvent(const GFxEvent& event)
{
   ASSERT_RENDER_THREAD
   BASSERT(mpMovie.GetPtr() != NULL);

   if (mpMovie.GetPtr() == NULL)
      return;

   mpMovie->HandleEvent(event);
}

//============================================================================
//============================================================================
void BFlashMovie::setBackGroundColor(XMCOLOR color)
{
   ASSERT_RENDER_THREAD
   mBackgroundColor = color;
}

//============================================================================
//============================================================================
void BFlashMovie::setBackGroundAlpha(float alpha)
{
   ASSERT_RENDER_THREAD
   mBackgroundColor.a = (UINT) (255.0f * alpha);
}

//============================================================================
//============================================================================
void BFlashMovie::setDimensions(int x, int y, int width, int height)
{
   mX = x;
   mY = y;
   mWidth = width;
   mHeight = height;
}

//============================================================================
//============================================================================
void BFlashMovie::render()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLEID(FlashMovie, 0xFFFF0000);

   BASSERT(mpMovie.GetPtr() != NULL);
   if (mpMovie.GetPtr() == NULL)
      return;

   if (!mpMovie)
      return;

   if (!mbInitialized)
      return;

   static bool bTraceFileName = false;
   if (bTraceFileName)
      trace("Render Flash File: %s", BStrConv::toA(mpData->mFilename));

   mTimeTicks = GetTickCount();

   const BFlashPlayerSettings& settings = gFlashManager.getSettings();
   if (!settings.FastForward)
      mMovieTicks = mTimeTicks;
   else // Simulate time.          
      mMovieTicks = mMovieLastTicks + (UInt32) (1000.0f / mpData->mMovieInfo.FPS);

   int     deltaTicks  = mMovieTicks - mMovieLastTicks;

   // AJL 2/8/07 - Clamp delta to 300 milliseconds
   if (deltaTicks > 1200 && !settings.FastForward)
   {
      //trace("BFlashMovie::render: %u", deltaTicks);
      deltaTicks = 1200;
   }

   float   deltaT      = deltaTicks / 1000.f;

   mMovieLastTicks = mMovieTicks;
   mFrameCounter++;
   
   renderBegin();

   if (getFlag(eFlagRenderToTexture))
   {      
      renderToTextureBegin();
   }
   else
   {
      // Potential out-of bounds range is not a problem here,
      // because it will be adjusted for inside of the player.   
      GViewport viewPort;      
      // The height and width of the buffer surface within witch a viewport is located.
      viewPort.BufferHeight = BD3D::mD3DPP.BackBufferHeight;
      viewPort.BufferWidth  = BD3D::mD3DPP.BackBufferWidth;
                        
      //The left and top side of the viewport rectangle, counting in pixels from the left
      viewPort.Left = mX;
      viewPort.Top = mY;

      // The height and width of the viewport rectangle in pixels
      viewPort.Width = mWidth;
      viewPort.Height = mHeight;

      viewPort.Flags = GViewport::View_RenderTextureAlpha;
      mpMovie->SetViewport(viewPort);
      //mpMovie->SetViewport((BD3D::mD3DPP.BackBufferWidth-mViewWidth)/2, (BD3D::mD3DPP.BackBufferHeight-mViewHeight)/2, mViewWidth, mViewHeight, );
   }

   const GColor bColor(mBackgroundColor.c);
   mpMovie->SetBackgroundColor(bColor);

#ifndef BUILD_FINAL
   BTimer timer;   
#endif
   if (!getFlag(eFlagPaused))
   {
#ifndef BUILD_FINAL
      timer.start();
#endif
      if (mpMovie)
         mpMovie->Advance(deltaT * mSpeedScale);

#ifndef BUILD_FINAL
      timer.stop();
      gFlashManager.mWorkerStats.mCPUTimeAdvance += (timer.getElapsedSeconds() * 1000.0f);
#endif
   }

#ifndef BUILD_FINAL
   timer.start();
#endif
   if (mpMovie)
      mpMovie->Display();

#ifndef BUILD_FINAL
   timer.stop();
   static bool bTraceMovieTimes = false;
   if (bTraceMovieTimes)
      trace("Render Flash File: %s -- time: %4.3f", BStrConv::toA(mpData->mFilename), timer.getElapsedSeconds() * 1000.0f);

   gFlashManager.mWorkerStats.mCPUTimeDisplay += (timer.getElapsedSeconds() * 1000.0f);
#endif

   if (getFlag(eFlagRenderToTexture))   
      renderToTextureEnd();

   renderEnd();

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mMoviesRenderedPerFrame++;
#endif
}

//============================================================================
//============================================================================
void BFlashMovie::releaseGPUHeapTexture()
{
   if (getFlag(eFlagRenderToTexture) && mpRenderTargetTexture)
   {
      gGPUFrameHeap.releaseD3DResource(mpRenderTargetTexture);
      mpRenderTargetTexture = NULL;
   }
}

//============================================================================
//============================================================================
void BFlashMovie::renderBegin()
{
   BD3D::mpDev->GetViewport(&mSavedViewport);
   BD3D::mpDev->GetScissorRect(&mSavedScissorRect);

   gRenderDraw.setDefaultRenderStates();
   gRenderDraw.setDefaultSamplerStates();
}

//============================================================================
//============================================================================
void BFlashMovie::renderEnd()
{
   BD3D::mpDev->SetViewport(&mSavedViewport);
   BD3D::mpDev->SetScissorRect(&mSavedScissorRect);
}

//============================================================================
//============================================================================
void BFlashMovie::renderToTextureBegin()
{
   ASSERT_RENDER_THREAD

   BASSERT(mpMovie.GetPtr() != NULL);
   if (mpMovie.GetPtr() == NULL)
      return;
   
   BD3D::mpDev->GetRenderTarget(0, &mpSavedColorSurf);
   BD3D::mpDev->GetDepthStencilSurface(&mpSavedDepthSurf);
   BD3D::mpDev->GetViewport(&mSavedViewport);
   BD3D::mpDev->GetScissorRect(&mSavedScissorRect);
      
   D3DSURFACE_PARAMETERS surfaceParams;
   Utils::ClearObj(surfaceParams);
   surfaceParams.Base = 0;

   D3DFORMAT renderFormat = D3DFMT_A8R8G8B8;
   D3DMULTISAMPLE_TYPE multiSampleType = D3DMULTISAMPLE_NONE;
   D3DFORMAT depthStencilFormat = D3DFMT_D24S8;
   
   HRESULT hres;
   // render target
   hres = gGPUFrameHeap.createRenderTarget(mWidth, mHeight, renderFormat, multiSampleType, 0, FALSE, &mpRenderTarget, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));

   // depth stencil
   surfaceParams.Base = XGSurfaceSize(mWidth, mHeight, renderFormat, multiSampleType);
   hres = gGPUFrameHeap.createRenderTarget(mWidth, mHeight, depthStencilFormat, multiSampleType, 0, FALSE, &mpDepthStencil, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));
   
   // texture
   hres = gGPUFrameHeap.createTexture(mWidth, mHeight, 1, 0, renderFormat, 0, &mpRenderTargetTexture, NULL);
   BVERIFY(SUCCEEDED(hres));

   BD3D::mpDev->SetRenderTarget(0, mpRenderTarget);
   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
            
   GViewport desc;
   
   desc.BufferWidth = mWidth; 
   desc.BufferHeight= mHeight;
   desc.Left = 0;
   desc.Top  = 0;
   desc.Width = mWidth;
   desc.Height = mHeight;
   desc.Flags = GViewport::View_RenderTextureAlpha;

   mpMovie->SetViewport(desc);   
}

//============================================================================
//============================================================================
void BFlashMovie::renderToTextureEnd()
{
   ASSERT_RENDER_THREAD
         
   D3DRECT resolveRect;
   resolveRect.x1 = 0;
   resolveRect.y1 = 0;
   resolveRect.x2 = mWidth;
   resolveRect.y2 = mHeight;
   
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, &resolveRect, mpRenderTargetTexture, NULL, 0, 0, NULL, 0, 0, NULL);   
   
   //-- Restore old state
   BD3D::mpDev->SetRenderTarget(0, mpSavedColorSurf);
   mpSavedColorSurf->Release();
   mpSavedColorSurf = NULL;

   BD3D::mpDev->SetDepthStencilSurface(mpSavedDepthSurf);
   if (mpSavedDepthSurf)
   {
      mpSavedDepthSurf->Release();
      mpSavedDepthSurf = NULL;
   }

   BD3D::mpDev->SetViewport(&mSavedViewport);
   BD3D::mpDev->SetScissorRect(&mSavedScissorRect);

   gRenderDraw.setDefaultRenderStates();
   gRenderDraw.setDefaultSamplerStates();

   gGPUFrameHeap.releaseD3DResource(mpRenderTarget);
   mpRenderTarget = NULL;
   gGPUFrameHeap.releaseD3DResource(mpDepthStencil);
   mpDepthStencil = NULL;

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mResolvedPixelsPerFrame += (mWidth*mHeight);
   gFlashManager.mWorkerStats.mResolvesPerFrame++;
#endif
}

//============================================================================
//============================================================================
void BFlashMovie::clear()
{
   ASSERT_RENDER_THREAD
   mFlags.setAll(0);
   setFlag(eFlagScaleEnable, true);
   setFlag(eFlagPaused, false);

   // Clear timing 
   mSpeedScale          = 1.0f;
   mFrameCounter        = 0;
   mTimeStartTicks      = 0;
   mTimeTicks           = 0;

   mMovieStartTicks     = 0;
   mMovieLastTicks      = 0;
   mMovieTicks          = 0;

   mWidth               = 0; 
   mHeight              = 0;
   mX                   = 0;
   mY                   = 0;
   mBackgroundColor.c   = cDWORDBlackNoAlpha;
   mbInitialized = false;
}

//============================================================================
// Updates the view size based on the mScaleEnable flag and window size.
//============================================================================
void BFlashMovie::updateViewSize()
{
   ASSERT_RENDER_THREAD

   //if (mScaleEnable)
   if (getFlag(eFlagScaleEnable))
   {
#if 0
      static bool bTestScaling = false;
      static bool bIs4x3 = false;
      static bool bIs16x9 = true;
      if (bTestScaling)
      {         
         const float authoringDimensionWidth  = 1800.0f;
         const float authoringDimensionHeight = 1300.0f;
         const float authoringSafeAreaWidth   = 1400.0f;
         const float authoringSafeAreaHeight  = 900.0f;

         const float backBufferSafeAreaScalar = 0.9f;
         const float backBufferSafeAreaWidth = BD3D::mD3DPP.BackBufferWidth * backBufferSafeAreaScalar;
         const float backBufferSafeAreaHeight = BD3D::mD3DPP.BackBufferHeight * backBufferSafeAreaScalar;

         float scalar = 1.0f;
         if (bIs16x9)
            scalar = backBufferSafeAreaHeight / authoringSafeAreaHeight;
         else if (bIs4x3)
            scalar = backBufferSafeAreaWidth / authoringSafeAreaWidth;

         float newWidth =  scalar * authoringDimensionWidth;
         float newHeight = scalar * authoringDimensionHeight;

         int offsetX = Math::FloatToIntRound( 0.5f * (((float)BD3D::mD3DPP.BackBufferWidth) - newWidth));
         int offsetY = Math::FloatToIntRound( 0.5f * (((float)BD3D::mD3DPP.BackBufferHeight) - newHeight));

         mX = offsetX;
         mY = offsetY;
         mWidth = Math::FloatToIntRound(newWidth);
         mHeight = Math::FloatToIntRound(newHeight);
         return;
      }
#endif

      //const D3DVIEWPORT9& viewPort = gRenderDraw.getWorkerActiveRenderViewport().getViewport();
      SInt width = GTL::gmax<SInt>(BD3D::mD3DPP.BackBufferWidth, 4);
      SInt height= GTL::gmax<SInt>(BD3D::mD3DPP.BackBufferHeight, 4);

      // Determine movie size and location based on the aspect ratio  
      float hw = (Float) mpData->mMovieInfo.Height / (Float) mpData->mMovieInfo.Width;
      if (width * hw > height)
      {
         // Use height as the basis for aspect ratio
         mWidth   = (SInt)((float) height / hw);
         mHeight  = height;
      }
      else
      {
         // Use width
         mWidth   = width;
         mHeight  = (SInt) (width * hw);
      }

      // Compute input scale
      mScaleX = (Float) mWidth / (Float) mpData->mMovieInfo.Width;
      mScaleY = (Float) mHeight / (Float) mpData->mMovieInfo.Height;

      static bool bRetainAspectRatio = false;
      if (!bRetainAspectRatio)
      {
         if (width<=mpData->mMovieInfo.Width)
            mWidth = width;
         if (height<=mpData->mMovieInfo.Height)
            mHeight = height;
      }
   }
   else
   {
      // No scaling, just center the image
      mWidth   = mpData->mMovieInfo.Width;
      mHeight  = mpData->mMovieInfo.Height;
      mScaleX = 1.0f;
      mScaleY = 1.0f;
   }
}
