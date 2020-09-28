//==============================================================================
// displayStats.cpp
//
// Copyright (c) 2007-2008 Ensemble Studios
//==============================================================================
#include "common.h"
#include "displayStats.h"

// xgame
#include "ai.h"
#include "aifactoid.h"
#include "bid.h"
#include "bidmgr.h"
#include "configsGame.h"
#include "kb.h"
#include "timingtracker.h"
#include "ui.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "selectionmanager.h"
#include "statsManager.h"
#include "pathingLimiter.h"
#include "player.h"
#include "protoobject.h"
#include "protosquad.h"
#include "prototech.h"
#include "squadactionmove.h"
#include "tactic.h"
#include "team.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "recordgame.h"
#include "worldsoundmanager.h"
#include "squadactionattack.h"
#include "flashmanager.h"
#include "pather.h"
#include "protopower.h"
#include "achievementmanager.h"
#include "userachievements.h"
#include "userprofilemanager.h"
#include "skullmanager.h"
#include "liveVoice.h"

#include "grannymanager.h"

// xinput
#include "keyboard.h"

// xgameRender
#include "debugTextDisplay.h"
#include "render.h"
#include "camera.h"
#include "renderControl.h"
#include "flashallocator.h"
#include "D3DTextureManager.h"

// xmultiplayer
#include "LiveSystem.h"
#include "mpSimDataObject.h"
#include "mpGameSession.h"

// xsound
#include "soundmanager.h"

#include "memoryStats.h"

#include "visual.h"
#include "scoremanager.h"


const uint cMaxCharsPerLine = 100;


BDisplayStats::eDisplayStatMode BDisplayStats::mDisplayStatMode;
int                              BDisplayStats::mDisplayStatType[BDisplayStats::cNumberDisplayStatModes];
   
//==============================================================================
// BDisplayStats::showHeapStats
//==============================================================================
void BDisplayStats::showHeapStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   printKernelMemoryStats(textDisplay.getConsoleOutput());
   printHeapStats(textDisplay.getConsoleOutput(), true);
#endif
}

//==============================================================================
// BDisplayStats::showDetailedHeapStats
//==============================================================================
void BDisplayStats::showDetailedHeapStats(BDebugTextDisplay& textDisplay, bool totals, bool deltas)
{
#ifndef BUILD_FINAL      
   printKernelMemoryStats(textDisplay.getConsoleOutput());
         
   printDetailedHeapStats(textDisplay.getConsoleOutput(), totals, deltas);
   
   uint flashAllocations;
   GetFlashAllocationStats(flashAllocations);
   textDisplay.skipLine();
   textDisplay.printf("Scaleform Allocator Blocks: %u", flashAllocations);
#endif   
}

#if 0
//==============================================================================
// BDisplayStats::showTextureManagerStats
//==============================================================================
void BDisplayStats::showTextureManagerStats(BDebugTextDisplay& textDisplay, uint page)
{
#ifndef BUILD_FINAL      
   BTextureManager::BStats stats(gTextureManager.getStats(cTRTStatic));
   
   textDisplay.setColor(cColorWhite);
   textDisplay.printf("Texture Manager Statistics Page %i of 8: Total textures: %u", page + 1, stats.mTotalLive);
      
   textDisplay.printf("Not ready: %u, Ready: %u, Load Pending: %u, Load Failed: %u",
      stats.mStatusCount[cTextureStatusNotReady],
      stats.mStatusCount[cTextureStatusReady],
      stats.mStatusCount[cTextureStatusLoadPending],
      stats.mStatusCount[cTextureStatusLoadFailed]);
      
   textDisplay.printf("Total Textures: %u, Allocations: %u, Allocation Size: %u",
      stats.mTotalAllocStats.mTotalTextures,
      stats.mTotalAllocStats.mTotalAllocations,
      stats.mTotalAllocStats.mTotalAllocationSize);
   
   textDisplay.setColor(cColorGreen);      
   
   if (stats.mTotalAllocStats.mTotalAllocationSize)
   {
      textDisplay.printf("Base Size: %u (%2.1f%%), Mip Size: %u (%2.1f%%), Wasted Size: %u (%2.1f%%)",
         stats.mTotalAllocStats.mTotalBaseSize, stats.mTotalAllocStats.mTotalBaseSize*100.0f/stats.mTotalAllocStats.mTotalAllocationSize,
         stats.mTotalAllocStats.mTotalMipSize, stats.mTotalAllocStats.mTotalMipSize*100.0f/stats.mTotalAllocStats.mTotalAllocationSize,
         stats.mTotalAllocStats.mTotalWastedSize, stats.mTotalAllocStats.mTotalWastedSize*100.0f/stats.mTotalAllocStats.mTotalAllocationSize );
   }

   stats.mTracker.dump(textDisplay.getConsoleOutput(), page, 0);
#endif   
}
#endif

//==============================================================================
// BDisplayStats::showD3DTextureManagerStats
//==============================================================================
void BDisplayStats::showD3DTextureManagerStats(BDebugTextDisplay& textDisplay, uint page)
{
#ifndef BUILD_FINAL      
   const BD3DTextureAllocationStatsTracker& statsTracker = gD3DTextureManager.getStatTracker();

   textDisplay.setColor(cColorWhite);
   textDisplay.printf("D3D Texture Manager Statistics Page %i of 8", page + 1);
   
   BD3DTextureManager::BManagerStats managerStats;
   gD3DTextureManager.getManagerStats(managerStats);
         
   textDisplay.printf("Textures: %u Invalid: %u Initialized: %u Loaded: %u LoadingInBackgrnd: %u LoadFailed: %u",
      managerStats.mTotalTextures, 
      managerStats.mTotalState[BD3DTextureManager::BManagedTexture::cStatusInvalid], 
      managerStats.mTotalState[BD3DTextureManager::BManagedTexture::cStatusInitialized], 
      managerStats.mTotalState[BD3DTextureManager::BManagedTexture::cStatusLoaded], 
      managerStats.mTotalState[BD3DTextureManager::BManagedTexture::cStatusLoadingInBackground], 
      managerStats.mTotalState[BD3DTextureManager::BManagedTexture::cStatusLoadFailed]);
         
   textDisplay.setColor(cColorGreen);      
   
   statsTracker.dump(textDisplay.getConsoleOutput(), page, 0);
#endif   
}

#if 0
//==============================================================================
// BDisplayStats::showD3DTextureManagerStats
//==============================================================================
void BDisplayStats::showD3DTextureManagerDetailStats(BDebugTextDisplay& textDisplay, uint category, uint page)
{
#ifndef BUILD_FINAL      
   const BD3DTextureAllocationStatsTracker& statsTracker = gD3DTextureManager.getStatTracker();

   BString categoryName("Unknown");
   switch (category)
   {
      case BD3DTextureManager::cSystem:            categoryName.set("System"); break;
      case BD3DTextureManager::cUGXCommon:         categoryName.set("UGX Common"); break;
      case BD3DTextureManager::cUGXMaterial:       categoryName.set("UGX Material"); break;
      case BD3DTextureManager::cScaleformCommon:   categoryName.set("UI Shared"); break;
      case BD3DTextureManager::cScaleformPreGame:  categoryName.set("UI PreGame"); break;
      case BD3DTextureManager::cScaleformInGame:   categoryName.set("UI In Game"); break;
      case BD3DTextureManager::cUI:                categoryName.set("Non-Flash UI"); break;
      case BD3DTextureManager::cTerrainCommon:     categoryName.set("Terrain Common"); break;
      case BD3DTextureManager::cTerrainRibbon:     categoryName.set("Terrain Ribbon"); break;
      case BD3DTextureManager::cTerrainImpact:     categoryName.set("Terrain Impact"); break;
   };

   textDisplay.setColor(cColorWhite);
   textDisplay.printf("D3D Texture Manager Handles: %u, Detailed Statistics Page %i of 5", gD3DTextureManager.getTotalManagedTextureHandles(), page + 1);
   
   BD3DTextureManager::BDetailManagerStats managerStats;
   gD3DTextureManager.getDetailManagerStats(category, managerStats);
         
   textDisplay.printf("Category: %s Textures: %u Alloc: %u Invalid: %u Initialized: %u Loaded: %u Failed: %u",
      categoryName.getPtr(), managerStats.mTotalTextures, managerStats.mTotalAlloc, managerStats.mTotalState[0], managerStats.mTotalState[1], managerStats.mTotalState[2], managerStats.mTotalState[3]);
         
   textDisplay.setColor(cColorGreen);      
   
   statsTracker.dump(textDisplay.getConsoleOutput(), page, category);
#endif   
}
#endif

//==============================================================================
// BDisplayStats::showNetworkStats
//==============================================================================
void BDisplayStats::showNetworkStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   if (!gLiveSystem->isMultiplayerGameActive())
   {
      textDisplay.print("Multiplayer not active");
   }
   else
   {
      BMPSimDataObject* simobj = gLiveSystem->getMPSession()->getSimObject();
      if (!simobj)
         return;
               
      DWORD compensationAmount, compensationInterval;
      DWORD sendUpdateInterval, actualSendUpdateInterval, pingApprox, networkStall, totalSendInterval;
      BYTE sessionRecentTiming, localRecentTiming;

      simobj->getTimingCounters(compensationAmount, compensationInterval, sendUpdateInterval, actualSendUpdateInterval, pingApprox, networkStall, totalSendInterval, sessionRecentTiming, localRecentTiming);

      textDisplay.print("Multiplayer Stats:");
                  
      textDisplay.printf("      compensationAmount: %04d, compensationInterval: %04d, sendUpdateInterval: %04d", compensationAmount, compensationInterval, sendUpdateInterval);
      textDisplay.printf("actualSendUpdateInterval: %04d,           pingApprox: %04d,       networkStall: %04d", actualSendUpdateInterval, pingApprox, networkStall);
      textDisplay.printf("       totalSendInterval: %04d,  sessionRecentTiming: %04d,  localRecentTiming: %04d", totalSendInterval, sessionRecentTiming, localRecentTiming);
      
      textDisplay.skipLine(1);
                  
      if (!simobj->getMPGame())
         return;
         
      if (!simobj->getMPGame()->getSession())
         return;
         
      if (!simobj->getMPGame()->getSession()->getTimeSync())
         return;
         
//-- FIXING PREFIX BUG ID 3027
      const BTimeSync* pTimeSync = simobj->getMPGame()->getSession()->getTimeSync();
//--

      DWORD t = timeGetTime();
      textDisplay.printf("            Current time: %04u", t);
      textDisplay.printf("mEarliestAllowedRecvTime: %04u", pTimeSync->mEarliestAllowedRecvTime);
      textDisplay.printf("               mRecvTime: %04u", pTimeSync->mRecvTime);
      textDisplay.printf("           mLastRecvTime: %04u", pTimeSync->mLastRecvTime);
      textDisplay.printf("     mSendUpdateInterval: %04u", pTimeSync->mSendUpdateInterval);
      textDisplay.printf("     mRecvUpdateInterval: %04u", pTimeSync->mRecvUpdateInterval);
      textDisplay.printf("               mSendTime: %04u", pTimeSync->mSendTime);
      textDisplay.printf("            mTimeRolling: %04d", pTimeSync->mTimeRolling);
      textDisplay.printf("      mNetworkStallStart: %04u", pTimeSync->mNetworkStallStart);
      textDisplay.printf("         mUpdateRealTime: %04u", pTimeSync->mUpdateRealTime);
      textDisplay.printf("      mPingApproximation: %04u", pTimeSync->mPingApproximation);
      textDisplay.printf("     mActualSendInterval: %04u", pTimeSync->mActualSendInterval);
      textDisplay.printf("     mCompensationAmount: %04u", pTimeSync->mCompensationAmount);
      textDisplay.printf("   mCompensationInterval: %04u", pTimeSync->mCompensationInterval);
      textDisplay.printf("              mGameState: %04d", pTimeSync->mGameState);
      
      for (uint i = 0; i < XNetwork::cMaxClients; i++)
      {
         if (-1 == pTimeSync->mLastClientTimeHistory[i].getEarliest())
            break;
         textDisplay.printf("    mLastClientTime %02i: %04d", i, pTimeSync->mLastClientTimeHistory[i].getEarliest());
      }

      textDisplay.printf("           mLastSentTime: %04d", pTimeSync->mLastSentTime);

      textDisplay.skipLine();

      const BMemoryHeapStats& stats = gNetworkHeap.getStats();

      textDisplay.printf(
         "%s: CurNumAllocs: %i, CurAllocBytes: %i, MaxNumAllocs: %i, MaxAllocBytes: %i",
         stats.mName,
         stats.mCurrentAllocations,
         stats.mCurrentAllocationSize,
         stats.mMostAllocations,
         stats.mMostAllocationSize);

      textDisplay.printf("  TotalNews: %i, TotalDeletes: %i TotalReallocs: %i, TotalAllocBytes: %I64i",
         stats.mTotalNews,
         stats.mTotalDeletes,
         stats.mTotalReallocations,
         stats.mTotalAllocationSize);
   }      
#endif   
}

//==============================================================================
// BDisplayStats::showSoundStats
//==============================================================================
void BDisplayStats::showSoundStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   {
      textDisplay.print("Sound Stats:");

      unsigned long engineMem;
      long engineAllocs;
      unsigned long physMem;
      long physAllocs; 
      unsigned long virtMem; 
      long virtAllocs;

      gSoundManager.getMemoryInformation(&engineMem, &engineAllocs, &physMem, &physAllocs, &virtMem, &virtAllocs);
      textDisplay.printf("Engine Mem: %i, Engine Allocs: %i, Physical Mem: %i, Physical Allocs %i, Virt Mem: %i, Virt Allocs: %i", engineMem, engineAllocs, physMem, physAllocs, virtMem, virtAllocs);
      textDisplay.printf("Total Memory Use: %i, Total Allocations: %i", engineMem+physMem+virtMem, engineAllocs+physAllocs+virtAllocs);
      textDisplay.skipLine(1);

      if(gWorld && gWorld->getWorldSoundManager())
      {
         //-- Num World Sounds
         long numWorldSounds=0;
         numWorldSounds = gWorld->getWorldSoundManager()->getNumWorldSounds();
         textDisplay.printf("Num World Sounds: %i", numWorldSounds);

         textDisplay.skipLine(1);

         //-- Num Value 
         /*float allyCombatValue=gWorld->getBattleManager()->getNumHPInCombat(gUserManager.getPrimaryUser()->getPlayerID(), true, false);
         textDisplay.printf("\nAlly Combat Value: %f", allyCombatValue);
         
         float enemyCombatValue=gWorld->getBattleManager()->getNumHPInCombat(gUserManager.getPrimaryUser()->getPlayerID(), false, true);
         textDisplay.printf("\nEnemy Combat Value: %f", enemyCombatValue);*/

         if(gWorld->getWorldSoundManager()->getMusicManager())
         {
            BSimString currentState;
            gWorld->getWorldSoundManager()->getMusicManager()->getCurrentState(currentState);
            BSimString queuedState;
            gWorld->getWorldSoundManager()->getMusicManager()->getQueuedState(queuedState);
               
            DWORD switchTimer = gWorld->getWorldSoundManager()->getMusicManager()->getSwitchTimer();            

            textDisplay.printf("Current State: %s  --- Queued State: %s  --- Switch Timer: %d", currentState.getPtr(), queuedState.getPtr(), switchTimer);
         }
      }



      textDisplay.skipLine(1);
   }      
