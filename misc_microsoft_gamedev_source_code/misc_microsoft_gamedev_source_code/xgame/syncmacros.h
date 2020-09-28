//==============================================================================
// SyncMacros.h
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================
#pragma once

#include "random.h"
// To add your own sync tags, read the INSTRUCTIONS in syncdefines.h
#include "syncdefines.h"
#include "syncmanager.h"


//Only sync from the sim thread because the sync manager is not thread safe.
#define SYNC_CODE(tag, desc, file, line)                                                 \
{                                                                                        \
   if(gEventDispatcher.getThreadIndex() == cThreadIndexSim)            \
   {                                                                                     \
      static BSyncCallEntry sEntry(desc, file, (WORD)line, tag);   \
      BSyncManager::getInstance()->syncCode(sEntry);                                     \
   }                                                                                     \
}                                                                                        

#define SYNC_DATA(tag, desc, value, file, line)                                               \
{                                                                                             \
    if(gEventDispatcher.getThreadIndex() == cThreadIndexSim)                 \
   {                                                                                          \
      static BSyncCallEntry sEntry(desc, file, (WORD)line, tag, value); \
      BSyncManager::getInstance()->syncData(sEntry, value);                                   \
   }                                                                                          \
}

#define CODE_SYNC(tag, userinfo)          SYNC_CODE(tag, userinfo, __FILE__, __LINE__)
#define DATA_SYNC(tag, userinfo, v, f, l) SYNC_DATA(tag, userinfo, v, f, l)

//==============================================================================
// #3 - sync macros - the ...FLData() methods are for when you want to explicitly specify your
//      own file and line information, rather than having it taken automatically from the
//      code that calls this sync macro
#ifdef SYNC_Rand   
   #define syncRandCode(userinfo)                         CODE_SYNC(cRandSync, userinfo)
   #define syncRandData(userinfo, v)                      DATA_SYNC(cRandSync, userinfo, v, __FILE__, __LINE__)
   #define syncRandFLData(userinfo, v, f, l)              DATA_SYNC(cRandSync, userinfo, v, f, l)
#else 
   #define syncRandCode(userinfo)                         ((void)0)
   #define syncRandData(userinfo, v)                      ((void)0)
   #define syncRandFLData(userinfo, v, f, l)                        ((void)0)      
#endif

#ifdef SYNC_Player   
   #define syncPlayerCode(userinfo)                       CODE_SYNC(cPlayerSync, userinfo)
   #define syncPlayerData(userinfo, v)                    DATA_SYNC(cPlayerSync, userinfo, v, __FILE__, __LINE__) 
   #define syncPlayerFLData(userinfo, v, f, l)            DATA_SYNC(cPlayerSync, userinfo, v, f, l)
#else
   #define syncPlayerCode(userinfo)                       ((void)0)
   #define syncPlayerData(userinfo, v)                    ((void)0)
   #define syncPlayerFLData(v, f, l)                      ((void)0)      
#endif

#ifdef SYNC_Team
   #define syncTeamCode(userinfo)                       CODE_SYNC(cTeamSync, userinfo)
   #define syncTeamData(userinfo, v)                    DATA_SYNC(cTeamSync, userinfo, v, __FILE__, __LINE__)
   #define syncTeamFLData(userinfo, v, f, l)            DATA_SYNC(cTeamSync, userinfo, v, f, l)
#else
   #define syncTeamCode(userinfo)                       ((void)0)
   #define syncTeamData(userinfo, v)                    ((void)0)
   #define syncTeamFLData(v, f, l)                      ((void)0)
#endif

#ifdef SYNC_Squad
   #define syncSquadCode(userinfo)                        CODE_SYNC(cSquadSync, userinfo)
   #define syncSquadData(userinfo, v)                     DATA_SYNC(cSquadSync, userinfo, v, __FILE__, __LINE__)
   #define syncSquadFLData(userinfo, v, f, l)             DATA_SYNC(cSquadSync, userinfo, v, f, l)
#else
   #define syncSquadCode(userinfo)                        ((void)0)
   #define syncSquadData(userinfo, v)                     ((void)0)
   #define syncSquadFLData(v, f, l)                       ((void)0)      
#endif

#ifdef SYNC_Platoon
   #define syncPlatoonCode(userinfo)                        CODE_SYNC(cPlatoonSync, userinfo)
   #define syncPlatoonData(userinfo, v)                     DATA_SYNC(cPlatoonSync, userinfo, v, __FILE__, __LINE__)
   #define syncPlatoonFLData(userinfo, v, f, l)             DATA_SYNC(cPlatoonSync, userinfo, v, f, l)
