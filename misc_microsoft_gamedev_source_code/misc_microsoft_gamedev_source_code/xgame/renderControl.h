//============================================================================
//
//  File: renderControl.h
//
//  Copyright (c) 2006-2008 Ensemble Studios
//
//============================================================================
#pragma once
#include "threading\eventDispatcher.h"
#include "math\runningAverage.h"
#include "renderThread.h"
#include "interestVolume.h"
#include "bitvector.h"
#include "ugxGeomRenderTypes.h"
#include "AtgFont.h"
#include "debugTextDisplay.h"
#include "dirShadowManager.h"

#ifndef BUILD_FINAL
#include "barChart.h"
#endif;

class BUser;
class BWorld;
class BCubemapGen;
class BGPUDXTVideo;

//============================================================================
// class BRenderControl
//============================================================================
class BRenderControl : public BEventReceiver, BRenderCommandListener
{
public:
   // Sim thread methods
   BRenderControl();
   ~BRenderControl();
   
   BEventReceiverHandle getSimEventHandle(void) const { return mSimEventHandle; };
      
   void init(void);
   void deinit(void);
         
   void startOfFrame(BWorld* pWorld, uint numUsers, BUser** ppUsers, uint macroTileIndex);
      
   uint getNumMacroTiles(void) const { return mSimNumMacroTiles; }
         
   void beginViewport(long userIndex, BUser* pUser, uint viewportIndex, bool enableObscuredUnits = false, bool enableTeamColors = false);
   void renderViewport();
   void endViewport(void);
   
   void endOfFrame(void);
   
   enum
   {
      cFlagDebugRenderWireframe,
      cFlagDebugRenderBoundingBoxes,
      cFlagDisableRenderUnits,
      cFlagDisableRenderSky,
      cFlagDisableRenderTerrain,
      cFlagDisableRenderParticles,
      cFlagDisableShadowRendering,
      cFlagDisableMeshDCBRendering,
      cFlagRandomSplatTest,
      cFlagDebugRenderParticleMagnets,
      cFlagDebugRenderUIWireframe,
   }; 
         
   bool getFlag(long n) const { ASSERT_THREAD(cThreadIndexSim); return (mSimFlags.isSet(n)!=0); }
   void setFlag(long n, bool v) { ASSERT_THREAD(cThreadIndexSim); if (v) mSimFlags.set(n); else mSimFlags.unset(n); }
      
   void showPersistentStats(BDebugTextDisplay& debugTextDisplay, bool showGPUIdle);
   void showRenderStats(BDebugTextDisplay& debugTextDisplay, uint page);
   void showPackedTextureStats(BDebugTextDisplay& debugTextDisplay);
   void showParticleStats(BDebugTextDisplay& debugTextDisplay);
   void showFlashStats(BDebugTextDisplay& textDisplay);
   void showTerrainStats(BDebugTextDisplay& debugTextDisplay);
   void showImpactEffectStats(BDebugTextDisplay& textDisplay);

   bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   
   void screenshot(const char* pFilename, bool hdr);
   bool megaScreenshot(const char* pFilename, uint cols, uint rows, bool doJitterVersion, uint jitterAAQuality);
   void generateHeightfield(void);
   void generateCubeMap(const char* pFilename);
   void generateMinimap(const char* pFilename, int zoomLevel);

#ifndef BUILD_FINAL   
   void startVideo(const char* pFilename, bool downsample, bool autoConvert, float fpsLockRate, bool raw);
   void endVideo(void);

   BBarChart* const getBarChart() { return &mBarChart;} 
   BBarChart* const getBarChartMem() { return &mBarChartMem;} 
#endif   
      
   // Call whenever a new scenario is loaded.
   void newScenario(void);

   // get/set clipping plane distances
   float getNearClipPlane(void) const { return mSimNearClipPlane; }
   float getFarClipPlane(void) const { return mSimFarClipPlane; }
   void setNearFarClipPlanes(float nearClip, float farClip) { mSimNearClipPlane = nearClip; mSimFarClipPlane = farClip; }
         
   double getSimGameTime(void) const { return mSimGameTime; }
   double getSimPrevRenderTime(void) const { return mSimPrevRenderTime; }
   
   BDirShadowManager::BShadowMode getShadowMode(void) const { return mSimShadowMode; }
   void setShadowMode(BDirShadowManager::BShadowMode shadowMode) { mSimShadowMode = shadowMode; }
   
   void enableWorkerRender(bool onOff) { mNextWorkerRenderState = onOff; }


private:
   void overrideUserCamera(BUser* pUser);
   void sampleUserCameras(uint numUsers, BUser** ppUsers);
   