#endif   
}

//==============================================================================
// BDisplayStats::showPathingStats
//==============================================================================
void BDisplayStats::showPathingStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   {
      BFixedString<256> t;
      textDisplay.print("Pathing Stats:     -Average values are relative to only frames in which at least one findPath() was called.-");

      if(gWorld && gWorld->getPathingLimiter())
      {
         BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
         textDisplay.printf("Platoon Avg LRPs/Frame: %6.2f Platoon Max LRPs/Frame: %6d",
            pathLimiter->getAvePlatoonLRPperFrame(), pathLimiter->getMaxPlatoonLRPperFrame());
         textDisplay.printf("Platoon Avg SRPs/Frame: %6.2f Platoon Max SRPs/Frame: %6d",
            pathLimiter->getAvePlatoonSRPperFrame(), pathLimiter->getMaxPlatoonSRPperFrame());
         textDisplay.printf("Squad Avg SRPs/Frame:   %6.2f Squad Max SRPs/Frame:   %6d",
            pathLimiter->getAveSquadSRPperFrame(), pathLimiter->getMaxSquadSRPperFrame());
         textDisplay.skipLine();
         textDisplay.printf("Denied Paths:");
         textDisplay.printf("Player Platoon Denied Frames   Max Platoon Denied Frames   Squad Denied Frames   Max Squad Denied Frames");
         for (long n = 0; n < gWorld->getNumberPlayers(); n++)
         {
            textDisplay.printf("%3d    %6d                  %6d                      %6d                %6d", 
               n, pathLimiter->getPlatoonFramesDenied(n), pathLimiter->getMaxPlatoonFramesDenied(n), pathLimiter->getSquadFramesDenied(n), pathLimiter->getMaxSquadFramesDenied(n));
         }
      }
      textDisplay.skipLine();
      textDisplay.printf(cColorWhite, "SRP Calls/Update:   Avg: %6.3f        Max: %6d", gPather.mLLAverageCallsPerUpdate.getAverage(), gPather.mLLAverageCallsPerUpdate.getMaximum());
      textDisplay.printf(cColorWhite, "SRP Time/Call:      Avg: %6.3f        Max: %6.3f", gPather.mLLAverageTimePerCall.getAverage(), gPather.mLLAverageTimePerCall.getMaximum());
      textDisplay.skipLine();
      textDisplay.printf(cColorWhite, "HullArea Time/Call: Avg: %6.3f        Max: %6.3f", gPather.mHullAreaAverageTimePerCall.getAverage(), gPather.mHullAreaAverageTimePerCall.getMaximum());
      textDisplay.skipLine();
      textDisplay.printf(cColorWhite, "LRP Calls/Update:   Avg: %6.3f        Max: %6d", gPather.mHLAverageCallsPerUpdate.getAverage(), gPather.mHLAverageCallsPerUpdate.getMaximum());
      textDisplay.printf(cColorWhite, "LRP Time/Call:      Avg: %6.3f        Max: %6.3f", gPather.mHLAverageTimePerCall.getAverage(), gPather.mHLAverageTimePerCall.getMaximum());

   }      
#endif   
}

//==============================================================================
// BDisplayStats::showUpdateStats
//==============================================================================
void BDisplayStats::showUpdateStats(BDebugTextDisplay& textDisplay, BWorld* pWorld, BUserManager* pUserManager)
{
#ifndef BUILD_FINAL
   if ((!pWorld) || (!pUserManager))
      return;
   float sx=gUI.mfSafeX1;
   float sy=Math::Max(gUI.mfSafeY1, textDisplay.getPosY());
   float yh=gFontManager.getLineHeight();
   BHandle fontHandle=textDisplay.getFontHandle();
   BFixedString<256> t;
   BFixedString<256> t2;

   //Entity Numbers.
   t.format("Entities %u  Squads %u  Objects %u  Units %u  Projectiles %u  Dopples %u LaunchedProjectilesPerFrame %u", 
      pWorld->getNumberEntities(),
      pWorld->getNumberSquads(),
      pWorld->getNumberObjects(),
      pWorld->getNumberUnits(),
      pWorld->getNumberProjectiles(),
      pWorld->getNumberDopples(),
      pWorld->getNumberProjectilesLaunchedThisFrame()
      );
   gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
   sy+=yh;
   t.format("\n");
   gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
   sy+=yh;
#endif

#ifdef UNITOFFENDERS
   //Rip through the offenders.
   BOffenderArray offenders;
   pWorld->getUnitOffenders(offenders);
   for (uint i=0; i < offenders.getSize(); i++)
   {
      const BUnit* pUnit=pWorld->getUnit(offenders[i].mID);
      if (!pUnit || !pUnit->getProtoObject())
         t.format("%8.3f: Invalid Unit %u.", offenders[i].mTime*1000.0f, offenders[i].mID.asLong());
      else
         t.format("%8.3f: Unit %u(%s).", offenders[i].mTime*1000.0f, pUnit->getID().asLong(), pUnit->getProtoObject()->getName().getPtr());
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
      sy+=yh;
   }

#endif
}


//==============================================================================
// BDisplayStats::showObjectStats
//==============================================================================
void BDisplayStats::showObjectStats(BDebugTextDisplay& textDisplay, BWorld* pWorld, BUserManager* pUserManager)
{
#ifndef BUILD_FINAL
   if ((!pWorld) || (!pUserManager))
      return;
      
   float sx=gUI.mfSafeX1;
   float sy=Math::Max(gUI.mfSafeY1, textDisplay.getPosY());
   float yh=gFontManager.getLineHeight();
      
   BHandle fontHandle = textDisplay.getFontHandle();
   BFixedString<256> t, t2;
   //BUString t, t2;

   if(gConfig.isDefined(cConfigDebugSelectionPicking))
   {
      //-- Unit picking (selection) data
      t.format("DebugSelectionPicking  Left/Right: +-PickRadius, Up/Down: +-PickOffset");
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
      sy += yh;
      t.format(" CtrlLeft/Right:+-SelRadX, CtrlUp/Down:+-SelRadZ");
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
      sy += yh;
      t.format(" AltLeft/Right: +-ObsRadX, AltUp/Down: +-ObsRadZ, AltPgUp/PgDn: +=ObsRadY");
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
      sy += yh;
      BEntityID selectedID=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverObject();
//-- FIXING PREFIX BUG ID 3028
      const BObject* pObject=pWorld->getObject(selectedID);
//--
      if(pObject)
      {
         const BProtoObject* pProtoObject=pObject->getProtoObject();
         t.format("%s", pProtoObject->getName().getPtr());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDYellow);
         sy += yh;
         t.format("PickRadius=%.1f, PickOffset=%.1f, PickPriority=%s", pProtoObject->getPickRadius(), pProtoObject->getPickOffset(), gDatabase.getPickPriorityName(pProtoObject->getPickPriority()));
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
         sy += yh;
         t.format("SelectedRadiusX=%.1f, SelectedRadiusZ=%.1f", pProtoObject->getSelectedRadiusX(), pProtoObject->getSelectedRadiusZ());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
         sy += yh;
         t.format("ObstructionRadiusX=%.1f, ObstructionRadiusY=%.1f, ObstructionZ=%.1f", pProtoObject->getObstructionRadiusX(), pProtoObject->getObstructionRadiusY(), pProtoObject->getObstructionRadiusZ());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
         sy += yh;
      }
      else
      {
         t.format("No object");
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDYellow);
         sy += yh;
      }
   }
   else
   {
      //-- Entity stats
      t.format("Entities %u  Squads %u  Objects %u  Units %u  Projectiles %u  Dopples %u LaunchedProjectilesPerFrame %u", 
         pWorld->getNumberEntities(),
         pWorld->getNumberSquads(),
         pWorld->getNumberObjects(),
         pWorld->getNumberUnits(),
         pWorld->getNumberProjectiles(),
         pWorld->getNumberDopples(),
         pWorld->getNumberProjectilesLaunchedThisFrame()
         );
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
      sy += yh;

      //-- Number of units selected and group health percent
      BSelectionManager* pSelectionMgr=pUserManager->getUser(BUserManager::cPrimaryUser)->getSelectionManager();
      long selectedCount=pSelectionMgr->getNumberSelectedUnits();
      if(selectedCount>0)
      {
         long unitCount=0;
         float unitHealth=0.0f;
         float totalUnitHPs = 0.0f;
         float totalMaxUnitHPs = 0.0f;
         const BEntityIDArray& selectedUnits=pSelectionMgr->getSelectedUnits();
         for(long i=0; i<selectedUnits.getNumber(); i++)
         {
//-- FIXING PREFIX BUG ID 3030
            const BUnit* pUnit=gWorld->getUnit(selectedUnits[i]);
//--
            if(pUnit)
            {
               float unitHPPct = pUnit->getHPPercentage();
               unitHealth += unitHPPct;
               unitCount++;

               // Raw hitpoints.  There isn't a function to return total HPs that accounts for garrisoned units like the HP percent
               // and max HPs functions do.  So calculate it from those two values.
               float maxHP = pUnit->getHPMax();
               totalUnitHPs += (unitHPPct * maxHP);
               totalMaxUnitHPs += maxHP;
            }
         }
         if(unitCount>0)
         {
            t.format("Selection count %d  Group health %.1f%%  Total HPs %.1f (%.1f)", selectedCount, unitHealth/unitCount*100.0f, totalUnitHPs, totalMaxUnitHPs);
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDYellow);
            sy += yh;
         }
      }

      //-- Current object/unit/squad stats
      BEntityID selectedID;
      if(selectedCount>0)
         selectedID=pSelectionMgr->getSelected(0);
      else
         selectedID=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverObject();
      if(selectedID!=cInvalidObjectID)
      {
         BObject* pObject=pWorld->getObject(selectedID);
         if(pObject)
         {
            BVisual* pVisual=pObject->getVisual();
            if(pVisual)
            {
//-- FIXING PREFIX BUG ID 3029
               const BProtoVisual* pProtoVisual=pVisual->getProtoVisual();
//--
               if(pProtoVisual)
               {
                  long animType=pVisual->getAnimationType(cActionAnimationTrack);
                  float animLen=pVisual->getAnimationDuration(cActionAnimationTrack);
                  float animPos=pVisual->getAnimationPosition(cActionAnimationTrack);
                  float animPct=(animLen==0.0f?0.0f:animPos/animLen);
                  float animClock=pVisual->getAnimationClock(cActionAnimationTrack);
                  t.format("Visual %s Anim %s (len=%.1f pos=%.1f pct=%.2f, clk=%.1f)", pProtoVisual->getName().getPtr(), gVisualManager.getAnimName(animType), animLen, animPos, animPct, animClock);
                  gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
                  sy += yh;
               }
            }

            BSquad* pSquad=pWorld->getSquad(pObject->getParentID());
            if(pSquad)
            {
               const BProtoSquad* pProtoSquad=pSquad->getProtoSquad();
               const BProtoSquad* pBaseProtoSquad=gDatabase.getGenericProtoSquad(pProtoSquad->getBaseType());
               t.format("Squad %d %s, mode %s, BaseDPS: %.1f, DmgType: %s, Level: %d, XP: %.1f", pSquad->getID().asLong(), pProtoSquad ? pProtoSquad->getName().getPtr() : "", gDatabase.getSquadModeName(pSquad->getSquadMode()), pBaseProtoSquad->getAttackGradeDPS(), gDatabase.getDamageTypeName(pBaseProtoSquad->getDamageType()), pSquad->getLevel(), pSquad->getXP());
               gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
               sy += yh;
               t="AttackRatingDPS:";
               uint numDamageTypes = gDatabase.getNumberAttackRatingDamageTypes();
               for (uint i=0; i<numDamageTypes; i++)
               {
                  t2.format(" %s %.1f", gDatabase.getDamageTypeName(static_cast<BDamageTypeID>(i)), pProtoSquad->getAttackRatingDPS(gDatabase.getAttackRatingDamageType(i)));
                  t+=t2;

                  // Line too long so print it and start a new line
                  if ((t.getLen() > cMaxCharsPerLine) && (i < (numDamageTypes - 1)))
                  {
                     gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
                     sy += yh;
                     t = "  ";
                  }
               }
               gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
               sy += yh;
               long nodeCount=(pProtoSquad ? pProtoSquad->getNumberUnitNodes() : 1);
               for(long j=0; j<nodeCount; j++)
               {
                  long protoID=(pProtoSquad ? pProtoSquad->getUnitNode(j).mUnitType : pObject->getProtoObject()->getBaseType());
                  const BProtoObject* pPlayerProtoObject=pSquad->getPlayer()->getProtoObject(protoID);
                  const BProtoObject* pBaseProtoObject=gDatabase.getGenericProtoObject(protoID);
                  t.format("%s Hp=%.1f(%.1f), Sp=%.1f(%.1f), Amo=%.1f(%.1f), LOS=%.1f(%.1f), Spd=%.1f(%.1f), BldPts=%.1f(%.1f)", 
                     pPlayerProtoObject->getName().getPtr(),
                     pPlayerProtoObject->getHitpoints(), pBaseProtoObject->getHitpoints(), 
                     pPlayerProtoObject->getShieldpoints(), pBaseProtoObject->getShieldpoints(), 
                     pPlayerProtoObject->getMaxAmmo(), pBaseProtoObject->getMaxAmmo(),
                     pPlayerProtoObject->getProtoLOS(), pBaseProtoObject->getProtoLOS(),
                     pPlayerProtoObject->getDesiredVelocity(), pBaseProtoObject->getDesiredVelocity(),
                     pPlayerProtoObject->getBuildPoints(), pBaseProtoObject->getBuildPoints());
                  gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
                  sy += yh;
                  t.format("  BaseDPS: %.1f, AttackRatingDPS:", pBaseProtoObject->getAttackGradeDPS());
                  for (uint i=0; i<numDamageTypes; i++)
                  {
                     t2.format(" %s %.1f", gDatabase.getDamageTypeName((int8)i), pPlayerProtoObject->getAttackRatingDPS(gDatabase.getAttackRatingDamageType(i)));
                     t+=t2;

                     // Line too long so print it and start a new line
                     if ((t.getLen() > cMaxCharsPerLine) && (i < (numDamageTypes - 1)))
                     {
                        gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
                        sy += yh;
                        t = "    ";
                     }
                  }
                  gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);

                  // Movement/Pathing information
                  const BSquadActionMove* pSquadActionMove = reinterpret_cast<const BSquadActionMove*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadMove));
                  if (pSquadActionMove)
                  {
                     sy +=yh;
                     uint32 pathingDelays = pSquadActionMove->getNumPathingDelays();
                     uint32 maxPathingDelays = pSquadActionMove->getMaxPathingDelays();
                     t.format("Curr Squad pathing delays: %d  Max Consecutive Squad pathing delays: %d", pathingDelays, maxPathingDelays);
                     gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
                  }

                  sy += yh;
                  //float lw=gFontManager.getLineLength("Unit XXXXXX Hp=XXXXX.X Sp=XXXX.X Amo=XXX.X RngX=X.X DmgX=X.X", 77);
                  //long unitCount=0;
                  long childCount=pSquad->getNumberChildren();
                  //for(long i=0; i<childCount; i++)
                  //{
                  //   const BUnit* pUnit=pWorld->getUnit(pSquad->getChild(i));
                  //   if(pUnit && pUnit->getProtoID()==protoID)
                  //      unitCount++;
                  //}
                  //long halfUnitCount=unitCount/2;
                  //if(halfUnitCount*2<unitCount)
                  //   halfUnitCount++;

                  //long counter=0;
                  for(long i=0; i<childCount; i++)
                  {
                     const BUnit* pUnit=pWorld->getUnit(pSquad->getChild(i));
                     if(pUnit && pUnit->getProtoID()==protoID)
                     {
                        t.format("  Unit %d Hp=%.2f Sp=%.2f Amo=%.2f V=%.2f RngX=%.2f DmgX=%.2f AccX=%.2f DmgRcvX=%.2f VelX=%.2f WorkX=%.2f", pUnit->getID().asLong(), pUnit->getHitpoints(), pUnit->getShieldpoints(), pUnit->getAmmunition(), pUnit->getVelocity().length(), pUnit->getWeaponRangeScalar(), pUnit->getDamageModifier(), pUnit->getAccuracyScalar(), pUnit->getDamageTakenScalar(), pUnit->getVelocityScalar(), pUnit->getWorkRateScalar());
                        //float x=(counter>=halfUnitCount ? sx+lw : sx);
                        //float y=(counter>=halfUnitCount ? sy+((counter-halfUnitCount)*yh) : sy+(counter*yh));
                        //gFontManager.drawText(fontHandle, sx, sy, t, cDWORDYellow);
                        gFontManager.drawText(fontHandle, sx, sy, t, cDWORDYellow);
                        sy += yh;
                        //counter++;
                     }
                  }
                  //sy += (halfUnitCount*yh);
                  BTactic* pTactic=pPlayerProtoObject->getTactic();
                  BTactic* pBaseTacitc=pBaseProtoObject->getTactic();
                  if(pTactic && pBaseTacitc)
                  {
                     //Weapons
                     for(long i=0; i<pTactic->getNumberWeapons(); i++)
                     {
                        const BWeapon* pWeapon=pTactic->getWeapon(i);
                        const BWeapon* pBaseWeapon=pBaseTacitc->getWeapon(i);
                        if(pWeapon && pBaseWeapon)
                        {
                            t.format("  Weapon %s MinRng=%.1f(%.1f), MaxRng=%.1f(%.1f), DPS=%.2f(%.2f)", 
                              pWeapon->mpStaticData->mName.getPtr(),                               
                              pWeapon->mMinRange, pBaseWeapon->mMinRange, 
                              pWeapon->mMaxRange, pBaseWeapon->mMaxRange, 
                              pWeapon->mDamagePerSecond, pBaseWeapon->mDamagePerSecond);

                            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
                            sy += yh;
                            t.format("    AOE Radius=%.1f(%.1f), Primary=%.2f(%.1f), Distance=%.1f(%.1f), Damage=%.2f(%.1f)", 
                              pWeapon->mAOERadius, pBaseWeapon->mAOERadius, 
                              pWeapon->mAOEPrimaryTargetFactor, pBaseWeapon->mAOEPrimaryTargetFactor, 
                              pWeapon->mAOEDistanceFactor, pBaseWeapon->mAOEDistanceFactor, 
                              pWeapon->mAOEDamageFactor, pBaseWeapon->mAOEDamageFactor);
                            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
                            sy += yh;
                         }
                         else
                            t.format("  Weapon Not Defined");
                     }

                     //Actions
                     for(long i=0; i<pTactic->getNumberProtoActions(); i++)
                     {
                        const BProtoAction* pProtoAction=pTactic->getProtoAction(i);
                        const BProtoAction* pBaseProtoAction=pBaseTacitc->getProtoAction(i);

                        
                        if((pProtoAction && pProtoAction->getWorkRate()>0.0f) || (pBaseProtoAction && pBaseProtoAction->getWorkRate()>0.0f))
                        {

                           t.format("  ProtoAction %s WorkRate=%.2f(%.2f)", (pProtoAction ? pProtoAction->getName().getPtr() : "Unknown"), (pProtoAction ? pProtoAction->getWorkRate() : 0.0f), (pBaseProtoAction ? pBaseProtoAction->getWorkRate() : 0.0f));                        
                              
                           BFixedString<128> dpaString;
                           dpaString.format(" DPA(animType=%d) = %.2f", pProtoAction->getAnimType(), pProtoAction->getDamagePerAttack());
                           t.append(dpaString); 
                           
                           gFontManager.drawText(fontHandle, sx, sy, t, cDWORDRed);
                           sy += yh;
                        }
                     }
                  }
               }

               //-- Display the squad's actual dps
               #ifdef DPS_TRACKER
               BSquadActionAttack* pSquadAttackAction = (BSquadActionAttack*)pSquad->getActionByType(BAction::cActionTypeSquadAttack);
               if(pSquadAttackAction)
               {
                  t.format("Squads recorded DPS over the last %d seconds: %f", BUnit::cMaxDamageHistory, pSquadAttackAction->getRealDPS());
                  gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
                  sy += yh;
               }
               #endif

               //-- Display the squad's damage bank
               t.format("Squads current damage bank: %f", pSquad->getDamageBank());
               gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
               sy += yh;
            }
            else
            {
               const BProtoObject* pProtoObject=pObject->getProtoObject();
               BUnit* pUnit=pObject->getUnit();
               if(pUnit)
               {
                  t.format("Unit %d %s Hp=%.1f/%.1f, LOS=%.1f, Resources=%.1f", pUnit->getID().asLong(), pProtoObject->getName().getPtr(), 
                     pUnit->getHitpoints(), pProtoObject->getHitpoints(), pUnit->getLOS(), pUnit->getResourceAmount());
                  gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
                  sy += yh;
               }
               else
               {
                  t.format("Object %d %s", pObject->getID().asLong(), pProtoObject->getName().getPtr());
                  gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
                  sy += yh;
               }
            }

            // Entity ref stuff.
            uint numEntityRefs = pObject->getNumberEntityRefs();
            t.format("Number Entity Refs: %d", numEntityRefs);
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
            sy += yh;
            BSimString erString;
            for (uint er=0; er<numEntityRefs; er++)
            {
               BEntityRef* pEntityRef = pObject->getEntityRefByIndex(er);
               if (pEntityRef)
               {
                  pEntityRef->createDebugString(erString);
                  gFontManager.drawText(fontHandle, sx, sy, erString, cDWORDGreen);
                  sy += yh;
               }               
            }

            // Parent socket entity refs (since you can't point at the building's socket because it's turned off once a building is built on it).
