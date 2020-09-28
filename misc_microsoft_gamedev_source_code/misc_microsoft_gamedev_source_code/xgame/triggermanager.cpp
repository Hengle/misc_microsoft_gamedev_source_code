//==============================================================================
// triggermanager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "configsgame.h"
#include "trigger.h"
#include "triggercondition.h"
#include "triggereffect.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "action.h"
#include "gamedirectories.h"
#include "archiveManager.h"

// For perf stuff
#ifndef FINAL_BUILD
   #include "world.h"
#endif

GFIMPLEMENTVERSION(BTriggerManager, 1);

// Halwes - 6/9/2008 - Initial max values are guesses by DP and have not been set using any metrics yet.
#ifndef BUILD_FINAL
   #define MAX_CREATE_SQUAD_PER_FRAME 40 
   #define MAX_WORK_SQUAD_PER_FRAME 40
   #define MAX_SQUADS_IN_WORLD 500
   #define SPIKE_DELAY 3000 // ms
   #define GAME_START_DELAY 1000 // ms
#endif

enum
{
   cSaveMarkerTrigger=10000,
};

// Global Trigger Manager
BTriggerManager gTriggerManager;


//==============================================================================
// BTriggerManager::createTriggerScriptFromFile
//==============================================================================
BTriggerScriptID BTriggerManager::createTriggerScriptFromFile(long fileDir, const BSimString& fileName)
{
   // Load the trigger script file.
   BXMLReader scriptFileReader;
   const BXMLReader* pReader = gDatabase.getPreloadXmlFile(fileDir, fileName);
   if (!pReader)
   {
      if (!scriptFileReader.load(fileDir, fileName))
      {
         #ifndef BUILD_FINAL
            BSimString errorMsg;
            errorMsg.format("Unable to load the file %s!", fileName.getPtr());
            BASSERTM(false, errorMsg); // please don't take this out again. It's useful to know when files are missing in the archive build.
            trace(errorMsg);
         #endif
         return (cInvalidTriggerScriptID);
      }
      pReader = &scriptFileReader;
   }

   // Get the trigger script root node from the file.
   BXMLNode scriptFileRoot(pReader->getRootNode());
 
   if (scriptFileRoot.getName() != "TriggerSystem")
   {
      #ifndef BUILD_FINAL
         BSimString errorMsg;
         errorMsg.format("Trigger script with name %s does not have root trigger system node.", fileName.getPtr());
         BASSERTM(false, errorMsg);
      #endif
      return (cInvalidTriggerScriptID);
   }

   // Create the trigger script from XML
   BTriggerScript* pScript = createTriggerScriptFromXML(scriptFileRoot);
   if (pScript)
   {
      pScript->setFileInfo((int16)fileDir, fileName);
      return (pScript->getID());
   }
   else
      return cInvalidTriggerScriptID;
}


//==============================================================================
// BTriggerManager::createTriggerScriptFromXML
//==============================================================================
BTriggerScript* BTriggerManager::createTriggerScriptFromXML(BXMLNode triggerScriptNode)
{
   // Get trigger system instance and immediately assign it an ID.
   BTriggerScript *pTriggerScript = BTriggerScript::getInstance();
   pTriggerScript->setID(gTriggerManager.getUniqueTriggerScriptID());

   // Try to load it.
   if (!pTriggerScript->loadFromXML(triggerScriptNode))
   {
      BTriggerScript::releaseInstance(pTriggerScript);
      return (NULL);
   }

   // W00t!
   mTriggerScripts.add(pTriggerScript);   
   // Halwes - 1/9/2008 - Debug
   //BSimString debugDataString;
   //debugDataString.format("CreateScript - Name: %s, Index: %d\n", pTriggerScript->getName().getPtr(), mTriggerScripts.getSize());
   //gConsole.output(cChannelTriggers, debugDataString);
   return pTriggerScript;
}


//==============================================================================
// BTriggerManager::activateTriggerScript
//==============================================================================
void BTriggerManager::activateTriggerScript(BTriggerScriptID triggerScriptID)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->activate();
}


//==============================================================================
// BTriggerManager::BTriggerManager
//==============================================================================
BTriggerManager::BTriggerManager()
{
   mNextTriggerScriptID = 0;
   mBaseNextTriggerScriptID = 0;

#ifndef BUILD_FINAL
   mCurrentScriptName = "";
   mCurrentScriptID = cInvalidTriggerScriptID;
   mCurrentTriggerID = cInvalidTriggerID;
   mCreatePerfCount = 0;
   mWorkPerfCount = 0;
   mFlagPerfSpiked = false;
   mPerfSpikeTimeStamp = 0;
   mTriggerEffectNames.resize(BTriggerEffect::cNumTriggerEffectTypes);
#endif

   mFlagPaused = false;

   mTriggerScriptExternals.setHeap(&gSimHeap);
   mTriggerScriptExternals.init();
}

//==============================================================================
// BTriggerManager::init
//==============================================================================
void BTriggerManager::init()
{
   mBaseNextTriggerScriptID=mNextTriggerScriptID;

   mUserClassManager.load();
}

