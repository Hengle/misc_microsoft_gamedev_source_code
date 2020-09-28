//==============================================================================
// SyncDefines.h
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================
#pragma once

// To add your own sync tags, read the INSTRUCTIONS below
//==============================================================================
// Includes

// master sync control
#define SYNC

//==============================================================================
enum { cDisabledSyncState = 0, cXORSyncState, cFullSyncState, cFull1v1SyncState };
class BSyncDefine
{
   public:
      long              mTagID;
      char              *mName;         
      long              mState;
};


//==============================================================================
// INSTRUCTIONS
//==============================================================================
// To create your own sync tags, just add your own code to the two sections
// below, then the section in syncmacros.h, the section at the top of syncmanager.cpp, and the sections
// in aconfigenums.h and .cpp
// (6 sections total)
// To simplify things, there are commented out "templates" at the bottom
// of each section. Just copy each of those, and replace the word "ReplaceMe" with the name
// of your system, such as "Unit"



//==============================================================================
// #1 - Control #defines for master-enabling/disabling sync tags
#ifdef SYNC

#ifndef BUILD_FINAL
   #ifndef BUILD_PROFILE
      // Debug/release sync tags
      #define SYNC_Rand
      #define SYNC_Player
      #define SYNC_Team
      #define SYNC_UnitGroup
      #define SYNC_Unit
      #define SYNC_UnitDetail
      #define SYNC_UnitAction
      #define SYNC_Squad
      #define SYNC_Projectile
      #define SYNC_World
      #define SYNC_Tech
      #define SYNC_Command   
      #define SYNC_Pathing
      #define SYNC_Movement
      #define SYNC_Visibility
      #define SYNC_FinalRelease
      #define SYNC_FinalDetail
      #define SYNC_Checksum
      #define SYNC_Trigger
      #define SYNC_TriggerVar
      #define SYNC_Anim
      #define SYNC_Dopple
      #define SYNC_Comm
      #define SYNC_Platoon
   #else
      // Mininmal syncing for profile builds
      #define SYNC_FinalRelease
      #define SYNC_FinalDetail
   #endif
#else
   // Final Release sync tag
   #define SYNC_FinalRelease
   #define SYNC_FinalDetail
#endif

#endif

//==============================================================================
// #2 - sync tag names
enum
{
   cRandSync = 0,
   cPlayerSync,
   cTeamSync,
   cUnitGroupSync,
   cUnitSync,
   cUnitDetailSync,
   cUnitActionSync,
   cSquadSync,
   cProjectileSync,
   cWorldSync,
   cTechSync,
   cCommandSync,
   cPathingSync,
   cMovementSync,
   cVisibilitySync,
   cFinalReleaseSync,
   cFinalDetailSync,
   cChecksumSync,
   cTriggerSync,
   cTriggerVarSync,
   cAnimSync,
   cDoppleSync,
   cCommSync,
   cPlatoonSync,
   cNumberOfSyncDefines // this must be the last entry in this enum
                        // This must also be less than BSyncData::cExtractTagBitMask
};

extern BSyncDefine syncDefines[];