//-- FIXING PREFIX BUG ID 3031
            const BEntityRef* pSocketRef = pObject->getFirstEntityRefByType(BEntityRef::cTypeParentSocket);
//--
            if (pSocketRef)
            {
               BObject* pSocket = gWorld->getObject(pSocketRef->mID);
               if (pSocket)
               {
                  numEntityRefs = pSocket->getNumberEntityRefs();
                  t.format("Parent Socket Number Entity Refs: %d", numEntityRefs);
                  gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
                  sy += yh;
                  BSimString erString;
                  for (uint er=0; er<numEntityRefs; er++)
                  {
                     BEntityRef* pEntityRef = pSocket->getEntityRefByIndex(er);
                     if (pEntityRef)
                     {
                        pEntityRef->createDebugString(erString);
                        gFontManager.drawText(fontHandle, sx, sy, erString, cDWORDCyan);
                        sy += yh;
                     }               
                  }
               }
            }
         }
      }
   }
#endif
}   

//==============================================================================
// BDisplayStats::showFPS
//==============================================================================
void BDisplayStats::showFPS(BDebugTextDisplay& textDisplay, BWorld* pWorld, BTimingTracker* pTimingTracker)
{
#ifndef BUILD_FINAL
   if ((!pWorld) || (!pTimingTracker))
      return;
      
   // FPS
   double gameTime=pWorld->getGametimeFloat();
   double realTime=pWorld->getTotalRealtime();
   double diff=gameTime-realTime;
   DWORD frameLen=pTimingTracker->getLastFrameLength();
   DWORD frameAvg=pTimingTracker->getFrameAverage();
   DWORD updateLen=pTimingTracker->getLastUpdateLength();
   DWORD updateAvg=pTimingTracker->getUpdateAverage();
   DWORD outsideUpdateAvg=pTimingTracker->getOutsideUpdateAverage();
   DWORD totalLen=frameLen+updateLen;
   DWORD totalAvg=frameAvg+updateAvg;
   float fps=(totalLen>0 ? 1000.0f/(float)totalLen : 1000.0f);
   float fpsAvg=(totalAvg>0 ? 1000.0f/(float)totalAvg : 1000.0f);

   float gameSpeed=1.0f;
   gConfig.get(cConfigGameSpeed, &gameSpeed);

   textDisplay.printf(cColorGreen, "Actual FPS Ave: %.2f, Time/Frame Ave: %.2fms, Time/Frame Peak: %.2fms", gRender.getAverageFPS(), gRender.getAverageFrameTime() * 1000.0f, gRender.getMaxFrameTime() * 1000.0f);

   textDisplay.printf(cColorGreen, "Timing Tracker: FPS Ave: %.1f Cur: %.1f, UpdateNumber: %04u, Total Ave: %04u Cur: %04u, Frame Ave: %04u Cur: %04u", 
      fpsAvg, fps, gWorld->getUpdateNumber(), 
      totalAvg, totalLen, 
      frameAvg, frameLen);

   textDisplay.printf(cColorGreen, " Update Avg: %04u Cur: %04u Outside: %04u, GameTime: %.1f, Diff: %.1f, GameSpeed: %.1f, SubUpdating: %d", 
      updateAvg, updateLen, outsideUpdateAvg,
      gameTime, diff, gameSpeed, (int)gEnableSubUpdating);
      
   BFlashManager::BStats flashStats;
   gFlashManager.getSimStats(flashStats);
   textDisplay.printf((flashStats.mASInvokesTotalTime > .002f) ? cColorRed : cColorGreen, 
      "Scaleform Invokes: %u, CPUInvokeTime: %3.2fms, Movies: %u, Decal: %u, CPUAdvance: %3.2fms, CPUDisp: %3.2fms",
      flashStats.mASInvokesPerFrame, 
      flashStats.mASInvokesTotalTime * 1000.0f, 
      flashStats.mMoviesRenderedPerFrame,
      flashStats.mDecalMoviesRenderedPerFrame,
      flashStats.mCPUTimeAdvance,
      flashStats.mCPUTimeDisplay);
#endif
}

//==============================================================================
// BDisplayStats::showStats
//==============================================================================
void BDisplayStats::showRenderStats(BDebugTextDisplay& textDisplay, uint page, BUserManager* pUserManager)
{
   if ((page == 0) && (pUserManager))
   {
      // Camera settings
      BUser* pUser=pUserManager->getUser(BUserManager::cPrimaryUser);
      if (pUser)
      {
         BVector hover=(pUser->getFlagHaveHoverPoint()?pUser->getHoverPoint():cInvalidVector);
         BVector cameraHover=(pUser->getFlagHaveHoverPoint()?pUser->getCameraHoverPoint():cInvalidVector);
         float cameraHeight=-1.0f;
         gTerrainSimRep.getCameraHeightRaycast(pUser->getCameraHoverPoint()+BVector(0.0f,1.0f,0.0f), cameraHeight, true);
         float zoomDiff=pUser->getCamera()->getCameraLoc().distance(hover)-pUser->getCameraZoom();
         textDisplay.printf("Camera: (%3.2f, %3.2f, %3.2f, Pitch=%.1f Yaw=%.1f Zoom=%.1f FOV=%.1f ZoomDiff=%.1f Limit=%d", 
                            pUser->getCamera()->getCameraLoc().x,
                            pUser->getCamera()->getCameraLoc().y,
                            pUser->getCamera()->getCameraLoc().z,
                            pUser->getCameraPitch(), pUser->getCameraYaw(), pUser->getCameraZoom(), pUser->getCameraFOV(), zoomDiff, (long)!pUser->getFlagNoCameraLimits());

         textDisplay.printf("SimHover: %.1f, %.1f, %.1f (0x%08X, 0x%08X, 0x%08X) CameraHover: %.1f, %.1f, %.1f CameraHeight: %.1f", 
                            hover.x, hover.y, hover.z, 
                            *(DWORD*)&hover.x, *(DWORD*)&hover.y, *(DWORD*)&hover.z,
                            cameraHover.x, cameraHover.y, cameraHover.z, cameraHeight);
      }                            
   }                         

   gRenderControl.showRenderStats(textDisplay, page);
}