//==============================================================================
// BTriggerManager::reset
//==============================================================================
void BTriggerManager::reset()
{
   uint numTriggerScripts = mTriggerScripts.getSize();
   for (uint i=0; i<numTriggerScripts; i++)
   {
      BTriggerScript *pScript = mTriggerScripts[i];
      if (!pScript || pScript->getID()<mBaseNextTriggerScriptID)
         continue;
      BTriggerScript::releaseInstance(pScript);
      mTriggerScripts[i]=NULL;
   }
   mTriggerScripts.removeValueAllInstances(NULL);   
   mNextTriggerScriptID = mBaseNextTriggerScriptID;

   BTrigger::mFreeList.clear();
   BTriggerCondition::mFreeList.clear();
   BTriggerEffect::mFreeList.clear();
   BTriggerScript::mFreeList.clear();
   BTriggerVarTech::mFreeList.clear();
   BTriggerVarTechStatus::mFreeList.clear();
   BTriggerVarOperator::mFreeList.clear();
   BTriggerVarMathOperator::mFreeList.clear();
   BTriggerVarProtoObject::mFreeList.clear();
   BTriggerVarObjectType::mFreeList.clear();
   BTriggerVarProtoSquad::mFreeList.clear();
   BTriggerVarTrigger::mFreeList.clear();
   BTriggerVarTime::mFreeList.clear();
   BTriggerVarPlayer::mFreeList.clear();
   BTriggerVarEntity::mFreeList.clear();
   BTriggerVarEntityList::mFreeList.clear();
   BTriggerVarSound::mFreeList.clear();
   BTriggerVarUILocation::mFreeList.clear();
   BTriggerVarUIEntity::mFreeList.clear();
   BTriggerVarCost::mFreeList.clear();
   BTriggerVarAnimType::mFreeList.clear();
   BTriggerVarActionStatus::mFreeList.clear();
   BTriggerVarPower::mFreeList.clear();
   BTriggerVarBool::mFreeList.clear();
   BTriggerVarFloat::mFreeList.clear();
   BTriggerVarIterator::mFreeList.clear();
   BTriggerVarTeam::mFreeList.clear();
   BTriggerVarPlayerList::mFreeList.clear();
   BTriggerVarTeamList::mFreeList.clear();
   BTriggerVarPlayerState::mFreeList.clear();
   BTriggerVarObjective::mFreeList.clear();
   BTriggerVarString::mFreeList.clear();
   BTriggerVarMessageIndex::mFreeList.clear();
   BTriggerVarMessageJustify::mFreeList.clear();
   BTriggerVarMessagePoint::mFreeList.clear();
   BTriggerVarColor::mFreeList.clear();
   BTriggerVarUnit::mFreeList.clear();
   BTriggerVarUnitList::mFreeList.clear();
   BTriggerVarSquad::mFreeList.clear();
   BTriggerVarSquadList::mFreeList.clear();
   BTriggerVarUIUnit::mFreeList.clear();
   BTriggerVarUISquad::mFreeList.clear();
   BTriggerVarUISquadList::mFreeList.clear();
   BTriggerVarProtoObjectList::mFreeList.clear();
   BTriggerVarObjectTypeList::mFreeList.clear();
   BTriggerVarProtoSquadList::mFreeList.clear();
   BTriggerVarTechList::mFreeList.clear();
   BTriggerVarObjectDataType::mFreeList.clear();
   BTriggerVarObjectDataRelative::mFreeList.clear();
   BTriggerVarCiv::mFreeList.clear();
   BTriggerVarProtoObjectCollection::mFreeList.clear();
   BTriggerVarObject::mFreeList.clear();
   BTriggerVarObjectList::mFreeList.clear();
   BTriggerVarGroup::mFreeList.clear();
   BTriggerVarRefCountType::mFreeList.clear();
   BTriggerVarUnitFlag::mFreeList.clear();
   BTriggerVarSquadFlag::mFreeList.clear();
   BTriggerVarLOSType::mFreeList.clear();
   BTriggerVarEntityFilterSet::mFreeList.clear();
   BTriggerVarPopBucket::mFreeList.clear();
   BTriggerVarListPosition::mFreeList.clear();
   BTriggerVarRelationType::mFreeList.clear();
   BTriggerVarExposedAction::mFreeList.clear();
   BTriggerVarSquadMode::mFreeList.clear();
   BTriggerVarExposedScript::mFreeList.clear();
   BTriggerVarKBBase::mFreeList.clear();
   BTriggerVarKBBaseList::mFreeList.clear();
   BTriggerVarDataScalar::mFreeList.clear();
   BTriggerVarKBBaseQuery::mFreeList.clear();
   BTriggerVarDesignLine::mFreeList.clear();
   BTriggerVarLocStringID::mFreeList.clear();
   BTriggerVarLeader::mFreeList.clear();
   BTriggerVarCinematic::mFreeList.clear();
   BTriggerVarTalkingHead::mFreeList.clear();
   BTriggerVarFlareType::mFreeList.clear();
   BTriggerVarCinematicTag::mFreeList.clear();
   BTriggerVarIconType::mFreeList.clear();
   BTriggerVarDifficulty::mFreeList.clear();
   BTriggerVarInteger::mFreeList.clear();
   BTriggerVarHUDItem::mFreeList.clear();
   BTriggerVarFlashableUIItem::mFreeList.clear();
   BTriggerVarControlType::mFreeList.clear();
   BTriggerVarUIButton::mFreeList.clear();
   BTriggerVarMissionType::mFreeList.clear();
   BTriggerVarMissionState::mFreeList.clear();
   BTriggerVarMissionTargetType::mFreeList.clear();
   BTriggerVarIntegerList::mFreeList.clear();
   BTriggerVarBidType::mFreeList.clear();
   BTriggerVarBidState::mFreeList.clear();
   BTriggerVarBuildingCommandState::mFreeList.clear();
   BTriggerVarVector::mFreeList.clear();
   BTriggerVarVectorList::mFreeList.clear();
   BTriggerVarPlacementRule::mFreeList.clear();
   BTriggerVarKBSquad::mFreeList.clear();
   BTriggerVarKBSquadList::mFreeList.clear();
   BTriggerVarKBSquadQuery::mFreeList.clear();
   BTriggerVarAISquadAnalysis::mFreeList.clear();
   BTriggerVarAISquadAnalysisComponent::mFreeList.clear();
   BTriggerVarKBSquadFilterSet::mFreeList.clear();
   BTriggerVarChatSpeaker::mFreeList.clear();
   BTriggerVarRumbleType::mFreeList.clear();
   BTriggerVarRumbleMotor::mFreeList.clear();
   BTriggerVarTechDataCommandType::mFreeList.clear();
   BTriggerVarSquadDataType::mFreeList.clear();
   BTriggerVarEventType::mFreeList.clear();
   BTriggerVarTimeList::mFreeList.clear();
   BTriggerVarDesignLineList::mFreeList.clear();
   BTriggerVarGameStatePredicate::mFreeList.clear();
   BTriggerVarFloatList::mFreeList.clear();
   BTriggerVarUILocationMinigame::mFreeList.clear();
   BTriggerVarConcept::mFreeList.clear();
   BTriggerVarConceptList::mFreeList.clear();
   BTriggerVarUserClassType::mFreeList.clear();

   BKBSquadFilterCurrentlyVisible::mFreeList.clear();
   BKBSquadFilterInList::mFreeList.clear();
   BKBSquadFilterPlayers::mFreeList.clear();
   BKBSquadFilterObjectTypes::mFreeList.clear();
   BKBSquadFilterPlayerRelation::mFreeList.clear();
   BKBSquadFilterMinStaleness::mFreeList.clear();
   BKBSquadFilterMaxStaleness::mFreeList.clear();

   mTriggerScriptExternals.clear();
}

