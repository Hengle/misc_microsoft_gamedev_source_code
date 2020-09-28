//==============================================================================
// triggervar.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "database.h"
#include "generaleventmanager.h"
#include "protoobject.h"
#include "protosquad.h"
#include "prototech.h"
#include "scenario.h"
#include "squad.h"
#include "techtree.h"
#include "trigger.h"
#include "triggermanager.h"
#include "triggervar.h"
#include "unit.h"
#include "world.h"
#include "configsgame.h"
#include "unitactionbuilding.h"
#include "chatmanager.h"


// xsystem
#include "xmlreader.h"

GFIMPLEMENTVERSION(BTriggerVar, 3);

//==============================================================================
// Block Pool Implementations
//==============================================================================
IMPLEMENT_FREELIST(BTriggerVarTech, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTechStatus, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarOperator, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarProtoObject, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarObjectType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarProtoSquad, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTrigger, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTime, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarPlayer, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarEntity, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarEntityList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarSound, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUILocation, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUIEntity, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarCost, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarAnimType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarActionStatus, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarPower, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarBool, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarFloat, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarIterator, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTeam, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarPlayerList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTeamList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarPlayerState, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarObjective, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUnit, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUnitList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarSquad, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarSquadList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUIUnit, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUISquad, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUISquadList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarString, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarMessageIndex, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarMessageJustify, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarMessagePoint, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarColor, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarProtoObjectList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarObjectTypeList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarProtoSquadList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTechList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarMathOperator, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarObjectDataType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarObjectDataRelative, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarCiv, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarProtoObjectCollection, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarObject, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarObjectList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarGroup, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarRefCountType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUnitFlag, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarLOSType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarEntityFilterSet, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarPopBucket, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarListPosition, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarRelationType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarExposedAction, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarSquadMode, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarExposedScript, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarKBBase, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarKBBaseList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarDataScalar, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarKBBaseQuery, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarDesignLine, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarLocStringID, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarLeader, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarCinematic, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTalkingHead, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarFlareType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarCinematicTag, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarIconType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarDifficulty, 4, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarInteger, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarHUDItem, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarFlashableUIItem, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarControlType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUIButton, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarMissionType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarMissionState, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarMissionTargetType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarIntegerList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarBidType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarBidState, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarBuildingCommandState, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarVector, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarVectorList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarPlacementRule, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarKBSquad, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarKBSquadList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarKBSquadQuery, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarAISquadAnalysis, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarAISquadAnalysisComponent, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarKBSquadFilterSet, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarChatSpeaker, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarRumbleType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarRumbleMotor, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTechDataCommandType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarSquadDataType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarEventType, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarTimeList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarDesignLineList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarGameStatePredicate, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarFloatList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUILocationMinigame, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarSquadFlag, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarConcept, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarConceptList, 5, &gSimHeap);
IMPLEMENT_FREELIST(BTriggerVarUserClassType, 5, &gSimHeap);


// NEWTRIGGERVARTYPE
// Add IMPLEMENT_FREELIST macro for your new trigger variable type.

//==============================================================================
// BTriggerVar::outputDebugDataToXFS()
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVar::outputDebugDataToXFS()
{
   if (gConfig.isDefined(cEnableTriggerDebugs))
   {
      BSimString debugDataString;
      BSimString varName = getName();
      if (varName.isEmpty())
         varName = "(NONE)";
      BSimString varDataAsString;
      getVariableDataAsString(varDataAsString);

      debugDataString.format("Trigger Variable:\t\tID = %d,\t\tName = %s,\t\tValue = %s", getID(), varName.getPtr(), varDataAsString.getPtr());

      if (gConfig.isDefined(cEnableTriggerDebugsStatusText))
      {
         gConsole.outputStatusText(2000, 0xffffffff, cChannelTriggers, debugDataString);
      }
      else
      {
         gConsole.output(cChannelTriggers, debugDataString);
      }
   }
}
#endif


//==============================================================================
// BTriggerVar::outputDebugDataToXFS()
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVar::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.set("(NOT IMPLEMENTED)");
}
#endif


//==============================================================================
// BTriggerVarTech::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarTech::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getProtoTechName(mData));
}
#endif