//==============================================================================
// BDisplayStats::showWorldStats
//==============================================================================
void BDisplayStats::showWorldStats(BDebugTextDisplay& textDisplay, BWorld* pWorld)
{
#ifndef BUILD_FINAL
   if (!pWorld)
      return;

   const BWorld::BStats &stats = pWorld->getTimeStats();

   textDisplay.skipLine();
   textDisplay.printf(cColorWhite, "World Stats");   
   textDisplay.printf(cColorWhite, "Total PreAsyncUpd: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mTotalPreAsyncUpdateTime * 1000.0f, stats.mAvgTotalPreAsyncUpdateTime.getAverage() * 1000.0f, stats.mMaxTotalPreAsyncUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Total AsyncUpdate: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mTotalAsyncUpdateTime * 1000.0f, stats.mAvgTotalAsyncUpdateTime.getAverage() * 1000.0f, stats.mMaxTotalAsyncUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Total PostAsyncUpd:%8.3f  Avg: %8.3f  Max: %8.3f", stats.mTotalPostAsyncUpdateTime * 1000.0f, stats.mAvgTotalPostAsyncUpdateTime.getAverage() * 1000.0f, stats.mMaxTotalPostAsyncUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Total SubUpdate1:  %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSubUpdate1Time * 1000.0f, stats.mAvgSubUpdate1Time.getAverage() * 1000.0f, stats.mMaxSubUpdate1Time * 1000.0f);
   textDisplay.printf(cColorWhite, "Total SubUpdate2:  %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSubUpdate2Time * 1000.0f, stats.mAvgSubUpdate2Time.getAverage() * 1000.0f, stats.mMaxSubUpdate2Time * 1000.0f);
   textDisplay.printf(cColorWhite, "Total SubUpdate3:  %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSubUpdate3Time * 1000.0f, stats.mAvgSubUpdate3Time.getAverage() * 1000.0f, stats.mMaxSubUpdate3Time * 1000.0f);
   textDisplay.printf(cColorWhite, "Total Update:      %8.3f  Avg: %8.3f  Max: %8.3f", stats.mTotalUpdateTime * 1000.0f, stats.mAvgTotalUpdateTime.getAverage() * 1000.0f, stats.mMaxTotalUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Total Sim Render:  %8.3f  Avg: %8.3f  Max: %8.3f", stats.mTotalSimRenderTime * 1000.0f, stats.mAvgSimRenderTime.getAverage() * 1000.0f, stats.mMaxSimRenderTime * 1000.0f);
   textDisplay.skipLine();
   textDisplay.printf(cColorWhite, "Command Proc Pre:  %8.3f  Avg: %8.3f  Max: %8.3f", stats.mCommandManagerUpdateTime * 1000.0f , stats.mAvgCommandManagerUpdateTime.getAverage() * 1000.0f,  stats.mMaxCommandManagerUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Command Serv Pre:  %8.3f  Avg: %8.3f  Max: %8.3f", stats.mCommandManagerServiceTime * 1000.0f, stats.mAvgCommandManagerServiceTime.getAverage() * 1000.0f, stats.mMaxCommandManagerServiceTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Team Pre:          %8.3f  Avg: %8.3f  Max: %8.3f", stats.mTeamPreAsyncTime * 1000.0f, stats.mAvgTeamPreAsyncTime.getAverage() * 1000.0f, stats.mMaxTeamPreAsyncTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Player Pre:        %8.3f  Avg: %8.3f  Max: %8.3f", stats.mPlayerPreAsyncTime * 1000.0f, stats.mAvgPlayerPreAsyncTime.getAverage() * 1000.0f, stats.mMaxPlayerPreAsyncTime * 1000.0f);
   textDisplay.printf(cColorWhite, "AI Pre:            %8.3f  Avg: %8.3f  Max: %8.3f", stats.mAIPreAsyncTime * 1000.0f, stats.mAvgAIPreAsyncTime.getAverage() * 1000.0f, stats.mMaxAIPreAsyncTime * 1000.0f);
   textDisplay.printf(cColorWhite, "KB Pre:            %8.3f  Avg: %8.3f  Max: %8.3f", stats.mKBPreAsyncTime * 1000.0f, stats.mAvgKBPreAsyncTime.getAverage() * 1000.0f, stats.mMaxKBPreAsyncTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Army Pre:          %8.3f  Avg: %8.3f  Max: %8.3f   Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mArmyPreAsyncTime * 1000.0f, stats.mAvgArmyPreAsyncTime.getAverage() * 1000.0f, stats.mMaxArmyPreAsyncTime * 1000.0f, stats.mArmyUpdateTime * 1000.0f, stats.mAvgArmyUpdateTime.getAverage() * 1000.0f, stats.mMaxArmyUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Platoon Pre:       %8.3f  Avg: %8.3f  Max: %8.3f   Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mPlatoonPreAsyncTime * 1000.0f, stats.mAvgPlatoonPreAsyncTime.getAverage() * 1000.0f, stats.mMaxPlatoonPreAsyncTime * 1000.0f, stats.mPlatoonUpdateTime * 1000.0f, stats.mAvgPlatoonUpdateTime.getAverage() * 1000.0f, stats.mMaxPlatoonUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Squad Pre:         %8.3f  Avg: %8.3f  Max: %8.3f   Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSquadPreAsyncTime * 1000.0f, stats.mAvgSquadPreAsyncTime.getAverage() * 1000.0f, stats.mMaxSquadPreAsyncTime * 1000.0f, stats.mSquadUpdateTime * 1000.0f, stats.mAvgSquadUpdateTime.getAverage() * 1000.0f, stats.mMaxSquadUpdateTime * 1000.0f);
   textDisplay.printf(cColorGreen, "   Entity:         %8.3f  Avg: %8.3f  Max: %8.3f", BSquad::mStats.mUpdateEntityTime * 1000.0f, BSquad::mStats.mAvgEntityTime.getAverage() * 1000.0f, BSquad::mStats.mMaxEntityTime * 1000.0f);
   textDisplay.printf(cColorGreen, "   Formation:      %8.3f  Avg: %8.3f  Max: %8.3f", BSquad::mStats.mUpdateFormationTime * 1000.0f, BSquad::mStats.mAvgFormationTime.getAverage() * 1000.0f, BSquad::mStats.mMaxFormationTime * 1000.0f);
   textDisplay.printf(cColorGreen, "   Garr Squad:     %8.3f  Avg: %8.3f  Max: %8.3f", BSquad::mStats.mUpdateGarrisonedSquadsTime * 1000.0f, BSquad::mStats.mAvgGarrisonedSquadsTime.getAverage() * 1000.0f, BSquad::mStats.mMaxGarrisonedSquadsTime * 1000.0f);
   textDisplay.printf(cColorGreen, "   Hitched Squad:  %8.3f  Avg: %8.3f  Max: %8.3f", BSquad::mStats.mUpdateHitchedSquadTime * 1000.0f, BSquad::mStats.mAvgHitchedSquadTime.getAverage() * 1000.0f, BSquad::mStats.mMaxHitchedSquadTime * 1000.0f);
   textDisplay.printf(cColorGreen, "   Leash:          %8.3f  Avg: %8.3f  Max: %8.3f", BSquad::mStats.mUpdateLeashTime * 1000.0f, BSquad::mStats.mAvgLeashTime.getAverage() * 1000.0f, BSquad::mStats.mMaxLeashTime * 1000.0f);
   textDisplay.printf(cColorGreen, "   Orders:         %8.3f  Avg: %8.3f  Max: %8.3f", BSquad::mStats.mUpdateOrdersTime * 1000.0f, BSquad::mStats.mAvgOrdersTime.getAverage() * 1000.0f, BSquad::mStats.mMaxOrdersTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Projectile Pre:    %8.3f  Avg: %8.3f  Max: %8.3f   Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mProjectilePreAsyncTime * 1000.0f, stats.mAvgProjectilePreAsyncTime.getAverage() * 1000.0f, stats.mMaxProjectilePreAsyncTime * 1000.0f, stats.mProjectileUpdateTime * 1000.0f, stats.mAvgProjectileUpdateTime.getAverage() * 1000.0f, stats.mMaxProjectileUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Unit Pre:          %8.3f  Avg: %8.3f  Max: %8.3f   Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mUnitPreAsyncTime * 1000.0f, stats.mAvgUnitPreAsyncTime.getAverage() * 1000.0f, stats.mMaxUnitPreAsyncTime * 1000.0f, stats.mUnitUpdateTime * 1000.0f, stats.mAvgUnitUpdateTime.getAverage() * 1000.0f, stats.mMaxUnitUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Dopple Pre:        %8.3f  Avg: %8.3f  Max: %8.3f   Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mDopplePreAsyncTime * 1000.0f, stats.mAvgDopplePreAsyncTime.getAverage() * 1000.0f, stats.mMaxDopplePreAsyncTime * 1000.0f, stats.mDoppleUpdateTime * 1000.0f, stats.mAvgDoppleUpdateTime.getAverage() * 1000.0f, stats.mMaxDoppleUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Object Pre:        %8.3f  Avg: %8.3f  Max: %8.3f   Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mObjectPreAsyncTime * 1000.0f, stats.mAvgObjectPreAsyncTime.getAverage() * 1000.0f, stats.mMaxObjectPreAsyncTime * 1000.0f, stats.mObjectUpdateTime * 1000.0f, stats.mAvgObjectUpdateTime.getAverage() * 1000.0f, stats.mMaxObjectUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Entity Sched Post: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mEntitySchedulerUpdateTime * 1000.0f, stats.mAvgEntitySchedulerUpdateTime.getAverage() * 1000.0f, stats.mMaxEntitySchedulerUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Physics Post:      %8.3f  Avg: %8.3f  Max: %8.3f", stats.mPhysicsUpdateTime * 1000.0f, stats.mAvgPhysicsUpdateTime.getAverage() * 1000.0f, stats.mMaxPhysicsUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "TextVisual Update: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mTextVisualUpdateTime * 1000.0f, stats.mAvgTextVisualUpdateTime.getAverage() * 1000.0f, stats.mMaxTextVisualUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Sound Post:        %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSoundUpdateTime * 1000.0f, stats.mAvgSoundUpdateTime.getAverage() * 1000.0f, stats.mMaxSoundUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Battle Post:       %8.3f  Avg: %8.3f  Max: %8.3f", stats.mBattleUpdateTime * 1000.0f, stats.mAvgBattleUpdateTime.getAverage() * 1000.0f, stats.mMaxBattleUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Power Post:        %8.3f  Avg: %8.3f  Max: %8.3f", stats.mPowerUpdateTime * 1000.0f, stats.mAvgPowerUpdateTime.getAverage() * 1000.0f, stats.mMaxPowerUpdateTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Trigger Post:      %8.3f  Avg: %8.3f  Max: %8.3f", stats.mTriggerUpdateTime * 1000.0f, stats.mAvgTriggerUpdateTime.getAverage() * 1000.0f, stats.mMaxTriggerUpdateTime * 1000.0f);
   textDisplay.skipLine();
   textDisplay.printf(cColorWhite, "Objects SimRender: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSimRenderObjectsTime * 1000.0f, stats.mAvgSimRenderObjectsTime.getAverage() * 1000.0f, stats.mMaxSimRenderObjectsTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Units SimRender:   %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSimRenderUnitsTime * 1000.0f, stats.mAvgSimRenderUnitsTime.getAverage() * 1000.0f, stats.mMaxSimRenderUnitsTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Squads SimRender:  %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSimRenderSquadsTime * 1000.0f, stats.mAvgSimRenderSquadsTime.getAverage() * 1000.0f, stats.mMaxSimRenderSquadsTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Proj SimRender:    %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSimRenderProjectilesTime * 1000.0f, stats.mAvgSimRenderProjectilesTime.getAverage() * 1000.0f, stats.mMaxSimRenderProjectilesTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Dopples SimRender: %8.3f  Avg: %8.3f  Max: %8.3f", stats.mSimRenderDopplesTime * 1000.0f, stats.mAvgSimRenderDopplesTime.getAverage() * 1000.0f, stats.mMaxSimRenderDopplesTime * 1000.0f);
   textDisplay.printf(cColorWhite, "Players SimRender: %8.3f   Avg: %8.3f   Max: %8.3f", stats.mSimRenderPlayersTime * 1000.0f, stats.mAvgSimRenderPlayersTime.getAverage() * 1000.0f, stats.mMaxSimRenderPlayersTime * 1000.0f);
#endif
}


//==============================================================================
// BDisplayStats::showKBStats
//==============================================================================
void BDisplayStats::showKBStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (!pUser)
      return;
   const BPlayer* pPlayer = pUser->getPlayer();
   if (!pPlayer)
      return;
   const BTeam* pTeam = pPlayer->getTeam();
   if (!pTeam)
      return;

   textDisplay.printf("KB Stats Summary (Team %i KB)", pTeam->getID());

   // Dumber
   const BKB* pKB = gWorld->getKB(pTeam->getID());
   if (!pKB)
   {
      textDisplay.skipLine();
      textDisplay.printf("No KB detected.");
      return;
   }

   // Got a KB.
   textDisplay.skipLine();
   textDisplay.printf("Total KBSquads:         %u", pKB->getNumberKBSquads());
   textDisplay.printf("Total KBBases: %u",          pKB->getNumberBases());

   long numPlayers = gWorld->getNumberPlayers();
   for (long p=0; p<numPlayers; p++)
   {
      textDisplay.skipLine();
      const BKBPlayer* pKBPlayer = pKB->getKBPlayer(p);
      if (!pKBPlayer)
      {
         textDisplay.printf("Team %d KB has no data about player %d", pTeam->getID(), p);
         continue;
      }
      
      textDisplay.printf("Team %d KB data about player %d", pTeam->getID(), p);
      uint numKBSquads = pKBPlayer->getNumberKBSquads();
      uint validKBSquads = pKBPlayer->getNumberValidKBSquads();
      uint numKBBases = pKBPlayer->getKBBaseIDs().getSize();
      uint numEmptyKBBases = pKBPlayer->getNumberEmptyKBBases();
      BASSERT(numKBSquads >= validKBSquads);
      BASSERT(numKBBases >= numEmptyKBBases);
      uint invalidKBSquads = numKBSquads - validKBSquads;
      textDisplay.printf("   KBSquads Total:    %u", numKBSquads);
      textDisplay.printf("   KBSquads Valid:    %u", validKBSquads);
      textDisplay.printf("   KBSquads Invalid:  %u", invalidKBSquads);
      textDisplay.printf("   KBBases:           %u", numKBBases);
      textDisplay.printf("   KBBases Empty:     %u", numEmptyKBBases);
   }
#endif
}


//==============================================================================
//==============================================================================
void BDisplayStats::showBidManagerStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   const BPlayer* pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   if (!pPlayer)
      return;
   const BAI* pAI = gWorld->getAI(pPlayer->getID());
   if (!pAI)
      return;
   const BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return;

   textDisplay.setColor(BColor(gWorld->getPlayerColor(pPlayer->getID(), BWorld::cPlayerColorContextUI)));
   textDisplay.printf("================= Bid Manager Summary (Player %d) =======================", pPlayer->getID());
   textDisplay.setColor(cColorWhite);
   textDisplay.printf("%12s %5s %9s %5s %5s %12s %s", "BidState", "DBGID", "Score", "EPri", "TCost", "TimeWaiting", "BidTypeAndData"); 

   const BBidIDArray& bidIDs = pBidManager->getBidIDs();
   uint numBidIDs = bidIDs.getSize();
   for (uint i=0; i<numBidIDs; i++)
   {

      const BBid* pBid = gWorld->getBid(bidIDs[i]);
      if (!pBid)
         continue;

      // Get our bid state and color string.
      BFixedString32 bidStateString;
      if (pBid->getState() == BidState::cInactive)
      {
         bidStateString = "(Inactive)";
         textDisplay.setColor(cColorGrey);
      }
      else if (pBid->getState() == BidState::cWaiting)
      {
         bidStateString = "(Waiting)";
         textDisplay.setColor(cColorYellow);
      }
      else if (pBid->getState() == BidState::cApproved)
      {
         bidStateString = "(Approved)";
         textDisplay.setColor(cColorGreen);
      }
      else
      {
         bidStateString = "ERROR";
         textDisplay.setColor(cColorRed);
      }

      // Get our WHAT ARE WE BUYING string
      BSimString bidTypeString = "INVALID";
      if (pBid->isType(BidType::cSquad))
      {
         const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pBid->getSquadToBuy());
         if (pProtoSquad)
            bidTypeString.format("SQUAD(%s)", pProtoSquad->getName().getPtr());
      }
      else if (pBid->isType(BidType::cTech))
      {
         const BProtoTech* pProtoTech = pPlayer->getProtoTech(pBid->getTechToBuy());
         if (pProtoTech)
            bidTypeString.format("TECH(%s)", pProtoTech->getName().getPtr());
      }
      else if (pBid->isType(BidType::cBuilding))
      {
         const BProtoObject* pProtoObject = pPlayer->getProtoObject(pBid->getBuildingToBuy());
         if (pProtoObject)
            bidTypeString.format("BUILDING(%s)", pProtoObject->getName().getPtr());
      }
      else if (pBid->isType(BidType::cPower))
      {
         const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(pBid->getPowerToCast());
         if (pProtoPower)
            bidTypeString.format("POWER(%s)", pProtoPower->getName().getPtr());
      }

      // Get our time waiting string.
      BSimString timeWaitingString;
      float elapsedTimeSeconds = ((pBid->getElapsedTime() % 60000) / 1000.0f);
      DWORD elapsedTimeMinutes = (pBid->getElapsedTime() / 60000);
      timeWaitingString.format("%.2d:%.1f", elapsedTimeMinutes, elapsedTimeSeconds);

      // Paste all the crap together messily...
      textDisplay.printf("%12s %5d %9d %5.0f %5.0f %12s %s", bidStateString.getPtr(), pBid->mDebugID, static_cast<int>(pBid->getScore()), pBid->getEffectivePriority(), pBid->getTotalCost(), timeWaitingString.getPtr(), bidTypeString.getPtr());
   }
#endif
}

//==============================================================================
//==============================================================================
void BDisplayStats::showAISummaryStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   showAITopicStats(textDisplay);
   textDisplay.skipLine();
   showAIMissionStats(textDisplay);
   textDisplay.skipLine();
   showBidManagerStats(textDisplay);
   textDisplay.skipLine();
   showAIMissionTargetStats(textDisplay);
#endif
}


//==============================================================================
//==============================================================================
void BDisplayStats::showAIMissionTargetStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   const BPlayer* pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   if (!pPlayer)
      return;
   const BTeam* pTeam = pPlayer->getTeam();
   if (!pTeam)
      return;
   const BAI* pAI = gWorld->getAI(pPlayer->getID());
   if (!pAI)
      return;
   textDisplay.setColor(BColor(gWorld->getPlayerColor(pPlayer->getID(), BWorld::cPlayerColorContextUI)));
   textDisplay.printf("================= Mission Target Summary (Player %d) =======================", pPlayer->getID());
   textDisplay.setColor(cColorWhite);
   textDisplay.printf("%5s %5s %5s %5s %5s %5s %5s %5s, %34s", "DBGID", "Wraps", "Total", "Inst", "Class", "Affd", "Perm", "Base", "Name");   

   BAIMissionTargetIDArray targetIDs = pAI->getMissionTargetIDs();
   std::sort(targetIDs.begin(), targetIDs.end(), BMissionTargetScoreSortFunctor());
   uint numMissionTargets = targetIDs.getSize();
   for (uint i=0; i<numMissionTargets; i++)
   {
      const BAIMissionTarget* pMT = gWorld->getAIMissionTarget(targetIDs[i]);
      if (!pMT || !pMT->getFlagAllowScoring())
         continue;

      float sTotal = pMT->getScore().getTotal();
      float sInst = pMT->getScore().getInstance();
      float sClass = pMT->getScore().getClass();
      float sAfford = pMT->getScore().getAfford();
      float sPermis = pMT->getScore().getPermission();
      uint numWrappers = pMT->getNumWrapperRefs();

      BSimString positionString;
      positionString.format("(%4.0f,%4.0f)", pMT->getPosition().x, pMT->getPosition().z);

      int baseDebugID = -1;
      if (pMT->isTargetType(MissionTargetType::cKBBase))
      {
         const BKBBase* pKBBase = gWorld->getKBBase(pMT->getKBBaseID());
         if (pKBBase)
            baseDebugID = pKBBase->mDebugID;
      }

      textDisplay.printf("%5u %5u %1.3f %1.3f %1.3f %1.3f %1.3f %5d, %34s", pMT->mDebugID, numWrappers, sTotal, sInst, sClass, sAfford, sPermis, baseDebugID, pMT->mName.getPtr());
   }
#endif
}


