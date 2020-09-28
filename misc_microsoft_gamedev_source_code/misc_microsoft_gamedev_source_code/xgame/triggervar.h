//==============================================================================
// triggervar.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "aitypes.h"
#include "savegame.h"
#include "cost.h"
#include "entityfilter.h"
#include "kbbase.h"
#include "kbsquad.h"
#include "kbsquadfilter.h"
#include "simtypes.h"
#include "syncmacros.h"
#include "trigger.h"


//==============================================================================
// Forward Declarations
//==============================================================================
class BTriggerVarTech;
class BTriggerVarTechStatus;
class BTriggerVarOperator;
class BTriggerVarProtoObject;
class BTriggerVarObjectType;
class BTriggerVarProtoSquad;
class BTriggerVarSound;
class BTriggerVarEntity;
class BTriggerVarEntityList;
class BTriggerVarCost;
class BTriggerVarTrigger;
class BTriggerVarTime;
class BTriggerVarPlayer;
class BTriggerVarUILocation;
class BTriggerVarUIEntity;
class BTriggerVarAnimType;
class BTriggerVarActionStatus;
class BTriggerVarPower;
class BTriggerVarBool;
class BTriggerVarFloat;
class BTriggerVarIterator;
class BTriggerVarTeam;
class BTriggerVarPlayerList;
class BTriggerVarTeamList;
class BTriggerVarPlayerState;
class BTriggerVarObjective;
class BTriggerVarUnit;
class BTriggerVarUnitList;
class BTriggerVarSquad;
class BTriggerVarSquadList;
class BTriggerVarUIUnit;
class BTriggerVarUISquad;
class BTriggerVarUISquadList;
class BTriggerVarString;
class BTriggerVarMessageIndex;
class BTriggerVarMessageJustify;
class BTriggerVarMessagePoint;
class BTriggerVarColor;
class BTriggerVarProtoObjectList;
class BTriggerVarObjectTypeList;
class BTriggerVarProtoSquadList;
class BTriggerVarTechList;
class BTriggerVarMathOperator;
class BTriggerVarObjectDataType;
class BTriggerVarObjectDataRelative;
class BTriggerVarCiv;
class BTriggerVarProtoObjectCollection;
class BTriggerVarObject;
class BTriggerVarObjectList;
class BTriggerVarGroup;
class BTriggerVarRefCountType;
class BTriggerVarUnitFlag;
class BTriggerVarSquadFlag;
class BTriggerVarLOSType;
class BTriggerVarEntityFilterSet;
class BTriggerVarPopBucket;
class BTriggerVarListPosition;
class BTriggerVarRelationType;
class BTriggerVarExposedAction;
class BTriggerVarSquadMode;
class BTriggerVarExposedScript;
class BTriggerVarKBBase;
class BTriggerVarKBBaseList;
class BTriggerVarDataScalar;
class BTriggerVarKBBaseQuery;
class BTriggerVarDesignLine;
class BTriggerVarLocStringID;
class BTriggerVarLeader;
class BTriggerVarCinematic;
class BTriggerVarTalkingHead;
class BTriggerVarFlareType;
class BTriggerVarCinematicTag;
class BTriggerVarIconType;
class BTriggerVarDifficulty;
class BTriggerVarInteger;
class BTriggerVarHUDItem;
class BTriggerVarFlashableUIItem;
class BTriggerVarControlType;
class BTriggerVarUIButton;
class BTriggerVarMissionType;
class BTriggerVarMissionState;
class BTriggerVarMissionTargetType;
class BTriggerVarIntegerList;
class BTriggerVarBidType;
class BTriggerVarBidState;
class BTriggerVarBuildingCommandState;
class BTriggerVarVector;
class BTriggerVarVectorList;
class BTriggerVarPlacementRule;
class BTriggerVarKBSquad;
class BTriggerVarKBSquadList;
class BTriggerVarKBSquadQuery;
class BTriggerVarAISquadAnalysis;
class BTriggerVarAISquadAnalysisComponent;
class BTriggerVarKBSquadFilterSet;
class BTriggerVarChatSpeaker;
class BTriggerVarRumbleType;
class BTriggerVarRumbleMotor;
class BTriggerVarTechDataCommandType;
class BTriggerVarSquadDataType;
class BTriggerVarEventType;
class BTriggerVarTimeList;
class BTriggerVarDesignLineList;
class BTriggerVarGameStatePredicate;
class BTriggerVarFloatList;
class BTriggerVarUILocationMinigame;
class BTriggerVarConcept;
class BTriggerVarConceptList;
class BTriggerVarUserClassType;

// NEWTRIGGERVARTYPE
// Add the forward declaration of your trigger var class type here.


//==============================================================================
// class BTriggerVar
// Base class of all trigger variable types.
//==============================================================================
class BTriggerVar  
{
public:

   // All of the trigger var types.
   enum
   {
      cVarTypeInvalid = 0,
      cVarTypeTech,
      cVarTypeTechStatus,
      cVarTypeOperator,
      cVarTypeProtoObject,
      cVarTypeObjectType,
      cVarTypeProtoSquad,
      cVarTypeSound,
      cVarTypeEntity,
      cVarTypeEntityList,
      cVarTypeTrigger,
      cVarTypeTime,
      cVarTypePlayer,
      cVarTypeUILocation,
      cVarTypeUIEntity,
      cVarTypeCost,
      cVarTypeAnimType,
      cVarTypeActionStatus,
      cVarTypePower,
      cVarTypeBool,
      cVarTypeFloat,
      cVarTypeIterator,
      cVarTypeTeam,
      cVarTypePlayerList,
      cVarTypeTeamList,
      cVarTypePlayerState,
      cVarTypeObjective,
      cVarTypeUnit,
      cVarTypeUnitList,
      cVarTypeSquad,
      cVarTypeSquadList,
      cVarTypeUIUnit,
      cVarTypeUISquad,
      cVarTypeUISquadList,
      cVarTypeString,
      cVarTypeMessageIndex,
      cVarTypeMessageJustify,
      cVarTypeMessagePoint,
      cVarTypeColor,
      cVarTypeProtoObjectList,
      cVarTypeObjectTypeList,
      cVarTypeProtoSquadList,
      cVarTypeTechList,
      cVarTypeMathOperator,
      cVarTypeObjectDataType,
      cVarTypeObjectDataRelative,
      cVarTypeCiv,
      cVarTypeProtoObjectCollection,
      cVarTypeObject,
      cVarTypeObjectList,
      cVarTypeGroup,
      cVarTypeRefCountType,
      cVarTypeUnitFlag,
      cVarTypeLOSType,
      cVarTypeEntityFilterSet,
      cVarTypePopBucket,
      cVarTypeListPosition,
      cVarTypeRelationType,
      cVarTypeExposedAction,
      cVarTypeSquadMode,
      cVarTypeExposedScript,
      cVarTypeKBBase,
      cVarTypeKBBaseList,
      cVarTypeDataScalar,
      cVarTypeKBBaseQuery,
      cVarTypeDesignLine,
      cVarTypeLocStringID,
      cVarTypeLeader,
      cVarTypeCinematic,
      cVarTypeFlareType,
      cVarTypeCinematicTag,
      cVarTypeIconType,
      cVarTypeDifficulty,
      cVarTypeInteger,
      cVarTypeHUDItem,
      cVarTypeControlType,
      cVarTypeUIButton,
      cVarTypeMissionType,
      cVarTypeMissionState,
      cVarTypeMissionTargetType,
      cVarTypeIntegerList,
      cVarTypeBidType,
      cVarTypeBidState,
      cVarTypeBuildingCommandState,
      cVarTypeVector,
      cVarTypeVectorList,
      cVarTypePlacementRule,
      cVarTypeKBSquad,
      cVarTypeKBSquadList,
      cVarTypeKBSquadQuery,
      cVarTypeAISquadAnalysis,
      cVarTypeAISquadAnalysisComponent,
      cVarTypeKBSquadFilterSet,
      cVarTypeChatSpeaker,
      cVarTypeRumbleType,
      cVarTypeRumbleMotor,
      cVarTypeTechDataCommandType,
      cVarTypeSquadDataType,
      cVarTypeEventType,
      cVarTypeTimeList,
      cVarTypeDesignLineList,
      cVarTypeGameStatePredicate,
      cVarTypeFloatList,
      cVarTypeUILocationMinigame,
      cVarTypeSquadFlag,
      cVarTypeFlashableUIItem,
      cVarTypeTalkingHead,
      cVarTypeConcept,
      cVarTypeConceptList,
      cVarTypeUserClassType,

      // NEWTRIGGERVARTYPE
      // Add the enumeration for your trigger var type here.
      // Adding var types is more involved than adding a new condition or effect (and may contain gotchas), if not 100% sure, ask Marc.
      // Editor side stuff, ask Andrew.

      cNumVarTypes, // Always last.  MUST BE IN RANGE OF 0-255!!!!!
      cMinVarType = cVarTypeInvalid,
      cMaxVarType = cNumVarTypes-1,
   };

   // Trigger var ID values
   enum
   {
      cVarIDInvalid = 0,
      cVarIDMax = 0xFFFF,
   };

   // Trigger var signature ID values.
   enum
   {
      cVarSigIDInvalid = 0,
      cVarSigIDMax = 0xFF,
   };

   // ID accessors.
   BTriggerVarID getID() const { return (mID); }
   void setID(BTriggerVarID v) { mID = v; }

   virtual BTriggerVarType getType() const = 0;
   bool isType(BTriggerVarType v) const { return (getType() == v); }

   // DEVELOPMENT ONLY
#ifndef BUILD_FINAL
   const BSimString& getName() const { return (mName); }
   void setName(const BSimString& name) { mName = name; }
   void outputDebugDataToXFS();
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif

   // Initialization stuff.
   void initialize();
   void checkInitialized() const;

   // Flags accessors.
   bool isNull() const { return (mbNull); }
   void setNull() { mbNull = true; }
   bool isUsed() const { return (!mbNull); }