#else
   #define syncPlatoonCode(userinfo)                        ((void)0)
   #define syncPlatoonData(userinfo, v)                     ((void)0)
   #define syncPlatoonFLData(v, f, l)                       ((void)0)      
#endif

#ifdef SYNC_UnitGroup   
   #define syncUnitGroupCode(userinfo)                    CODE_SYNC(cUnitGroupSync, userinfo)
   #define syncUnitGroupData(userinfo, v)                 DATA_SYNC(cUnitGroupSync, userinfo, v, __FILE__, __LINE__) 
   #define syncUnitGroupFLData(userinfo, v, f, l)         DATA_SYNC(cUnitGroupSync, userinfo, v, f, l)
#else
   #define syncUnitGroupCode(userinfo)                    ((void)0)
   #define syncUnitGroupData(userinfo, v)                 ((void)0)
   #define syncUnitGroupFLData(v, f, l)                   ((void)0)      
#endif

#ifdef SYNC_Unit   
   #define syncUnitCode(userinfo)                         CODE_SYNC(cUnitSync, userinfo) 
   #define syncUnitData(userinfo, v)                      DATA_SYNC(cUnitSync, userinfo, v, __FILE__, __LINE__)  
   #define syncUnitFLData(userinfo, v, f, l)              DATA_SYNC(cUnitSync, userinfo, v, f, l) 
#else
   #define syncUnitCode(userinfo)                         ((void)0)
   #define syncUnitData(userinfo, v)                      ((void)0)
   #define syncUnitFLData(v, f, l)                        ((void)0)      
#endif

#ifdef SYNC_UnitDebug   
   #define syncUnitDebugCode(userinfo)                    CODE_SYNC(cUnitDebugSync, userinfo)
   #define syncUnitDebugData(userinfo, v)                 DATA_SYNC(cUnitDebugSync, userinfo, v, __FILE__, __LINE__) 
   #define syncUnitDebugFLData(userinfo, v, f, l)         DATA_SYNC(cUnitDebugSync, userinfo, v, f, l)
#else
   #define syncUnitDebugCode(userinfo)                    ((void)0)
   #define syncUnitDebugData(userinfo, v)                 ((void)0)
   #define syncUnitDebugFLData(v, f, l)                   ((void)0)      
#endif

#ifdef SYNC_World   
   #define syncWorldCode(userinfo)                         CODE_SYNC(cWorldSync, userinfo)
   #define syncWorldData(userinfo, v)                      DATA_SYNC(cWorldSync, userinfo, v, __FILE__, __LINE__) 
   #define syncWorldFLData(userinfo, v, f, l)              DATA_SYNC(cWorldSync, userinfo, v, f, l)
#else
   #define syncWorldCode(userinfo)                         ((void)0)
   #define syncWorldData(userinfo, v)                      ((void)0)
   #define syncWorldFLData(v, f, l)                        ((void)0)      
#endif

#ifdef SYNC_Tech   
   #define syncTechCode(userinfo)                          CODE_SYNC(cTechSync, userinfo)
   #define syncTechData(userinfo, v)                       DATA_SYNC(cTechSync, userinfo, v, __FILE__, __LINE__)
   #define syncTechFLData(userinfo, v, f, l)               DATA_SYNC(cTechSync, userinfo, v, f, l)
#else
   #define syncTechCode(userinfo)                          ((void)0)
   #define syncTechData(userinfo, v)                       ((void)0)
   #define syncTechFLData(v, f, l)                         ((void)0)      
#endif

#ifdef SYNC_TechDebug   
   #define syncTechDebugCode(userinfo)                     CODE_SYNC(cTechDebugSync, userinfo)
   #define syncTechDebugData(userinfo, v)                  DATA_SYNC(cTechDebugSync, userinfo, v, __FILE__, __LINE__)
   #define syncTechDebugFLData(userinfo, v, f, l)          DATA_SYNC(cTechDebugSync, userinfo, v, f, l)
#else
   #define syncTechDebugCode(userinfo)                     ((void)0)
   #define syncTechDebugData(userinfo, v)                  ((void)0)
   #define syncTechDebugFLData(v, f, l)                    ((void)0)      
#endif