//==============================================================================
//==============================================================================
void BDisplayStats::showAIMissionStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   const BPlayer* pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   if (!pPlayer)
      return;
   const BTeam* pTeam = pPlayer->getTeam();
   if (!pTeam)
      return;
   const BAI* pAI = gWorld->getAI(pPlayer->getID());
   if (!pAI)
      return;
   textDisplay.setColor(BColor(gWorld->getPlayerColor(pPlayer->getID(), BWorld::cPlayerColorContextUI)));
   textDisplay.printf("================= Mission Summary (Player %d) =======================", pPlayer->getID());
   textDisplay.setColor(cColorWhite);
   textDisplay.printf("%5s %34s %7s %3s %5s %5s %5s %5s %5s", "DBGID", "Name", "State", "Sq", "Total", "Inst", "Class", "Affd", "Perm");

   const BAIMissionIDArray& missionIDs = pAI->getMissionIDs();
   uint numMissions = missionIDs.getSize();
   for (uint i=0; i<numMissions; i++)
   {
      const BAIMission* pMission = gWorld->getAIMission(missionIDs[i]);
      if (!pMission)
         continue;

      // Active missions = green, inactive = red.
      if (pMission->getFlagActive())
         textDisplay.setColor(cColorGreen);
      else
         textDisplay.setColor(cColorWhite);

      const char* pMissionStateName = gDatabase.getMissionStateName(pMission->getState());
      const char* pMissionTargetName = "NONE";
      const BAIMissionTargetWrapper* pCurrentTargetWrapper = pMission->getCurrentTargetWrapper();
      if (pCurrentTargetWrapper)
      {
         const BAIMissionTarget* pMT = pCurrentTargetWrapper ? gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID()) : NULL;
         if (pMT)
            pMissionTargetName = pMT->mName.getPtr();
      }

      uint numSquads = pMission->getNumSquads();

      const BAIMissionScore& launchScore = pMission->getLaunchScores();
      float tot = launchScore.getTotal();
      float inst = launchScore.getInstance();
      float cls = launchScore.getClass();
      float affd = launchScore.getAfford();
      float perm = launchScore.getPermission();

      textDisplay.printf("%5u %34s %7s %3u %1.3f %1.3f %1.3f %1.3f %1.3f", pMission->mDebugID, pMissionTargetName, pMissionStateName, numSquads, tot, inst, cls, affd, perm);
   }
#endif
}


//==============================================================================
//==============================================================================
void BDisplayStats::showAITopicStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   const BPlayer* pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   if (!pPlayer)
      return;
   const BTeam* pTeam = pPlayer->getTeam();
   if (!pTeam)
      return;
   const BAI* pAI = gWorld->getAI(pPlayer->getID());
   if (!pAI)
      return;

   textDisplay.setColor(BColor(gWorld->getPlayerColor(pPlayer->getID(), BWorld::cPlayerColorContextUI)));
   textDisplay.printf("================= Topic Summary (Player %d - Difficulty %1.2f) =======================", pPlayer->getID(), pPlayer->getDifficulty());
   textDisplay.setColor(cColorWhite);
   textDisplay.printf("%5s %16s %12s %12s %12s", "DBGID", "TopicName", "Tickets(Cur)", "Tickets(Min)", "Tickets(Max)");

   const BAITopicIDArray& topicIDs = pAI->getTopicIDs();
   uint numTopicIDs = topicIDs.getSize();
   for (uint i=0; i<numTopicIDs; i++)
   {
      const BAITopic* pTopic = gWorld->getAITopic(topicIDs[i]);
      if (!pTopic)
         continue;

      // Active topic = green, inactive = red.
      if (pTopic->getFlagActive())
         textDisplay.setColor(cColorGreen);
      else
         textDisplay.setColor(cColorWhite);

      const BSimUString& topicName = pTopic->getName();
      uint currentTickets = pTopic->getCurrentTickets();
      uint minTickets = pTopic->getMinTickets();
      uint maxTickets = pTopic->getMaxTickets();
      
      textDisplay.printf("%5u %16S %12u %12u %12u", pTopic->mDebugID, topicName.getPtr(), currentTickets, minTickets, maxTickets);
   }
#endif
}


//==============================================================================
//==============================================================================
void BDisplayStats::showFactoidStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   BPlayer* pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   if (!pPlayer)
      return;

   textDisplay.setColor(BColor(gWorld->getPlayerColor(pPlayer->getID(), BWorld::cPlayerColorContextUI)));
   textDisplay.printf("================= Topic Summary (Player %d) =======================", pPlayer->getID());

   BAIFactoidManager* pFM = pPlayer->getFactoidManager();
   if (!pFM)
   {
      textDisplay.printf("NO FACTOID MANAGER PRESENT\n");
      return;
   }
   textDisplay.setColor(cColorWhite);

   // Show the last time we played anything.
   DWORD lastFactoidPlayTime = pFM->getLastPlayTime();
   DWORD currentGameTime = gWorld->getGametime();
   DWORD timeSinceLastPlayed = currentGameTime - lastFactoidPlayTime;
   DWORD minutes = timeSinceLastPlayed / 60000;
   float seconds = static_cast<float>(timeSinceLastPlayed % 60000) / 1000.0f;
   textDisplay.printf("\nTime Since Factoid Played: %d:%04.1f\n", minutes, seconds);

   // Show the min priority to play.
   int lowestPermissiblePriority = pFM->getMinPriority();
   textDisplay.printf("\nLowest priority permissible: %d\n", lowestPermissiblePriority);

   // Display the table of priority requests
   textDisplay.printf("\nCurrent Factoid Queue:\n");
   //textDisplay.printf("\n%10s %-30s\n", "Priority", "Event");
   for (uint pri=5; pri>0; pri--)
   {
      const BAIFactoidEvent* pFactoidEvent = pFM->getFactoidEvent(pri);
      if (!pFactoidEvent)
         textDisplay.printf("\n%10d %-30s\n", pri, "NO EVENT");
      else
         if (pFactoidEvent->mName)
            textDisplay.printf("\n%10d %-30s\n", pri, pFactoidEvent->mName.getPtr());
   }
   if (pFM->mLastLine)
      textDisplay.printf("\n%s\n",pFM->mLastLine.getPtr());
   if (pFM->mLineBeforeLast)
      textDisplay.printf("\n%s\n",pFM->mLineBeforeLast.getPtr());
#endif
}



//==============================================================================
// BDisplayStats::showTriggerScriptStats
//==============================================================================
void BDisplayStats::showTriggerScriptStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   textDisplay.printf("Trigger Script Stats Summary");

   uint totalNumTriggerScripts = gTriggerManager.getNumberTriggerScripts();
   uint inactiveNumTriggerScripts = 0;
   uint activeNumTriggerScripts = 0;
   for (uint i=0; i<totalNumTriggerScripts; i++)
   {
//-- FIXING PREFIX BUG ID 3035
      const BTriggerScript* pTriggerScript = gTriggerManager.getTriggerScriptByIndex(i);
//--
      if (!pTriggerScript)
         continue;

      if (pTriggerScript->isActive())
         activeNumTriggerScripts++;
      else
         inactiveNumTriggerScripts++;
   }

   textDisplay.printf("   Total number trigger scripts: %u", totalNumTriggerScripts);
   textDisplay.printf("      Inactive: %u", inactiveNumTriggerScripts);
   textDisplay.printf("      Active: %u", activeNumTriggerScripts);
   textDisplay.skipLine();

   for (uint i=0; i<totalNumTriggerScripts; i++)
   {
      BTriggerScript* pTriggerScript = gTriggerManager.getTriggerScriptByIndex(i);
      if (!pTriggerScript)
         continue;

      if (pTriggerScript->isActive())
      {
         BSimString triggerScriptInfo;
         uint activeTriggersCount = pTriggerScript->getActiveTriggersCount();
         triggerScriptInfo.format("ID=%d, Name=%s, TriggersActive=%u (", pTriggerScript->getID(), pTriggerScript->getName().getPtr(), activeTriggersCount);
         for (uint i=0; i<activeTriggersCount; i++)
         {
            BTriggerID activeTriggerID = pTriggerScript->getActiveTriggerIDByIndex(i);
//-- FIXING PREFIX BUG ID 3036
            const BTrigger* pTrigger = pTriggerScript->getTrigger(activeTriggerID);
//--
            if (pTrigger)
            {
               BSimString appendString;
               appendString.format("%d", pTrigger->getID());
               triggerScriptInfo.append(appendString);
               if (i < (activeTriggersCount-1))
                  triggerScriptInfo.append(", ");
            }
         }

         triggerScriptInfo.append(")");
         textDisplay.printf("%s", triggerScriptInfo.getPtr());
      }
   }

#endif
}