//==============================================================================
// BTriggerVarTechStatus::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarTechStatus::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mData == BTechTree::cStatusUnobtainable)
      varDataAsString = "Unobtainable";
   else if (mData == BTechTree::cStatusObtainable)
      varDataAsString = "Obtainable";
   else if (mData == BTechTree::cStatusAvailable)
      varDataAsString = "Available";
   else if (mData == BTechTree::cStatusResearching)
      varDataAsString = "Researching";
   else if (mData == BTechTree::cStatusActive)
      varDataAsString = "Active";
   else if (mData == BTechTree::cStatusDisabled)
      varDataAsString = "Disabled";
   else if (mData == BTechTree::cStatusCoopResearching)
      varDataAsString = "CoopResearching";
   else
      varDataAsString = "INVALID";
}
#endif


//==============================================================================
// BTriggerVarOperator::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarOperator::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mData == Math::cNotEqualTo)
      varDataAsString = "NotEqualTo";
   else if (mData == Math::cLessThan)
      varDataAsString = "LessThan";
   else if (mData == Math::cLessThanOrEqualTo)
      varDataAsString = "LessThanOrEqualTo";
   else if (mData == Math::cEqualTo)
      varDataAsString = "EqualTo";
   else if (mData == Math::cGreaterThanOrEqualTo)
      varDataAsString = "GreaterThanOrEqualTo";
   else if (mData == Math::cGreaterThan)
      varDataAsString = "GreaterThan";
   else
      varDataAsString = "INVALID";
}
#endif


//==============================================================================
// BTriggerVarMathOperator::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarMathOperator::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mData == Math::cOpTypeAdd)
      varDataAsString = "Add";
   else if (mData == Math::cOpTypeSubtract)
      varDataAsString = "Subtract";
   else if (mData == Math::cOpTypeMultiply)
      varDataAsString = "Multiply";
   else if (mData == Math::cOpTypeDivide)
      varDataAsString = "Divide";
   else if (mData == Math::cOpTypeModulus)
      varDataAsString = "Modulus";
   else
      varDataAsString = "INVALID";
}
#endif


//==============================================================================
// BTriggerVarProtoObject::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarProtoObject::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getProtoObjectName(mData));
}
#endif


//==============================================================================
// BTriggerVarObjectType::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarObjectType::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getObjectTypeName(mData));
}
#endif


//==============================================================================
// BTriggerVarProtoSquad::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarProtoSquad::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getProtoSquadName(mData));
}
#endif


//==============================================================================
// BTriggerVarUILocation::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarUILocation::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mResult == cUILocationResultOK)
      varDataAsString.format("ResultOK - Location(%f, %f, %f)", mLocation.x, mLocation.y, mLocation.z);
   else if (mResult == cUILocationResultCancel)
      varDataAsString = "ResultCancel - Location(INVALID)";
   else if (mResult == cUILocationResultUILockError)
      varDataAsString = "ResultUILockError - Location(INVALID)";
   else
      varDataAsString = "ResultWaiting - Location(INVALID)";
}
#endif


//==============================================================================
// BTriggerVarCost::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarCost::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString = "";
   long numResources = mData.getNumberResources();
   for (long i=0; i<numResources; i++)
   {
      BSimString appendStr;
      appendStr.format("%s: %f, ", gDatabase.getResourceName(i), mData.get(i));
      varDataAsString.append(appendStr);
   }
}
#endif


//==============================================================================
// BTriggerVarUnit::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarUnit::getVariableDataAsString(BSimString& varDataAsString)
{
   BUnit *pUnit = gWorld->getUnit(mData);
   if (!pUnit)
   {
      varDataAsString = "(NULL)";
   }
   else
   {
      const BSimString& protoName = pUnit->getProtoObject()->getName();
      long unitIDAsLong = pUnit->getID().asLong();
      long unitPlayerID = pUnit->getPlayerID();
      varDataAsString.format("ID=%d, Player=%d, ProtoObjectType=%sS", unitIDAsLong, unitPlayerID, protoName.getPtr());
   }
}
#endif


//==============================================================================
// BTriggerVarSquad::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarSquad::getVariableDataAsString(BSimString& varDataAsString)
{
   BSquad *pSquad = gWorld->getSquad(mData);
   if (!pSquad)
   {
      varDataAsString = "(NULL)";
   }
   else
   {
      const BProtoSquad *pProtoSquad = pSquad->getProtoSquad();
      long squadIDAsLong = pSquad->getID().asLong();
      long squadPlayerID = pSquad->getPlayerID();
      BSimString protoSquadName = pProtoSquad ? pProtoSquad->getName() : "(NO PROTOSQUAD)";
      varDataAsString.format("ID=%d, Player=%d, ProtoSquadType=%s", squadIDAsLong, squadPlayerID, protoSquadName.getPtr());
   }
}
#endif