#ifdef SYNC_Command   
   #define syncCommandCode(userinfo)                       CODE_SYNC(cCommandSync, userinfo)
   #define syncCommandData(userinfo, v)                    DATA_SYNC(cCommandSync, userinfo, v, __FILE__, __LINE__)
   #define syncCommandFLData(userinfo, v, f, l)            DATA_SYNC(cCommandSync, userinfo, v, f, l)
#else
   #define syncCommandCode(userinfo)                       ((void)0)
   #define syncCommandData(userinfo, v)                    ((void)0)
   #define syncCommandFLData(v, f, l)                      ((void)0)      
#endif

#ifdef SYNC_Pathing   
   #define syncPathingCode(userinfo)                       CODE_SYNC(cPathingSync, userinfo)
   #define syncPathingData(userinfo, v)                    DATA_SYNC(cPathingSync, userinfo, v, __FILE__, __LINE__)
   #define syncPathingFLData(userinfo, v, f, l)            DATA_SYNC(cPathingSync, userinfo, v, f, l)
#else
   #define syncPathingCode(userinfo)                       ((void)0)
   #define syncPathingData(userinfo, v)                    ((void)0)
   #define syncPathingFLData(v, f, l)                      ((void)0)      
#endif

#ifdef SYNC_EntityMovement   
   #define syncEntityMovementCode(userinfo)                CODE_SYNC(cEntityMovementSync, userinfo)
   #define syncEntityMovementData(userinfo, v)             DATA_SYNC(cEntityMovementSync, userinfo, v, __FILE__, __LINE__)
   #define syncEntityMovementFLData(userinfo, v, f, l)     DATA_SYNC(cEntityMovementSync, userinfo, v, f, l)
#else
   #define syncEntityMovementCode(userinfo)                ((void)0)
   #define syncEntityMovementData(userinfo, v)             ((void)0)
   #define syncEntityMovementFLData(v, f, l)               ((void)0)      
#endif

#ifdef SYNC_TerrainLowLevel   
   #define syncTerrainLowLevelCode(userinfo)               CODE_SYNC(cTerrainLowLevelSync, userinfo)
   #define syncTerrainLowLevelData(userinfo, v)            DATA_SYNC(cTerrainLowLevelSync, userinfo, v, __FILE__, __LINE__)
   #define syncTerrainLowLevelFLData(userinfo, v, f, l)    DATA_SYNC(cTerrainLowLevelSync, userinfo, v, f, l)
#else
   #define syncTerrainLowLevelCode(userinfo)               ((void)0)
   #define syncTerrainLowLevelData(userinfo, v)            ((void)0)
   #define syncTerrainLowLevelFLData(v, f, l)              ((void)0)      
#endif

#ifdef SYNC_Comm   
   #define syncCommCode(userinfo)                          CODE_SYNC(cCommSync, userinfo)
   #define syncCommData(userinfo, v)                       DATA_SYNC(cCommSync, userinfo, v, __FILE__, __LINE__)
   #define syncCommFLData(userinfo, v, f, l)               DATA_SYNC(cCommSync, userinfo, v, f, l)
#else
   #define syncCommCode(userinfo)                          ((void)0)
   #define syncCommData(userinfo, v)                       ((void)0)
   #define syncCommFLData(v, f, l)                         ((void)0)      
#endif

#ifdef SYNC_UnitDetail   
   #define syncUnitDetailCode(userinfo)                    CODE_SYNC(cUnitDetailSync, userinfo)
   #define syncUnitDetailData(userinfo, v)                 DATA_SYNC(cUnitDetailSync, userinfo, v, __FILE__, __LINE__)
   #define syncUnitDetailFLData(userinfo, v, f, l)         DATA_SYNC(cUnitDetailSync, userinfo, v, f, l)
#else
   #define syncUnitDetailCode(userinfo)                    ((void)0)
   #define syncUnitDetailData(userinfo, v)                 ((void)0)
   #define syncUnitDetailFLData(v, f, l)                   ((void)0)      
#endif

#ifdef SYNC_DummyObject   
   #define syncDummyObjectCode(userinfo)                   CODE_SYNC(cDummyObjectSync, userinfo)
   #define syncDummyObjectData(userinfo, v)                DATA_SYNC(cDummyObjectSync, userinfo, v, __FILE__, __LINE__)
   #define syncDummyObjectFLData(userinfo, v, f, l)        DATA_SYNC(cDummyObjectSync, userinfo, v, f, l)
#else
   #define syncDummyObjectCode(userinfo)                   ((void)0)
   #define syncDummyObjectData(userinfo, v)                ((void)0)
   #define syncDummyObjectFLData(v, f, l)                  ((void)0)      
