//==============================================================================
// triggermanager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "triggerscript.h"
#include "trigger.h"
#include "triggereffect.h"
#include "savegame.h"
#include "gamefilemacros.h"
#include "triggeruserdata.h"

// xcore
#include "containers\staticArray.h"

// xsystem
#include "xmlreader.h"

// Forward declarations
class BCost;


//==============================================================================
// class BTriggerManager
// The global trigger manager handles updating all the trigger systems in the game
//==============================================================================
class BTriggerManager
{
public:

   // Constructor / Destructor
   BTriggerManager();
   ~BTriggerManager(){}

   void init();   // called right after the database is initialized
   void reset();  // called when the game world is reset
   void shutdown(); // called when program is exiting

   static BTriggerVarType getTriggerVarTypeEnum(const BSimString& varTypeName);
   static int getTriggerOperatorTypeEnum(const BSimString& operatorTypeName);
   static int getTriggerMathOperatorTypeEnum(const BSimString& mathOperatorTypeName);
   static int getTriggerListPositionTypeEnum(const BSimString& listPositionTypeName);

   BTriggerScriptID createTriggerScriptFromFile(long fileDir, const BSimString& fileName);
   BTriggerScript* createTriggerScriptFromXML(BXMLNode triggerScriptNode);
   void activateTriggerScript(BTriggerScriptID triggerScriptID);

   void activateTrigger(BTriggerScriptID triggerScriptID, BTriggerID triggerID);
   void deactivateTrigger(BTriggerScriptID triggerScriptID, BTriggerID triggerID);
   void setAsyncTriggerConditionState(BTriggerScriptID triggerScriptID, BTriggerID triggerID, long conditionID, bool state);

   void addExternalPlayerID(BTriggerScriptID triggerScriptID, BPlayerID playerID);
   void addExternalProtoPowerID(BTriggerScriptID triggerScriptID, BProtoPowerID protoPowerID);
   void addExternalSquadID(BTriggerScriptID triggerScriptID, BEntityID squadID);
   void addExternalSquadIDs(BTriggerScriptID triggerScriptID, BEntityIDArray squadIDs);
   void addExternalUnitID(BTriggerScriptID triggerScriptID, BEntityID unitID);
   void addExternalUnitIDs(BTriggerScriptID triggerScriptID, BEntityIDArray unitIDs);
   void addExternalCost(BTriggerScriptID triggerScriptID, BCost cost);
   void addExternalLocation(BTriggerScriptID triggerScriptID, BVector location);
   void addExternalLocationList(BTriggerScriptID triggerScriptID, const BLocationArray& locationList);
   void addExternalFlag(BTriggerScriptID triggerScriptID, int index, bool flag);
   void addExternalFloat(BTriggerScriptID triggerScriptID, float value);


   void update(DWORD currentUpdateTime);
   BTriggerScript* getTriggerScript(BTriggerScriptID triggerScriptID);
   BTriggerScript* getTriggerScriptByIndex(uint index);
   uint getNumberTriggerScripts() const { return (mTriggerScripts.getSize()); }

   BTriggerScriptID getUniqueTriggerScriptID();
   void invalidateEntityID(BEntityID entityID);

   BTriggerScriptExternals* getTriggerScriptExternal(uint index);
   BTriggerScriptExternals* acquireTriggerScriptExternal(uint &index);
   void releaseTriggerScriptExternal(uint index);

   BTriggerUserClassManager* getUserClassManager() { return &mUserClassManager; }
   BTriggerTableManager* getTableManager() { return &mTableManager; }

#ifndef BUILD_FINAL
   const BSimString& getCurrentScriptName() const { return (mCurrentScriptName); }
   BTriggerScriptID getCurrentTriggerScriptID() const { return (mCurrentScriptID); }
   BTriggerID getCurrentTriggerID() const { return (mCurrentTriggerID); }
   void setCurrentScriptName(const BSimString& name) { mCurrentScriptName = name; }
   void setCurrentScriptID(BTriggerScriptID id) { mCurrentScriptID = id; }
   void setCurrentTriggerID(BTriggerID id) { mCurrentTriggerID = id; }   
   bool getFlagLogTriggerNames() { return mFlagLogTriggerNames; }
   void setFlagLogTriggerNames(bool v) { mFlagLogTriggerNames = v; }
   void incrementCreatePerfCount(uint plusCount = 1) { mCreatePerfCount += plusCount; }
   void incrementWorkPerfCount(uint plusCount = 1) { mWorkPerfCount += plusCount; }   
   void resetCreatePerfCount() { mCreatePerfCount = 0; }
   void resetWorkPerfCount() { mWorkPerfCount = 0; }
   void resetPerfSpike();
   void activatePerfSpike();
   void evaluateCreatePerfCount();
   void evaluateWorkPerfCount();
   bool getFlagPerfSpiked() { return (mFlagPerfSpiked); }
#endif

   bool getFlagPaused() const { return (mFlagPaused); }
   void setFlagPaused(bool v) { mFlagPaused = v; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   BTriggerUserClassManager mUserClassManager;
   BTriggerTableManager mTableManager;

   BSmallDynamicSimArray<BTriggerScript*> mTriggerScripts;
   BFreeList<BTriggerScriptExternals> mTriggerScriptExternals;
#ifndef BUILD_FINAL
   BSimString mCurrentScriptName;
   BTriggerScriptID mCurrentScriptID;
   BTriggerID mCurrentTriggerID;
   uint mCreatePerfCount;
   uint mWorkPerfCount;   
   DWORD mPerfSpikeTimeStamp;
   bool mFlagLogTriggerNames;
   bool mFlagPerfSpiked;
public:
   BStaticArray<BSimString, BTriggerEffect::cNumTriggerEffectTypes, false> mTriggerEffectNames;
protected:
#endif

   bool mFlagPaused;

   BTriggerScriptID mNextTriggerScriptID;
   BTriggerScriptID mBaseNextTriggerScriptID;
};

#ifdef ENABLE_BASSERT_NORETURN
   #ifdef XBOX
      #ifdef ENABLE_BASSERT_NORMAL
         #define BTRIGGER_ASSERT(exp) do { if (!(exp)) { BSimString errorMessage; errorMessage.format("ScriptName = %s, ScriptID = %d, TriggerID = %d", gTriggerManager.getCurrentScriptName().getPtr(), gTriggerManager.getCurrentTriggerScriptID(), gTriggerManager.getCurrentTriggerID()); BASSERT_TRIGGER_EXCEPTION(#exp, errorMessage, __FILE__, __LINE__); } } while (0)
         #define BTRIGGER_ASSERTM(exp, msg) do { if (!(exp)) { BSimString errorMessage; errorMessage.format("%s - ScriptName = %s, ScriptID = %d, TriggerID = %d", msg, gTriggerManager.getCurrentScriptName().getPtr(), gTriggerManager.getCurrentTriggerScriptID(), gTriggerManager.getCurrentTriggerID()); BASSERT_TRIGGER_EXCEPTION(#exp, errorMessage, __FILE__, __LINE__); } } while (0)
      #else
         #define BTRIGGER_ASSERT(exp)           ((void)0)
         #define BTRIGGER_ASSERTM(exp, msg)     ((void)0)
      #endif
   #else
      #define BTRIGGER_ASSERT(exp) ((void)0)
      #define BTRIGGER_ASSERTM(exp, msg) ((void)0)
   #endif
#else
   #define BTRIGGER_ASSERT(exp) ((void)0)
   #define BTRIGGER_ASSERTM(exp, msg) ((void)0)
#endif

// Global Trigger Manager
extern BTriggerManager gTriggerManager;