//==============================================================================
// BTriggerManager::shutdown
//==============================================================================
void BTriggerManager::shutdown()
{
   gTriggerManager.reset();
}

//==============================================================================
// BTriggerManager::getTriggerVarTypeEnum
//==============================================================================
BTriggerVarType BTriggerManager::getTriggerVarTypeEnum(const BSimString& varTypeName)
{
   BTriggerVarType varTypeEnum = BTriggerVar::cVarTypeInvalid;

   if (varTypeName == "Tech")
      varTypeEnum = BTriggerVar::cVarTypeTech;
   else if (varTypeName == "TechStatus")
      varTypeEnum = BTriggerVar::cVarTypeTechStatus;
   else if (varTypeName == "Operator")
      varTypeEnum = BTriggerVar::cVarTypeOperator;
   else if (varTypeName == "ProtoObject")
      varTypeEnum = BTriggerVar::cVarTypeProtoObject;
   else if (varTypeName == "ObjectType")
      varTypeEnum = BTriggerVar::cVarTypeObjectType;
   else if (varTypeName == "ProtoSquad")
      varTypeEnum = BTriggerVar::cVarTypeProtoSquad;
   else if (varTypeName == "Sound")
      varTypeEnum = BTriggerVar::cVarTypeSound;
   else if (varTypeName == "Entity")
      varTypeEnum = BTriggerVar::cVarTypeEntity;
   else if (varTypeName == "EntityList")
      varTypeEnum = BTriggerVar::cVarTypeEntityList;
   else if (varTypeName == "Trigger")
      varTypeEnum = BTriggerVar::cVarTypeTrigger;
   else if (varTypeName == "Distance")
      varTypeEnum = BTriggerVar::cVarTypeFloat;
   else if (varTypeName == "Time")
      varTypeEnum = BTriggerVar::cVarTypeTime;
   else if (varTypeName == "Player")
      varTypeEnum = BTriggerVar::cVarTypePlayer;
   else if (varTypeName == "Count")
      varTypeEnum = BTriggerVar::cVarTypeInteger;
   else if (varTypeName == "Location")
      varTypeEnum = BTriggerVar::cVarTypeVector;
   else if (varTypeName == "UILocation")
      varTypeEnum = BTriggerVar::cVarTypeUILocation;
   else if (varTypeName == "UIEntity")
      varTypeEnum = BTriggerVar::cVarTypeUIEntity;
   else if (varTypeName == "Cost")
      varTypeEnum = BTriggerVar::cVarTypeCost;
   else if (varTypeName == "AnimType")
      varTypeEnum = BTriggerVar::cVarTypeAnimType;
   else if (varTypeName == "ActionStatus")
      varTypeEnum = BTriggerVar::cVarTypeActionStatus;
   else if (varTypeName == "Percent")
      varTypeEnum = BTriggerVar::cVarTypeFloat;
   else if (varTypeName == "Hitpoints")
      varTypeEnum = BTriggerVar::cVarTypeFloat;
   else if (varTypeName == "Power")
      varTypeEnum = BTriggerVar::cVarTypePower;
   else if (varTypeName == "Bool")
      varTypeEnum = BTriggerVar::cVarTypeBool;
   else if (varTypeName == "Float")
      varTypeEnum = BTriggerVar::cVarTypeFloat;
   else if (varTypeName == "Iterator")
      varTypeEnum = BTriggerVar::cVarTypeIterator;
   else if (varTypeName == "Team")
      varTypeEnum = BTriggerVar::cVarTypeTeam;
   else if (varTypeName == "PlayerList")
      varTypeEnum = BTriggerVar::cVarTypePlayerList;
   else if (varTypeName == "TeamList")
      varTypeEnum = BTriggerVar::cVarTypeTeamList;
   else if (varTypeName == "PlayerState")
      varTypeEnum = BTriggerVar::cVarTypePlayerState;
   else if (varTypeName == "Objective")
      varTypeEnum = BTriggerVar::cVarTypeObjective;
   else if (varTypeName == "Unit")
      varTypeEnum = BTriggerVar::cVarTypeUnit;
   else if (varTypeName == "UnitList")
      varTypeEnum = BTriggerVar::cVarTypeUnitList;
   else if (varTypeName == "Squad")
      varTypeEnum = BTriggerVar::cVarTypeSquad;
   else if (varTypeName == "SquadList")
      varTypeEnum = BTriggerVar::cVarTypeSquadList;
   else if (varTypeName == "UIUnit")
      varTypeEnum = BTriggerVar::cVarTypeUIUnit;
   else if (varTypeName == "UISquad")
      varTypeEnum = BTriggerVar::cVarTypeUISquad;
   else if (varTypeName == "UISquadList")
      varTypeEnum = BTriggerVar::cVarTypeUISquadList;
   else if (varTypeName == "String")
      varTypeEnum = BTriggerVar::cVarTypeString;
   else if (varTypeName == "MessageIndex")
      varTypeEnum = BTriggerVar::cVarTypeMessageIndex;
   else if (varTypeName == "MessageJustify")
      varTypeEnum = BTriggerVar::cVarTypeMessageJustify;
   else if (varTypeName == "MessagePoint")
      varTypeEnum = BTriggerVar::cVarTypeMessagePoint;
   else if (varTypeName == "Color")
      varTypeEnum = BTriggerVar::cVarTypeColor;
   else if (varTypeName == "ProtoObjectList")
      varTypeEnum = BTriggerVar::cVarTypeProtoObjectList;
   else if (varTypeName == "ObjectTypeList")
      varTypeEnum = BTriggerVar::cVarTypeObjectTypeList;
   else if (varTypeName == "ProtoSquadList")
      varTypeEnum = BTriggerVar::cVarTypeProtoSquadList;
   else if (varTypeName == "TechList")
      varTypeEnum = BTriggerVar::cVarTypeTechList;
   else if (varTypeName == "MathOperator")
      varTypeEnum = BTriggerVar::cVarTypeMathOperator;
   else if (varTypeName == "ObjectDataType")
      varTypeEnum = BTriggerVar::cVarTypeObjectDataType;
   else if (varTypeName == "ObjectDataRelative")
      varTypeEnum = BTriggerVar::cVarTypeObjectDataRelative;
   else if (varTypeName == "Civ")
      varTypeEnum = BTriggerVar::cVarTypeCiv;
   else if (varTypeName == "ProtoObjectCollection")
      varTypeEnum = BTriggerVar::cVarTypeProtoObjectCollection;
   else if (varTypeName == "Object")
      varTypeEnum = BTriggerVar::cVarTypeObject;
   else if (varTypeName == "ObjectList")
      varTypeEnum = BTriggerVar::cVarTypeObjectList;
   else if (varTypeName == "Group")
      varTypeEnum = BTriggerVar::cVarTypeGroup;
   else if (varTypeName == "LocationList")
      varTypeEnum = BTriggerVar::cVarTypeVectorList;
   else if (varTypeName == "RefCountType")
      varTypeEnum = BTriggerVar::cVarTypeRefCountType;
   else if (varTypeName == "UnitFlag")
      varTypeEnum = BTriggerVar::cVarTypeUnitFlag;
   else if (varTypeName == "LOSType")
      varTypeEnum = BTriggerVar::cVarTypeLOSType;
   else if (varTypeName == "EntityFilterSet")
      varTypeEnum = BTriggerVar::cVarTypeEntityFilterSet;
   else if (varTypeName == "PopBucket")
      varTypeEnum = BTriggerVar::cVarTypePopBucket;
   else if (varTypeName == "ListPosition")
      varTypeEnum = BTriggerVar::cVarTypeListPosition;
   else if (varTypeName == "Diplomacy")
      varTypeEnum = BTriggerVar::cVarTypeRelationType;
   else if (varTypeName == "ExposedAction")
      varTypeEnum = BTriggerVar::cVarTypeExposedAction;
   else if (varTypeName == "SquadMode")
      varTypeEnum = BTriggerVar::cVarTypeSquadMode;
   else if (varTypeName == "ExposedScript")
      varTypeEnum = BTriggerVar::cVarTypeExposedScript;
   else if (varTypeName == "KBBase")
      varTypeEnum = BTriggerVar::cVarTypeKBBase;
   else if (varTypeName == "KBBaseList")
      varTypeEnum = BTriggerVar::cVarTypeKBBaseList;
   else if (varTypeName == "DataScalar")
      varTypeEnum = BTriggerVar::cVarTypeDataScalar;
   else if (varTypeName == "KBBaseQuery")
      varTypeEnum = BTriggerVar::cVarTypeKBBaseQuery;
   else if (varTypeName == "DesignLine")
      varTypeEnum = BTriggerVar::cVarTypeDesignLine;
   else if (varTypeName == "LocStringID")
      varTypeEnum = BTriggerVar::cVarTypeLocStringID;
   else if (varTypeName == "Leader")
      varTypeEnum = BTriggerVar::cVarTypeLeader;
   else if (varTypeName == "Cinematic")
      varTypeEnum = BTriggerVar::cVarTypeCinematic;
   else if (varTypeName == "TalkingHead")
      varTypeEnum = BTriggerVar::cVarTypeTalkingHead;
   else if (varTypeName == "Direction")
      varTypeEnum = BTriggerVar::cVarTypeVector;
   else if (varTypeName == "FlareType")
      varTypeEnum = BTriggerVar::cVarTypeFlareType;
   else if (varTypeName == "CinematicTag")
      varTypeEnum = BTriggerVar::cVarTypeCinematicTag;
   else if (varTypeName == "IconType")
      varTypeEnum = BTriggerVar::cVarTypeIconType;
   else if (varTypeName == "Difficulty")
      varTypeEnum = BTriggerVar::cVarTypeDifficulty;
   else if (varTypeName == "Integer")
      varTypeEnum = BTriggerVar::cVarTypeInteger;
   else if (varTypeName == "HUDItem")
      varTypeEnum = BTriggerVar::cVarTypeHUDItem;
   else if (varTypeName == "FlashableUIItem")
      varTypeEnum = BTriggerVar::cVarTypeFlashableUIItem;
   else if (varTypeName == "ControlType")
      varTypeEnum = BTriggerVar::cVarTypeControlType;
   else if (varTypeName == "UIButton")
      varTypeEnum = BTriggerVar::cVarTypeUIButton;
   else if (varTypeName == "MissionType")
      varTypeEnum = BTriggerVar::cVarTypeMissionType;
   else if (varTypeName == "MissionState")
      varTypeEnum = BTriggerVar::cVarTypeMissionState;
   else if (varTypeName == "MissionTargetType")
      varTypeEnum = BTriggerVar::cVarTypeMissionTargetType;
   else if (varTypeName == "IntegerList")
      varTypeEnum = BTriggerVar::cVarTypeIntegerList;
   else if (varTypeName == "BidType")
      varTypeEnum = BTriggerVar::cVarTypeBidType;
   else if (varTypeName == "BidState")
      varTypeEnum = BTriggerVar::cVarTypeBidState;
   else if (varTypeName == "BuildingCommandState")
      varTypeEnum = BTriggerVar::cVarTypeBuildingCommandState;
   else if (varTypeName == "Vector")
      varTypeEnum = BTriggerVar::cVarTypeVector;
   else if (varTypeName == "VectorList")
      varTypeEnum = BTriggerVar::cVarTypeVectorList;
   else if (varTypeName == "PlacementRule")
      varTypeEnum = BTriggerVar::cVarTypePlacementRule;
   else if (varTypeName == "KBSquad")
      varTypeEnum = BTriggerVar::cVarTypeKBSquad;
   else if (varTypeName == "KBSquadList")
      varTypeEnum = BTriggerVar::cVarTypeKBSquadList;
   else if (varTypeName == "KBSquadQuery")
      varTypeEnum = BTriggerVar::cVarTypeKBSquadQuery;
   else if (varTypeName == "AISquadAnalysis")
      varTypeEnum = BTriggerVar::cVarTypeAISquadAnalysis;
   else if (varTypeName == "AISquadAnalysisComponent")
      varTypeEnum = BTriggerVar::cVarTypeAISquadAnalysisComponent;
   else if (varTypeName == "KBSquadFilterSet")
      varTypeEnum = BTriggerVar::cVarTypeKBSquadFilterSet;
   else if (varTypeName == "ChatSpeaker")
      varTypeEnum = BTriggerVar::cVarTypeChatSpeaker;
   else if (varTypeName == "RumbleType")
      varTypeEnum = BTriggerVar::cVarTypeRumbleType;
   else if (varTypeName == "RumbleMotor")
      varTypeEnum = BTriggerVar::cVarTypeRumbleMotor;
   else if (varTypeName == "CommandType")
      varTypeEnum = BTriggerVar::cVarTypeTechDataCommandType;
   else if (varTypeName == "SquadDataType")
      varTypeEnum = BTriggerVar::cVarTypeSquadDataType;
   else if (varTypeName == "EventType")
      varTypeEnum = BTriggerVar::cVarTypeEventType;
   else if (varTypeName == "TimeList")
      varTypeEnum = BTriggerVar::cVarTypeTimeList;
   else if (varTypeName == "DesignLineList")
      varTypeEnum = BTriggerVar::cVarTypeDesignLineList;
   else if (varTypeName == "GameStatePredicate")
      varTypeEnum = BTriggerVar::cVarTypeGameStatePredicate;
   else if (varTypeName == "FloatList")
      varTypeEnum = BTriggerVar::cVarTypeFloatList;
   else if (varTypeName == "UILocationMinigame")
      varTypeEnum = BTriggerVar::cVarTypeUILocationMinigame;
   else if (varTypeName == "SquadFlag")
      varTypeEnum = BTriggerVar::cVarTypeSquadFlag;
   else if (varTypeName == "Concept")
      varTypeEnum = BTriggerVar::cVarTypeConcept;
   else if (varTypeName == "ConceptList")
      varTypeEnum = BTriggerVar::cVarTypeConceptList;
   else if (varTypeName == "UserClassType")
      varTypeEnum = BTriggerVar::cVarTypeUserClassType;

   // NEWTRIGGERVARTYPE
   // Add your else if statement here...
   // The text is the name that is exported by the editor... and the enumeration is the enum set up in triggervar.h
   // These things should not ever change, so be sure of what you want when you add it.

   BASSERTM(varTypeEnum != BTriggerVar::cVarTypeInvalid, "Invalid var type enum");
   return (varTypeEnum);
}