//==============================================================================
// BTriggerVarUIUnit::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarUIUnit::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mResult == cUIUnitResultOK)
   {
      BUnit *pUnit = gWorld->getUnit(mUnit);
      if (!pUnit)
      {
         varDataAsString = "ResultOK - Unit: INVALID";
      }
      else
      {
         const BSimString& protoName = pUnit->getProtoObject()->getName();
         long unitIDAsLong = pUnit->getID().asLong();
         long unitPlayerID = pUnit->getPlayerID();
         varDataAsString.format("ResultOK - Unit: ID=%d, Player=%d, ProtoObjectType=%s", unitIDAsLong, unitPlayerID, protoName.getPtr());
      }
   }
   else if (mResult == cUIUnitResultCancel)
      varDataAsString = "ResultCancel - Unit: INVALID";
   else if (mResult == cUIUnitResultUILockError)
      varDataAsString = "ResultUILockError - Unit: INVALID";
   else
      varDataAsString = "ResultWaiting - Unit: INVALID";
}
#endif


//==============================================================================
// BTriggerVarUISquad::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarUISquad::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mResult == cUISquadResultOK)
   {
      BSquad *pSquad = gWorld->getSquad(mSquad);
      if (!pSquad)
      {
         varDataAsString = "ResultOK - Squad: INVALID";
      }
      else
      {
         const BProtoSquad *pProtoSquad = pSquad->getProtoSquad();
         long squadIDAsLong = pSquad->getID().asLong();
         long squadPlayerID = pSquad->getPlayerID();
         BSimString protoSquadName = pProtoSquad ? pProtoSquad->getName() : "(NO PROTOSQUAD)";
         varDataAsString.format("ResultOK - Squad: ID=%d, Player=%d, ProtoSquadType=%s", squadIDAsLong, squadPlayerID, protoSquadName.getPtr());
      }
   }
   else if (mResult == cUISquadResultCancel)
      varDataAsString = "ResultCancel - Squad: INVALID";
   else if (mResult == cUISquadResultUILockError)
      varDataAsString = "ResultUILockError - Squad: INVALID";
   else
      varDataAsString = "ResultWaiting - Squad: INVALID";
}
#endif

//==============================================================================
// BTriggerVarCiv::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarCiv::getVariableDataAsString(BSimString& varDataAsString)
{
   BCiv *pCiv = gDatabase.getCiv(mData);
   if (pCiv)
      varDataAsString = pCiv->getCivName();
   else
      varDataAsString = "INVALID";
}
#endif


//==============================================================================
// BTriggerVarObject::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarObject::getVariableDataAsString(BSimString& varDataAsString)
{
   BObject *pObject = gWorld->getObject(mData);
   if (!pObject)
   {
      varDataAsString = "(NULL)";
   }
   else
   {
      const BSimString& protoName = pObject->getProtoObject()->getName();
      long objectIDAsLong = pObject->getID().asLong();
      long objectPlayerID = pObject->getPlayerID();
      varDataAsString.format("ID=%d, Player=%d, ProtoObjectType=%s", objectIDAsLong, objectPlayerID, protoName.getPtr());
   }
}
#endif


//==============================================================================
// BTriggerVarRefCountType::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarRefCountType::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getRefCountTypeName(mData));
}
#endif


//==============================================================================
// BTriggerVarUnitFlag::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarUnitFlag::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getUnitFlagName(mData));
}
#endif

//==============================================================================
// BTriggerVarSquadFlag::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarSquadFlag::getVariableDataAsString(BSimString &varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getSquadFlagName(mData));
}
#endif


//==============================================================================
// BTriggerVarLOSType::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarLOSType::getVariableDataAsString(BSimString &varDataAsString)
{
   switch(mData)
   {
   case BWorld::cCPLOSNormal:
      {
         varDataAsString = "LOSNormal";
         break;
      }
   case BWorld::cCPLOSFullVisible:
      {
         varDataAsString = "LOSFullVisible";
         break;
      }
   case BWorld::cCPLOSDontCare:
   default:
      {
         varDataAsString = "LOSDontCare";
         break;
      }
   }
}
#endif

//==============================================================================
// BTriggerVarPopBucket::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarPopBucket::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString = gDatabase.getPopName(mData);
}
#endif