   // Derived variable type accessors.
   BTriggerVarTech* asTech(void) { BASSERT(isType(cVarTypeTech)); return(reinterpret_cast<BTriggerVarTech*>(this)); }
   BTriggerVarTechStatus* asTechStatus(void) { BASSERT(isType(cVarTypeTechStatus)); return(reinterpret_cast<BTriggerVarTechStatus*>(this)); }
   BTriggerVarOperator* asOperator(void) { BASSERT(isType(cVarTypeOperator)); return(reinterpret_cast<BTriggerVarOperator*>(this)); }
   BTriggerVarProtoObject* asProtoObject(void) { BASSERT(isType(cVarTypeProtoObject)); return(reinterpret_cast<BTriggerVarProtoObject*>(this)); }
   BTriggerVarObjectType* asObjectType(void) { BASSERT(isType(cVarTypeObjectType)); return(reinterpret_cast<BTriggerVarObjectType*>(this)); }
   BTriggerVarProtoSquad* asProtoSquad(void) { BASSERT(isType(cVarTypeProtoSquad)); return(reinterpret_cast<BTriggerVarProtoSquad*>(this)); }
   BTriggerVarSound* asSound(void) { BASSERT(isType(cVarTypeSound)); return(reinterpret_cast<BTriggerVarSound*>(this)); }
   BTriggerVarEntity* asEntity(void) { BASSERT(isType(cVarTypeEntity)); return(reinterpret_cast<BTriggerVarEntity*>(this)); }
   BTriggerVarEntityList* asEntityList(void) { BASSERT(isType(cVarTypeEntityList)); return(reinterpret_cast<BTriggerVarEntityList*>(this)); }
   BTriggerVarTrigger* asTrigger(void) { BASSERT(isType(cVarTypeTrigger)); return(reinterpret_cast<BTriggerVarTrigger*>(this)); }
   BTriggerVarTime* asTime(void) { BASSERT(isType(cVarTypeTime)); return(reinterpret_cast<BTriggerVarTime*>(this)); }
   BTriggerVarPlayer* asPlayer(void) { BASSERT(isType(cVarTypePlayer)); return(reinterpret_cast<BTriggerVarPlayer*>(this)); }
   BTriggerVarUILocation* asUILocation(void) { BASSERT(isType(cVarTypeUILocation)); return(reinterpret_cast<BTriggerVarUILocation*>(this)); }
   BTriggerVarUIEntity* asUIEntity(void) { BASSERT(isType(cVarTypeUIEntity)); return(reinterpret_cast<BTriggerVarUIEntity*>(this)); }
   BTriggerVarCost* asCost(void) { BASSERT(isType(cVarTypeCost)); return(reinterpret_cast<BTriggerVarCost*>(this)); }
   BTriggerVarAnimType* asAnimType(void) { BASSERT(isType(cVarTypeAnimType)); return(reinterpret_cast<BTriggerVarAnimType*>(this)); }
   BTriggerVarActionStatus* asActionStatus(void) { BASSERT(isType(cVarTypeActionStatus)); return(reinterpret_cast<BTriggerVarActionStatus*>(this)); }
   BTriggerVarPower* asPower(void) { BASSERT(isType(cVarTypePower)); return(reinterpret_cast<BTriggerVarPower*>(this)); }
   BTriggerVarBool* asBool(void) { BASSERT(isType(cVarTypeBool)); return(reinterpret_cast<BTriggerVarBool*>(this)); }
   BTriggerVarFloat* asFloat(void) { BASSERT(isType(cVarTypeFloat)); return(reinterpret_cast<BTriggerVarFloat*>(this)); }
   BTriggerVarIterator* asIterator(void) { BASSERT(isType(cVarTypeIterator)); return(reinterpret_cast<BTriggerVarIterator*>(this)); }
   BTriggerVarTeam* asTeam(void) { BASSERT(isType(cVarTypeTeam)); return(reinterpret_cast<BTriggerVarTeam*>(this)); }
   BTriggerVarPlayerList* asPlayerList(void) { BASSERT(isType(cVarTypePlayerList)); return(reinterpret_cast<BTriggerVarPlayerList*>(this)); }
   BTriggerVarTeamList* asTeamList(void) { BASSERT(isType(cVarTypeTeamList)); return(reinterpret_cast<BTriggerVarTeamList*>(this)); }
   BTriggerVarPlayerState* asPlayerState(void) { BASSERT(isType(cVarTypePlayerState)); return(reinterpret_cast<BTriggerVarPlayerState*>(this)); }
   BTriggerVarObjective* asObjective(void) { BASSERT(isType(cVarTypeObjective)); return(reinterpret_cast<BTriggerVarObjective*>(this)); }
   BTriggerVarUnit* asUnit(void) { BASSERT(isType(cVarTypeUnit)); return(reinterpret_cast<BTriggerVarUnit*>(this)); }
   BTriggerVarUnitList* asUnitList(void) { BASSERT(isType(cVarTypeUnitList)); return(reinterpret_cast<BTriggerVarUnitList*>(this)); }
   BTriggerVarSquad* asSquad(void) { BASSERT(isType(cVarTypeSquad)); return(reinterpret_cast<BTriggerVarSquad*>(this)); }
   BTriggerVarSquadList* asSquadList(void) { BASSERT(isType(cVarTypeSquadList)); return(reinterpret_cast<BTriggerVarSquadList*>(this)); }
   BTriggerVarUIUnit* asUIUnit(void) { BASSERT(isType(cVarTypeUIUnit)); return(reinterpret_cast<BTriggerVarUIUnit*>(this)); }
   BTriggerVarUISquad* asUISquad(void) { BASSERT(isType(cVarTypeUISquad)); return(reinterpret_cast<BTriggerVarUISquad*>(this)); } 
   BTriggerVarUISquadList* asUISquadList() { BASSERT(isType(cVarTypeUISquadList)); return (reinterpret_cast<BTriggerVarUISquadList*>(this)); }
   BTriggerVarString* asString(void){ BASSERT(isType(cVarTypeString)); return(reinterpret_cast<BTriggerVarString*>(this)); }
   BTriggerVarMessageIndex* asMessageIndex(void){ BASSERT(isType(cVarTypeMessageIndex)); return(reinterpret_cast<BTriggerVarMessageIndex*>(this)); }
   BTriggerVarMessageJustify* asMessageJustify(void){ BASSERT(isType(cVarTypeMessageJustify)); return(reinterpret_cast<BTriggerVarMessageJustify*>(this)); }
   BTriggerVarMessagePoint* asMessagePoint(void){ BASSERT(isType(cVarTypeMessagePoint)); return(reinterpret_cast<BTriggerVarMessagePoint*>(this)); }
   BTriggerVarColor* asColor(void){ BASSERT(isType(cVarTypeColor)); return(reinterpret_cast<BTriggerVarColor*>(this)); }
   BTriggerVarProtoObjectList* asProtoObjectList(void) { BASSERT(isType(cVarTypeProtoObjectList)); return(reinterpret_cast<BTriggerVarProtoObjectList*>(this)); }
   BTriggerVarObjectTypeList* asObjectTypeList(void) { BASSERT(isType(cVarTypeObjectTypeList)); return(reinterpret_cast<BTriggerVarObjectTypeList*>(this)); }
   BTriggerVarProtoSquadList* asProtoSquadList(void) { BASSERT(isType(cVarTypeProtoSquadList)); return(reinterpret_cast<BTriggerVarProtoSquadList*>(this)); }
   BTriggerVarTechList* asTechList(void) { BASSERT(isType(cVarTypeTechList)); return(reinterpret_cast<BTriggerVarTechList*>(this)); }
   BTriggerVarMathOperator* asMathOperator( void ){ BASSERT( isType( cVarTypeMathOperator ) ); return( reinterpret_cast<BTriggerVarMathOperator*>( this ) ); }
   BTriggerVarObjectDataType* asObjectDataType( void ){ BASSERT( isType( cVarTypeObjectDataType ) ); return( reinterpret_cast<BTriggerVarObjectDataType*>( this ) ); }
   BTriggerVarObjectDataRelative* asObjectDataRelative( void ){ BASSERT( isType( cVarTypeObjectDataRelative ) ); return( reinterpret_cast<BTriggerVarObjectDataRelative*>( this ) ); }
   BTriggerVarCiv* asCiv(void) { BASSERT(isType(cVarTypeCiv)); return(reinterpret_cast<BTriggerVarCiv*>(this)); }
   BTriggerVarProtoObjectCollection* asProtoObjectCollection(void) { BASSERT(isType(cVarTypeProtoObjectCollection)); return(reinterpret_cast<BTriggerVarProtoObjectCollection*>(this)); }
   BTriggerVarObject* asObject(void) { BASSERT(isType(cVarTypeObject)); return(reinterpret_cast<BTriggerVarObject*>(this)); }
   BTriggerVarObjectList* asObjectList(void) { BASSERT(isType(cVarTypeObjectList)); return(reinterpret_cast<BTriggerVarObjectList*>(this)); }
   BTriggerVarGroup* asGroup(void) { BASSERT(isType(cVarTypeGroup)); return(reinterpret_cast<BTriggerVarGroup*>(this)); }
   BTriggerVarRefCountType* asRefCountType(void) { BASSERT(isType(cVarTypeRefCountType)); return(reinterpret_cast<BTriggerVarRefCountType*>(this)); }
   BTriggerVarUnitFlag* asUnitFlag(void) { BASSERT(isType(cVarTypeUnitFlag)); return(reinterpret_cast<BTriggerVarUnitFlag*>(this)); }
   BTriggerVarLOSType* asLOSType(void) { BASSERT(isType(cVarTypeLOSType)); return(reinterpret_cast<BTriggerVarLOSType*>(this)); }
   BTriggerVarEntityFilterSet* asEntityFilterSet(void) { BASSERT(isType(cVarTypeEntityFilterSet)); return(reinterpret_cast<BTriggerVarEntityFilterSet*>(this)); }
   BTriggerVarPopBucket* asPopBucket(void){ BASSERT(isType(cVarTypePopBucket)); return (reinterpret_cast<BTriggerVarPopBucket*>(this)); }
   BTriggerVarListPosition* asListPosition(void){ BASSERT(isType(cVarTypeListPosition)); return (reinterpret_cast<BTriggerVarListPosition*>(this)); }
   BTriggerVarRelationType* asRelationType(void){ BASSERT(isType(cVarTypeRelationType)); return (reinterpret_cast<BTriggerVarRelationType*>(this)); }
   BTriggerVarExposedAction* asExposedAction(void){ BASSERT(isType(cVarTypeExposedAction)); return (reinterpret_cast<BTriggerVarExposedAction*>(this)); }
   BTriggerVarSquadMode* asSquadMode(void){ BASSERT(isType(cVarTypeSquadMode)); return (reinterpret_cast<BTriggerVarSquadMode*>(this)); }
   BTriggerVarExposedScript* asExposedScript(){ BASSERT(isType(cVarTypeExposedScript)); return (reinterpret_cast<BTriggerVarExposedScript*>(this)); }
   BTriggerVarKBBase* asKBBase() { BASSERT(isType(cVarTypeKBBase)); return (reinterpret_cast<BTriggerVarKBBase*>(this)); }
   BTriggerVarKBBaseList* asKBBaseList() { BASSERT(isType(cVarTypeKBBaseList)); return (reinterpret_cast<BTriggerVarKBBaseList*>(this)); }
   BTriggerVarDataScalar* asDataScalar() { BASSERT(isType(cVarTypeDataScalar)); return (reinterpret_cast<BTriggerVarDataScalar*>(this)); }
   BTriggerVarKBBaseQuery* asKBBaseQuery() { BASSERT(isType(cVarTypeKBBaseQuery)); return (reinterpret_cast<BTriggerVarKBBaseQuery*>(this)); }
   BTriggerVarDesignLine* asDesignLine() { BASSERT(isType(cVarTypeDesignLine)); return (reinterpret_cast<BTriggerVarDesignLine*>(this)); }
   BTriggerVarLocStringID* asLocStringID() { BASSERT(isType(cVarTypeLocStringID)); return (reinterpret_cast<BTriggerVarLocStringID*>(this)); }
   BTriggerVarLeader* asLeader() { BASSERT(isType(cVarTypeLeader)); return (reinterpret_cast<BTriggerVarLeader*>(this)); }
   BTriggerVarCinematic* asCinematic() { BASSERT(isType(cVarTypeCinematic)); return (reinterpret_cast<BTriggerVarCinematic*>(this)); }
   BTriggerVarTalkingHead* asTalkingHead() { BASSERT(isType(cVarTypeTalkingHead)); return (reinterpret_cast<BTriggerVarTalkingHead*>(this)); }
   BTriggerVarFlareType* asFlareType() { BASSERT(isType(cVarTypeFlareType)); return (reinterpret_cast<BTriggerVarFlareType*>(this)); }
   BTriggerVarCinematicTag* asCinematicTag(){ BASSERT(isType(cVarTypeCinematicTag)); return (reinterpret_cast<BTriggerVarCinematicTag*>(this)); }
   BTriggerVarIconType* asIconType() { BASSERT(isType(cVarTypeIconType)); return (reinterpret_cast<BTriggerVarIconType*>(this)); }
   BTriggerVarDifficulty* asDifficulty() { BASSERT(isType(cVarTypeDifficulty)); return (reinterpret_cast<BTriggerVarDifficulty*>(this)); }
   BTriggerVarInteger* asInteger() { BASSERT(isType(cVarTypeInteger)); return (reinterpret_cast<BTriggerVarInteger*>(this)); }
   BTriggerVarHUDItem* asHUDItem() { BASSERT(isType(cVarTypeHUDItem)); return(reinterpret_cast<BTriggerVarHUDItem*>(this)); }
   BTriggerVarFlashableUIItem* asUIItem() { BASSERT(isType(cVarTypeFlashableUIItem)); return(reinterpret_cast<BTriggerVarFlashableUIItem*>(this)); }
   BTriggerVarControlType* asControlType() { BASSERT(isType(cVarTypeControlType)); return(reinterpret_cast<BTriggerVarControlType*>(this)); }
   BTriggerVarUIButton* asUIButton() { BASSERT(isType(cVarTypeUIButton)); return(reinterpret_cast<BTriggerVarUIButton*>(this)); }
   BTriggerVarMissionType* asMissionType() { BASSERT(isType(cVarTypeMissionType)); return (reinterpret_cast<BTriggerVarMissionType*>(this)); }
   BTriggerVarMissionState* asMissionState() { BASSERT(isType(cVarTypeMissionState)); return (reinterpret_cast<BTriggerVarMissionState*>(this)); }
   BTriggerVarMissionTargetType* asMissionTargetType() { BASSERT(isType(cVarTypeMissionTargetType)); return (reinterpret_cast<BTriggerVarMissionTargetType*>(this)); }
   BTriggerVarIntegerList* asIntegerList() { BASSERT(isType(cVarTypeIntegerList)); return (reinterpret_cast<BTriggerVarIntegerList*>(this)); }
   BTriggerVarBidType* asBidType() { BASSERT(isType(cVarTypeBidType)); return (reinterpret_cast<BTriggerVarBidType*>(this)); }
   BTriggerVarBidState* asBidState() { BASSERT(isType(cVarTypeBidState)); return (reinterpret_cast<BTriggerVarBidState*>(this)); }
   BTriggerVarBuildingCommandState * asBuildingCommandState() { BASSERT(isType(cVarTypeBuildingCommandState)); return (reinterpret_cast<BTriggerVarBuildingCommandState*>(this)); }
   BTriggerVarVector* asVector() { BASSERT(isType(cVarTypeVector)); return (reinterpret_cast<BTriggerVarVector*>(this)); }
   BTriggerVarVectorList* asVectorList() { BASSERT(isType(cVarTypeVectorList)); return (reinterpret_cast<BTriggerVarVectorList*>(this)); }
   BTriggerVarPlacementRule* asPlacementRule() { BASSERT(isType(cVarTypePlacementRule)); return (reinterpret_cast<BTriggerVarPlacementRule*>(this)); }
   BTriggerVarKBSquad* asKBSquad() { BASSERT(isType(cVarTypeKBSquad)); return (reinterpret_cast<BTriggerVarKBSquad*>(this)); }
   BTriggerVarKBSquadList* asKBSquadList() { BASSERT(isType(cVarTypeKBSquadList)); return (reinterpret_cast<BTriggerVarKBSquadList*>(this)); }
   BTriggerVarKBSquadQuery* asKBSquadQuery() { BASSERT(isType(cVarTypeKBSquadQuery)); return (reinterpret_cast<BTriggerVarKBSquadQuery*>(this)); }
   BTriggerVarAISquadAnalysis* asAISquadAnalysis() { BASSERT(isType(cVarTypeAISquadAnalysis)); return (reinterpret_cast<BTriggerVarAISquadAnalysis*>(this)); }
   BTriggerVarAISquadAnalysisComponent* asAISquadAnalysisComponent() { BASSERT(isType(cVarTypeAISquadAnalysisComponent)); return (reinterpret_cast<BTriggerVarAISquadAnalysisComponent*>(this)); }
   BTriggerVarKBSquadFilterSet* asKBSquadFilterSet() { BASSERT(isType(cVarTypeKBSquadFilterSet)); return (reinterpret_cast<BTriggerVarKBSquadFilterSet*>(this)); }
   BTriggerVarChatSpeaker* asChatSpeaker(void) { BASSERT(isType(cVarTypeChatSpeaker)); return(reinterpret_cast<BTriggerVarChatSpeaker*>(this)); }
   BTriggerVarRumbleType* asRumbleType() { BASSERT(isType(cVarTypeRumbleType)); return(reinterpret_cast<BTriggerVarRumbleType*>(this)); }
   BTriggerVarRumbleMotor* asRumbleMotor() { BASSERT(isType(cVarTypeRumbleMotor)); return(reinterpret_cast<BTriggerVarRumbleMotor*>(this)); }
   BTriggerVarTechDataCommandType* asTechDataCommandType() { BASSERT(isType(cVarTypeTechDataCommandType)); return (reinterpret_cast<BTriggerVarTechDataCommandType*>(this)); }
   BTriggerVarSquadDataType* asSquadDataType() { BASSERT(isType(cVarTypeSquadDataType)); return (reinterpret_cast<BTriggerVarSquadDataType*>(this)); }
   BTriggerVarEventType* asEventType() { BASSERT(isType(cVarTypeEventType)); return (reinterpret_cast<BTriggerVarEventType*>(this)); }
   BTriggerVarTimeList* asTimeList() { BASSERT(isType(cVarTypeTimeList)); return (reinterpret_cast<BTriggerVarTimeList*>(this)); }
   BTriggerVarDesignLineList* asDesignLineList() { BASSERT(isType(cVarTypeDesignLineList)); return (reinterpret_cast<BTriggerVarDesignLineList*>(this)); }
   BTriggerVarGameStatePredicate* asGameStatePredicate() { BASSERT(isType(cVarTypeGameStatePredicate)); return (reinterpret_cast<BTriggerVarGameStatePredicate*>(this)); }
   BTriggerVarFloatList* asFloatList() { BASSERT(isType(cVarTypeFloatList)); return (reinterpret_cast<BTriggerVarFloatList*>(this)); }
   BTriggerVarUILocationMinigame* asUILocationMinigame(void) { BASSERT(isType(cVarTypeUILocationMinigame)); return(reinterpret_cast<BTriggerVarUILocationMinigame*>(this)); }
   BTriggerVarSquadFlag* asSquadFlag(void) { BASSERT(isType(cVarTypeSquadFlag)); return(reinterpret_cast<BTriggerVarSquadFlag*>(this)); }
   BTriggerVarConcept* asConcept(void) { BASSERT(isType(cVarTypeConcept)); return(reinterpret_cast<BTriggerVarConcept*>(this)); }
   BTriggerVarConceptList* asConceptList(void) { BASSERT(isType(cVarTypeConceptList)); return(reinterpret_cast<BTriggerVarConceptList*>(this)); }
   BTriggerVarUserClassType* asUserClassType(void) { BASSERT(isType(cVarTypeUserClassType)); return(reinterpret_cast<BTriggerVarUserClassType*>(this)); }