//==============================================================================
//==============================================================================
int BTriggerManager::getTriggerOperatorTypeEnum(const BSimString& operatorTypeName)
{
   int operatorTypeEnum = -1;

   if (operatorTypeName == "NotEqualTo")
      operatorTypeEnum = Math::cNotEqualTo;
   else if (operatorTypeName == "LessThan")
      operatorTypeEnum = Math::cLessThan;
   else if (operatorTypeName == "LessThanOrEqualTo")
      operatorTypeEnum = Math::cLessThanOrEqualTo;
   else if (operatorTypeName == "EqualTo")
      operatorTypeEnum = Math::cEqualTo;
   else if (operatorTypeName == "GreaterThanOrEqualTo")
      operatorTypeEnum = Math::cGreaterThanOrEqualTo;
   else if (operatorTypeName == "GreaterThan")
      operatorTypeEnum = Math::cGreaterThan;

   BASSERTM(operatorTypeEnum != -1, "Invalid operator type enum");
   return (operatorTypeEnum);
}

//==============================================================================
//==============================================================================
int BTriggerManager::getTriggerMathOperatorTypeEnum(const BSimString& mathOperatorTypeName)
{
   int mathOperatorTypeEnum = -1; // -1 is invalid.. I guess.

   if (mathOperatorTypeName == "Add")
      mathOperatorTypeEnum = Math::cOpTypeAdd;
   else if (mathOperatorTypeName == "Subtract")
      mathOperatorTypeEnum = Math::cOpTypeSubtract;
   else if (mathOperatorTypeName == "Multiply")
      mathOperatorTypeEnum = Math::cOpTypeMultiply;
   else if (mathOperatorTypeName == "Divide")
      mathOperatorTypeEnum = Math::cOpTypeDivide;
   else if (mathOperatorTypeName == "Modulus")
      mathOperatorTypeEnum = Math::cOpTypeModulus;

   BASSERTM(mathOperatorTypeEnum != -1, "Invalid math operator type enum");
   return (mathOperatorTypeEnum);
}

