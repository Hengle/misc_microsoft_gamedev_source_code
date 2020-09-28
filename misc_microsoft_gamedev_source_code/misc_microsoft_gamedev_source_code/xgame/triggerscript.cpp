//==============================================================================
// triggerscript.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "database.h"
#include "entity.h"
#include "generaleventmanager.h"
#include "scenario.h"
#include "syncmacros.h"
#include "trigger.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"

// xinputsystem
#include "inputcontrolenum.h"

// xsystem
#include "xmlreader.h"

// xvisual
#include "protovisual.h"
#include "soundmanager.h"

GFIMPLEMENTVERSION(BTriggerScript, 2);
GFIMPLEMENTVERSION(BTriggerScriptExternals, 1);
GFIMPLEMENTVERSION(BTriggerGroup, 1);

IMPLEMENT_FREELIST(BTriggerScript, 4, &gSimHeap);

//==============================================================================
//==============================================================================
void BTriggerScript::onAcquire()
{
   mID = cInvalidTriggerScriptID;
   mFileDir = -1;
   mFileName.empty();
   uint index = UINT_MAX;
   BTriggerScriptExternals* pExternals = gTriggerManager.acquireTriggerScriptExternal(index);
   BFATAL_ASSERT(index <= UINT16_MAX);
   mExternalsIndex = static_cast<uint16>(index);
   pExternals->clearAll();
#ifndef BUILD_FINAL
   mName = "";
   mType = BTriggerScript::cTypeInvalid;
#endif
   mbActive = false;
   mbUpdating = false;
   mbCleanup = false;
   mbLogEffects = false;
   mFlagPaused = false;
   mPlayerID = -1;

   mAILogFilterByGroup = false;
   mAILogGroup = cInvalidTriggerGroupID;  
}


//==============================================================================
//==============================================================================
void BTriggerScript::onRelease()
{
   // Unlock any users that are locked by this script.
   long numUsers = gUserManager.getUserCount();
   for (long i=0; i<numUsers; i++)
   {
      BTriggerScriptID scriptID = this->getID();
      BUser *pUser = gUserManager.getUser(i);
      if (pUser && pUser->isUserLockedByTriggerScript(scriptID))
         pUser->unlockUser();
   }

   // Release all our triggers.
   releaseTriggers();

   // Release all our trigger vars.
   releaseTriggerVars();

   releaseExternals();

   mTriggerVarUnitLists.clear();
   mTriggerVarSquadLists.clear();
   mTriggerVarObjectLists.clear();
   mInvalidatedEntityIDs.clear();
   mActiveTriggers.clear();
   mTriggerEvalList.clear();
}

//==============================================================================
//==============================================================================
void BTriggerScript::releaseTriggers()
{
   uint numTriggers = mTriggers.getSize();
   for (uint i=0; i<numTriggers; i++)
      BTrigger::releaseInstance(mTriggers[i]);
   mTriggers.clear();
}

//==============================================================================
//==============================================================================
void BTriggerScript::releaseTriggerVars()
{
   uint numTriggerVars = mTriggerVars.getSize();
   for (uint i=0; i<numTriggerVars; i++)
   {
      if (mTriggerVars[i])
         releaseTriggerVarInstance(mTriggerVars[i]);
   }
   mTriggerVars.clear();
   mTriggerVarUnitLists.clear();
   mTriggerVarSquadLists.clear();
   mTriggerVarObjectLists.clear();
}

//==============================================================================
//==============================================================================
void BTriggerScript::releaseExternals()
{
   BTriggerScriptExternals* pExternals = gTriggerManager.getTriggerScriptExternal(mExternalsIndex);
   if (pExternals)
      pExternals->clearAll();
   gTriggerManager.releaseTriggerScriptExternal(mExternalsIndex);
   mExternalsIndex = UINT16_MAX;
}

//==============================================================================
// BTriggerScript::activate
//==============================================================================
void BTriggerScript::activate()
{
   mbActive = true;

   uint numTriggers = mTriggers.getSize();
   for (uint i=0; i<numTriggers; i++)
   {
      BTrigger*  pTrigger = mTriggers[i];
      BASSERT(pTrigger);
      if (pTrigger->isStartActive())
         activateTrigger(pTrigger->getID());
   }
}

//==============================================================================
// BTriggerScript::activateTrigger
//==============================================================================
void BTriggerScript::activateTrigger(BTriggerID triggerID)
{
   BTrigger*  pTrigger = getTrigger(triggerID);
   BASSERT(pTrigger);
   pTrigger->onActivated();
   mActiveTriggers.uniqueAdd(triggerID);
   mTriggerEvalList.uniqueAdd(triggerID);
}


//==============================================================================
// BTriggerScript::deactivateTrigger
//==============================================================================
void BTriggerScript::deactivateTrigger(BTriggerID triggerID)
{
   mActiveTriggers.removeValueAllInstances(triggerID);
   mTriggerEvalList.removeValueAllInstances(triggerID);
}


//==============================================================================
//==============================================================================
void BTriggerScript::invalidateEntityID(BEntityID entityID)
{
   // Don't bother with inactive trigger scripts.
   if (!isActive())
      return;

   // If we're in the middle of updating the script, purge the invalidated handle immediately.
   if (mbUpdating)
   {
      uint entityType = entityID.getType();
      if (entityType == BEntity::cClassTypeUnit)
      {
         uint numUnitListVars = mTriggerVarUnitLists.getSize();
         for (uint i=0; i<numUnitListVars; i++)
         {
            if (mTriggerVarUnitLists[i]->isUsed())
               mTriggerVarUnitLists[i]->removeUnit(entityID);
         }
      }
      else if (entityType == BEntity::cClassTypeSquad)
      {
         uint numSquadListVars = mTriggerVarSquadLists.getSize();
         for (uint i=0; i<numSquadListVars; i++)
         {
            if (mTriggerVarSquadLists[i]->isUsed())
               mTriggerVarSquadLists[i]->removeSquad(entityID);
         }
      }
      else if (entityType == BEntity::cClassTypeObject)
      {
         uint numObjectListVars = mTriggerVarObjectLists.getSize();
         for (uint i=0; i<numObjectListVars; i++)
         {
            if (mTriggerVarObjectLists[i]->isUsed())
               mTriggerVarObjectLists[i]->removeObject(entityID);
         }
      }
   }
   // Otherwise, cache it off and purge it when we go to update.
   else
   {
      mInvalidatedEntityIDs.add(entityID);
   }
}


//==============================================================================
//==============================================================================
BTriggerScriptExternals* BTriggerScript::getExternals()
{
   return (gTriggerManager.getTriggerScriptExternal(mExternalsIndex));
}


//==============================================================================
// BTriggerScript::loadFromXML
//==============================================================================
bool BTriggerScript::loadFromXML(BXMLNode triggerScriptNode)
{
   DWORD numTriggerVars = 0;
   if (!triggerScriptNode.getAttribValueAsDWORD("NextTriggerVarID", numTriggerVars))
   {
      return (false);
   }

#ifndef BUILD_FINAL
   if (!triggerScriptNode.getAttribValueAsString("Name", mName))
      mName = "";
   BSimString triggerScriptType;
   if (triggerScriptNode.getAttribValueAsString("Type", triggerScriptType))
      mType = BTriggerScript::getTriggerScriptTypeEnum(triggerScriptType);
   else
      mType = BTriggerScript::cTypeInvalid;
#endif


   BXMLNode triggerVarsNode(triggerScriptNode.getChildNode("TriggerVars"));
   if (!triggerVarsNode)
   {
      return (false);
   }
   if (!loadTriggerVarsFromXML(triggerVarsNode, static_cast<uint>(numTriggerVars)))
   {
      return (false);
   }

   BXMLNode triggersNode(triggerScriptNode.getChildNode("Triggers"));
   if (!triggersNode)
   {
      return (false);
   }
   if (!loadTriggersFromXML(triggersNode))
   {
      return (false);
   }

   return (true);
}


//==============================================================================
// BTriggerScript::loadTriggersFromXML
//==============================================================================
bool BTriggerScript::loadTriggersFromXML(BXMLNode triggersNode)
{
   BEditorIDToIDMap editorIDToIDMap;

   uint numTriggerNodes = static_cast<uint>(triggersNode.getNumberChildren());
   for (uint i=0; i<numTriggerNodes; i++)
   {
      BXMLNode triggerNode(triggersNode.getChild(i));
      if (triggerNode.getName() != "Trigger")
         return (false);
      BTrigger *pTrigger = BTrigger::getInstance();
      pTrigger->setParentTriggerScript(this);
      pTrigger->setID(mTriggers.getSize());
      mTriggers.add(pTrigger);
      if (!pTrigger->loadFromXML(triggerNode))
         return (false);

      // Add to our map of EditorIDs -> IDs
      BTriggerID triggerEditorID = pTrigger->getEditorID();
      BTriggerID triggerID = pTrigger->getID();
      editorIDToIDMap.insert(triggerEditorID, triggerID);
   }

   // Remap all references to EditorID's to the actual index ID's here using the hash map.
   if (!remapTriggerEditorIDsToIDs(editorIDToIDMap))
      return (false);

   return (true);
}


//==============================================================================
//==============================================================================
bool BTriggerScript::remapTriggerEditorIDsToIDs(const BEditorIDToIDMap& editorIDToIDMap)
{
   uint numTriggerVars = mTriggerVars.getSize();
   for (uint i=0; i<numTriggerVars; i++)
   {
      BTriggerVar* pVar = mTriggerVars[i];
      if (pVar && pVar->isType(BTriggerVar::cVarTypeTrigger))
      {
         BTriggerID editorID = pVar->asTrigger()->readVar();
         BEditorIDToIDMap::const_iterator it = editorIDToIDMap.find(editorID);
         if (it != editorIDToIDMap.end())
         {
            pVar->asTrigger()->writeVar(it->second);
         }
      }
   }

   return (true);
}



//==============================================================================
// THIS IS A HACK.
// Just temporary until all the trigger scripts have been updated.
//==============================================================================
bool BTriggerScript::HACK_doesStringContainOnlyNumbers(const BSimString& str) const
{
   long length = str.length();
   const BCHAR_T* pStrPtr = str.getPtr();
   BASSERTM(length > 0, "HACK_doesStringContainOnlyNumbers passed in a string with zero length.  Unexpected results may occur.");
   BASSERTM(pStrPtr, "HACK_doesStringContainOnlyNumbers passed in a crap string.");
   if (length == 0)
      return (false);
   if (!pStrPtr)
      return (false);

   for (long i=0; i<length; i++)
   {
      if (pStrPtr[i] < '0' || pStrPtr[i] > '9')
         return (false);
   }

   return (true);
}


