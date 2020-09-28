//==============================================================================
// commands.h
//
// Copyright (c) 1999-2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "command.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "powertypes.h"

//==============================================================================
// BWorkCommand
//==============================================================================
class BWorkCommand : public BCommand
{
   public:
      enum
      {
         cFlagAttackMove=cNumberCommandFlags,
         cNumberWorkCommandFlags
      };      
      
                              BWorkCommand(void);
      virtual                 ~BWorkCommand(void);

      BEntityID               getUnitID(void) const { return mUnitID; };
      long                    getHitZoneIndex() const { return( mHitZoneIndex ); }
      long                    getBuildProtoID(void) const { return mBuildProtoID; }
      float                   getRange(void) const { return(mRange); }
      BVector                 getTargetPosition(void) const { return(mTerrainPoint); }
      float                   getSpeedMultiplier(void) const { return(mSpeedMultiplier); }
      virtual BEntityID       getTargetID(void) const { return(mUnitID); }
      virtual long            getSquadMode() const { return mSquadMode; }
      virtual long            getAbilityID() const { return mAbilityID; }
      virtual float           getAngle() const { return mAngle; }            

      void                    setUnitID(BEntityID v) { mUnitID = v; }
      void                    setHitZoneIndex( long index ){ mHitZoneIndex = index; }
      void                    setBuildProtoID(long v) { mBuildProtoID=v; }
      void                    setRange(float v) { mRange=v; }
      void                    setTargetPosition(const BVector v) { mTerrainPoint=v; }
      void                    setSpeedMultiplier(float v) { mSpeedMultiplier = v; }
      void                    setSquadMode(long v) { mSquadMode = v; }
      void                    setAbilityID(long v) { mAbilityID = v; }
      void                    setAngle(float v) { mAngle=v; }

      // Override squad plotter
      bool                    getFlagOverridePosition() const { return (mFlagOverridePosition); }
      void                    setFlagOverridePosition(bool v) { mFlagOverridePosition = v; }
      bool                    getFlagOverrideRange() const { return (mFlagOverrideRange); }
      void                    setFlagOverrideRange(bool v) { mFlagOverrideRange = v; }

      virtual bool            processArmy(BArmy*  pArmy);

      virtual bool            meter(BCommand*  pLastSentCommand);                                                                     

      virtual void            sync();

      virtual void            serialize(BSerialBuffer& sb);
      virtual void            deserialize(BSerialBuffer& sb);

      

   protected:
      long                    mSquadMode;
      long                    mAbilityID;
      BEntityID               mUnitID;
      long                    mBuildProtoID;
      float                   mRange;
      BVector                 mTerrainPoint;
      float                   mSpeedMultiplier;
      long                    mHitZoneIndex;
      float                   mAngle;

      // Override squad plotter
      bool                    mFlagOverridePosition:1;
      bool                    mFlagOverrideRange:1;
};

//==============================================================================
// BPowerCommand
//==============================================================================
class BPowerCommand : public BCommand
{
   public:

      typedef enum BPowerCommandFlag
      {
         cGeneric0 = cNumberCommandFlags,
         cGeneric1,
         cNoCost,
         cNumberPowerCommandFlags,
      } BPowerCommandFlag;

      enum
      {
         cTypeUndefined,
         cTypeGrantPower,
         cTypeInvokePower,
         cTypeInvokeAbility,
         cTypeInvokePower2,   // the new stuff
      };

                              BPowerCommand(void);
      virtual                 ~BPowerCommand(void) { }

      // Get Accessors
      long                    getType(void) const { return(mType); }
      long                    getNumUses(void) const { return(mNumUses); }
      BProtoPowerID           getProtoPowerID(void) const { return mProtoPowerID; }
      BPowerLevel             getPowerLevel() const { return (mPowerLevel); }
      virtual long            getAbilityID() const { return (mAbilityID); }
      BEntityIDArray          getAbilitySquads() const { return (mAbilitySquads); }
      BEntityIDArray          getPowerUnits() const { return (mPowerUnits); }
      BVector                 getTargetLocation() const { return (mTargetLocation); }
      BVectorArray            getTargetLocations() const { return (mTargetLocations); }
      BEntityID               getSquadID() const { return mSquadID; }
      BPowerUserID            getPowerUserID() const { return (mPowerUserID); }