//==============================================================================
//==============================================================================
int BTriggerManager::getTriggerListPositionTypeEnum(const BSimString& listPositionTypeName)
{
   int listPositionTypeEnum = BTriggerVarListPosition::cListPosTypeFirst;

   if (listPositionTypeName == "First")
      listPositionTypeEnum = BTriggerVarListPosition::cListPosTypeFirst;
   else if (listPositionTypeName == "Last")
      listPositionTypeEnum = BTriggerVarListPosition::cListPosTypeLast;
   else if (listPositionTypeName == "Random")
      listPositionTypeEnum = BTriggerVarListPosition::cListPosTypeRandom;

   return (listPositionTypeEnum);
}

//==============================================================================
// BTriggerManager::activateTrigger
//==============================================================================
void BTriggerManager::activateTrigger(BTriggerScriptID triggerScriptID, BTriggerID triggerID)
{
   BTriggerScript *pTriggerScript = getTriggerScript(triggerScriptID);
   BASSERT(pTriggerScript);
   if (!pTriggerScript)
      return;
   pTriggerScript->activateTrigger(triggerID);
}


//==============================================================================
// BTriggerManager::deactivateTrigger
//==============================================================================
void BTriggerManager::deactivateTrigger(BTriggerScriptID triggerScriptID, BTriggerID triggerID)
{
   BTriggerScript *pTriggerScript = getTriggerScript(triggerScriptID);
   BASSERT(pTriggerScript);
   if (!pTriggerScript)
      return;
   pTriggerScript->deactivateTrigger(triggerID);
}