//==============================================================================
// BTriggerVarListPosition::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarListPosition::getVariableDataAsString(BSimString& varDataAsString)
{
   switch(mData)
   {
      case cListPosTypeFirst:
         varDataAsString = "First";
         break;

      case cListPosTypeLast:
         varDataAsString = "Last";
         break;

      case cListPosTypeRandom:
         varDataAsString = "Random";
         break;
   }
}
#endif

//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarRelationType::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString = gDatabase.getRelationTypeName(mData);
}
#endif

//==============================================================================
// Convert exposed action enumerations to strings for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarExposedAction::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%d", mData);
}
#endif

//==============================================================================
// Convert squad mode enumerations to strings for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarSquadMode::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString = gDatabase.getSquadModeName(mData);
}
#endif

//==============================================================================
// Convert exposed script to string for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarExposedScript::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%d", mData);
}
#endif

//==============================================================================
// Convert scalar data to string for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarDataScalar::getVariableDataAsString(BSimString& varDataAsString)
{
   switch (mData)
   {
      case cDataScalarAccuracy:
         varDataAsString = "Accuracy";
         break;

      case cDataScalarWorkRate:
         varDataAsString = "WorkRate";
         break;

      case cDataScalarDamage:
         varDataAsString = "Damage";
         break;

      case cDataScalarDamageTaken:
         varDataAsString = "DamageTaken";
         break;

      case cDataScalarLOS:
         varDataAsString = "LOS";
         break;

      case cDataScalarVelocity:
         varDataAsString = "Velocity";
         break;

      case cDataScalarWeaponRange:
         varDataAsString = "WeaponRange";
         break;
   }
}
#endif

//==============================================================================
// Convert unit list to string for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarUnitList::getVariableDataAsString(BSimString& varDataAsString)
{   
   uint numUnits = (uint)mData.getNumber();
   varDataAsString.format( "%d: ", numUnits);
   for (uint i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 2546
      const BUnit* pUnit = gWorld->getUnit(mData[i]);
//--
      if (pUnit)
      {
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (pProtoObject && (i < 7))
         {
            BSimString thisString = varDataAsString;
            varDataAsString.format("%s%s,\n", thisString.getPtr(), pProtoObject->getName().getPtr());
         }
      }
   }
}
#endif

//==============================================================================
// Convert squad list to string for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarSquadList::getVariableDataAsString(BSimString& varDataAsString)
{   
   uint numSquads = (uint)mData.getNumber();
   varDataAsString.format( "%d: ", numSquads);
   for (uint i = 0; i < numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 2547
      const BSquad* pSquad = gWorld->getSquad(mData[i]);
//--
      if (pSquad)
      {
         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (pProtoObject && (i < 7))
         {
            BSimString thisString = varDataAsString;
            varDataAsString.format("%s%s,\n", thisString.getPtr(), pProtoObject->getName().getPtr());
         }
      }
   }
}
#endif

//==============================================================================
//==============================================================================
void BTriggerVarUnitList::addUnit(BEntityID unitID)
{
   BTRIGGER_ASSERTM(gWorld->getUnit(unitID), "Adding a unitID to a TriggerVarUnitList that is not valid!  This should never happen!");
   syncTriggerVar(unitID); 
   mData.uniqueAdd(unitID);
   initialize();
}

//==============================================================================
// BTriggerVarHUDItem::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarHUDItem::getVariableDataAsString(BSimString& varDataAsString)
{
   varDataAsString.format("%s", gDatabase.getHUDItemName(mData));
}
#endif

//==============================================================================
// BTriggerVarControlType::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
extern const BCHAR_T* gControlNames[];
void BTriggerVarControlType::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mData==-1)
      varDataAsString.format("INVALID");
   else
      varDataAsString.format("%s", gControlNames[mData]);
}
#endif

//==============================================================================
// BTriggerVarUIButton::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarUIButton::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mResult == cUIButtonResultWaiting)
      varDataAsString = "ResultSelected";
   else
      varDataAsString = "ResultWaiting";
}
#endif

//==============================================================================
// BTriggerVarBuildingCommandState::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarBuildingCommandState::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mResult == cResultWaiting)
      varDataAsString = "ResultWaiting";
   else
      varDataAsString = "ResultDone";
}
#endif