#endif

#ifdef SYNC_ObMgr   
   #define syncObMgrCode(userinfo)                         CODE_SYNC(cObMgrSync, userinfo)
   #define syncObMgrData(userinfo, v)                      DATA_SYNC(cObMgrSync, userinfo, v, __FILE__, __LINE__)
   #define syncObMgrFLData(userinfo, v, f, l)              DATA_SYNC(cObMgrSync, userinfo, v, f, l)
#else
   #define syncObMgrCode(userinfo)                         ((void)0)
   #define syncObMgrData(userinfo, v)                      ((void)0)
   #define syncObMgrFLData(v, f, l)                        ((void)0)      
#endif

#ifdef SYNC_Projectile   
   #define syncProjectileCode(userinfo)                     CODE_SYNC(cProjectileSync, userinfo)
   #define syncProjectileData(userinfo, v)                  DATA_SYNC(cProjectileSync, userinfo, v, __FILE__, __LINE__)
   #define syncProjectileFLData(userinfo, v, f, l)          DATA_SYNC(cProjectileSync, userinfo, v, f, l)
#else
   #define syncProjectileCode(userinfo)                     ((void)0)
   #define syncProjectileData(userinfo, v)                  ((void)0)
   #define syncProjectileFLData(v, f, l)                    ((void)0)      
#endif

#ifdef SYNC_UnitAction   
   #define syncUnitActionCode(userinfo)                    CODE_SYNC(cUnitActionSync, userinfo)
   #define syncUnitActionData(userinfo, v)                 DATA_SYNC(cUnitActionSync, userinfo, v, __FILE__, __LINE__)
   #define syncUnitActionFLData(userinfo, v, f, l)         DATA_SYNC(cUnitActionSync, userinfo, v, f, l)
#else
   #define syncUnitActionCode(userinfo)                    ((void)0)
   #define syncUnitActionData(userinfo, v)                 ((void)0)
   #define syncUnitActionFLData(v, f, l)                   ((void)0)      
#endif

#ifdef SYNC_UnitAI   
   #define syncUnitAICode(userinfo)                         CODE_SYNC(cUnitAISync, userinfo)
   #define syncUnitAIData(userinfo, v)                      DATA_SYNC(cUnitAISync, userinfo, v, __FILE__, __LINE__)
   #define syncUnitAIFLData(userinfo, v, f, l)              DATA_SYNC(cUnitAISync, userinfo, v, f, l)
#else
   #define syncUnitAICode(userinfo)                         ((void)0)
   #define syncUnitAIData(userinfo, v)                      ((void)0)
   #define syncUnitAIFLData(v, f, l)                        ((void)0)      
#endif

#ifdef SYNC_Visibility
   #define syncVisibilityCode(userinfo)                     CODE_SYNC(cVisibilitySync, userinfo)
   #define syncVisibilityData(userinfo, v)                  DATA_SYNC(cVisibilitySync, userinfo, v, __FILE__, __LINE__)
   #define syncVisibilityFLData(userinfo, v, f, l)          DATA_SYNC(cVisibilitySync, userinfo, v, f, l)
#else
   #define syncVisibilityCode(userinfo)                     ((void)0)
   #define syncVisibilityData(userinfo, v)                  ((void)0)
   #define syncVisibilityFLData(v, f, l)                    ((void)0)      
#endif

#ifdef SYNC_GodPowers
   #define syncGodPowersCode(userinfo)                      CODE_SYNC(cGodPowersSync, userinfo)
   #define syncGodPowersData(userinfo, v)                   DATA_SYNC(cGodPowersSync, userinfo, v, __FILE__, __LINE__)
   #define syncGodPowersFLData(userinfo, v, f, l)           DATA_SYNC(cGodPowersSync, userinfo, v, f, l)
#else
   #define syncGodPowersCode(userinfo)                      ((void)0)
   #define syncGodPowersData(userinfo, v)                   ((void)0)
   #define syncGodPowersFLData(v, f, l)                     ((void)0)      
#endif

#ifdef SYNC_FinalRelease   
   #define syncFinalReleaseCode(userinfo)                   CODE_SYNC(cFinalReleaseSync, userinfo)
   #define syncFinalReleaseData(userinfo, v)                DATA_SYNC(cFinalReleaseSync, userinfo, v, __FILE__, __LINE__)
   #define syncFinalReleaseFLData(userinfo, v, f, l)        DATA_SYNC(cFinalReleaseSync, userinfo, v, f, l)