//==============================================================================
// BTriggerManager::setAsyncTriggerConditionState
//==============================================================================
void BTriggerManager::setAsyncTriggerConditionState(BTriggerScriptID triggerScriptID, BTriggerID triggerID, long conditionID, bool state)
{
   BTriggerScript *pTriggerScript = getTriggerScript(triggerScriptID);
   if (!pTriggerScript)
      return;
   BTrigger *pTrigger = pTriggerScript->getTrigger(triggerID);
   if (!pTrigger)
      return;

   pTrigger->setAsyncConditionState(conditionID, state);
}


//==============================================================================
// BTriggerManager::addExternalPlayerID
//==============================================================================
void BTriggerManager::addExternalPlayerID(BTriggerScriptID triggerScriptID, BPlayerID playerID)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addPlayerID(playerID);
}


//==============================================================================
// BTriggerManager::addExternalProtoPowerID
//==============================================================================
void BTriggerManager::addExternalProtoPowerID(BTriggerScriptID triggerScriptID, BProtoPowerID protoPowerID)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addProtoPowerID(protoPowerID);
}


//==============================================================================
// BTriggerManager::addExternalSquadID
//==============================================================================
void BTriggerManager::addExternalSquadID(BTriggerScriptID triggerScriptID, BEntityID squadID)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addSquadID(squadID);
}

