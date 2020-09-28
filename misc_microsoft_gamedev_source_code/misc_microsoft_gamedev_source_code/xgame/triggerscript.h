//==============================================================================
// triggerscript.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "cost.h"
#include "simtypes.h"
#include "triggervar.h"


// xsystem
#include "xmlreader.h"


// Forward declarations
class BTrigger;
class BTriggerVar;

// Vars.
__declspec(selectany) extern const BTriggerScriptID cInvalidTriggerScriptID = 0xFFFFFFFF;
__declspec(selectany) extern const BTriggerGroupID cInvalidTriggerGroupID = 0xFFFFFFFF;

// Typedefs
typedef BHashMap<BTriggerVarID, BTriggerVarID> BEditorIDToIDMap;


class BTriggerScriptExternals
{
public:

   enum
   {
      cFlag0 = 0,
      cFlag1,
      cNumberFlags,
   };

   BTriggerScriptExternals() { mFlags.setNumber(cNumberFlags); mFlags.clear(); }
   ~BTriggerScriptExternals() {}

   void clearAll() { mbPlayerID = false; mbProtoPowerID = false; mbSquadID = false; mbSquadIDs = false; mbUnitID = false; mbUnitIDs = false; mbCost = false; mbLocation = false; mbLocationList = false; }
   void addPlayerID(BPlayerID playerID) { mPlayerID = playerID; mbPlayerID = true; }
   void addProtoPowerID(BProtoPowerID protoPowerID) { mProtoPowerID = protoPowerID; mbProtoPowerID = true; }
   void addSquadID(BEntityID squadID) { mSquadID = squadID; mbSquadID = true; }
   void addSquadIDs(BEntityIDArray squadIDs){ mSquadIDs = squadIDs; mbSquadIDs = true; }
   void addUnitID(BEntityID unitID) { mUnitID = unitID; mbUnitID = true; }
   void addUnitIDs(BEntityIDArray unitIDs){ mUnitIDs = unitIDs; mbUnitIDs = true; }
   void addCost(BCost cost) { mCost = cost; mbCost = true; }
   void addLocation(BVector location) { mLocation = location; mbLocation = true; }
   void addLocationList(const BVectorArray& locationArray) { mLocationList = locationArray; mbLocationList = true; }
   void addFlag(int n, bool v) { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); mbFlags = true; }
   void addFloat(float value) { mFloat = value; mbFloat = true; }
   bool retrievePlayerID(BPlayerID &destPlayerID) const { if (mbPlayerID) destPlayerID = mPlayerID; return (mbPlayerID); }
   bool retrieveProtoPowerID(BProtoPowerID &destProtoPowerID) const { if (mbProtoPowerID) destProtoPowerID = mProtoPowerID; return (mbProtoPowerID); }
   bool retrieveSquadID(BEntityID &destSquadID) const { if (mbSquadID) destSquadID = mSquadID; return (mbSquadID); }
   bool retrieveSquadIDs(BEntityIDArray& destSquadIDs) const { if (mbSquadIDs) destSquadIDs = mSquadIDs; return (mbSquadIDs); }
   bool retrieveUnitID(BEntityID &destUnitID) const { if (mbUnitID) destUnitID = mUnitID; return (mbUnitID); }
   bool retrieveUnitIDs(BEntityIDArray& destUnitIDs) const { if (mbUnitIDs) destUnitIDs = mUnitIDs; return (mbUnitIDs); }
   bool retrieveCost(BCost &destCost) const { if (mbCost) destCost = mCost; return (mbCost); }
   bool retrieveLocation(BVector &destLocation) const { if (mbLocation) destLocation = mLocation; return (mbLocation); }
   bool retrieveLocationList(BVectorArray& destLocationList) const { if (mbLocationList) destLocationList = mLocationList; return (mbLocationList); }
   bool retrieveFlag(int n, bool& v) const { if (mbFlags) v = (mFlags.isBitSet(n) > (DWORD)0); return (mbFlags); }
   bool retrieveFloat(float& f) const { if (mbFloat) f =  mFloat; return (mbFloat); }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   
                                    // 70 bytes total
   BVector mLocation;               // 16 bytes
   BVectorArray mLocationList;    // 12 bytes
   
   BEntityIDArray mSquadIDs;        // 8 bytes        
   BEntityIDArray mUnitIDs;         // 8 bytes
   BCost mCost;                     // 8 bytes
   BBitArray mFlags;                // ?
   float mFloat;

   BEntityID mSquadID;              // 4 bytes
   BEntityID mUnitID;               // 4 bytes
   BPlayerID mPlayerID;             // 4 bytes
   BProtoPowerID mProtoPowerID;     // 4 bytes

   union
   {
      WORD mBitVars;
      struct
      {
         bool mbPlayerID      : 1;        // 1 byte
         bool mbProtoPowerID  : 1;        //
         bool mbSquadID       : 1;        //
         bool mbSquadIDs      : 1;        //
         bool mbUnitID        : 1;        //
         bool mbUnitIDs       : 1;        //
         bool mbCost          : 1;        //
         bool mbLocation      : 1;        //

         bool mbLocationList  : 1;        // 1 byte
         bool mbFlags         : 1;        //
         bool mbFloat         : 1;        //
      };
   };
};


