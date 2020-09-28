//==============================================================================
// displayStats.h
//
// Copyright (c) 2007-2008 Ensemble Studios
//==============================================================================
#pragma once

class BTimingTracker;
class BWorld;
class BDebugTextDisplay;
class BUserManager;
class BSession;

class BDisplayStats
{
public:
   enum eDisplayStat
   {
      cDisplayStatFPS,
      cDisplayStatWorld,
      cDisplayStatPathing,
      cDisplayStatUpdate,
      
      cDisplayStatObject,
      cDisplayStatNetwork,
      cDisplayStatSound,
      
      cDisplayStatKB,
      cDisplayStatBidManager,
      cDisplayStatAISummary,
      cDisplayStatAIMissionTargets,
      cDisplayStatAIMissions,
      cDisplayStatAITopics,
      cDisplayStatFactoids,
      cDIsplayStatTriggerScripts,
      cDisplayStatSquad,
      cDisplayStatClassSize,
      cDisplayStatImpactEffects,
      cDisplayStatAchievements,
      cDisplayStatCampaignProgress1,
      cDisplayStatCampaignProgress2,

      cNumberDisplayStatTypes
   };
   
   enum eDisplayStatRender
   {
      cDisplayStatRender1,
      cDisplayStatRender2,
      cDisplayStatRender3,
      cDisplayStatRender4,
      cDisplayStatRender5,
      cDisplayStatRender6,
      cDisplayStatRender7,
      cDisplayStatRender8,

      cDisplayStatTextureHeap,
      cDisplayStatHeapTotals,
      cDisplayStatHeapDeltas,      
      cDisplayStatRockall,
                        
      cDisplayStatFlash,
      cDisplayFontMemory,
      cDisplayStatParticles,
      cDisplayStatTerrain,
      
      cDisplayD3DTextureManager1,
      cDisplayD3DTextureManager2,
      cDisplayD3DTextureManager3,
      cDisplayD3DTextureManager4,
      cDisplayD3DTextureManager5,
      cDisplayD3DTextureManager6,
      cDisplayD3DTextureManager7,
      cDisplayD3DTextureManager8,
                                    
      cNumberDisplayStatRenderTypes
   };
   
   enum eDisplayStatTimings
   {
      cDisplayStatTimings,
      cDisplayProxyInfo,

      cNumberDisplayStatTimingsTypes
   };

   enum eDisplayStatMode
   {
      cDSMDisabled,
      cDSMSim,
      cDSMRender,
      cDSMTimings,
      
      cNumberDisplayStatModes
   };
   
   static void reset(void) { mDisplayStatMode = cDSMDisabled; Utils::ClearObj(mDisplayStatType); }
   
   static eDisplayStatMode getMode(void) { return mDisplayStatMode; }
   static int getType(eDisplayStatMode mode) { return mDisplayStatType[mode]; }
   
   static void setMode(eDisplayStatMode mode) { mDisplayStatMode = mode; }
   static void setType(int type);
   
   static bool isDisplaying(void);
   
   static bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   
   static void show(BWorld* pWorld, BTimingTracker* pTimingTracker, BUserManager* pUserManager);
   
private:
      
   static eDisplayStatMode mDisplayStatMode;
   static int              mDisplayStatType[cNumberDisplayStatModes];
      
   static void showHeapStats(BDebugTextDisplay& textDisplay);
   static void showDetailedHeapStats(BDebugTextDisplay& textDisplay, bool totals, bool deltas);
   //static void showTextureManagerStats(BDebugTextDisplay& textDisplay, uint page);
   static void showD3DTextureManagerStats(BDebugTextDisplay& textDisplay, uint page);
   
   static void showNetworkStats(BDebugTextDisplay& textDisplay);
   static void showSoundStats(BDebugTextDisplay& textDisplay);
   static void showPathingStats(BDebugTextDisplay& textDisplay);
   static void showUpdateStats(BDebugTextDisplay& textDisplay, BWorld* pWorld, BUserManager* pUserManager);
   static void showObjectStats(BDebugTextDisplay& textDisplay, BWorld* pWorld, BUserManager* pUserManager);
   static void showFPS(BDebugTextDisplay& textDisplay, BWorld* pWorld, BTimingTracker* pTimingTracker);
   static void showRenderStats(BDebugTextDisplay& textDisplay, uint page, BUserManager* pUserManager);
   static void showWorldStats(BDebugTextDisplay& textDisplay, BWorld* pWorld);
   static void showKBStats(BDebugTextDisplay& textDisplay);
   static void showBidManagerStats(BDebugTextDisplay& textDisplay);
   static void showAISummaryStats(BDebugTextDisplay& textDisplay);
   static void showAIMissionTargetStats(BDebugTextDisplay& textDisplay);
   static void showAIMissionStats(BDebugTextDisplay& textDisplay);
   static void showAITopicStats(BDebugTextDisplay& textDisplay);
   static void showFactoidStats(BDebugTextDisplay& textDisplay);
   static void showTriggerScriptStats(BDebugTextDisplay& textDisplay);
   static void showSquadStats(BDebugTextDisplay& textDisplay, BWorld* pWorld, BUserManager* pUserManager, BTimingTracker* pTimingTracker);
   static void showClassSizeStats(BDebugTextDisplay& textDisplay);
   static void showTimingStats(BDebugTextDisplay& textDisplay, const BWorld* const pWorld, const BTimingTracker* const pTimingTracker);
   static void showProxyInfo(BDebugTextDisplay& textDisplay);
   static void showVoiceInfo(BDebugTextDisplay& textDisplay, BSession* pSession, BString& name, uint voiceSession);
   static void showAchievements(BDebugTextDisplay& textDisplay, BUserManager* pUserManager);
   static void showCampaignProgress(BDebugTextDisplay& textDisplay, BUserManager* pUserManager, int page);
   static void showFontMemory(BDebugTextDisplay& textDisplay);
};