//==============================================================================
// Add a list of squad IDs to the trigger script external input
//==============================================================================
void BTriggerManager::addExternalSquadIDs(BTriggerScriptID triggerScriptID, BEntityIDArray squadIDs)
{
   BTriggerScript* pScript = getTriggerScript(triggerScriptID);
   if (pScript)
   {
      pScript->getExternals()->addSquadIDs(squadIDs);
   }
}

//==============================================================================
// BTriggerManager::addExternalUnitID
//==============================================================================
void BTriggerManager::addExternalUnitID(BTriggerScriptID triggerScriptID, BEntityID unitID)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addUnitID(unitID);
}

//==============================================================================
// Add a list of unit IDs to the trigger script external input
//==============================================================================
void BTriggerManager::addExternalUnitIDs(BTriggerScriptID triggerScriptID, BEntityIDArray unitIDs)
{
   BTriggerScript* pScript = getTriggerScript(triggerScriptID);
   if (pScript)
   {
      pScript->getExternals()->addUnitIDs(unitIDs);
   }
}

//==============================================================================
// BTriggerManager::addExternalCost
//==============================================================================
void BTriggerManager::addExternalCost(BTriggerScriptID triggerScriptID, BCost cost)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addCost(cost);
}


//==============================================================================
//==============================================================================
void BTriggerManager::addExternalLocation(BTriggerScriptID triggerScriptID, BVector location)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addLocation(location);
}


//==============================================================================
//==============================================================================
void BTriggerManager::addExternalLocationList(BTriggerScriptID triggerScriptID, const BLocationArray& locationList)
{
   BTriggerScript *pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addLocationList(locationList);
}

//==============================================================================
//==============================================================================
void BTriggerManager::addExternalFlag(BTriggerScriptID triggerScriptID, int index, bool flag)
{
   BTriggerScript* pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addFlag(index, flag);
}

//==============================================================================
//==============================================================================
void BTriggerManager::addExternalFloat(BTriggerScriptID triggerScriptID, float value)
{
   BTriggerScript* pScript = getTriggerScript(triggerScriptID);
   if (pScript)
      pScript->getExternals()->addFloat(value);
}

//==============================================================================
// BTriggerManager::update
//==============================================================================
void BTriggerManager::update(DWORD currentUpdateTime)
{
   SCOPEDSAMPLE(BTriggerManager_update);
   
#ifndef BUILD_FINAL
   resetPerfSpike();
   resetCreatePerfCount();
   resetWorkPerfCount();
#endif

   // Do not update any scripts if paused.
   if (mFlagPaused)
      return;

   uint numTriggerScripts = mTriggerScripts.getSize();
   for (uint i = 0; i < numTriggerScripts; i++)
   {
      BTriggerScript* pScript = mTriggerScripts[i];
      if (!pScript || !pScript->isActive())
         continue;

#ifndef BUILD_FINAL
      setCurrentScriptName(pScript->getName());
      setCurrentScriptID(pScript->getID());
      setCurrentTriggerID(cInvalidTriggerID);
#endif

      pScript->update(currentUpdateTime);

#ifndef BUILD_FINAL
      evaluateCreatePerfCount();
      evaluateWorkPerfCount();

      setCurrentScriptName("");
      setCurrentScriptID(cInvalidTriggerScriptID);
      setCurrentTriggerID(cInvalidTriggerID);
#endif

      if (pScript->isMarkedForCleanup())
      {
         BTriggerScript::releaseInstance(pScript);
         mTriggerScripts[i] = NULL;
      }
   }

   mTriggerScripts.removeValueAllInstances(NULL);   
}

//==============================================================================
//==============================================================================
void BTriggerManager::invalidateEntityID(BEntityID entityID)
{
   uint numTriggerScripts = mTriggerScripts.getSize();
   for (uint i = 0; i < numTriggerScripts; i++)
   {
      BTriggerScript* pScript = mTriggerScripts[i];      
      if (!pScript)
         continue;
      if (!pScript->isActive())
         continue;
      pScript->invalidateEntityID(entityID);
   }
}