   // NEWTRIGGERVARTYPE
   // Add your as<mytype>() function here.  Follow same format.

   // Save games
   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) 
   { 
      #ifndef BUILD_FINAL
         GFWRITESTRING(pStream, BSimString, mName, 200);
      #else
         BSimString tempStr;
         GFWRITESTRING(pStream, BSimString, tempStr, 200);
      #endif
      GFWRITEBITBOOL(pStream, mbNull); 
      GFWRITEBITBOOL(pStream, mbInitialized); 
      return true; 
   };
   virtual bool load(BStream* pStream, int saveType) 
   { 
      #ifndef BUILD_FINAL
         GFREADSTRING(pStream, BSimString, mName, 200);
      #else
         BSimString tempStr;
         GFREADSTRING(pStream, BSimString, tempStr, 200);
      #endif
      GFREADBITBOOL(pStream, mbNull); 
      GFREADBITBOOL(pStream, mbInitialized); 
      return true; 
   };

protected:

   // Constructor / Destructor.  Keep it protected so we never instantiate the base class.
   BTriggerVar(){}
   ~BTriggerVar(){}

   #ifndef BUILD_FINAL
      BSimString mName;
   #endif
   BUInt16<BTriggerVarID, UINT16_MIN, UINT16_MAX> mID;

   bool mbNull          : 1;
   bool mbInitialized   : 1;
};

__declspec(selectany) extern const BTriggerVarID cInvalidTriggerVarID = BTriggerVar::cVarIDInvalid;


//==============================================================================
// class BTriggerVarTech
//==============================================================================
class BTriggerVarTech : public BTriggerVar, IPoolable
{
public:

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTech, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTech); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapProtoTechID(mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarTechStatus
//==============================================================================
class BTriggerVarTechStatus : public BTriggerVar, IPoolable
{
public:

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTechStatus, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTechStatus); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapProtoTechID(mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarOperator
//==============================================================================
class BTriggerVarOperator : public BTriggerVar, IPoolable
{
public:

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarOperator, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeOperator); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mData;
};

//==============================================================================
// BTriggerVarMathOperator
//==============================================================================
class BTriggerVarMathOperator : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarMathOperator, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeMathOperator); }
      long readVar() const { checkInitialized(); return (mData); }
      void writeVar(long v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
      #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString);
      #endif
   protected:
      long mData;
};

//==============================================================================
// BTriggerVarProtoObject
//==============================================================================
class BTriggerVarProtoObject : public BTriggerVar, IPoolable
{
public:

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarProtoObject, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeProtoObject); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapProtoObjectID(mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarObjectType
//==============================================================================
class BTriggerVarObjectType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarObjectType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeObjectType); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapObjectType(mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarProtoSquad
//==============================================================================
class BTriggerVarProtoSquad : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarProtoSquad, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeProtoSquad); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData);  gSaveGame.remapProtoSquadID(mData);return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarTrigger
//==============================================================================
class BTriggerVarTrigger : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTrigger, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTrigger); }
   BTriggerID readVar() const { checkInitialized(); return (mData); }
   void writeVar(BTriggerID v) { mData = v; syncTriggerVar((DWORD)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BTriggerID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BTriggerID, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BTriggerID mData;
};


//==============================================================================
// BTriggerVarTime
//==============================================================================
class BTriggerVarTime : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTime, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTime); }
   DWORD readVar() const { checkInitialized(); return (mData); }
   void writeVar(DWORD v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, DWORD, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, DWORD, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%u ms.", mData); }
   #endif
protected:
   DWORD mData;
};


//==============================================================================
// BTriggerVarPlayer
//==============================================================================
class BTriggerVarPlayer : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarPlayer, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypePlayer); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
   #endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarEntity
//==============================================================================
class BTriggerVarEntity : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarEntity, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeEntity); }
   BEntityID readVar() const { checkInitialized(); return (mData); }
   void writeVar(BEntityID v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BEntityID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BEntityID, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BEntityID mData;
};


//==============================================================================
// BTriggerVarEntityList
//==============================================================================
class BTriggerVarEntityList : public BTriggerVar, IPoolable
{

public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarEntityList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeEntityList); }
   const BEntityIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BEntityIDArray& v) { mData = v; initialize(); }
   void addEntityID(BEntityID entityID) { mData.uniqueAdd(entityID); syncTriggerVar(entityID); initialize(); }
   void removeEntityID(BEntityID entityID) { syncTriggerVar(entityID); mData.removeValue(entityID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BEntityIDArray mData;
};


//==============================================================================
// BTriggerVarSound
//==============================================================================
class BTriggerVarSound : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarSound, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeSound); }
   const BSimString& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BSimString &v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITESTRING(pStream, BSimString, mData, 200); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADSTRING(pStream, BSimString, mData, 200); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%s", mData); }
   #endif
protected:
   BSimString mData;
};


//==============================================================================
// BTriggerVarUILocation
//==============================================================================
class BTriggerVarUILocation : public BTriggerVar, IPoolable
{
public:

   enum
   {
      cUILocationResultWaiting,
      cUILocationResultOK,
      cUILocationResultCancel,
      cUILocationResultUILockError,
   };

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cUILocationResultWaiting; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUILocation, 5);

   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUILocation); }

   long readResult() const { checkInitialized(); return (mResult); }
   BVector readLocation() const { checkInitialized(); return (mLocation); }

   void writeResult(long v) { mResult = v; syncTriggerVar(v); initialize(); }
   void writeLocation(BVector v) { mLocation = v; syncTriggerVar(v); initialize(); }

   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mResult); GFWRITEVECTOR(pStream, mLocation); return BTriggerVar::save(pStream, saveType); }

   virtual bool load(BStream* pStream, int saveType) 
   { 
      GFREADVAR(pStream, long, mResult); 
      GFREADVECTOR(pStream, mLocation); 
      if (mGameFileVersion < 2)
         mLocation.set(mLocation.y, mLocation.z, mLocation.w);
      return BTriggerVar::load(pStream, saveType); 
   }

   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mResult;
   BVector mLocation;
};


//==============================================================================
// BTriggerVarUIEntity
//==============================================================================
class BTriggerVarUIEntity : public BTriggerVar, IPoolable
{
public:

   enum
   {
      cUIEntityResultWaiting,
      cUIEntityResultOK,
      cUIEntityResultCancel,
      cUIEntityResultUILockError,
   };
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cUIEntityResultWaiting; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUIEntity, 5);

   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUIEntity); }
   long readResult() const { checkInitialized(); return (mResult); }
   BEntityID readEntity() const { checkInitialized(); return (mEntity); }

   void writeResult(long v) { mResult = v; syncTriggerVar(v); initialize(); }
   void writeEntity(BEntityID v) { mEntity = v; syncTriggerVar(v); initialize(); }

   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mResult); GFWRITEVAR(pStream, BEntityID, mResult); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mResult); GFREADVAR(pStream, BEntityID, mResult); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   long mResult;
   BEntityID mEntity;
};


