//==============================================================================
// render.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// ximage
#include "RGBAImage.h"

// local
#include "renderViewParams.h"

// xrender
#include "renderViewport.h"
#include "matrixTracker.h"
#include "renderThread.h"
#include "renderDraw.h"
#include "asyncFileManager.h"
#include "gpuDXTPack.h"

template<class SampleType, class AccumType> class BRunningAverage;

//==============================================================================
// class BRender
//==============================================================================
#pragma warning(push)
#pragma warning(disable:4324)
class BRender : public BRenderCommandListener
{
public:
   BRender();
   ~BRender();

   enum BAspectRatioMode
   {
      cAspectRatioMode16x9 = 0,
      cAspectRatioMode4x3,
      cAspectRatioModeTotal,
   };
   
   void					      create(void);
   void					      destroy(void);

   bool					      getInitialized(void) { return mInitialized; }
   bool					      getStarted(void) { return mStarted; }
      
   bool                    start(const BD3D::BCreateDeviceParams& createDeviceParams,
                                 long startupDirID = cDirBase,
                                 long textureManagerBaseDirID = cDirBase,
                                 long effectCompilerDefaultDirID = cDirBase,
                                 long fontsDirID = cDirBase,
                                 long dataDirID = cDirBase);
                                                                     
   const D3DPRESENT_PARAMETERS& getPresentParameters(void) { BDEBUG_ASSERT(mStarted); return gRenderDraw.getPresentParams(); }
   const D3DDISPLAYMODE&   getDisplayMode(void) { BDEBUG_ASSERT(mStarted); return gRenderDraw.getDisplayMode(); }
   
   // These methods are safe to call from the main thread, which may not be the case for each manager.
   long                    getTextureManagerBaseDirID(void) const { return mTextureManagerBaseDirID; }
   long                    getEffectCompilerDefaultDirID(void) const { return mEffectCompilerDefaultDirID; }

   // Returns the backbuffer's width/height. 
   // IMPORTANT: This is NOT necessarily the dimensions of the game's master viewport, or the user's viewport. 
   // See the API's in viewportManager.cpp.
   DWORD                   getWidth(void) { BDEBUG_ASSERT(mStarted); return gRenderDraw.getPresentParams().BackBufferWidth; }
   DWORD                   getHeight(void) { BDEBUG_ASSERT(mStarted); return gRenderDraw.getPresentParams().BackBufferHeight; }
      
   // Displaying startup status is very slow - it blocks until the worker thread and GPU are idle.
   void                    startupStatus(const BCHAR_T *fmt, ...);

   void                    debugPrintClear(void);
   void                    debugPrintSetY(uint y);   
   void                    debugPrintf(const BCHAR_T *fmt, ...);
   
   bool                    screenShot(const char* pFilename = NULL, bool hdr = false);
   bool                    minimapScreenShot(const char* pFilename = NULL);
      
   // Returns reference to view params object. Game sets these parameters to control the camera, frustum, aspect ratio, etc.
   // WARNING: The view params are updated by the renderer AFTER the sim update is complete. So if you access this before rendering, you'll be access the previous frame's values.
   const BRenderViewParams& getViewParams(void) const { BDEBUG_ASSERT(mStarted); ASSERT_MAIN_THREAD; return mViewParams; }
   int getAspectRatioMode() const { return mAspectRatioMode; };
   
   void                    beginFrame(float deltaT);
   
   // Updates the matrix tracker and render viewport - uses the BRenderViewParams as a source. Call after updating the BRenderViewParams object.   
   void                    beginViewport(int viewportIndex); 
   void                    endViewport(void);
   void                    beginFullScreenUI(void);
   void                    endFullScreenUI(void);
         
   void                    endFrame(void);
      
   const XMMATRIX&         getWorldXMMatrix(void) const { ASSERT_MAIN_THREAD; return mWorldMatrix; }
   const BMatrix&          getWorldBMatrix(void) const { ASSERT_MAIN_THREAD; return *reinterpret_cast<const BMatrix*>(&mWorldMatrix); }
   
   void                    setWorldMatrix(BMatrix matrix) { mWorldMatrix = matrix; }
   void                    setWorldMatrix(XMMATRIX matrix) { mWorldMatrix = matrix; }
   void                    setWorldMatrix(const D3DXMATRIX& matrix) { memcpy(&mWorldMatrix, &matrix, sizeof(mWorldMatrix)); }
   void                    setWorldMatrixIdentity(void) { mWorldMatrix = XMMatrixIdentity(); }
   
   double                  getAverageFPS(void);
   double                  getAverageFrameTime(void);
   double                  getMaxFrameTime(void);
   void                    resetAverageFPS(void);
                           
private:
   XMMATRIX                mWorldMatrix;
   
   BRenderThread::BD3DDeviceInfo mDeviceInfo;
               
   bool                    mInitialized;
   bool                    mStarted;
   
   long                    mTextureManagerBaseDirID;
   long                    mEffectCompilerDefaultDirID;
   long                    mFontsDirID;
   
   BRenderViewParams       mViewParams; 
   int                     mAspectRatioMode;
      
   float                   mTimeLoop;
   
   uint64                  mLastBeginFrameTime;
   double                  mLastFrameLength;
   typedef BRunningAverage<double, double> BRunningAverageD;
   BRunningAverageD*       mpLastFrameLengthFilter;
   
   uint                    mDebugPrintY;
      
   void                    clear(void);
   void                    updateIntrinsics(float deltaT);
   void                    updateSceneMatricesAndViewport(int viewportIndex);
   void                    updateDebugKeys(void);
   
   enum 
   {  
      cRenderCommandHDRScreenshot = 0
   };

   bool                    workerSaveHDRScreenshot(const char* pFilename);
   bool                    workerSaveHDRScreenshotInternal(const char* pFilename);
   
   void                    updateAverageFPS(void);
   void                    updateViewports(void);
   
   static void             renderThreadStart(void* pData);
   static void             renderThreadDestroy(void* pData);
      
   virtual void            initDeviceData(void);
   virtual void            processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void            deinitDeviceData(void);
};
#pragma warning(pop)

extern BRender gRender;