//==============================================================================
//==============================================================================
BTriggerScriptExternals* BTriggerManager::getTriggerScriptExternal(uint index)
{
   if (mTriggerScriptExternals.isInUse(index))
      return (mTriggerScriptExternals.get(index));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
BTriggerScriptExternals* BTriggerManager::acquireTriggerScriptExternal(uint &index)
{
   return (mTriggerScriptExternals.acquire(index, true));
}


//==============================================================================
//==============================================================================
void BTriggerManager::releaseTriggerScriptExternal(uint index)
{
   if (mTriggerScriptExternals.isInUse(index))
      mTriggerScriptExternals.release(index);
}

#ifndef BUILD_FINAL
//==============================================================================
// Reset the perf spiked flag and time stamp
//==============================================================================
void BTriggerManager::resetPerfSpike() 
{ 
   if (gWorld->getGametime() > mPerfSpikeTimeStamp)
   {
      mPerfSpikeTimeStamp = 0; 
      mFlagPerfSpiked = false; 
   }
}

//==============================================================================
// Set the perf spiked flag and time stamp
//==============================================================================
void BTriggerManager::activatePerfSpike() 
{ 
      mFlagPerfSpiked = true; 
      mPerfSpikeTimeStamp = gWorld->getGametime() + SPIKE_DELAY;
}

//==============================================================================
// Evaluate squad creation performance counter
//==============================================================================
void BTriggerManager::evaluateCreatePerfCount()
{    
   bool frameResult = true;
   if (gWorld->getGametime() > GAME_START_DELAY)
   {
      frameResult = (mCreatePerfCount < MAX_CREATE_SQUAD_PER_FRAME);
   }
   bool maxResult = (mCreatePerfCount > 0) ? (gWorld->getNumberSquads() < MAX_SQUADS_IN_WORLD) : true;
   if(gConfig.isDefined(cConfigEnableTriggerPerfAsserts))
   {
      BTRIGGER_ASSERTM(frameResult, "PERF WARNING:  Too many squads created via triggers in one frame!");
      BTRIGGER_ASSERTM(maxResult, "PERF WARNING:  Trigger system creating squads beyond maximum number of squads allowed!");
   }

   if (!frameResult || !maxResult)
   {
      activatePerfSpike();
   }
}

//==============================================================================
// Evaluate squad work performance counter
//==============================================================================
void BTriggerManager::evaluateWorkPerfCount() 
{ 
   bool result = (mWorkPerfCount < MAX_WORK_SQUAD_PER_FRAME);

   if(gConfig.isDefined(cConfigEnableTriggerPerfAsserts))
   {
      BTRIGGER_ASSERTM(result, "PERF WARNING:  Too many squads commanded to move/work via triggers in one frame!");
   }

   if (!result)
   {
      activatePerfSpike();
   }
}
#endif

//==============================================================================
//==============================================================================
BTriggerScript* BTriggerManager::getTriggerScript(BTriggerScriptID triggerScriptID)
{
   uint numTriggerScripts = mTriggerScripts.getSize();
   for (uint i=0; i<numTriggerScripts; i++)
   {
      BTriggerScript *pScript = mTriggerScripts[i];
      if (pScript && pScript->getID() == triggerScriptID)
         return (pScript);
   }
   return (NULL);
}


//==============================================================================
//==============================================================================
BTriggerScript* BTriggerManager::getTriggerScriptByIndex(uint index)
{
   uint numTriggerScripts = mTriggerScripts.getSize();
   if (index < numTriggerScripts)
      return (mTriggerScripts[index]);
   else
      return (NULL);
}


//==============================================================================
// BTriggerManager::getUniqueTriggerScriptID
//==============================================================================
BTriggerScriptID BTriggerManager::getUniqueTriggerScriptID()
{
   BTriggerScriptID returnVal = mNextTriggerScriptID;
   mNextTriggerScriptID++;
   return (returnVal);
}

//==============================================================================
//==============================================================================
bool BTriggerManager::save(BStream* pStream, int saveType) const
{
   int scriptCount = mTriggerScripts.getNumber();
   GFVERIFYCOUNT(scriptCount, 5000);
   GFWRITEVAR(pStream, int, scriptCount);
   for (int i=0; i<scriptCount; i++)
   {
//-- FIXING PREFIX BUG ID 3289
      const BTriggerScript* pScript = mTriggerScripts[i];
//--
      if (!pScript)
         continue;
      BTriggerScriptID scriptID = pScript->getID();
      if (scriptID == -1)
      {
         GFERROR("GameFile Error: invalid trigger script id");
         return false;
      }
      GFWRITEVAR(pStream, BTriggerScriptID, scriptID);
      if (!pScript->save(pStream, saveType))
         return false;
      GFWRITEMARKER(pStream, cSaveMarkerTrigger);
   }
   GFWRITEVAL(pStream, BTriggerScriptID, -1);

   GFWRITEVAR(pStream, bool, mFlagPaused);
   GFWRITEVAR(pStream, BTriggerScriptID, mNextTriggerScriptID);
   GFWRITEVAR(pStream, BTriggerScriptID, mBaseNextTriggerScriptID);

   return true;
}

//==============================================================================
//==============================================================================
bool BTriggerManager::load(BStream* pStream, int saveType)
{
   int scriptCount;
   GFREADVAR(pStream, int, scriptCount);
   GFVERIFYCOUNT(scriptCount, 5000);
   uint16 loadCount = 0;
   BTriggerScriptID scriptID;
   GFREADVAR(pStream, BTriggerScriptID, scriptID);
   while (scriptID != -1)
   {
      if (loadCount >= scriptCount)
      {
         GFERROR("GameFile Error: too many triggers");
         return false;
      }
      BTriggerScript* pScript = BTriggerScript::getInstance();
      pScript->setID(scriptID);
      mTriggerScripts.add(pScript);   
      if (!pScript->load(pStream, saveType))
         return false;
      loadCount++;
      GFREADMARKER(pStream, cSaveMarkerTrigger);
      GFREADVAR(pStream, BTriggerScriptID, scriptID);
   }

   GFREADVAR(pStream, bool, mFlagPaused);
   GFREADVAR(pStream, BTriggerScriptID, mNextTriggerScriptID);
   GFREADVAR(pStream, BTriggerScriptID, mBaseNextTriggerScriptID);

   return true;
}