//==============================================================================
// BTriggerVarCost
//==============================================================================
class BTriggerVarCost : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarCost, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeCost); }
   BCost& readVar() { checkInitialized(); return (mData); }
   void writeVar(BCost& v) { mData = v; initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BCost mData;
};


//==============================================================================
// BTriggerVarAnimType
//==============================================================================
class BTriggerVarAnimType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarAnimType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeAnimType); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapAnimType(mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarActionStatus
//==============================================================================
class BTriggerVarActionStatus : public BTriggerVar, IPoolable
{
public:

   enum
   {
      cNotDone       = 0,
      cDoneSuccess   = 1,
      cDoneFailure   = 2,
   };
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarActionStatus, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeActionStatus); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarPower
//==============================================================================
class BTriggerVarPower : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarPower, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypePower); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapProtoPowerID(mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarBool
//==============================================================================
class BTriggerVarBool : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarBool, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeBool); }
   bool readVar() const { checkInitialized(); return (mData); }
   void writeVar(bool v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, bool, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, bool, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString = (mData) ? "TRUE" : "FALSE"; }
   #endif
protected:
   bool mData;
};


//==============================================================================
// BTriggerVarFloat
//==============================================================================
class BTriggerVarFloat : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarFloat, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeFloat); }
   float readVar() const { checkInitialized(); return (mData); }
   void writeVar(float v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, float, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, float, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%f", mData); }
   #endif
protected:
   float mData;
};


//==============================================================================
// BTriggerVarIterator
//==============================================================================
class BTriggerVarIterator : public BTriggerVar, IPoolable
{

public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mListVar = BTriggerVar::cVarIDInvalid; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarIterator, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeIterator); }


   BTriggerVarID getListVarID() const { return (mListVar); }
   void attachIterator(BTriggerVarID v) { mListVar = v; mVisitedEntities.clear(); mVisitedPlayers.clear(); mVisitedTeams.clear(); mVisitedUnits.clear(); mVisitedSquads.clear(); mVisitedObjects.clear(); mVisitedVectors.clear(); mVisitedKBBases.clear(); mVisitedProtoObjects.clear(); mVisitedProtoSquads.clear(); mVisitedObjectTypes.clear(); mVisitedTechs.clear(); mVisitedKBSquads.clear(); initialize(); }

   bool isEntityVisited(BEntityID entityID) { checkInitialized(); return (mVisitedEntities.find(entityID) != cInvalidIndex); }
   bool isPlayerVisited(long playerID) { checkInitialized(); return (mVisitedPlayers.find(playerID) != cInvalidIndex); }
   bool isTeamVisited(BTeamID teamID) { checkInitialized(); return (mVisitedTeams.find(teamID) != cInvalidIndex); }
   bool isUnitVisited(BEntityID unitID) { checkInitialized(); return (mVisitedUnits.find(unitID) != cInvalidIndex); }
   bool isSquadVisited(BEntityID squadID) { checkInitialized(); return (mVisitedSquads.find(squadID) != cInvalidIndex); }
   bool isObjectVisited(BEntityID objectID) { checkInitialized(); return (mVisitedObjects.find(objectID) != cInvalidIndex); }
   bool isVectorVisited(BVector vector) { checkInitialized(); return (mVisitedVectors.find(vector) != cInvalidIndex); }
   bool isKBBaseVisited(BKBBaseID kbBaseID) { checkInitialized(); return (mVisitedKBBases.find(kbBaseID) != cInvalidIndex); }
   bool isProtoObjectVisited(BProtoObjectID protoObjectID) { checkInitialized(); return (mVisitedProtoObjects.find(protoObjectID) != cInvalidIndex); }
   bool isProtoSquadVisited(BProtoSquadID protoSquadID) { checkInitialized(); return (mVisitedProtoSquads.find(protoSquadID) != cInvalidIndex); }
   bool isObjectTypeVisited(BObjectTypeID objectTypeID) { checkInitialized(); return (mVisitedObjectTypes.find(objectTypeID) != cInvalidIndex); }
   bool isTechVisited(BProtoTechID protoTechID) { checkInitialized(); return (mVisitedTechs.find(protoTechID) != cInvalidIndex); }
   bool isKBSquadVisited(BKBSquadID KBSquadID) { checkInitialized(); return (mVisitedKBSquads.find(KBSquadID) != cInvalidIndex); }

   void visitEntity(BEntityID entityID) { checkInitialized(); syncTriggerVar(entityID); mVisitedEntities.uniqueAdd(entityID); }
   void visitPlayer(long playerID) { checkInitialized(); syncTriggerVar(playerID); mVisitedPlayers.uniqueAdd(playerID); }
   void visitTeam(BTeamID teamID) { checkInitialized(); syncTriggerVar(teamID); mVisitedTeams.uniqueAdd(teamID); }
   void visitUnit(BEntityID unitID) { checkInitialized(); syncTriggerVar(unitID); mVisitedUnits.uniqueAdd(unitID); }
   void visitSquad(BEntityID squadID) { checkInitialized(); syncTriggerVar(squadID); mVisitedSquads.uniqueAdd(squadID); }
   void visitObject(BEntityID objectID) { checkInitialized(); mVisitedObjects.uniqueAdd(objectID); }
   void visitVector(BVector vector) { checkInitialized(); syncTriggerVar(vector); mVisitedVectors.uniqueAdd(vector); }
   void visitKBBase(BKBBaseID kbBaseID) { checkInitialized(); syncTriggerVar(kbBaseID); mVisitedKBBases.uniqueAdd(kbBaseID); }
   void visitProtoObject(BProtoObjectID protoObjectID) { checkInitialized(); syncTriggerVar(protoObjectID); mVisitedProtoObjects.uniqueAdd(protoObjectID); }
   void visitProtoSquad(BProtoSquadID protoSquadID) { checkInitialized(); syncTriggerVar(protoSquadID); mVisitedProtoSquads.uniqueAdd(protoSquadID); }
   void visitObjectType(BObjectTypeID objectTypeID) { checkInitialized(); syncTriggerVar(objectTypeID); mVisitedObjectTypes.uniqueAdd(objectTypeID); }
   void visitTech(BProtoTechID protoTechID) { checkInitialized(); syncTriggerVar(protoTechID); mVisitedTechs.uniqueAdd(protoTechID); }
   void visitKBSquad(BKBSquadID KBSquadID) { checkInitialized(); syncTriggerVar(KBSquadID); mVisitedKBSquads.uniqueAdd(KBSquadID); }


   virtual bool save(BStream* pStream, int saveType) 
   { 
      GFWRITEVAR(pStream, BTriggerVarID, mListVar);
      GFWRITEARRAY(pStream, BEntityID, mVisitedEntities, uint16, 2000);
      GFWRITEARRAY(pStream, BPlayerID, mVisitedPlayers, uint8, 20);
      GFWRITEARRAY(pStream, BTeamID, mVisitedTeams, uint8, 10);
      GFWRITEARRAY(pStream, BEntityID, mVisitedUnits, uint16, 2000);
      GFWRITEARRAY(pStream, BEntityID, mVisitedSquads, uint16, 2000);
      GFWRITEARRAY(pStream, BEntityID, mVisitedObjects, uint16, 2000);
      GFWRITEARRAY(pStream, BVector, mVisitedVectors, uint16, 10000);
      GFWRITEARRAY(pStream, BKBBaseID, mVisitedKBBases, uint16, 1000);
      GFWRITEARRAY(pStream, BProtoObjectID, mVisitedProtoObjects, uint16, 2000);
      GFWRITEARRAY(pStream, BProtoSquadID, mVisitedProtoSquads, uint16, 2000);
      GFWRITEARRAY(pStream, BObjectTypeID, mVisitedObjectTypes, uint16, 2000);
      GFWRITEARRAY(pStream, BProtoTechID, mVisitedTechs, uint16, 2000);
      GFWRITEARRAY(pStream, BKBSquadID, mVisitedKBSquads, uint16, 2000);
      return BTriggerVar::save(pStream, saveType);
   }
   virtual bool load(BStream* pStream, int saveType) 
   { 
      GFREADVAR(pStream, BTriggerVarID, mListVar);
      GFREADARRAY(pStream, BEntityID, mVisitedEntities, uint16, 2000);
      GFREADARRAY(pStream, BPlayerID, mVisitedPlayers, uint8, 20);
      GFREADARRAY(pStream, BTeamID, mVisitedTeams, uint8, 10);
      GFREADARRAY(pStream, BEntityID, mVisitedUnits, uint16, 2000);
      GFREADARRAY(pStream, BEntityID, mVisitedSquads, uint16, 2000);
      GFREADARRAY(pStream, BEntityID, mVisitedObjects, uint16, 2000);
      GFREADARRAY(pStream, BVector, mVisitedVectors, uint16, 10000);
      GFREADARRAY(pStream, BKBBaseID, mVisitedKBBases, uint16, 2000);
      GFREADARRAY(pStream, BProtoObjectID, mVisitedProtoObjects, uint16, 2000);
      GFREADARRAY(pStream, BProtoSquadID, mVisitedProtoSquads, uint16, 2000);
      GFREADARRAY(pStream, BObjectTypeID, mVisitedObjectTypes, uint16, 2000);
      GFREADARRAY(pStream, BProtoTechID, mVisitedTechs, uint16, 2000);
      GFREADARRAY(pStream, BKBSquadID, mVisitedKBSquads, uint16, 2000);
      gSaveGame.remapProtoObjectIDs(mVisitedProtoObjects);
      gSaveGame.remapProtoSquadIDs(mVisitedProtoSquads);
      gSaveGame.remapObjectTypes(mVisitedObjectTypes);
      gSaveGame.remapProtoTechIDs(mVisitedTechs);
      return BTriggerVar::load(pStream, saveType);
   }

   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BTriggerVarID mListVar;
   BEntityIDArray mVisitedEntities;
   BPlayerIDArray mVisitedPlayers;
   BTeamIDArray mVisitedTeams;
   BEntityIDArray mVisitedUnits;
   BEntityIDArray mVisitedSquads;
   BEntityIDArray mVisitedObjects;
   BVectorArray mVisitedVectors;
   BKBBaseIDArray mVisitedKBBases;
   BProtoObjectIDArray mVisitedProtoObjects;
   BProtoSquadIDArray mVisitedProtoSquads;
   BObjectTypeIDArray mVisitedObjectTypes;
   BTechIDArray mVisitedTechs;
   BKBSquadIDArray mVisitedKBSquads;

};


//==============================================================================
// BTriggerVarTeam
//==============================================================================
class BTriggerVarTeam : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTeam, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTeam); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
   #endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarPlayerList
//==============================================================================
class BTriggerVarPlayerList : public BTriggerVar, IPoolable
{

public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarPlayerList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypePlayerList); }
   const BPlayerIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BPlayerIDArray& v) { mData = v; initialize(); }
   void addPlayer(long playerID) { mData.uniqueAdd(playerID); syncTriggerVar(playerID); initialize(); }
   void removePlayer(long playerID) { mData.removeValue(playerID); syncTriggerVar(playerID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BPlayerID, mData, uint8, 20); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BPlayerID, mData, uint8, 20); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BPlayerIDArray mData;
};


//==============================================================================
// BTriggerVarTeamList
//==============================================================================
class BTriggerVarTeamList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTeamList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTeamList); }
   const BTeamIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BTeamIDArray& v) { mData = v; initialize(); }
   void addTeam(BTeamID teamID) { mData.uniqueAdd(teamID); syncTriggerVar(teamID); initialize(); }
   void removeTeam(BTeamID teamID) { mData.removeValue(teamID); syncTriggerVar(teamID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BTeamID, mData, uint8, 20); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BTeamID, mData, uint8, 20); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BTeamIDArray mData;
};


//==============================================================================
// BTriggerVarPlayerState
//==============================================================================
class BTriggerVarPlayerState : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarPlayerState, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypePlayerState); }
      BPlayerState readVar() const { checkInitialized(); return (mData); }
      void writeVar(BPlayerState v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BPlayerState, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BPlayerState, mData); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      BPlayerState mData;
};


//==============================================================================
// BTriggerVarObjective
//==============================================================================
class BTriggerVarObjective : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarObjective, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeObjective); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarString
//==============================================================================
class BTriggerVarString : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarString, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeString); }
      const BSimString& readVar() const { checkInitialized(); return (mData); }
      void writeVar(const BSimString& v){ mData = v; initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITESTRING(pStream, BSimString, mData, 1000); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADSTRING(pStream, BSimString, mData, 1000); return BTriggerVar::load(pStream, saveType); }
      #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString = mData; }
      #endif
   protected:
      BSimString mData;
};