   void updateTime(BWorld* pWorld);
   // Sim thread methods
   void updateSceneMetrics(BVec3& worldMin, BVec3& worldMax);
   void renderDebugBoundingBoxes(BWorld* pWorld);
   void updateUnits(BUser* pUser, BWorld* pWorld, const BVec3& worldMin, const BVec3& worldMax);
   void updateDecalManager(double gameTime);
   void updateUIManager(double gameTime);
   void updateWorldVisibility(void);
   void updateRenderThread(uint viewportIndex, float deltaT, double gameRealTime, double gameTime, FILETIME actualTime, const BVec3& worldMin, const BVec3& worldMax, bool enableObscuredUnits, uint numTeamColors, const DWORD* pTeamColors);
   void updateShadowMode(void);
   void calculateJitterPoints(uint AAQuality, bool jitterAAOffsets);   
   void generateMegaScreenshotEndFrame(void);
   void generateCubemapEndFrame(void); 
   void generateMinimapScreenshotEndFrame(void);
   
   // Render thread methods
   void workerUpdateTerrain(void);
   void workerStartOfFrame(void* pData);
   void workerBeginViewport(void* pData);
   void workerRenderReflection(void);
   void workerRenderFlashComponents(void);
   void workerRenderShadow(void);
   void workerRenderMeshInstancesBeginFrame(void);
   void workerRenderMeshInstancesJoin(void);
   static void workerRenderMeshInstancesForkCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
   void workerRenderMeshInstances(uint tileIndex, bool belowDecals);
   void workerRenderMeshInstancesEndFrame(void);
   void workerRenderVisible(void);
   void workerEndViewport(void* pData);
   void workerRenderViewport(void* pData);
#ifndef BUILD_FINAL      
   void workerTickVideoCapture(void);
#endif   
   void workerEndFrame(void* pData);
   void workerHandleInput(void* pData);
   void workerRenderFarLayerUnits(eUGXGeomPass pass);
   void workerRenderNearLayer(uint tileIndex);
   void workerGenerateCubemapBeginViewport(void);
   void workerGenerateHeightfield(void);
   void workerGenerateCubemapCaptureColorBuffer(void);
   void workerRenderVisibleMeshes(IDirect3DDevice9* pDev, bool setCommandBufferPredication);
#ifndef BUILD_FINAL      
   void workerStartVideo(void* pData);
   void workerEndVideo(void* pData);
#endif   


#ifndef BUILD_FINAL
   static void workerShowRenderStats(void* pData);
   static void workerShowGPUIdleTime(void* pData);
   void workerShowCPUUtilization(float x, float y);
   void workerResetCPUUtilization(void);
#endif   

   static void renderBeginMegascreenshot(void* pData);
   static void renderEndMegascreenshot(void* pData);
   
   bool renderGetFlag(long n) const { ASSERT_THREAD(cThreadIndexRender); return (mUpdateData.mFlags.isSet(n)!=0); }

#ifndef BUILD_FINAL
   void renderSetFlag(long n, bool v) { ASSERT_THREAD(cThreadIndexRender); if (v) mUpdateData.mFlags.set(n); else mUpdateData.mFlags.unset(n); }
#endif

   enum
   {
      cEventClassChangeSHFillLight = cEventClassFirstUser,
      cEventClassSaveSHFillLight
   };

   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
      
   BEventReceiverHandle       mSimEventHandle;
   BInterestVolume            mSimInterestVolume;
   BBitVector32               mSimFlags;
   eUGXGeomVisMode            mSimVisMode;
   FILETIME                   mSimActualTime;
   BWorld*                    mpSimWorld;
   
   BVec4                      mSimGenerateCubemapPos;
   uint                       mSimGenerateCubemapFaceIndex;
   bool                       mSimGenerateCubemapFlag;
   uint                       mSimGenerateCubemapDim;
   bool                       mSimGenerateCubemapWasUIEnabled;
   bool                       mSimGenerateCubemapSH;
   BString                    mSimFLSSaveAsFileName;
   
   BString                    mSimHDRScreenshotFilename;
   bool                       mSimHDRScreenshotFlag;
   
   BString                    mSimMegaScreenshotFilename;
   ushort                     mSimMegaScreenshotCols; 
   ushort                     mSimMegaScreenshotRows;
      