//==============================================================================
// BTriggerVarChatSpeaker::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarChatSpeaker::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mData == BChatMessage::cChatSpeakerSerena)
      varDataAsString = "Serena";
   else if (mData == BChatMessage::cChatSpeakerForge)
      varDataAsString = "Forge";
   else if (mData == BChatMessage::cChatSpeakerCutter)
      varDataAsString = "Cutter";
   else if (mData == BChatMessage::cChatSpeakerGod)
      varDataAsString = "Voice of God";
   else if (mData == BChatMessage::cChatSpeakerSoldiers)
      varDataAsString = "Generic Soldiers";
   else if (mData == BChatMessage::cChatSpeakerPolice)
      varDataAsString = "Arcadian Police";
   else if (mData == BChatMessage::cChatSpeakerCivilians)
      varDataAsString = "Civilians";
   else if (mData == BChatMessage::cChatSpeakerAnders)
      varDataAsString = "Anders";
   else if (mData == BChatMessage::cChatSpeakerRhinoCommander)
      varDataAsString = "Rhino Commander";
   else if (mData == BChatMessage::cChatSpeakerSpartan1)
      varDataAsString = "Spartan 1";
   else if (mData == BChatMessage::cChatSpeakerSpartan2)
      varDataAsString = "Spartan 2";
   else if (mData == BChatMessage::cChatSpeakerSpartanSniper)
      varDataAsString = "Spartan Sniper";
   else if (mData == BChatMessage::cChatSpeakerSpartanRocketLauncher)
      varDataAsString = "Spartan Rocket Launcher";
   else if (mData == BChatMessage::cChatSpeakerCovenant)
      varDataAsString = "Covenant";
   else if (mData == BChatMessage::cChatSpeakerArbiter)
      varDataAsString = "Arbiter";
   else if (mData == BChatMessage::cChatSpeakerODST)
      varDataAsString = "ODST";

}
#endif

//==============================================================================
// BTriggerVarRumbleType::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarRumbleType::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mData==-1)
      varDataAsString.format("INVALID");
   else
      varDataAsString.format("%s", BGamepad::getRumbleTypeName(mData));
}
#endif

//==============================================================================
// BTriggerVarRumbleMotor::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarRumbleMotor::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mData==-1)
      varDataAsString.format("INVALID");
   else
      varDataAsString.format("%s", BGamepad::getRumbleMotorName(mData));
}
#endif

//==============================================================================
// BTriggerVarEventType::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarEventType::getVariableDataAsString(BSimString& varDataAsString)
{   
   BEventDefinitions::getVariableDataAsString(mData, varDataAsString);
}
#endif

//==============================================================================
// BTriggerVarGameStatePredicate::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarGameStatePredicate::getVariableDataAsString(BSimString& varDataAsString)
{   
   BGameStateDefinitions::getVariableDataAsString(mData, varDataAsString);
}
#endif

//==============================================================================
// BTriggerVarUILocationMinigame::getVariableDataAsString
//==============================================================================
#ifndef BUILD_FINAL
void BTriggerVarUILocationMinigame::getVariableDataAsString(BSimString& varDataAsString)
{
   if (mResult == cUILocationResultOK)
      varDataAsString.format("ResultOK - Location(%f, %f, %f) - Data(%f)", mLocation.x, mLocation.y, mLocation.z, mData);
   else if (mResult == cUILocationResultCancel)
      varDataAsString = "ResultCancel - Location(INVALID) - Data(INVALID)";
   else if (mResult == cUILocationResultUILockError)
      varDataAsString = "ResultUILockError - Location(INVALID) - Data(INVALID)";
   else
      varDataAsString = "ResultWaiting - Location(INVALID) - Data(INVALID)";
}
#endif

////==============================================================================
//// BTriggerVarConcept::getVariableDataAsString
////==============================================================================
//#ifndef BUILD_FINAL
//void BTriggerVarConcept::getVariableDataAsString(BSimString& varDataAsString)
//{
//   varDataAsString.format("NOTIMPL");
//}
//#endif
//
////==============================================================================
//// BTriggerVarConceptList::getVariableDataAsString
////==============================================================================
//#ifndef BUILD_FINAL
//void BTriggerVarConceptList::getVariableDataAsString(BSimString& varDataAsString)
//{
//   varDataAsString.format("NOTIMPL");
//}
//#endif

// NEWTRIGGERVARTYPE
// Add getVariableDataAsString function definition.  See above functions for examples.