//==============================================================================
// BTriggerVarMessageIndex
//==============================================================================
class BTriggerVarMessageIndex : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarMessageIndex, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeMessageIndex); }
      long readVar() const { checkInitialized(); return (mData); }
      void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      long mData;
};


//==============================================================================
// BTriggerVarMessageJustify
//==============================================================================
class BTriggerVarMessageJustify : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarMessageJustify, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeMessageJustify); }
      long readVar() const { checkInitialized(); return (mData); }
      void writeVar(long v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      long mData;
};


//==============================================================================
// BTriggerVarMessagePoint
//==============================================================================
class BTriggerVarMessagePoint : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarMessagePoint, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeMessagePoint); }
      float readVar() const { checkInitialized(); return (mData); }
      void writeVar(float v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, float, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, float, mData); return BTriggerVar::load(pStream, saveType); }

   protected:
      float mData;
};

//==============================================================================
// BTriggerVarColor
//==============================================================================
class BTriggerVarColor : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarColor, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeColor); }
      BColor readVar() const { checkInitialized(); return (mData); }
      void writeVar(BColor v){ mData = v; syncTriggerVar(v.asDWORD()); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BColor, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BColor, mData); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      BColor mData;
};

//==============================================================================
// BTriggerVarUnit
//==============================================================================
class BTriggerVarUnit : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUnit, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUnit); }
   BEntityID readVar() const { checkInitialized(); return (mData); }
   void writeVar(BEntityID v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BEntityID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BEntityID, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BEntityID mData;
};


//==============================================================================
// BTriggerVarUnitList
//==============================================================================
class BTriggerVarUnitList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUnitList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUnitList); }
   const BEntityIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BEntityIDArray& v) { mData = v; initialize(); }
   void addUnit(BEntityID unitID);
   void removeUnit(BEntityID unitID) { syncTriggerVar(unitID); mData.removeValue(unitID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BEntityIDArray mData;
};


//==============================================================================
// BTriggerVarSquad
//==============================================================================
class BTriggerVarSquad : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarSquad, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeSquad); }
   BEntityID readVar() const { checkInitialized(); return (mData); }
   void writeVar(BEntityID v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BEntityID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BEntityID, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BEntityID mData;
};


//==============================================================================
// BTriggerVarSquadList
//==============================================================================
class BTriggerVarSquadList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarSquadList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeSquadList); }
   const BEntityIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BEntityIDArray& v) { mData = v; initialize(); }
   void addSquad(BEntityID squadID);
   void removeSquad(BEntityID squadID) { syncTriggerVar(squadID); mData.removeValue(squadID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BEntityIDArray mData;
};


//==============================================================================
// BTriggerVarUIUnit
//==============================================================================
class BTriggerVarUIUnit : public BTriggerVar, IPoolable
{
public:

   enum
   {
      cUIUnitResultWaiting,
      cUIUnitResultOK,
      cUIUnitResultCancel,
      cUIUnitResultUILockError,
   };

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cUIUnitResultWaiting; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUIUnit, 5);

   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUIUnit); }

   long readResult() const { checkInitialized(); return (mResult); }
   BEntityID readUnit() const { checkInitialized(); return (mUnit); }

   void writeResult(long v) { mResult = v; syncTriggerVar(v); initialize(); }
   void writeUnit(BEntityID unitID) { mUnit = unitID; syncTriggerVar(unitID); initialize(); }

   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mResult); GFWRITEVAR(pStream, BEntityID, mUnit); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mResult); GFREADVAR(pStream, BEntityID, mUnit); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mResult;
   BEntityID mUnit;
};


//==============================================================================
// BTriggerVarUISquad
//==============================================================================
class BTriggerVarUISquad : public BTriggerVar, IPoolable
{
public:

   enum
   {
      cUISquadResultWaiting,
      cUISquadResultOK,
      cUISquadResultCancel,
      cUISquadResultUILockError,
   };

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cUISquadResultWaiting; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUISquad, 5);

   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUISquad); }

   long readResult() const { checkInitialized(); return (mResult); }
   BEntityID readSquad() const { checkInitialized(); return (mSquad); }

   void writeResult(long v) { mResult = v; syncTriggerVar(v); initialize(); }
   void writeSquad(BEntityID squadID) { mSquad = squadID; syncTriggerVar(squadID); initialize(); }

   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mResult); GFWRITEVAR(pStream, BEntityID, mSquad); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mResult); GFREADVAR(pStream, BEntityID, mSquad); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   long mResult;
   BEntityID mSquad;
};

//==============================================================================
// BTriggerVarUISquadList
//==============================================================================
class BTriggerVarUISquadList : public BTriggerVar, IPoolable
{
   public:

      enum
      {
         cUISquadListResultWaiting,
         cUISquadListResultOK,
         cUISquadListResultCancel,
         cUISquadListResultUILockError,
      };

      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cUISquadListResultWaiting; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarUISquadList, 5);

      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUISquadList); }

      long readResult() const { checkInitialized(); return (mResult); }
      BEntityIDArray readSquadList() const { checkInitialized(); return (mSquadList); }

      void writeResult(long v) { mResult = v; syncTriggerVar(v); initialize(); }
      void writeSquadList(BEntityIDArray squadList) { mSquadList = squadList; initialize(); }

      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mResult); GFWRITEARRAY(pStream, BEntityID, mSquadList, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mResult); GFREADARRAY(pStream, BEntityID, mSquadList, uint16, 2000); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      long mResult;
      BEntityIDArray mSquadList;
};

//==============================================================================
// BTriggerVarProtoObjectList
//==============================================================================
class BTriggerVarProtoObjectList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarProtoObjectList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeProtoObjectList); }
   const BProtoObjectIDArray& readVar() const { checkInitialized(); return (mData); }
   BProtoObjectIDArray& readVar() { checkInitialized(); return (mData); }
   void writeVar(const BProtoObjectIDArray& v) { mData = v; initialize(); }
   void addProtoObjectID(long protoObjectID) { mData.uniqueAdd(protoObjectID); syncTriggerVar(protoObjectID); initialize(); }
   void removeProtoObjectID(long protoObjectID) { mData.removeValue(protoObjectID); syncTriggerVar(protoObjectID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BProtoObjectID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BProtoObjectID, mData, uint16, 2000); gSaveGame.remapProtoObjectIDs(mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BProtoObjectIDArray mData;
};


//==============================================================================
// BTriggerVarObjectTypeList
//==============================================================================
class BTriggerVarObjectTypeList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarObjectTypeList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeObjectTypeList); }
   const BObjectTypeIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BObjectTypeIDArray& v) { mData = v; initialize(); }
   void addObjectTypeID(long objectTypeID) { mData.uniqueAdd(objectTypeID); syncTriggerVar(objectTypeID); initialize(); }
   void removeObjectTypeID(long objectTypeID) { mData.removeValue(objectTypeID); syncTriggerVar(objectTypeID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BObjectTypeID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BObjectTypeID, mData, uint16, 2000); gSaveGame.remapObjectTypes(mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BObjectTypeIDArray mData;
};


//==============================================================================
// BTriggerVarProtoSquadList
//==============================================================================
class BTriggerVarProtoSquadList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarProtoSquadList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeProtoSquadList); }
   const BProtoSquadIDArray& readVar() const { checkInitialized(); return (mData); }
   BProtoSquadIDArray& readVar() { checkInitialized(); return (mData); }
   void writeVar(const BProtoSquadIDArray& v) { mData = v; initialize(); }
   void addProtoSquadID(long protoSquadID) { mData.uniqueAdd(protoSquadID); syncTriggerVar(protoSquadID); initialize(); }
   void removeProtoSquadID(long protoSquadID) { mData.removeValue(protoSquadID); syncTriggerVar(protoSquadID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BProtoSquadID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BProtoSquadID, mData, uint16, 2000); gSaveGame.remapProtoSquadIDs(mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif
protected:
   BProtoSquadIDArray mData;
};


//==============================================================================
// BTriggerVarTechList
//==============================================================================
class BTriggerVarTechList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTechList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTechList); }
   const BTechIDArray& readVar() const { checkInitialized(); return (mData); }
   BTechIDArray& readVar() { checkInitialized(); return (mData); }
   void writeVar(const BTechIDArray& v) { mData = v; initialize(); }
   void addTechID(long techID) { mData.uniqueAdd(techID); syncTriggerVar(techID); initialize(); }
   void removeTechID(long techID) { mData.removeValue(techID); syncTriggerVar(techID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BProtoTechID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BProtoTechID, mData, uint16, 2000); gSaveGame.remapProtoTechIDs(mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BTechIDArray mData;
};

//==============================================================================
// BTriggerVarObjectDataType
//==============================================================================
class BTriggerVarObjectDataType : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarObjectDataType, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeObjectDataType); }
      const long readVar() const { checkInitialized(); return (mData); }
      void writeVar(const long v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }

   protected:
      long mData;
};

//==============================================================================
// BTriggerVarObjectDataRelative
//==============================================================================
class BTriggerVarObjectDataRelative : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarObjectDataRelative, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeObjectDataRelative); }
      const long readVar() const { checkInitialized(); return (mData); }
      void writeVar(const long v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }

   protected:
      long mData;
};


//==============================================================================
// BTriggerVarCiv
//==============================================================================
class BTriggerVarCiv : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarCiv, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeCiv); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v){ mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapCivID(mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   long mData;
};


//==============================================================================
// BTriggerVarProtoObjectCollection
//==============================================================================
class BTriggerVarProtoObjectCollection : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarProtoObjectCollection, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeProtoObjectCollection); }
   const BProtoObjectIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BProtoObjectIDArray& v) { mData = v; initialize(); }
   void addProtoObjectID(long protoObjectID) { mData.add(protoObjectID); syncTriggerVar(protoObjectID); initialize(); }
   void removeProtoObjectID(long protoObjectID) { mData.removeValue(protoObjectID); syncTriggerVar(protoObjectID); }
   void clear() { mData.clear(); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BProtoObjectID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BProtoObjectID, mData, uint16, 2000); gSaveGame.remapProtoObjectIDs(mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BProtoObjectIDArray mData;
};


//==============================================================================
// BTriggerVarObject
//==============================================================================
class BTriggerVarObject : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarObject, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeObject); }
   BEntityID readVar() const { checkInitialized(); return (mData); }
   void writeVar(BEntityID v) { mData = v; initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BEntityID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BEntityID, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   BEntityID mData;
};


//==============================================================================
// BTriggerVarObjectList
//==============================================================================
class BTriggerVarObjectList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarObjectList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeObjectList); }
   const BEntityIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BEntityIDArray& v) { mData = v; initialize(); }
   void addObject(BEntityID objectID);
   void removeObject(BEntityID objectID) { mData.removeValue(objectID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BEntityID, mData, uint16, 2000); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BEntityIDArray mData;
};


//==============================================================================
// BTriggerVarGroup
//==============================================================================
class BTriggerVarGroup : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarGroup, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeGroup); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   long mData;
};


//==============================================================================
// class BTriggerVarRefCountType
//==============================================================================
class BTriggerVarRefCountType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarRefCountType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeRefCountType); }
   short readVar() const { checkInitialized(); return (mData); }
   void writeVar(short v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, short, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, short, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   short mData;
};


//==============================================================================
// class BTriggerVarUnitFlag
//==============================================================================
class BTriggerVarUnitFlag : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUnitFlag, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUnitFlag); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   long mData;
};

//==============================================================================
// class BTriggerVarSquadFlag
//==============================================================================
class BTriggerVarSquadFlag : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarSquadFlag, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeSquadFlag); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   long mData;
};


//==============================================================================
// class BTriggerVarLOSType
//==============================================================================
class BTriggerVarLOSType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarLOSType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeLOSType); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   long mData;
};