//==============================================================================
//==============================================================================
void BDisplayStats::showSquadStats(BDebugTextDisplay& textDisplay, BWorld* pWorld,
   BUserManager* pUserManager, BTimingTracker* pTimingTracker)
{
   #ifndef BUILD_FINAL
   if ((!pWorld) || (!pUserManager))
      return;

   long simDebugValue=0;
   gConfig.get(cConfigRenderSimDebug, &simDebugValue);
      
   float sx=gUI.mfSafeX1;
   float sy=Math::Max(gUI.mfSafeY1, textDisplay.getPosY());
   float yh=gFontManager.getLineHeight();
      
   BHandle fontHandle = textDisplay.getFontHandle();
   BFixedString<256> t;
   BFixedString<256> t2;

   // FPS
   double gameTime=pWorld->getGametimeFloat();
   DWORD frameLen=pTimingTracker->getLastFrameLength();
   DWORD frameAvg=pTimingTracker->getFrameAverage();
   DWORD updateLen=pTimingTracker->getLastUpdateLength();
   DWORD updateAvg=pTimingTracker->getUpdateAverage();
   DWORD totalLen=frameLen+updateLen;
   DWORD totalAvg=frameAvg+updateAvg;
   float fps=(totalLen>0 ? 1000.0f/(float)totalLen : 1000.0f);
   float fpsAvg=(totalAvg>0 ? 1000.0f/(float)totalAvg : 1000.0f);
   float gameSpeed=1.0f;
   gConfig.get(cConfigGameSpeed, &gameSpeed);
   t.format("Actual FPS Ave=%.2f, Time/Frame Ave=%.2fms,   Timing Tracker: FPS Ave=%.1f, Cur=%.1f", gRender.getAverageFPS(),
      gRender.getAverageFrameTime() * 1000.0f, fpsAvg, fps);
   gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
   sy+=yh;
   t.format("  UpdateNumber=%04u,   UpdateLength: Ave=%04u, Cur=%04u, GameTime=%.1f, GameSpeed=%.1f",
      gWorld->getUpdateNumber(), updateAvg, updateLen, gameTime, gameSpeed);
   gFontManager.drawText(fontHandle, sx, sy, t, cDWORDGreen);
   sy+=yh;

   //-- Number of units selected
   BSelectionManager* pSelectionMgr=pUserManager->getUser(BUserManager::cPrimaryUser)->getSelectionManager();
   long selectedCount=pSelectionMgr->getNumberSelectedUnits();
   BEntityID selectedID;
   if (selectedCount <= 0)
   {
      selectedID=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverObject();
      if (!selectedID.isValid())
      {
         t.format("Nothing selected or hovered over.");
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDRed);
         sy+=yh;

         // EVEN if there is not a selected unit, FTLOG draw the hover point too.
         BVector hoverPoint=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverPoint();
         t.format("Hover Point: %.2f, %.2f, %.2f.", hoverPoint.x, hoverPoint.y, hoverPoint.z);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
         return;
      }
   }
   else
      selectedID=pSelectionMgr->getSelected(0);

   //-- Current object/unit/squad stats
   BUnit* pUnit=pWorld->getUnit(selectedID);
   if (!pUnit)
   {
      t.format("No unit selected.");
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDRed);
      sy+=yh;

      // EVEN if there is not a selected unit, FTLOG draw the hover point too.
      BVector hoverPoint=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverPoint();
      t.format("Hover Point: %.2f, %.2f, %.2f.", hoverPoint.x, hoverPoint.y, hoverPoint.z);
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
      return;
   }
   //Get the squad and platoon from the unit.
   BSquad *pSquad=pUnit->getParentSquad();
   const BProtoSquad* pProtoSquad=NULL;
   const BProtoObject* pProtoObject=NULL;
   BPlatoon* pPlatoon=NULL;
   if (pSquad)
   {
      pProtoSquad=pSquad->getProtoSquad();
      pProtoObject=pSquad->getProtoObject();
      pPlatoon=pSquad->getParentPlatoon();
   }

   //Unit data first.
   if ((simDebugValue == 1) || (simDebugValue == 4))
   {
      t.format("Unit: %s, (1 of %d), PlayerID=%d, ID=%s, Pos=(%.2f, %.2f, %.2f), For=(%.2f, %.2f, %.2f), Vel=%f.",
         pUnit->getProtoObject()->getName().getPtr() ? pUnit->getProtoObject()->getName().getPtr() : "NoName",
         pUnit->getPlayer()->getNumUnitsOfType(pUnit->getProtoID()), pUnit->getPlayerID(), pUnit->getID().getDebugString().getPtr(),
         pUnit->getPosition().x, pUnit->getPosition().y, pUnit->getPosition().z, pUnit->getForward().x, pUnit->getForward().y, pUnit->getForward().z,
         pUnit->getVelocity().length());
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
      sy+=yh;
      //Visual.
      BVisual* pVisual=pUnit->getVisual();
      if (pVisual)
      {
//-- FIXING PREFIX BUG ID 3039
         const BProtoVisual* pProtoVisual=pVisual->getProtoVisual();
//--
         if(pProtoVisual)
         {
            // Action track
            long animType=pVisual->getAnimationType(cActionAnimationTrack);
            float animLen=pVisual->getAnimationDuration(cActionAnimationTrack);
            float animPos=pVisual->getAnimationPosition(cActionAnimationTrack);
            float animPct=(animLen==0.0f?0.0f:animPos/animLen);
            float animClock=pVisual->getAnimationClock(cActionAnimationTrack);
            t.format("  Visual: %s, Action Anim %s (len=%.1f pos=%.1f pct=%.2f, clk=%.1f)", 
               pProtoVisual->getName().getPtr(), gVisualManager.getAnimName(animType), animLen, animPos, animPct, animClock);
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
            sy += yh;

            // Movement track
            animType=pVisual->getAnimationType(cMovementAnimationTrack);
            animLen=pVisual->getAnimationDuration(cMovementAnimationTrack);
            animPos=pVisual->getAnimationPosition(cMovementAnimationTrack);
            animPct=(animLen==0.0f?0.0f:animPos/animLen);
            animClock=pVisual->getAnimationClock(cMovementAnimationTrack);
            t.format("  Move Anim %s (len=%.1f pos=%.1f pct=%.2f, clk=%.1f)",
               gVisualManager.getAnimName(animType), animLen, animPos, animPct, animClock);
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
            sy += yh;
         }
      }
      //Ammo.
      uint numberAmmos=pUnit->getNumberVisualAmmos();
      t.format("  TacticState=%d, Ammo=%.2f, (Max=%.2f), %d VisualAmmos:", pUnit->getTacticState(),
         pUnit->getAmmunition(), pUnit->getProtoObject()->getMaxAmmo(), numberAmmos);
      for (uint i=0; i < numberAmmos; i++)
      {
         if (i < numberAmmos-1)
            t2.format(" %d,", pUnit->getVisualAmmo(i));
         else
            t2.format(" %d.", pUnit->getVisualAmmo(i));
         t.append(t2);
      }
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
      sy+=yh;
      //Actions.
      long actionCount=pUnit->getNumberActions();
      t.format("  %d Actions:", actionCount);
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
      sy+=yh;
      for (long i=0; i < actionCount; i++)
      {
         const BAction* pAction=pUnit->getActionByIndexConst(i);
         if (!pAction)
            continue;
         
         uint numberDebugLines=pAction->getNumberDebugLines();
         BSimString debugString;
         for (uint j=0; j < numberDebugLines; j++)
         {
            pAction->getDebugLine(j, debugString);
            if (j == 0)
               t.format("    A[%02d]: %s", i, debugString.getPtr());
            else
               t.format("             %s", debugString.getPtr());
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
            sy+=yh;
         }
      }
      //Controllers.
      long controllerCount=pUnit->getNumberControllers();
      t.format("  %d Controllers:", controllerCount);
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
      sy+=yh;
      for (long i=0; i < controllerCount; i++)
      {
         const BActionController* pAC=pUnit->getController(i);
         if (!pAC)
            continue;
         
         t.format("    Controller[%02d]: ActionID=%d, OppID=%d.", i, pAC->getActionID(), pAC->getOppID());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;
      }   
      //Opps.
      long oppCount=pUnit->getNumberOpps();
      t.format("  %d Opps:", oppCount);
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
      sy+=yh;
      for (long i=0; i < oppCount; i++)
      {
         const BUnitOpp* pOpp=pUnit->getOppByIndex(i);
         if (!pOpp)
            continue;
         
         t.format("    Opp[%02d]: ID=%d, Type=%s, Target=%s (%.2f, %2.f, %.2f)%s, Pri=%d, Lsh=%d, Trig=%d.", i, pOpp->getID(),
            pOpp->getTypeName(), pOpp->getTarget().getID().getDebugString().getPtr(),
            pOpp->getTarget().getPosition().x, pOpp->getTarget().getPosition().y, pOpp->getTarget().getPosition().z,
            pOpp->getTarget().isPositionValid() ? "(V)" : "", pOpp->getPriority(), pOpp->getLeash(), pOpp->getTrigger());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;
      }   
      // Attacking units
      BEntityIDArray attackingUnits;
      uint numAttackingUnits = pUnit->getAttackingUnits(attackingUnits);
      if (numAttackingUnits == 0)
      {
         gFontManager.drawText(fontHandle, sx, sy, "  Is NOT currently under attack.", cDWORDWhite);
         sy += yh;
      }
      else
      {
         t.format("  Is under attack by %u units:", numAttackingUnits);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDWhite);
         sy += yh;
         /* DLM 11/13/08 - no one cares what units the unit is under attack by.  And it's scrolling shit off the screen that we do care about.
         for (uint attackingUnitIdx=0; attackingUnitIdx<numAttackingUnits; attackingUnitIdx++)
         {
//-- FIXING PREFIX BUG ID 3040
            const BUnit* pAttackingUnit = gWorld->getUnit(attackingUnits[attackingUnitIdx]);
//--
            if (!pAttackingUnit)
               continue;
            const BProtoObject* pAttackingUnitProtoObject = pAttackingUnit->getProtoObject();
            if (!pAttackingUnitProtoObject)
               continue;

            t.format("    Unit %d %s", pAttackingUnit->getID().getIndex(), pAttackingUnitProtoObject->getName().getPtr());
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
            sy += yh;
         }
         */
      }
   }
   
   //Squad.
   if ((simDebugValue == 1) || (simDebugValue == 3))
   {
      if (pSquad)
      {
         if (pProtoSquad)
         {
            t.format("Squad: %s, (1 of %d) ID=%s, Mode %s, Pos=(%.2f, %.2f, %.2f), For=(%.2f, %.2f, %.2f), Vel=%f.", 
               pProtoSquad->getName().getPtr(), pSquad->getPlayer()->getNumSquadsOfType(pProtoSquad->getID()), pSquad->getID().getDebugString().getPtr(),
               gDatabase.getSquadModeName(pSquad->getSquadMode()),
               pSquad->getPosition().x, pSquad->getPosition().y, pSquad->getPosition().z, pSquad->getForward().x, pSquad->getForward().y, pSquad->getForward().z,
               pSquad->getVelocity().length());

            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
            sy+=yh;

            t.format("   Level=%i, XP=%f. Value=%f.", pSquad->getLevel(), pSquad->getXP(), 
               pProtoSquad->getCombatValue());
         }
         else
            t.format("Squad: %s, ID=%s, Mode %s, Pos=(%.2f, %.2f, %.2f), For=(%.2f, %.2f, %.2f), Vel=%f., Level=%i, XP=%f.",
               pProtoObject ? pProtoObject->getName().getPtr() : "NoName", pSquad->getID().getDebugString().getPtr(),
               gDatabase.getSquadModeName(pSquad->getSquadMode()),
               pSquad->getPosition().x, pSquad->getPosition().y, pSquad->getPosition().z, pSquad->getForward().x, pSquad->getForward().y, pSquad->getForward().z,
               pSquad->getVelocity().length(), pSquad->getLevel(), pSquad->getXP());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
         sy+=yh;
         //Distances.
         // mrh 3/13/08 - Leash distance no longer vary with weapons.
         float attackRange=pSquad->getAttackRange();
         float leashDistance=pSquad->getLeashDistance();
         //pSquad->getCombatDistances(attackRange, leashDistance);
         t.format("  AttackRange=%8.3f, LeashDistance=%8.3f.", attackRange, leashDistance);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;

         //Actions.
         int actionCount=pSquad->getNumberActions();
         t.format("  %d Actions:", actionCount);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;
         for (long i=0; i < actionCount; i++)
         {
            const BAction* pAction=pSquad->getActionByIndexConst(i);
            if (!pAction)
               continue;
            
            uint numberDebugLines=pAction->getNumberDebugLines();
            BSimString debugString;
            for (uint j=0; j < numberDebugLines; j++)
            {
               pAction->getDebugLine(j, debugString);
               if (j == 0)
                  t.format("    A[%02d]: %s", i, debugString.getPtr());
               else
                  t.format("             %s", debugString.getPtr());
               gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
               sy+=yh;
            }
         }
         //Orders.
         t.format("  %d Orders.", pSquad->getNumberOrders());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;
         for (uint i=0; i < pSquad->getNumberOrders(); i++)
         {
            const BSimOrderEntry* pOrderEntry=pSquad->getOrderEntry(i);
            if (!pOrderEntry)
               continue;
            BSimOrder* pOrder=pOrderEntry->getOrder();
            if (!pOrder)
               continue;
            t.format("    O[%02d]: (0x%p) ID=%d, Owner=%s, Type=%s%s, Target=%s (%.2f, %.2f, %.2f), Pri=%d, %s.", i, pOrder, pOrder->getID(), pOrder->getOwnerID().getDebugString().getPtr(),
               pOrder->getTypeName(pOrderEntry->getType()), pOrder->getAttackMove() ? "(AttackMove)" : "", 
               pOrder->getTarget().getID().getDebugString().getPtr(), pOrder->getTarget().getPosition().x, pOrder->getTarget().getPosition().y,
               pOrder->getTarget().getPosition().z, pOrder->getPriority(), pOrderEntry->getStateName());
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
            sy+=yh;
         }
         //Roles.
         /*t.format("  %d Roles.", pSquad->getNumberRoles());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;
         for (uint i=0; i < pSquad->getNumberRoles(); i++)
         {
            BEntityRole* pRole=pSquad->getRole(i);
            if (!pRole)
               continue;
            if (pRole->getTimed())
            {
               if (pRole->getTimeout() > gWorld->getGametime())
                  t.format("    R[%02d]: EID=%d, Type=%s, Timeout=%d (%d from now).", i,
                     pRole->getEntityID().getDebugString().getPtr(), pRole->getTypeName(), pRole->getTimeout(),
                     pRole->getTimeout()-gWorld->getGametime());
               else
                  t.format("    R[%02d]: EID=%d, Type=%s, Timeout=%d (should be now).", i,
                     pRole->getEntityID().getDebugString().getPtr(), pRole->getTypeName(), pRole->getTimeout());
            }
            else
               t.format("    R[%02d]: EID=%d, Type=%s, No Timeout.", i,
                  pRole->getEntityID().getDebugString().getPtr(), pRole->getTypeName() );
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
            sy+=yh;
         }*/
      }
      else
      {
         t.format("No squad for this unit.");
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
         sy+=yh;
      }
   }

   //Platoon.
   if ((simDebugValue == 1) || (simDebugValue == 2))
   {
      if (pPlatoon)
      {
         t.format("Platoon: ID=%s, Pos=(%.2f, %.2f, %.2f), For=(%.2f, %.2f, %.2f).",
            pPlatoon->getID().getDebugString().getPtr(),
            pPlatoon->getPosition().x, pPlatoon->getPosition().y, pPlatoon->getPosition().z,
            pPlatoon->getForward().x, pPlatoon->getForward().y, pPlatoon->getForward().z);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
         sy+=yh;
         //Actions.
         int actionCount=pPlatoon->getNumberActions();
         t.format("  %d Actions:", actionCount);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;
         for (long i=0; i < actionCount; i++)
         {
            const BAction* pAction=pPlatoon->getActionByIndexConst(i);
            if (!pAction)
               continue;
            
            uint numberDebugLines=pAction->getNumberDebugLines();
            BSimString debugString;
            for (uint j=0; j < numberDebugLines; j++)
            {
               pAction->getDebugLine(j, debugString);
               if (j == 0)
                  t.format("    A[%02d]: %s", i, debugString.getPtr());
               else
                  t.format("             %s", debugString.getPtr());
               gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
               sy+=yh;
            }
         }
         //Orders.
         t.format("  %d Orders:", pPlatoon->getNumberOrders());
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
         sy+=yh;
         for (uint i=0; i < pPlatoon->getNumberOrders(); i++)
         {
            const BSimOrderEntry* pOrderEntry=pPlatoon->getOrderEntry(i);
            if (!pOrderEntry)
               continue;
            BSimOrder* pOrder=pOrderEntry->getOrder();
            t.format("    O[%02d]: (0x%p) ID=%d, Owner=%s, Type=%s%s, Target=%s (%.2f, %.2f, %.2f), Pri=%d, %s.", i, pOrder, pOrder->getID(), pOrder->getOwnerID().getDebugString().getPtr(),
               pOrder->getTypeName(pOrderEntry->getType()), pOrder->getAttackMove() ? "(AttackMove)" : "", 
               pOrder->getTarget().getID().getDebugString().getPtr(), pOrder->getTarget().getPosition().x, pOrder->getTarget().getPosition().y,
               pOrder->getTarget().getPosition().z, pOrder->getPriority(), pOrderEntry->getStateName());
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
            sy+=yh;
         }
      }
      else
      {
         t.format("No platoon for this unit.");
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDCyan);
         sy+=yh;
      }
   }
   
   //Get the hover object and put up a smidge of data about it if it's not the selected object.
   BEntityID hoverID=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverObject();
   if (hoverID.isValid() && (hoverID != selectedID))
   {
      BUnit* hoverUnit=pWorld->getUnit(hoverID);
      if (hoverUnit)
      {
         t.format("Hover Unit: %s, ID=%s, Pos=(%.2f, %.2f, %.2f), For=(%.2f, %.2f, %.2f).",
            hoverUnit->getProtoObject()->getName().getPtr() ? hoverUnit->getProtoObject()->getName().getPtr() : "NoName", hoverUnit->getID().getDebugString().getPtr(),
            hoverUnit->getPosition().x, pUnit->getPosition().y, hoverUnit->getPosition().z, hoverUnit->getForward().x, hoverUnit->getForward().y, hoverUnit->getForward().z);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
         sy+=yh;
         // EVEN if there is a hover unit, FTLOG draw the hover point too.
         BVector hoverPoint=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverPoint();
         t.format("Hover Point: %.2f, %.2f, %.2f.", hoverPoint.x, hoverPoint.y, hoverPoint.z);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
         sy+=yh;


         float utuHD=-1.0f;
         float stuHD=-1.0f;
         float stsHD=-1.0f;
         if (pUnit)
            utuHD=pUnit->calculateXZDistance(hoverUnit);
         if (pSquad)
            stuHD=pSquad->calculateXZDistance(hoverUnit);
         if (pSquad && hoverUnit->getParentSquad())
            stsHD=pSquad->calculateXZDistance(hoverUnit->getParentSquad());
         t.format("  unitToUnitD=%.2f, squadToUnitD=%.2f, squadToSquadD=%.2f.", utuHD, stuHD, stsHD);
         gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
         sy+=yh;

         // Attacking units
         BEntityIDArray hoverAttackingUnits;
         uint numHoverAttackingUnits = hoverUnit->getAttackingUnits(hoverAttackingUnits);
         if (numHoverAttackingUnits == 0)
         {
            gFontManager.drawText(fontHandle, sx, sy, "  Is NOT currently under attack.", cDWORDWhite);
            sy += yh;
         }
         else
         {
            t.format("  Is under attack by %u units:", numHoverAttackingUnits);
            gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
            sy += yh;
            /* DLM 11/13/08 - No one cares what specific units are attacking this one.  
            for (uint hoverAttackingUnitIdx=0; hoverAttackingUnitIdx<numHoverAttackingUnits; hoverAttackingUnitIdx++)
            {
//-- FIXING PREFIX BUG ID 3041
               const BUnit* pHoverAttackingUnit = gWorld->getUnit(hoverAttackingUnits[hoverAttackingUnitIdx]);
//--
               if (!pHoverAttackingUnit)
                  continue;
               const BProtoObject* pHoverAttackingUnitProtoObject = pHoverAttackingUnit->getProtoObject();
               if (!pHoverAttackingUnitProtoObject)
                  continue;

               t.format("    Unit %d %s", pHoverAttackingUnit->getID().getIndex(), pHoverAttackingUnitProtoObject->getName().getPtr());
               gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
               sy += yh;
            }
            */
         }
      }
   }
   else
   {
      BVector hoverPoint=pUserManager->getUser(BUserManager::cPrimaryUser)->getHoverPoint();
      t.format("Hover Point: %.2f, %.2f, %.2f.", hoverPoint.x, hoverPoint.y, hoverPoint.z);
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDOrange);
      sy+=yh;
      float uHD=-1.0f;
      float sHD=-1.0f;
      if (pUnit)
         uHD=pUnit->calculateXZDistance(hoverPoint);
      if (pSquad)
         sHD=pSquad->calculateXZDistance(hoverPoint);
      t.format("  unitD=%.2f, squadD=%.2f.", uHD, sHD);
      gFontManager.drawText(fontHandle, sx, sy, t, cDWORDLightGrey);
      sy+=yh;
   }
     
   #endif
}   