      // Set Accessors
      void                    setType(long v) { mType = v; }
      void                    setNumUses(long v) { mNumUses = v; }
      void                    setProtoPowerID(BProtoPowerID v) { mProtoPowerID = v; }
      void                    setPowerLevel(BPowerLevel v) { mPowerLevel = v; }
      void                    setAbilityID(int id){ mAbilityID = id; }
      void                    setAbilitySquads(BEntityIDArray units){ mAbilitySquads = units; }
      void                    setPowerUnits(BEntityIDArray units) { mPowerUnits = units; }
      void                    setTargetLocation(BVector location) { mTargetLocation = location; }
      void                    setTargetLocations(BVectorArray locations) { mTargetLocations = locations; }
      void                    setSquadID(BEntityID v) { mSquadID = v; }
      void                    setPowerUserID(BPowerUserID v) { mPowerUserID = v; }

      //Flag methods.
      bool                    getFlag(BPowerCommandFlag n) const { if (mFlags.isBitSet(n) > (DWORD)0) return(true); return(false); }
      void                    setFlag(BPowerCommandFlag n, bool v) { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); }     

      //Serialize, Deserialize, and Process.
      virtual void            serialize(BSerialBuffer& sb);
      virtual void            deserialize(BSerialBuffer& sb);

      virtual bool            processPlayer(void);

   protected:
      BEntityIDArray          mAbilitySquads;
      BEntityIDArray          mPowerUnits;
      BVectorArray            mTargetLocations;
      BVector                 mTargetLocation;   
      long                    mType;
      long                    mNumUses;
      BProtoPowerID           mProtoPowerID;
      BPowerLevel             mPowerLevel;
      long                    mAbilityID;      
      BEntityID               mSquadID;
      BPowerUserID            mPowerUserID;
};

//==============================================================================
// BPowerInputCommand
//==============================================================================
class BPowerInputCommand : public BCommand
{
public:

   typedef enum BPowerInputCommandFlag
   {
      cGeneric0 = cNumberCommandFlags,
      cGeneric1,
      cNoCost,
      cNumberPowerInputCommandFlags,
   } BPowerCommandFlag;

   enum
   {
      cTypeUndefined,
      cTypeConfirm,
      cTypePosition,
      cTypeDirection,
      cTypeShutdown,
   };

   BPowerInputCommand(void);
   virtual                 ~BPowerInputCommand(void) { }

   // Get Accessors
   long                    getType(void) const { return(mType); }
   BVector                 getVector() const { return (mVector); }
   BPowerUserID            getPowerUserID() const { return mPowerUserID; }

   // Set Accessors
   void                    setType(long v) { mType = v; }
   void                    setVector(BVector location) { mVector = location; }
   void                    setPowerUserID(BPowerUserID v) { mPowerUserID = v; }

   //Flag methods.
   bool                    getFlag(BPowerCommandFlag n) const { if (mFlags.isBitSet(n) > (DWORD)0) return(true); return(false); }
   void                    setFlag(BPowerCommandFlag n, bool v) { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); }     

   //Serialize, Deserialize, and Process.
   virtual void            serialize(BSerialBuffer& sb);
   virtual void            deserialize(BSerialBuffer& sb);

   virtual bool            processPlayer(void);

protected:
   BVector                 mVector;
   long                    mType;
   BPowerUserID            mPowerUserID;
};

//==============================================================================
// BTriggerCommand
//==============================================================================
class BTriggerCommand : public BCommand
{
public:

   enum
   {
      cUnused0=cNumberCommandFlags,
      cNumberTriggerCommandFlags
   };

   enum
   {
      cTypeUndefined,
      cTypeBroadcastAsyncCondition,
      cTypeActivateTriggerScript,
      cTypeActivateTrigger,
      cTypeDeactivateTrigger,
      cTypeBroadcastInputUILocationResult,
      cTypeBroadcastInputUIUnitResult,
      cTypeBroadcastInputUISquadResult,
      cTypeBroadcastInputUISquadListResult,
      cTypeBroadcastInputUIButtonResult,
      cTypeBroadcastInputUIPlaceSquadListResult,
      cTypeBroadcastInputUILocationMinigameResult,
   };

   BTriggerCommand(void);
   virtual                 ~BTriggerCommand(void) { }

   // Get Accessors
   long                    getType(void) const { return(mType); }
   BTriggerScriptID        getTriggerScriptID(void) const { return(mTriggerScriptID); }
   const BSimString&       getTriggerScript(void) const { return (mTriggerScript); }
   BTriggerID              getTriggerID(void) const { return(mTriggerID); }
   long                    getConditionID(void) const { return(mConditionID); }
   BTriggerVarID           getTriggerVarID(void) const { return(mTriggerVarID); }
   long                    getInputResult(void) const { return(mInputResult); }
   BVector                 getInputLocation(void) const { return(mInputLocation); }
   BEntityID               getInputEntity(void) const { return(mInputEntity); }
   BEntityID               getInputUnit(void) const { return(mInputUnit); }
   BEntityID               getInputSquad(void) const { return(mInputSquad); }
   BEntityIDArray          getInputSquadList() const { return(mInputSquadList); }
   float                   getInputQuality() const { return(mInputQuality); }
   bool                    getAsyncCondition(void) const { return(mAsyncCondition); }

   // Set Accessors
   void                    setType(long v) { mType = v; }
   void                    setTriggerScriptID(BTriggerScriptID v) { mTriggerScriptID = v; }
   void                    setTriggerScript(const BSimString& v) { mTriggerScript = v; }
   void                    setTriggerID(BTriggerID v) { mTriggerID = v; }
   void                    setConditionID(long v) { mConditionID = v; }
   void                    setTriggerVarID(BTriggerVarID v) { mTriggerVarID = v; }
   void                    setInputResult(long v) { mInputResult = v; }
   void                    setInputLocation(BVector v) { mInputLocation = v; }
   void                    setInputEntity(BEntityID v) { mInputEntity = v; }
   void                    setInputUnit(BEntityID v) { mInputUnit = v; }
   void                    setInputSquad(BEntityID v) { mInputSquad = v; }
   void                    setInputSquadList(BEntityIDArray v) { mInputSquadList = v; }
   void                    setInputModifiers(bool action, bool speed) { mInputActionModifier=action; mInputSpeedModifier=speed; }
   void                    setInputQuality(float v) { mInputQuality = v; }
   void                    setAsyncCondition(bool v) { mAsyncCondition = v; }

   //Serialize, Deserialize, and Process.
   virtual void            serialize(BSerialBuffer& sb);
   virtual void            deserialize(BSerialBuffer& sb);

   virtual bool            processPlayer(void);

protected:
   BEntityIDArray          mInputSquadList;
   BEntityID               mInputEntity;
   BEntityID               mInputUnit;
   BEntityID               mInputSquad;
   BVector                 mInputLocation;   
   float                   mInputQuality;
   long                    mType;
   BTriggerScriptID        mTriggerScriptID;
   BSimString              mTriggerScript;
   BTriggerID              mTriggerID;
   long                    mConditionID;
   BTriggerVarID           mTriggerVarID;
   long                    mInputResult;   
   bool                    mInputActionModifier;
   bool                    mInputSpeedModifier;
   bool                    mAsyncCondition;

};