//==============================================================================
// class BTriggerVarEntityFilterSet
//==============================================================================
class BTriggerVarEntityFilterSet : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ clearFilters(); mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){ clearFilters(); }
   DECLARE_FREELIST(BTriggerVarEntityFilterSet, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeEntityFilterSet); }
   BEntityFilterSet* readVar() { checkInitialized(); return (&mData); }
   void writeVar(BEntityFilterSet* v){ mData.copyFilterSet(v); initialize(); }
   void clearFilters() { mData.clearFilters(); initialize(); }
   void addEntityFilterIsAlive(bool invertFilter) { mData.addEntityFilterIsAlive(invertFilter); initialize(); }
   void addEntityFilterIsIdle(bool invertFilter){ mData.addEntityFilterIsIdle(invertFilter); initialize(); }
   void addEntityFilterInList(bool invertFilter, const BEntityIDArray &entityList) { mData.addEntityFilterInList(invertFilter, entityList); initialize(); }
   void addEntityFilterPlayers(bool invertFilter, const BPlayerIDArray &players) { mData.addEntityFilterPlayers(invertFilter, players); initialize(); }
   void addEntityFilterTeams(bool invertFilter, const BTeamIDArray &teams) { mData.addEntityFilterTeams(invertFilter, teams); initialize(); }
   void addEntityFilterProtoObjects(bool invertFilter, const BProtoObjectIDArray &protoObjects) { mData.addEntityFilterProtoObjects(invertFilter, protoObjects); initialize(); }
   void addEntityFilterProtoSquads(bool invertFilter, const BProtoSquadIDArray &protoSquads) { mData.addEntityFilterProtoSquads(invertFilter, protoSquads); initialize(); }
   void addEntityFilterObjectTypes(bool invertFilter, const BObjectTypeIDArray &objectTypes) { mData.addEntityFilterObjectTypes(invertFilter, objectTypes); initialize(); }
   void addEntityFilterRefCount(bool invertFilter, long refCountType, long compareType, long count) { mData.addEntityFilterRefCount(invertFilter, refCountType, compareType, count); initialize(); }
   void addEntityFilterRelationType(bool invertFilter, BRelationType relationType, BTeamID teamID){ mData.addEntityFilterRelationType(invertFilter, relationType, teamID); initialize(); }
   void addEntityFilterMaxObjectType(bool invertFilter, BObjectTypeID objectTypeID, uint maxCount, bool applyToSquads = true, bool applyToUnits = false) { mData.addEntityFilterMaxObjectType(invertFilter, objectTypeID, maxCount, applyToSquads, applyToUnits); initialize(); }
   void addEntityFilterIsSelected(bool invertFilter, BPlayerID player) { mData.addEntityFilterIsSelected(invertFilter, player); initialize(); }
   void addEntityFilterCanChangeOwner(bool invertFilter) { mData.addEntityFilterCanChangeOwner(invertFilter); initialize(); }
   void addEntityFilterJacking(bool invertFilter) { mData.addEntityFilterJacking(invertFilter); initialize(); }

   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   BEntityFilterSet mData;
};

//==============================================================================
// class BTriggerVarPopBucket
//==============================================================================
class BTriggerVarPopBucket : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarPopBucket, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypePopBucket); }
      long readVar() const { checkInitialized(); return (mData); }
      void writeVar(long v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapPopID(mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString& varDataAsString);
   #endif
   protected:
      long mData;
};

//==============================================================================
// class BTriggerVarListPosition
//==============================================================================
class BTriggerVarListPosition : public BTriggerVar, IPoolable
{
   public:
      enum
      {
         cListPosTypeFirst = 0,
         cListPosTypeLast,
         cListPosTypeRandom
      };
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarListPosition, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeListPosition); }
      long readVar() const { checkInitialized(); return (mData); }
      void writeVar(long v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString& varDataAsString);
   #endif
   protected:
      long mData;
};

//==============================================================================
//==============================================================================
class BTriggerVarRelationType : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarRelationType, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeRelationType); }
      BRelationType readVar() const { checkInitialized(); return (mData); }
      void writeVar(BRelationType v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BRelationType, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BRelationType, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString& varDataAsString);
   #endif
   protected:
      BRelationType mData;
};

//==============================================================================
// class BTriggerVarExposedAction
//==============================================================================
class BTriggerVarExposedAction : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(void){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(void){}
      DECLARE_FREELIST(BTriggerVarExposedAction, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeExposedAction); }
      long readVar(void) const { checkInitialized(); return (mData); }
      void writeVar(long v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
      #if !defined(BUILD_FINAL)
         virtual void getVariableDataAsString(BSimString& varDataAsString);
      #endif
   protected:
      long mData;
};

//==============================================================================
// class BTriggerVarSquadMode
//==============================================================================
class BTriggerVarSquadMode : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarSquadMode, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeSquadMode); }
      int readVar() const { checkInitialized(); return (mData); }
      void writeVar(int v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
      #if !defined(BUILD_FINAL)
         virtual void getVariableDataAsString(BSimString& varDataAsString);
      #endif
   protected:
      int mData;
};

//==============================================================================
// class BTriggerVarExposedScript
//==============================================================================
class BTriggerVarExposedScript : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(void){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(void){}
      DECLARE_FREELIST(BTriggerVarExposedScript, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeExposedScript); }
      uint readVar() const { checkInitialized(); return (mData); }
      void writeVar(uint v){ mData = v; syncTriggerVar((int)v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, uint, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, uint, mData); return BTriggerVar::load(pStream, saveType); }
      #if !defined(BUILD_FINAL)
         virtual void getVariableDataAsString(BSimString& varDataAsString);
      #endif
   protected:
      uint mData;
};


//==============================================================================
// class BTriggerVarKBBase
//==============================================================================
class BTriggerVarKBBase : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(void){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(void){}
   DECLARE_FREELIST(BTriggerVarKBBase, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeKBBase); }
   BKBBaseID readVar(void) const { checkInitialized(); return (mData); }
   void writeVar(BKBBaseID v){ mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BKBBaseID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BKBBaseID, mData); return BTriggerVar::load(pStream, saveType); }
   //#if !defined(BUILD_FINAL)
   //virtual void getVariableDataAsString(BSimString& varDataAsString);
   //#endif
protected:
   BKBBaseID mData;
};


//==============================================================================
// BTriggerVarKBBaseList
//==============================================================================
class BTriggerVarKBBaseList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarKBBaseList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeKBBaseList); }
   const BKBBaseIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BKBBaseIDArray& v) { mData = v; initialize(); }
   void addKBBase(BKBBaseID kbBaseID) { mData.uniqueAdd(kbBaseID); syncTriggerVar(kbBaseID); initialize(); }
   void removeKBBase(BKBBaseID kbBaseID) { mData.removeValue(kbBaseID); syncTriggerVar(kbBaseID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BKBBaseID, mData, uint16, 4000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BKBBaseID, mData, uint16, 4000); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BKBBaseIDArray mData;
};


//==============================================================================
// class BTriggerVarDataScalar
//==============================================================================
class BTriggerVarDataScalar : public BTriggerVar, IPoolable
{
   public:
      enum
      {
         cDataScalarAccuracy = 0,
         cDataScalarWorkRate,
         cDataScalarDamage,
         cDataScalarLOS,
         cDataScalarVelocity,
         cDataScalarWeaponRange,
         cDataScalarDamageTaken,
      };
      virtual void onAcquire(void){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(void){}
      DECLARE_FREELIST(BTriggerVarDataScalar, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeDataScalar); }
      uint readVar() const { checkInitialized(); return (mData); }
      void writeVar(uint v){ mData = v; syncTriggerVar((int)v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, uint, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, uint, mData); return BTriggerVar::load(pStream, saveType); }
      #if !defined(BUILD_FINAL)
         virtual void getVariableDataAsString(BSimString& varDataAsString);
      #endif
   protected:
      uint mData;
};


//==============================================================================
// class BTriggerVarKBBaseQuery
//==============================================================================
class BTriggerVarKBBaseQuery : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarKBBaseQuery, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeKBBaseQuery); }
   const BKBBaseQuery& readVar() { checkInitialized(); return (mData); }
   void resetQuery() { mData.resetQuery(); initialize(); }
   void setPointRadius(BVector pos, float rad) { mData.setPointRadius(pos, rad); syncTriggerVar(pos); syncTriggerVar(rad); }
   void setPlayerRelation(BPlayerID playerID, BRelationType relation) { mData.setPlayerRelation(playerID, relation); syncTriggerVar(playerID); syncTriggerVar(relation); }
   void setMinStaleness(DWORD minStaleness) { mData.setMinStaleness(minStaleness); syncTriggerVar(minStaleness); }
   void setMaxStaleness(DWORD maxStaleness) { mData.setMaxStaleness(maxStaleness); syncTriggerVar(maxStaleness); }
   void setSelfAsAlly(bool selfAsAlly) { mData.setSelfAsAlly(selfAsAlly); syncTriggerVar(selfAsAlly); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   BKBBaseQuery mData;
};

//==============================================================================
// BTriggerVarDesignLine
//==============================================================================
class BTriggerVarDesignLine : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarDesignLine, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeDesignLine); }
      BDesignLineID readVar() const { checkInitialized(); return (mData); }
      void writeVar(BDesignLineID v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BDesignLineID, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BDesignLineID, mData); return BTriggerVar::load(pStream, saveType); }
      #ifndef BUILD_FINAL
         virtual void getVariableDataAsString(BSimString& varDataAsString) { varDataAsString.format("%d", mData); }
      #endif
   protected:
      BDesignLineID mData;
};


//==============================================================================
// BTriggerVarLocStringID
//==============================================================================
class BTriggerVarLocStringID : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarLocStringID, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeLocStringID); }
   uint32 readVar() const { checkInitialized(); return (mData); }
   void writeVar(uint32 v) { mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, uint32, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, uint32, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
#endif
protected:
   uint32 mData;
};


//==============================================================================
//==============================================================================
class BTriggerVarLeader : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarLeader, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeLeader); }
   BLeaderID readVar() const { checkInitialized(); return (mData); }
   void writeVar(BLeaderID v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BLeaderID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BLeaderID, mData); gSaveGame.remapLeaderID(mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString& varDataAsString);
#endif
protected:
   BInt8<BLeaderID, -1, 64> mData;
};

//==============================================================================
// BTriggerVarCinematic
//==============================================================================
class BTriggerVarCinematic : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarCinematic, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeCinematic); }
      uint readVar() const { checkInitialized(); return (mData); }
      void writeVar(uint v) { mData = v; syncTriggerVar((int)v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, uint, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, uint, mData); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      uint mData;
};

//==============================================================================
// BTriggerVarTalkingHead
//==============================================================================
class BTriggerVarTalkingHead : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarTalkingHead, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTalkingHead); }
   uint readVar() const { checkInitialized(); return (mData); }
   void writeVar(uint v) { mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, uint, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, uint, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   uint mData;
};


//==============================================================================
// BTriggerVarFlareType
//==============================================================================
class BTriggerVarFlareType : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarFlareType, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeFlareType); }
      int readVar() const { checkInitialized(); return (mData); }
      void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
      #ifndef BUILD_FINAL
         virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
      #endif
   protected:
      int mData;
};

//==============================================================================
// BTriggerVarCinematicTag
//==============================================================================
class BTriggerVarCinematicTag : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarCinematicTag, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeCinematicTag); }
      uint readVar() const { checkInitialized(); return (mData); }
      void writeVar(uint v) { mData = v; syncTriggerVar((int)v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, uint, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, uint, mData); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      uint mData;
};

//==============================================================================
// BTriggerVarIconType
//==============================================================================
class BTriggerVarIconType : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarIconType, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeIconType); }
      int readVar() const { checkInitialized(); return (mData); }
      void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); gSaveGame.remapProtoObjectID(mData); return BTriggerVar::load(pStream, saveType); }
      #ifndef BUILD_FINAL
         virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
      #endif
   protected:
      int mData;
};


//==============================================================================
// BTriggerVarDifficulty
//==============================================================================
class BTriggerVarDifficulty : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarDifficulty, 4);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeDifficulty); }
      BDifficultyType readVar() const { checkInitialized(); return (mData); }
      void writeVar(BDifficultyType v){ mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BDifficultyType, mData); return (BTriggerVar::save(pStream, saveType)); }
      virtual bool load(BStream* pStream, int saveType) 
      { 
         if (mGameFileVersion >= 3)
            GFREADVAR(pStream, BDifficultyType, mData); 
         return (BTriggerVar::load(pStream, saveType)); 
      }
      #ifndef BUILD_FINAL
         //virtual void getVariableDataAsString(BSimString &varDataAsString);
      #endif
   protected:
      BDifficultyType mData;
};