//==============================================================================
// BDisplayStats::showClassSizeStats
//==============================================================================
void BDisplayStats::showClassSizeStats(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL

   textDisplay.printf("Class Size Summary");
   textDisplay.skipLine();
   textDisplay.printf("BEntity: %u", sizeof(BEntity));
   textDisplay.printf("|---BObject: %u", sizeof(BObject));
   textDisplay.printf("|---BArmy: %u", sizeof(BArmy));
   textDisplay.printf("|---BPlatoon: %u", sizeof(BPlatoon));
   textDisplay.printf("|---BSquad: %u", sizeof(BSquad));
   textDisplay.printf("|---BObject: %u", sizeof(BObject));
   textDisplay.printf("    |---BUnit: %u", sizeof(BUnit));
   textDisplay.printf("    |---BProjectile: %u", sizeof(BProjectile));
   textDisplay.printf("    |---BDopple: %u", sizeof(BDopple));
   textDisplay.printf("BActionList: %u", sizeof(BActionList));
   textDisplay.skipLine();
   textDisplay.printf("BProtoObject: %u", sizeof(BProtoObject));
   textDisplay.printf("BProtoObjectStatic: %u", sizeof(BProtoObjectStatic));
   textDisplay.skipLine();
   textDisplay.printf("BProtoSquad: %u", sizeof(BProtoSquad));
   textDisplay.printf("BProtoSquadStatic: %u", sizeof(BProtoSquadStatic));
   textDisplay.skipLine();
   textDisplay.printf("BProtoTech: %u", sizeof(BProtoTech));
   textDisplay.printf("BProtoTechStatic: %u", sizeof(BProtoTechStatic));
   textDisplay.skipLine();
   textDisplay.printf("BPlayer: %u", sizeof(BPlayer));
   textDisplay.printf("BTeam: %u", sizeof(BTeam));
   textDisplay.printf("BUser: %u", sizeof(BUser));

   textDisplay.skipLine();
   textDisplay.printf("Stats Manager Usage: %u", gStatsManager.getMemUsage());

#endif
}

//==============================================================================
// BDisplayStats::showTimingStats
//==============================================================================
void BDisplayStats::showTimingStats(BDebugTextDisplay& textDisplay, const BWorld* const pWorld, const BTimingTracker* const pTimingTracker)
{
#ifndef BUILD_FINAL
   if ((!pWorld) || (!pTimingTracker))
      return; 

   double gameTime=pWorld->getGametimeFloat();
   double realTime=pWorld->getTotalRealtime();
   double diff=gameTime-realTime;
   DWORD frameLen=pTimingTracker->getLastFrameLength();
   DWORD frameAvg=pTimingTracker->getFrameAverage();
   DWORD frameDev=pTimingTracker->getFrameDeviation();
   DWORD updateLen=pTimingTracker->getLastUpdateLength();
   DWORD updateAvg=pTimingTracker->getUpdateAverage();
   DWORD updateDev=pTimingTracker->getUpdateDeviation();
   DWORD totalLen=frameLen+updateLen;
   float fps=(totalLen>0 ? 1000.0f/(float)totalLen : 1000.0f);
   DWORD updateNumber = pWorld->getUpdateNumber();

   const BWorld::BStats &stats = pWorld->getTimeStats();

   textDisplay.printf(cColorWhite, "GameTime:                 %.1f \n", pWorld->getGametimeFloat());
   textDisplay.printf(cColorWhite, "UpdateNumber:             %04u \n", updateNumber);
   textDisplay.printf(cColorWhite, "UpdateTime/Realtime diff: %4.2f \n\n", diff);
   textDisplay.printf(cColorWhite, "Physical FPS Ave:         %4.2f          Time/Frame Ave: %4.2fms \n", gRender.getAverageFPS(), gRender.getAverageFrameTime() * 1000.0f);
   textDisplay.printf(cColorWhite, "Sim Thread FPS:           %4.2f \n", fps);
   textDisplay.printf(cColorWhite, "Sim Frame Ave:            %04u         Curr:           %04u       STDEV: %04u\n", frameAvg, frameLen, frameDev);
   textDisplay.printf(cColorWhite, "Sim Update Ave:           %04u         Curr:           %04u       STDEV: %04u\n", updateAvg, updateLen, updateDev);
   textDisplay.printf(cColorWhite, "Sim PreAsyncUpdate Time:  %6.3f(ms)  Avg:            %06.3f(ms)", stats.mTotalPreAsyncUpdateTime * 1000.0f, stats.mAvgTotalPreAsyncUpdateTime.getAverage() * 1000.0f);
   textDisplay.printf(cColorWhite, "Sim AsyncUpdate Time:     %6.3f(ms)  Avg:            %06.3f(ms)", stats.mTotalAsyncUpdateTime * 1000.0f, stats.mAvgTotalAsyncUpdateTime.getAverage() * 1000.0f);
   textDisplay.printf(cColorWhite, "Sim PostAsyncUpdate Time: %6.3f(ms)  Avg:            %06.3f(ms)", stats.mTotalPostAsyncUpdateTime * 1000.0f, stats.mAvgTotalPostAsyncUpdateTime.getAverage() * 1000.0f);
   textDisplay.printf(cColorWhite, "Sim Update Time:          %6.3f(ms)  Avg:            %06.3f(ms)", stats.mTotalUpdateTime * 1000.0f, stats.mAvgTotalUpdateTime.getAverage() * 1000.0f);

   showNetworkStats(textDisplay);

   //force display of the timing bars
   gRenderControl.getBarChart()->render(BBarChart::eRenderAlways);
   
#endif
}

//==============================================================================
// 
//==============================================================================
void BDisplayStats::showProxyInfo(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   if (!gLiveSystem->isMultiplayerGameActive())
   {
      textDisplay.print("Multiplayer not active");
      return;
   }

   BSession* pPartySession = (gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->getPartySession() ? gLiveSystem->getMPSession()->getPartySession()->getSession() : NULL);
   BSession* pGameSession = (gLiveSystem->getMPSession() ? gLiveSystem->getMPSession()->getSession() : NULL);

   if (pPartySession)
   {
      textDisplay.printf("Proxy - Party Session");
      for (uint i=0; i < XNetwork::cMaxClients; ++i)
      {
         textDisplay.printf("%d %16s (%s)", i, pPartySession->mUserProxyInfo[i].getPtr(), pPartySession->mUserProxyInfo[i+XNetwork::cMaxClients].getPtr());
      }
      textDisplay.skipLine();
   }
   if (pGameSession)
   {
      textDisplay.printf("Proxy - Game Session");
      for (uint i=0; i < XNetwork::cMaxClients; ++i)
      {
         textDisplay.printf("%d %16s (%s)", i, pGameSession->mUserProxyInfo[i].getPtr(), pGameSession->mUserProxyInfo[i+XNetwork::cMaxClients].getPtr());
      }
      textDisplay.skipLine();
   }

   showVoiceInfo(textDisplay, pPartySession, BString(L"Party"), BVoice::cPartySession);
   showVoiceInfo(textDisplay, pGameSession, BString(L"Game"), BVoice::cGameSession);

#endif
}

//==============================================================================
// 
//==============================================================================
void BDisplayStats::showVoiceInfo(BDebugTextDisplay& textDisplay, BSession* pSession, BString& name, uint voiceSession)
{
#ifndef BUILD_FINAL
   if (pSession == NULL)
      return;

   BVoice::Session session = static_cast<BVoice::Session>(voiceSession);

   // piggybacking voice on the proxy page
   BLiveVoice* pVoice = gLiveSystem->getLiveVoice();
   if (!pVoice)
      return;

   uint controllerID = (gUserManager.getPrimaryUser() ? static_cast<uint>(gUserManager.getPrimaryUser()->getPort()) : 0);

   textDisplay.printf("Voice - %s - Local User %s on Controller %d", name.getPtr(), (gUserManager.getPrimaryUser() ? gUserManager.getPrimaryUser()->getName().getPtr() : ""), controllerID);
   textDisplay.printf("#       Name       Headset  Muted (TCR90 / Session)  Talking");
   // 1 WHWHWHWHWHWHWHWH    0       0      0        0         0
   // #       Name       Headset  Muted (TCR90 / Session)  Talking
   // %d %16s    %d       %d      %d        %d         %d
   for (uint i=0; i < XNetwork::cMaxClients; ++i)
   {
      const BMachine* pMachine = pSession->getMachine(i);

      BOOL tcr90Mute = FALSE;
      BOOL sessionMute = FALSE;
      BOOL muted = pVoice->isMuted(session, controllerID, i, (pMachine ? pMachine->mUsers[0].mXuid : INVALID_XUID), tcr90Mute, sessionMute);

      textDisplay.printf("%d %16s    %d       %d      %d        %d         %d",
         i,
         (pMachine ? pMachine->mUsers[0].mGamertag.getPtr() : ""),
         pVoice->isHeadsetPresent(session, i),
         muted, tcr90Mute, sessionMute,
         pVoice->isTalking(session, i)
         );
   }
   textDisplay.skipLine();
#endif
}