#else
   #define syncFinalReleaseCode(userinfo)                   ((void)0)
   #define syncFinalReleaseData(userinfo, v)                ((void)0)
   #define syncFinalReleaseFLData(v, f, l)                  ((void)0)      
#endif

#ifdef SYNC_FinalDetail   
   #define syncFinalDetailCode(userinfo)                    CODE_SYNC(cFinalDetailSync, userinfo)
   #define syncFinalDetailData(userinfo, v)                 DATA_SYNC(cFinalDetailSync, userinfo, v, __FILE__, __LINE__)
   #define syncFinalDetailFLData(userinfo, v, f, l)         DATA_SYNC(cFinalDetailSync, userinfo, v, f, l)
#else
   #define syncFinalDetailCode(userinfo)                    ((void)0)
   #define syncFinalDetailData(userinfo, v)                 ((void)0)
   #define syncFinalDetailFLData(v, f, l)                   ((void)0)      
#endif

#ifdef SYNC_Record   
   #define syncRecordCode(userinfo)                         CODE_SYNC(cRecordSync, userinfo)
   #define syncRecordData(userinfo, v)                      DATA_SYNC(cRecordSync, userinfo, v, __FILE__, __LINE__)
   #define syncRecordFLData(userinfo, v, f, l)              DATA_SYNC(cRecordSync, userinfo, v, f, l)
#else
   #define syncRecordCode(userinfo)                         ((void)0)
   #define syncRecordData(userinfo, v)                      ((void)0)
   #define syncRecordFLData(v, f, l)                        ((void)0)      
#endif

#ifdef SYNC_Checksum
   #define syncChecksum(tag)                                DATA_SYNC(cChecksumSync, gSync->getCurrentChecksum(tag), __FILE__, __LINE__)
#else
   #define syncChecksum(tag)                                ((void)0)
#endif


#ifdef SYNC_Trigger
   #define syncTriggerCode(userinfo)                       CODE_SYNC(cTriggerSync, userinfo)
   #define syncTriggerData(userinfo, v)                    DATA_SYNC(cTriggerSync, userinfo, v, __FILE__, __LINE__)
   #define syncTriggerFLData(userinfo, v, f, l)            DATA_SYNC(cTriggerSync, userinfo, v, f, l)
#else
   #define syncTriggerCode(userinfo)                       ((void)0)
   #define syncTriggerData(userinfo, v)                    ((void)0)
   #define syncTriggerFLData(v, f, l)                      ((void)0)
#endif

#ifdef SYNC_TriggerVar
   #define syncTriggerVar(v)                                DATA_SYNC(cTriggerVarSync, "BTriggerVarWriteData", v, __FILE__, __LINE__)
#else
   #define syncTriggerVar(v)                                ((void)0)
#endif

#ifdef SYNC_Anim   
   #define syncAnimCode(userinfo)                           CODE_SYNC(cAnimSync, userinfo)
   #define syncAnimData(userinfo, v)                        DATA_SYNC(cAnimSync, userinfo, v, __FILE__, __LINE__) 
   #define syncAnimFLData(userinfo, v, f, l)                DATA_SYNC(cAnimSync, userinfo, v, f, l)
#else
   #define syncAnimCode(userinfo)                           ((void)0)
   #define syncAnimData(userinfo, v)                        ((void)0)
   #define syncAnimFLData(v, f, l)                          ((void)0)      
#endif

#ifdef SYNC_Dopple   
   #define syncDoppleCode(userinfo)                         CODE_SYNC(cDoppleSync, userinfo)
   #define syncDoppleData(userinfo, v)                      DATA_SYNC(cDoppleSync, userinfo, v, __FILE__, __LINE__) 
   #define syncDoppleFLData(userinfo, v, f, l)              DATA_SYNC(cDoppleSync, userinfo, v, f, l)
#else
   #define syncDoppleCode(userinfo)                         ((void)0)
   #define syncDoppleData(userinfo, v)                      ((void)0)
   #define syncDoppleFLData(v, f, l)                        ((void)0)      
#endif

/*
#ifdef SYNC_ReplaceMe   
   #define syncReplaceMeCode(userinfo)                      CODE_SYNC(cReplaceMeSync, userinfo, __FILE__, __LINE__)
   #define syncReplaceMeData(userinfo, v)                   DATA_SYNC(cReplaceMeSync, userinfo, v, __FILE__, __LINE__)
   #define syncReplaceMeFLData(userinfo, v, f, l)           DATA_SYNC(cReplaceMeSync, userinfo, v, f, l)
#else
   #define syncReplaceMeCode(userinfo)                      ((void)0)
   #define syncReplaceMeData(userinfo, v)                   ((void)0)
   #define syncReplaceMeFLData(v, f, l)                     ((void)0)      
#endif
*/