//==============================================================================
// BTriggerVarInteger
//==============================================================================
class BTriggerVarInteger : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarInteger, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeInteger); }
   int32 readVar() const { checkInitialized(); return (mData); }
   void writeVar(int32 v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int32, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int32, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
   #endif
protected:
   int32 mData;
};

//==============================================================================
// class BTriggerVarHUDItem
//==============================================================================
class BTriggerVarHUDItem : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarHUDItem, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeHUDItem); }
   int readVar() const { checkInitialized(); return (mData); }
   void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   int mData;
};

//==============================================================================
// class BTriggerVarFlashableUIItem
//==============================================================================
class BTriggerVarFlashableUIItem : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarFlashableUIItem, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeFlashableUIItem); }
   int readVar() const { checkInitialized(); return (mData); }
   void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
/*
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
*/
protected:
   int mData;
};

//==============================================================================
// class BTriggerVarControlType
//==============================================================================
class BTriggerVarControlType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarControlType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeControlType); }
   int readVar() const { checkInitialized(); return (mData); }
   void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   int mData;
};

//==============================================================================
// BTriggerVarUIButton
//==============================================================================
class BTriggerVarUIButton : public BTriggerVar, IPoolable
{
public:

   enum
   {
      cUIButtonResultWaiting,
      cUIButtonResultPressed,
   };
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cUIButtonResultWaiting; mX=0.0f; mY=0.0f; mAnalog=0.0f; mActionModifier=false; mSpeedModifier=false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUIButton, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUIButton); }

   long readResult() const { checkInitialized(); return (mResult); }
   float readX() const { checkInitialized(); return (mX); }
   float readY() const { checkInitialized(); return (mY); }
   float readAnalog() const { checkInitialized(); return (mAnalog); }
   bool readActionModifier() const { checkInitialized(); return (mActionModifier); }
   bool readSpeedModifier() const { checkInitialized(); return (mSpeedModifier); }

   void writeResult(long v, float x, float y, float analog, bool actionModifier, bool speedModifier) { syncTriggerVar(v); syncTriggerVar(x); syncTriggerVar(y); syncTriggerVar(analog); syncTriggerVar(actionModifier); syncTriggerVar(speedModifier); mResult = v; mX=x; mY=y; mAnalog=analog; mActionModifier=actionModifier; mSpeedModifier=speedModifier; initialize(); }

   virtual bool save(BStream* pStream, int saveType) 
   { 
      GFWRITEVAR(pStream, long, mResult);
      GFWRITEVAR(pStream, float, mX);
      GFWRITEVAR(pStream, float, mY);
      GFWRITEVAR(pStream, float, mAnalog);
      GFWRITEVAR(pStream, bool, mActionModifier);
      GFWRITEVAR(pStream, bool, mSpeedModifier);
      return BTriggerVar::save(pStream, saveType); 
   }
   virtual bool load(BStream* pStream, int saveType) 
   { 
      GFREADVAR(pStream, long, mResult);
      GFREADVAR(pStream, float, mX);
      GFREADVAR(pStream, float, mY);
      GFREADVAR(pStream, float, mAnalog);
      GFREADVAR(pStream, bool, mActionModifier);
      GFREADVAR(pStream, bool, mSpeedModifier);
      return BTriggerVar::load(pStream, saveType); 
   }

   #ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
   #endif

protected:
   long mResult;
   float mX;
   float mY;
   float mAnalog;
   bool mActionModifier;
   bool mSpeedModifier;
};

//==============================================================================
// BTriggerVarMissionType
//==============================================================================
class BTriggerVarMissionType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarMissionType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeMissionType); }
   BAIMissionType readVar() const { checkInitialized(); return (mData); }
   void writeVar(BAIMissionType v) { mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BAIMissionType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BAIMissionType, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BUInt8<BAIMissionType, MissionType::cMin, MissionType::cMax> mData;
};


//==============================================================================
// BTriggerVarMissionState
//==============================================================================
class BTriggerVarMissionState : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarMissionState, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeMissionState); }
   BAIMissionState readVar() const { checkInitialized(); return (mData); }
   void writeVar(BAIMissionState v) { mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BAIMissionState, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BAIMissionState, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BUInt8<BAIMissionState, MissionState::cMin, MissionState::cMax> mData;
};



//==============================================================================
// BTriggerVarMissionTargetType
//==============================================================================
class BTriggerVarMissionTargetType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarMissionTargetType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeMissionTargetType); }
   BAIMissionTargetType readVar() const { checkInitialized(); return (mData); }
   void writeVar(BAIMissionTargetType v) { mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BAIMissionTargetType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BAIMissionTargetType, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BUInt8<BAIMissionTargetType, MissionTargetType::cMin, MissionTargetType::cMax> mData;
};


//==============================================================================
// BTriggerVarIntegerList
//==============================================================================
class BTriggerVarIntegerList : public BTriggerVar, IPoolable
{

public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarIntegerList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeIntegerList); }
   const BInt32Array& readVar() const { checkInitialized(); return (mData); }
   BInt32Array& readVar() { checkInitialized(); return (mData); }
   void writeVar(const BInt32Array& v) { mData = v; initialize(); }
   void addInt(int32 intVal) { mData.add(intVal); syncTriggerVar(intVal); initialize(); }
   void removeInt(int32 intVal) { mData.removeValue(intVal); syncTriggerVar(intVal); initialize(); }
   void removeIntAll(int32 intVal) { mData.removeValueAllInstances(intVal); syncTriggerVar(intVal); initialize(); }
   void clear() { mData.clear(); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, int32, mData, uint16, 10000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, int32, mData, uint16, 10000); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BInt32Array mData;
};


//==============================================================================
// BTriggerVarBidType
//==============================================================================
class BTriggerVarBidType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarBidType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeBidType); }
   BBidType readVar() const { checkInitialized(); return (mData); }
   void writeVar(BBidType v) { mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BBidType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BBidType, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BUInt8<BBidType, BidType::cMin, BidType::cMax> mData;
};


//==============================================================================
// BTriggerVarBidState
//==============================================================================
class BTriggerVarBidState : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarBidState, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeBidState); }
   BBidState readVar() const { checkInitialized(); return (mData); }
   void writeVar(BBidState v) { mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BBidState, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BBidState, mData); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BUInt8<BBidState, BidState::cMin, BidState::cMax> mData;
};

//==============================================================================
// BTriggerVarBuildingCommandState
//==============================================================================
class BTriggerVarBuildingCommandState : public BTriggerVar, IPoolable
{
public:
   enum
   {
      cResultWaiting,
      cResultDone,
   };

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cResultWaiting; }
   virtual void onRelease() { mSquads.clear(); }
   DECLARE_FREELIST(BTriggerVarBuildingCommandState, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeBuildingCommandState); }

   long readResult() const { checkInitialized(); return (mResult); }
   const BEntityIDArray& readSquads() const  { checkInitialized(); return mSquads; }

   void writeResult(long v) { mResult = v; syncTriggerVar(v); initialize(); }
   void writeTrainedSquadID(BEntityID v) { mSquads.add(v); syncTriggerVar(v); }

   void clearTrainedSquadIDs() { mSquads.clear(); }


   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mResult); GFWRITEARRAY(pStream, BEntityID, mSquads, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mResult); GFREADARRAY(pStream, BEntityID, mSquads, uint16, 2000); return BTriggerVar::load(pStream, saveType); }

#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif

protected:
   long mResult;
   BEntityIDArray mSquads;
};


//==============================================================================
//==============================================================================
class BTriggerVarVector : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarVector, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeVector); }
   BVector readVar() const { checkInitialized(); return (mData); }
   void writeVar(BVector v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVECTOR(pStream, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVECTOR(pStream, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("(%f,%f,%f)", mData.x, mData.y, mData.z); }
#endif
protected:
   BVector mData;
};


//==============================================================================
//==============================================================================
class BTriggerVarVectorList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarVectorList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeVectorList); }
   const BVectorArray& readVar() const { checkInitialized(); return (mData); }
   BVectorArray& readVar() { checkInitialized(); return (mData); }
   void writeVar(const BVectorArray& v) { mData = v; initialize(); }
   void addVector(BVector v) { mData.add(v); syncTriggerVar(v); initialize(); }
   void removeVector(BVector v) { mData.removeValue(v); syncTriggerVar(v); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BVector, mData, uint16, 10000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BVector, mData, uint16, 10000); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BVectorArray mData;
};

//==============================================================================
// class BTriggerVarPlacementRule
//==============================================================================
class BTriggerVarPlacementRule : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarPlacementRule, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypePlacementRule); }
      int readVar() const { checkInitialized(); return (mData); }
      void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); gSaveGame.remapPlacementRuleID(mData); return BTriggerVar::load(pStream, saveType); }
      #ifndef BUILD_FINAL
         virtual void getVariableDataAsString(BSimString& varDataAsString) { varDataAsString.format("%d", mData); }
      #endif
   protected:
      long mData;
};

//==============================================================================
// class BTriggerVarKBSquad
//==============================================================================
class BTriggerVarKBSquad : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(void){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(void){}
   DECLARE_FREELIST(BTriggerVarKBSquad, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeKBSquad); }
   BKBSquadID readVar(void) const { checkInitialized(); return (mData); }
   void writeVar(BKBSquadID v){ mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BKBSquadID, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BKBSquadID, mData); return BTriggerVar::load(pStream, saveType); }
//#if !defined(BUILD_FINAL)
   //virtual void getVariableDataAsString(BSimString& varDataAsString);
//#endif
protected:
   BKBSquadID mData;
};

//==============================================================================
// BTriggerVarKBSquadList
//==============================================================================
class BTriggerVarKBSquadList : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarKBSquadList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeKBSquadList); }
   const BKBSquadIDArray& readVar() const { checkInitialized(); return (mData); }
   void writeVar(const BKBSquadIDArray& v) { mData = v; initialize(); }
   void addKBSquad(BKBSquadID KBSquadID) { mData.uniqueAdd(KBSquadID); syncTriggerVar(KBSquadID); initialize(); }
   void removeKBSquad(BKBSquadID KBSquadID) { mData.removeValue(KBSquadID); syncTriggerVar(KBSquadID); }
   void clear() { mData.clear(); initialize(); }
   void shuffle();
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BKBSquadID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BKBSquadID, mData, uint16, 2000); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //   virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BKBSquadIDArray mData;
};

//==============================================================================
// class BTriggerVarKBSquadQuery
//==============================================================================
class BTriggerVarKBSquadQuery : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarKBSquadQuery, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeKBSquadQuery); }
   const BKBSquadQuery& readVar() { checkInitialized(); return (mData); }
   void resetQuery() { mData.resetQuery(); initialize(); }
   void setPointRadius(BVector pos, float rad) { mData.setPointRadius(pos, rad); syncTriggerVar(pos); syncTriggerVar(rad); }
   void setPlayerRelation(BPlayerID playerID, BRelationType relation) { mData.setPlayerRelation(playerID, relation); syncTriggerVar(playerID); syncTriggerVar(relation); }
   void setObjectType(BObjectTypeID objectTypeID) { mData.setObjectType(objectTypeID); syncTriggerVar(objectTypeID); }
   void setBase(BKBBaseID baseID) { mData.setBase(baseID); syncTriggerVar(baseID); }
   void setMinStaleness(DWORD minStaleness) { mData.setMinStaleness(minStaleness); syncTriggerVar(minStaleness); }
   void setMaxStaleness(DWORD maxStaleness) { mData.setMaxStaleness(maxStaleness); syncTriggerVar(maxStaleness); }
   void setCurrentlyVisible(bool currentlyVisible) { mData.setCurrentlyVisible(currentlyVisible); syncTriggerVar(currentlyVisible); }
   void setSelfAsAlly(bool selfAsAlly) { mData.setSelfAsAlly(selfAsAlly); syncTriggerVar(selfAsAlly); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   BKBSquadQuery mData;
};

//==============================================================================
// class BTriggerVarAISquadAnalysis
//==============================================================================
class BTriggerVarAISquadAnalysis : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(void){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(void){}
   DECLARE_FREELIST(BTriggerVarAISquadAnalysis, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeAISquadAnalysis); }
   const BAISquadAnalysis& readVar(void) const { checkInitialized(); return (mData); }
   void writeVar(const BAISquadAnalysis& v){ mData = v; initialize(); }
   void reset() { mData.reset(); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mData); return BTriggerVar::load(pStream, saveType); }
   //#if !defined(BUILD_FINAL)
   //virtual void getVariableDataAsString(BSimString& varDataAsString);
   //#endif