//==============================================================================
// BDisplayStats::showClassSizeStats
//==============================================================================
void BDisplayStats::showAchievements(BDebugTextDisplay& textDisplay, BUserManager* pUserManager)
{
#ifndef BUILD_FINAL

   //textDisplay.printf("Achievements for Primary User");
   //textDisplay.skipLine();
   const BUser * const user = pUserManager->getPrimaryUser();
   if(!user)
   {
      textDisplay.printf("User Not Found");
      return;
   }

   const BUserProfile * const profile = user->getProfile();
   if(!profile)
   {
      textDisplay.printf("No profile found");
      return;
   }
   const BUserAchievementList * const list = profile->getAchievementList();
   if(!list)
   {
      textDisplay.printf("No AchievementList found");
      return;
   }

   const BDynamicSimArray<BAchievementProto*> & rules = gAchievementManager.getRules();
   uint count = rules.getNumber();
   for (uint i=0; i<count; i++)
   {
      const BUserAchievement * const pA = list->getAchievement(i);

      BSimString granted;
      granted.set("-------");

      if (pA)
      {
         if( pA->isGranted() )
            granted.set("Granted");
      }

      if (rules[i]->isType(cAT_Accumulator))
      {
         const BProtoAccumulatorAchievement* const pRule = reinterpret_cast<const BProtoAccumulatorAchievement* const>(rules[i]);
         BASSERT(pRule);
         uint current = 0;
         if (pA)
         {
            const BUserAccumulatorAchievement* const pAA = reinterpret_cast<const BUserAccumulatorAchievement* const>(pA);
            current = pAA->getQuantity();
         }
         textDisplay.printf("%s  %s: (Accumulator) current: %u  required: %u", granted.getPtr(), rules[i]->getName().getPtr(), current, pRule->getQuantity() );
      }
      else if (rules[i]->isType(cAT_Campaign))
      {
         const BProtoCampaignAchievement* const pRule = reinterpret_cast<const BProtoCampaignAchievement* const>(rules[i]);
         BASSERT(pRule);
         uint current = 0;
         if (pA)
         {
            const BUserCampaignAchievement* const pAA = reinterpret_cast<const BUserCampaignAchievement* const>(pA);
            current = pAA->getQuantity();
         }
         textDisplay.printf("%s  %s: (Campaign) current: %u  required: %u", granted.getPtr(), rules[i]->getName().getPtr(), current, pRule->getNumScenariosToBeCompleted() );
      }
      else if (rules[i]->isType(cAT_Skirmish))
      {
         const BProtoSkirmishAchievement* const pRule = reinterpret_cast<const BProtoSkirmishAchievement* const>(rules[i]);
         BASSERT(pRule);
         uint numMap = 0, numLeader = 0, numMode = 0;
         uint numMap2 = 0, numLeader2 = 0, numMode2 = 0;
         numLeader = pRule->getNumLeaders();
         numMap = pRule->getNumMaps();
         numMode = pRule->getNumModes();
         int requiredRank = pRule->getRequiredRank();
         if (pA)
         {
            const BUserSkirmishAchievement* const pAA = reinterpret_cast<const BUserSkirmishAchievement* const>(pA);
            for(uint i=0; i<numLeader; i++)
               numLeader2 += (pAA->getLeaderCompleted(i) ? 1 : 0);
            for(uint i=0; i<numMap; i++)
               numMap2 += (pAA->getMapCompleted(i) ? 1 : 0);
            for(uint i=0; i<numMode; i++)
               numMode2 += (pAA->getModeCompleted(i) ? 1 : 0);
         }
         if (requiredRank != -1)
         {
            int currentRank = -1;
            if ((gUserManager.getPrimaryUser() != NULL) && (gUserManager.getPrimaryUser()->getProfile() != NULL))
               currentRank = gUserManager.getPrimaryUser()->getProfile()->getRank().mRank;
            textDisplay.printf("%s  %s: (Skirmish) %u/%u Rank", granted.getPtr(), rules[i]->getName().getPtr(), currentRank, requiredRank );
         }
         else if ( numMap == 0 && numLeader == 0 && numMode == 0 )
            textDisplay.printf("%s  %s: (Skirmish) ", granted.getPtr(), rules[i]->getName().getPtr() );
         else
            textDisplay.printf("%s  %s: (Skirmish) %u/%u Leaders  %u/%u Maps  %u/%u Modes ", granted.getPtr(), rules[i]->getName().getPtr(), numLeader2, numLeader, numMap2, numMap, numMode2, numMode );
      }
      else if (rules[i]->isType(cAT_Trigger))
      {
         textDisplay.printf("%s  %s:  (Trigger)", granted.getPtr(), rules[i]->getName().getPtr() );
      }
      else if (rules[i]->isType(cAT_Map))
      {
         //const BProtoMapAchievement* const pRule = reinterpret_cast<const BProtoMapAchievement* const>(rules[i]);
         //BASSERT(pRule);
         if (pA)
         {
            //BUserMapAchievement* pAA = reinterpret_cast<BUserMapAchievement*>(pA);
            //pAA->mMaps;
         }
         textDisplay.printf("%s  %s:  (Map) ", granted.getPtr(), rules[i]->getName().getPtr() );
      }
      else if (rules[i]->isType(cAT_Scenario))
      {
         //const BProtoScenarioAchievement* const pRule = reinterpret_cast<const BProtoScenarioAchievement* const>(rules[i]);
         //BASSERT(pRule);
         //pRule->mScenarioName;
         //pRule->mDifficulty;
         textDisplay.printf("%s  %s:  (Scenario) ", granted.getPtr(), rules[i]->getName().getPtr() );
      }
      else if (rules[i]->isType(cAT_Leaders))
      {
         textDisplay.printf("%s  %s:  (Leaders) not implemented yet", granted.getPtr(), rules[i]->getName().getPtr() );
      }
      else if (rules[i]->isType(cAT_Abilities))
      {
         textDisplay.printf("%s  %s:  (Abilities) not implemented yet", granted.getPtr(), rules[i]->getName().getPtr() );
      }
      else if (rules[i]->isType(cAT_Meta))
      {
         const BProtoMetaAchievement* const pRule = reinterpret_cast<const BProtoMetaAchievement* const>(rules[i]);
         BString debugString;
         debugString.set("(no data)");
         pRule->getDebugString(gUserManager.getPrimaryUser(), debugString);
         textDisplay.printf("%s  %s:  (Meta) %s", granted.getPtr(), rules[i]->getName().getPtr(), debugString.getPtr() );
      }
      else if (rules[i]->isType(cAT_Misc))
      {
         const BProtoMiscAchievement* const pRule = reinterpret_cast<const BProtoMiscAchievement* const>(rules[i]);
         switch (pRule->mMiscType)
         {
            case cMiscATSkulls:
               textDisplay.printf("%s  %s:  (Misc) Skulls %d / %d", granted.getPtr(), rules[i]->getName().getPtr(), 
                  gCollectiblesManager.getNumSkullsCollected(gUserManager.getPrimaryUser(), false, false), pRule->mQuantity );
               break;
            case cMiscATTimeline:
               textDisplay.printf("%s  %s:  (Misc) Timeline %d / %d", granted.getPtr(), rules[i]->getName().getPtr(), 
                  gCollectiblesManager.getNumTimelineEventsUnlocked(gUserManager.getPrimaryUser()), pRule->mQuantity );
               break;
            case cMiscATPlayedTime:
               textDisplay.printf("%s  %s:  (Misc) PlayedTime %d / %d", granted.getPtr(), rules[i]->getName().getPtr(), 
                  gUserManager.getPrimaryUser()->getProfile()->getTotalGameTime(), pRule->mQuantity );
               break;
            case cMiscATFinalKillByPower:
               textDisplay.printf("%s  %s:  (Misc) Final Kill By Power", granted.getPtr(), rules[i]->getName().getPtr());
               break;
            default:
               textDisplay.printf("%s  %s:  (Misc) not implemented yet", granted.getPtr(), rules[i]->getName().getPtr() );
               break;
         }
      }

   }

#endif
}

//==============================================================================
// BDisplayStats::showCampaignProgress
//==============================================================================
void BDisplayStats::showCampaignProgress(BDebugTextDisplay& textDisplay, BUserManager* pUserManager, int page)
{
#ifndef BUILD_FINAL

   textDisplay.printf("Campaign Progress for Primary User");
   textDisplay.skipLine();
   const BUser * const user = pUserManager->getPrimaryUser();
   if(!user)
   {
      textDisplay.printf("User Not Found");
      return;
   }

   const BUserProfile * const profile = user->getProfile();
   if(!profile)
   {
      textDisplay.printf("No profile found");
      return;
   }
   
   if (user->getPlayer() != NULL)
   {
      long plrID = user->getPlayer()->getID();
      BScorePlayer *pScore = gScoreManager.getPlayerByID(plrID);
      if (pScore)
      {
         textDisplay.printf("Player Score: %d -- kills %.02f (%d) -- lost %.02f (%d)", gScoreManager.getFinalScore(plrID), 
            pScore->mCombatValueKilled, pScore->mSquadsKilled, 
            pScore->mCombatValueLost, pScore->mSquadsLost);
      }
   }

   profile->mCampaignProgress.displayInfoInStats(textDisplay, page);
#endif
}

//==============================================================================
// BDisplayStats::showFontMemory
//==============================================================================
void BDisplayStats::showFontMemory(BDebugTextDisplay& textDisplay)
{
#ifndef BUILD_FINAL
   displayFontMemoryStats(textDisplay);
#endif
}

//==============================================================================
// BDisplayStats::setType
//==============================================================================
void BDisplayStats::setType(int type) 
{ 
   switch (mDisplayStatMode)
   {  
      case cDSMDisabled:
      {
         break;
      }
      case cDSMSim:
      {
         mDisplayStatType[cDSMSim] = Math::Clamp(type, 0, cNumberDisplayStatTypes - 1);
         break;
      }
      case cDSMRender:
      {
         mDisplayStatType[cDSMRender] = Math::Clamp(type, 0, cNumberDisplayStatRenderTypes - 1);
         break;
      }
   }   
}

//==============================================================================
// BDisplayStats::isDisplaying
//==============================================================================
bool BDisplayStats::isDisplaying(void)
{
   if (mDisplayStatMode == BDisplayStats::cDSMDisabled) 
      return false;
     
   if ((mDisplayStatMode == cDSMSim) && (mDisplayStatType[cDSMSim] == BDisplayStats::cDisplayStatFPS)) 
      return false;
   
   return true;
}

//==============================================================================
// BDisplayStats::update
//==============================================================================
bool BDisplayStats::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);
   repeat;
   stop;
   
   if (start)
   {
      eDisplayStatMode mode = cDSMSim;
      int maxTypes = cNumberDisplayStatTypes;

      switch (controlType)
      {
         case cKeyF2:
         {
            mode =  cDSMSim;
            maxTypes = cNumberDisplayStatTypes;

            break;
         }
         case cKeyF4:
         {
            mode =  cDSMRender;
            maxTypes = cNumberDisplayStatRenderTypes;
            break;
         }
         case cKeyF5:
         {
            mode =  cDSMTimings;
            maxTypes = cNumberDisplayStatTimingsTypes;
            break;
         }
         default:
         {
            return false;
         } 
      };



      if (gInputSystem.getKeyboard()->isKeyActive(cKeyAlt))
      {
         if (mDisplayStatMode == mode)
         {
            if(gInputSystem.getKeyboard()->isKeyActive(cKeyShift))
            {
               mDisplayStatType[mode]--;
               if(mDisplayStatType[mode]<0)
                  mDisplayStatType[mode]=maxTypes-1;
            }
            else
            {
               mDisplayStatType[mode]++;
               if(mDisplayStatType[mode]>=maxTypes)
                  mDisplayStatType[mode]=0;
            }
         }
      }
      else
      {
         if (mDisplayStatMode == mode)
            mDisplayStatMode = cDSMDisabled;
         else if (mDisplayStatMode == cDSMDisabled)
            mDisplayStatMode = mode;
         else
            mDisplayStatMode = mode;
      }

      return true;
   }
   
   return false;
}

//==============================================================================
// BDisplayStats::show
//==============================================================================
void BDisplayStats::show(BWorld* pWorld, BTimingTracker* pTimingTracker, BUserManager* pUserManager)
{
#ifndef BUILD_FINAL
   SCOPEDSAMPLE(BModeGameShowStats);

   BHandle fontHandle = gFontManager.getFontCourier10();
   gFontManager.setFont(fontHandle);
   
   // Please try to use the BDebugTextDisplay class for future debug printing.
   BDebugTextDisplay textDisplay(fontHandle);
   
   textDisplay.setColor(cColorGreen);
         
   if (mDisplayStatMode == cDSMRender)
   {
      textDisplay.printf("Render Stats - Page %d", mDisplayStatType[cDSMRender]);

      switch (mDisplayStatType[cDSMRender])
      {
         case cDisplayStatRender1:
         case cDisplayStatRender2:
         case cDisplayStatRender3:
         case cDisplayStatRender4:
         case cDisplayStatRender5:
         case cDisplayStatRender6:
         case cDisplayStatRender7:
         case cDisplayStatRender8:
         {
            showRenderStats(textDisplay, mDisplayStatType[cDSMRender] - cDisplayStatRender1, pUserManager);
            break;
         }
         case cDisplayStatParticles:
         {
            gRenderControl.showParticleStats(textDisplay);
            break;
         }
         case cDisplayStatFlash:
         {
            showFPS(textDisplay, pWorld, pTimingTracker);
            gRenderControl.showFlashStats(textDisplay);
            break;
         }
         case cDisplayStatTerrain:
         {
            gRenderControl.showTerrainStats(textDisplay);
            break;
         }
         case cDisplayD3DTextureManager1:
         case cDisplayD3DTextureManager2:
         case cDisplayD3DTextureManager3:
         case cDisplayD3DTextureManager4:
         case cDisplayD3DTextureManager5:
         case cDisplayD3DTextureManager6:
         case cDisplayD3DTextureManager7:
         case cDisplayD3DTextureManager8:
         {  
            showD3DTextureManagerStats(textDisplay, mDisplayStatType[cDSMRender] - cDisplayD3DTextureManager1);
            break;
         }
#if 0         
         case cDisplayD3DTextureManager1InGameUI:
         case cDisplayD3DTextureManager2InGameUI:
         case cDisplayD3DTextureManager3InGameUI:
         case cDisplayD3DTextureManager4InGameUI:
         case cDisplayD3DTextureManager5InGameUI:
         {  
            showD3DTextureManagerDetailStats(textDisplay, BD3DTextureManager::cScaleformInGame, mDisplayStatType[cDSMRender] - cDisplayD3DTextureManager1InGameUI);
            break;
         }
#endif         
         case cDisplayStatHeapTotals:
         {
            showDetailedHeapStats(textDisplay, true, false);
            break;
         }
         case cDisplayStatHeapDeltas:
         {
            showDetailedHeapStats(textDisplay, false, true);
            break;
         }
         case cDisplayStatTextureHeap:
         {
            printXboxTextureHeapStats(textDisplay.getConsoleOutput());
            break;
         }
         case cDisplayStatRockall:
         {
            showHeapStats(textDisplay);
            break;
         }
         case cDisplayFontMemory:
         {
            showFontMemory(textDisplay);
            break;
         }
      } // switch(displayStatType)
   }
   else if (mDisplayStatMode == cDSMSim)
   {
      // [10/7/2008 xemu] uber-hacktastical thing to make just one more line of room for page 16 (achievements) 
      if (mDisplayStatType[cDSMSim] != cDisplayStatAchievements)
         textDisplay.printf("Sim Stats - Page %d", mDisplayStatType[cDSMSim]);

      switch (mDisplayStatType[cDSMSim])
      {
         case cDisplayStatFPS:
         {
            showFPS(textDisplay, pWorld, pTimingTracker);
            break;
         }
         case cDisplayStatNetwork:
         {
            showNetworkStats(textDisplay);
            break;
         }

         case cDisplayStatWorld:
         {
            showFPS(textDisplay, pWorld, pTimingTracker);
            showWorldStats(textDisplay, pWorld);
            break;
         }
         
         case cDisplayStatSound:
         {
            showSoundStats(textDisplay);
            break;
         }
         case cDisplayStatPathing:
         {
            showPathingStats(textDisplay);
            break;
         }
         case cDisplayStatUpdate:
         {
            showFPS(textDisplay, pWorld, pTimingTracker);
            showUpdateStats(textDisplay, pWorld, pUserManager);
            break;
         }
         case cDisplayStatObject:
         {
            showObjectStats(textDisplay, pWorld, pUserManager);
            break;
         }
         case cDisplayStatKB:
         {
            showKBStats(textDisplay);
            break;
         }
         case cDisplayStatBidManager:
         {
            showBidManagerStats(textDisplay);
            break;
         }
         case cDisplayStatAISummary:
         {
            showAISummaryStats(textDisplay);
            break;
         }
         case cDisplayStatAIMissionTargets:
         {
            showAIMissionTargetStats(textDisplay);
            break;
         }
         case cDisplayStatAIMissions:
         {
            showAIMissionStats(textDisplay);
            break;
         }
         case cDisplayStatAITopics:
         {
            showAITopicStats(textDisplay);
            break;
         }
         case cDisplayStatFactoids:
         {
            showFactoidStats(textDisplay);
            break;
         }
         case cDIsplayStatTriggerScripts:
         {
            showTriggerScriptStats(textDisplay);
            break;
         }
         case cDisplayStatSquad:
         {
            showSquadStats(textDisplay, pWorld, pUserManager, pTimingTracker);
            break;
         }
         case cDisplayStatClassSize:
         {
            showClassSizeStats(textDisplay);
            break;
         }

         case cDisplayStatImpactEffects:
         {
            showFPS(textDisplay, pWorld, pTimingTracker);
            gRenderControl.showImpactEffectStats(textDisplay);
            break;
         }
         case cDisplayStatAchievements:
         {
            showAchievements(textDisplay, pUserManager);
            break;
         }
         case cDisplayStatCampaignProgress1:
         {
            showCampaignProgress(textDisplay, pUserManager, 1);
            break;
         }
         case cDisplayStatCampaignProgress2:
         {
            showCampaignProgress(textDisplay, pUserManager, 2);
            break;
         }
         
         
      } // switch(displayStatType)
   }
   else if (mDisplayStatMode == cDSMTimings)
   {
      textDisplay.printf("Timing Stats - Page %d", mDisplayStatType[cDSMTimings]);

      switch (mDisplayStatType[cDSMTimings])
      {
         case cDisplayStatTimings:
            {
               showTimingStats(textDisplay, pWorld, pTimingTracker);
               break;
            }
         case cDisplayProxyInfo:
            {
               showProxyInfo(textDisplay);
               break;
            }
      };
   }
   
   gRenderControl.showPersistentStats(textDisplay, (mDisplayStatMode == cDSMSim) && (mDisplayStatType[cDSMSim] == cDisplayStatFPS));
#endif
}