class BTriggerGroup
{
public:
   BTriggerGroup(){}
   ~BTriggerGroup(){}

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const
   {
      BFAIL("BTriggerGroup::save -- this class is deprecated and only here to load old savegames");      
      return false;
   }
   bool load(BStream* pStream, int saveType)
   {
      GFREADARRAY(pStream,BTriggerID,mDescendantTriggers,uint16,10000);
      GFREADVAR(pStream,BTriggerGroupID,mGroupID);
      return true;
   }

protected:
                                                            // 12 bytes total
   BSmallDynamicSimArray<BTriggerID> mDescendantTriggers;   // 8 bytes
   BTriggerGroupID mGroupID;                                // 4 bytes
};


//==============================================================================
// class BTriggerScript
// A trigger script is a system of triggers (like one god power, or the scenario trigger script)
//==============================================================================
class BTriggerScript : public IPoolable
{
public:

   enum
   {
      cTypeInvalid         = 0,  // Trigger script whose type is unknown (hasn't been saved since type was added.)
      cTypeScenario        = 1,  // Trigger scripts that are part of a scenario.
      cTypePower           = 2,  // Trigger scripts that are powers.
      cTypeAbility         = 3,  // Trigger scripts that are abilities.
      cTypeTriggerScript   = 4,  // Trigger scripts that are "other trigger scripts" which may be spawned from another script or attached to a scenario externally

      cTypeMin = cTypeInvalid,
      cTypeMax = cTypeTriggerScript,
   };

   BTriggerScript(){ mExternalsIndex = UINT16_MAX; }
   ~BTriggerScript(){}
   virtual void onAcquire();
   virtual void onRelease();
   DECLARE_FREELIST(BTriggerScript, 4);
   void setID(BTriggerScriptID v) { mID = v; }
   BTriggerScriptID getID() const { return (mID); }

   bool loadFromXML(BXMLNode triggerScriptNode);
   bool isActive() const { return (mbActive); }
   bool isMarkedForCleanup() const { return (mbCleanup); }

   bool getFlagPaused() const { return (mFlagPaused); }
   void setFlagPaused(bool v) { mFlagPaused = v; }
   void toggleFlagPaused() { mFlagPaused = !mFlagPaused; }

   uint getNumberTriggerVars() const { return mTriggerVars.getSize(); }
   BTriggerVar* getTriggerVar(BTriggerVarID varID) { if (varID < 0 || varID >= mTriggerVars.getSize()) { BASSERT(0); return NULL; } else return (mTriggerVars[varID]); }
   BTrigger* getTrigger(BTriggerID triggerID);

   //bool BTriggerScript::writeVar(BTriggerVar *pVar, BTriggerVarType varTypeEnum, BSimString& varNodeText);
   bool writeVar(BTriggerVar *pVar, BTriggerVarType varTypeEnum, BXMLNode varNode, uint &numVarsRequiringAutoFixUp);

#ifndef BUILD_FINAL
   const BSimString& getName() const { return (mName); }
   uint getType() const { return (mType); }
   static uint getTriggerScriptTypeEnum(const BSimString& scriptTypeName);
#endif