//==============================================================================
// BTriggerVar::checkInitialized()
//==============================================================================
void BTriggerVar::checkInitialized() const
{
#ifndef BUILD_FINAL
   if (!mbInitialized)
   {
      BSimString debugDataString;
      BSimString varName = getName();
      if (varName.isEmpty())
         varName = "(NONE)";
      debugDataString.format("TRIGGER VARIABLE NOT INITIALIZED: ID = %d, Name = %s is not initialized!", getID(), varName.getPtr());
      BTRIGGER_ASSERTM(false, debugDataString.getPtr());
   }
#endif
}


//==============================================================================
// BTriggerVar::initialize()
//==============================================================================
void BTriggerVar::initialize()
{
#ifndef BUILD_FINAL
   mbInitialized = true;
#endif
}


//==============================================================================
// BTriggerVarEntityList::shuffle
//==============================================================================
void BTriggerVarEntityList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}


//==============================================================================
// BTriggerVarPlayerList::shuffle
//==============================================================================
void BTriggerVarPlayerList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}

//==============================================================================
// Convert player list to string for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarPlayerList::getVariableDataAsString(BSimString& varDataAsString)
{   
   uint numPlayers = mData.getSize();
   varDataAsString.format( "%d: ", numPlayers);
   for (uint i = 0; i < numPlayers; i++)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(mData[i]);
      if (pPlayer)
      {
         BSimString thisString = varDataAsString;
         varDataAsString.format("%s-%s-%d,\n", thisString.getPtr(), pPlayer->getName().getPtr(), pPlayer->getID());
      }
   }
}
#endif

//==============================================================================
// BTriggerVarTeamList::shuffle
//==============================================================================
void BTriggerVarTeamList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}


//==============================================================================
// BTriggerVarUnitList::shuffle
//==============================================================================
void BTriggerVarUnitList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}


//==============================================================================
// BTriggerVarSquadList::shuffle
//==============================================================================
void BTriggerVarSquadList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}

//==============================================================================
//==============================================================================
void BTriggerVarSquadList::addSquad(BEntityID squadID)
{
   BTRIGGER_ASSERTM(gWorld->getSquad(squadID), "Adding a squadID to a TriggerVarSquadList that is not valid!  This should never happen!");
   syncTriggerVar(squadID); 
   mData.uniqueAdd(squadID);
   initialize();
}

//==============================================================================
//==============================================================================
void BTriggerVarObjectList::addObject(BEntityID objectID)
{
   BTRIGGER_ASSERTM(gWorld->getObject(objectID), "Adding an objectID to a TriggerVarObjectList that is not valid!  This should never happen!");
   mData.uniqueAdd(objectID);
   initialize();
}


//==============================================================================
// BTriggerVarProtoObjectList::shuffle
//==============================================================================
void BTriggerVarProtoObjectList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}


//==============================================================================
// BTriggerVarObjectTypeList::shuffle
//==============================================================================
void BTriggerVarObjectTypeList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}


//==============================================================================
// BTriggerVarProtoSquadList::shuffle
//==============================================================================
void BTriggerVarProtoSquadList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}

//==============================================================================
// Convert proto squad list to string for debug purposes
//==============================================================================
#if !defined(BUILD_FINAL)
void BTriggerVarProtoSquadList::getVariableDataAsString(BSimString& varDataAsString)
{   
   uint numProtoSquads = (uint)mData.getNumber();
   varDataAsString.format( "%d: ", numProtoSquads);
   for (uint i = 0; i < numProtoSquads; i++)
   {
      //-- FIXING PREFIX BUG ID 2547
      const BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(mData[i]);
      //--
      if (pProtoSquad && (i < 7))
      {                  
         BSimString thisString = varDataAsString;
         varDataAsString.format("%s%s,\n", thisString.getPtr(), pProtoSquad->getName().getPtr());
      }
   }
}
#endif

//==============================================================================
// BTriggerVarTechList::shuffle
//==============================================================================
void BTriggerVarTechList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}


//==============================================================================
// BTriggerVarObjectList::shuffle
//==============================================================================
void BTriggerVarObjectList::shuffle()
{
   long numEntries = mData.getNumber();
   if (numEntries <= 1)
      return;
   long maxIndex = numEntries-1;
   for (long i=0; i<numEntries; i++)
   {
      long randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}


//==============================================================================
//==============================================================================
void BTriggerVarVectorList::shuffle()
{
   uint numEntries = mData.getSize();
   if (numEntries <= 1)
      return;
   uint maxIndex = numEntries-1;
   for (uint i=0; i<numEntries; i++)
   {
      uint randomIndex = getRandRange(cSimRand, 0, maxIndex);
      mData.swapIndices(i, randomIndex);
   }
}