//==============================================================================
// BBuildingCommand
//==============================================================================
class BBuildingCommand : public BCommand
{
   public:
      BBuildingCommand() : BCommand(cCommandBuilding, cNumberCommandFlags), mType(-1), mTargetID(-1), mTargetPosition(cOriginVector), mCount(0), mSocketID(cInvalidObjectID) {}
      virtual ~BBuildingCommand() {}

      void                    setType(long type) { mType=type; }
      void                    setTargetID(long id) { mTargetID=id; }
      void                    setTargetPosition(BVector pos) { mTargetPosition=pos; }
      void                    setCount(long count) { mCount=count; }
      void                    setSocketID(BEntityID socketID) { mSocketID=socketID; }

      virtual bool            processUnit(BUnit*  pUnit);

      virtual void            serialize(BSerialBuffer& sb);
      virtual void            deserialize(BSerialBuffer& sb);

   protected:
      long                    mType;
      long                    mTargetID;
      BVector                 mTargetPosition;
      long                    mCount;
      BEntityID               mSocketID;
};

//==============================================================================
// BGameCommand
//==============================================================================
class BGameCommand : public BCommand
{
   public:
      enum
      {
         cTypeQuickBuild,
         cTypeSwitchPlayer,
         cTypeAddResources,
         cTypeAddPopulation,
         cTypeFogOfWar,
         cTypeDestroySquad,
         cTypeDestroyUnit,
         cTypeResign,
         cTypeDisconnect,
         cTypeFlare,
         cTypeSetGlobalRallyPoint,
         cTypeClearGlobalRallyPoint,
         cTypeSetBuildingRallyPoint,
         cTypeClearBuildingRallyPoint,
         cTypeSelectPower,
         cTypeRevealMap,
         cTypeCreateSquad,
         cTypeCreateObject,
         cTypeTribute,
         cTypeRepair,
         cTypeAttackMove,
         cTypeUnitPower,
         cTypeLookAtPosBroadcast,
         cTypeGameSpeed,
         cTypeToggleAI,
         cTypeReverseHotDrop,
         cTypeActivateSkull,
         cTypeSubUpdating,
      };

      BGameCommand() : BCommand(cCommandGame, cNumberCommandFlags), mType(-1), mData(0), mData2(0), mPosition(cOriginVector), mPosition2(cOriginVector) {}
      virtual ~BGameCommand() {}

      void                    setType(long type) { mType=type; }
      void                    setData(long data) { mData=data; }
      void                    setData2(long data2) { mData2=data2; }
      void                    setDataFloat(float data) { mDataFloat=data; }
      void                    setData2Float(float data2) { mData2Float=data2; }
      void                    setPosition(BVector position) { mPosition=position; }
      void                    setPosition2(BVector position) { mPosition2=position; }

      long                    getGameCommandType() const { return mType; }

      virtual bool            processGame();
      virtual bool            processArmy(BArmy* pArmy);

      virtual void            serialize(BSerialBuffer& sb);
      virtual void            deserialize(BSerialBuffer& sb);

   protected:
      long                    mType;
      union
      {
         long                 mData;
         float                mDataFloat;
      };
      union
      {
         long                 mData2;
         float                mData2Float;
      };
      BVector                 mPosition;
      BVector                 mPosition2;
};


//==============================================================================
// BGeneralEventCommand
//==============================================================================
class BGeneralEventCommand : public BCommand
{
   public:
      enum
      {
         cEventFired,
      };

      BGeneralEventCommand() : BCommand(cCommandGeneralEvent, cNumberCommandFlags), mType(-1) {}
      virtual ~BGeneralEventCommand() {}

      void                    setType(long type) { mType=type; }

      virtual bool            processGame();

      virtual void            serialize(BSerialBuffer& sb);
      virtual void            deserialize(BSerialBuffer& sb);

      void                    addSubscriber(uint subscriber);
   protected:
      long                    mType;

      BDynamicArray<uint>      mSubscribers;


};