template<long line, long f> long _syncGetRand(long tag, const char* file)
{
   long randval = gRandomManager._getRand(tag);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "getRand()", randval, file, line);
   }
   return(randval);
}

template<long line, long f> long _syncGetRandMax(long tag, long maxV, const char* file)
{
   long randval = gRandomManager._getRand(tag, maxV);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "getRandMax()", randval, file, line);
   }
   return(randval);
}

template<long line, long f> long _syncGetRandRange(long tag, long minV, long maxV, const char* file)
{
   long randval = gRandomManager._getRandRange(tag, minV, maxV);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "getRandRange()", randval, file, line);
   }
   return(randval);   
}

template<long line, long f> DWORD _syncGetRandRangeDWORD(long tag, DWORD minV, DWORD maxV, const char* file)
{
   DWORD randval = gRandomManager._getRandRange(tag, minV, maxV);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "getRandRangeDWORD()", randval, file, line);
   }
   return (randval);   
}

template<long line, long f> void _syncSetRandSeed(long tag, long seed, const char* file)
{
   gRandomManager._setSeed(tag, seed);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "setSeed()", seed, file, line);
   }
}

template<long line, long f> void _syncSetGlobalRandSeed(long seed, const char* file)
{
   gRandomManager._setGlobalSeed(seed);
   DATA_SYNC(cRandSync, "_syncSetGlobalRandSeed()", seed, file, line);
}

template<long line, long f> float _syncGetRandDistribution(long tag, const char* file)
{
   float fval = gRandomManager._getRandDistribution(tag);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "_syncGetRandDistribution()", fval, file, line);
   }
   return(fval);
}

template<long line, long f> float _syncGetRandRangeFloat(long tag, float min, float max, const char* file)
{
   float fval = gRandomManager._getRandRangeFloat(tag, min, max);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "_syncGetRandRangeFloat()", fval, file, line);
   }
   return(fval);
}

template<long line, long f> float _syncGetRandGaussian(long tag, float mean, float stdd, const char* file)
{
   float fval = gRandomManager._getRandGaussian(tag, mean, stdd);
   if (gRandomManager.getSync(tag))
   {
      DATA_SYNC(cRandSync, "_syncGetRandGaussian()", fval, file, line);
   }
   return(fval);
}

#ifndef BUILD_FINAL 

// undefine the macros from random.h
#undef getRand
#undef getRandMax
#undef getRandRange
#undef getRandRangeDWORD
#undef setRandSeed
#undef setGlobalRandSeed
#undef getRandDistribution
#undef getRandRangeFloat
#undef getRandGaussian

// define our own version that does syncing
#define syncRandVal(val)                 DATA_SYNC(cRandSync, "randVal", val, __FILE__, __LINE__)
#define getRand(tag)                     _syncGetRand<__LINE__,__COUNTER__>(tag, __FILE__)
#define getRandMax(tag, max)             _syncGetRandMax<__LINE__,__COUNTER__>(tag, max, __FILE__)
#define getRandRange(tag, min, max)      _syncGetRandRange<__LINE__,__COUNTER__>(tag, min, max, __FILE__)
#define getRandRangeDWORD(tag, min, max) _syncGetRandRangeDWORD<__LINE__,__COUNTER__>(tag, min, max, __FILE__)
#define setRandSeed(tag, seedval)        _syncSetRandSeed<__LINE__,__COUNTER__>(tag, seedval, __FILE__)
#define setGlobalRandSeed(seed)          _syncSetGlobalRandSeed<__LINE__,__COUNTER__>(seed, __FILE__)
#define getRandDistribution(tag)         _syncGetRandDistribution<__LINE__,__COUNTER__>(tag, __FILE__)
#define getRandRangeFloat(tag, min, max) _syncGetRandRangeFloat<__LINE__,__COUNTER__>(tag, min, max, __FILE__)
#define getRandGaussian(tag, mean, stdd) _syncGetRandGaussian<__LINE__,__COUNTER__>(tag, mean, stdd, __FILE__)
#else

#define syncRandVal(val)                 ((void)0)
#define setGlobalRandSeed(seed)          gRandomManager._setGlobalSeed(seed)

#endif