   uint getActiveTriggersCount() const { return (mActiveTriggers.getSize()); }
   BTriggerID getActiveTriggerIDByIndex(uint index) const { if (index < mActiveTriggers.getSize()) return (mActiveTriggers[index]); return (cInvalidTriggerID); }

   void update(DWORD currentUpdateTime);

   void activate();
   void activateTrigger(BTriggerID triggerID);
   void deactivateTrigger(BTriggerID triggerID);
   void markForCleanup() { mbCleanup = true; mActiveTriggers.setNumber(0); mTriggerEvalList.setNumber(0); }
   void invalidateEntityID(BEntityID entityID);

   BTriggerScriptExternals* getExternals();

   void setFileInfo(int16 fileDir, const BSimString& fileName) { mFileDir=fileDir; mFileName=fileName; }
   int16 getFileDir() const { return mFileDir; }
   const BSimString& getFileName() const { return mFileName; }

   uint getNumberTriggers() const { return mTriggers.getSize(); }
   BTrigger* getTriggerByIndex(uint index) { return mTriggers[index]; }

   void setLogEffects(bool val) { mbLogEffects=val; }
   bool getLogEffects() const { return mbLogEffects; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   //AI log parameters
   long mPlayerID;    // If not set to -1, this indicates the player and AI that will be used for trace logging.
                        // Only applies to main AI script, script must register with aiBindLog trigger effect.
   bool mAILogFilterByGroup;
   long mAILogGroup;

protected:

   bool loadTriggerVarsFromXML(BXMLNode triggerVarsNode, uint maxNumTriggerVars);
   bool loadTriggersFromXML(BXMLNode triggersNode);
   bool remapTriggerEditorIDsToIDs(const BEditorIDToIDMap& editorIDToIDMap);

   bool HACK_doesStringContainOnlyNumbers(const BSimString& str) const;

   void releaseTriggerVarInstance(BTriggerVar *pVar);
   BTriggerVar* getTriggerVarInstance(BTriggerVarType type);

   void releaseTriggers();
   void releaseTriggerVars();
   void releaseExternals();
                                                                                          // 86 bytes total (83 bytes in non-final)
   BSmallDynamicSimArray<BEntityID> mInvalidatedEntityIDs;                                // 8 bytes
   BSmallDynamicSimArray<BTriggerID> mActiveTriggers;                                     // 8 bytes
   BSmallDynamicSimArray<BTriggerID> mTriggerEvalList;                                    // 8 bytes
   BSmallDynamicSimArray<BTrigger*> mTriggers;                                            // 8 bytes
   BSmallDynamicSimArray<BTriggerVar*> mTriggerVars;                                      // 8 bytes
   BSmallDynamicSimArray<BTriggerVarUnitList*> mTriggerVarUnitLists;                      // 8 bytes
   BSmallDynamicSimArray<BTriggerVarSquadList*> mTriggerVarSquadLists;                    // 8 bytes
   BSmallDynamicSimArray<BTriggerVarObjectList*> mTriggerVarObjectLists;                  // 8 bytes
   BSimString mFileName;
   BTriggerScriptID mID;                                                                  // 4 bytes
   int16 mFileDir;
   uint16 mExternalsIndex;                                                                // 2 bytes

#ifndef BUILD_FINAL
   BSimString mName;                                                                      // 2 bytes  (non-FINAL)
   BUInt8<BTriggerScriptType, BTriggerScript::cTypeMin, BTriggerScript::cTypeMax> mType;  // 1 byte   (non-FINAL)
#endif

   union
   {
      BYTE mBitVars;
      struct
      {
         bool mbActive   : 1;                                                                   // 1 byte
         bool mbUpdating : 1;                                                                   //
         bool mbCleanup  : 1;                                                                   //
         bool mbLogEffects : 1;                                                                 //
         bool mFlagPaused : 1;
      };
   };
};