//==============================================================================
// BTriggerScript::writeVar
//==============================================================================
bool BTriggerScript::writeVar(BTriggerVar *pVar, BTriggerVarType varTypeEnum, BXMLNode varNode, uint &numVarsRequiringAutoFixUp)
{
      BSimString tempStr;
      const BSimString& varNodeText = varNode.getText(tempStr);

      switch (varTypeEnum)
      {
      case BTriggerVar::cVarTypeTech:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BProtoTechID techID = gDatabase.getProtoTech(varNodeText);
               #ifndef BUILD_FINAL
               if (techID == cInvalidTechID)
               {
                  BSimString errorString;
                  errorString.format("Script %s, Variable ID=%d, Name=%s, contains tech %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), varNodeText.getPtr());
                  BFAIL(errorString.getPtr());
               }
               #endif
               pVar->asTech()->writeVar(techID);
            }
            break;
         }
      case BTriggerVar::cVarTypeTechStatus:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (HACK_doesStringContainOnlyNumbers(varNodeText))
               {
                  #ifndef BUILD_FINAL
                  numVarsRequiringAutoFixUp++;
                  #endif
                  pVar->asTechStatus()->writeVar(varNodeText.asLong());
               }
               else
                  pVar->asTechStatus()->writeVar(gDatabase.getTechStatusFromName(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeOperator:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               int varNodeEnum = gTriggerManager.getTriggerOperatorTypeEnum(varNodeText);
               pVar->asOperator()->writeVar(varNodeEnum);               
            }
            break;
         }
      case BTriggerVar::cVarTypeProtoObject:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BProtoObjectID protoObjectID = gDatabase.getProtoObject(varNodeText);
               #ifndef BUILD_FINAL
               if (protoObjectID == cInvalidProtoObjectID)
               {
                  BSimString errorString;
                  errorString.format("Script %s, Variable ID=%d, Name=%s, contains protoobject %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), varNodeText.getPtr());
                  BFAIL(errorString.getPtr());
               }
               #endif
               pVar->asProtoObject()->writeVar(protoObjectID);
            }
            break;
         }
      case BTriggerVar::cVarTypeProtoSquad:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BProtoSquadID protoSquadID = gDatabase.getProtoSquad(varNodeText);
               #ifndef BUILD_FINAL
               if (protoSquadID == cInvalidProtoSquadID)
               {
                  BSimString errorString;
                  errorString.format("Script %s, Variable ID=%d, Name=%s, contains protosquad %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), varNodeText.getPtr());
                  BFAIL(errorString.getPtr());
               }
               #endif
               pVar->asProtoSquad()->writeVar(protoSquadID);
            }
            break;
         }
      case BTriggerVar::cVarTypeSound:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asSound()->writeVar(varNodeText);
            break;
         }
      case BTriggerVar::cVarTypeEntity:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asEntity()->writeVar(gScenario.getEntityIDFromScenarioObjectID(varNodeText.asLong()));
            break;
         }
      case BTriggerVar::cVarTypeEntityList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asEntityList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BEntityIDArray entityList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BEntityID entityID = gScenario.getEntityIDFromScenarioObjectID(token.asLong());
                  BEntity *pEntity = gWorld->getEntity(entityID);
                  if (pEntity)
                  {
                     entityList.uniqueAdd(entityID);
                  }
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asEntityList()->writeVar(entityList);            
            }
            break;
         }
      case BTriggerVar::cVarTypeTrigger:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asTrigger()->writeVar(varNodeText.asLong());
            break;
         }
      case BTriggerVar::cVarTypeTime:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asTime()->writeVar((DWORD)varNodeText.asLong());
            break;
         }
      case BTriggerVar::cVarTypePlayer:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asPlayer()->writeVar(gScenario.getPlayerIDFromScenarioPlayerID(varNodeText.asLong()));
            break;
         }
      case BTriggerVar::cVarTypeTeam:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asTeam()->writeVar(varNodeText.asLong());
            break;
         }
      case BTriggerVar::cVarTypeCost:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BCost costVal;
               BEntityIDArray entityList;
               BSimString costToken;
               BSimString resourceIDToken;
               BSimString resourceAmountToken;
               long strLen = varNodeText.length();
               long loc = costToken.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  long costTokenLen = costToken.length();
                  long equalsLoc = resourceIDToken.copyTok(costToken, costTokenLen, -1, B("="));
                  BASSERT(equalsLoc >= 0);
                  resourceAmountToken.copyTok(costToken, costTokenLen, equalsLoc+1);

                  long resourceID = resourceIDToken.asLong();
                  float resourceAmount = resourceAmountToken.asFloat();
                  if (BCost::isValidResourceID(resourceID))
                     costVal.set(resourceID, resourceAmount);

                  loc = costToken.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asCost()->writeVar(costVal);
            }
            // Hmm figure out the format for initializing cost variables.
            break;
         }
      case BTriggerVar::cVarTypeAnimType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (HACK_doesStringContainOnlyNumbers(varNodeText))
               {
                  #ifndef BUILD_FINAL
                  numVarsRequiringAutoFixUp++;
                  #endif
                  pVar->asAnimType()->writeVar(varNodeText.asLong());
               }
               else
                  pVar->asAnimType()->writeVar(gDatabase.getAnimTypeFromName(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeObjectType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BObjectTypeID objectTypeID = gDatabase.getObjectType(varNodeText);
               #ifndef BUILD_FINAL
               if (objectTypeID == cInvalidObjectTypeID)
               {
                  BSimString errorString;
                  errorString.format("Script %s, Variable ID=%d, Name=%s, contains objecttype %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), varNodeText.getPtr());
                  BFAIL(errorString.getPtr());
               }
               #endif
               pVar->asObjectType()->writeVar(objectTypeID);
            }
            break;
         }
      case BTriggerVar::cVarTypePower:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BProtoPowerID protoPowerID = gDatabase.getProtoPowerIDByName(varNodeText);
               #ifndef BUILD_FINAL
               if (protoPowerID == cInvalidProtoPowerID)
               {
                  BSimString errorString;
                  errorString.format("Script %s, Variable ID=%d, Name=%s, contains protopower %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), varNodeText.getPtr());
                  BFAIL(errorString.getPtr());
               }
               #endif
               pVar->asPower()->writeVar(protoPowerID);
            }
            break;
         }
      case BTriggerVar::cVarTypeBool:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BASSERT(varNodeText == "True" || varNodeText == "False");
               if (varNodeText == "True")
                  pVar->asBool()->writeVar(true);
               else if (varNodeText == "False")
                  pVar->asBool()->writeVar(false);
            }
            break;
         }
      case BTriggerVar::cVarTypeFloat:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asFloat()->writeVar(varNodeText.asFloat());
            break;
         }
      case BTriggerVar::cVarTypeActionStatus:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (HACK_doesStringContainOnlyNumbers(varNodeText))
               {
                  #ifndef BUILD_FINAL
                  numVarsRequiringAutoFixUp++;
                  #endif
                  pVar->asActionStatus()->writeVar(varNodeText.asLong());
               }
               else
                  pVar->asActionStatus()->writeVar(gDatabase.getActionStatusFromName(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypePlayerList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asPlayerList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BPlayerIDArray playerList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  playerList.uniqueAdd(gScenario.getPlayerIDFromScenarioPlayerID(token.asLong()));
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asPlayerList()->writeVar(playerList);
            }
            break;
         }
      case BTriggerVar::cVarTypeTeamList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asTeamList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BTeamIDArray teamList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  teamList.uniqueAdd(token.asLong());
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asTeamList()->writeVar(teamList);
            }
            break;
         }
      case BTriggerVar::cVarTypePlayerState:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asPlayerState()->writeVar(gDatabase.getPlayerState(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeObjective:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asObjective()->writeVar(varNodeText.asLong());
            break;
         }
      case BTriggerVar::cVarTypeObject:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BEntityID objectID = gScenario.getEntityIDFromScenarioObjectID(varNodeText.asLong());
               BObject *pObject = gWorld->getObject(objectID);

               //uint type = objectID.getType();
               BSquad* pSquad = NULL;
               BEntity* pEntity = NULL;
               if (!pObject)
               {
                  pObject = gWorld->getUnit(objectID);
               }
               if (!pObject)
               {
                  pSquad = gWorld->getSquad(objectID);
                  if(pSquad)
                     pObject = pSquad->getObject();
               }
               if (!pObject)
               {
                  pEntity = gWorld->getEntity(objectID);
                  if(pEntity)
                     pObject = pEntity->getObject();
               }

               if (pObject)
                  pVar->asObject()->writeVar(objectID);
            }
            break;
         }
      case BTriggerVar::cVarTypeObjectList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asObjectList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BEntityID objectID = gScenario.getEntityIDFromScenarioObjectID(token.asLong());
                  BObject *pObject = gWorld->getObject(objectID);
                  if (pObject)
                     pVar->asObjectList()->addObject(objectID);
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
            }
            break;
         }
      case BTriggerVar::cVarTypeUnit:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BEntityID squadID = gScenario.getEntityIDFromScenarioObjectID(varNodeText.asLong());
               BSquad *pSquad = gWorld->getSquad(squadID);
               if (pSquad && pSquad->getNumberChildren() == 1)
                  pVar->asUnit()->writeVar(pSquad->getLeader());
            }
            break;
         }
      case BTriggerVar::cVarTypeUnitList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asUnitList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BEntityID squadID = gScenario.getEntityIDFromScenarioObjectID(token.asLong());
                  BSquad *pSquad = gWorld->getSquad(squadID);
                  if (pSquad && pSquad->getNumberChildren() == 1)
                     pVar->asUnitList()->addUnit(pSquad->getLeader());
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
            }
            break;
         }
      case BTriggerVar::cVarTypeSquad:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asSquad()->writeVar(gScenario.getEntityIDFromScenarioObjectID(varNodeText.asLong()));
            }
            break;
         }
      case BTriggerVar::cVarTypeSquadList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asSquadList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BEntityID squadID = gScenario.getEntityIDFromScenarioObjectID(token.asLong());
                  BSquad *pSquad = gWorld->getSquad(squadID);
                  if (pSquad)
                     pVar->asSquadList()->addSquad(squadID);
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
            }
            break;
         }
      case BTriggerVar::cVarTypeString:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asString()->writeVar(varNodeText);
            }
            break;
         }            
      case BTriggerVar::cVarTypeMessageIndex:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asMessageIndex()->writeVar(varNodeText.asLong());
            }
            break;
         }            
      case BTriggerVar::cVarTypeMessageJustify:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asMessageJustify()->writeVar(varNodeText.asLong());
            }
            break;
         }            
      case BTriggerVar::cVarTypeMessagePoint:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asMessagePoint()->writeVar(varNodeText.asFloat());
            }
            break;
         }            
      case BTriggerVar::cVarTypeColor:
         {
            
            BVector temp;
            if (!pVar->isNull() && !varNodeText.isEmpty() && varNode.getTextAsVector(temp))
            {
               BColor color;
               color.r = temp.x / 255.0f;
               color.g = temp.y / 255.0f;
               color.b = temp.z / 255.0f;

               pVar->asColor()->writeVar(color);
            }
            break;
         }            
      case BTriggerVar::cVarTypeProtoObjectList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asProtoObjectList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BProtoObjectIDArray protoObjectList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BProtoObjectID protoObjectID = gDatabase.getProtoObject(token);
                  #ifndef BUILD_FINAL
                  if (protoObjectID == cInvalidProtoObjectID)
                  {
                     BSimString errorString;
                     errorString.format("Script %s, Variable ID=%d, Name=%s, contains protoobject %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), token.getPtr());
                     BFAIL(errorString.getPtr());
                  }
                  #endif
                  protoObjectList.add(protoObjectID);
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asProtoObjectList()->writeVar(protoObjectList);
            }
            break;
         }
      case BTriggerVar::cVarTypeObjectTypeList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asObjectTypeList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BObjectTypeIDArray objectTypeList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BObjectTypeID objectTypeID = gDatabase.getObjectType(token);
                  #ifndef BUILD_FINAL
                  if (objectTypeID == cInvalidObjectTypeID)
                  {
                     BSimString errorString;
                     errorString.format("Script %s, Variable ID=%d, Name=%s, contains objecttype %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), token.getPtr());
                     BFAIL(errorString.getPtr());
                  }
                  #endif
                  objectTypeList.uniqueAdd(objectTypeID);
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asObjectTypeList()->writeVar(objectTypeList);
            }
            break;
         }
      case BTriggerVar::cVarTypeProtoSquadList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asProtoSquadList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BProtoSquadIDArray protoSquadList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BProtoSquadID protoSquadID = gDatabase.getProtoSquad(token);
                  #ifndef BUILD_FINAL
                  if (protoSquadID == cInvalidProtoSquadID)
                  {
                     BSimString errorString;
                     errorString.format("Script %s, Variable ID=%d, Name=%s, contains protosquad %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), token.getPtr());
                     BFAIL(errorString.getPtr());
                  }
                  #endif
                  protoSquadList.add(protoSquadID);
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asProtoSquadList()->writeVar(protoSquadList);
            }
            break;
         }
      case BTriggerVar::cVarTypeTechList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asTechList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BTechIDArray techList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BProtoTechID protoTechID = gDatabase.getProtoTech(token);
                  #ifndef BUILD_FINAL
                  if (protoTechID == cInvalidTechID)
                  {
                     BSimString errorString;
                     errorString.format("Script %s, Variable ID=%d, Name=%s, contains tech %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), token.getPtr());
                     BFAIL(errorString.getPtr());
                  }
                  #endif
                  techList.uniqueAdd(protoTechID);
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asTechList()->writeVar(techList);
            }
            break;
         }
      case BTriggerVar::cVarTypeMathOperator:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               int varNodeEnum = gTriggerManager.getTriggerMathOperatorTypeEnum(varNodeText);
               pVar->asMathOperator()->writeVar(varNodeEnum);
            }
            break;
         }
      case BTriggerVar::cVarTypeObjectDataType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asObjectDataType()->writeVar(gDatabase.getProtoObjectDataType(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeObjectDataRelative:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asObjectDataRelative()->writeVar(gDatabase.getProtoObjectDataRelativity(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeCiv:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asCiv()->writeVar(gDatabase.getCivID(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeProtoObjectCollection:
         {
            // Empty collections are valid and shouldn't give uninitialized asserts.
            pVar->asProtoObjectCollection()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BProtoObjectIDArray protoObjectCollection;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  BProtoObjectID protoObjectID = gDatabase.getProtoObject(token);
                  #ifndef BUILD_FINAL
                  if (protoObjectID == cInvalidProtoObjectID)
                  {
                     BSimString errorString;
                     errorString.format("Script %s, Variable ID=%d, Name=%s, contains protoobject %s which is no longer in the database.", mName.getPtr(), pVar->getID(), pVar->getName().getPtr(), token.getPtr());
                     BFAIL(errorString.getPtr());
                  }
                  #endif
                  protoObjectCollection.add(protoObjectID);
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asProtoObjectCollection()->writeVar(protoObjectCollection);
            }
            break;
         }
      case BTriggerVar::cVarTypeGroup:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asGroup()->writeVar(varNodeText.asLong());
            break;
         }
      case BTriggerVar::cVarTypeRefCountType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asRefCountType()->writeVar((short)gDatabase.getRefCountType(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeUnitFlag:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asUnitFlag()->writeVar(gDatabase.getUnitFlag(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeLOSType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (varNodeText == "LOSNormal")
                  pVar->asLOSType()->writeVar(BWorld::cCPLOSNormal);
               else if (varNodeText == "LOSFullVisible")
                  pVar->asLOSType()->writeVar(BWorld::cCPLOSFullVisible);
               else if (varNodeText == "LOSDontCare")
                  pVar->asLOSType()->writeVar(BWorld::cCPLOSDontCare);
               else
                  pVar->asLOSType()->writeVar(BWorld::cCPLOSDontCare);
            }
            break;
         }
      case BTriggerVar::cVarTypePopBucket:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {               
               pVar->asPopBucket()->writeVar(gDatabase.getPop(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeListPosition:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               int listPos = gTriggerManager.getTriggerListPositionTypeEnum(varNodeText);
               pVar->asListPosition()->writeVar(listPos);
            }
            break;
         }
      case BTriggerVar::cVarTypeRelationType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BRelationType relationType = gDatabase.getRelationType(varNodeText);
               pVar->asRelationType()->writeVar(relationType);
            }
            break;
         }
      case BTriggerVar::cVarTypeExposedAction:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (HACK_doesStringContainOnlyNumbers(varNodeText))
               {
                  #ifndef BUILD_FINAL
                  numVarsRequiringAutoFixUp++;
                  #endif
                  pVar->asExposedAction()->writeVar(varNodeText.asLong());
               }
               else
                  pVar->asExposedAction()->writeVar(gDatabase.getExposedActionFromName(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeSquadMode:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               int squadMode = gDatabase.getSquadMode(varNodeText);
               pVar->asSquadMode()->writeVar(squadMode);
            }
            break;
         }
    case BTriggerVar::cVarTypeExposedScript:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asExposedScript()->writeVar((uint)varNodeText.asLong());
            }
            break;
         }
      case BTriggerVar::cVarTypeKBBaseList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asKBBaseList()->clear();
            break;
         }
      case BTriggerVar::cVarTypeDataScalar:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (HACK_doesStringContainOnlyNumbers(varNodeText))
               {
                  #ifndef BUILD_FINAL
                  numVarsRequiringAutoFixUp++;
                  #endif
                  pVar->asDataScalar()->writeVar((uint)varNodeText.asLong());
               }
               else
                  pVar->asDataScalar()->writeVar(gDatabase.getDataScalarFromName(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeKBBaseQuery:
         {
            pVar->asKBBaseQuery()->resetQuery();
            break;
         }
      case BTriggerVar::cVarTypeDesignLine:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asDesignLine()->writeVar(varNodeText.asLong());
            break;
         }
      case BTriggerVar::cVarTypeLocStringID:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asLocStringID()->writeVar(varNodeText.asLong());
            break;
         }
      case BTriggerVar::cVarTypeLeader:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asLeader()->writeVar(gDatabase.getLeaderID(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeCinematic:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asCinematic()->writeVar((uint)varNodeText.asLong());
            }
            break;
         }
      case BTriggerVar::cVarTypeTalkingHead:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asTalkingHead()->writeVar((uint)varNodeText.asLong());
            }
            break;
         }
      case BTriggerVar::cVarTypeFlareType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (HACK_doesStringContainOnlyNumbers(varNodeText))
               {
                  #ifndef BUILD_FINAL
                  numVarsRequiringAutoFixUp++;
                  #endif
                  pVar->asFlareType()->writeVar((int)varNodeText.asLong());
               }
               else
                  pVar->asFlareType()->writeVar(gDatabase.getFlareTypeFromName(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeIconType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asIconType()->writeVar(gDatabase.getProtoObject(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeCinematicTag:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asCinematicTag()->writeVar((uint)varNodeText.asLong());
            }
            break;
         }
      case BTriggerVar::cVarTypeDifficulty:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asDifficulty()->writeVar(varNodeText.asLong());
            }
            break;
         }
      case BTriggerVar::cVarTypeInteger:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               int32 integerVarValue = 0;
               if (varNode.getTextAsInt32(integerVarValue))
                  pVar->asInteger()->writeVar(integerVarValue);
            }
            break;
         }
      case BTriggerVar::cVarTypeHUDItem:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asHUDItem()->writeVar(gDatabase.getHUDItem(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeFlashableUIItem:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asUIItem()->writeVar(gDatabase.getFlashableUIItem(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeControlType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asControlType()->writeVar(lookupControl(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeMissionType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asMissionType()->writeVar(gDatabase.getMissionType(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeMissionState:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asMissionState()->writeVar(gDatabase.getMissionState(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeMissionTargetType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asMissionTargetType()->writeVar(gDatabase.getMissionTargetType(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeIntegerList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asIntegerList()->clear();
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BInt32Array integerList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  integerList.add(token.asLong());
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asIntegerList()->writeVar(integerList);
            }
            break;
         }
      case BTriggerVar::cVarTypeBidType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asBidType()->writeVar(gDatabase.getBidType(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeBidState:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asBidState()->writeVar(gDatabase.getBidState(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeBuildingCommandState:
         {
            // Run-time only initialized.
            // Therefore, there cannot be a predetermined value in the editor so do not load anything here.
            break;
         }
      case BTriggerVar::cVarTypeVector:
         {
            BVector v;
            if (!pVar->isNull() && !varNodeText.isEmpty() && varNode.getTextAsVector(v))
               pVar->asVector()->writeVar(v);
            break;
         }
      case BTriggerVar::cVarTypeVectorList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asVectorList()->clear();

            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BSimString token;
               BSimString vecTok;
               BVector vec;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B("|"));
               while (loc != -1)
               {
                  long vecLen = token.length();
                  long tokenVec = vecTok.copyTok(token, vecLen, -1, B(","));
                  float x = vecTok.asFloat();
                  tokenVec = vecTok.copyTok(token, vecLen, tokenVec+1, B(","));
                  float y = vecTok.asFloat();
                  tokenVec = vecTok.copyTok(token, vecLen, tokenVec+1, B(","));
                  float z = vecTok.asFloat();
                  vec.set(x, y, z);
                  pVar->asVectorList()->addVector(vec);

                  loc = token.copyTok(varNodeText, strLen, loc+1, B("|"));
               }
            }
            break;
         }
      case BTriggerVar::cVarTypePlacementRule:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asPlacementRule()->writeVar(gDatabase.getPlacementRules(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeKBSquadList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asKBSquadList()->clear();
            break;
         }
      case BTriggerVar::cVarTypeKBSquadQuery:
         {
            pVar->asKBSquadQuery()->resetQuery();
            break;
         }
      case BTriggerVar::cVarTypeAISquadAnalysisComponent:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asAISquadAnalysisComponent()->writeVar(gDatabase.getAISquadAnalysisComponent(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeChatSpeaker:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               if (HACK_doesStringContainOnlyNumbers(varNodeText))
               {
                  #ifndef BUILD_FINAL
                  numVarsRequiringAutoFixUp++;
                  #endif
                  pVar->asChatSpeaker()->writeVar(varNodeText.asLong());
               }
               else
                  pVar->asChatSpeaker()->writeVar(gDatabase.getChatSpeakerFromName(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeRumbleType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asRumbleType()->writeVar(BGamepad::getRumbleType(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeRumbleMotor:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asRumbleMotor()->writeVar(BGamepad::getRumbleMotor(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeTechDataCommandType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asTechDataCommandType()->writeVar(gDatabase.getProtoObjectCommandType(varNodeText.getPtr()));
            break;
         }
      case BTriggerVar::cVarTypeSquadDataType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asSquadDataType()->writeVar(gDatabase.getProtoObjectDataType(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeEventType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asEventType()->writeVar(BEventDefinitions::getGeneralEventEnum(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeTimeList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asTimeList()->clear();
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BDWORDArray timeList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  timeList.add((DWORD)token.asLong());
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asTimeList()->writeVar(timeList);
            }
            break;
         }
      case BTriggerVar::cVarTypeDesignLineList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asDesignLineList()->clear();
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BDesignLineIDArray lineList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  lineList.add(token.asLong());
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asDesignLineList()->writeVar(lineList);
            }
            break;
         }
      case BTriggerVar::cVarTypeGameStatePredicate:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               pVar->asGameStatePredicate()->writeVar(BGameStateDefinitions::getEnum(varNodeText));
            }
            break;
         }
      case BTriggerVar::cVarTypeFloatList:
         {
            // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asFloatList()->clear();
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BFloatArray floatList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  floatList.add(token.asFloat());
                  loc = token.copyTok(varNodeText, strLen, loc + 1, B(","));
               }
               pVar->asFloatList()->writeVar(floatList);
            }
            break;
         }
      case BTriggerVar::cVarTypeSquadFlag:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
               pVar->asSquadFlag()->writeVar(gDatabase.getSquadFlag(varNodeText));
            break;
         }
      case BTriggerVar::cVarTypeConcept:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               int32 integerVarValue = 0;
               if (varNode.getTextAsInt32(integerVarValue))
                  pVar->asConcept()->writeVar(integerVarValue);
            }
            break;
         }
      case BTriggerVar::cVarTypeConceptList:
         {
           // Empty lists are valid and shouldn't give uninitialized asserts.
            pVar->asConceptList()->clear();
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               BInt32Array integerList;
               BSimString token;
               long strLen = varNodeText.length();
               long loc = token.copyTok(varNodeText, strLen, -1, B(","));
               while (loc != -1)
               {
                  integerList.add(token.asLong());
                  loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
               }
               pVar->asConceptList()->writeVar(integerList);
            }
            break;
         }
      case BTriggerVar::cVarTypeUserClassType:
         {
            if (!pVar->isNull() && !varNodeText.isEmpty())
            {
               int32 integerVarValue = 0;
               if (varNode.getTextAsInt32(integerVarValue))
                  pVar->asUserClassType()->writeVar(integerVarValue);
               //if it is a name..
               //BSimString className;
               //if(varNode.getText(className))
               //{
               //   integerVarValue = gTriggerManager.getUserClassManager()->getUserClassID(className);                  
               //}
               //pVar->asConcept()->writeVar(integerVarValue);
            }
            break;
         }

      // NEWTRIGGERVARTYPE
      // Add your code that loads the "default value" of your trigger variable above this comment.
      // See above examples for ways this is done.  You can pretty much follow their examples for most data types (unless it's a complex type).
      // If your variable type is one that cannot have a value before runtime (very rare) it should go in the block below.
      case BTriggerVar::cVarTypeUILocation:
      case BTriggerVar::cVarTypeUILocationMinigame:
      case BTriggerVar::cVarTypeUIEntity:
      case BTriggerVar::cVarTypeIterator:
      case BTriggerVar::cVarTypeUIUnit:
      case BTriggerVar::cVarTypeUISquad:
      case BTriggerVar::cVarTypeUISquadList:  
      case BTriggerVar::cVarTypeEntityFilterSet:
      case BTriggerVar::cVarTypeKBBase:
      case BTriggerVar::cVarTypeUIButton:
      case BTriggerVar::cVarTypeKBSquad:
      case BTriggerVar::cVarTypeAISquadAnalysis:
      case BTriggerVar::cVarTypeKBSquadFilterSet:
         {
            // User input and iterators are run-time only initialized.
            // Therefore, there cannot be a predetermined value in the editor so do not load anything here.
            break;
         }
      default:
         BFATAL_ASSERT(false);
         return (false);
      }

   return true;

}



//==============================================================================
// BTriggerScript::loadTriggerVarsFromXML
//==============================================================================
bool BTriggerScript::loadTriggerVarsFromXML(BXMLNode triggerVarsNode, uint maxNumTriggerVars)
{
   // HACKOTRON
   BTriggerVarID highestVarID = 0;
   uint numTriggerVars = static_cast<uint>(triggerVarsNode.getNumberChildren());
   for (uint i=0; i<numTriggerVars; i++)
   {
      BXMLNode varNode(triggerVarsNode.getChild(i));
      if (varNode.getName() != "TriggerVar")
         return (false);
      DWORD varIDDWORD = BTriggerVar::cVarIDInvalid;
      if (!varNode.getAttribValueAsDWORD("ID", varIDDWORD))
         return (false);
      if (varIDDWORD > highestVarID)
         highestVarID = varIDDWORD;
   }
   maxNumTriggerVars = highestVarID+1;

   // Initialize our vars to NULL.
   mTriggerVars.setNumber(maxNumTriggerVars);
   for (uint i=0; i<maxNumTriggerVars; i++)
      mTriggerVars[i] = NULL;


   uint numVarsRequiringAutoFixUp = 0;


   for (uint i=0; i<numTriggerVars; i++)
   {
      BXMLNode  varNode(triggerVarsNode.getChild(i));
      if (varNode.getName() != "TriggerVar")
         return (false);
      DWORD varIDDWORD = BTriggerVar::cVarIDInvalid;
      if (!varNode.getAttribValueAsDWORD("ID", varIDDWORD))
         return (false);

      BASSERT(varIDDWORD < BTriggerVar::cVarIDMax);
      BTriggerVarID varID = static_cast<BTriggerVarID>(varIDDWORD);
      if (varID >= maxNumTriggerVars)
         return (false);
      if (mTriggerVars[varID])
         return (false);   // should not already have this ID initialized.
#ifndef BUILD_FINAL
      BSimString varName = "";
      varNode.getAttribValueAsString("Name", varName);
#endif
      BSimString varTypeString;
      if (!varNode.getAttribValueAsString("Type", varTypeString))
         return (false);
      BTriggerVarType varTypeEnum = gTriggerManager.getTriggerVarTypeEnum(varTypeString);
      if (varTypeEnum == BTriggerVar::cVarTypeInvalid)
         return (false);
      
      //BSimString tempStr;
      //const BSimString& varNodeText = varNode.getText(tempStr);

      BTriggerVar *pVar = getTriggerVarInstance(varTypeEnum);
      BASSERTM(pVar, "Unable to allocate trigger var instance.");
      if (!pVar)
         return (false);

      bool isnull = false;
      if (varNode.getAttribValueAsBool("IsNull", isnull) && isnull)
         pVar->setNull();

      pVar->setID(varID);
      #ifndef BUILD_FINAL
         pVar->setName(varName);
      #endif
  
      if(writeVar(pVar, varTypeEnum, varNode, numVarsRequiringAutoFixUp) == false)
      {
         return false; //return false only on fatal assert.
      }      

//      pVar->setID(varID);
//#ifndef BUILD_FINAL
//      pVar->setName(varName);
//#endif
      mTriggerVars[varID] = pVar;
      if (varTypeEnum == BTriggerVar::cVarTypeUnitList)
         mTriggerVarUnitLists.add(pVar->asUnitList());
      else if (varTypeEnum == BTriggerVar::cVarTypeSquadList)
         mTriggerVarSquadLists.add(pVar->asSquadList());
   }

#ifndef BUILD_FINAL
   if (numVarsRequiringAutoFixUp > 0)
   {
      BSimString errorMessage;
      errorMessage.format("Trigger script %s contains %d trigger vars requiring an automatic data fix-up.  Please re-save it in the trigger editor.", mName.getPtr(), numVarsRequiringAutoFixUp);
      BASSERTM(false, errorMessage.getPtr());
   }
#endif

   return (true);
}


//==============================================================================
// BTriggerScript::getTriggerVarInstance
//==============================================================================
BTriggerVar* BTriggerScript::getTriggerVarInstance(BTriggerVarType type)
{
   BTriggerVar *pVar = NULL;
   switch(type)
   {
   case BTriggerVar::cVarTypeTech: { pVar = BTriggerVarTech::getInstance(); break; }
   case BTriggerVar::cVarTypeTechStatus: { pVar = BTriggerVarTechStatus::getInstance(); break; }
   case BTriggerVar::cVarTypeOperator: { pVar = BTriggerVarOperator::getInstance(); break; }
   case BTriggerVar::cVarTypeProtoObject: { pVar = BTriggerVarProtoObject::getInstance(); break; }
   case BTriggerVar::cVarTypeObjectType: { pVar = BTriggerVarObjectType::getInstance(); break; }
   case BTriggerVar::cVarTypeProtoSquad: { pVar = BTriggerVarProtoSquad::getInstance(); break; }
   case BTriggerVar::cVarTypeSound: { pVar = BTriggerVarSound::getInstance(); break; }
   case BTriggerVar::cVarTypeEntity: { pVar = BTriggerVarEntity::getInstance(); break; }
   case BTriggerVar::cVarTypeEntityList: { pVar = BTriggerVarEntityList::getInstance(); break; }
   case BTriggerVar::cVarTypeTrigger: { pVar = BTriggerVarTrigger::getInstance(); break; }
   case BTriggerVar::cVarTypeTime: { pVar = BTriggerVarTime::getInstance(); break; }
   case BTriggerVar::cVarTypePlayer: { pVar = BTriggerVarPlayer::getInstance(); break; }
   case BTriggerVar::cVarTypeUILocation: { pVar = BTriggerVarUILocation::getInstance(); break; }                                      
   case BTriggerVar::cVarTypeUIEntity: { pVar = BTriggerVarUIEntity::getInstance(); break; }
   case BTriggerVar::cVarTypeCost: { pVar = BTriggerVarCost::getInstance(); break; }
   case BTriggerVar::cVarTypeAnimType: { pVar = BTriggerVarAnimType::getInstance(); break; }
   case BTriggerVar::cVarTypeActionStatus: { pVar = BTriggerVarActionStatus::getInstance(); break;}
   case BTriggerVar::cVarTypePower: { pVar = BTriggerVarPower::getInstance(); break; }
   case BTriggerVar::cVarTypeBool: { pVar = BTriggerVarBool::getInstance(); break; }
   case BTriggerVar::cVarTypeFloat: { pVar = BTriggerVarFloat::getInstance(); break; }
   case BTriggerVar::cVarTypeIterator: { pVar = BTriggerVarIterator::getInstance(); break; }
   case BTriggerVar::cVarTypeTeam: { pVar = BTriggerVarTeam::getInstance(); break; }
   case BTriggerVar::cVarTypePlayerList: { pVar = BTriggerVarPlayerList::getInstance(); break; }
   case BTriggerVar::cVarTypeTeamList: { pVar = BTriggerVarTeamList::getInstance(); break; }
   case BTriggerVar::cVarTypePlayerState: { pVar = BTriggerVarPlayerState::getInstance(); break; }
   case BTriggerVar::cVarTypeObjective: { pVar = BTriggerVarObjective::getInstance(); break; }
   case BTriggerVar::cVarTypeUnit: { pVar = BTriggerVarUnit::getInstance(); break; }
   case BTriggerVar::cVarTypeUnitList: { pVar = BTriggerVarUnitList::getInstance(); break; }
   case BTriggerVar::cVarTypeSquad: { pVar = BTriggerVarSquad::getInstance(); break; }
   case BTriggerVar::cVarTypeSquadList: { pVar = BTriggerVarSquadList::getInstance(); break; }
   case BTriggerVar::cVarTypeUIUnit: { pVar = BTriggerVarUIUnit::getInstance(); break; }
   case BTriggerVar::cVarTypeUISquad: { pVar = BTriggerVarUISquad::getInstance(); break; }
   case BTriggerVar::cVarTypeUISquadList: { pVar = BTriggerVarUISquadList::getInstance(); break; }
   case BTriggerVar::cVarTypeString: { pVar = BTriggerVarString::getInstance(); break; }
   case BTriggerVar::cVarTypeMessageIndex: { pVar = BTriggerVarMessageIndex::getInstance(); break; }
   case BTriggerVar::cVarTypeMessageJustify: { pVar = BTriggerVarMessageJustify::getInstance(); break; }
   case BTriggerVar::cVarTypeMessagePoint: { pVar = BTriggerVarMessagePoint::getInstance(); break; }
   case BTriggerVar::cVarTypeColor: { pVar = BTriggerVarColor::getInstance(); break; }
   case BTriggerVar::cVarTypeProtoObjectList: { pVar = BTriggerVarProtoObjectList::getInstance(); break; }
   case BTriggerVar::cVarTypeObjectTypeList: { pVar = BTriggerVarObjectTypeList::getInstance(); break; }
   case BTriggerVar::cVarTypeProtoSquadList: { pVar = BTriggerVarProtoSquadList::getInstance(); break; }
   case BTriggerVar::cVarTypeTechList: { pVar = BTriggerVarTechList::getInstance(); break; }
   case BTriggerVar::cVarTypeMathOperator: { pVar = BTriggerVarMathOperator::getInstance(); break; }
   case BTriggerVar::cVarTypeObjectDataType: { pVar = BTriggerVarObjectDataType::getInstance(); break; }
   case BTriggerVar::cVarTypeObjectDataRelative: { pVar = BTriggerVarObjectDataRelative::getInstance(); break; }
   case BTriggerVar::cVarTypeCiv: { pVar = BTriggerVarCiv::getInstance(); break; }
   case BTriggerVar::cVarTypeProtoObjectCollection: { pVar = BTriggerVarProtoObjectCollection::getInstance(); break; }
   case BTriggerVar::cVarTypeObject: { pVar = BTriggerVarObject::getInstance(); break; }
   case BTriggerVar::cVarTypeObjectList: { pVar = BTriggerVarObjectList::getInstance(); break; }
   case BTriggerVar::cVarTypeGroup: { pVar = BTriggerVarGroup::getInstance(); break; }
   case BTriggerVar::cVarTypeRefCountType: { pVar = BTriggerVarRefCountType::getInstance(); break; }
   case BTriggerVar::cVarTypeUnitFlag: { pVar = BTriggerVarUnitFlag::getInstance(); break; }
   case BTriggerVar::cVarTypeLOSType: { pVar = BTriggerVarLOSType::getInstance(); break; }
   case BTriggerVar::cVarTypeEntityFilterSet: { pVar = BTriggerVarEntityFilterSet::getInstance(); break; }
   case BTriggerVar::cVarTypePopBucket: { pVar = BTriggerVarPopBucket::getInstance(); break; }
   case BTriggerVar::cVarTypeListPosition: { pVar = BTriggerVarListPosition::getInstance(); break; }
   case BTriggerVar::cVarTypeRelationType: { pVar = BTriggerVarRelationType::getInstance(); break; }
   case BTriggerVar::cVarTypeExposedAction: { pVar = BTriggerVarExposedAction::getInstance(); break; }
   case BTriggerVar::cVarTypeSquadMode: { pVar = BTriggerVarSquadMode::getInstance(); break; }
   case BTriggerVar::cVarTypeExposedScript: { pVar = BTriggerVarExposedScript::getInstance(); break; }
   case BTriggerVar::cVarTypeKBBase: { pVar = BTriggerVarKBBase::getInstance(); break; }
   case BTriggerVar::cVarTypeKBBaseList: { pVar = BTriggerVarKBBaseList::getInstance(); break; }
   case BTriggerVar::cVarTypeDataScalar: { pVar = BTriggerVarDataScalar::getInstance(); break; }
   case BTriggerVar::cVarTypeKBBaseQuery: { pVar = BTriggerVarKBBaseQuery::getInstance(); break; }
   case BTriggerVar::cVarTypeDesignLine: { pVar = BTriggerVarDesignLine::getInstance(); break; }
   case BTriggerVar::cVarTypeLocStringID: { pVar = BTriggerVarLocStringID::getInstance(); break; }
   case BTriggerVar::cVarTypeLeader: { pVar = BTriggerVarLeader::getInstance(); break; }
   case BTriggerVar::cVarTypeCinematic: { pVar = BTriggerVarCinematic::getInstance(); break; }
   case BTriggerVar::cVarTypeTalkingHead: { pVar = BTriggerVarTalkingHead::getInstance(); break; }
   case BTriggerVar::cVarTypeFlareType: { pVar = BTriggerVarFlareType::getInstance(); break; }
   case BTriggerVar::cVarTypeCinematicTag: { pVar = BTriggerVarCinematicTag::getInstance(); break; }
   case BTriggerVar::cVarTypeIconType: { pVar = BTriggerVarIconType::getInstance(); break; }
   case BTriggerVar::cVarTypeDifficulty: { pVar = BTriggerVarDifficulty::getInstance(); break; }
   case BTriggerVar::cVarTypeInteger: { pVar = BTriggerVarInteger::getInstance(); break; }
   case BTriggerVar::cVarTypeHUDItem: { pVar = BTriggerVarHUDItem::getInstance(); break; }
   case BTriggerVar::cVarTypeFlashableUIItem: { pVar = BTriggerVarFlashableUIItem::getInstance(); break; }
   case BTriggerVar::cVarTypeControlType: { pVar = BTriggerVarControlType::getInstance(); break; }
   case BTriggerVar::cVarTypeUIButton: { pVar = BTriggerVarUIButton::getInstance(); break; }
   case BTriggerVar::cVarTypeMissionType: { pVar = BTriggerVarMissionType::getInstance(); break; }
   case BTriggerVar::cVarTypeMissionState: { pVar = BTriggerVarMissionState::getInstance(); break; }
   case BTriggerVar::cVarTypeMissionTargetType: { pVar = BTriggerVarMissionTargetType::getInstance(); break; }
   case BTriggerVar::cVarTypeIntegerList: { pVar = BTriggerVarIntegerList::getInstance(); break; }
   case BTriggerVar::cVarTypeBidType: { pVar = BTriggerVarBidType::getInstance(); break; }
   case BTriggerVar::cVarTypeBidState: { pVar = BTriggerVarBidState::getInstance(); break; }
   case BTriggerVar::cVarTypeBuildingCommandState: { pVar = BTriggerVarBuildingCommandState::getInstance(); break; }
   case BTriggerVar::cVarTypeVector: { pVar = BTriggerVarVector::getInstance(); break; }
   case BTriggerVar::cVarTypeVectorList: { pVar = BTriggerVarVectorList::getInstance(); break; }
   case BTriggerVar::cVarTypePlacementRule: { pVar = BTriggerVarPlacementRule::getInstance(); break; }
   case BTriggerVar::cVarTypeKBSquad: { pVar = BTriggerVarKBSquad::getInstance(); break; }
   case BTriggerVar::cVarTypeKBSquadList: { pVar = BTriggerVarKBSquadList::getInstance(); break; }
   case BTriggerVar::cVarTypeKBSquadQuery: { pVar = BTriggerVarKBSquadQuery::getInstance(); break; }
   case BTriggerVar::cVarTypeAISquadAnalysis: { pVar = BTriggerVarAISquadAnalysis::getInstance(); break; }
   case BTriggerVar::cVarTypeAISquadAnalysisComponent: { pVar = BTriggerVarAISquadAnalysisComponent::getInstance(); break; }
   case BTriggerVar::cVarTypeKBSquadFilterSet: { pVar = BTriggerVarKBSquadFilterSet::getInstance(); break; }
   case BTriggerVar::cVarTypeChatSpeaker: { pVar = BTriggerVarChatSpeaker::getInstance(); break; }
   case BTriggerVar::cVarTypeRumbleType: { pVar = BTriggerVarRumbleType::getInstance(); break; }
   case BTriggerVar::cVarTypeRumbleMotor: { pVar = BTriggerVarRumbleMotor::getInstance(); break; }
   case BTriggerVar::cVarTypeTechDataCommandType: { pVar = BTriggerVarTechDataCommandType::getInstance(); break; }
   case BTriggerVar::cVarTypeSquadDataType: { pVar = BTriggerVarSquadDataType::getInstance(); break; }
   case BTriggerVar::cVarTypeEventType: { pVar = BTriggerVarEventType::getInstance(); break; }
   case BTriggerVar::cVarTypeTimeList: { pVar = BTriggerVarTimeList::getInstance(); break; }
   case BTriggerVar::cVarTypeDesignLineList: { pVar = BTriggerVarDesignLineList::getInstance(); break; }
   case BTriggerVar::cVarTypeGameStatePredicate: { pVar = BTriggerVarGameStatePredicate::getInstance(); break; }
   case BTriggerVar::cVarTypeFloatList: { pVar = BTriggerVarFloatList::getInstance(); break; }
   case BTriggerVar::cVarTypeUILocationMinigame: { pVar = BTriggerVarUILocationMinigame::getInstance(); break; }                                      
   case BTriggerVar::cVarTypeSquadFlag: { pVar = BTriggerVarSquadFlag::getInstance(); break; }
   case BTriggerVar::cVarTypeConcept: { pVar = BTriggerVarConcept::getInstance(); break; }
   case BTriggerVar::cVarTypeConceptList: { pVar = BTriggerVarConceptList::getInstance(); break; }
   case BTriggerVar::cVarTypeUserClassType: { pVar = BTriggerVarUserClassType::getInstance(); break; }

   // NEWTRIGGERVARTYPE
   // Add your case statement for your trigger var type here, that gets an instance from the freelist.

   default: BASSERTM(false, "You are trying to get an instance of an unknown trigger var type.  Botched.");
   }

   return (pVar);
}


//==============================================================================
// BTriggerScript::releaseTriggerVarInstance
//==============================================================================
void BTriggerScript::releaseTriggerVarInstance(BTriggerVar *pVar)
{
   BASSERT(pVar);
   BTriggerVarType triggerVarType = pVar->getType();
   switch(triggerVarType)
   {
   case BTriggerVar::cVarTypeTech: { BTriggerVarTech::releaseInstance(reinterpret_cast<BTriggerVarTech*>(pVar)); break; }
   case BTriggerVar::cVarTypeTechStatus: { BTriggerVarTechStatus::releaseInstance(reinterpret_cast<BTriggerVarTechStatus*>(pVar)); break; }
   case BTriggerVar::cVarTypeOperator: { BTriggerVarOperator::releaseInstance(reinterpret_cast<BTriggerVarOperator*>(pVar)); break; }
   case BTriggerVar::cVarTypeProtoObject: { BTriggerVarProtoObject::releaseInstance(reinterpret_cast<BTriggerVarProtoObject*>(pVar)); break; }
   case BTriggerVar::cVarTypeObjectType: { BTriggerVarObjectType::releaseInstance(reinterpret_cast<BTriggerVarObjectType*>(pVar)); break; }
   case BTriggerVar::cVarTypeProtoSquad: { BTriggerVarProtoSquad::releaseInstance(reinterpret_cast<BTriggerVarProtoSquad*>(pVar)); break; }
   case BTriggerVar::cVarTypeSound: { BTriggerVarSound::releaseInstance(reinterpret_cast<BTriggerVarSound*>(pVar)); break; }
   case BTriggerVar::cVarTypeEntity: { BTriggerVarEntity::releaseInstance(reinterpret_cast<BTriggerVarEntity*>(pVar)); break; }
   case BTriggerVar::cVarTypeEntityList: { BTriggerVarEntityList::releaseInstance(reinterpret_cast<BTriggerVarEntityList*>(pVar)); break; }
   case BTriggerVar::cVarTypeTrigger: { BTriggerVarTrigger::releaseInstance(reinterpret_cast<BTriggerVarTrigger*>(pVar)); break; }
   case BTriggerVar::cVarTypeTime: { BTriggerVarTime::releaseInstance(reinterpret_cast<BTriggerVarTime*>(pVar)); break; }
   case BTriggerVar::cVarTypePlayer: { BTriggerVarPlayer::releaseInstance(reinterpret_cast<BTriggerVarPlayer*>(pVar)); break; }
   case BTriggerVar::cVarTypeUILocation: { BTriggerVarUILocation::releaseInstance(reinterpret_cast<BTriggerVarUILocation*>(pVar)); break; }
   case BTriggerVar::cVarTypeUIEntity: { BTriggerVarUIEntity::releaseInstance(reinterpret_cast<BTriggerVarUIEntity*>(pVar)); break; }
   case BTriggerVar::cVarTypeCost: { BTriggerVarCost::releaseInstance(reinterpret_cast<BTriggerVarCost*>(pVar)); break; }
   case BTriggerVar::cVarTypeAnimType: { BTriggerVarAnimType::releaseInstance(reinterpret_cast<BTriggerVarAnimType*>(pVar)); break; }
   case BTriggerVar::cVarTypeActionStatus: { BTriggerVarActionStatus::releaseInstance(reinterpret_cast<BTriggerVarActionStatus*>(pVar)); break; }
   case BTriggerVar::cVarTypePower: { BTriggerVarPower::releaseInstance(reinterpret_cast<BTriggerVarPower*>(pVar)); break; }
   case BTriggerVar::cVarTypeBool: { BTriggerVarBool::releaseInstance(reinterpret_cast<BTriggerVarBool*>(pVar)); break; }
   case BTriggerVar::cVarTypeFloat: { BTriggerVarFloat::releaseInstance(reinterpret_cast<BTriggerVarFloat*>(pVar)); break; }
   case BTriggerVar::cVarTypeIterator: { BTriggerVarIterator::releaseInstance(reinterpret_cast<BTriggerVarIterator*>(pVar)); break; }
   case BTriggerVar::cVarTypeTeam: { BTriggerVarTeam::releaseInstance(reinterpret_cast<BTriggerVarTeam*>(pVar)); break; }
   case BTriggerVar::cVarTypePlayerList: { BTriggerVarPlayerList::releaseInstance(reinterpret_cast<BTriggerVarPlayerList*>(pVar)); break; }
   case BTriggerVar::cVarTypeTeamList: { BTriggerVarTeamList::releaseInstance(reinterpret_cast<BTriggerVarTeamList*>(pVar)); break; }
   case BTriggerVar::cVarTypePlayerState: { BTriggerVarPlayerState::releaseInstance(reinterpret_cast<BTriggerVarPlayerState*>(pVar)); break; }
   case BTriggerVar::cVarTypeObjective: { BTriggerVarObjective::releaseInstance(reinterpret_cast<BTriggerVarObjective*>(pVar)); break; }
   case BTriggerVar::cVarTypeUnit: { BTriggerVarUnit::releaseInstance(reinterpret_cast<BTriggerVarUnit*>(pVar)); break; }
   case BTriggerVar::cVarTypeUnitList: { BTriggerVarUnitList::releaseInstance(reinterpret_cast<BTriggerVarUnitList*>(pVar)); break; }
   case BTriggerVar::cVarTypeSquad: { BTriggerVarSquad::releaseInstance(reinterpret_cast<BTriggerVarSquad*>(pVar)); break; }
   case BTriggerVar::cVarTypeSquadList: { BTriggerVarSquadList::releaseInstance(reinterpret_cast<BTriggerVarSquadList*>(pVar)); break; }
   case BTriggerVar::cVarTypeUIUnit: { BTriggerVarUIUnit::releaseInstance(reinterpret_cast<BTriggerVarUIUnit*>(pVar)); break; }
   case BTriggerVar::cVarTypeUISquad: { BTriggerVarUISquad::releaseInstance(reinterpret_cast<BTriggerVarUISquad*>(pVar)); break; }
   case BTriggerVar::cVarTypeUISquadList: { BTriggerVarUISquadList::releaseInstance(reinterpret_cast<BTriggerVarUISquadList*>(pVar)); break; }
   case BTriggerVar::cVarTypeString: { BTriggerVarString::releaseInstance(reinterpret_cast<BTriggerVarString*>(pVar)); break; }
   case BTriggerVar::cVarTypeMessageIndex: { BTriggerVarMessageIndex::releaseInstance(reinterpret_cast<BTriggerVarMessageIndex*>(pVar)); break; }
   case BTriggerVar::cVarTypeMessageJustify: { BTriggerVarMessageJustify::releaseInstance(reinterpret_cast<BTriggerVarMessageJustify*>(pVar)); break; }
   case BTriggerVar::cVarTypeMessagePoint: { BTriggerVarMessagePoint::releaseInstance(reinterpret_cast<BTriggerVarMessagePoint*>(pVar)); break; }
   case BTriggerVar::cVarTypeColor: { BTriggerVarColor::releaseInstance(reinterpret_cast<BTriggerVarColor*>(pVar)); break; }
   case BTriggerVar::cVarTypeProtoObjectList: { BTriggerVarProtoObjectList::releaseInstance(reinterpret_cast<BTriggerVarProtoObjectList*>(pVar)); break; }
   case BTriggerVar::cVarTypeObjectTypeList: { BTriggerVarObjectTypeList::releaseInstance(reinterpret_cast<BTriggerVarObjectTypeList*>(pVar)); break; }
   case BTriggerVar::cVarTypeProtoSquadList: { BTriggerVarProtoSquadList::releaseInstance(reinterpret_cast<BTriggerVarProtoSquadList*>(pVar)); break; }
   case BTriggerVar::cVarTypeTechList: { BTriggerVarTechList::releaseInstance(reinterpret_cast<BTriggerVarTechList*>(pVar)); break; }
   case BTriggerVar::cVarTypeMathOperator: { BTriggerVarMathOperator::releaseInstance(reinterpret_cast<BTriggerVarMathOperator*>(pVar)); break; }
   case BTriggerVar::cVarTypeObjectDataType: { BTriggerVarObjectDataType::releaseInstance(reinterpret_cast<BTriggerVarObjectDataType*>(pVar)); break; }
   case BTriggerVar::cVarTypeObjectDataRelative: { BTriggerVarObjectDataRelative::releaseInstance(reinterpret_cast<BTriggerVarObjectDataRelative*>(pVar)); break; }
   case BTriggerVar::cVarTypeCiv: { BTriggerVarCiv::releaseInstance(reinterpret_cast<BTriggerVarCiv*>(pVar)); break; }
   case BTriggerVar::cVarTypeProtoObjectCollection: { BTriggerVarProtoObjectCollection::releaseInstance(reinterpret_cast<BTriggerVarProtoObjectCollection*>(pVar)); break; }
   case BTriggerVar::cVarTypeObject: { BTriggerVarObject::releaseInstance(reinterpret_cast<BTriggerVarObject*>(pVar)); break; }
   case BTriggerVar::cVarTypeObjectList: { BTriggerVarObjectList::releaseInstance(reinterpret_cast<BTriggerVarObjectList*>(pVar)); break; }
   case BTriggerVar::cVarTypeGroup: { BTriggerVarGroup::releaseInstance(reinterpret_cast<BTriggerVarGroup*>(pVar)); break; }
   case BTriggerVar::cVarTypeRefCountType: { BTriggerVarRefCountType::releaseInstance(reinterpret_cast<BTriggerVarRefCountType*>(pVar)); break; }
   case BTriggerVar::cVarTypeUnitFlag: { BTriggerVarUnitFlag::releaseInstance(reinterpret_cast<BTriggerVarUnitFlag*>(pVar)); break; }
   case BTriggerVar::cVarTypeLOSType: { BTriggerVarLOSType::releaseInstance(reinterpret_cast<BTriggerVarLOSType*>(pVar)); break; }
   case BTriggerVar::cVarTypeEntityFilterSet: { BTriggerVarEntityFilterSet::releaseInstance(reinterpret_cast<BTriggerVarEntityFilterSet*>(pVar)); break; }
   case BTriggerVar::cVarTypePopBucket: { BTriggerVarPopBucket::releaseInstance(reinterpret_cast<BTriggerVarPopBucket*>(pVar)); break; }
   case BTriggerVar::cVarTypeListPosition: { BTriggerVarListPosition::releaseInstance(reinterpret_cast<BTriggerVarListPosition*>(pVar)); break; }
   case BTriggerVar::cVarTypeRelationType: { BTriggerVarRelationType::releaseInstance(reinterpret_cast<BTriggerVarRelationType*>(pVar)); break; }
   case BTriggerVar::cVarTypeExposedAction: { BTriggerVarExposedAction::releaseInstance(reinterpret_cast<BTriggerVarExposedAction*>(pVar)); break; }
   case BTriggerVar::cVarTypeSquadMode: { BTriggerVarSquadMode::releaseInstance(reinterpret_cast<BTriggerVarSquadMode*>(pVar)); break; }
   case BTriggerVar::cVarTypeExposedScript: { BTriggerVarExposedScript::releaseInstance(reinterpret_cast<BTriggerVarExposedScript*>(pVar)); break; }
   case BTriggerVar::cVarTypeKBBase: { BTriggerVarKBBase::releaseInstance(reinterpret_cast<BTriggerVarKBBase*>(pVar)); break; }
   case BTriggerVar::cVarTypeKBBaseList: { BTriggerVarKBBaseList::releaseInstance(reinterpret_cast<BTriggerVarKBBaseList*>(pVar)); break; }
   case BTriggerVar::cVarTypeDataScalar: { BTriggerVarDataScalar::releaseInstance(reinterpret_cast<BTriggerVarDataScalar*>(pVar)); break; }
   case BTriggerVar::cVarTypeKBBaseQuery: { BTriggerVarKBBaseQuery::releaseInstance(reinterpret_cast<BTriggerVarKBBaseQuery*>(pVar)); break; }
   case BTriggerVar::cVarTypeDesignLine: { BTriggerVarDesignLine::releaseInstance(reinterpret_cast<BTriggerVarDesignLine*>(pVar)); break; }
   case BTriggerVar::cVarTypeLocStringID: { BTriggerVarLocStringID::releaseInstance(reinterpret_cast<BTriggerVarLocStringID*>(pVar)); break; }
   case BTriggerVar::cVarTypeLeader: { BTriggerVarLeader::releaseInstance(reinterpret_cast<BTriggerVarLeader*>(pVar)); break; }
   case BTriggerVar::cVarTypeCinematic: { BTriggerVarCinematic::releaseInstance(reinterpret_cast<BTriggerVarCinematic*>(pVar)); break; }
   case BTriggerVar::cVarTypeTalkingHead: { BTriggerVarTalkingHead::releaseInstance(reinterpret_cast<BTriggerVarTalkingHead*>(pVar)); break; }
   case BTriggerVar::cVarTypeFlareType: { BTriggerVarFlareType::releaseInstance(reinterpret_cast<BTriggerVarFlareType*>(pVar)); break; }
   case BTriggerVar::cVarTypeCinematicTag: { BTriggerVarCinematicTag::releaseInstance(reinterpret_cast<BTriggerVarCinematicTag*>(pVar)); break; }
   case BTriggerVar::cVarTypeIconType: { BTriggerVarIconType::releaseInstance(reinterpret_cast<BTriggerVarIconType*>(pVar)); break; }
   case BTriggerVar::cVarTypeDifficulty: { BTriggerVarDifficulty::releaseInstance(reinterpret_cast<BTriggerVarDifficulty*>(pVar)); break; }
   case BTriggerVar::cVarTypeInteger: { BTriggerVarInteger::releaseInstance(reinterpret_cast<BTriggerVarInteger*>(pVar)); break; }
   case BTriggerVar::cVarTypeHUDItem: { BTriggerVarHUDItem::releaseInstance(reinterpret_cast<BTriggerVarHUDItem*>(pVar)); break; }
   case BTriggerVar::cVarTypeFlashableUIItem: { BTriggerVarFlashableUIItem::releaseInstance(reinterpret_cast<BTriggerVarFlashableUIItem*>(pVar)); break; }
   case BTriggerVar::cVarTypeControlType: { BTriggerVarControlType::releaseInstance(reinterpret_cast<BTriggerVarControlType*>(pVar)); break; }
   case BTriggerVar::cVarTypeUIButton: { BTriggerVarUIButton::releaseInstance(reinterpret_cast<BTriggerVarUIButton*>(pVar)); break; }
   case BTriggerVar::cVarTypeMissionType: { BTriggerVarMissionType::releaseInstance(reinterpret_cast<BTriggerVarMissionType*>(pVar)); break; }
   case BTriggerVar::cVarTypeMissionState: { BTriggerVarMissionState::releaseInstance(reinterpret_cast<BTriggerVarMissionState*>(pVar)); break; }
   case BTriggerVar::cVarTypeMissionTargetType: { BTriggerVarMissionTargetType::releaseInstance(reinterpret_cast<BTriggerVarMissionTargetType*>(pVar)); break; }
   case BTriggerVar::cVarTypeIntegerList: { BTriggerVarIntegerList::releaseInstance(reinterpret_cast<BTriggerVarIntegerList*>(pVar)); break; }
   case BTriggerVar::cVarTypeBidType: { BTriggerVarBidType::releaseInstance(reinterpret_cast<BTriggerVarBidType*>(pVar)); break; }
   case BTriggerVar::cVarTypeBidState: { BTriggerVarBidState::releaseInstance(reinterpret_cast<BTriggerVarBidState*>(pVar)); break; }
   case BTriggerVar::cVarTypeBuildingCommandState: { BTriggerVarBuildingCommandState::releaseInstance(reinterpret_cast<BTriggerVarBuildingCommandState*>(pVar)); break; }
   case BTriggerVar::cVarTypeVector: { BTriggerVarVector::releaseInstance(reinterpret_cast<BTriggerVarVector*>(pVar)); break; }
   case BTriggerVar::cVarTypeVectorList: { BTriggerVarVectorList::releaseInstance(reinterpret_cast<BTriggerVarVectorList*>(pVar)); break; }
   case BTriggerVar::cVarTypePlacementRule: { BTriggerVarPlacementRule::releaseInstance(reinterpret_cast<BTriggerVarPlacementRule*>(pVar)); break; }
   case BTriggerVar::cVarTypeKBSquad: { BTriggerVarKBSquad::releaseInstance(reinterpret_cast<BTriggerVarKBSquad*>(pVar)); break; }
   case BTriggerVar::cVarTypeKBSquadList: { BTriggerVarKBSquadList::releaseInstance(reinterpret_cast<BTriggerVarKBSquadList*>(pVar)); break; }
   case BTriggerVar::cVarTypeKBSquadQuery: { BTriggerVarKBSquadQuery::releaseInstance(reinterpret_cast<BTriggerVarKBSquadQuery*>(pVar)); break; }
   case BTriggerVar::cVarTypeAISquadAnalysis: { BTriggerVarAISquadAnalysis::releaseInstance(reinterpret_cast<BTriggerVarAISquadAnalysis*>(pVar)); break; }
   case BTriggerVar::cVarTypeAISquadAnalysisComponent: { BTriggerVarAISquadAnalysisComponent::releaseInstance(reinterpret_cast<BTriggerVarAISquadAnalysisComponent*>(pVar)); break; }
   case BTriggerVar::cVarTypeKBSquadFilterSet: { BTriggerVarKBSquadFilterSet::releaseInstance(reinterpret_cast<BTriggerVarKBSquadFilterSet*>(pVar)); break; }
   case BTriggerVar::cVarTypeChatSpeaker: { BTriggerVarChatSpeaker::releaseInstance(reinterpret_cast<BTriggerVarChatSpeaker*>(pVar)); break; }
   case BTriggerVar::cVarTypeRumbleType: { BTriggerVarRumbleType::releaseInstance(reinterpret_cast<BTriggerVarRumbleType*>(pVar)); break; }
   case BTriggerVar::cVarTypeRumbleMotor: { BTriggerVarRumbleMotor::releaseInstance(reinterpret_cast<BTriggerVarRumbleMotor*>(pVar)); break; }
   case BTriggerVar::cVarTypeTechDataCommandType: { BTriggerVarTechDataCommandType::releaseInstance(reinterpret_cast<BTriggerVarTechDataCommandType*>(pVar)); break; }
   case BTriggerVar::cVarTypeSquadDataType: { BTriggerVarSquadDataType::releaseInstance(reinterpret_cast<BTriggerVarSquadDataType*>(pVar)); break; }
   case BTriggerVar::cVarTypeEventType: { BTriggerVarEventType::releaseInstance(reinterpret_cast<BTriggerVarEventType*>(pVar)); break; }
   case BTriggerVar::cVarTypeTimeList: { BTriggerVarTimeList::releaseInstance(reinterpret_cast<BTriggerVarTimeList*>(pVar)); break; }
   case BTriggerVar::cVarTypeDesignLineList: { BTriggerVarDesignLineList::releaseInstance(reinterpret_cast<BTriggerVarDesignLineList*>(pVar)); break; }
   case BTriggerVar::cVarTypeGameStatePredicate: { BTriggerVarGameStatePredicate::releaseInstance(reinterpret_cast<BTriggerVarGameStatePredicate*>(pVar)); break; }
   case BTriggerVar::cVarTypeFloatList: { BTriggerVarFloatList::releaseInstance(reinterpret_cast<BTriggerVarFloatList*>(pVar)); break; }
   case BTriggerVar::cVarTypeUILocationMinigame: { BTriggerVarUILocationMinigame::releaseInstance(reinterpret_cast<BTriggerVarUILocationMinigame*>(pVar)); break; }
   case BTriggerVar::cVarTypeSquadFlag: { BTriggerVarSquadFlag::releaseInstance(reinterpret_cast<BTriggerVarSquadFlag*>(pVar)); break; }
   case BTriggerVar::cVarTypeConcept: { BTriggerVarConcept::releaseInstance(reinterpret_cast<BTriggerVarConcept*>(pVar)); break; }
   case BTriggerVar::cVarTypeConceptList: { BTriggerVarConceptList::releaseInstance(reinterpret_cast<BTriggerVarConceptList*>(pVar)); break; }
   case BTriggerVar::cVarTypeUserClassType: { BTriggerVarUserClassType::releaseInstance(reinterpret_cast<BTriggerVarUserClassType*>(pVar)); break; }

   // NEWTRIGGERVARTYPE
   // Add your case statement for your trigger var type here that releases your var to the correct block pool.

   default: BASSERTM(false, "You are trying to release an instance of an unknown trigger var type.  Botched.");
   }
}


//==============================================================================
// BTriggerScript::getTrigger
//==============================================================================
BTrigger* BTriggerScript::getTrigger(BTriggerID triggerID)
{
   BASSERT(triggerID < mTriggers.getSize());
   if (triggerID < mTriggers.getSize())
      return (mTriggers[triggerID]);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
uint BTriggerScript::getTriggerScriptTypeEnum(const BSimString& scriptTypeName)
{
   if (scriptTypeName == "Scenario")
      return (BTriggerScript::cTypeScenario);
   else if (scriptTypeName == "Power")
      return (BTriggerScript::cTypePower);
   else if (scriptTypeName == "Ability")
      return (BTriggerScript::cTypeAbility);
   else if (scriptTypeName == "TriggerScript")
      return (BTriggerScript::cTypeTriggerScript);
   else
      return (BTriggerScript::cTypeInvalid);
}
#endif


//==============================================================================
// BTriggerScript::update
//==============================================================================
void BTriggerScript::update(DWORD currentUpdateTime)
{
   syncTriggerData("BTriggerScript::update() - updating trigger script ID = ", static_cast<DWORD>(getID()));
   SCOPEDSAMPLE(BTriggerScript_update);

   // Make sure we're active, and flag our script as currently updating.
   BASSERT(isActive());

   if (mFlagPaused)
      return;

   mbUpdating = true;

   // As the first task after we set the update flag to true, purge any cached invalidated entityID's from our lists.
   uint numInvalidatedEntityIDs = mInvalidatedEntityIDs.getSize();
   for (uint i=0; i<numInvalidatedEntityIDs; i++)
      invalidateEntityID(mInvalidatedEntityIDs[i]);
   BASSERT(mInvalidatedEntityIDs.getSize() == numInvalidatedEntityIDs);
   mInvalidatedEntityIDs.clear();

   // Reset the evaluate count.
   uint numTriggers = mTriggers.getSize();
   for (uint i=0; i<numTriggers; i++)
   {
      BASSERT(mTriggers[i]);
      mTriggers[i]->resetEvaluateCount();
   }

   mTriggerEvalList = mActiveTriggers;
   // NOTE:  Do not cache off mTriggerEvalList.getNumber() because it may increase while we are in the loop and we want to get to those latecomers.

   while (!mTriggerEvalList.isEmpty())
   {
      BTrigger*  pTrigger = getTrigger(mTriggerEvalList[0]);
      BASSERT(pTrigger);
      mTriggerEvalList.removeIndex(0);
      if (pTrigger->hasEvaluationsRemaining() && pTrigger->timeToEvaluate(currentUpdateTime))
      {
         #ifndef BUILD_FINAL
            gTriggerManager.setCurrentTriggerID(pTrigger->getEditorID());
         #endif

         // We update these outside of evaluateConditions() because that function may early bail and not update all of them.
         // TODO:  Does this need to be outside of timeToEvaluate()??
         pTrigger->updateAsyncConditions();

         // If we evaluate to true, we fire the trigger (regardless of the type).
         // However, if we're a conditional trigger, we fire even if we're false (and pass that in)
         bool conditionEvalResult = pTrigger->evaluateConditions(currentUpdateTime);
         if (conditionEvalResult || pTrigger->isConditional())
            pTrigger->fireEffects(conditionEvalResult);

         #ifndef BUILD_FINAL
            gTriggerManager.setCurrentTriggerID(cInvalidTriggerID);
         #endif
      }
   }

   if (mActiveTriggers.isEmpty())
      markForCleanup();

   mbUpdating = false;
}

//==============================================================================
//==============================================================================
bool BTriggerScript::save(BStream* pStream, int saveType) const
{
   GFWRITESTRING(pStream, BSimString, mFileName, 200);
   GFWRITEVAR(pStream, BTriggerScriptID, mID);
   GFWRITEVAR(pStream, int16, mFileDir);

   #ifndef BUILD_FINAL
      GFWRITESTRING(pStream, BSimString, mName, 200);
      GFWRITEVAR(pStream, uint8, mType);
   #else
      BSimString name;
      GFWRITESTRING(pStream, BSimString, name, 200);
      GFWRITEVAL(pStream, uint8, 0);
   #endif

   // Vars
   uint varCount = mTriggerVars.size();
   GFWRITEVAR(pStream, uint, varCount);
   GFVERIFYCOUNT(varCount,500000);
   for (uint i=0; i<varCount; i++)
   {
      BTriggerVar* pVar = mTriggerVars[i];
      if (pVar)
      {
         GFWRITEVAR(pStream, uint, i);
         BTriggerVarType varType = pVar->getType();
         GFWRITEVAR(pStream, BTriggerVarType, varType);
         if (!pVar->save(pStream, saveType))
            return false;
      }
   }
   GFWRITEVAL(pStream, uint, UINT_MAX);

   // Triggers
   uint16 triggerCount = (uint16)mTriggers.size();
   GFVERIFYCOUNT(triggerCount, 10000);
   GFWRITEVAR(pStream, uint16, triggerCount);
   for (uint16 i=0; i<triggerCount; i++) \
   {
      if (!mTriggers[i]->save(pStream, saveType))
         return false;
   }

   // Externals
//-- FIXING PREFIX BUG ID 2116
   const BTriggerScriptExternals* pExternals = gTriggerManager.getTriggerScriptExternal(mExternalsIndex);
//--
   bool haveExternals = (pExternals != NULL);
   GFWRITEVAR(pStream, bool, haveExternals);
   if (pExternals && !pExternals->save(pStream, saveType))
      return false;

   GFWRITEARRAY(pStream, BEntityID, mInvalidatedEntityIDs, uint16, 10000);
   GFWRITEARRAY(pStream, BTriggerID, mActiveTriggers, uint16, 10000);
   GFWRITEARRAY(pStream, BTriggerID, mTriggerEvalList, uint16, 10000);

   GFWRITEBITBOOL(pStream, mbActive);
   GFWRITEBITBOOL(pStream, mbUpdating);
   GFWRITEBITBOOL(pStream, mbCleanup);
   GFWRITEBITBOOL(pStream, mbLogEffects);

   return true;
}

//==============================================================================
//==============================================================================
bool BTriggerScript::load(BStream* pStream, int saveType)
{
   GFREADSTRING(pStream, BSimString, mFileName, 200);
   GFREADVAR(pStream, BTriggerScriptID, mID);
   GFREADVAR(pStream, int16, mFileDir);

   #ifndef BUILD_FINAL
      GFREADSTRING(pStream, BSimString, mName, 200);
      GFREADVAR(pStream, uint8, mType);
   #else
      BSimString name;
      GFREADSTRING(pStream, BSimString, name, 200);
      uint8 type = 0;
      GFREADVAR(pStream, uint8, type);
   #endif

   // Vars
   uint varCount;
   GFREADVAR(pStream, uint, varCount);
   GFVERIFYCOUNT(varCount, 500000);
   if (!mTriggerVars.setNumber(varCount))
      return false;
   for (uint i=0; i<varCount; i++)
      mTriggerVars[i] = NULL;
   uint loadedCount=0;
   uint varID;
   GFREADVAR(pStream, uint, varID);
   while (varID != UINT_MAX)
   {
      if (varID >= varCount)
      {
         GFERROR("GameFile Error: invalid trigger var ID %u", varID);
         return false;
      }
      if (loadedCount >= varCount)
      {
         GFERROR("GameFile Error: too many trigger vars");
         return false;
      }
      BTriggerVarType varType;
      GFREADVAR(pStream, BTriggerVarType, varType);
      BTriggerVar* pVar = getTriggerVarInstance(varType);
      if (!pVar)
         return false;
      pVar->setID(varID);
      mTriggerVars[varID] = pVar;
      if (!pVar->load(pStream, saveType))
         return false;
      if (varType == BTriggerVar::cVarTypeUnitList)
         mTriggerVarUnitLists.add(pVar->asUnitList());
      else if (varType == BTriggerVar::cVarTypeSquadList)
         mTriggerVarSquadLists.add(pVar->asSquadList());
      else if (varType == BTriggerVar::cVarTypeObjectList)
         mTriggerVarObjectLists.add(pVar->asObjectList());
      loadedCount++;
      GFREADVAR(pStream, uint, varID);
   }

   // Triggers
   uint16 triggerCount;
   GFREADVAR(pStream, uint16, triggerCount);
   GFVERIFYCOUNT(triggerCount, 10000);
   for (uint16 i=0; i<triggerCount; i++)
   {
      BTrigger* pTrigger = BTrigger::getInstance();
      if (!pTrigger)
         return false;
      pTrigger->setParentTriggerScript(this);
      mTriggers.add(pTrigger);
      if (!pTrigger->load(pStream, saveType))
         return false;
   }

   // Externals
   bool haveExternals;
   GFREADVAR(pStream, bool, haveExternals);
   BTriggerScriptExternals* pExternals = gTriggerManager.getTriggerScriptExternal(mExternalsIndex);
   if (haveExternals)
   {
      if (!pExternals)
      {
         uint index = UINT_MAX;
         BTriggerScriptExternals* pExternals = gTriggerManager.acquireTriggerScriptExternal(index);
         BFATAL_ASSERT(index <= UINT16_MAX);
         mExternalsIndex = static_cast<uint16>(index);
         pExternals->clearAll();
      }
      if (!pExternals->load(pStream, saveType))
         return false;
   }
   else
   {
      if (pExternals)
         releaseExternals();
   }

   if (mGameFileVersion < 2)
   {
      BSmallDynamicSimArray<BTriggerGroup> junk;
      GFREADCLASSARRAY(pStream, saveType, junk, uint16, 1000);
   }

   GFREADARRAY(pStream, BEntityID, mInvalidatedEntityIDs, uint16, 10000);
   GFREADARRAY(pStream, BTriggerID, mActiveTriggers, uint16, 10000);
   GFREADARRAY(pStream, BTriggerID, mTriggerEvalList, uint16, 10000);

   GFREADBITBOOL(pStream, mbActive);
   GFREADBITBOOL(pStream, mbUpdating);
   GFREADBITBOOL(pStream, mbCleanup);
   GFREADBITBOOL(pStream, mbLogEffects);

   return true;
}

//==============================================================================
//==============================================================================
bool BTriggerScriptExternals::save(BStream* pStream, int saveType) const
{
   GFWRITEBITBOOL(pStream, mbPlayerID);
   GFWRITEBITBOOL(pStream, mbProtoPowerID);
   GFWRITEBITBOOL(pStream, mbSquadID);
   GFWRITEBITBOOL(pStream, mbSquadIDs);
   GFWRITEBITBOOL(pStream, mbUnitID);
   GFWRITEBITBOOL(pStream, mbUnitIDs);
   GFWRITEBITBOOL(pStream, mbCost);
   GFWRITEBITBOOL(pStream, mbLocation);
   GFWRITEBITBOOL(pStream, mbLocationList);
   GFWRITEBITBOOL(pStream, mbFlags);
   GFWRITEBITBOOL(pStream, mbFloat);
   if (mbLocation)
      GFWRITEVECTOR(pStream, mLocation);
   if (mbLocationList)
      GFWRITEVECTORARRAY(pStream, mLocationList, uint16, 2000);
   if (mbSquadIDs)
      GFWRITEARRAY(pStream, BEntityID, mSquadIDs, uint16, 2000);
   if (mbUnitIDs)
      GFWRITEARRAY(pStream, BEntityID, mUnitIDs, uint16, 2000);
   if (mbCost)
      GFWRITECLASS(pStream, saveType, mCost);
   if (mbFlags)
      GFWRITEBITARRAY(pStream, mFlags, uint8, 32);
   if (mbFloat)
      GFWRITEVAR(pStream, float, mFloat);
   if (mbSquadID)
      GFWRITEVAR(pStream, BEntityID, mSquadID);
   if (mbUnitID)
      GFWRITEVAR(pStream, BEntityID, mUnitID);
   if (mbPlayerID)
      GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   if (mbProtoPowerID)
      GFWRITEVAR(pStream, BProtoPowerID, mProtoPowerID);
   return true;
}

//==============================================================================
//==============================================================================
bool BTriggerScriptExternals::load(BStream* pStream, int saveType)
{
   GFREADBITBOOL(pStream, mbPlayerID);
   GFREADBITBOOL(pStream, mbProtoPowerID);
   GFREADBITBOOL(pStream, mbSquadID);
   GFREADBITBOOL(pStream, mbSquadIDs);
   GFREADBITBOOL(pStream, mbUnitID);
   GFREADBITBOOL(pStream, mbUnitIDs);
   GFREADBITBOOL(pStream, mbCost);
   GFREADBITBOOL(pStream, mbLocation);
   GFREADBITBOOL(pStream, mbLocationList);
   GFREADBITBOOL(pStream, mbFlags);
   GFREADBITBOOL(pStream, mbFloat);
   if (mbLocation)
      GFREADVECTOR(pStream, mLocation);
   if (mbLocationList)
      GFREADVECTORARRAY(pStream, mLocationList, uint16, 2000);
   if (mbSquadIDs)
      GFREADARRAY(pStream, BEntityID, mSquadIDs, uint16, 2000);
   if (mbUnitIDs)
      GFREADARRAY(pStream, BEntityID, mUnitIDs, uint16, 2000);
   if (mbCost)
      GFREADCLASS(pStream, saveType, mCost);
   if (mbFlags)
      GFREADBITARRAY(pStream, mFlags, uint8, 32);
   if(mbFloat)
      GFREADVAR(pStream, float, mFloat);
   if (mbSquadID)
      GFREADVAR(pStream, BEntityID, mSquadID);
   if (mbUnitID)
      GFREADVAR(pStream, BEntityID, mUnitID);
   if (mbPlayerID)
      GFREADVAR(pStream, BPlayerID, mPlayerID);
   if (mbProtoPowerID)
      GFREADVAR(pStream, BProtoPowerID, mProtoPowerID);
   return true;
}