   bool                       mSimMegaScreenshotFlag;
   bool                       mSimMegaScreenshotWasUIEnabledFlag;
   bool                       mSimMegaScreenshotOrigJPEGScreenshotFlag;
   bool                       mSimMegaScreenshotWasPausedFlag;
   bool                       mSimMegaScreenshotUseJitter;
   uint                       mSimMegaScreenshotJitterAAQuality;
   BVec2*                     mpSimMegaScreenshotOffsets;
   BVec2*                     mpSimMegaScreenshotAAOffsets;
      
   bool                       mSimCreatingMegaScreenshotFlag;

   int                        mSimMinimapScreenshotZoomLevel;
   bool                       mSimMinimapScreenshotFlag;   
   bool                       mSimMinimapScreenshotOldRenderSkirtState;
   bool                       mSimMinimapScreenshotOldCameraState;
   bool                       mSimMinimapScreenshotWasUIEnabledFlag;
   bool                       mSimMinimapScreenshotWasFogMaskDisabledFlag;
   bool                       mSimMinimapScreenshotWasDOFEnabled;
   BString                    mSimMinimapFilename;
      
   ushort                     mSimViewportIndex;
   BUser*                     mpSimUser;
   
   ushort                     mSimMacroTileIndex;
   ushort                     mSimNumMacroTiles;
         
   bool                       mSimNewScenarioFlag;
   bool                       mSimGenerateHeightfield;
   
   double                     mSimPrevRenderTime;

   double                     mSimGameTime;

   volatile BOOL              mDoWorkerRender;
   BOOL                       mNextWorkerRenderState;
   
   BDirShadowManager::BShadowMode mSimShadowMode;
   
   float                      mSimNearClipPlane;
   float                      mSimFarClipPlane;
               
   struct BUpdateData
   {
      // mGameRealTime is from BWorld's getTotalRealtime()
      double mGameRealTime;
      
      // mGameTime is from BWorld's getGametimeFloat()
      double mGameTime;

      // mActualTime is UTC real-time, from GetSystemTimeAsFileTime() (100ns resolution), from the sim's point of view.
      FILETIME mActualTime; 
      
      // deltaT is from BWorld's getLastUpdateLengthFloat()
      float mDeltaT;
      
      BVec3 mWorldMin;
      BVec3 mWorldMax;
      
      BBitVector32 mFlags;
      eUGXGeomVisMode mVisMode;
      
      BVec4 mGenerateCubemapPos;
      uint mGenerateCubemapFaceIndex;
      uint mGenerateCubemapDim;
      
      uint mNumTeamColors;
      
      long mUserMode;         // mpSimUser->getUserMode()
                  
      BDirShadowManager::BShadowMode mShadowMode;
                  
      bool mGenerateCubemapFlag;
      bool mGenerateCubemapSH;
                        
      BFixedStringMaxPath mHDRScreenshotFilename;
      bool mHDRScreenshotFlag;
      
      bool mNewScenarioFlag;
      
      bool mDisableAdaptation;
      
      bool mMegaScreenshotFlag;
      
      bool mEnableObscuredUnits;

      bool mMinimapScreenShotFlag;
                        
      enum { cMaxTeamColors = 32 };
      DWORD mTeamColors[cMaxTeamColors];
   };
   
   // mUpdateData can only be accessed by the render thread!
   BUpdateData    mUpdateData;
   
   ATG::Font      mRenderFont;
   BCubemapGen*   mpRenderCubemapGen;
   bool           mRenderTerrainHeightfield;
   
   BWin32Event    mRenderMeshInstancesForkFinished;
   bool           mRenderMeshInstancesForkEventChecked : 1;
                     
#ifndef BUILD_FINAL   
   BRunningAverage<float, float> mTotalGPUPacksAverage;
   BRunningAverage<float, float> mTotalGPUPackPixelsAverage;
#endif   

#ifndef BUILD_FINAL
   uint64                        mPrevFrameSysTime;
   uint64                        mPrevThreadKernelTimes[cThreadIndexMax];
   
   BRunningAverage<float, float> mAveThreadCPUUtilization[cThreadIndexMax];

   BBarChart   mBarChart;
   BBarChart   mBarChartMem;
   BTimer      mRenderThreadTimer;
   BBarChartBarHandle mRenderUpdateBarChartHandle;
   BBarChartBarHandle mRenderDrawBarChartHandle;
#endif   

#ifndef BUILD_FINAL
   BGPUDXTVideo*                 mpRenderGPUDXTVideo;
   BRenderString                 mGPUDXTVideoFilename;
   bool                          mGPUDXTVideoAutoConvert;
#endif   

};

extern BRenderControl gRenderControl;