protected:
   BAISquadAnalysis mData;
};


//==============================================================================
// class BTriggerVarAISquadAnalysisComponent
//==============================================================================
class BTriggerVarAISquadAnalysisComponent : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(void){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(void){}
   DECLARE_FREELIST(BTriggerVarAISquadAnalysisComponent, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeAISquadAnalysisComponent); }
   BAISquadAnalysisComponent readVar(void) const { checkInitialized(); return (mData); }
   void writeVar(BAISquadAnalysisComponent v){ mData = v; syncTriggerVar((int)v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, BAISquadAnalysisComponent, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, BAISquadAnalysisComponent, mData); return BTriggerVar::load(pStream, saveType); }
   //#if !defined(BUILD_FINAL)
   //virtual void getVariableDataAsString(BSimString& varDataAsString);
   //#endif
protected:
   BAISquadAnalysisComponent mData;
};


//==============================================================================
//==============================================================================
class BTriggerVarKBSquadFilterSet : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ clearFilters(); mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){ clearFilters();}
   DECLARE_FREELIST(BTriggerVarKBSquadFilterSet, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeKBSquadFilterSet); }
   BKBSquadFilterSet* readVar() { checkInitialized(); return (&mData); }
   void clearFilters() { mData.clearFilters(); initialize(); }
   void addKBSquadFilterCurrentlyVisible(bool invertFilter) { mData.addKBSquadFilterCurrentlyVisible(invertFilter); initialize(); }
   void addKBSquadFilterObjectTypes(bool invertFilter, const BObjectTypeIDArray &objectTypes) { mData.addKBSquadFilterObjectTypes(invertFilter, objectTypes); initialize(); }
   void addKBSquadFilterPlayers(bool invertFilter, const BPlayerIDArray &players) { mData.addKBSquadFilterPlayers(invertFilter, players); initialize(); }
   void addKBSquadFilterInList(bool invertFilter, const BKBSquadIDArray &KBSquadList) { mData.addKBSquadFilterInList(invertFilter, KBSquadList); initialize(); }
   void addKBSquadFilterPlayerRelation(bool invertFilter, BPlayerID playerID, BRelationType relationType, bool selfAsAlly) { mData.addKBSquadFilterPlayerRelation(invertFilter, playerID, relationType, selfAsAlly); initialize(); }
   void addKBSquadFilterMinStaleness(bool invertFilter, DWORD minStaleness) { mData.addKBSquadFilterMinStaleness(invertFilter, minStaleness); initialize(); }
   void addKBSquadFilterMaxStaleness(bool invertFilter, DWORD maxStaleness) { mData.addKBSquadFilterMaxStaleness(invertFilter, maxStaleness); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITECLASS(pStream, saveType, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADCLASS(pStream, saveType, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   BKBSquadFilterSet mData;
};

//==============================================================================
// BTriggerVarChatSpeaker
//==============================================================================
class BTriggerVarChatSpeaker : public BTriggerVar, IPoolable
{
public:

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarChatSpeaker, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeChatSpeaker); }
   long readVar() const { checkInitialized(); return (mData); }
   void writeVar(long v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   long mData;
};

//==============================================================================
// BTriggerVarRumbleType
//==============================================================================
class BTriggerVarRumbleType : public BTriggerVar, IPoolable
{
public:

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarRumbleType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeRumbleType); }
   int readVar() const { checkInitialized(); return (mData); }
   void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   int mData;
};

//==============================================================================
// BTriggerVarRumbleMotor
//==============================================================================
class BTriggerVarRumbleMotor : public BTriggerVar, IPoolable
{
public:

   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarRumbleMotor, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeRumbleMotor); }
   int readVar() const { checkInitialized(); return (mData); }
   void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif
protected:
   int mData;
};

//==============================================================================
// BTriggerVarTechDataCommandType
//==============================================================================
class BTriggerVarTechDataCommandType : public BTriggerVar, IPoolable
{
   public:

      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarTechDataCommandType, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTechDataCommandType); }
      int readVar() const { checkInitialized(); return (mData); }
      void writeVar(int v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int, mData); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString& varDataAsString);
      //#endif
   protected:
      int mData;
};

//==============================================================================
// BTriggerVarSquadDataType
//==============================================================================
class BTriggerVarSquadDataType : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarSquadDataType, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeSquadDataType); }
      const long readVar() const { checkInitialized(); return (mData); }
      void writeVar(const long v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }

   protected:
      long mData;
};

//==============================================================================
// BTriggerVarEventType
//==============================================================================
class BTriggerVarEventType : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarEventType, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeEventType); }
      const long readVar() const { checkInitialized(); return (mData); }
      void writeVar(const long v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }

#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif

   protected:
      long mData;
};

//==============================================================================
// BTriggerVarTimeList
//==============================================================================
class BTriggerVarTimeList : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarTimeList, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeTimeList); }
      const BDWORDArray& readVar() const { checkInitialized(); return (mData); }
      BDWORDArray& readVar() { checkInitialized(); return (mData); }
      void writeVar(const BDWORDArray& v) { mData = v; initialize(); }
      void addTime(DWORD timeVal) { mData.add(timeVal); syncTriggerVar(timeVal); initialize(); }
      void removeTime(DWORD timeVal) { mData.removeValue(timeVal); syncTriggerVar(timeVal); initialize(); }
      void removeTimeAll(DWORD timeVal) { mData.removeValueAllInstances(timeVal); syncTriggerVar(timeVal); initialize(); }
      void clear() { mData.clear(); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, DWORD, mData, uint16, 1000); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, DWORD, mData, uint16, 1000); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      BDWORDArray mData;
};

//==============================================================================
// List of design lines
//==============================================================================
class BTriggerVarDesignLineList : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarDesignLineList, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeDesignLineList); }
      const BDesignLineIDArray& readVar() const { checkInitialized(); return (mData); }
      BDesignLineIDArray& readVar() { checkInitialized(); return (mData); }
      void writeVar(const BDesignLineIDArray& v) { mData = v; initialize(); }
      void addLine(BDesignLineID v) { mData.add(v); syncTriggerVar(v); initialize(); }
      void removeLine(BDesignLineID v) { mData.removeValue(v); syncTriggerVar(v); }
      void removeLineAll(BDesignLineID lineVal) { mData.removeValueAllInstances(lineVal); syncTriggerVar(lineVal); initialize(); }
      void clear() { mData.clear(); initialize(); }
      void shuffle();
      virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, BDesignLineID, mData, uint16, 2000); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, BDesignLineID, mData, uint16, 2000); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //   virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      BDesignLineIDArray mData;
};

//==============================================================================
// BTriggerVarGameStatePredicate
//==============================================================================
class BTriggerVarGameStatePredicate : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarGameStatePredicate, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeGameStatePredicate); }
      const long readVar() const { checkInitialized(); return (mData); }
      void writeVar(const long v) { mData = v; syncTriggerVar(v); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mData); return BTriggerVar::load(pStream, saveType); }

#ifndef BUILD_FINAL
   virtual void getVariableDataAsString(BSimString &varDataAsString);
#endif

   protected:
      long mData;
};

//==============================================================================
// BTriggerVarFloatList
//==============================================================================
class BTriggerVarFloatList : public BTriggerVar, IPoolable
{
   public:
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarFloatList, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeFloatList); }
      const BFloatArray& readVar() const { checkInitialized(); return (mData); }
      BFloatArray& readVar() { checkInitialized(); return (mData); }
      void writeVar(const BFloatArray& v) { mData = v; initialize(); }
      void addFloat(float intVal) { mData.add(intVal); syncTriggerVar(intVal); initialize(); }
      void removeFloat(float intVal) { mData.removeValue(intVal); syncTriggerVar(intVal); initialize(); }
      void removeFloatAll(float intVal) { mData.removeValueAllInstances(intVal); syncTriggerVar(intVal); initialize(); }
      void clear() { mData.clear(); initialize(); }
      virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, float, mData, uint16, 10000); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, float, mData, uint16, 10000); return BTriggerVar::load(pStream, saveType); }
      //#ifndef BUILD_FINAL
      //virtual void getVariableDataAsString(BSimString &varDataAsString);
      //#endif
   protected:
      BFloatArray mData;       
};

//==============================================================================
// BTriggerVarUILocationMinigame
//==============================================================================
class BTriggerVarUILocationMinigame : public BTriggerVar, IPoolable
{
   public:

      enum
      {
         cUILocationResultWaiting,
         cUILocationResultOK,
         cUILocationResultCancel,
         cUILocationResultUILockError,
      };
      virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; mResult = cUILocationResultWaiting; }
      virtual void onRelease(){}
      DECLARE_FREELIST(BTriggerVarUILocationMinigame, 5);
      virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUILocationMinigame); }

      long readResult() const { checkInitialized(); return (mResult); }
      BVector readLocation() const { checkInitialized(); return (mLocation); }
      float readData() const { checkInitialized(); return (mData); }

      void writeResult(long v) { mResult = v; syncTriggerVar(v); initialize(); }
      void writeLocation(BVector v) { mLocation = v; syncTriggerVar(v); initialize(); }
      void writeData(float v) { mData = v; syncTriggerVar(v); initialize(); }


      virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, long, mResult); GFWRITEVECTOR(pStream, mLocation); GFWRITEVAR(pStream, float, mData); return BTriggerVar::save(pStream, saveType); }
      virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, long, mResult); GFREADVECTOR(pStream, mLocation); GFREADVAR(pStream, float, mData); return BTriggerVar::load(pStream, saveType); }

      #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString);
      #endif
   protected:
      long mResult;
      BVector mLocation;
      float mData;
};


//==============================================================================
// BTriggerVarConcept
//==============================================================================
class BTriggerVarConcept : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarConcept, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeConcept); }
   int32 readVar() const { checkInitialized(); return (mData); }
   void writeVar(int32 v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int32, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int32, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
   #endif
protected:
   int32 mData;
};


//==============================================================================
// BTriggerVarConceptList
//==============================================================================
class BTriggerVarConceptList : public BTriggerVar, IPoolable
{

public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarConceptList, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeConceptList); }
   const BInt32Array& readVar() const { checkInitialized(); return (mData); }
   BInt32Array& readVar() { checkInitialized(); return (mData); }
   void writeVar(const BInt32Array& v) { mData = v; initialize(); }
   void addInt(int32 intVal) { mData.add(intVal); syncTriggerVar(intVal); initialize(); }
   void removeInt(int32 intVal) { mData.removeValue(intVal); syncTriggerVar(intVal); initialize(); }
   void removeIntAll(int32 intVal) { mData.removeValueAllInstances(intVal); syncTriggerVar(intVal); initialize(); }
   void clear() { mData.clear(); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEARRAY(pStream, int32, mData, uint16, 10000); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADARRAY(pStream, int32, mData, uint16, 10000); return BTriggerVar::load(pStream, saveType); }
   //#ifndef BUILD_FINAL
   //virtual void getVariableDataAsString(BSimString &varDataAsString);
   //#endif
protected:
   BInt32Array mData;
};


//==============================================================================
// BTriggerVarUserClassType
//==============================================================================
class BTriggerVarUserClassType : public BTriggerVar, IPoolable
{
public:
   virtual void onAcquire(){ mID = BTriggerVar::cVarIDInvalid; mbNull = false; mbInitialized = false; }
   virtual void onRelease(){}
   DECLARE_FREELIST(BTriggerVarUserClassType, 5);
   virtual BTriggerVarType getType() const { return (BTriggerVar::cVarTypeUserClassType); }
   int32 readVar() const { checkInitialized(); return (mData); }
   void writeVar(int32 v) { mData = v; syncTriggerVar(v); initialize(); }
   virtual bool save(BStream* pStream, int saveType) { GFWRITEVAR(pStream, int32, mData); return BTriggerVar::save(pStream, saveType); }
   virtual bool load(BStream* pStream, int saveType) { GFREADVAR(pStream, int32, mData); return BTriggerVar::load(pStream, saveType); }
   #ifndef BUILD_FINAL
      virtual void getVariableDataAsString(BSimString &varDataAsString) { varDataAsString.format("%d", mData); }
   #endif
protected:
   int32 mData;
};
// NEWTRIGGERVARTYPE
// Add your class for your new trigger var type here.  See above classes for examples.
// Should be apparent what functionality is the same across all var types and what functionality is different.
