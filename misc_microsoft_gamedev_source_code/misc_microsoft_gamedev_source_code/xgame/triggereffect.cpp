//==============================================================================
// triggereffect.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "binkInterface.h"
#include "breakpoint.h"
#include "commandmanager.h"
#include "commands.h"
#include "database.h"
#include "designobjectsmanager.h"
#include "entityactionidle.h"
#include "EntityGrouper.h"
#include "gamemode.h"
#include "gamesettings.h"
#include "generaleventmanager.h"
#include "hintengine.h"
#include "minimap.h"
#include "player.h"
#include "protoobject.h"
#include "protopower.h"
#include "protosquad.h"
#include "prototech.h"
#include "selectionmanager.h"
#include "simhelper.h"
#include "SimOrderManager.h"
#include "squad.h"
#include "squadplotter.h"
#include "syncmacros.h"
#include "tactic.h"
#include "team.h"
#include "techtree.h"
#include "triggereffect.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "unit.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "visiblemap.h"
#include "world.h"
#include "soundmanager.h"
#include "syncmacros.h"
#include "game.h"
#include "gamedirectories.h"
#include "squadactiontransport.h"
#include "techeffect.h"
#include "alert.h"
#include "ability.h"
#include "uimanager.h"
#include "uiwidgets.h"
#include "renderthread.h"
#include "squadactionreinforce.h"
#include "SimOrderManager.h"
#include "modemanager.h"
#include "worldsoundmanager.h"
#include "camera.h"
#include "configsGame.h"
#include "uigame.h"
#include "terrain.h"
#include "squadactioncarpetbomb.h"
#include "pather.h"
#include "unitactionmoveair.h"
#include "timermanager.h"
#include "unitactionrangedattack.h"
#include "statsManager.h"
#include "vincehelper.h"
#include "Formation2.h"
#include "mathutil.h"
#include "corpsemanager.h"
#include "transitionManager.h"
#include "unitactiontentacledormant.h"
#include "unitactionhotdrop.h"
#include "cinematic.h"
#include "scoremanager.h"
#include "powermanager.h"
#include "power.h"
#include "powertypes.h"
#include "powercleansing.h"
#include "unitactiontowerwall.h"
#include "achievementmanager.h"

// xsystem
#include "xmlreader.h" 
#include "syncmacros.h"

//xLive - so that it can query if a mp game is active
#include "liveSystem.h"

IMPLEMENT_FREELIST(BTriggerEffect, 6, &gSimHeap);
 
GFIMPLEMENTVERSION(BTriggerEffect, 2);

#define TE_RANDOM_LOCATION_SEARCH_MAX 1000

//==============================================================================
// Returns false if the entity is a spartan hijacked unit/squad.  This is for
// filtering these units out of entity lists to give commands to.
//==============================================================================
bool filterOutSpartanHijackedEntities(BEntity* pEntity)
{
   BSquad* pSquad = pEntity->getSquad();
   if (!pSquad)
   {
      BUnit* pUnit = pEntity->getUnit();
      if (pUnit)
         pSquad = pUnit->getParentSquad();
   }

   if (pSquad && pSquad->getFlagSpartanContainer())
      return false;

   return true;
}

//==============================================================================
// BTriggerEffect::loadFromXML(BXMLNode node)
//==============================================================================
bool BTriggerEffect::loadFromXML(BXMLNode node)
{
   // Get the trigger effect DBID.
   mDBID = -1;
   if (!node.getAttribValueAsLong("DBID", mDBID))
      return (false);
   if (mDBID == -1)
      return (false);

#ifndef BUILD_FINAL
   if (mDBID >= 0 && mDBID < cNumTriggerEffectTypes && gTriggerManager.mTriggerEffectNames[mDBID].isEmpty())
      node.getAttribValueAsString("Type", gTriggerManager.mTriggerEffectNames[mDBID]);
#endif

   // Get the trigger version.
   DWORD versionDWORD = cTEVersionInvalid;
   if (!node.getAttribValueAsDWORD("Version", versionDWORD))
      return (false);
   if (versionDWORD == cTEVersionInvalid)
      return (false);
   mVersion = versionDWORD;

   // Get our mapping for the sig ids.
   DWORD maxSigID = 0;
   long numTriggerVarNodes = node.getNumberChildren();
   for (long i=0; i<numTriggerVarNodes; i++)
   {
      BXMLNode varNode(node.getChild(i));
      DWORD varDBIDDWORD = 0;
      if (varNode.getAttribValueAsDWORD("SigID", varDBIDDWORD) && varDBIDDWORD > maxSigID)
         maxSigID = varDBIDDWORD;
   }

   // If we have some variables, add one slot in the array for the invalid sigid.  (KIND OF A HACK)
   //if (maxSigID > 0)
   //   maxSigID++;

   mEffectVars.resize(maxSigID);
   for (uint i=0; i<maxSigID; i++)
      mEffectVars[i] = NULL;

   // Get all the inputs and outputs for this trigger effect
   for (long i=0; i<numTriggerVarNodes; i++)
   {
      BXMLNode varNode(node.getChild(i));
      
      // Get the trigger var ID
      long triggerVarIDLong = BTriggerVar::cVarIDInvalid;
      if (!varNode.getTextAsLong(triggerVarIDLong))
         return (false);
      BTRIGGER_ASSERT(triggerVarIDLong < BTriggerVar::cVarIDMax);
      BTriggerVarID triggerVarID = (BTriggerVarID)triggerVarIDLong;
      BTriggerVar* pVar = getParentTriggerScript()->getTriggerVar(triggerVarID);
      if (!pVar)
         return (false);

      // Get the signature ID of the var (the DBID) that uniquely identifies it within the signature.
      DWORD varDBIDDWORD = BTriggerVar::cVarSigIDInvalid;
      if (!varNode.getAttribValueAsDWORD("SigID", varDBIDDWORD))
         return (false);
      if (varDBIDDWORD == BTriggerVar::cVarSigIDInvalid)
         return (false);
      BTRIGGER_ASSERT(varDBIDDWORD < BTriggerVar::cVarSigIDMax);
      BTriggerVarSigID varDBID = static_cast<BTriggerVarSigID>(varDBIDDWORD);

      mEffectVars[varDBID-1] = pVar;
   }

   return (true);
}


//==============================================================================
// BTriggerEffect::fire()
//==============================================================================
void BTriggerEffect::fire()
{
   SCOPEDSAMPLE(BTriggerEffect_fire);

   #ifndef BUILD_FINAL
      if (mpParentTriggerScript->getLogEffects())
         gConsole.output(cChannelTriggers, ". TriggerEffect %d '%s'", mDBID, ((mDBID > 0 && mDBID < cNumTriggerEffectTypes) ? gTriggerManager.mTriggerEffectNames[mDBID].getPtr() : ""));
   #endif

   switch (mDBID)
   {
   case cTETriggerActivate:               { teTriggerActivate(); break; }
   case cTETriggerDeactivate:             { teTriggerDeactivate(); break; }
   case cTEPlaySound:                     { tePlaySound(); break; }
   case cTEPlayRelationSound:             { tePlayRelationSound(); break; }
   case cTECreateObject:                  { teCreateObject(); break; }
   case cTECreateSquad:                   { teCreateSquad(); break; }
   case cTEKill:                          { teKill(); break; }
   case cTEDestroy:                       { teDestroy(); break; }
   case cTEPayCost:                       { tePayCost(); break; }
   case cTERefundCost:                    { teRefundCost(); break; }
   case cTECountIncrement:                { teCountIncrement(); break; }
   case cTECountDecrement:                { teCountDecrement(); break; }
   case cTERandomLocation:                { teRandomLocation(); break; }
   case cTEShutdown:                      { teShutdown(); break; }
   case cTELaunchProjectile:              { teLaunchProjectile(); break; }
   case cTELocationTieToGround:           { teLocationTieToGround(); break; }
   case cTELocationAdjust:                { teLocationAdjust(); break; }
   case cTETechActivate:                  { teTechActivate(); break; }
   case cTETechDeactivate:                { teTechDeactivate(); break; }
   case cTEUnload:                        { teUnload(); break; }
   case cTEMove:                          { teMove(); break; }
   case cTETransportSquads:               { teTransportSquads(); break; }
   case cTECarpetBomb:                    { teCarpetBomb(); break; }
   case cTEAttachmentAddType:             { teAttachmentAddType(); break; }
   case cTEAttachmentRemoveAll:           { teAttachmentRemoveAll(); break; }
   case cTEUsePower:                      { teUsePower(); break; }
   case cTEGetClosestPowerSquad:          { teGetClosestPowerSquad(); break; }
   case cTESetUIPowerRadius:              { teSetUIPowerRadius(); break; }
   case cTEAttachmentRemoveType:          { teAttachmentRemoveType(); break; }
   case cTEInputUILocation:               { teInputUILocation(); break; }
   case cTECopyTech:                      { teCopyTech(); break; }
   case cTECopyTechStatus:                { teCopyTechStatus(); break; }
   case cTECopyOperator:                  { teCopyOperator(); break; }
   case cTECopyProtoObject:               { teCopyProtoObject(); break; }
   case cTECopyObjectType:                { teCopyObjectType(); break; }
   case cTECopyProtoSquad:                { teCopyProtoSquad(); break; }
   case cTECopySound:                     { teCopySound(); break; }
   case cTECopyEntity:                    { teCopyEntity(); break; }
   case cTECopyEntityList:                { teCopyEntityList(); break; }
   case cTECopyCost:                      { teCopyCost(); break; }
   case cTECopyDistance:                  { teCopyDistance(); break; }
   case cTECopyTime:                      { teCopyTime(); break; }
   case cTECopyPlayer:                    { teCopyPlayer(); break; }
   case cTECopyCount:                     { teCopyCount(); break; }
   case cTECopyLocation:                  { teCopyLocation(); break; }
   case cTECopyPercent:                   { teCopyPercent(); break; }
   case cTECopyHitpoints:                 { teCopyHitpoints(); break; }
   case cTECopyBool:                      { teCopyBool(); break; }
   case cTECopyFloat:                     { teCopyFloat(); break; }
   case cTEGetDistanceUnitUnit:           { teGetDistanceUnitUnit(); break; }
   case cTEGetDistanceUnitLocation:       { teGetDistanceUnitLocation(); break; }
   case cTEGetUnits:                      { teGetUnits(); break; }
   case cTEGetSquads:                     { teGetSquads(); break; }
   case cTEWork:                          { teWork(); break; }
   case cTEIteratorPlayerList:            { teIteratorPlayerList(); break; }
   case cTEIteratorTeamList:              { teIteratorTeamList(); break; }
   case cTEGetTeams:                      { teGetTeams(); break; }
   case cTEGetTeamPlayers:                { teGetTeamPlayers(); break; }
   case cTEPlayerListAdd:                 { tePlayerListAdd(); break; }
   case cTEPlayerListRemove:              { tePlayerListRemove(); break; }
   case cTETeamListAdd:                   { teTeamListAdd(); break; }
   case cTETeamListRemove:                { teTeamListRemove(); break; }
   case cTESetPlayerState:                { teSetPlayerState(); break; }
   case cTEFlareMinimapSpoof:             { teFlareMinimapSpoof(); break; }
   case cTEObjectiveComplete:             { teObjectiveComplete(); break; }
   case cTEObjectiveUserMessage:          { teObjectiveUserMessage(); break; }
   case cTEUserMessage:                   { teUserMessage(); break; }
   case cTEFlareMinimapNormal:            { teFlareMinimapNormal(); break; }
   case cTEChangeOwner:                   { teChangeOwner(); break; }
   case cTEMKTest:                        { teMKTest(); break; }
   case cTECopyUnit:                      { teCopyUnit(); break; }
   case cTECopyUnitList:                  { teCopyUnitList(); break; }
   case cTECopySquad:                     { teCopySquad(); break; }
   case cTECopySquadList:                 { teCopySquadList(); break; }
   case cTEUnitListGetSize:               { teUnitListGetSize(); break; }
   case cTESquadListGetSize:              { teSquadListGetSize(); break; }
   case cTEUnitListAdd:                   { teUnitListAdd(); break; }
   case cTESquadListAdd:                  { teSquadListAdd(); break; }
   case cTEUnitListRemove:                { teUnitListRemove(); break; }
   case cTESquadListRemove:               { teSquadListRemove(); break; }
   case cTEIteratorUnitList:              { teIteratorUnitList(); break; }
   case cTEIteratorSquadList:             { teIteratorSquadList(); break; }
   case cTECreateUnit:                    { teCreateUnit(); break; }
   case cTEPlayAnimationUnit:             { tePlayAnimationUnit(); break; }
   case cTEPlayAnimationSquad:            { tePlayAnimationSquad(); break; }
   case cTEInputUIUnit:                   { teInputUIUnit(); break; }
   case cTEInputUISquad:                  { teInputUISquad(); break; }
   case cTETimeUserMessage:               { teTimeUserMessage(); break; }                                          
   case cTECopyColor:                     { teCopyColor(); break; }
   case cTECopyString:                    { teCopyString(); break; }
   case cTETransform:                     { teTransform(); break; }
   case cTEGetPlayers:                    { teGetPlayers(); break; }
   case cTEGetHealth:                     { teGetHealth(); break; }
   case cTERandomCount:                   { teRandomCount(); break; }                                             
   case cTEMathCount:                     { teMathCount(); break; }
   case cTEMathHitpoints:                 { teMathHitpoints(); break; }
   case cTEAsString:                      { teAsString(); break; }
   case cTEObjectiveDisplay:              { teObjectiveDisplay(); break; }
   case cTECalculatePercentCount:         { teCalculatePercentCount(); break; }
   case cTECalculatePercentHitpoints:     { teCalculatePercentHitpoints(); break; }
   case cTECalculatePercentTime:          { teCalculatePercentTime(); break; }
   case cTELerpCount:                     { teLerpCount(); break; }
   case cTELerpColor:                     { teLerpColor(); break; }
   case cTEGetLocation:                   { teGetLocation(); break; }
   case cTEMathPercent:                   { teMathPercent(); break; }
   case cTEGetOwner:                      { teGetOwner(); break; }
   case cTEDebugVarTech:                  { teDebugVarTech(); break; }
   case cTEDebugVarTechStatus:            { teDebugVarTechStatus(); break; }
   case cTEDebugVarOperator:              { teDebugVarOperator(); break; }
   case cTEDebugVarProtoObject:           { teDebugVarProtoObject(); break; }
   case cTEDebugVarObjectType:            { teDebugVarObjectType(); break; }
   case cTEDebugVarProtoSquad:            { teDebugVarProtoSquad(); break; }
   case cTEDebugVarSound:                 { teDebugVarSound(); break; }
   case cTEDebugVarDistance:              { teDebugVarDistance(); break; }
   case cTEDebugVarTime:                  { teDebugVarTime(); break; }
   case cTEDebugVarPlayer:                { teDebugVarPlayer(); break; }
   case cTEDebugVarCount:                 { teDebugVarCount(); break; }
   case cTEDebugVarLocation:              { teDebugVarLocation(); break; }
   case cTEDebugVarUILocation:            { teDebugVarUILocation(); break; }
   case cTEDebugVarCost:                  { teDebugVarCost(); break; }
   case cTEDebugVarAnimType:              { teDebugVarAnimType(); break; }
   case cTEDebugVarPercent:               { teDebugVarPercent(); break; }
   case cTEDebugVarHitpoints:             { teDebugVarHitpoints(); break; }
   case cTEDebugVarPower:                 { teDebugVarPower(); break; }
   case cTEDebugVarBool:                  { teDebugVarBool(); break; }
   case cTEDebugVarFloat:                 { teDebugVarFloat(); break; }
   case cTEDebugVarIterator:              { teDebugVarIterator(); break; }
   case cTEDebugVarTeam:                  { teDebugVarTeam(); break; }
   case cTEDebugVarPlayerList:            { teDebugVarPlayerList(); break; }
   case cTEDebugVarTeamList:              { teDebugVarTeamList(); break; }
   case cTEDebugVarPlayerState:           { teDebugVarPlayerState(); break; }
   case cTEDebugVarObjective:             { teDebugVarObjective(); break; }
   case cTEDebugVarUnit:                  { teDebugVarUnit(); break; }
   case cTEDebugVarUnitList:              { teDebugVarUnitList(); break; }
   case cTEDebugVarSquad:                 { teDebugVarSquad(); break; }
   case cTEDebugVarUIUnit:                { teDebugVarUIUnit(); break; }
   case cTEDebugVarUISquad:               { teDebugVarUISquad(); break; }
   case cTEDebugVarString:                { teDebugVarString(); break; }
   case cTEDebugVarColor:                 { teDebugVarColor(); break; }
   case cTEDebugVarProtoObjectList:       { teDebugVarProtoObjectList(); break; }
   case cTEDebugVarObjectTypeList:        { teDebugVarObjectTypeList(); break; }
   case cTEDebugVarProtoSquadList:        { teDebugVarProtoSquadList(); break; }
   case cTEDebugVarTechList:              { teDebugVarTechList(); break; }
   case cTEDebugVarMathOperator:          { teDebugVarMathOperator(); break; }
   case cTEModifyProtoData:               { teModifyProtoData(); break; }
   case cTEGetPlayerCiv:                  { teGetPlayerCiv(); break; }
   case cTEGetIdleDuration:               { teGetIdleDuration(); break; }
   case cTEEnableAttackNotifications:     { teEnableAttackNotifications(); break; }
   case cTELerpPercent:                   { teLerpPercent(); break; }
   case cTEMathTime:                      { teMathTime(); break; }
   case cTEGetGameTime:                   { teGetGameTime(); break; }
   case cTEGetGameTimeRemaining:          { teGetGameTimeRemaining(); break; }
   case cTEGetHitZoneHealth:              { teGetHitZoneHealth(); break; }
   case cTESetHitZoneHealth:              { teSetHitZoneHealth(); break; }
   case cTESetHitZoneActive:              { teSetHitZoneActive(); break; }
   case cTECopyMessageIndex:              { teCopyMessageIndex(); break; }
   case cTESetResources:                  { teSetResources(); break; }
   case cTEGetResources:                  { teGetResources(); break; }
   case cTEGetResourcesTotals:            { teGetResourcesTotals(); break; }
   case cTESetResourcesTotals:            { teSetResourcesTotals(); break; }
   case cTEMathResources:                 { teMathResources(); break; }
   case cTEIteratorObjectList:            { teIteratorObjectList(); break; }
   case cTEForbid:                        { teForbid(); break; }
   case cTEInvertBool:                    { teInvertBool(); break; }
   case cTERevealer:                      { teRevealer(); break; }
   case cTEMathDistance:                  { teMathDistance(); break; }
   case cTEGroupDeactivate:               { teGroupDeactivate(); break; }
   case cTEIteratorLocationList:          { teIteratorLocationList(); break; }
   case cTELocationListAdd:               { teLocationListAdd(); break; }
   case cTELocationListRemove:            { teLocationListRemove(); break; }
   case cTELocationListGetSize:           { teLocationListGetSize(); break; }
   case cTELerpLocation:                  { teLerpLocation(); break; }
   case cTEMathLocation:                  { teMathLocation(); break; }
   case cTESquadListPartition:            { teSquadListPartition(); break; }
   case cTESquadListShuffle:              { teSquadListShuffle(); break; }
   case cTELocationListShuffle:           { teLocationListShuffle(); break; }
   case cTEEntityListShuffle:             { teEntityListShuffle(); break; }
   case cTEPlayerListShuffle:             { tePlayerListShuffle(); break; }
   case cTETeamListShuffle:               { teTeamListShuffle(); break; }
   case cTEUnitListShuffle:               { teUnitListShuffle(); break; }
   case cTEProtoObjectListShuffle:        { teProtoObjectListShuffle(); break; }
   case cTEObjectTypeListShuffle:         { teObjectTypeListShuffle(); break; }
   case cTEProtoSquadListShuffle:         { teProtoSquadListShuffle(); break; }
   case cTETechListShuffle:               { teTechListShuffle(); break; }
   case cTEUnitListPartition:             { teUnitListPartition(); break; }
   case cTELocationListPartition:         { teLocationListPartition(); break; }
   case cTERefCountUnitAdd:               { teRefCountUnitAdd(); break; }
   case cTERefCountUnitRemove:            { teRefCountUnitRemove(); break; }
   case cTERefCountSquadAdd:              { teRefCountSquadAdd(); break; }
   case cTERefCountSquadRemove:           { teRefCountSquadRemove(); break; }
   case cTERepair:                        { teRepair(); break; }
   case cTEUnitFlagSet:                   { teUnitFlagSet(); break; }
   case cTEGetChildUnits:                 { teGetChildUnits(); break; }
   case cTEDamage:                        { teDamage(); break; }
   case cTEUIUnlock:                      { teUIUnlock(); break; }
   case cTESquadListDiff:                 { teSquadListDiff(); break; }
   case cTEUnitListDiff:                  { teUnitListDiff(); break; }
   case cTECombatDamage:                  { teCombatDamage(); break; }
   case cTEEntityFilterClear:             { teEntityFilterClear(); break; }
   case cTEEntityFilterAddIsAlive:        { teEntityFilterAddIsAlive(); break; }
   case cTEEntityFilterAddInList:         { teEntityFilterAddInList(); break; }
   case cTEEntityFilterAddPlayers:        { teEntityFilterAddPlayers(); break; }
   case cTEEntityFilterAddTeams:          { teEntityFilterAddTeams(); break; }
   case cTEEntityFilterAddProtoObjects:   { teEntityFilterAddProtoObjects(); break; }
   case cTEEntityFilterAddProtoSquads:    { teEntityFilterAddProtoSquads(); break; }
   case cTEEntityFilterAddObjectTypes:    { teEntityFilterAddObjectTypes(); break; }
   case cTEEntityFilterAddIsSelected:     { teEntityFilterAddIsSelected(); break; }
   case cTEEntityFilterAddCanChangeOwner: { teEntityFilterAddCanChangeOwner(); break; }
   case cTEEntityFilterAddJacking:        { teEntityFilterAddJacking(); break; }
   case cTEUnitListFilter:                { teUnitListFilter(); break; }
   case cTESquadListFilter:               { teSquadListFilter(); break; }
   case cTEEntityFilterAddRefCount:       { teEntityFilterAddRefCount(); break; }
   case cTEMathFloat:                     { teMathFloat(); break; }
   case cTETeleport:                      { teTeleport(); break; }
   case cTEEntityFilterAddIsIdle:         { teEntityFilterAddIsIdle(); break; }
   case cTEAsFloat:                       { teAsFloat(); break; }
   case cTESettle:                        { teSettle(); break; }
   case cTECopyObjective:                 { teCopyObjective(); break; }
   case cTEAttachmentRemoveObject:        { teAttachmentRemoveObject(); break; }
   case cTEAttachmentRemoveUnit:          { teAttachmentRemoveUnit(); break; }
   case cTEAttachmentAddObject:           { teAttachmentAddObject(); break; }
   case cTEAttachmentAddUnit:             { teAttachmentAddUnit(); break; }
   case cTEEntityFilterAddDiplomacy:      { teEntityFilterAddDiplomacy(); break; }
   case cTESetIgnoreUserInput:            { teSetIgnoreUserInput(); break; }
   case cTEEnableFogOfWar:                { teEnableFogOfWar(); break; }
   case cTEBidCreateBlank:                { teBidCreateBlank(); break; }
   case cTEBidCreateBuilding:             { teBidCreateBuilding(); break; }
   case cTEBidCreateTech:                 { teBidCreateTech(); break; }
   case cTEBidCreateSquad:                { teBidCreateSquad(); break; }
   case cTEBidDelete:                     { teBidDelete(); break; }
   case cTEBidSetBuilding:                { teBidSetBuilding(); break; }
   case cTEBidSetTech:                    { teBidSetTech(); break; }
   case cTEBidSetSquad:                   { teBidSetSquad(); break; }
   case cTEBidClear:                      { teBidClear(); break; }
   case cTEBidSetPriority:                { teBidSetPriority(); break;}
   case cTEChangeSquadMode:               { teChangeSquadMode(); break; }
   case cTEGetAmmo:                       { teGetAmmo(); break; }
   case cTESetAmmo:                       { teSetAmmo(); break; }
   case cTEBidPurchase:                   { teBidPurchase(); break; }
   case cTELaunchScript:                  { teLaunchScript(); break; }
   case cTEGetPlayerTeam:                 { teGetPlayerTeam(); break; }
   case cTEModifyDataScalar:              { teModifyDataScalar(); break; }
   case cTECloak:                         { teCloak(); break; }
   case cTEBlocker:                       { teBlocker(); break; }
   case cTESensorLock:                    { teSensorLock(); break; }
   case cTEDesignFindSphere:              { teDesignFindSphere(); break; }
   case cTEGetPlayers2:                   { teGetPlayers2(); break; }
   case cTEPlayersToTeams:                { tePlayersToTeams(); break; }
   case cTETeamsToPlayers:                { teTeamsToPlayers(); break; }
   case cTEReinforceSquad:                { teReinforceSquad(); break; }
   case cTEMovePath:                      { teMovePath(); break; }
   case cTEIteratorKBBaseList:            { teIteratorKBBaseList(); break; }
   case cTEKBBaseGetDistance:             { teKBBaseGetDistance(); break; }
   case cTEKBBaseGetMass:                 { teKBBaseGetMass(); break; }
   case cTEKBBQReset:                     { teKBBQReset(); break; }
   case cTEKBBQExecute:                   { teKBBQExecute(); break; }
   case cTEKBBQPointRadius:               { teKBBQPointRadius(); break; }
   case cTEKBBQPlayerRelation:            { teKBBQPlayerRelation(); break; }
   case cTECopyKBBase:                    { teCopyKBBase(); break; }
   case cTEPowerGrant:                    { tePowerGrant(); break; }
   case cTEPowerRevoke:                   { tePowerRevoke(); break; }
   case cTEGetSquadTrainerType:           { teGetSquadTrainerType(); break; }
   case cTEGetTechResearcherType:         { teGetTechResearcherType(); break; }
   case cTEDesignLineGetPoints:           { teDesignLineGetPoints(); break; }
   case cTEEnableShield:                  { teEnableShield(); break; }
   case cTEKBBQMinStaleness:              { teKBBQMinStaleness(); break; }
   case cTEKBBQMaxStaleness:              { teKBBQMaxStaleness(); break; }
   case cTEGetPlayerLeader:               { teGetPlayerLeader(); break; }
   case cTELaunchCinematic:               { teLaunchCinematic(); break; }
   case cTECopyProtoObjectList:           { teCopyProtoObjectList(); break; }
   case cTECopyProtoSquadList:            { teCopyProtoSquadList(); break; }
   case cTECopyObjectTypeList:            { teCopyObjectTypeList(); break; }
   case cTECopyTechList:                  { teCopyTechList(); break; }
   case cTEIteratorProtoObjectList:       { teIteratorProtoObjectList(); break; }
   case cTEIteratorProtoSquadList:        { teIteratorProtoSquadList(); break; }
   case cTEIteratorObjectTypeList:        { teIteratorObjectTypeList(); break; }
   case cTEIteratorTechList:              { teIteratorTechList(); break; }   
   case cTESetDirection:                  { teSetDirection(); break; }
   case cTECopyDirection:                 { teCopyDirection(); break; }
   case cTEGetDirectionFromLocations:     { teGetDirectionFromLocations(); break; }
   case cTEDebugDirection:                { teDebugDirection(); break; }
   case cTEAsCount:                       { teAsCount(); break; }
   case cTEGetDirection:                  { teGetDirection(); break; }
   case cTEKBBQExecuteClosest:            { teKBBQExecuteClosest(); break; }
   case cTEGetLegalSquads:                { teGetLegalSquads(); break; }
   case cTEGetLegalTechs:                 { teGetLegalTechs(); break; }
   case cTEGetLegalBuildings:             { teGetLegalBuildings(); break; }
   case cTESetMobile:                     { teSetMobile(); break; }
   case cTEGetKBBaseLocation:             { teGetKBBaseLocation(); break; }
   case cTEGetDistanceLocationLocation:   { teGetDistanceLocationLocation(); break; }
   case cTEPlayWorldSoundAtPosition:      { tePlayWorldSoundAtPosition(); break; }
   case cTEPlayWorldSoundOnObject:        { tePlayWorldSoundOnObject(); break; }
   case cTERandomTime:                    { teRandomTime(); break; }
   case cTEGetParentSquad:                { teGetParentSquad(); break; }
   case cTECreateIconObject:              { teCreateIconObject(); break; }
   case cTESetFollowCam:                  { teSetFollowCam(); break; }
   case cTEEnableFollowCam:               { teEnableFollowCam(); break; }
   case cTEGetGarrisonedUnits:            { teGetGarrisonedUnits(); break; }
   case cTEGetDifficulty:                 { teGetDifficulty(); break; }
   case cTEHUDToggle:                     { teHUDToggle(); break; }
   case cTEInputUIButton:                 { teInputUIButton(); break; }
   case cTEDebugVarUIButton:              { teDebugVarUIButton(); break; }
   case cTESetRenderTerrainSkirt:         { teSetRenderTerrainSkirt(); break; }
   case cTESetOccluded:                   { teSetOccluded(); break; }
   case cTESetPosition:                   { teSetPosition(); break; }
   case cTELightsetAnimate:               { teLightsetAnimate(); break; }
   case cTESetFlagNearLayer:              { teSetFlagNearLayer(); break; }
   case cTEAIGenerateMissionTargets:      { teAIGenerateMissionTargets(); break; }
   case cTEAIScoreMissionTargets:         { teAIScoreMissionTargets(); break; }
   case cTEAISortMissionTargets:          { teAISortMissionTargets(); break; }
   case cTEAIMissionLaunch:               { teAIMissionLaunch(); break; }
   case cTEAIMissionCancel:               { teAIMissionCancel(); break; }
   case cTEAIGetMissionTargets:           { teAIGetMissionTargets(); break; }
   case cTELocationAdjustDir:             { teLocationAdjustDir(); break; }
   case cTEBuildingCommand:               { teBuildingCommand(); break; }
   case cTEAIMissionAddSquads:            { teAIMissionAddSquads(); break; }
   case cTEAIMissionRemoveSquads:         { teAIMissionRemoveSquads(); break; }
   case cTEAIMissionGetSquads:            { teAIMissionGetSquads(); break; }
   case cTECloakDetected:                 { teCloakDetected(); break; }
   case cTEAIMissionTargetGetScores:      { teAIMissionTargetGetScores(); break; }
   case cTECopyIntegerList:               { teCopyIntegerList(); break; }
   case cTEAISetScoringParms:             { teAISetScoringParms(); break; }
   case cTECountToInt:                    { teCountToInt(); break; }
   case cTEIntToCount:                    { teIntToCount(); break; }
   case cTEProtoObjectListAdd:            { teProtoObjectListAdd(); break; }
   case cTEProtoObjectListRemove:         { teProtoObjectListRemove(); break; }
   case cTEProtoSquadListAdd:             { teProtoSquadListAdd(); break; }
   case cTEProtoSquadListRemove:          { teProtoSquadListRemove(); break; }
   case cTETechListAdd:                   { teTechListAdd(); break; }
   case cTETechListRemove:                { teTechListRemove(); break; }
   case cTEIntegerListAdd:                { teIntegerListAdd(); break; }
   case cTEIntegerListRemove:             { teIntegerListRemove(); break; }
   case cTEIntegerListGetSize:            { teIntegerListGetSize(); break; }
   case cTEBidGetData:                    { teBidGetData(); break; }
   case cTEAITopicCreate:                 { teAITopicCreate(); break; }
   case cTEAITopicDestroy:                { teAITopicDestroy(); break; }
   case cTEAITopicModifyTickets:          { teAITopicModifyTickets(); break; }
   case cTEAIMissionModifyTickets:        { teAIMissionModifyTickets(); break; }
   case cTEPlayAnimationObject:           { tePlayAnimationObject(); break; }
   case cTECopyObject:                    { teCopyObject(); break; }
   case cTEDebugVarBuildingCommandState:  { teDebugVarBuildingCommandState(); break; }
   case cTEAIScnMissionAttackArea:        { teAIScnMissionAttackArea(); break; }
   case cTEAIScnMissionDefendArea:        { teAIScnMissionDefendArea(); break; }
   case cTEAIGetMissions:                 { teAIGetMissions(); break; }
   case cTEGetDeadUnitCount:              { teGetDeadUnitCount(); break; }
   case cTEGetDeadSquadCount:             { teGetDeadSquadCount(); break; }
   case cTECopyLocationList:              { teCopyLocationList(); break; }
   case cTEAIMissionGetLaunchScores:      { teAIMissionGetLaunchScores(); break; }
   case cTEAIRemoveFromMissions:          { teAIRemoveFromMissions(); break; }
   case cTEBreakpoint:                    { teBreakpoint(); break; }
   case cTEGetPrimaryUser:                { teGetPrimaryUser(); break; }
   case cTEGetBidsMatching:               { teGetBidsMatching(); break; }
   case cTEAIMissionGetTarget:            { teAIMissionGetTarget(); break; }
   case cTEBidSetTargetLocation:          { teBidSetTargetLocation(); break; }
   case cTEBidSetBuilder:                 { teBidSetBuilder(); break; }
   case cTEAITopicLotto:                  { teAITopicLotto(); break; }
   case cTEEnableUserMessage:             { teEnableUserMessage(); break; }
   case cTEGetProtoSquad:                 { teGetProtoSquad(); break; }
   case cTEGetProtoObject:                { teGetProtoObject(); break; }
   case cTECameraShake:                   { teCameraShake(); break; }
   case cTECustomCommandAdd:              { teCustomCommandAdd(); break; }
   case cTECustomCommandRemove:           { teCustomCommandRemove(); break; }
   case cTEConnectHitpointBar:            { teConnectHitpointBar(); break; }
   case cTEAirStrike:                     { teAirStrike(); break; }
   case cTEGetPlayerPop:                  { teGetPlayerPop(); break; }
   case cTEGetPlayerEconomy:              { teGetPlayerEconomy(); break; }
   case cTEGetPlayerScore:                { teGetPlayerScore(); break; }
   case cTEInputUISquadList:              { teInputUISquadList(); break; }
   case cTEEntityFilterAddMaxObjectType:  { teEntityFilterAddMaxObjectType(); break; }
   case cTECreateTimer:                   { teCreateTimer(); break; }
   case cTEDestroyTimer:                  { teDestroyTimer(); break; }
   case cTEKBSQInit:                      { teKBSQInit(); break; }
   case cTEKBSQReset:                     { teKBSQReset(); break; }
   case cTEKBSQExecute:                   { teKBSQExecute(); break; }
   case cTEAICalculations:                { teAICalculations(); break; }
   case cTEAICalculations2:               { teAICalculations2(); break; }
   case cTEAIAnalyzeSquadList:            { teAIAnalyzeSquadList(); break; }
   case cTEAIAnalyzeOffenseAToB:          { teAIAnalyzeOffenseAToB(); break; }
   case cTEGetMeanLocation:               { teGetMeanLocation(); break; }
   case cTECopyObjectList:                { teCopyObjectList(); break; }
   case cTEAISAGetComponent:              { teAISAGetComponent(); break; }
   case cTEAIAnalyzeKBSquadList:          { teAIAnalyzeKBSquadList(); break; }
   case cTEAIAnalyzeProtoSquadList:       { teAIAnalyzeProtoSquadList(); break; }
   case cTEShowGarrisonedCount:           { teShowGarrisonedCount(); break; }
   case cTEShowCitizensSaved:             { teShowCitizensSaved(); break; }
   case cTESetCitizensSaved:              { teSetCitizensSaved(); break; }
   case cTECostToFloat:                   { teCostToFloat(); break; }
   case cTEAIUnopposedTimeToKill:         { teAIUnopposedTimeToKill(); break; }
   case cTEGetCost:                       { teGetCost(); break; }
   case cTEGetPrimaryHealthComponent:     { teGetPrimaryHealthComponent(); break; }
   case cTEShowObjectivePointer:          { teShowObjectivePointer(); break; }
   case cTEKBSquadFilterClear:            { teKBSquadFilterClear(); break; }
   case cTEKBSFAddObjectTypes:            { teKBSFAddObjectTypes(); break; }
   case cTEKBSFAddPlayers:                { teKBSFAddPlayers(); break; }
   case cTEKBSFAddInList:                 { teKBSFAddInList(); break; }
   case cTEKBSFAddPlayerRelation:         { teKBSFAddPlayerRelation(); break; }
   case cTEKBSFAddMinStaleness:           { teKBSFAddMinStaleness(); break; }
   case cTEKBSFAddMaxStaleness:           { teKBSFAddMaxStaleness(); break; }
   case cTEKBSquadListFilter:             { teKBSquadListFilter(); break; }
   case cTEKBSquadListGetSize:            { teKBSquadListGetSize(); break; }
   case cTEKBSquadGetOwner:               { teKBSquadGetOwner(); break; }
   case cTEKBSquadGetLocation:            { teKBSquadGetLocation(); break; }
   case cTEConvertKBSquadsToSquads:       { teConvertKBSquadsToSquads(); break; }
   case cTEKBSquadGetProtoSquad:          { teKBSquadGetProtoSquad(); break; }
   case cTEKBBaseGetKBSquads:             { teKBBaseGetKBSquads(); break; }
   case cTECopyKBSquad:                   { teCopyKBSquad(); break; }
   case cTEKBSQExecuteClosest:            { teKBSQExecuteClosest(); break; }
   case cTEKBSQPointRadius:               { teKBSQPointRadius(); break; }
   case cTEKBSQPlayerRelation:            { teKBSQPlayerRelation(); break; }
   case cTEKBSQObjectType:                { teKBSQObjectType(); break; }
   case cTEKBSQBase:                      { teKBSQBase(); break; }
   case cTEKBSQMinStaleness:              { teKBSQMinStaleness(); break; }
   case cTEKBSQMaxStaleness:              { teKBSQMaxStaleness(); break; }
   case cTEKBSQCurrentlyVisible:          { teKBSQCurrentlyVisible(); break; }
   case cTEAISetFocus:                    { teAISetFocus(); break; }
   case cTESetPlayableBounds:             { teSetPlayableBounds(); break; }                             
   case cTEResetBlackMap:                 { teResetBlackMap(); break; }
   case cTEGetLOS:                        { teGetLOS(); break; }
   case cTESetUnitAttackTarget:           { teSetUnitAttackTarget(); break; }
   case cTEAsTime:                        { teAsTime(); break; }
   case cTERallyPointSet:                 { teRallyPointSet(); break; }
   case cTERallyPointClear:               { teRallyPointClear(); break; }
   case cTERallyPointGet:                 { teRallyPointGet(); break; }
   case cTEAIBindLog:                     { teAIBindLog(); break; }
   case cTEUITogglePowerOverlay:          { teUITogglePowerOverlay(); break; }
   case cTEPlayChat:                      { tePlayChat(); break; }
   case cTEShowProgressBar:               { teShowProgressBar(); break; }
   case cTEUpdateProgressBar:             { teUpdateProgressBar(); break; }
   case cTEShowObjectCounter:             { teShowObjectCounter(); break; }
   case cTEUpdateObjectCounter:           { teUpdateObjectCounter(); break; }
   case cTEDebugVarSquadList:             { teDebugVarSquadList(); break; }
   case cTEMegaTurretAttack:              { teMegaTurretAttack(); break; }
   case cTEGetPop:                        { teGetPop(); break; }
   case cTERoundFloat:                    { teRoundFloat(); break; }
   case cTEPowerClear:                    { tePowerClear(); break; }
   case cTEPowerInvoke:                   { tePowerInvoke(); break; }
   case cTESetPlayerPop:                  { teSetPlayerPop(); break; }
   case cTECopyIconType:                  { teCopyIconType(); break; }
   case cTEGetPlayerMilitaryStats:        { teGetPlayerMilitaryStats(); break; }
   case cTEGetObjectiveStats:             { teGetObjectiveStats(); break; }
   case cTELerpTime:                      { teLerpTime(); break; }
   case cTECombineString:                 { teCombineString(); break; }
   case cTETimeToFloat:                   { teTimeToFloat(); break; }
   case cTEBlockMinimap:                  { teBlockMinimap(); break; }
   case cTEBlockLeaderPowers:             { teBlockLeaderPowers(); break; }
   case cTEAIChat:                        { teAIChat(); break; }
   case cTECopyChatSpeaker:               { teCopyChatSpeaker(); break; }
   case cTEAIMissionTargetGetLocation:    { teAIMissionTargetGetLocation(); break; }                                          
   case cTEAIGetFlareAlerts:              { teAIGetFlareAlerts(); break; }
   case cTEAIGetAttackAlerts:             { teAIGetAttackAlerts(); break; }
   case cTEAIGetLastFlareAlert:           { teAIGetLastFlareAlert(); break; }
   case cTEAIGetLastAttackAlert:           { teAIGetLastAttackAlert(); break; }
   case cTEAIGetAlertData:                { teAIGetAlertData(); break; }
   case cTEGetGarrisonedSquads:           { teGetGarrisonedSquads(); break; }
   case cTETransferGarrisonedSquad:       { teTransferGarrisonedSquad(); break; }
   case cTETransferGarrisoned:            { teTransferGarrisoned(); break; }
   case cTEAISetBiases:                   { teAISetBiases(); break; }
   case cTERumbleStart:                   { teRumbleStart(); break; }
   case cTERumbleStop:                    { teRumbleStop(); break; }
   case cTEBidQuery:                      { teBidQuery(); break; }
   case cTEInputUIPlaceSquads:            { teInputUIPlaceSquads(); break; }
   case cTEGetNumTransports:              { teGetNumTransports(); break; }
   case cTESetOverrideTint:               { teSetOverrideTint(); break; }
   case cTEEnableOverrideTint:            { teEnableOverrideTint(); break; }
   case cTESetTransportPickUpLocations:   { teSetTransportPickUpLocations(); break; }
   case cTETimerSet:                      { teTimerSet(); break; }
   case cTETimerGet:                      { teTimerGet(); break; }
   case cTETimerSetPaused:                { teTimerSetPaused(); break; }
   case cTEAITopicPriorityRequest:        { teAITopicPriorityRequest(); break; }
   case cTEPlayerSelectSquads:            { tePlayerSelectSquads(); break; }
   case cTESetResourceHandicap:           { teSetResourceHandicap(); break; }
   case cTEParkingLotSet:                 { teParkingLotSet(); break; }
   case cTEUnpack:                        { teUnpack(); break; }
   case cTETableLoad:                     { teTableLoad(); break; }
   case cTEHintMessageShow:               { teHintMessageShow(); break; }
   case cTEGetPlayerColor:                { teGetPlayerColor(); break; }
   case cTEHintGlowToggle:                { teHintGlowToggle(); break; } 
   case cTEHintCalloutCreate:             { teHintCalloutCreate(); break; }
   case cTEHintCalloutDestroy:            { teHintCalloutDestroy(); break; }
   case cTEEventSubscribe:                { teEventSubscribe(); break; }
   case cTEEventSetFilter:                { teEventSetFilter(); break; }
   case cTEHintMessageDestroy:            { teHintMessageDestroy(); break; }   
   case cTECopyControlType:               { teCopyControlType(); break; }
   case cTECopyLocStringID:               { teCopyLocStringID(); break; }
   case cTESetLevel:                      { teSetLevel(); break; }
   case cTEGetLevel:                      { teGetLevel(); break; }
   case cTEKBSquadListDiff:               { teKBSquadListDiff(); break; }                                          
   case cTEModifyProtoSquadData:          { teModifyProtoSquadData(); break; }
   case cTESetScenarioScore:              { teSetScenarioScore(); break; }
   case cTESetScenarioScoreInfo:          { teSetScenarioScoreInfo(); break; }
   case cTECreateObstructionUnit:         { teCreateObstructionUnit(); break; }
   case cTEPlayVideo:                     { tePlayVideo(); break; }
   case cTEObjectListRemove:              { teObjectListRemove(); break; }
   case cTEEventReset:                    { teEventReset(); break; }
   case cTEEventFilterCamera:             { teEventFilterCamera(); break; }
   case cTEEventFilterEntity:             { teEventFilterEntity(); break; }
   case cTEEventFilterEntityList:         { teEventFilterEntityList(); break; }
   case cTEEnableChats:                   { teEnableChats(); break; }
   case cTEObjectListAdd:                 { teObjectListAdd(); break; }
   case cTEObjectListGetSize:             { teObjectListGetSize(); break; }
   case cTEClearCorpseUnits:              { teClearCorpseUnits(); break; }
   case cTEAddXP:                         { teAddXP(); break; }
   case cTEEventFilterGameState:				{ teEventFilterGameState(); break; }
   case cTEEventFilterType:			   	{ teEventFilterType(); break; }
   case cTEEventFilterNumeric:            { teEventFilterNumeric(); break; }
   case cTEBidSetBlockedBuilders:         { teBidSetBlockedBuilders(); break; }
   case cTEGetBuildingTrainQueue:         { teGetBuildingTrainQueue(); break; }
   case cTEAIRegisterHook:                { teAIRegisterHook(); break; }
   case cTEGetClosestSquad:               { teGetClosestSquad(); break; }
   //case cTEMoveToFace:                    { teMoveToFace(); break; }
   case cTECopyTimeList:                  { teCopyTimeList(); break; }
   case cTETimeListAdd:                   { teTimeListAdd(); break; }
   case cTETimeListRemove:                { teTimeListRemove(); break; }
   case cTETimeListGetSize:               { teTimeListGetSize(); break; }
   case cTEClearBlackMap:                 { teClearBlackMap(); break; }
   case cTECopyDesignLineList:            { teCopyDesignLineList(); break;}
   case cTEDesignLineListAdd:             { teDesignLineListAdd(); break; }
   case cTEDesignLineListRemove:          { teDesignLineListRemove(); break; }
   case cTEDesignLineListGetSize:         { teDesignLineListGetSize(); break; }
   case cTEBidSetQueueLimits:             { teBidSetQueueLimits(); break; }                                          
   case cTECopyDesignLine:                { teCopyDesignLine(); break; }
   case cTEProtoSquadListGetSize:         { teProtoSquadListGetSize(); break; }
   case cTEGetObstructionRadius:          { teGetObstructionRadius(); break; }
   case cTECreateSquads:                  { teCreateSquads(); break; }
   case cTECopyLOSType:                   { teCopyLOSType(); break; }
   case cTEAISetAssetMultipliers:         { teAISetAssetMultipliers(); break; }
   case cTEAIGetOpportunityRequests:      { teAIGetOpportunityRequests(); break; }
   case cTEAIGetTerminalMissions:         { teAIGetTerminalMissions(); break; }
   case cTEAIClearOpportunityRequests:    { teAIClearOpportunityRequests(); break; }
   case cTEAITopicSetFocus:               { teAITopicSetFocus(); break; }
   case cTESetCamera:                     { teSetCamera(); break; }
   case cTECopyFloatList:                 { teCopyFloatList(); break; }
   case cTEFloatListAdd:                  { teFloatListAdd(); break; }
   case cTEFloatListRemove:               { teFloatListRemove(); break; }
   case cTEFloatListGetSize:              { teFloatListGetSize(); break; }
   case cTEGetUTechBldings:               { teGetUTechBldings(); break; }
   case cTERepairByCombatValue:           { teRepairByCombatValue(); break; }
   case cTEInputUILocationMinigame:       { teInputUILocationMinigame(); break; }
   case cTEAIQueryMissionTargets:         { teAIQueryMissionTargets(); break; }
   case cTEGetClosestPath:                { teGetClosestPath(); break; }
   case cTESetSelectable:                 { teSetSelectable(); break; }
   case cTESetTrickleRate:                { teSetTrickleRate(); break; }
   case cTEGetTrickleRate:                { teGetTrickleRate(); break; }
   case cTEChatDestroy:                   { teChatDestroy(); break; }
   case cTEShowMessage:                   { teShowMessage(); break; }
   case cTEFadeToColor:                   { teFadeToColor(); break; }
   case cTESquadFlagSet:                  { teSquadFlagSet(); break; }
   case cTEAICalculateOffenseRatioAToB:   { teAICalculateOffenseRatioAToB(); break; }
   case cTESetAutoAttackable:             { teSetAutoAttackable(); break; }
   case cTECopyAISquadAnalysis:           { teCopyAISquadAnalysis(); break; }
   case cTEFadeTransition:                { teFadeTransition(); break; }
   case cTEFlashUIElement:                { teFlashUIElement(); break; }
   case cTEObjectiveIncrementCounter:     { teObjectiveIncrementCounter(); break; }
   case cTEObjectiveDecrementCounter:     { teObjectiveDecrementCounter(); break; }
   case cTEObjectiveGetCurrentCounter:    { teObjectiveGetCurrentCounter(); break; }
   case cTEObjectiveGetFinalCounter:      { teObjectiveGetFinalCounter(); break; }
   case cTETeamSetDiplomacy:              { teTeamSetDiplomacy(); break; }
   case cTEConceptGetParameters:          { teConceptGetParameters(); break; }
   case cTEConceptSetttings:              { teConceptSetttings(); break; }
   case cTEConceptPermission:             { teConceptPermission(); break; }
   case cTEConceptSetState:               { teConceptSetState(); break; }
   case cTEConceptSetPrecondition:        { teConceptSetPrecondition(); break; }
   case cTEConceptStartSub:               { teConceptStartSub(); break; }
   case cTEConceptClearSub:               { teConceptClearSub(); break; }
   case cTEConceptSetParameters:          { teConceptSetParameters(); break; }
   case cTEGetNPCPlayersByName:           { teGetNPCPlayersByName(); break; }
   case cTEGetClosestUnit:                { teGetClosestUnit(); break; }
   case cTEClearBuildingCommandState:     { teClearBuildingCommandState(); break; }
   case cTEActivateTentacle:              { teActivateTentacle(); break; }
   case cTERecycleBuilding:               { teRecycleBuilding(); break; }
   case cTEAIGetMemory:                   { teAIGetMemory(); break; }
   case cTEBidCreatePower:                { teBidCreatePower(); break; }
   case cTEBidSetPower:                   { teBidSetPower(); break; }
   case cTEBidAddToMissions:              { teBidAddToMissions(); break; }
   case cTEBidRemoveFromMissions:         { teBidRemoveFromMissions(); break; }
   case cTEPowerUsed:                     { tePowerUsed(); break; }
   case cTESetTeleporterDestination:      { teSetTeleporterDestination(); break; }
   case cTESetGarrisonedCount:            { teSetGarrisonedCount(); break; }
   case cTEPlayerListGetSize:             { tePlayerListGetSize(); break; }
   case cTEConceptResetCooldown:          { teConceptResetCooldown(); break; }
   case cTEGetPowerRadius:                { teGetPowerRadius(); break; }
   case cTETransportSquads2:              { teTransportSquads2(); break; }
   case cTELocationListGetByIndex:        { teLocationListGetByIndex(); break; }
   case cTELocationListGetClosest:        { teLocationListGetClosest(); break; }
   case cTEAIMissionSetMoveAttack:        { teAIMissionSetMoveAttack(); break; }
   case cTEInfect:                        { teInfect(); break; }
   case cTEKBAddSquadsToKB:               { teKBAddSquadsToKB(); break; }
   case cTEBidSetPadSupplies:             { teBidSetPadSupplies(); break; }
   case cTEEventDelete:                   { teEventDelete(); break; }
   case cTEEventClearFilters:             { teEventClearFilters(); break; }
   case cTEAIFactoidSubmit:               { teAIFactoidSubmit(); break; }
   case cTEFlashEntity:                   { teFlashEntity(); break; }
   case cTEMissionResult:                 { teMissionResult(); break; }
   case cTESetTowerWallDestination:       { teSetTowerWallDestination(); break; }
   case cTEAICreateAreaTarget:            { teAICreateAreaTarget(); break; }
   case cTEAIMissionCreate:               { teAIMissionCreate(); break; }
   case cTEEnableMusicManager:            { teEnableMusicManager(); break; }
   case cTEResetDopple:                   { teResetDopple(); break; }
   case cTEAICreateWrapper:               { teAICreateWrapper(); break; }
   case cTEAIReorderWrapper:              { teAIReorderWrapper(); break; }
   case cTEAIDestroyWrapper:              { teAIDestroyWrapper(); break; }
   case cTEAIWrapperModifyRadius:         { teAIWrapperModifyRadius(); break; }
   case cTEAIWrapperModifyFlags:          { teAIWrapperModifyFlags(); break; }
   case cTEAIWrapperModifyParms:          { teAIWrapperModifyParms(); break; }
   case cTESaveGame:                      { teSaveGame(); break; }
   case cTELoadGame:                      { teLoadGame(); break; }
   case cTEReverseHotDrop:                { teReverseHotDrop(); break; }
   case cTEEventSubscribeUseCount:        { teEventSubscribeUseCount(); break; }
   case cTEObjectTypeToProtoObjects:      { teObjectTypeToProtoObjects(); break; }
   case cTEDebugVarDesignLine:            { teDebugVarDesignLine(); break; }
   case cTEShowInfoDialog:                { teShowInfoDialog(); break; }
   case cTEPowerUserShutdown:             { tePowerUserShutdown(); break; }
   case cTEPowerToInt:                    { tePowerToInt(); break; }
   case cTEIntToPower:                    { teIntToPower(); break; }
   case cTEChangeControlledPlayer:        { teChangeControlledPlayer(); break; }
   case cTEGrantAchievement:              { teGrantAchievement(); break; }
   case cTEAIAddTeleporterZone:           { teAIAddTeleporterZone(); break; }
   case cTEResetAbilityTimer:             { teResetAbilityTimer(); break; }
   case cTECustomCommandExecute:          { teCustomCommandExecute(); break; }
   case cTEIgnoreDpad:                    { teIgnoreDpad(); break; }
   case cTEAISetPlayerAssetModifier:      { teAISetPlayerAssetModifier(); break; }
   case cTEEnableLetterBox:               { teEnableLetterBox(); break; }
   case cTEEnableScreenBlur:              { teEnableScreenBlur(); break; }
   case cTEAISetPlayerDamageModifiers:    { teAISetPlayerDamageModifiers(); break; }
   case cTESetPowerAvailableTime:         { teSetPowerAvailableTime(); break; }
   case cTEPowerMenuEnable:               { tePowerMenuEnable(); break; }
   case cTESetMinimapNorthPointerRotation:{ teSetMinimapNorthPointerRotation(); break; }
   case cTEHideCircleMenu:                { teHideCircleMenu(); break; }
   case cTEChatForceSubtitles:            { teChatForceSubtitles(); break; }
   case cTEAISetPlayerMultipliers:        { teAISetPlayerMultipliers(); break; }
   case cTEAIMissionSetFlags:             { teAIMissionSetFlags(); break; }
   case cTESetMinimapSkirtMirroring:      { teSetMinimapSkirtMirroring(); break; }
   case cTEAISetWinRange:                 { teAISetWinRange(); break; }
   case cTEGetPopularSquadType:           { teGetPopularSquadType(); break; }
   case cTETeleportUnitsOffObstruction:   { teTeleportUnitsOffObstruction(); break; }
   case cTELockPlayerUser:                { teLockPlayerUser(); break; }
   case cTEGetGameMode:                   { teGetGameMode(); break; }
   case cTEAISetPlayerBuildSpeedModifiers:{ teAISetPlayerBuildSpeedModifiers(); break; }
   case cTEPatherObstructionUpdates:      { tePatherObstructionUpdates(); break; }
   case cTEPatherObstructionRebuild:      { tePatherObstructionRebuild(); break; }

   // NEWTRIGGEREFFECT
   // Add your case statement calling your trigger effect function here, following the same conventions.
   // Any questions, ask Marc.

   default: { BTRIGGER_ASSERT(false); break; }
   }

   // Sync the trigger effects.
   syncTriggerData("BTriggerEffect::fire() - evaluated effect DBID = ", mDBID);
   syncTriggerData("BTriggerEffect::fire() - evaluated effect Version = ", static_cast<DWORD>(mVersion));
}


//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teKill()
{
   switch (getVersion())
   {
      case 3:
         teKillV3();
         break;

      case 4:
         teKillV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Kill a Unit, UnitList, Squad, or SquadList
//==============================================================================
void BTriggerEffect::teKillV3()
{
   enum 
   { 
      cUnit = 3, 
      cUnitList = 4, 
      cSquad = 5, 
      cSquadList = 6, 
   };

   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useUnit && !useUnitList && !useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, or SquadList designated!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }  
   
   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }   

   // Kill units
   uint count = unitList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         pUnit->kill(false);
      }
   }

   // Kill squads
   count = squadList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         pSquad->kill(false);
      }
   }
}

//==============================================================================
// Kill a Unit, UnitList, Squad, SquadList, Object, and/or ObjectList
//==============================================================================
void BTriggerEffect::teKillV4()
{
   enum 
   { 
      cUnit = 3, 
      cUnitList = 4, 
      cSquad = 5, 
      cSquadList = 6, 
      cObject = 7,
      cObjectList = 8,
   };

   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useObject = getVar(cObject)->isUsed();
   bool useObjectList = getVar(cObjectList)->isUsed();

   if (!useUnit && !useUnitList && !useSquad && !useSquadList && !useObject && !useObjectList)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, SquadList, Object, or ObjectList designated!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }  

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }   

   // Collect objects
   BEntityIDArray objectList;
   if (useObjectList)
   {
      objectList = getVar(cObjectList)->asObjectList()->readVar();
   }

   if (useObject)
   {
      objectList.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   // Kill units
   uint count = unitList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         pUnit->kill(false);
      }
   }

   // Kill squads
   count = squadList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         pSquad->kill(false);
      }
   }

   // Kill objects
   count = objectList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject)
      {
         pObject->kill(false);
      }
   }
}

//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teDestroy()
{
   switch (getVersion())
   {
      case 3:
         teDestroyV3();
         break;

      case 4:
         teDestroyV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Destroy a Unit, UnitList, Squad, or SquadList
//==============================================================================
void BTriggerEffect::teDestroyV3()
{
   enum 
   { 
      cUnit = 3, 
      cUnitList = 4, 
      cSquad = 5, 
      cSquadList = 6, 
   };

   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useUnit && !useUnitList && !useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, or SquadList designated!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }   

   // Destory units
   uint count = unitList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         pUnit->kill(true);
      }
   }

   // Destroy squads
   count = squadList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         // [11/5/2008 xemu] don't score a unit killed by a trigger 
         pSquad->setFlagPreventScoring(true);
         pSquad->kill(true);
      }
   }
}

//==============================================================================
// Destroy a Unit, UnitList, Squad, SquadList, Object, and/or ObjectList
//==============================================================================
void BTriggerEffect::teDestroyV4()
{
   enum 
   { 
      cUnit = 3, 
      cUnitList = 4, 
      cSquad = 5, 
      cSquadList = 6, 
      cObject = 7,
      cObjectList = 8,
   };

   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useObject = getVar(cObject)->isUsed();
   bool useObjectList = getVar(cObjectList)->isUsed();

   if (!useUnit && !useUnitList && !useSquad && !useSquadList && !useObject && !useObjectList)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, SquadList, Object, or ObjectList designated!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (useUnitList) 
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }   

   // Collect objects
   BEntityIDArray objectList;
   if (useObjectList)
   {
      objectList = getVar(cObjectList)->asObjectList()->readVar();
   }

   if (useObject)
   {
      objectList.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   // Destory units
   uint count = unitList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         pUnit->kill(true);
      }
   }

   // Destroy squads
   count = squadList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         // [11/5/2008 xemu] don't score a unit killed by a trigger 
         pSquad->setFlagPreventScoring(true);
         pSquad->kill(true);
      }
   }

   // Destroy objects
   count = objectList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject)
      {
         pObject->kill(true);
      }
   }
}

//==============================================================================
// BTriggerEffect::teTriggerActivate()
//==============================================================================
void BTriggerEffect::teTriggerActivate()
{
   enum { cInputTrigger = 1, };
   BTriggerID triggerID = getVar(cInputTrigger)->asTrigger()->readVar();
   gTriggerManager.activateTrigger(getParentTriggerScript()->getID(), triggerID);
}


//==============================================================================
// BTriggerEffect::teTriggerDeactivate()
//==============================================================================
void BTriggerEffect::teTriggerDeactivate()
{
   enum { cInputTrigger = 1, };
   BTriggerID triggerID = getVar(cInputTrigger)->asTrigger()->readVar();
   gTriggerManager.deactivateTrigger(getParentTriggerScript()->getID(), triggerID);
}


//==============================================================================
// BTriggerEffect::tePlaySound()
//==============================================================================
void BTriggerEffect::tePlaySound()
{
   switch (getVersion())
   {
   case 1:
      tePlaySoundV1();
      break;

   case 2:
      tePlaySoundV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}

//==============================================================================
// BTriggerEffect::tePlaySound()
//==============================================================================
void BTriggerEffect::tePlaySoundV1()
{
   enum 
   { 
      cInputSound = 1, 
   };

   const BSimString& sndCue = getVar(cInputSound)->asSound()->readVar();
   gSoundManager.playCue(sndCue);
}

//==============================================================================
// BTriggerEffect::tePlaySound()
//==============================================================================
void BTriggerEffect::tePlaySoundV2()
{
   enum 
   { 
      cInputSound = 1, 
      cQueue = 3,
   };

   const BSimString& sndCue = getVar(cInputSound)->asSound()->readVar();
   bool queue = getVar(cQueue)->isUsed() ? getVar(cQueue)->asBool()->readVar() : false;
   gSoundManager.playCue(sndCue, cInvalidWwiseObjectID, queue);
}


//==============================================================================
// BTriggerEffect::tePlayRelationSound()
//==============================================================================
void BTriggerEffect::tePlayRelationSound()
{
   switch (getVersion())
   {
   case 1:
      tePlayRelationSoundV1();
      break;
   case 2:
      tePlayRelationSoundV2();
      break;
   default:
      BASSERTM(false, "Invalid version!");
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayRelationSoundV1()
{
   enum { cInputPlayer = 1, cInputSelfSound = 2, cInputAllySound = 3, cInputEnemySound = 4, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   bool useSelfSound = getVar(cInputSelfSound)->isUsed();
   bool useAllySound = getVar(cInputAllySound)->isUsed();
   bool useEnemySound = getVar(cInputEnemySound)->isUsed();   

//-- FIXING PREFIX BUG ID 5482
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return;

   // Check for SELF
   if (useSelfSound && pPlayer->getID() == playerID)
   {
      const BSimString& selfSound = getVar(cInputSelfSound)->asSound()->readVar();
      gSoundManager.playCue(selfSound);
   }
   // Check for ALLY
   else if (useAllySound && pPlayer->isAlly(playerID))
   {
      const BSimString& allySound = getVar(cInputAllySound)->asSound()->readVar();
      gSoundManager.playCue(allySound);
   }
   // Check for ENEMY
   else if (useEnemySound && pPlayer->isEnemy(playerID))
   {
      const BSimString& enemySound = getVar(cInputEnemySound)->asSound()->readVar();
      gSoundManager.playCue(enemySound);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayRelationSoundV2()
{
   enum { cInputPlayer = 1, cSelfUnsc = 2, cAllyUnsc = 3, cEnemyUnsc = 4, cSelfCov = 5, cAllyCov = 6, cEnemyCov = 7, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();

//-- FIXING PREFIX BUG ID 5483
   const BTriggerVarSound* pSelfUnsc = getVar(cSelfUnsc)->asSound();
//--
//-- FIXING PREFIX BUG ID 5484
   const BTriggerVarSound* pAllyUnsc = getVar(cAllyUnsc)->asSound();
//--
//-- FIXING PREFIX BUG ID 5485
   const BTriggerVarSound* pEnemyUnsc = getVar(cEnemyUnsc)->asSound();
//--
//-- FIXING PREFIX BUG ID 5486
   const BTriggerVarSound* pSelfCov = getVar(cSelfCov)->asSound();
//--
//-- FIXING PREFIX BUG ID 5487
   const BTriggerVarSound* pAllyCov = getVar(cAllyCov)->asSound();
//--
//-- FIXING PREFIX BUG ID 5488
   const BTriggerVarSound* pEnemyCov = getVar(cEnemyCov)->asSound();
//--
//-- FIXING PREFIX BUG ID 5489
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return;

   if (pSelfUnsc->isUsed() && pPlayer->getID() == playerID)
   {
      gSoundManager.playCue(pSelfUnsc->readVar());
   }
   else if (pSelfCov->isUsed() && pPlayer->getID() == playerID)
   {
      gSoundManager.playCue(pSelfCov->readVar());
   }
   else if (pAllyUnsc->isUsed() && pPlayer->isAlly(playerID, false))
   {
      gSoundManager.playCue(pAllyUnsc->readVar());
   }
   else if (pAllyCov->isUsed() && pPlayer->isAlly(playerID, false))
   {
      gSoundManager.playCue(pAllyCov->readVar());
   }
   else if (pEnemyUnsc->isUsed() && pPlayer->isEnemy(playerID))
   {
      gSoundManager.playCue(pEnemyUnsc->readVar());
   }
   else if (pEnemyCov->isUsed() && pPlayer->isEnemy(playerID))
   {
      gSoundManager.playCue(pEnemyCov->readVar());
   }
}

//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teInputUILocation()
{
   switch (getVersion())
   {
      case 3:
         teInputUILocationV3();
         break;

      case 4:
         teInputUILocationV4();
         break;

      case 5:
         teInputUILocationV5();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Put player into input location mode with proper placement rules
//==============================================================================
void BTriggerEffect::teInputUILocationV3()
{
   enum 
   { 
      cInputPlayer = 1, 
      cOutputUILocation = 2, 
      cInputProtoObject = 3, 
      cInputCheckObstruction = 4, 
      cLosType = 5,
      cInputPlacementRule = 6,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BProtoObjectID protoObjectID = getVar(cInputProtoObject)->isUsed() ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   bool checkObstruction = getVar(cInputCheckObstruction)->isUsed() ? getVar(cInputCheckObstruction)->asBool()->readVar() : false;
   long losType = getVar(cLosType)->isUsed() ? getVar(cLosType)->asLOSType()->readVar() : BWorld::cCPLOSDontCare;
   getVar(cOutputUILocation)->asUILocation()->writeResult(BTriggerVarUILocation::cUILocationResultWaiting);
   BTriggerVarID UILocationVarID = getVar(cOutputUILocation)->getID();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   long placementRuleIndex = getVar(cInputPlacementRule)->isUsed() ? getVar(cInputPlacementRule)->asPlacementRule()->readVar() : -1;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUILocation);
      pUser->setUIPowerRadius(0.0f);
      pUser->setBuildProtoID(protoObjectID);
      pUser->setUILocationCheckObstruction(checkObstruction);
      pUser->setUILocationLOSType(losType);
      pUser->setUILocationCheckMoving(false);
      pUser->setUILocationPlacementSuggestion(false);
      pUser->setUILocationPlacementRuleIndex(placementRuleIndex);
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UILocationVarID);
   }
   else
   {
      BTriggerCommand* pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandTrigger);
      pCommand->setSenders(1, &playerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUILocationResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UILocationVarID);
      pCommand->setInputResult(BTriggerVarUILocation::cUILocationResultUILockError);
      pCommand->setInputLocation(cInvalidVector);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// Put player into input location mode with proper placement rules
//==============================================================================
void BTriggerEffect::teInputUILocationV4()
{
   enum 
   { 
      cInputPlayer = 1, 
      cOutputUILocation = 2, 
      cInputProtoObject = 3, 
      cInputCheckObstruction = 4, 
      cLosType = 5,
      cInputPlacementRule = 6,
      cInputLOSCenteryOnly = 7,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BProtoObjectID protoObjectID = getVar(cInputProtoObject)->isUsed() ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   bool checkObstruction = getVar(cInputCheckObstruction)->isUsed() ? getVar(cInputCheckObstruction)->asBool()->readVar() : false;
   long losType = getVar(cLosType)->isUsed() ? getVar(cLosType)->asLOSType()->readVar() : BWorld::cCPLOSDontCare;
   getVar(cOutputUILocation)->asUILocation()->writeResult(BTriggerVarUILocation::cUILocationResultWaiting);
   BTriggerVarID UILocationVarID = getVar(cOutputUILocation)->getID();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   long placementRuleIndex = getVar(cInputPlacementRule)->isUsed() ? getVar(cInputPlacementRule)->asPlacementRule()->readVar() : -1;
   bool losCenterOnly = getVar(cInputLOSCenteryOnly)->isUsed() ? getVar(cInputLOSCenteryOnly)->asBool()->readVar() : false;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUILocation);
      pUser->setUIPowerRadius(0.0f);
      pUser->setBuildProtoID(protoObjectID);
      pUser->setUILocationCheckObstruction(checkObstruction);
      pUser->setUILocationLOSType(losType);
      pUser->setUILocationLOSCenterOnly(losCenterOnly);
      pUser->setUILocationCheckMoving(false);
      pUser->setUILocationPlacementSuggestion(false);
      pUser->setUILocationPlacementRuleIndex(placementRuleIndex);
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UILocationVarID);
   }
   else
   {
      BTriggerCommand* pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandTrigger);
      pCommand->setSenders(1, &playerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUILocationResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UILocationVarID);
      pCommand->setInputResult(BTriggerVarUILocation::cUILocationResultUILockError);
      pCommand->setInputLocation(cInvalidVector);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// Put player into input location mode with proper placement rules
//==============================================================================
void BTriggerEffect::teInputUILocationV5()
{
   enum 
   { 
      cInputPlayer = 1, 
      cOutputUILocation = 2, 
      cInputProtoObject = 3, 
      cInputCheckObstruction = 4, 
      cLosType = 5,
      cInputPlacementRule = 6,
      cInputLOSCenteryOnly = 7,
      cInputCheckMoving = 8,
      cInputPlacementSuggestion = 9,
      cInputProtoSquad = 10,
      cInputUseSquadObstruction = 11,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BProtoObjectID protoObjectID = getVar(cInputProtoObject)->isUsed() ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   bool checkObstruction = getVar(cInputCheckObstruction)->isUsed() ? getVar(cInputCheckObstruction)->asBool()->readVar() : false;
   long losType = getVar(cLosType)->isUsed() ? getVar(cLosType)->asLOSType()->readVar() : BWorld::cCPLOSDontCare;
   getVar(cOutputUILocation)->asUILocation()->writeResult(BTriggerVarUILocation::cUILocationResultWaiting);
   BTriggerVarID UILocationVarID = getVar(cOutputUILocation)->getID();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   long placementRuleIndex = getVar(cInputPlacementRule)->isUsed() ? getVar(cInputPlacementRule)->asPlacementRule()->readVar() : -1;
   bool losCenterOnly = getVar(cInputLOSCenteryOnly)->isUsed() ? getVar(cInputLOSCenteryOnly)->asBool()->readVar() : false;
   bool checkMoving = getVar(cInputCheckMoving)->isUsed() ? getVar(cInputCheckMoving)->asBool()->readVar() : false;
   bool suggestPlacement = getVar(cInputPlacementSuggestion)->isUsed() ? getVar(cInputPlacementSuggestion)->asBool()->readVar() : false;
   BProtoSquadID protoSquadID = getVar(cInputProtoSquad)->isUsed() ? getVar(cInputProtoSquad)->asProtoSquad()->readVar() : cInvalidProtoSquadID;
   bool useSquadObstruction = getVar(cInputUseSquadObstruction)->isUsed() ? getVar(cInputUseSquadObstruction)->asBool()->readVar() : false;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUILocation);
      pUser->setUIPowerRadius(0.0f);
      pUser->setBuildProtoID(protoObjectID, protoSquadID, useSquadObstruction);
      pUser->setUILocationCheckObstruction(checkObstruction);
      pUser->setUILocationLOSType(losType);
      pUser->setUILocationLOSCenterOnly(losCenterOnly);
      pUser->setUILocationPlacementSuggestion(suggestPlacement);
      pUser->setUILocationCheckMoving(checkMoving);
      pUser->setUILocationPlacementRuleIndex(placementRuleIndex);
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UILocationVarID);
   }
   else
   {
      BTriggerCommand* pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandTrigger);
      pCommand->setSenders(1, &playerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUILocationResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UILocationVarID);
      pCommand->setInputResult(BTriggerVarUILocation::cUILocationResultUILockError);
      pCommand->setInputLocation(cInvalidVector);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}


//==============================================================================
// BTriggerEffect::teCreateObject()
//==============================================================================
void BTriggerEffect::teCreateObject()
{
   SCOPEDSAMPLE(BTriggerEffect_teCreateObject);

   switch (getVersion())
   {
      case 5:
         teCreateObjectV5();
         break;

      case 6:
         teCreateObjectV6();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Create an object
//==============================================================================
void BTriggerEffect::teCreateObjectV5()
{
   enum 
   { 
      cProtoObject = 1, 
      cLocation = 2, 
      cPlayer = 3, 
      cCreatedObject = 4, 
      cAddToObjectList = 8, 
      cClearExisting = 9, 
      cFacing = 10,
   };
   bool useCreatedObject = getVar(cCreatedObject)->isUsed();
   bool useAddToObjectList = getVar(cAddToObjectList)->isUsed();
   bool useClearExisting = getVar(cClearExisting)->isUsed();
   bool clearExisting = useClearExisting ? getVar(cClearExisting)->asBool()->readVar() : false;   

   // Seed our object create parms & create our object.
   BObjectCreateParms parms;
   parms.mProtoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   parms.mPlayerID = getVar(cPlayer)->asPlayer()->readVar();
   parms.mPosition = getVar(cLocation)->asVector()->readVar();
   parms.mForward = getVar(cFacing)->isUsed() ? getVar(cFacing)->asVector()->readVar() : cZAxisVector;
   parms.mRight.assignCrossProduct(cYAxisVector, parms.mForward);
   parms.mRight.normalize();

//-- FIXING PREFIX BUG ID 5490
   const BObject* pObject = gWorld->createObject(parms);
//--
   BEntityID createdObjectID = pObject ? pObject->getID() : cInvalidObjectID;

   if (useCreatedObject)
      getVar(cCreatedObject)->asObject()->writeVar(createdObjectID);

   if (useAddToObjectList)
   {
      if (clearExisting)
      {
         getVar(cAddToObjectList)->asObjectList()->clear();
      }

      if (createdObjectID != cInvalidObjectID)
         getVar(cAddToObjectList)->asObjectList()->addObject(createdObjectID);
   }
}

//==============================================================================
// Create an object
//==============================================================================
void BTriggerEffect::teCreateObjectV6()
{
   enum 
   { 
      cProtoObject = 1, 
      cLocation = 2, 
      cPlayer = 3, 
      cCreatedObject = 4, 
      cAddToObjectList = 8, 
      cClearExisting = 9, 
      cFacing = 10,
      cNoPhysics = 11,
   };

   bool useCreatedObject = getVar(cCreatedObject)->isUsed();
   bool useAddToObjectList = getVar(cAddToObjectList)->isUsed();
   bool useClearExisting = getVar(cClearExisting)->isUsed();
   bool clearExisting = useClearExisting ? getVar(cClearExisting)->asBool()->readVar() : false;   

   // Seed our object create parms & create our object.
   BObjectCreateParms parms;
   parms.mProtoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   parms.mPlayerID = getVar(cPlayer)->asPlayer()->readVar();
   parms.mPosition = getVar(cLocation)->asVector()->readVar();
   parms.mForward = getVar(cFacing)->isUsed() ? getVar(cFacing)->asVector()->readVar() : cZAxisVector;
   parms.mRight.assignCrossProduct(cYAxisVector, parms.mForward);
   parms.mRight.normalize();
   parms.mNoPhysics = getVar(cNoPhysics)->isUsed() ? getVar(cNoPhysics)->asBool()->readVar() : false;

//-- FIXING PREFIX BUG ID 5491
   const BObject* pObject = gWorld->createObject(parms);
//--
   BEntityID createdObjectID = pObject ? pObject->getID() : cInvalidObjectID;

   if (useCreatedObject)
      getVar(cCreatedObject)->asObject()->writeVar(createdObjectID);

   if (useAddToObjectList)
   {
      if (clearExisting)
      {
         getVar(cAddToObjectList)->asObjectList()->clear();
      }

      if (createdObjectID != cInvalidObjectID)
         getVar(cAddToObjectList)->asObjectList()->addObject(createdObjectID);
   }
}

//==============================================================================
// BTriggerEffect::teCreateSquad()
//==============================================================================
void BTriggerEffect::teCreateSquad()
{
   SCOPEDSAMPLE(BTriggerEffect_teCreateSquad);

   switch(getVersion())
   {
      case 6:
         teCreateSquadV6();
         break;

      case 7:
         teCreateSquadV7();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//============================================
// Create a squad with optional fly-in action
//============================================
void BTriggerEffect::teCreateSquadV6()
{
   enum 
   { 
      cInputProtoSquad = 1, 
      cInputCreatePosition = 2, 
      cInputPlayer = 3, 
      cOutputCreatedSquad = 4, 
      cOutputAddToList = 5, 
      cOutputClearExisting = 6, 
      cFlyInStart = 8,
      cFlyInEnd = 9,
      cRallyPoint = 10,
      cAttackMoveRallyPoint = 11,
   };

   long protoSquad = getVar(cInputProtoSquad)->asProtoSquad()->readVar();
   BVector createPos = getVar(cInputCreatePosition)->asVector()->readVar();
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   bool useOutputCreatedSquad = getVar(cOutputCreatedSquad)->isUsed();
   bool useOutputAddToList = getVar(cOutputAddToList)->isUsed();
   bool clearExisting = getVar(cOutputClearExisting)->isUsed() ? getVar(cOutputClearExisting)->asBool()->readVar() : false;
   bool useFlyInStart = getVar(cFlyInStart)->isUsed();
   bool useFlyInEnd = getVar(cFlyInEnd)->isUsed();
   BVector rallyPoint = getVar(cRallyPoint)->isUsed() ? getVar(cRallyPoint)->asVector()->readVar() : cInvalidVector;
   bool attackMove = getVar(cAttackMoveRallyPoint)->isUsed() ? getVar(cAttackMoveRallyPoint)->asBool()->readVar() : false;
   bool flyInSuccess = false;

   // Create object.
   BEntityID createdSquadID = gWorld->createEntity(protoSquad, true, playerID, createPos, cZAxisVector, cXAxisVector, true);
   BSquad* pSquad = gWorld->getSquad(createdSquadID);
   if (pSquad)
   {
#ifndef BUILD_FINAL
      gTriggerManager.incrementCreatePerfCount();
#endif

      bool useRallyPoint = (rallyPoint == cInvalidVector) ? false : true;
      if (useFlyInStart || useFlyInEnd)
      {
         BPlayer* pPlayer = gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 5492
         const BCiv* pCiv = pPlayer ? pPlayer->getCiv() : NULL;
//--
         long transportProtoID = pCiv ? pCiv->getTransportTriggerProtoID() : cInvalidProtoID;
         if (transportProtoID != cInvalidProtoID)
         {
            const BVector start = useFlyInStart ? getVar(cFlyInStart)->asVector()->readVar() : cInvalidVector;
            const BVector end = useFlyInEnd ? getVar(cFlyInEnd)->asVector()->readVar() : cInvalidVector;
            const BVector* pStart = (start == cInvalidVector) ? NULL : &start;
            const BVector* pEnd = (end == cInvalidVector) ? NULL : &end;

            // Create an order
            BSimOrder* pOrder = gSimOrderManager.createOrder();
            // Init order
            if (pOrder)  
            {
               pOrder->setPriority(BSimOrder::cPriorityTrigger);
            }
            
            flyInSuccess = BSquadActionTransport::flyInSquad(pOrder, pSquad, pStart, createPos, cZAxisVector, cXAxisVector, pEnd, playerID, pPlayer->getCiv()->getTransportTriggerProtoID(), useRallyPoint, rallyPoint, attackMove, cInvalidCueIndex, true, false, cInvalidVector);
#ifndef BUILD_FINAL
            // Account for transport squad
            if (flyInSuccess)
            {
               gTriggerManager.incrementCreatePerfCount();
            }
#endif
         }
      }

      if (!flyInSuccess && useRallyPoint)
      {
         // Build the move command.
         BWorkCommand tempCommand;
         tempCommand.setWaypoints(&rallyPoint, 1);

         // Get the squad's army
         BArmy* pArmy = pSquad->getParentArmy();
         if (!pArmy)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = pSquad->getPlayerID();
            pArmy = gWorld->createArmy(objectParms);
            if (pArmy)
            {
               BEntityIDArray workingSquads;
               workingSquads.add(pSquad->getID());
               pArmy->addSquads(workingSquads, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
            }
         }

         // Set the command to be from a trigger to the army
         tempCommand.setRecipientType(BCommand::cArmy);
         tempCommand.setSenderType(BCommand::cTrigger);

         // Set the attack move
         tempCommand.setFlag(BWorkCommand::cFlagAttackMove, attackMove);

         //Give the command to the army.
         pArmy->queueOrder(&tempCommand);

#ifndef BUILD_FINAL
         gTriggerManager.incrementWorkPerfCount();
#endif
      }
   }

   if (useOutputCreatedSquad)
   {
      if (pSquad)
         getVar(cOutputCreatedSquad)->asSquad()->writeVar(createdSquadID);
      else
         getVar(cOutputCreatedSquad)->asSquad()->writeVar(cInvalidObjectID);
   }

   if (useOutputAddToList)
   {
      if (clearExisting)
      {
         getVar(cOutputAddToList)->asSquadList()->clear();
      }

      if (pSquad)
         getVar(cOutputAddToList)->asSquadList()->addSquad(createdSquadID);
   }
}

//============================================
// Create a squad with optional fly-in action
//============================================
void BTriggerEffect::teCreateSquadV7()
{
   enum 
   { 
      cInputProtoSquad = 1, 
      cInputCreatePosition = 2, 
      cInputPlayer = 3, 
      cOutputCreatedSquad = 4, 
      cOutputAddToList = 5, 
      cOutputClearExisting = 6, 
      cFlyInStart = 8,
      cFlyInEnd = 9,
      cRallyPoint = 10,
      cAttackMoveRallyPoint = 11,
      cFacing = 12,
   };

   long protoSquad = getVar(cInputProtoSquad)->asProtoSquad()->readVar();
   BVector createPos = getVar(cInputCreatePosition)->asVector()->readVar();
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   bool useOutputCreatedSquad = getVar(cOutputCreatedSquad)->isUsed();
   bool useOutputAddToList = getVar(cOutputAddToList)->isUsed();
   bool clearExisting = getVar(cOutputClearExisting)->isUsed() ? getVar(cOutputClearExisting)->asBool()->readVar() : false;
   bool useFlyInStart = getVar(cFlyInStart)->isUsed();
   bool useFlyInEnd = getVar(cFlyInEnd)->isUsed();
   BVector rallyPoint = getVar(cRallyPoint)->isUsed() ? getVar(cRallyPoint)->asVector()->readVar() : cInvalidVector;
   bool attackMove = getVar(cAttackMoveRallyPoint)->isUsed() ? getVar(cAttackMoveRallyPoint)->asBool()->readVar() : false;
   bool useFacing = getVar(cFacing)->isUsed();
   BVector facing =  useFacing ? getVar(cFacing)->asVector()->readVar() : cZAxisVector;
   BVector right; 
   right.assignCrossProduct(cYAxisVector, facing);
   right.normalize();
   bool flyInSuccess = false;

   // Create object.
   BEntityID createdSquadID = gWorld->createEntity(protoSquad, true, playerID, createPos, facing, right, true);
   BSquad* pSquad = gWorld->getSquad(createdSquadID);
   if (pSquad)
   {
#ifndef BUILD_FINAL
      gTriggerManager.incrementCreatePerfCount();
#endif

      bool useRallyPoint = (rallyPoint == cInvalidVector) ? false : true;
      if (useFlyInStart || useFlyInEnd)
      {
         BPlayer* pPlayer = gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 5493
         const BCiv* pCiv = pPlayer ? pPlayer->getCiv() : NULL;
//--
         long transportProtoID = pCiv ? pCiv->getTransportTriggerProtoID() : cInvalidProtoID;
         if (transportProtoID != cInvalidProtoID)
         {
            const BVector start = useFlyInStart ? getVar(cFlyInStart)->asVector()->readVar() : cInvalidVector;
            const BVector end = useFlyInEnd ? getVar(cFlyInEnd)->asVector()->readVar() : cInvalidVector;
            const BVector* pStart = (start == cInvalidVector) ? NULL : &start;
            const BVector* pEnd = (end == cInvalidVector) ? NULL : &end;

            // Create an order
            BSimOrder* pOrder = gSimOrderManager.createOrder();
            // Init order
            if (pOrder)  
            {
               pOrder->setPriority(BSimOrder::cPriorityTrigger);
            }

            flyInSuccess = BSquadActionTransport::flyInSquad(pOrder, pSquad, pStart, createPos, cZAxisVector, cXAxisVector, pEnd, playerID, pPlayer->getCiv()->getTransportTriggerProtoID(), useRallyPoint, rallyPoint, attackMove, cInvalidCueIndex, true, useFacing, facing);
#ifndef BUILD_FINAL
            // Account for transport squad
            if (flyInSuccess)
            {
               gTriggerManager.incrementCreatePerfCount();
            }
#endif
         }
      }

      if (!flyInSuccess && useRallyPoint)
      {
         BVectorArray wayPoints;
         wayPoints.add(rallyPoint);
         
         if (useFacing)
         {
            facing.normalize();
            facing *= (0.5f + pSquad->getObstructionRadius());
            facing += rallyPoint;
            wayPoints.add(facing);
         }

         // Build the move command.
         BWorkCommand tempCommand;         
         tempCommand.setWaypoints(wayPoints.getPtr(), wayPoints.getNumber());

         // Get the squad's army
         BArmy* pArmy = pSquad->getParentArmy();
         if (!pArmy)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = pSquad->getPlayerID();
            pArmy = gWorld->createArmy(objectParms);
            if (pArmy)
            {
               BEntityIDArray workingSquads;
               workingSquads.add(pSquad->getID());
               pArmy->addSquads(workingSquads, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
            }
         }

         // Set the command to be from a trigger to the army
         tempCommand.setRecipientType(BCommand::cArmy);
         tempCommand.setSenderType(BCommand::cTrigger);

         // Set the attack move
         tempCommand.setFlag(BWorkCommand::cFlagAttackMove, attackMove);

         //Give the command to the army.
         pArmy->queueOrder(&tempCommand);

#ifndef BUILD_FINAL
         gTriggerManager.incrementWorkPerfCount();
#endif
      }
   }

   if (useOutputCreatedSquad)
   {
      if (pSquad)
         getVar(cOutputCreatedSquad)->asSquad()->writeVar(createdSquadID);
      else
         getVar(cOutputCreatedSquad)->asSquad()->writeVar(cInvalidObjectID);
   }

   if (useOutputAddToList)
   {
      if (clearExisting)
      {
         getVar(cOutputAddToList)->asSquadList()->clear();
      }

      if (pSquad)
         getVar(cOutputAddToList)->asSquadList()->addSquad(createdSquadID);
   }
}

//==============================================================================
// BTriggerEffect::tePayCost()
//==============================================================================
void BTriggerEffect::tePayCost()
{
   enum { cInputPlayer = 1, cInputCost = 2, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BCost cost = getVar(cInputCost)->asCost()->readVar();
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPlayer->payCost(&cost);
}

//==============================================================================
// BTriggerEffect::teRefundCost()
//==============================================================================
void BTriggerEffect::teRefundCost()
{
   enum { cInputPlayer = 1, cInputCost = 2, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BCost cost = getVar(cInputCost)->asCost()->readVar();
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPlayer->refundCost(&cost);
}


//==============================================================================
// BTriggerEffect::teCountIncrement()
//==============================================================================
void BTriggerEffect::teCountIncrement()
{
   enum { cInputCount = 1, cOutputCount = 2, };
   long count = getVar(cInputCount)->asInteger()->readVar();

   // Increment.
   count++;

   getVar(cOutputCount)->asInteger()->writeVar(count);
}


//==============================================================================
// BTriggerEffect::teCountDecrement()
//==============================================================================
void BTriggerEffect::teCountDecrement()
{
   enum { cInputCount = 1, cOutputCount = 2, };
   long count = getVar(cInputCount)->asInteger()->readVar();

   // Decrement
   count--;

   getVar(cOutputCount)->asInteger()->writeVar(count);
}


//==============================================================================
// BTriggerEffect::teRandomLocation()
//==============================================================================
void BTriggerEffect::teRandomLocation()
{
   switch (getVersion())
   {
      case 3:
         teRandomLocationV3();
         break;

      case 4:
         teRandomLocationV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is obsolete!");
         break;
   }
}

//==============================================================================
// Find a random location and optionally test for obstructions and pathing
//==============================================================================
void BTriggerEffect::teRandomLocationV3()
{
   enum 
   { 
      cInputLocation = 1, 
      cInputInnerRadius = 2,       
      cOutputLocation = 3, 
      cInputOuterRadius = 4,
      cInputTestObstructions = 5,
      cInputTestPathing = 6,
      cInputProtoObjectID = 7,
   };

   bool innerUsed = getVar(cInputInnerRadius)->isUsed();
   BVector inLoc = getVar(cInputLocation)->asVector()->readVar();
   float innerRad = innerUsed ? getVar(cInputInnerRadius)->asFloat()->readVar() : 0.0f;
   float outerRad = getVar(cInputOuterRadius)->asFloat()->readVar();
   bool testObstruction = getVar(cInputTestObstructions)->isUsed() ? getVar(cInputTestObstructions)->asBool()->readVar() : false;
   bool testPathing = getVar(cInputTestPathing)->isUsed() ? getVar(cInputTestPathing)->asBool()->readVar() : false;
   BProtoObjectID protoObjectID = (BProtoObjectID)getVar(cInputProtoObjectID)->isUsed() ? getVar(cInputProtoObjectID)->asProtoObject()->readVar() : cInvalidProtoID;
   
   BVector outLoc = cInvalidVector;   
   bool found = false;
   uint searchCount = 0;
   while (!found)
   {
      outLoc = BSimHelper::randomCircularDistribution(inLoc, outerRad, innerRad);
      found = true;
      if (testObstruction)
      {
         BVector suggestion = cInvalidVector;
         BVector tempForward = outLoc - inLoc;
         tempForward.normalize();
         const BVector forward = tempForward;
         long losMode = BWorld::cCPLOSDontCare;
         DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPSetPlacementSuggestion | BWorld::cCPExpandHull | BWorld::cCPIgnoreMoving | BWorld::cCPIgnoreParkingAndFlatten;
         if (protoObjectID == cInvalidObjectID)
         {
            flags |= BWorld::cCPIgnorePlacementRules;            
         }
         found = gWorld->checkPlacement(protoObjectID, cInvalidPlayerID, outLoc, suggestion, forward, losMode, flags);
         outLoc = suggestion;
      }
      
      if (found && testPathing)
      {
//-- FIXING PREFIX BUG ID 5494
         const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjectID);
//--
         float pathingRadius = (pProtoObject) ? pProtoObject->getObstructionRadius() : 2.0f;
         float range = 0.0f;
         const BEntityIDArray* ignoreList = NULL;
         static BPath path;
         path.reset();
         bool skipBegin = false;
         bool fullPathOnly = true;
         long targetID = -1L;
         int pathType = BPather::cLongRange;
         int pathClass = BPather::cLandPath;         
         bool canJump = false;               //Don't put random locations in a jump area
         long result = gPather.findPath(&gObsManager, outLoc, inLoc, pathingRadius, range, ignoreList, &path, skipBegin, fullPathOnly, canJump, targetID, pathType, pathClass);         
         found = (BPath::cFull == result);
      }

      if (!found)
      {
         searchCount++;
      }

      if (searchCount >= TE_RANDOM_LOCATION_SEARCH_MAX)
      {
         break;
      }
   }
      
   #ifndef BUILD_FINAL
      if ((testObstruction || testPathing) && !found)
      {
         BTRIGGER_ASSERTM(false, "WARNING:  Random location obstructions and/or pathing could not be verified.  Reevaluate trigger parameters.");
      }
   #endif

   gTerrainSimRep.getHeight(outLoc, true);
   getVar(cOutputLocation)->asVector()->writeVar(outLoc);
}

//==============================================================================
// Find a random location and optionally test for obstructions and pathing
//==============================================================================
void BTriggerEffect::teRandomLocationV4()
{
   enum 
   { 
      cInputLocation = 1, 
      cInputInnerRadius = 2,       
      cOutputLocation = 3, 
      cInputOuterRadius = 4,
      cInputTestObstructions = 5,
      cInputTestPathing = 6,
      cInputProtoObjectID = 7,
      cInProtoSquadID = 8,
   };

   bool innerUsed = getVar(cInputInnerRadius)->isUsed();
   BVector inLoc = getVar(cInputLocation)->asVector()->readVar();
   float innerRad = innerUsed ? getVar(cInputInnerRadius)->asFloat()->readVar() : 0.0f;
   float outerRad = getVar(cInputOuterRadius)->asFloat()->readVar();
   bool testObstruction = getVar(cInputTestObstructions)->isUsed() ? getVar(cInputTestObstructions)->asBool()->readVar() : false;
   bool testPathing = getVar(cInputTestPathing)->isUsed() ? getVar(cInputTestPathing)->asBool()->readVar() : false;
   BProtoObjectID protoObjectID = (BProtoObjectID)getVar(cInputProtoObjectID)->isUsed() ? getVar(cInputProtoObjectID)->asProtoObject()->readVar() : cInvalidProtoID;
   BProtoSquadID protoSquadID = getVar(cInProtoSquadID)->isUsed() ? getVar(cInProtoSquadID)->asProtoSquad()->readVar() : cInvalidProtoSquadID;

   BVector outLoc = cInvalidVector;   
   bool found = false;
   uint searchCount = 0;
   while (!found)
   {
      outLoc = BSimHelper::randomCircularDistribution(inLoc, outerRad, innerRad);
      found = true;
      if (testObstruction)
      {
         BVector suggestion = cInvalidVector;
         BVector tempForward = outLoc - inLoc;
         tempForward.normalize();
         const BVector forward = tempForward;
         long losMode = BWorld::cCPLOSDontCare;
         DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPSetPlacementSuggestion | BWorld::cCPExpandHull | BWorld::cCPIgnoreMoving | BWorld::cCPIgnoreParkingAndFlatten;
         if ((protoObjectID == cInvalidObjectID) && (protoSquadID == cInvalidProtoSquadID))
         {
            flags |= BWorld::cCPIgnorePlacementRules;            
         }
         bool useSquadObstruction = (protoSquadID == cInvalidProtoSquadID) ? false : true;
         found = gWorld->checkPlacement(protoObjectID, cInvalidPlayerID, outLoc, suggestion, forward, losMode, flags, 1, NULL, NULL, -1, protoSquadID, useSquadObstruction);
         outLoc = suggestion;
      }

      if (found && testPathing)
      {
         float pathingRadius = -1.0f;
         BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);
         if (pProtoSquad)
         {
            float obsX = 0.0f;
            float obsZ = 0.0f;
            pProtoSquad->calcObstructionRadiiFromObjects(obsX, obsZ);
            pathingRadius = Math::Max(obsX, obsZ);
         }

         if (pathingRadius < 0.0f)
         {
//-- FIXING PREFIX BUG ID 5496
            const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjectID);
//--
            pathingRadius = (pProtoObject) ? pProtoObject->getObstructionRadius() : 2.0f;
         }
         float range = 0.0f;
         const BEntityIDArray* ignoreList = NULL;
         static BPath path;
         path.reset();
         bool skipBegin = false;
         bool fullPathOnly = true;
         long targetID = -1L;
         int pathType = BPather::cLongRange;
         int pathClass = BPather::cLandPath;         
         bool canJump = false;               //Don't put random locations in a jump area
         long result = gPather.findPath(&gObsManager, outLoc, inLoc, pathingRadius, range, ignoreList, &path, skipBegin, fullPathOnly, canJump, targetID, pathType, pathClass);         
         found = (BPath::cFull == result);
      }

      if (!found)
      {
         searchCount++;
      }

      if (searchCount >= TE_RANDOM_LOCATION_SEARCH_MAX)
      {
         break;
      }
   }

   #ifndef BUILD_FINAL
      if ((testObstruction || testPathing) && !found)
      {
         BTRIGGER_ASSERTM(false, "WARNING:  Random location obstructions and/or pathing could not be verified.  Reevaluate trigger parameters.");
      }
   #endif

   gTerrainSimRep.getHeight(outLoc, true);
   getVar(cOutputLocation)->asVector()->writeVar(outLoc);
}

//==============================================================================
// BTriggerEffect::teShutdown()
//==============================================================================
void BTriggerEffect::teShutdown()
{
   BTRIGGER_ASSERT(getParentTriggerScript());
   getParentTriggerScript()->markForCleanup();
}


//==============================================================================
// BTriggerEffect::teLocationTieToGround()
//==============================================================================
void BTriggerEffect::teLocationTieToGround()
{
   enum { cInputLocation = 1, cOutputLocation = 2, };
   BVector inLoc = getVar(cInputLocation)->asVector()->readVar();
   BVector outLoc = inLoc;
   gTerrainSimRep.getHeight(outLoc, true);
   getVar(cOutputLocation)->asVector()->writeVar(outLoc);
}


//==============================================================================
// BTriggerEffect::teLocationAdjust()
//==============================================================================
void BTriggerEffect::teLocationAdjust()
{
   switch (getVersion())
   {
      case 2:
         teLocationAdjustV2();
         break;

      case 3:
         teLocationAdjustV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// BTriggerEffect::teLocationAdjustV2()
//==============================================================================
void BTriggerEffect::teLocationAdjustV2()
{
   enum { cInputSourceLoc = 1, cInputAdjustX = 2, cOutputResultLoc = 3, cInputAdjustY = 4, cInputAdjustZ = 5, cInputAdjustVector = 6, };
   BVector inLoc = getVar(cInputSourceLoc)->asVector()->readVar();
   BVector outLoc = inLoc;

   if (getVar(cInputAdjustX)->isUsed())
      outLoc.x += getVar(cInputAdjustX)->asFloat()->readVar();
   if (getVar(cInputAdjustY)->isUsed())
      outLoc.y += getVar(cInputAdjustY)->asFloat()->readVar();
   if (getVar(cInputAdjustZ)->isUsed())
      outLoc.z += getVar(cInputAdjustZ)->asFloat()->readVar();
   if (getVar(cInputAdjustVector)->isUsed())
      outLoc += getVar(cInputAdjustVector)->asVector()->readVar();

   getVar(cOutputResultLoc)->asVector()->writeVar(outLoc);
}

//==============================================================================
// Adjust the elements of a vector
//==============================================================================
void BTriggerEffect::teLocationAdjustV3()
{
   enum 
   { 
      cInputSourceLoc = 1, 
      cInputAdjustX = 2, 
      cOutputResultLoc = 3, 
      cInputAdjustY = 4, 
      cInputAdjustZ = 5, 
      cInputAdjustVector = 6, 
      cInUseMultiply = 7,
   };

   BVector inLoc = getVar(cInputSourceLoc)->asVector()->readVar();
   BVector outLoc = inLoc;
   bool useMultiply = getVar(cInUseMultiply)->isUsed() ? getVar(cInUseMultiply)->asBool()->readVar() : false;

   if (useMultiply)
   {
      if (getVar(cInputAdjustX)->isUsed())
         outLoc.x *= getVar(cInputAdjustX)->asFloat()->readVar();
      if (getVar(cInputAdjustY)->isUsed())
         outLoc.y *= getVar(cInputAdjustY)->asFloat()->readVar();
      if (getVar(cInputAdjustZ)->isUsed())
         outLoc.z *= getVar(cInputAdjustZ)->asFloat()->readVar();
      if (getVar(cInputAdjustVector)->isUsed())
      {
         BVector adjustVector = getVar(cInputAdjustVector)->asVector()->readVar();
         outLoc.x *= adjustVector.x;
         outLoc.y *= adjustVector.y;
         outLoc.z *= adjustVector.z;
      }
   }
   else
   {
      if (getVar(cInputAdjustX)->isUsed())
         outLoc.x += getVar(cInputAdjustX)->asFloat()->readVar();
      if (getVar(cInputAdjustY)->isUsed())
         outLoc.y += getVar(cInputAdjustY)->asFloat()->readVar();
      if (getVar(cInputAdjustZ)->isUsed())
         outLoc.z += getVar(cInputAdjustZ)->asFloat()->readVar();
      if (getVar(cInputAdjustVector)->isUsed())
         outLoc += getVar(cInputAdjustVector)->asVector()->readVar();
   }

   getVar(cOutputResultLoc)->asVector()->writeVar(outLoc);
}

//==============================================================================
// BTriggerEffect::teLaunchProjectile()
//==============================================================================
void BTriggerEffect::teLaunchProjectile()
{
   uint version = getVersion();
   if (version == 1)
   {
      teLaunchProjectileV1();
   }
   else if (version == 2)
   {
      teLaunchProjectileV2();
   }
   else if (version == 3)
   {
      teLaunchProjectileV3();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teLaunchProjectileV1()
//==============================================================================
void BTriggerEffect::teLaunchProjectileV1()
{
   enum { cInputPlayer = 1, cInputProtoObject = 2, cInputLaunchLocation = 3, cInputTargetLocation = 4, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long protoObject = getVar(cInputProtoObject)->asProtoObject()->readVar();
   BVector launchLoc = getVar(cInputLaunchLocation)->asVector()->readVar();
   BVector targetLoc = getVar(cInputTargetLocation)->asVector()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   BProtoObject* pProtoObj = (pPlayer ? pPlayer->getProtoObject(protoObject) : gDatabase.getGenericProtoObject(protoObject));
   if (!pProtoObj)
      return;
   BTactic *pTactic = pProtoObj->getTactic();
   if (!pTactic)
      return;
   BProtoAction *pProtoAction = pTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false);
   if (!pProtoAction)
      return;

   // Create the projectile
   BObjectCreateParms parms;
   parms.mPlayerID = playerID;
   parms.mProtoObjectID = pProtoAction->getProjectileID();
   parms.mPosition = launchLoc;

   gWorld->launchProjectile(parms, targetLoc, XMVectorZero(), XMVectorZero(), pProtoAction->getDamagePerSecond(), pProtoAction, pProtoAction);
}


//==============================================================================
// BTriggerEffect::teLaunchProjectileV2()
//==============================================================================
void BTriggerEffect::teLaunchProjectileV2()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputProtoObject = 2, 
      cInputLaunchLocation = 3, 
      cInputTargetLocation = 4, 
      cInputTargetUnit = 5, 
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BProtoObjectID protoObject = getVar(cInputProtoObject)->asProtoObject()->readVar();
   BVector launchLoc = getVar(cInputLaunchLocation)->asVector()->readVar();
   bool useTargetLoc = getVar(cInputTargetLocation)->isUsed();
   bool useTargetUnit = getVar(cInputTargetUnit)->isUsed();
   if (!useTargetLoc && !useTargetUnit)
      return;
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   BProtoObject* pProjProtoObject = (pPlayer ? pPlayer->getProtoObject(protoObject) : gDatabase.getGenericProtoObject(protoObject));
   if (!pProjProtoObject)
      return;
   BTactic* pProjTactic = pProjProtoObject->getTactic();
   if (!pProjTactic)
      return;
   BProtoAction* pProtoAction = pProjTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false);
   if (!pProtoAction)
      return;

   // Create the projectile
   BObjectCreateParms parms;
   parms.mPlayerID = playerID;
   parms.mProtoObjectID = pProtoAction->getProjectileID();
   parms.mPosition = launchLoc;

   if (useTargetUnit)
   {
      BEntityID targetUnitID = getVar(cInputTargetUnit)->asUnit()->readVar();
      BUnit* pTargetUnit = gWorld->getUnit(targetUnitID);
      if (pTargetUnit)
      {
         // Calculate deviation and account for unit movement
         BVector targetPos = pTargetUnit->getPosition();
//          BVector diff = pTargetUnit->getVisualCenter() - targetPos;         
//          float obsY = diff.y;
//          const BProtoObject* pProtoObject = pTargetUnit->getProtoObject();
//          if (pProtoObject)
//          {
//             obsY = pProtoObject->getObstructionRadiusY();
//          }         
//         BVector targetOffset = BVector(0.0f, Math::Min(obsY, diff.y), 0.0f);

         // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
         BVector targetOffset = pTargetUnit->getSimCenter() - targetPos;

         float projectileSpeed = pProjProtoObject->getDesiredVelocity();
         BVector targetVelocity = pTargetUnit->getVelocity();
         float targetSpeed = targetVelocity.length();
         BVector targetingLead = cOriginVector;  
         bool useMovingAccuracy = false;
         float targetDist = launchLoc.distance(targetPos);
         if (pTargetUnit->isMoving() && (projectileSpeed > cFloatCompareEpsilon) && (targetSpeed > cFloatCompareEpsilon))
         {            
            if (targetDist > cFloatCompareEpsilon)
            {
               float t = targetDist / projectileSpeed;
               targetVelocity *= (Math::Min(targetSpeed, pProtoAction->getMaxVelocityLead()) / targetSpeed);
               BVector newTargetPos = targetPos + (targetVelocity * t);
               targetingLead = newTargetPos - targetPos;
               float targetDist2 = launchLoc.distance(newTargetPos);
               targetingLead *= (targetDist2 / targetDist);
               useMovingAccuracy = true;
            }
         }

         float accuracy = useMovingAccuracy ? pProtoAction->getMovingAccuracy() : pProtoAction->getAccuracy();
         float range = targetDist * 2.0f;
         float maxDeviation = useMovingAccuracy ? pProtoAction->getMovingMaxDeviation() : pProtoAction->getMaxDeviation();
         float accuracyDistFactor = pProtoAction->getAccuracyDistanceFactor();
         float accuracyDeviationFactor = pProtoAction->getAccuracyDeviationFactor();
         targetPos += targetOffset;
         targetOffset += gWorld->getProjectileDeviation(launchLoc, targetPos, targetingLead, accuracy, range, maxDeviation, accuracyDistFactor, 
            accuracyDeviationFactor);

         BVector orientation = cOriginVector;
         float dps = pProtoAction->getDamagePerSecond();         
         gWorld->launchProjectile(parms, targetOffset, targetingLead, orientation, dps, pProtoAction, (IDamageInfo*)pProtoAction, cInvalidObjectID, 
            pTargetUnit, -1, true);
      }
   }
   else if (useTargetLoc)
   {
      BVector targetLoc = getVar(cInputTargetLocation)->asVector()->readVar();
      gWorld->launchProjectile(parms, targetLoc, XMVectorZero(), XMVectorZero(), pProtoAction->getDamagePerSecond(), pProtoAction, pProtoAction, cInvalidObjectID, NULL, -1, true);
   }
}


//==============================================================================
// BTriggerEffect::teLaunchProjectileV3()
//==============================================================================
void BTriggerEffect::teLaunchProjectileV3()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputProtoObject = 2, 
      cInputLaunchLocation = 3, 
      cInputTargetLocation = 4, 
      cInputTargetUnit = 5,
      cInputOverrideDamage = 6,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BProtoObjectID protoObject = getVar(cInputProtoObject)->asProtoObject()->readVar();
   BVector launchLoc = getVar(cInputLaunchLocation)->asVector()->readVar();
   bool useTargetLoc = getVar(cInputTargetLocation)->isUsed();
   bool useTargetUnit = getVar(cInputTargetUnit)->isUsed();
   if (!useTargetLoc && !useTargetUnit)
      return;
   bool useOverrideDamage = getVar(cInputOverrideDamage)->isUsed();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   BProtoObject* pProjProtoObject = (pPlayer ? pPlayer->getProtoObject(protoObject) : gDatabase.getGenericProtoObject(protoObject));
   if (!pProjProtoObject)
      return;
   BTactic* pProjTactic = pProjProtoObject->getTactic();
   if (!pProjTactic)
      return;
   BProtoAction* pProtoAction = pProjTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false);
   if (!pProtoAction)
      return;

   // Create the projectile
   BObjectCreateParms parms;
   parms.mPlayerID = playerID;
   parms.mProtoObjectID = pProtoAction->getProjectileID();
   parms.mPosition = launchLoc;

   if (useTargetUnit)
   {
      BEntityID targetUnitID = getVar(cInputTargetUnit)->asUnit()->readVar();
      BUnit* pTargetUnit = gWorld->getUnit(targetUnitID);
      if (pTargetUnit)
      {
         // Calculate deviation and account for unit movement
         BVector targetPos = pTargetUnit->getPosition();
//          BVector diff = pTargetUnit->getVisualCenter() - targetPos;         
//          float obsY = diff.y;
//          const BProtoObject* pProtoObject = pTargetUnit->getProtoObject();
//          if (pProtoObject)
//          {
//             obsY = pProtoObject->getObstructionRadiusY();
//          }         
//          BVector targetOffset = BVector(0.0f, Math::Min(obsY, diff.y), 0.0f);

         // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
         BVector targetOffset = pTargetUnit->getSimCenter() - targetPos;

         float projectileSpeed = pProjProtoObject->getDesiredVelocity();
         BVector targetVelocity = pTargetUnit->getVelocity();
         float targetSpeed = targetVelocity.length();
         BVector targetingLead = cOriginVector;  
         bool useMovingAccuracy = false;
         float targetDist = launchLoc.distance(targetPos);
         if (pTargetUnit->isMoving() && (projectileSpeed > cFloatCompareEpsilon) && (targetSpeed > cFloatCompareEpsilon))
         {            
            if (targetDist > cFloatCompareEpsilon)
            {
               float t = targetDist / projectileSpeed;
               targetVelocity *= (Math::Min(targetSpeed, pProtoAction->getMaxVelocityLead()) / targetSpeed);
               BVector newTargetPos = targetPos + (targetVelocity * t);
               targetingLead = newTargetPos - targetPos;
               float targetDist2 = launchLoc.distance(newTargetPos);
               targetingLead *= (targetDist2 / targetDist);
               useMovingAccuracy = true;
            }
         }

         float accuracy = useMovingAccuracy ? pProtoAction->getMovingAccuracy() : pProtoAction->getAccuracy();
         float range = targetDist * 2.0f;
         float maxDeviation = useMovingAccuracy ? pProtoAction->getMovingMaxDeviation() : pProtoAction->getMaxDeviation();
         float accuracyDistFactor = pProtoAction->getAccuracyDistanceFactor();
         float accuracyDeviationFactor = pProtoAction->getAccuracyDeviationFactor();
         targetPos += targetOffset;
         targetOffset += gWorld->getProjectileDeviation(launchLoc, targetPos, targetingLead, accuracy, range, maxDeviation, accuracyDistFactor, 
            accuracyDeviationFactor);

         BVector orientation = cOriginVector;
         float dps = pProtoAction->getDamagePerSecond();
         if (useOverrideDamage)
            dps = getVar(cInputOverrideDamage)->asFloat()->readVar();
         gWorld->launchProjectile(parms, targetOffset, targetingLead, orientation, dps, pProtoAction, (IDamageInfo*)pProtoAction, cInvalidObjectID, pTargetUnit, -1, true);
      }
   }
   else if (useTargetLoc)
   {
      BVector targetLoc = getVar(cInputTargetLocation)->asVector()->readVar();
      float dps = pProtoAction->getDamagePerSecond();
      if (useOverrideDamage)
         dps = getVar(cInputOverrideDamage)->asFloat()->readVar();
      gWorld->launchProjectile(parms, targetLoc, XMVectorZero(), XMVectorZero(), dps, pProtoAction, pProtoAction, cInvalidObjectID, NULL, -1, true);
   }
}


//==============================================================================
// BTriggerEffect::teTechActivate()
//==============================================================================
void BTriggerEffect::teTechActivate()
{
   enum { cInputPlayer = 1, cInputTech = 2, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long techID = getVar(cInputTech)->asTech()->readVar();
   
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BTechTree *pTechTree = pPlayer->getTechTree();
   if (!pTechTree)
      return;

   pTechTree->activateTech(techID, -1);
}


//==============================================================================
// BTriggerEffect::teTechDeactivate()
//==============================================================================
void BTriggerEffect::teTechDeactivate()
{
   enum { cInputPlayer = 1, cInputTech = 2, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long techID = getVar(cInputTech)->asTech()->readVar();

   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BTechTree *pTechTree = pPlayer->getTechTree();
   if (!pTechTree)
      return;

   pTechTree->deactivateTech(techID, -1);
}

//==============================================================================
// BTriggerEffect::teUnload()
//==============================================================================
void BTriggerEffect::teUnload()
{
   switch (getVersion())
   {
      case 3:
         teUnloadV3();
         break;

      case 4:
         teUnloadV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;            
   }
}

//==============================================================================
// Unload/Ungarrison the squads
//==============================================================================
void BTriggerEffect::teUnloadV3()
{
   enum 
   { 
      cSquad = 3,
      cSquadList = 5, 
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No Squad or SquadList designated!");
      return;
   }

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }   

   // Unload squads
   BEntityIDArray garrisonedSquads;
   uint numSquads = squadList.getSize();
   for (uint i = 0; i < numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         pSquad->queueUnload(BSimOrder::cPriorityTrigger, garrisonedSquads);
      }
   }
}

//==============================================================================
// Unload/Ungarrison the squads
//==============================================================================
void BTriggerEffect::teUnloadV4()
{
   enum 
   { 
      cSquad = 3,
      cSquadList = 5, 
      cGarrisonedSquad = 6,
      cGarrisonedSquadList = 7, 
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useGarrisonedSquad = getVar(cGarrisonedSquad)->isUsed();
   bool useGarrisonedSquadList = getVar(cGarrisonedSquadList)->isUsed();

   if (!useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No Squad or SquadList designated!");
      return;
   }

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }   

   // List of garrisoned squads to unload
   BEntityIDArray garrisonedSquads;
   if (useGarrisonedSquadList)
      garrisonedSquads = getVar(cGarrisonedSquadList)->asSquadList()->readVar();
   if (useGarrisonedSquad)
      garrisonedSquads.add(getVar(cGarrisonedSquad)->asSquad()->readVar());

   // Unload squads
   uint numSquads = squadList.getSize();
   for (uint i = 0; i < numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         pSquad->queueUnload(BSimOrder::cPriorityTrigger, garrisonedSquads);
      }
   }
}

//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teMove()
{
   switch (getVersion())
   {
      case 6:
         teMoveV6();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//=============================================================================
// Move with optional attack and queue parameters
//=============================================================================
void BTriggerEffect::teMoveV6()
{
   enum 
   { 
      cSquad = 1, 
      cSquadList = 5, 
      cTargetUnit = 6, 
      cTargetLocation = 2, 
      cTargetSquad = 7, 
      cAttackMove = 8,
      cQueueOrder = 9,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useTargetUnit = getVar(cTargetUnit)->isUsed();
   bool useTargetLocation = getVar(cTargetLocation)->isUsed();
   bool useTargetSquad = getVar(cTargetSquad)->isUsed();
   bool attackMove = getVar(cAttackMove)->isUsed() ? getVar(cAttackMove)->asBool()->readVar() : false;
   bool queueOrder = getVar(cQueueOrder)->isUsed() ? getVar(cQueueOrder)->asBool()->readVar() : false;

   BEntityID targetUnitID = useTargetUnit ? getVar(cTargetUnit)->asUnit()->readVar() : cInvalidObjectID;
   BEntityID targetSquadID = useTargetSquad ? getVar(cTargetSquad)->asSquad()->readVar() : cInvalidObjectID;
   BVector targetLocation = useTargetLocation ? getVar(cTargetLocation)->asVector()->readVar() : cInvalidVector;
   if (useTargetUnit && !gWorld->getUnit(targetUnitID))
      useTargetUnit = false;
   if (useTargetSquad && !gWorld->getSquad(targetSquadID))
      useTargetSquad = false;
   if (!useSquad && !useSquadList)
      return;
   if (!useTargetUnit && !useTargetSquad && !useTargetLocation)
      return;

   // Get our list of squads.
   BEntityIDArray workingSquads;
   if (useSquadList) // squadlist first, then squad.
   {
      workingSquads = getVar(cSquadList)->asSquadList()->readVar();
   }
   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      if (gWorld->getSquad(squadID))
         workingSquads.uniqueAdd(squadID);
   }

   // Bail now if we have no squads to do the work.
   uint numWorkingSquads = workingSquads.getSize();
   if (numWorkingSquads == 0)
      return;

#ifndef BUILD_FINAL
   gTriggerManager.incrementWorkPerfCount(numWorkingSquads);
#endif

   // Group squads per player
   // MPB [8/27/2008] - Added the "filterOutSpartanHijackedEntities" function to remove hijacked vehicles from the list
   // of entities to command.  This is a hack to work around the fact that there is no player ID validation on the set
   // of entities that the trigger script wants to command, and because hijacking will change the playerID on a unit/squad.
   // This fixes bug PHX-9580.
   BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(workingSquads, filterOutSpartanHijackedEntities);
   uint numPlayerEntityIDs = playerEntityIDs.getSize();
   for (uint i = 0; i < numPlayerEntityIDs; i++)
   {
      uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
      if (numEntities > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            // Platoon the squads
            pArmy->addSquads(playerEntityIDs[i].mEntityIDs, !gConfig.isDefined(cConfigClassicPlatoonGrouping));

            // Setup the command
            BWorkCommand tempCommand;
            if (useTargetUnit)
            {
               tempCommand.setUnitID(targetUnitID);
            }
            else if (useTargetSquad)
            {
//-- FIXING PREFIX BUG ID 5282
               const BSquad* pTempSquad = gWorld->getSquad(targetSquadID);
//--
               if (pTempSquad)
               {
                  tempCommand.setUnitID(pTempSquad->getLeader());
               }
               else
                  return;
            }
            else if (useTargetLocation)
               tempCommand.setWaypoints(&targetLocation, 1);

            // Set the command to be from a trigger to the army
            tempCommand.setRecipientType(BCommand::cArmy);
            tempCommand.setSenderType(BCommand::cTrigger);

            // Set the attack move
            tempCommand.setFlag(BWorkCommand::cFlagAttackMove, attackMove);

            // Set the queue order
            tempCommand.setFlag(BCommand::cFlagAlternate, queueOrder);

            // Give the command to the army.
            pArmy->queueOrder(&tempCommand);
         }               
      }
   }
}

//==============================================================================
// Find correct version
//==============================================================================
void BTriggerEffect::teTransportSquads()
{
   switch (getVersion())
   {
      case 4:
         teTransportSquadsV4();
         break;

      case 5:
         teTransportSquadsV5();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Transport squads
//==============================================================================
void BTriggerEffect::teTransportSquadsV4()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputSquadList = 2, 
      cInputPickupLoc = 4, 
      cInputDropoffLoc = 5, 
      cInputNumTransports = 6,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   const BEntityIDArray& squadsToTransport = getVar(cInputSquadList)->asSquadList()->readVar();
   long protoObject = gWorld->getPlayer(playerID)->getCiv()->getTransportProtoID();
   BVector pickupLoc = getVar(cInputPickupLoc)->asVector()->readVar();
   BVector dropoffLoc = getVar(cInputDropoffLoc)->asVector()->readVar();

   // Create an order
   BSimOrder* pOrder = gSimOrderManager.createOrder();
   // Init order
   if (pOrder)  
   {
      pOrder->setPriority(BSimOrder::cPriorityTrigger);
   }

   BSquadActionTransport::transportSquads(pOrder, squadsToTransport, dropoffLoc, playerID, protoObject);
}

//==============================================================================
// Transport squads
//==============================================================================
void BTriggerEffect::teTransportSquadsV5()
{
   enum 
   { 
      cPlayer = 1, 
      cSquadList = 2, 
      cDropoff = 5, 
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   const BEntityIDArray& squadsToTransport = getVar(cSquadList)->asSquadList()->readVar();
   BProtoObjectID protoObject = gWorld->getPlayer(playerID)->getCiv()->getTransportProtoID();
   BVector dropoffLoc = getVar(cDropoff)->asVector()->readVar();

   BSimOrder* pOrder = gSimOrderManager.createOrder();
   if (!pOrder)  
      return;
   pOrder->setPriority(BSimOrder::cPriorityTrigger);

   BSquadActionTransport::transportSquads(pOrder, squadsToTransport, dropoffLoc, playerID, protoObject);
}

//==============================================================================
// Match correct version
//==============================================================================
void BTriggerEffect::teCarpetBomb()
{
   switch (getVersion())
   {
      case 3:
         teCarpetBombV3();
         break;

      case 4:
         teCarpetBombV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//===============================================================================================================
// Initiate a carpet bomb action directly on the squad with attack run distance and number of attacks parameters
//===============================================================================================================
void BTriggerEffect::teCarpetBombV3()
{
   enum 
   { 
      cInputLocation = 2, 
      cInputUnit = 3, 
      cInputAttackDistance = 4,
      cInputAttackNumber = 5,
   };

   BVector location = getVar(cInputLocation)->asVector()->readVar();
   BEntityID unitID = getVar(cInputUnit)->asUnit()->readVar();
   BUnit* pUnit = gWorld->getUnit(unitID);
   if (!pUnit)
      return;
//-- FIXING PREFIX BUG ID 5283
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   if (!pSquad)
      return;
   
   // Only fliers can perform a carpet bomb action
   const BProtoObject* pProtoObject = pUnit->getProtoObject();
   if (!pProtoObject)
      return;
   if (!pProtoObject->getFlagFlying())
      return;

   float attackDistance = getVar(cInputAttackDistance)->isUsed() ? getVar(cInputAttackDistance)->asFloat()->readVar() : cDefaultAttackRunDistance;
   uint attackNumber = getVar(cInputAttackNumber)->isUsed() ? (uint)getVar(cInputAttackNumber)->asInteger()->readVar() : cDefaultAttackNumber;

   // Create an order
   BSimOrder* pOrder = gSimOrderManager.createOrder();
   if (!pOrder)
      return;

   // Create the target
   BSimTarget target(location);

   // Init order
   pOrder->setTarget(target);
   pOrder->setPriority(BSimOrder::cPriorityTrigger);

   BSquadActionCarpetBomb::carpetBomb(pOrder, pSquad->getID(), location, pSquad->getPosition(), attackDistance, attackNumber);
}

//===============================================================================================================
// Initiate a carpet bomb action directly on the squad with attack run distance and number of attacks parameters
//===============================================================================================================
void BTriggerEffect::teCarpetBombV4()
{
   enum 
   { 
      cInputLocation = 2, 
      cInputAttackDistance = 4,
      cInputAttackNumber = 5,
      cInputSquad = 6,
   };

   BVector location = getVar(cInputLocation)->asVector()->readVar();
   BEntityID squadID = getVar(cInputSquad)->asSquad()->readVar();
//-- FIXING PREFIX BUG ID 5284
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   if (!pSquad)
      return;

   // Only fliers can perform a carpet bomb action
   const BProtoObject* pProtoObject = pSquad->getProtoObject();
   if (!(pProtoObject && pProtoObject->getFlagFlying()))
      return;   

   float attackDistance = getVar(cInputAttackDistance)->isUsed() ? getVar(cInputAttackDistance)->asFloat()->readVar() : cDefaultAttackRunDistance;
   uint attackNumber = getVar(cInputAttackNumber)->isUsed() ? (uint)getVar(cInputAttackNumber)->asInteger()->readVar() : cDefaultAttackNumber;

   // Create an order
   BSimOrder* pOrder = gSimOrderManager.createOrder();
   if (!pOrder)
      return;

   // Create the target
   BSimTarget target(location);

   // Init order
   pOrder->setTarget(target);
   pOrder->setPriority(BSimOrder::cPriorityTrigger);

   BVector baseLoc = cInvalidVector;
   uint numChildren = pSquad->getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         BUnitActionMoveAir* pMoveAction = reinterpret_cast<BUnitActionMoveAir*>(pUnit->getActionByType(BAction::cActionTypeUnitMoveAir));
         if (pMoveAction)
         {
            if (pMoveAction->isReturningToBase())
               return; // Can't go now - I'm going home
            else if (i==0) // Get the base location from the flight lead
               baseLoc = pMoveAction->getBaseLocation();
         }
      }
   }

   BSquadActionCarpetBomb::carpetBomb(pOrder, pSquad->getID(), location, baseLoc, attackDistance, attackNumber);
}

//==============================================================================
// BTriggerEffect::teAttachmentAddType()
//==============================================================================
void BTriggerEffect::teAttachmentAddType()
{
   switch(getVersion())
   {
      case 3:
         teAttachmentAddTypeV3();
         break;

      case 5:
         teAttachmentAddTypeV5();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// BTriggerEffect::teAttachmentAddTypeV3()
//==============================================================================
void BTriggerEffect::teAttachmentAddTypeV3()
{
   enum { cProtoObject = 2, cUnitList = 3, cSquadList = 4, cUnit = 5, cSquad = 6, };
   BEntityIDArray receivingUnits;
   long protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   BProtoObject *pProtoObject = gDatabase.getGenericProtoObject(protoObjectID);
   if (!pProtoObject)
      return;

   bool useUnit = getVar(cUnit)->isUsed();
   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      if (gWorld->getUnit(unitID))
         receivingUnits.uniqueAdd(unitID);
   }

   bool useUnitList = getVar(cUnitList)->isUsed();
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
      {
         receivingUnits.uniqueAdd(unitList[i]);
      }
   }

   bool useSquad = getVar(cSquad)->isUsed();
   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child=0; child<numChildren; child++)
         {
            receivingUnits.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   bool useSquadList = getVar(cSquadList)->isUsed();
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long i=0; i<numSquads; i++)
      {
         BSquad *pSquad = gWorld->getSquad(squadList[i]);
         if (!pSquad)
            continue;

         long numChildren = pSquad->getNumberChildren();
         for (long child=0; child<numChildren; child++)
         {
            receivingUnits.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   long numReceivingUnits = receivingUnits.getNumber();
   for (long i=0; i<numReceivingUnits; i++)
   {
      BUnit *pUnit = gWorld->getUnit(receivingUnits[i]);
      if (pUnit)
         pUnit->addAttachment(protoObjectID);
   }
}

//==============================================================================
// BTriggerEffect::teAttachmentAddTypeV5()
//==============================================================================
void BTriggerEffect::teAttachmentAddTypeV5()
{
   enum 
   { 
      cProtoObject = 2, 
      cUnitList    = 3, 
      cSquadList   = 4, 
      cUnit        = 5, 
      cSquad       = 6,
   };
     
   bool unitUsed      = getVar(cUnit)->isUsed();
   bool unitListUsed  = getVar(cUnitList)->isUsed();
   bool squadUsed     = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, and/or SquadList assigned!");
      return;
   }

   // Collect type to be added
   long          protoObjectID = cInvalidProtoID;
//-- FIXING PREFIX BUG ID 5287
   const BProtoObject* pProtoObject  = NULL;
//--
   protoObjectID               = getVar(cProtoObject)->asProtoObject()->readVar();
   pProtoObject                = gDatabase.getGenericProtoObject(protoObjectID);
   if (!pProtoObject)
   {
      BTRIGGER_ASSERTM(false, "Invalid object type!");
      return;
   }

   // Collect receiving units
   BEntityIDArray receivingUnits;
   if (unitListUsed)
   {
      receivingUnits = getVar(cUnitList)->asUnitList()->readVar();
   }
   
   if (unitUsed)
   {
      receivingUnits.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }
   
   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5285
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long j = 0; j < numChildren; j++)
            {
               receivingUnits.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5286
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long i = 0; i < numChildren; i++)
         {
            receivingUnits.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   long numReceivingUnits = receivingUnits.getNumber();
   for (long i = 0; i < numReceivingUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(receivingUnits[i]);
      if (pUnit)
      {
         pUnit->addAttachment(protoObjectID);
      }
   }
}

//XXXHalwes - 7/17/2007 - This might be invalid, need to revisit.
//==============================================================================
// BTriggerEffect::teAttachmentAddObject()
//==============================================================================
void BTriggerEffect::teAttachmentAddObject()
{
   enum 
   { 
      cUnit       = 1, 
      cObject     = 2,
      cObjectList = 3,
      cSquad      = 4,
   };

   bool objectUsed     = getVar(cObject)->isUsed();
   bool objectListUsed = getVar(cObjectList)->isUsed();
   if (!objectUsed && !objectListUsed)
   {
      BTRIGGER_ASSERTM(false, "No Object, and/or ObjectList assigned!");
      return;
   }

   // Collect objects to be added
   BEntityIDArray addObjects;
   if (objectListUsed)
   {
      addObjects = getVar(cObjectList)->asObjectList()->readVar();
   }

   if (objectUsed)
   {
      addObjects.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   // Collect receiving unit
   BUnit* pUnit = NULL;
   if (getVar(cUnit)->isUsed())
   {
      pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
   }
   else if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5288
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         pUnit = pSquad->getLeaderUnit();
      }
   }
   else
   {
      BTRIGGER_ASSERTM(false, "No receiving Unit or Squad assigned!");
      return;
   }

   // Attach objects
   if (pUnit)
   {
      long numAddObjects = addObjects.getNumber();
      for (long i = 0; i < numAddObjects; i++)
      {
         BObject* pObject = gWorld->getObject(addObjects[i]);
         if (pObject)
         {
            pUnit->addAttachment(pObject);
         }
      }
   }
}

//==============================================================================
// BTriggerEffect::teAttachmentAddUnit()
//==============================================================================
void BTriggerEffect::teAttachmentAddUnit()
{
   enum 
   { 
      cUnit        = 1,       
      cAddUnit     = 2,
      cAddUnitList = 3,
      cSquad       = 4,
   };

   bool addUnitUsed     = getVar(cAddUnit)->isUsed();
   bool addUnitListUsed = getVar(cAddUnitList)->isUsed();
   if (!addUnitUsed && !addUnitListUsed)
   {
      BTRIGGER_ASSERTM(false, "No Unit, and/or UnitList assigned!");
      return;
   }

   // Collect units to be added
   BEntityIDArray addUnitList;
   if (addUnitListUsed)
   {
      addUnitList = getVar(cAddUnitList)->asUnitList()->readVar();
   }

   if (addUnitUsed)
   {
      addUnitList.uniqueAdd(getVar(cAddUnit)->asUnit()->readVar());
   }

   // Collect receiving unit
   BUnit* pUnit = NULL;
   if (getVar(cUnit)->isUsed())
   {
      pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
   }
   else if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5289
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         pUnit = pSquad->getLeaderUnit();
      }
   }
   else
   {
      BTRIGGER_ASSERTM(false, "No receiving Unit or Squad assigned!");
      return;
   }

   // Attach units
   if (pUnit)
   {
      long numAddUnits = addUnitList.getNumber();
      for (long i = 0; i < numAddUnits; i++)
      {
         pUnit->attachObject(addUnitList[i]);
      }
   }
}

//==============================================================================
// BTriggerEffect::teAttachmentRemoveAll()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveAll()
{
   uint version = getVersion();
   if (version == 2)
   {
      teAttachmentRemoveAllV2();
   }
   else if (version == 3)
   {
      teAttachmentRemoveAllV3();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teAttachmentRemoveAllV2()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveAllV2()
{
   enum { cUnitList = 2, cSquadList = 3, };
   BEntityIDArray receivingUnits;

   bool useUnitList = getVar(cUnitList)->isUsed();
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
      {
         receivingUnits.uniqueAdd(unitList[i]);
      }
   }

   bool useSquadList = getVar(cSquadList)->isUsed();
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long i=0; i<numSquads; i++)
      {
         BSquad *pSquad = gWorld->getSquad(squadList[i]);
         if (!pSquad)
            continue;

         long numChildren = pSquad->getNumberChildren();
         for (long child=0; child<numChildren; child++)
         {
            receivingUnits.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   long numReceivingUnits = receivingUnits.getNumber();
   for (long i=0; i<numReceivingUnits; i++)
   {
      BUnit *pUnit = gWorld->getUnit(receivingUnits[i]);
      if (pUnit)
         pUnit->removeAllAttachments();
   }
}


//==============================================================================
// BTriggerEffect::teAttachmentRemoveAllV3()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveAllV3()
{
   enum { cUnitList = 2, cSquadList = 3, cUnit = 4, cSquad = 5, };

   bool useUnit = getVar(cUnit)->isUsed();
   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (pUnit)
         pUnit->removeAllAttachments();
   }

   bool useUnitList = getVar(cUnitList)->isUsed();
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
      {
         BUnit *pUnit = gWorld->getUnit(unitList[i]);
         if (pUnit)
            pUnit->removeAllAttachments();
      }
   }

   bool useSquad = getVar(cSquad)->isUsed();
   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long i=0; i<numChildren; i++)
         {
            BUnit *pUnit = gWorld->getUnit(pSquad->getChild(i));
            if (pUnit)
               pUnit->removeAllAttachments();
         }
      }
   }

   bool useSquadList = getVar(cSquadList)->isUsed();
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long i=0; i<numSquads; i++)
      {
         BSquad *pSquad = gWorld->getSquad(squadList[i]);
         if (!pSquad)
            continue;

         long numChildren = pSquad->getNumberChildren();
         for (long child=0; child<numChildren; child++)
         {
            BUnit *pUnit = gWorld->getUnit(pSquad->getChild(child));
            if (pUnit)
               pUnit->removeAllAttachments();
         }
      }
   }
}


//==============================================================================
// BTriggerEffect::teAttachmentRemoveType()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveType()
{
   uint version = getVersion();
   if (version == 2)
   {
      teAttachmentRemoveTypeV2();
   }
   else if (version == 3)
   {
      teAttachmentRemoveTypeV3();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teAttachmentRemoveTypeV2()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveTypeV2()
{
   enum { cProtoObject = 2, cUnitList = 3, cSquadList = 4, };

   BEntityIDArray receivingUnits;
   long protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   BProtoObject *pProtoObject = gDatabase.getGenericProtoObject(protoObjectID);
   if (!pProtoObject)
      return;

   bool useUnitList = getVar(cUnitList)->isUsed();
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
      {
         receivingUnits.uniqueAdd(unitList[i]);
      }
   }

   bool useSquadList = getVar(cSquadList)->isUsed();
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long i=0; i<numSquads; i++)
      {
         BSquad *pSquad = gWorld->getSquad(squadList[i]);
         if (!pSquad)
            continue;

         long numChildren = pSquad->getNumberChildren();
         for (long child=0; child<numChildren; child++)
         {
            receivingUnits.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   long numReceivingUnits = receivingUnits.getNumber();
   for (long i=0; i<numReceivingUnits; i++)
   {
      BUnit *pUnit = gWorld->getUnit(receivingUnits[i]);
      if (pUnit)
         pUnit->removeAttachmentsOfType(protoObjectID);
   }
}


//==============================================================================
// BTriggerEffect::teAttachmentRemoveTypeV3()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveTypeV3()
{
   enum { cProtoObject = 2, cUnitList = 3, cSquadList = 4, cUnit = 5, cSquad = 6, };
   long protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   BProtoObject *pProtoObject = gDatabase.getGenericProtoObject(protoObjectID);
   if (!pProtoObject)
      return;

   bool useUnit = getVar(cUnit)->isUsed();
   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (pUnit)
         pUnit->removeAttachmentsOfType(protoObjectID);
   }

   bool useUnitList = getVar(cUnitList)->isUsed();
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
      {
         BUnit *pUnit = gWorld->getUnit(unitList[i]);
         if (pUnit)
            pUnit->removeAttachmentsOfType(protoObjectID);
      }
   }

   bool useSquad = getVar(cSquad)->isUsed();
   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child=0; child<numChildren; child++)
         {
            BUnit *pUnit = gWorld->getUnit(pSquad->getChild(child));
            if (pUnit)
               pUnit->removeAttachmentsOfType(protoObjectID);
         }
      }
   }

   bool useSquadList = getVar(cSquadList)->isUsed();
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long i=0; i<numSquads; i++)
      {
         BSquad *pSquad = gWorld->getSquad(squadList[i]);
         if (!pSquad)
            continue;

         long numChildren = pSquad->getNumberChildren();
         for (long child=0; child<numChildren; child++)
         {
            BUnit *pUnit = gWorld->getUnit(pSquad->getChild(child));
            if (pUnit)
               pUnit->removeAttachmentsOfType(protoObjectID);
         }
      }
   }
}

//XXXHalwes - 7/17/2007 - This might be invalid need to revisit.
//==============================================================================
// BTriggerEffect::teAttachmentRemoveObject()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveObject()
{
   enum 
   { 
      cRemoveObject     = 1, 
      cRemoveObjectList = 2,
      cUnit             = 3,
      cUnitList         = 4,
      cSquad            = 5,
      cSquadList        = 6,
   };

   bool useObject     = getVar(cRemoveObject)->isUsed();
   bool useObjectList = getVar(cRemoveObjectList)->isUsed();

   if (!useObject && !useObjectList)
   {
      BTRIGGER_ASSERTM(false, "No Object or ObjectList assigned!");
      return;
   }

   // Collect objects
   BEntityIDArray objectList;
   if (useObjectList)
   {
      objectList = getVar(cRemoveObjectList)->asObjectList()->readVar();
   }

   if (useObject)
   {
      objectList.uniqueAdd(getVar(cRemoveObject)->asObject()->readVar());
   }

   // Collect units
   BEntityIDArray unitList;
   if (getVar(cUnitList)->isUsed())
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (getVar(cUnit)->isUsed())
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5291
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long j = 0; j < numChildren; j++)
            {
//-- FIXING PREFIX BUG ID 5299
               const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
//--
               unitList.uniqueAdd(pUnit->getID());
            }
         }
      }
   }

   if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5292
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long i = 0; i < numChildren; i++)
         {
//-- FIXING PREFIX BUG ID 5301
            const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
//--
            unitList.uniqueAdd(pUnit->getID());
         }
      }
   }

   // Remove objects from units
   long numUnits = unitList.getNumber();
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         long numObjects = objectList.getNumber();
         for (long j = 0; j < numObjects; j++)
         {
//-- FIXING PREFIX BUG ID 5303
            const BObject* pObject = gWorld->getObject(objectList[j]);
//--
            if (pObject)
            {
               pUnit->removeAttachment(pObject->getID());
            }
         }
      }
   }
}

//==============================================================================
// BTriggerEffect::teAttachmentRemoveUnit()
//==============================================================================
void BTriggerEffect::teAttachmentRemoveUnit()
{
   enum 
   { 
      cUnit           = 1,
      cUnitList       = 2, 
      cSquad          = 3, 
      cSquadList      = 4,       
      cRemoveUnit     = 5, 
      cRemoveUnitList = 6,
   };

   bool useRemoveUnit     = getVar(cRemoveUnit)->isUsed();
   bool useRemoveUnitList = getVar(cRemoveUnitList)->isUsed();

   if (!useRemoveUnit && !useRemoveUnitList)
   {
      BTRIGGER_ASSERTM(false, "No remove Unit and/or UnitList assigned!");
      return;
   }

   // Collect units to be removed
   BEntityIDArray removeUnitList;
   if (useRemoveUnitList)
   {
      removeUnitList = getVar(cRemoveUnitList)->asUnitList()->readVar();
   }

   if (useRemoveUnit)
   {
      removeUnitList.uniqueAdd(getVar(cRemoveUnit)->asUnit()->readVar());
   }

   // Collect units to be scrubbed
   BEntityIDArray unitList;
   if (getVar(cUnitList)->isUsed())
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (getVar(cUnit)->isUsed())
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5293
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5294
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long i = 0; i < numChildren; i++)
         {
            unitList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Remove units from units
   long numUnits = unitList.getNumber();
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         long numRemoveUnits = removeUnitList.getNumber();
         for (long j = 0; j < numRemoveUnits; j++)
         {
            pUnit->unattachObject(removeUnitList[j]);
         }
      }
   }
}

//==============================================================================
// Find the right version
//==============================================================================
void BTriggerEffect::teUsePower()
{
   switch (getVersion())
   {
      case 4:
         teUsePowerV4();
         break;

      case 5:
         teUsePowerV5();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// User the power
//==============================================================================
void BTriggerEffect::teUsePowerV4()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputPower = 2, 
      cInputSquad = 4, 
      cInputTarget = 5,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   long protoPowerID = getVar(cInputPower)->asPower()->readVar();
   BEntityID squadID = cInvalidObjectID;
   BEntityID targetID = cInvalidObjectID;
   if (getVar(cInputSquad)->isUsed())
      squadID = getVar(cInputSquad)->asSquad()->readVar();
   if (getVar(cInputTarget)->isUsed())
      targetID = getVar(cInputTarget)->asSquad()->readVar();
   pPlayer->usePower(protoPowerID, squadID, targetID);
}

//==============================================================================
// User the power
//==============================================================================
void BTriggerEffect::teUsePowerV5()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputPower = 2, 
      cInputSquad = 4, 
      cInputTarget = 5,
      cInputMultiplier = 6,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   long protoPowerID = getVar(cInputPower)->asPower()->readVar();
   BEntityID squadID = cInvalidObjectID;
   BEntityID targetID = cInvalidObjectID;
   if (getVar(cInputSquad)->isUsed())
      squadID = getVar(cInputSquad)->asSquad()->readVar();
   if (getVar(cInputTarget)->isUsed())
      targetID = getVar(cInputTarget)->asSquad()->readVar();
   float multiplier = getVar(cInputMultiplier)->isUsed() ? getVar(cInputMultiplier)->asFloat()->readVar() : 1.0f;
   pPlayer->usePower(protoPowerID, squadID, targetID, multiplier);
}

//==============================================================================
// Find the correct version
//==============================================================================
void BTriggerEffect::teGetClosestPowerSquad()
{
   switch (getVersion())
   {
      case 3:
         teGetClosestPowerSquadV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}


//==============================================================================
// Find the closest squad that uses this power
//==============================================================================
void BTriggerEffect::teGetClosestPowerSquadV3()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputPower = 2, 
      cInputLocation = 3, 
      cOutputSquad = 5, 
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long protoPowerID = getVar(cInputPower)->asPower()->readVar();
   BVector location = getVar(cInputLocation)->asVector()->readVar();
   BEntityID closestSquad = cInvalidObjectID;
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      BPowerEntry* pPowerEntry = pPlayer->findPowerEntry(protoPowerID);
      if (pPowerEntry)
      {
         float closestDistance = cMaximumFloat;
         uint numItems = pPowerEntry->mItems.getSize();
         for (uint i = 0; i < numItems; i++)
         {
            if ((pPowerEntry->mItems[i].mInfiniteUses || pPowerEntry->mItems[i].mUsesRemaining) && (!pPowerEntry->mItems[i].mRecharging))
            {
//-- FIXING PREFIX BUG ID 5296
               const BSquad* pSquad = gWorld->getSquad(pPowerEntry->mItems[i].mSquadID);
//--
               if (!pSquad)
                  continue;

               float dist = pSquad->calculateXZDistance(location);
               if (dist < closestDistance)
               {
                  closestDistance = dist;
                  closestSquad = pPowerEntry->mItems[i].mSquadID;
               }
            }
         }
      }
   }

   getVar(cOutputSquad)->asSquad()->writeVar(closestSquad);
}


//==============================================================================
// BTriggerEffect::teSetUIPowerRadius()
//==============================================================================
void BTriggerEffect::teSetUIPowerRadius()
{
   enum { cInputPlayer = 1, cInputPower = 2, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long protoPowerID = getVar(cInputPower)->asPower()->readVar();

   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;  
   BUser *pUser = pPlayer->getUser();
   if (!pUser)
      return;
//-- FIXING PREFIX BUG ID 5307
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
   if (pProtoPower)
   {
      pUser->setUIProtoPowerID(protoPowerID);
      pUser->setUIPowerRadius(pProtoPower->getUIRadius());
   }
   else
   {
      pUser->setUIProtoPowerID(-1);
      pUser->setUIPowerRadius(0.0f);
   }
}


//==============================================================================
// BTriggerEffect::teIteratorPlayerList()
//==============================================================================
void BTriggerEffect::teIteratorPlayerList()
{
   enum { cPlayerList = 1, cIterator = 2, };
   BTriggerVarID playerListVarID = getVar(cPlayerList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(playerListVarID);
};


//==============================================================================
// BTriggerEffect::teIteratorTeamList()
//==============================================================================
void BTriggerEffect::teIteratorTeamList()
{
   enum { cTeamList = 1, cIterator = 2, };
   BTriggerVarID teamListVarID = getVar(cTeamList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(teamListVarID);
};


//==============================================================================
// BTriggerEffect::teGetTeams()
//==============================================================================
void BTriggerEffect::teGetTeams()
{
   enum { cTeamList = 1, };
   BTeamIDArray teamList;
   long numTeams = gWorld->getNumberTeams();
   for (long i=0; i<numTeams; i++)
   {
      BTeam *pTeam = gWorld->getTeam(i);
      if (pTeam)
         teamList.uniqueAdd(pTeam->getID());
   }
   getVar(cTeamList)->asTeamList()->writeVar(teamList);
}


//==============================================================================
// BTriggerEffect::teGetTeamPlayers()
//==============================================================================
void BTriggerEffect::teGetTeamPlayers()
{
   enum { cTeam = 1, cPlayerList = 2, };
   BPlayerIDArray playerList;
   BTeam *pTeam = gWorld->getTeam(getVar(cTeam)->asTeam()->readVar());
   if (pTeam)
   {
      long numPlayers = pTeam->getNumberPlayers();
      for (long i=0; i<numPlayers; i++)
      {
         long playerID = pTeam->getPlayerID(i);
         playerList.uniqueAdd(playerID);
      }
   }
   getVar(cPlayerList)->asPlayerList()->writeVar(playerList);
}


//==============================================================================
// BTriggerEffect::tePlayerListAdd()
//==============================================================================
void BTriggerEffect::tePlayerListAdd()
{
   uint version = getVersion();
   if (version == 2)
   {
      tePlayerListAddV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::tePlayerListAddV2()
//==============================================================================
void BTriggerEffect::tePlayerListAddV2()
{
   enum { cAddPlayer = 2, cAddPlayerList = 3, cOutputList = 4, cClearExisting = 5, };
   BTriggerVarPlayerList *pOutputPlayerList = getVar(cOutputList)->asPlayerList();
   bool clearExisting = getVar(cClearExisting)->asBool()->readVar();
   if (clearExisting)
      pOutputPlayerList->clear();

   bool useAddPlayer = getVar(cAddPlayer)->isUsed();
   if (useAddPlayer)
   {
      long addPlayerID = getVar(cAddPlayer)->asPlayer()->readVar();
      pOutputPlayerList->addPlayer(addPlayerID);
   }

   bool useAddPlayerList = getVar(cAddPlayerList)->isUsed();
   if (useAddPlayerList)
   {
      const BPlayerIDArray &addList = getVar(cAddPlayerList)->asPlayerList()->readVar();
      long numAdds = addList.getNumber();
      for (long i=0; i<numAdds; i++)
      {
         long addPlayerID = addList[i];
         pOutputPlayerList->addPlayer(addPlayerID);
      }
   }
}


//==============================================================================
// BTriggerEffect::tePlayerListRemove()
//==============================================================================
void BTriggerEffect::tePlayerListRemove()
{
   uint version = getVersion();
   if (version == 2)
   {
      tePlayerListRemoveV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::tePlayerListRemoveV2()
//==============================================================================
void BTriggerEffect::tePlayerListRemoveV2()
{
   enum { cSrcList = 1, cRemovePlayer = 2, cRemovePlayerList = 3, cRemoveAll = 4, };
   BTriggerVarPlayerList *pPlayerList = getVar(cSrcList)->asPlayerList();
   bool removeAll = getVar(cRemoveAll)->asBool()->readVar();
   if (removeAll)
   {
      pPlayerList->clear();
      return;
   }

   bool useRemovePlayer = getVar(cRemovePlayer)->isUsed();
   if (useRemovePlayer)
   {
      long removePlayerID = getVar(cRemovePlayer)->asPlayer()->readVar();
      pPlayerList->removePlayer(removePlayerID);
   }

   bool useRemovePlayerList = getVar(cRemovePlayerList)->isUsed();
   if (useRemovePlayerList)
   {
      const BPlayerIDArray &removeList = getVar(cRemovePlayerList)->asPlayerList()->readVar();
      long numRemoves = removeList.getNumber();
      for (long i=0; i<numRemoves; i++)
      {
         long removePlayerID = removeList[i];
         pPlayerList->removePlayer(removePlayerID);
      }
   }
}


//==============================================================================
// BTriggerEffect::teTeamListAdd()
//==============================================================================
void BTriggerEffect::teTeamListAdd()
{
   uint version = getVersion();
   if (version == 2)
   {
      teTeamListAddV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teTeamListAdd()
//==============================================================================
void BTriggerEffect::teTeamListAddV2()
{
   enum { cAddTeam = 2, cAddTeamList = 3, cOutputList = 4, cClearExisting = 5, };
   BTriggerVarTeamList *pOutputTeamList = getVar(cOutputList)->asTeamList();
   bool clearExisting = getVar(cClearExisting)->asBool()->readVar();
   if (clearExisting)
      pOutputTeamList->clear();

   bool useAddTeam = getVar(cAddTeam)->isUsed();
   if (useAddTeam)
   {
      long addTeamID = getVar(cAddTeam)->asTeam()->readVar();
      pOutputTeamList->addTeam(addTeamID);
   }

   bool useAddTeamList = getVar(cAddTeamList)->isUsed();
   if (useAddTeamList)
   {
      const BTeamIDArray &addList = getVar(cAddTeamList)->asTeamList()->readVar();
      long numAdds = addList.getNumber();
      for (long i=0; i<numAdds; i++)
      {
         long addTeamID = addList[i];
         pOutputTeamList->addTeam(addTeamID);
      }
   }
}


//==============================================================================
// BTriggerEffect::teTeamListRemove()
//==============================================================================
void BTriggerEffect::teTeamListRemove()
{
   uint version = getVersion();
   if (version == 2)
   {
      teTeamListRemoveV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teTeamListRemoveV2()
//==============================================================================
void BTriggerEffect::teTeamListRemoveV2()
{
   enum { cSrcList = 1, cRemoveTeam = 2, cRemoveTeamList = 3, cRemoveAll = 4, };
   BTriggerVarTeamList *pTeamList = getVar(cSrcList)->asTeamList();
   bool removeAll = getVar(cRemoveAll)->asBool()->readVar();
   if (removeAll)
   {
      pTeamList->clear();
      return;
   }

   bool useRemoveTeam = getVar(cRemoveTeam)->isUsed();
   if (useRemoveTeam)
   {
      long removeTeamID = getVar(cRemoveTeam)->asTeam()->readVar();
      pTeamList->removeTeam(removeTeamID);
   }

   bool useRemoveTeamList = getVar(cRemoveTeamList)->isUsed();
   if (useRemoveTeamList)
   {
      const BTeamIDArray &removeList = getVar(cRemoveTeamList)->asTeamList()->readVar();
      long numRemoves = removeList.getNumber();
      for (long i=0; i<numRemoves; i++)
      {
         long removeTeamID = removeList[i];
         pTeamList->removeTeam(removeTeamID);
      }
   }
}

//==============================================================================
// Select correct version
//==============================================================================
void BTriggerEffect::teSetPlayerState()
{
   switch (getVersion())
   {
      case 2:
         teSetPlayerStateV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Set the player's state
//==============================================================================
void BTriggerEffect::teSetPlayerStateV2()
{
   enum 
   { 
      cPlayer = 1, 
      cPlayerState = 3, 
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   BPlayerState playerState = getVar(cPlayerState)->asPlayerState()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   pPlayer->setPlayerState(playerState);
}

//==============================================================================
// BTriggerEffect::teGetDistanceUnitUnit()
//==============================================================================
void BTriggerEffect::teGetDistanceUnitUnit()
{
   uint version = getVersion();
   if (version == 2)
   {
      teGetDistanceUnitUnitV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// Get the unit to unit distance
//==============================================================================
void BTriggerEffect::teGetDistanceUnitUnitV2()
{
   enum 
   { 
      cUnitA = 1, 
      cUnitB = 2, 
      cDistance = 3, 
   };

   BEntityID unitAID = getVar(cUnitA)->asUnit()->readVar();
   BEntityID unitBID = getVar(cUnitB)->asUnit()->readVar();
//-- FIXING PREFIX BUG ID 5310
   const BUnit* pUnitA = gWorld->getUnit(unitAID);
//--
//-- FIXING PREFIX BUG ID 5311
   const BUnit* pUnitB = gWorld->getUnit(unitBID);
//--
   BTRIGGER_ASSERT(pUnitA && pUnitB);   // Note, we may not want this assert here, but it's to see if designers are doing stuff to dead entities.
   if (!pUnitA || !pUnitB)
      return;

   float dist = pUnitA->BEntity::calculateXZDistance(pUnitB);
   getVar(cDistance)->asFloat()->writeVar(dist);
}


//==============================================================================
// BTriggerEffect::teGetDistanceUnitLocation()
//==============================================================================
void BTriggerEffect::teGetDistanceUnitLocation()
{
   uint version = getVersion();
   if (version == 2)
   {
      teGetDistanceUnitLocationV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// Get the unit to location distance
//==============================================================================
void BTriggerEffect::teGetDistanceUnitLocationV2()
{
   enum 
   { 
      cUnitA = 1, 
      cLocation = 2, 
      cDistance = 3, 
   };

   BEntityID unitAID = getVar(cUnitA)->asUnit()->readVar();
//-- FIXING PREFIX BUG ID 5312
   const BUnit* pUnitA = gWorld->getUnit(unitAID);
//--
   BTRIGGER_ASSERT(pUnitA);   // Note, we may not want this assert here, but it's to see if designers are doing stuff to dead entities.
   if (!pUnitA)
      return;

   BVector location = getVar(cLocation)->asVector()->readVar();
   float dist = pUnitA->BEntity::calculateXZDistance(location);
   getVar(cDistance)->asFloat()->writeVar(dist);
}


//==============================================================================
// BTriggerEffect::teGetUnits()
//==============================================================================
void BTriggerEffect::teGetUnits()
{
   uint version = getVersion();
   if (version == 3)
   {
      teGetUnitsV3();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teGetUnitsV3()
//==============================================================================
void BTriggerEffect::teGetUnitsV3()
{
   enum { cInputPlayer = 1, cInputObjectType = 2, cInputLocation = 3, cInputDistance = 4, cOutputUnitList = 5, cOutputCount = 6, cInputFilterList = 7, };
   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool useObjectType = getVar(cInputObjectType)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputUnitList = getVar(cOutputUnitList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   BEntityIDArray workingList(0,50);
   BEntityIDArray results(0,50);

   // Populate our list of units to filter.
   // Option #1, get units in an area.
   if (useLocation && useDistance)
   {
      BVector loc = getVar(cInputLocation)->asVector()->readVar();
      float dist = getVar(cInputDistance)->asFloat()->readVar();
      BUnitQuery query(loc, dist, true);
      if (usePlayer)
      {
         long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
         query.addPlayerFilter(playerID);
      }
      if (useObjectType)
      {
         long objectType = getVar(cInputObjectType)->asObjectType()->readVar();
         query.addObjectTypeFilter(objectType);
      }
      query.setFlagIgnoreDead(true);
      gWorld->getUnitsInArea(&query, &workingList);
   }
   // Option #3, get all units in the game.
   else
   {
      DWORD playerIDBitArray = 0;
      if (usePlayer)
      {
         BPlayerID pid = getVar(cInputPlayer)->asPlayer()->readVar();
         playerIDBitArray |= (1 << pid);
      }
      else
      {
         playerIDBitArray = 0xFFFFFFFF;
      }

      // Units of all players.
      BObjectTypeID objectTypeID = cInvalidObjectTypeID;
      if (useObjectType)
         objectTypeID = getVar(cInputObjectType)->asObjectType()->readVar();
      long numPlayers = gWorld->getNumberPlayers();
      BEntityIDArray playerUnitIDs;
      for (long pid=0; pid<numPlayers; pid++)
      {
         if (!(playerIDBitArray & (1 << pid)))
            continue;
         const BPlayer* pPlayer = gWorld->getPlayer(pid);
         if (!pPlayer)
            continue;
         if (useObjectType)
         {
            pPlayer->getUnitsOfType(objectTypeID, playerUnitIDs);
            workingList.append(playerUnitIDs);
         }
         else
         {
            pPlayer->getUnits(playerUnitIDs);
            workingList.append(playerUnitIDs);
         }
      }

      for (uint i=0; i<workingList.getSize(); /*i++*/)
      {
         const BUnit* pUnit = gWorld->getUnit(workingList[i]);
         if (!pUnit || !pUnit->isAlive())
         {
            workingList.removeIndex(i, false);
            continue;
         }

         ++i;
      }
   }

   if (useFilterList)
   {
      const BEntityIDArray& filterList = getVar(cInputFilterList)->asUnitList()->readVar();
      long numInWorkingList = workingList.getNumber();
      long numInFilterList = filterList.getNumber();
      for (long i=0; i<numInWorkingList; i++)
      {
         BEntityID entityID = workingList[i];
         if (filterList.find(entityID) != cInvalidIndex)
         {
            results.add(entityID);
            continue;
         }
         for (long f=0; f<numInFilterList; f++)
         {
            BSquad *pFilterSquad = gWorld->getSquad(filterList[f]);
            if (pFilterSquad && pFilterSquad->findChildIndexFromID(entityID) != cInvalidIndex)
            {
               results.add(entityID);
               break;
            }
         }
      }
   }
   else
      results = workingList;

   long numUnits = results.getNumber();
   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numUnits);
   }
   if (useOutputUnitList)
   {
      getVar(cOutputUnitList)->asUnitList()->writeVar(results);
   }
}


//==============================================================================
// BTriggerEffect::teGetSquads()
//==============================================================================
void BTriggerEffect::teGetSquads()
{
   uint version = getVersion();
   if (version == 4)
   {
      teGetSquadsV4();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teGetSquadsV4()
//==============================================================================
void BTriggerEffect::teGetSquadsV4()
{
   enum { cInputPlayer = 1, cInputProtoSquad = 2, cInputLocation = 3, cInputDistance = 4, cOutputSquadList = 5, cOutputCount = 6, cInputFilterList = 7, cObjectType = 8, };
   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool useProtoSquad = getVar(cInputProtoSquad)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputSquadList = getVar(cOutputSquadList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   bool useObjectType = getVar(cObjectType)->isUsed();
   long objectType = (useObjectType) ? getVar(cObjectType)->asObjectType()->readVar() : -1;
   BEntityIDArray results(0,50);

   // Populate our list of squads to filter.
   // Option #1, get squads in an area.
   if (useLocation && useDistance)
   {
      BEntityIDArray workingList(0,50);
      BVector loc = getVar(cInputLocation)->asVector()->readVar();
      float dist = getVar(cInputDistance)->asFloat()->readVar();
      BUnitQuery query(loc, dist, true);
      if (usePlayer)
      {
         long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
         query.addPlayerFilter(playerID);
      }
      if (useObjectType)
      {
         query.addObjectTypeFilter(objectType);
      }
      query.setFlagIgnoreDead(true);
      gWorld->getSquadsInArea(&query, &workingList);

      long numInWorkingList = workingList.getNumber();
      for (long i=0; i<numInWorkingList; i++)
      {
         BEntityID squadID = workingList[i];
         BSquad *pSquad = gWorld->getSquad(squadID);
         if (useProtoSquad)
         {
            long protoSquadID = getVar(cInputProtoSquad)->asProtoSquad()->readVar();
            if (!pSquad || (pSquad->getProtoSquadID() != protoSquadID))
               continue;
         }
         if (useFilterList)
         {
            const BEntityIDArray& filterList = getVar(cInputFilterList)->asSquadList()->readVar();
            if (filterList.find(squadID) == cInvalidIndex)
               continue;
         }
         if (useObjectType)
         {
            bool allChildrenQualify = true;
            long numSquadChildren = pSquad->getNumberChildren();
            for (long child=0; child<numSquadChildren; child++)
            {
               BUnit *pChildUnit = gWorld->getUnit(pSquad->getChild(child));
               if (!pChildUnit)
               {
                  allChildrenQualify = false;
                  break;
               }
               if (!pChildUnit->isType(objectType))
               {
                  allChildrenQualify = false;
                  break;
               }
            }
            if (!allChildrenQualify)
               continue;
         }

         results.add(squadID);
      }
   }
   // Option #2, get all squads in the game.
   else
   {
      BEntityIDArray workingSquadIDs;
      DWORD playerIDBitArray = 0;
      if (usePlayer)
      {
         BPlayerID pid = getVar(cInputPlayer)->asPlayer()->readVar();
         playerIDBitArray |= (1 << pid);
      }
      else
      {
         playerIDBitArray = 0xFFFFFFFF;
      }

      // Squads of all players.
      BProtoSquadID protoSquadID = cInvalidProtoSquadID;
      if (useProtoSquad)
         protoSquadID = getVar(cInputProtoSquad)->asProtoSquad()->readVar();
      long numPlayers = gWorld->getNumberPlayers();
      BEntityIDArray playerSquadIDs;
      for (long pid=0; pid<numPlayers; pid++)
      {
         if (!(playerIDBitArray & (1 << pid)))
            continue;
         const BPlayer* pPlayer = gWorld->getPlayer(pid);
         if (!pPlayer)
            continue;
         if (useProtoSquad)
         {
            pPlayer->getSquadsOfType(protoSquadID, playerSquadIDs);
            workingSquadIDs.append(playerSquadIDs);
         }
         else
         {
            pPlayer->getSquads(playerSquadIDs);
            workingSquadIDs.append(playerSquadIDs);
         }
      }

      for (uint i=0; i<workingSquadIDs.getSize(); /*i++*/)
      {
         const BSquad* pSquad = gWorld->getSquad(workingSquadIDs[i]);
         if (!pSquad || !pSquad->isAlive())
         {
            workingSquadIDs.removeIndex(i, false);
            continue;
         }

         if (useFilterList)
         {
            const BEntityIDArray& filterList = getVar(cInputFilterList)->asSquadList()->readVar();
            if (!filterList.contains(workingSquadIDs[i]))
            {
               workingSquadIDs.removeIndex(i, false);
               continue;
            }
         }

         if (useObjectType)
         {
            bool allChildrenQualify = true;
            uint numSquadChildren = pSquad->getNumberChildren();
            for (uint child = 0; child < numSquadChildren; child++)
            {
               BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(child));
               if (!pChildUnit)
               {
                  allChildrenQualify = false;
                  break;
               }

               if (!pChildUnit->isType(objectType))
               {
                  allChildrenQualify = false;
                  break;
               }
            }

            if (!allChildrenQualify)
            {
               workingSquadIDs.removeIndex(i, false);
               continue;
            }
         }

         ++i;
      }

      results = workingSquadIDs;
   }

   long numSquads = results.getNumber();
   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numSquads);
   }
   if (useOutputSquadList)
   {
      getVar(cOutputSquadList)->asSquadList()->writeVar(results);
   }
}


//=============================================================================
// BTriggerEffect::teWork
//=============================================================================
void BTriggerEffect::teWork()
{
   switch (getVersion())
   {
      case 3:
         teWorkV3();
         break;

      case 4:
         teWorkV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//=============================================================================
// Tell squad/unit to do work on something
//=============================================================================
void BTriggerEffect::teWorkV3()
{
   enum 
   { 
      cSquad = 1, 
      cSquadList = 2, 
      cTargetUnit = 3, 
      cTargetLocation = 4, 
      cTargetSquad = 5, 
      cAttackMove = 6,
      cQueueOrder = 7,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useTargetUnit = getVar(cTargetUnit)->isUsed();
   bool useTargetLocation = getVar(cTargetLocation)->isUsed();
   bool useTargetSquad = getVar(cTargetSquad)->isUsed();
   bool attackMove = getVar(cAttackMove)->isUsed() ? getVar(cAttackMove)->asBool()->readVar() : false;
   bool queueOrder = getVar(cQueueOrder)->isUsed() ? getVar(cQueueOrder)->asBool()->readVar() : false;

   BEntityID targetUnitID = useTargetUnit ? getVar(cTargetUnit)->asUnit()->readVar() : cInvalidObjectID;
   BEntityID targetSquadID = useTargetSquad ? getVar(cTargetSquad)->asSquad()->readVar() : cInvalidObjectID;
   BVector targetLocation = useTargetLocation ? getVar(cTargetLocation)->asVector()->readVar() : cInvalidVector;
   if (useTargetUnit && !gWorld->getUnit(targetUnitID))
      useTargetUnit = false;
   if (useTargetSquad && !gWorld->getSquad(targetSquadID))
      useTargetSquad = false;
   if (!useSquad && !useSquadList)
      return;
   if (!useTargetUnit && !useTargetSquad && !useTargetLocation)
      return;

   // Get our list of squads.
   BEntityIDArray workingSquads;
   if (useSquadList) // squadlist first then squad.
   {
      workingSquads = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      if (gWorld->getSquad(squadID))
         workingSquads.uniqueAdd(squadID);
   }

   // Bail now if we have no squads to do the work.
   uint numWorkingSquads = workingSquads.getSize();
   if (numWorkingSquads == 0)
      return;

#ifndef BUILD_FINAL
   gTriggerManager.incrementWorkPerfCount(numWorkingSquads);
#endif

   // Group squads per player
   // MPB [8/27/2008] - Added the "filterOutSpartanHijackedEntities" function to remove hijacked vehicles from the list
   // of entities to command.  This is a hack to work around the fact that there is no player ID validation on the set
   // of entities that the trigger script wants to command, and because hijacking will change the playerID on a unit/squad.
   // This fixes bug PHX-9580.
   BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(workingSquads, filterOutSpartanHijackedEntities);
   uint numPlayerEntityIDs = playerEntityIDs.getSize();
   for (uint i = 0; i < numPlayerEntityIDs; i++)
   {
      uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
      if (numEntities > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            // Platoon the squads
            pArmy->addSquads(playerEntityIDs[i].mEntityIDs, !gConfig.isDefined(cConfigClassicPlatoonGrouping));

            // Setup the command
            BWorkCommand tempCommand;
            if (useTargetUnit)
            {
               tempCommand.setUnitID(targetUnitID);
            }
            else if (useTargetSquad)
            {
//-- FIXING PREFIX BUG ID 5318
               const BSquad* pTempSquad = gWorld->getSquad(targetSquadID);
//--
               if (pTempSquad)
               {
                  tempCommand.setUnitID(pTempSquad->getLeader());
               }
               else
                  return;
            }
            else if (useTargetLocation)
               tempCommand.setWaypoints(&targetLocation, 1);

            // Set the command to be from a trigger to the army
            tempCommand.setRecipientType(BCommand::cArmy);
            tempCommand.setSenderType(BCommand::cTrigger);

            // Set the attack move
            tempCommand.setFlag(BWorkCommand::cFlagAttackMove, attackMove);

            // Set the queue order
            tempCommand.setFlag(BCommand::cFlagAlternate, queueOrder);

            // Give the command to the army.
            pArmy->queueOrder(&tempCommand);
         }               
      }
   }
}

//=============================================================================
// Tell squad/unit to do work on something
//=============================================================================
void BTriggerEffect::teWorkV4()
{
   enum 
   { 
      cSquad = 1, 
      cSquadList = 2, 
      cTargetUnit = 3, 
      cTargetLocation = 4, 
      cTargetSquad = 5, 
      cAttackMove = 6,
      cQueueOrder = 7,
      cDoAbility = 8,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useTargetUnit = getVar(cTargetUnit)->isUsed();
   bool useTargetLocation = getVar(cTargetLocation)->isUsed();
   bool useTargetSquad = getVar(cTargetSquad)->isUsed();
   bool attackMove = getVar(cAttackMove)->isUsed() ? getVar(cAttackMove)->asBool()->readVar() : false;
   bool queueOrder = getVar(cQueueOrder)->isUsed() ? getVar(cQueueOrder)->asBool()->readVar() : false;
   bool doAbility = getVar(cDoAbility)->isUsed() ? getVar(cDoAbility)->asBool()->readVar() : false;

   BEntityID targetUnitID = useTargetUnit ? getVar(cTargetUnit)->asUnit()->readVar() : cInvalidObjectID;
   BEntityID targetSquadID = useTargetSquad ? getVar(cTargetSquad)->asSquad()->readVar() : cInvalidObjectID;
   BVector targetLocation = useTargetLocation ? getVar(cTargetLocation)->asVector()->readVar() : cInvalidVector;
   if (useTargetUnit && !gWorld->getUnit(targetUnitID))
      useTargetUnit = false;
   if (useTargetSquad && !gWorld->getSquad(targetSquadID))
      useTargetSquad = false;
   if (!useSquad && !useSquadList)
      return;
   if (!useTargetUnit && !useTargetSquad && !useTargetLocation)
      return;

   // Get our list of squads.
   BEntityIDArray workingSquads;
   if (useSquadList) // squadlist first then squad.
   {
      workingSquads = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      if (gWorld->getSquad(squadID))
         workingSquads.uniqueAdd(squadID);
   }

   // Bail now if we have no squads to do the work.
   uint numWorkingSquads = workingSquads.getSize();
   if (numWorkingSquads == 0)
      return;

#ifndef BUILD_FINAL
   gTriggerManager.incrementWorkPerfCount(numWorkingSquads);
#endif

   // Group squads per player
   // MPB [8/27/2008] - Added the "filterOutSpartanHijackedEntities" function to remove hijacked vehicles from the list
   // of entities to command.  This is a hack to work around the fact that there is no player ID validation on the set
   // of entities that the trigger script wants to command, and because hijacking will change the playerID on a unit/squad.
   // This fixes bug PHX-9580.
   BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(workingSquads, filterOutSpartanHijackedEntities);
   uint numPlayerEntityIDs = playerEntityIDs.getSize();
   for (uint i = 0; i < numPlayerEntityIDs; i++)
   {
      uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
      if (numEntities > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            // Platoon the squads
            pArmy->addSquads(playerEntityIDs[i].mEntityIDs, !gConfig.isDefined(cConfigClassicPlatoonGrouping));

            // Setup the command
            BWorkCommand tempCommand;
            if (useTargetUnit)
            {
               tempCommand.setUnitID(targetUnitID);
            }
            else if (useTargetSquad)
            {
//-- FIXING PREFIX BUG ID 5319
               const BSquad* pTempSquad = gWorld->getSquad(targetSquadID);
//--
               if (pTempSquad)
               {
                  tempCommand.setUnitID(pTempSquad->getLeader());
               }
               else
                  return;
            }
            else if (useTargetLocation)
               tempCommand.setWaypoints(&targetLocation, 1);

            // Set the command to be from a trigger to the army
            tempCommand.setRecipientType(BCommand::cArmy);
            tempCommand.setSenderType(doAbility ? BCommand::cPlayer : BCommand::cTrigger);

            // Set the attack move
            tempCommand.setFlag(BWorkCommand::cFlagAttackMove, attackMove);

            // Set the queue order
            tempCommand.setFlag(BCommand::cFlagAlternate, queueOrder);

            // Set the ability info
            if (doAbility)
            {
               tempCommand.setAbilityID(gDatabase.getAIDCommand());
            }

            // Give the command to the army.
            pArmy->queueOrder(&tempCommand);
         }               
      }
   }
}

//XXXHalwes - 7/17/2007 - Is this still valid?  CreateIconObject in combination with a sound trigger can be used instead with more customization.
//=============================================================================
// Call correct version
//=============================================================================
void BTriggerEffect::teFlareMinimapSpoof()
{
   switch (getVersion())
   {
      case 2:
         teFlareMinimapSpoofV2();
         break;

      case 3:
         teFlareMinimapSpoofV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//XXXHalwes - 7/17/2007 - Is this still valid?  CreateIconObject in combination with a sound trigger can be used instead with more customization.
//=============================================================================
// Flare the assigned players
//=============================================================================
void BTriggerEffect::teFlareMinimapSpoofV2()
{
   enum { cFlareLocation = 1, cSrcPlayer = 2, cDestPlayer = 3, cDestPlayerList = 4, };
   bool useDestPlayer = getVar(cDestPlayer)->isUsed();
   bool useDestPlayerList = getVar(cDestPlayerList)->isUsed();
   long srcPlayerID = getVar(cSrcPlayer)->asPlayer()->readVar();
   BVector flareLocation = getVar(cFlareLocation)->asVector()->readVar();
   BPlayerIDArray playersToFlare;

   if (useDestPlayer)
   {
      playersToFlare.uniqueAdd(getVar(cDestPlayer)->asPlayer()->readVar());
   }

   if (useDestPlayerList)
   {
      const BPlayerIDArray& destPlayerList = getVar(cDestPlayerList)->asPlayerList()->readVar();
      uint destPlayerListSize = (uint)destPlayerList.getNumber();
      for (uint i = 0; i < destPlayerListSize; i++)
         playersToFlare.uniqueAdd(destPlayerList[i]);
   }

   uint numPlayers = playersToFlare.getSize();
   for (uint i=0; i<numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playersToFlare[i]);
      if (pPlayer)
         pPlayer->getAlertManager()->createFlareAlert(flareLocation, srcPlayerID);
   }
}

//XXXHalwes - 7/17/2007 - Is this still valid?  CreateIconObject in combination with a sound trigger can be used instead with more customization.
//=============================================================================
// Flare the assigned players with the specified flare type
//=============================================================================
void BTriggerEffect::teFlareMinimapSpoofV3()
{
   enum 
   { 
      cFlareLocation = 1, 
      cSrcPlayer = 2, 
      cDestPlayer = 3, 
      cDestPlayerList = 4, 
      cFlareType = 7,
   };

   bool useDestPlayer = getVar(cDestPlayer)->isUsed();
   bool useDestPlayerList = getVar(cDestPlayerList)->isUsed();
   long srcPlayerID = getVar(cSrcPlayer)->asPlayer()->readVar();
   BVector flareLocation = getVar(cFlareLocation)->asVector()->readVar();
   BPlayerIDArray playersToFlare;

   if (useDestPlayer)
   {
      playersToFlare.uniqueAdd(getVar(cDestPlayer)->asPlayer()->readVar());
   }

   if (useDestPlayerList)
   {
      const BPlayerIDArray& destPlayerList = getVar(cDestPlayerList)->asPlayerList()->readVar();
      uint destPlayerListSize = (uint)destPlayerList.getNumber();
      for (uint i = 0; i < destPlayerListSize; i++)
         playersToFlare.uniqueAdd(destPlayerList[i]);
   }

   int flareType = 0;
   if (getVar(cFlareType)->isUsed())
   {
      flareType = getVar(cFlareType)->asFlareType()->readVar();
   }

   uint numPlayers = playersToFlare.getSize();
   for (uint i=0; i<numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playersToFlare[i]);
      if (pPlayer)
         pPlayer->getAlertManager()->createFlareAlert(flareLocation, srcPlayerID);
   }
}

//=============================================================================
// BTriggerEffect::teObjectiveComplete
//=============================================================================
void BTriggerEffect::teObjectiveComplete()
{
   switch(getVersion())
   {
      case 2:
         teObjectiveCompleteV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "Obsolete objective complete trigger effect version!");
         break;
   }
}

//=============================================================================
// BTriggerEffect::teObjectiveCompleteV2
//=============================================================================
void BTriggerEffect::teObjectiveCompleteV2()
{
   enum 
   { 
      cObjective = 1, 
      cEnabled   = 2,
   };

   long objectiveID = getVar(cObjective)->asObjective()->readVar();
   bool enabled     = getVar(cEnabled)->asBool()->readVar();

   gWorld->getObjectiveManager()->setObjectiveCompleted(objectiveID, enabled);
}

//=============================================================================
// BTriggerEffect::teObjectiveDisplay
//=============================================================================
void BTriggerEffect::teObjectiveDisplay()
{
	switch (getVersion())
	{
	case 1:
		teObjectiveDisplayV1();
		break;

	case 2:
		teObjectiveDisplayV2();
		break;
	}
}

//=============================================================================
// BTriggerEffect::teObjectiveDisplayV1
//=============================================================================
void BTriggerEffect::teObjectiveDisplayV1()
{
	enum 
	{ 
		cObjective = 1,
		cEnabled   = 2,
	};

	long objectiveID = getVar(cObjective)->asObjective()->readVar();
	bool enabled     = getVar(cEnabled)->asBool()->readVar();

	gWorld->getObjectiveManager()->setObjectiveDisplayed(objectiveID, enabled);
}

//=============================================================================
// BTriggerEffect::teObjectiveDisplayV2
//=============================================================================
void BTriggerEffect::teObjectiveDisplayV2()
{
	enum 
	{ 
		cObjective = 1,
		cEnabled   = 2,
		cDuration  = 3
	};

	long objectiveID = getVar(cObjective)->asObjective()->readVar();
	bool enabled     = getVar(cEnabled)->asBool()->readVar();

	if (getVar(cDuration)->isUsed())
		gWorld->getObjectiveManager()->setObjectiveDisplayed(objectiveID, enabled, (float)(getVar(cDuration)->asTime()->readVar()*0.001f));
	else
		gWorld->getObjectiveManager()->setObjectiveDisplayed(objectiveID, enabled);
}

//==============================================================================
// User correct version
//==============================================================================
void BTriggerEffect::teGetLocation()
{
   switch (getVersion())
   {
      case 1:
         teGetLocationV1();
         break;

      case 2:
         teGetLocationV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Get the location of the unit or squad
//==============================================================================
void BTriggerEffect::teGetLocationV1()
{
   enum { cInputUnit = 1, cInputSquad = 2, cOutputLocation = 3, };
   bool useUnit = getVar(cInputUnit)->isUsed();
   bool useSquad = getVar(cInputSquad)->isUsed();
   BTRIGGER_ASSERT(!(useUnit && useSquad));

   if (useUnit)
   {
      BEntityID unitID = getVar(cInputUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (pUnit)
      {
         getVar(cOutputLocation)->asVector()->writeVar(pUnit->getPosition());
         return;
      }
   }

   if (useSquad)
   {
      BEntityID squadID = getVar(cInputSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         getVar(cOutputLocation)->asVector()->writeVar(pSquad->getPosition());
         return;
      }
   }
}

//==============================================================================
// Get the location of the unit, squad, or object
//==============================================================================
void BTriggerEffect::teGetLocationV2()
{
   enum 
   { 
      cInputUnit = 1, 
      cInputSquad = 2,       
      cOutputLocation = 3, 
      cInputObject = 4,
   };

   bool useUnit = getVar(cInputUnit)->isUsed();
   bool useSquad = getVar(cInputSquad)->isUsed();
   bool useObject = getVar(cInputObject)->isUsed();
   if(!useUnit && !useSquad && !useObject)
   {
      BTRIGGER_ASSERTM(false, "No unit, squad, or object designated!");
      return;
   }

   if (useUnit)
   {
      BEntityID unitID = getVar(cInputUnit)->asUnit()->readVar();
//-- FIXING PREFIX BUG ID 5313
      const BUnit* pUnit = gWorld->getUnit(unitID);
//--
      if (pUnit)
      {
         getVar(cOutputLocation)->asVector()->writeVar(pUnit->getPosition());
      }
   }
   else if (useSquad)
   {
      BEntityID squadID = getVar(cInputSquad)->asSquad()->readVar();
//-- FIXING PREFIX BUG ID 5297
      const BSquad* pSquad = gWorld->getSquad(squadID);
//--
      if (pSquad)
      {
         getVar(cOutputLocation)->asVector()->writeVar(pSquad->getPosition());
      }
   }
   else if (useObject)
   {
      BEntityID objectID = getVar(cInputObject)->asObject()->readVar();
//-- FIXING PREFIX BUG ID 5325
      const BObject* pObject = gWorld->getObject(objectID);
//--
      if (pObject)
      {
         getVar(cOutputLocation)->asVector()->writeVar(pObject->getPosition());
      }
   }
}

//=============================================================================
// BTriggerEffect::teMathPercent
//=============================================================================
void BTriggerEffect::teMathPercent()
{
   enum { cPercent1 = 1, cOp = 2, cPercent2 = 3, cOutput = 4, };
   float percent1 = getVar(cPercent1)->asFloat()->readVar();
   long op = getVar(cOp)->asMathOperator()->readVar();
   float percent2 = getVar(cPercent2)->asFloat()->readVar();
   float result = BTriggerEffect::calculateValuesFloat(percent1, op, percent2);
   getVar(cOutput)->asFloat()->writeVar(result);
}


//=============================================================================
// BTriggerEffect::teGetOwner
//=============================================================================
void BTriggerEffect::teGetOwner()
{
   enum { cUnit = 1, cSquad = 2, cOutputOwner = 3, };
   bool useUnit = getVar(cUnit)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   BTRIGGER_ASSERT(!(useUnit && useSquad));

   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (pUnit)
      {
         long playerID = pUnit->getPlayerID();
         getVar(cOutputOwner)->asPlayer()->writeVar(playerID);
         return;
      }
   }

   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         long playerID = pSquad->getPlayerID();
         getVar(cOutputOwner)->asPlayer()->writeVar(playerID);
         return;
      }
   }
}


//=============================================================================
// BTriggerEffect::teObjectiveUserMessage
//=============================================================================
void BTriggerEffect::teObjectiveUserMessage()
{
   enum 
   { 
      cObjective = 1, 
      cIndex     = 2,
      cXPos      = 3,
      cYPos      = 4,
      cJustify   = 5,
      cPoint     = 6,
      cAlpha     = 7,
      cTextColor = 8,
      cEnabled   = 9,
   };

   BColor nullColor;
   nullColor.set(-1.0f, -1.0f, -1.0f);

   long   objectiveID = getVar(cObjective)->asObjective()->readVar();
   long   index       = getVar(cIndex)->asMessageIndex()->readVar();
   float  xPos        = getVar(cXPos)->isUsed() ? getVar(cXPos)->asFloat()->readVar() : -1.0f;
   float  yPos        = getVar(cYPos)->isUsed() ? getVar(cYPos)->asFloat()->readVar() : -1.0f;
   long   justify     = getVar(cJustify)->isUsed() ? getVar(cJustify)->asMessageJustify()->readVar() : BUser::cUserMessageJustify_NULL;
   float  point       = getVar(cPoint)->isUsed() ? getVar(cPoint)->asMessagePoint()->readVar() : -1.0f;
   float  alpha       = getVar(cAlpha)->isUsed() ? getVar(cAlpha)->asFloat()->readVar() : -1.0f;
   BColor color       = getVar(cTextColor)->isUsed() ? getVar(cTextColor)->asColor()->readVar() : nullColor;
   bool   enabled     = getVar(cEnabled)->asBool()->readVar();

   gWorld->getObjectiveManager()->setObjectiveUserMessage(objectiveID, index, xPos, yPos, justify, point, alpha, color, enabled);
}


//=============================================================================
// BTriggerEffect::teUserMessage
//=============================================================================
void BTriggerEffect::teUserMessage()
{
   switch(getVersion())
   {
      case 3:
         teUserMessageV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "Obsolete user message trigger effect version!");
         break;
   }
}

//=============================================================================
// BTriggerEffect::teUserMessageV3
//=============================================================================
void BTriggerEffect::teUserMessageV3()
{
   enum 
   {       
      cPlayerList = 1, 
      cStringVar  = 2,
      cIndex      = 3,
      cXPos       = 4,
      cYPos       = 5,
      cJustify    = 6,
      cPoint      = 7,
      cAlpha      = 8,
      cTextColor  = 9,
      cEnabled    = 10,
      cPlayer     = 11,
   };

   BPlayerIDArray playerList;

   if (getVar(cPlayer)->isUsed())
   {
      playerList.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }

   if (getVar(cPlayerList)->isUsed())
   {
      const BPlayerIDArray& inputPlayerList = getVar(cPlayerList)->asPlayerList()->readVar();
      long                  numPlayers      = inputPlayerList.getNumber();
      for (long i = 0; i < numPlayers; i++)
      {
         playerList.uniqueAdd(inputPlayerList[i]);
      }
   }

   BColor nullColor;
   nullColor.set(-1.0f, -1.0f, -1.0f);

   BSimUString    pString    = getVar(cStringVar)->isUsed() ? getVar(cStringVar)->asString()->readVar() : "NULL";
   long           index      = getVar(cIndex)->asMessageIndex()->readVar();
   float          xPos       = getVar(cXPos)->isUsed() ? getVar(cXPos)->asFloat()->readVar() : -1.0f;
   float          yPos       = getVar(cYPos)->isUsed() ? getVar(cYPos)->asFloat()->readVar() : -1.0f;
   long           justify    = getVar(cJustify)->isUsed() ? getVar(cJustify)->asMessageJustify()->readVar() : BUser::cUserMessageJustify_NULL;
   float          point      = getVar(cPoint)->isUsed() ? getVar(cPoint)->asMessagePoint()->readVar() : -1.0f;
   float          alpha      = getVar(cAlpha)->isUsed() ? getVar(cAlpha)->asFloat()->readVar() : -1.0f;
   BColor         color      = getVar(cTextColor)->isUsed() ? getVar(cTextColor)->asColor()->readVar() : nullColor;
   bool           enabled    = getVar(cEnabled)->asBool()->readVar();

   BUser::setUserMessage(index, playerList, &pString, xPos, yPos, (BUser::USER_MESSAGE_JUSTIFY)justify, point, alpha, color, enabled);
}

//=============================================================================
// Call the correct version
//=============================================================================
void BTriggerEffect::teFlareMinimapNormal()
{
   switch (getVersion())
   {
      case 1:
         teFlareMinimapNormalV1();
         break;

      case 2:
         teFlareMinimapNormalV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//=============================================================================
// Normal flare from player at specified location
//=============================================================================
void BTriggerEffect::teFlareMinimapNormalV1()
{
   enum { cSrcPlayer = 1, cFlareLocation = 2, };
   BPlayerID srcPlayerID = getVar(cSrcPlayer)->asPlayer()->readVar();
   BVector flareLocation = getVar(cFlareLocation)->asVector()->readVar();

   long numPlayers = gWorld->getNumberPlayers();
   for (long i=0; i<numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(i);
      if (pPlayer && pPlayer->isAlly(srcPlayerID))
         pPlayer->getAlertManager()->createFlareAlert(flareLocation, srcPlayerID);
   }
}

//=============================================================================
// Specified flare type from assigned player at specified location
//=============================================================================
void BTriggerEffect::teFlareMinimapNormalV2()
{
   enum 
   { 
      cSrcPlayer = 1, 
      cFlareLocation = 2, 
      cFlareType = 3,
   };

   BPlayerID srcPlayerID = getVar(cSrcPlayer)->asPlayer()->readVar();
   BVector flareLocation = getVar(cFlareLocation)->asVector()->readVar();

   int flareType = 0;
   if (getVar(cFlareType)->isUsed())
   {
      flareType = getVar(cFlareType)->asFlareType()->readVar();
   }

   long numPlayers = gWorld->getNumberPlayers();
   for (long i=0; i<numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(i);
      if (pPlayer && pPlayer->isAlly(srcPlayerID))
         pPlayer->getAlertManager()->createFlareAlert(flareLocation, srcPlayerID);
   }
}

//=============================================================================
// BTriggerEffect::teChangeOwner
//=============================================================================
void BTriggerEffect::teChangeOwner()
{
   switch (getVersion())
   {
      case 3:
         teChangeOwnerV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}


//=============================================================================
// BTriggerEffect::teChangeOwnerV3
//=============================================================================
void BTriggerEffect::teChangeOwnerV3()
{
   enum 
   { 
      cNewOwner = 3, 
      cSquad = 6, 
      cSquadList = 7, 
   };   

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No squad, or squad list assigned!");
      return;
   }

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }
   
   BPlayerID newOwnerID = getVar(cNewOwner)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5329
   const BPlayer* pPlayer = gWorld->getPlayer(newOwnerID);
//--
   BTRIGGER_ASSERTM(pPlayer, "Invalid player assigned!");
   if (!pPlayer)
   {
      return;
   }

   // Change squads' owner   
   uint numSquads = (uint)squadList.getNumber();
   for (uint i = 0; i < numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         pSquad->changeOwner(newOwnerID);
      }
   }
}

//XXXHalwes - 7/17/2007 - Obsolete and remove?
//=============================================================================
// BTriggerEffect::teMKTest
//=============================================================================
void BTriggerEffect::teMKTest()
{  
   /*enum { cEntityList = 1, cOutputVar = 2 };

   BEntityIDArray entityList = getVar(cEntityList)->asEntityList()->readVar();
   BEntityID firstEntityID;
   long playerID = cInvalidPlayerID;
   if (entityList.getNumber() > 0)
   {
      firstEntityID = entityList[0];
      const BEntity *pEntity = gWorld->getEntity(firstEntityID);
      playerID = pEntity ? pEntity->getPlayerID() : cInvalidPlayerID;
   }

   // create a new plan
   BPlan *plan = gAI.planCreate(playerID, cPlanTypeMilitary);
   if (!plan)
      return;

   plan->addSquadList(entityList);

   // Return the plan ID to the trigger
   getVar(cOutputVar)->asInteger()->writeVar((long) plan->getPlanID());*/
}

//=============================================================================
// BTriggerEffect::teTimeUserMessage
//=============================================================================
void BTriggerEffect::teTimeUserMessage()
{
   switch(getVersion())
   {
      case 4:
         teTimeUserMessageV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "Obsolete time user message trigger effect version!");
         break;
   }
}

//=============================================================================
// BTriggerEffect::teTimeUserMessageV4
//=============================================================================
void BTriggerEffect::teTimeUserMessageV4()
{
   enum 
   {       
      cPlayerList = 1, 
      cTime       = 2,
      cIndex      = 3,
      cXPos       = 4,
      cYPos       = 5,
      cJustify    = 6,
      cPoint      = 7,
      cAlpha      = 8,
      cTextColor  = 9,
      cEnabled    = 10,
      cHour       = 11,
      cMinute     = 12,
      cSecond     = 13,
      cDecimal    = 14,
      cPlayer     = 15,
   };

   BPlayerIDArray playerList;

   if (getVar(cPlayer)->isUsed())
   {
      playerList.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }

   if (getVar(cPlayerList)->isUsed())
   {
      const BPlayerIDArray& inputPlayerList = getVar(cPlayerList)->asPlayerList()->readVar();
      long                  numPlayers      = inputPlayerList.getNumber();
      for (long i = 0; i < numPlayers; i++)
      {
         playerList.uniqueAdd(inputPlayerList[i]);
      }
   }

   BColor nullColor;
   nullColor.set(-1.0f, -1.0f, -1.0f);

   // Format field use
   bool bHourUsed = getVar(cHour)->isUsed();
   bool bMinUsed  = getVar(cMinute)->isUsed();
   bool bSecUsed  = getVar(cSecond)->isUsed();
   bool bDecUsed  = getVar(cDecimal)->isUsed();

   // Init vars
   DWORD  time       = getVar(cTime)->asTime()->readVar();
   long   index      = getVar(cIndex)->asMessageIndex()->readVar();
   float  xPos       = getVar(cXPos)->isUsed() ? getVar(cXPos)->asFloat()->readVar() : -1.0f;
   float  yPos       = getVar(cYPos)->isUsed() ? getVar(cYPos)->asFloat()->readVar() : -1.0f;
   long   justify    = getVar(cJustify)->isUsed() ? getVar(cJustify)->asMessageJustify()->readVar() : BUser::cUserMessageJustify_NULL;
   float  point      = getVar(cPoint)->isUsed() ? getVar(cPoint)->asMessagePoint()->readVar() : -1.0f;
   float  alpha      = getVar(cAlpha)->isUsed() ? getVar(cAlpha)->asFloat()->readVar() : -1.0f;
   BColor color      = getVar(cTextColor)->isUsed() ? getVar(cTextColor)->asColor()->readVar() : nullColor;
   bool   enabled    = getVar(cEnabled)->asBool()->readVar();
   bool   bHour      = bHourUsed ? getVar(cHour)->asBool()->readVar() : false;
   bool   bMin       = bMinUsed ? getVar(cMinute)->asBool()->readVar() : false;
   bool   bSec       = bSecUsed ? getVar(cSecond)->asBool()->readVar() : false;
   bool   bDec       = bDecUsed ? getVar(cDecimal)->asBool()->readVar() : false;

   // If all format fields are not used then have all fields on by default
   if (!bHourUsed && !bMinUsed && !bSecUsed && !bDecUsed)
   {
      bHour = true;
      bMin  = true;
      bSec  = true;
      bDec  = true;
   }

   BSimUString text     = L"";
   BSimUString txtHour  = L"";
   BSimUString txtMin   = L"";
   BSimUString txtSec   = L"";
   BSimUString txtDec   = L"";
   float       convert  = (float)time / 3600000.0f;
   long        hours    = (long)convert;
   convert              = (convert - floor(convert)) * 60.0f;
   long        minutes  = (long)convert;
   convert              = (convert - floor(convert)) * 60.0f;
   long        seconds  = (long)convert;
   long        cSeconds = (long)((convert - floor(convert)) * 100.0f);

   if (bHour)
   {
      if (hours < 10)
      {
         txtHour.format(L"0%d", hours);
      }
      else
      {
         txtHour.format(L"%d", hours);
      }

      if (bMin)
      {
         txtHour.format(L"%s:", txtHour.getPtr());
      }
   }

   if (bMin)
   {
      if (minutes < 10)
      {
         txtMin.format(L"0%d", minutes);
      }
      else
      {
         txtMin.format(L"%d", minutes);
      }

      if (bSec)
      {
         txtMin.format(L"%s:", txtMin.getPtr());
      }
   }

   if (bSec)
   {
      if (seconds < 10)
      {
         txtSec.format(L"0%d", seconds);
      }
      else
      {
         txtSec.format(L"%d", seconds);
      }

      if (bDec)
      {
         txtSec.format(L"%s.", txtSec.getPtr());
      }
   }

   if (bDec)
   {
      if (cSeconds < 10)
      {
         txtDec.format(L"0%d", cSeconds);
      }
      else
      {
         txtDec.format(L"%d", cSeconds);
      }
   }

   text.format(L"%s%s%s%s", txtHour.getPtr(), txtMin.getPtr(), txtSec.getPtr(), txtDec.getPtr());

   BUser::setUserMessage(index, playerList, &text, xPos, yPos, (BUser::USER_MESSAGE_JUSTIFY)justify, point, alpha, color, enabled);
}


//==============================================================================
// BTriggerEffect::teUnitListGetSize()
//==============================================================================
void BTriggerEffect::teUnitListGetSize()
{
   enum { cUnitList = 1, cOutputSize = 2, };
   const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
   long listSize = unitList.getSize();
   getVar(cOutputSize)->asInteger()->writeVar(listSize);
}


//==============================================================================
// BTriggerEffect::teSquadListGetSize()
//==============================================================================
void BTriggerEffect::teSquadListGetSize()
{
   enum { cSquadList = 1, cOutputSize = 2, };
   const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
   long listSize = squadList.getSize();
   getVar(cOutputSize)->asInteger()->writeVar(listSize);
}


//==============================================================================
// BTriggerEffect::teUnitListAdd()
//==============================================================================
void BTriggerEffect::teUnitListAdd()
{
   enum { cDestList = 1, cAddUnit = 2, cAddList = 3, cAddMoreList = 4, cClearExisting = 5, };
   BTriggerVarUnitList *pDestList = getVar(cDestList)->asUnitList();

   bool clearExisting = getVar(cClearExisting)->asBool()->readVar();
   if (clearExisting)
      pDestList->clear();

   bool useAddUnit = getVar(cAddUnit)->isUsed();
   if (useAddUnit)
   {
      BEntityID unitID = getVar(cAddUnit)->asUnit()->readVar();
      if (gWorld->getUnit(unitID))
         pDestList->addUnit(unitID);
   }

   bool useAddList = getVar(cAddList)->isUsed();
   if (useAddList)
   {
      const BEntityIDArray& addList = getVar(cAddList)->asUnitList()->readVar();
      long addListCount = addList.getNumber();
      for (long i=0; i<addListCount; i++)
         pDestList->addUnit(addList[i]);
   }

   bool useAddMoreList = getVar(cAddMoreList)->isUsed();
   if (useAddMoreList)
   {
      const BEntityIDArray& addMoreList = getVar(cAddMoreList)->asUnitList()->readVar();
      long addMoreListCount = addMoreList.getNumber();
      for (long i=0; i<addMoreListCount; i++)
         pDestList->addUnit(addMoreList[i]);
   }
}


//==============================================================================
// BTriggerEffect::teSquadListAdd()
//==============================================================================
void BTriggerEffect::teSquadListAdd()
{
   enum { cDestList = 1, cAddSquad = 2, cAddList = 3, cAddMoreList = 4, cClearExisting = 5, };
   BTriggerVarSquadList *pDestList = getVar(cDestList)->asSquadList();

   bool clearExisting = getVar(cClearExisting)->asBool()->readVar();
   if (clearExisting)
      pDestList->clear();

   bool useAddSquad = getVar(cAddSquad)->isUsed();
   if (useAddSquad)
   {
      BEntityID squadID = getVar(cAddSquad)->asSquad()->readVar();
      if (gWorld->getSquad(squadID))
         pDestList->addSquad(squadID);
   }

   bool useAddList = getVar(cAddList)->isUsed();
   if (useAddList)
   {
      const BEntityIDArray& addList = getVar(cAddList)->asSquadList()->readVar();
      long addListCount = addList.getNumber();
      for (long i=0; i<addListCount; i++)
         pDestList->addSquad(addList[i]);
   }

   bool useAddMoreList = getVar(cAddMoreList)->isUsed();
   if (useAddMoreList)
   {
      const BEntityIDArray& addMoreList = getVar(cAddMoreList)->asSquadList()->readVar();
      long addMoreListCount = addMoreList.getNumber();
      for (long i=0; i<addMoreListCount; i++)
         pDestList->addSquad(addMoreList[i]);
   }
}


//==============================================================================
// BTriggerEffect::teUnitListRemove()
//==============================================================================
void BTriggerEffect::teUnitListRemove()
{
   enum { cSrcList = 1, cRemoveUnit = 2, cRemoveList = 3, cRemoveAll = 4, };
   BTriggerVarUnitList *pUnitList = getVar(cSrcList)->asUnitList();

   bool removeAll = getVar(cRemoveAll)->asBool()->readVar();
   if (removeAll)
   {
      pUnitList->clear();
      return;
   }

   bool useRemoveUnit = getVar(cRemoveUnit)->isUsed();
   if (useRemoveUnit)
      pUnitList->removeUnit(getVar(cRemoveUnit)->asUnit()->readVar());

   bool useRemoveList = getVar(cRemoveList)->isUsed();
   if (useRemoveList)
   {
      const BEntityIDArray &removeList = getVar(cRemoveList)->asUnitList()->readVar();
      for (long i=0; i<removeList.getNumber(); i++)
      {
         pUnitList->removeUnit(removeList[i]);
      }
   }
}

//==============================================================================
// BTriggerEffect::teSquadListRemove()
//==============================================================================
void BTriggerEffect::teSquadListRemove()
{
   enum { cSrcList = 1, cRemoveSquad = 2, cRemoveList = 3, cRemoveAll = 4, };
   BTriggerVarSquadList *pSquadList = getVar(cSrcList)->asSquadList();

   bool removeAll = getVar(cRemoveAll)->asBool()->readVar();
   if (removeAll)
   {
      pSquadList->clear();
      return;
   }

   bool useRemoveSquad = getVar(cRemoveSquad)->isUsed();
   if (useRemoveSquad)
      pSquadList->removeSquad(getVar(cRemoveSquad)->asSquad()->readVar());

   bool useRemoveList = getVar(cRemoveList)->isUsed();
   if (useRemoveList)
   {
      const BEntityIDArray &removeList = getVar(cRemoveList)->asSquadList()->readVar();
      for (long i=0; i<removeList.getNumber(); i++)
      {
         pSquadList->removeSquad(removeList[i]);
      }
   }
}


//==============================================================================
// BTriggerEffect::teIteratorUnitList()
//==============================================================================
void BTriggerEffect::teIteratorUnitList()
{
   enum { cUnitList = 1, cIterator = 2, };
   BTriggerVarID unitListVarID = getVar(cUnitList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(unitListVarID);
}


//==============================================================================
// BTriggerEffect::teIteratorSquadList()
//==============================================================================
void BTriggerEffect::teIteratorSquadList()
{
   enum { cSquadList = 1, cIterator = 2, };
   BTriggerVarID squadListVarID = getVar(cSquadList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(squadListVarID);
}


//==============================================================================
// BTriggerEffect::teCreateUnit
//==============================================================================
void BTriggerEffect::teCreateUnit()
{
   SCOPEDSAMPLE(BTriggerEffect_teCreateUnit);
   
   switch (getVersion())
   {
      case 1:
         teCreateUnitV1();
         break;

      case 2:
         teCreateUnitV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Create a unit
//==============================================================================
void BTriggerEffect::teCreateUnitV1()
{
   enum { cProtoObject = 1, cPlayer = 2, cLocation = 3, cStartBuilt = 4, cOutputUnit = 5, cOutputUnitList = 6, cOutputSquad = 7, cOutputSquadList = 8, cClearExisting = 9, };
   long protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   long playerID = getVar(cPlayer)->asPlayer()->readVar();
   BVector createLocation = getVar(cLocation)->asVector()->readVar();
   bool startBuilt = getVar(cStartBuilt)->asBool()->readVar();

   bool useOutputUnit = getVar(cOutputUnit)->isUsed();
   bool useOutputUnitList = getVar(cOutputUnitList)->isUsed();
   bool useOutputSquad = getVar(cOutputSquad)->isUsed();
   bool useOutputSquadList = getVar(cOutputSquadList)->isUsed();
   bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   BProtoObject *pProtoObject = pPlayer ? pPlayer->getProtoObject(protoObjectID) : gDatabase.getGenericProtoObject(protoObjectID);
   BTRIGGER_ASSERT(pProtoObject);
   if (!pProtoObject)
      return;
   long objectClass = pProtoObject->getObjectClass();
   BTRIGGER_ASSERT(objectClass == cObjectClassBuilding || objectClass == cObjectClassSquad || objectClass == cObjectClassUnit);
   if (objectClass != cObjectClassBuilding && objectClass != cObjectClassSquad && objectClass != cObjectClassUnit)
      return;

   // BUILDING, UNIT, and SQUAD proto object types (the only we are allowing) all create squads for their respective entities and return the squad handle.
   BEntityID createdSquadID = cInvalidObjectID;
   BEntityID createdUnitID = cInvalidObjectID;
   createdSquadID = gWorld->createEntity(protoObjectID, false, playerID, createLocation, cZAxisVector, cXAxisVector, startBuilt, false, false, cInvalidObjectID, cInvalidPlayerID, cInvalidObjectID, cInvalidObjectID, true);
   BSquad *pSquad = gWorld->getSquad(createdSquadID);
   if (!pSquad)
   {
      if (useOutputUnit)
         getVar(cOutputUnit)->asUnit()->writeVar(cInvalidObjectID);
      if (useOutputSquad)
         getVar(cOutputSquad)->asSquad()->writeVar(cInvalidObjectID);
      return;
   }

#ifndef BUILD_FINAL
   gTriggerManager.incrementCreatePerfCount();
#endif

   //pSquad->doIdle();
   createdUnitID = pSquad->getLeader();
   BTRIGGER_ASSERT(gWorld->getUnit(createdUnitID));

   if (useOutputUnit)
   {
      getVar(cOutputUnit)->asUnit()->writeVar(createdUnitID);
   }

   if (useOutputUnitList)
   {
      BTriggerVarUnitList *pVarUnitList = getVar(cOutputUnitList)->asUnitList();
      if (clearExisting)
         pVarUnitList->clear();
      pVarUnitList->addUnit(createdUnitID);
   }

   if (useOutputSquad)
   {
      getVar(cOutputSquad)->asSquad()->writeVar(createdSquadID);
   }

   if (useOutputSquadList)
   {
      BTriggerVarSquadList *pVarSquadList = getVar(cOutputSquadList)->asSquadList();
      if (clearExisting)
         pVarSquadList->clear();
      pVarSquadList->addSquad(createdSquadID);
   }
}

//==============================================================================
// Create a unit
//==============================================================================
void BTriggerEffect::teCreateUnitV2()
{
   enum 
   { 
      cProtoObject = 1, 
      cPlayer = 2, 
      cLocation = 3, 
      cStartBuilt = 4, 
      cOutputUnit = 5, 
      cOutputUnitList = 6, 
      cOutputSquad = 7, 
      cOutputSquadList = 8, 
      cClearExisting = 9, 
      cFacing = 10,
   };
   
   BProtoObjectID protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();

   BVector createLocation = getVar(cLocation)->asVector()->readVar();
   BVector facing;
   if (getVar(cFacing)->isUsed())
   {
      facing = getVar(cFacing)->asVector()->readVar();
      if (!facing.safeNormalize())
         facing = cZAxisVector;
   }
   else
      facing = cZAxisVector;
   BVector right; 
   right.assignCrossProduct(cYAxisVector, facing);
   right.normalize();

   bool startBuilt = getVar(cStartBuilt)->asBool()->readVar();

   bool useOutputUnit = getVar(cOutputUnit)->isUsed();
   bool useOutputUnitList = getVar(cOutputUnitList)->isUsed();
   bool useOutputSquad = getVar(cOutputSquad)->isUsed();
   bool useOutputSquadList = getVar(cOutputSquadList)->isUsed();
   bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 5290
   const BProtoObject* pProtoObject = pPlayer ? pPlayer->getProtoObject(protoObjectID) : gDatabase.getGenericProtoObject(protoObjectID);
//--
   BTRIGGER_ASSERT(pProtoObject);
   if (!pProtoObject)
      return;   
   long objectClass = pProtoObject->getObjectClass();
   BTRIGGER_ASSERT((objectClass == cObjectClassBuilding) || (objectClass == cObjectClassSquad) || (objectClass == cObjectClassUnit));
   if ((objectClass != cObjectClassBuilding) && (objectClass != cObjectClassSquad) && (objectClass != cObjectClassUnit))
      return;

   // BUILDING, UNIT, and SQUAD proto object types (the only we are allowing) all create squads for their respective entities and return the squad handle.
   BEntityID createdSquadID = cInvalidObjectID;
   BEntityID createdUnitID = cInvalidObjectID;
   createdSquadID = gWorld->createEntity(protoObjectID, false, playerID, createLocation, facing, right, startBuilt, false, false, cInvalidObjectID, cInvalidPlayerID, cInvalidObjectID, cInvalidObjectID, true);
//-- FIXING PREFIX BUG ID 5302
   const BSquad* pSquad = gWorld->getSquad(createdSquadID);
//--
   if (!pSquad)
   {
      if (useOutputUnit)
         getVar(cOutputUnit)->asUnit()->writeVar(cInvalidObjectID);
      if (useOutputSquad)
         getVar(cOutputSquad)->asSquad()->writeVar(cInvalidObjectID);
      return;
   }

#ifndef BUILD_FINAL
   gTriggerManager.incrementCreatePerfCount();
#endif

   //pSquad->doIdle();
   createdUnitID = pSquad->getLeader();
   BTRIGGER_ASSERT(gWorld->getUnit(createdUnitID));

   if (useOutputUnit)
   {
      getVar(cOutputUnit)->asUnit()->writeVar(createdUnitID);
   }

   if (useOutputUnitList)
   {
      BTriggerVarUnitList* pVarUnitList = getVar(cOutputUnitList)->asUnitList();
      if (clearExisting)
         pVarUnitList->clear();
      pVarUnitList->addUnit(createdUnitID);
   }

   if (useOutputSquad)
   {
      getVar(cOutputSquad)->asSquad()->writeVar(createdSquadID);
   }

   if (useOutputSquadList)
   {
      BTriggerVarSquadList* pVarSquadList = getVar(cOutputSquadList)->asSquadList();
      if (clearExisting)
         pVarSquadList->clear();
      pVarSquadList->addSquad(createdSquadID);
   }
}

//==============================================================================
// Select correct version
//==============================================================================
void BTriggerEffect::tePlayAnimationUnit()
{
   switch (getVersion())
   {
      case 2:
         tePlayAnimationUnitV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Play an animation for a unit
//==============================================================================
void BTriggerEffect::tePlayAnimationUnitV2()
{
   enum 
   { 
      cUnit = 1, 
      cAnimType = 2, 
      cOutputAnimDuration = 4, 
   };

   long animType = getVar(cAnimType)->asAnimType()->readVar();
   DWORD duration = 0;
   BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());   
   if (pUnit)
   {
      // Way of the opp.
      //BUnitOpp* pOpp = BUnitOpp::getInstance();
      //pOpp->init();
      ////XXXHalwes - 10/26/2007 - Do we want to expose a target for facing purposes?
      ////pOpp->setTarget(mTarget);
      //pOpp->setType(BUnitOpp::cTypeAnimation);
      //BTRIGGER_ASSERT(animType < UINT8_MAX);
      //pOpp->setUserData((uint8)animType);
      //pOpp->generateID();
      //pOpp->setTrigger(true);
      //if (!pUnit->addOpp(pOpp))
      //{
      //   BUnitOpp::releaseInstance(pOpp);
      //}

      //BVisual* pVisual = pUnit->getVisual();
      //if (pVisual)
      //{
      //   BVisualAnimationData animData = pVisual->getAnimationData(cActionAnimationTrack, animType);
      //   duration = (DWORD)(animData.mDuration * 1000.0f);
      //}

      // Way of the animation state.
      // Remove idle action so that idle durations are not calculated incorrectly, units will automatically go idle so this is OK.
      pUnit->removeAllActionsOfType(BAction::cActionTypeEntityIdle);
      pUnit->unlockAnimation();
      pUnit->clearAnimationState();
      pUnit->setAnimationState(BObjectAnimationState::cAnimationStateMisc, animType, true, true);
      duration = pUnit->getAnimationDurationDWORD(cActionAnimationTrack);
      pUnit->lockAnimation(duration, true);
      pUnit->clearController(BActionController::cControllerAnimation);      
   }

   if (getVar(cOutputAnimDuration)->isUsed())
   {
      getVar(cOutputAnimDuration)->asTime()->writeVar(duration);
   }
}

//==============================================================================
// Select the correct version
//==============================================================================
void BTriggerEffect::tePlayAnimationSquad()
{
   switch (getVersion())
   {
      case 2:
         tePlayAnimationSquadV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Play an animation for a squad
//==============================================================================
void BTriggerEffect::tePlayAnimationSquadV2()
{
   enum 
   { 
      cSquad = 1, 
      cAnimType = 2, 
      cAnimDuration = 4,
   };

   long animType = getVar(cAnimType)->asAnimType()->readVar();
   BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
//-- FIXING PREFIX BUG ID 5304
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   if (pSquad)
   {
      // Way of the animation state
      DWORD duration = 0;
      uint numChildren = pSquad->getNumberChildren();
      for (uint i = 0; i < numChildren; i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
         {
            // Remove idle action so that idle durations are not calculated incorrectly, units will automatically go idle so this is OK.
            pUnit->removeAllActionsOfType(BAction::cActionTypeEntityIdle);
            pUnit->unlockAnimation();
            pUnit->clearAnimationState();
            pUnit->setAnimationState(BObjectAnimationState::cAnimationStateMisc, animType, true, true);
            DWORD thisDuration = pUnit->getAnimationDurationDWORD(cActionAnimationTrack);
            pUnit->lockAnimation(thisDuration, true);
            pUnit->clearController(BActionController::cControllerAnimation);      
            if (duration < thisDuration)
            {
               duration = thisDuration;
            }
         }
      }

      if (getVar(cAnimDuration)->isUsed())
      {
         getVar(cAnimDuration)->asTime()->writeVar(duration);
      }

      // Way of the opp
      //BUnitOpp opp;
      //opp.init();
      ////XXXHalwes - 10/26/2007 - Do we want to expose a target for facing purposes?
      ////opp.setTarget(mTarget);
      //opp.setType(BUnitOpp::cTypeAnimation);
      //opp.setSource(pSquad->getID());
      //BTRIGGER_ASSERT(animType < UINT8_MAX);
      //opp.setUserData(animType);
      //opp.generateID();
      //opp.setTrigger(true);      
      //pSquad->addOppToChildren(opp);
   }
}

//==============================================================================
// BTriggerEffect::teInputUIUnit()
//==============================================================================
void BTriggerEffect::teInputUIUnit()
{
   uint version = getVersion();
   if (version == 1)
   {
      teInputUIUnitV1();
   }
   else if (version == 2 || version == 3)
   {
      teInputUIUnitV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//==============================================================================
// BTriggerEffect::teInputUIUnitV1()
//==============================================================================
void BTriggerEffect::teInputUIUnitV1()
{
   enum { cUserPlayer = 1, cUnitPlayer = 2, cUnitProtoObject = 3, cOutputUIUnit = 4, };
   BTriggerVar* pUIUnit = getVar(cOutputUIUnit);
   pUIUnit->asUIUnit()->writeResult(BTriggerVarUIUnit::cUIUnitResultWaiting);

   long userPlayerID = getVar(cUserPlayer)->asPlayer()->readVar();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   BTriggerVarID UIUnitVarID = pUIUnit->getID();
   long unitPlayerID = getVar(cUnitPlayer)->isUsed() ? getVar(cUnitPlayer)->asPlayer()->readVar() : -1;   
   long unitProtoObjectID = getVar(cUnitProtoObject)->isUsed() ? getVar(cUnitProtoObject)->asProtoObject()->readVar() : -1;

   BPlayer *pPlayer = gWorld->getPlayer(userPlayerID);
   if (!pPlayer)
      return;
   BUser *pUser = pPlayer->getUser();
   if (!pUser)
      return;

   pUser->changeMode(BUser::cUserModeInputUIUnit);
   pUser->setBuildProtoID(-1);
   pUser->setUIPowerRadius(0.0f);
   pUser->resetAllInputUIModeData();
   if (unitPlayerID != -1)
      pUser->appendInputUIModePlayerFilter(unitPlayerID);
   if (unitProtoObjectID != -1)
      pUser->appendInputUIModeProtoObjectFilter(unitProtoObjectID);
   pUser->setTriggerScriptID(triggerScriptID);
   pUser->setTriggerVarID(UIUnitVarID);
}


//==============================================================================
// BTriggerEffect::teInputUIUnitV2()
//==============================================================================
void BTriggerEffect::teInputUIUnitV2()
{
   enum { cUserPlayer = 1, cUnitPlayer = 2, cOutputUIUnit = 4, cUnitObjectType = 5, cPower = 6, };
   BTriggerVar* pUIUnit = getVar(cOutputUIUnit);
   pUIUnit->asUIUnit()->writeResult(BTriggerVarUIUnit::cUIUnitResultWaiting);

   long userPlayerID = getVar(cUserPlayer)->asPlayer()->readVar();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   BTriggerVarID UIUnitVarID = pUIUnit->getID();
   long unitPlayerID = getVar(cUnitPlayer)->isUsed() ? getVar(cUnitPlayer)->asPlayer()->readVar() : -1;   
   long unitObjectType = getVar(cUnitObjectType)->isUsed() ? getVar(cUnitObjectType)->asObjectType()->readVar() : -1;
   long protoPowerID = (getVersion() > 2 && getVar(cPower)->isUsed()) ? getVar(cPower)->asPower()->readVar() : -1;

   BPlayer *pPlayer = gWorld->getPlayer(userPlayerID);
   if (!pPlayer)
      return;
   BUser *pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUIUnit);
      pUser->setBuildProtoID(-1);
      pUser->setUIPowerRadius(0.0f);
      pUser->setUIProtoPowerID(protoPowerID);
      pUser->resetAllInputUIModeData();
      if (unitPlayerID != -1)
         pUser->appendInputUIModePlayerFilter(unitPlayerID);
      if (unitObjectType != -1)
         pUser->appendInputUIModeObjectTypeFilter(unitObjectType);
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UIUnitVarID);
   }
   else
   {
      BTriggerCommand *pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(userPlayerID, cCommandTrigger);
      pCommand->setSenders(1, &userPlayerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUIUnitResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UIUnitVarID);
      pCommand->setInputResult(BTriggerVarUIUnit::cUIUnitResultUILockError);
      pCommand->setInputUnit(cInvalidObjectID);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}


//=====================================================
// Call correct version of InputUISquad trigger effect
//=====================================================
void BTriggerEffect::teInputUISquad()
{
   switch (getVersion())
   {
      case 2:
         teInputUISquadV2();
         break;

      case 3:
      case 4:
         teInputUISquadV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// BTriggerEffect::teInputUISquadV2()
//==============================================================================
void BTriggerEffect::teInputUISquadV2()
{
   enum { cUserPlayer = 1, cSquadPlayer = 2, cSquadProtoSquad = 3, cOutputUISquad = 4, cChildrenObjectType = 5, };
   BTriggerVar* pUISquad = getVar(cOutputUISquad);
   pUISquad->asUISquad()->writeResult(BTriggerVarUISquad::cUISquadResultWaiting);

   long userPlayerID = getVar(cUserPlayer)->asPlayer()->readVar();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   BTriggerVarID UISquadVarID = pUISquad->getID();
   long squadPlayerID = getVar(cSquadPlayer)->isUsed() ? getVar(cSquadPlayer)->asPlayer()->readVar() : -1;   
   long squadProtoSquadID = getVar(cSquadProtoSquad)->isUsed() ? getVar(cSquadProtoSquad)->asProtoSquad()->readVar() : -1;
   long childrenUnitObjectTypeID = getVar(cChildrenObjectType)->isUsed() ? getVar(cChildrenObjectType)->asObjectType()->readVar() : -1;

   BPlayer *pPlayer = gWorld->getPlayer(userPlayerID);
   if (!pPlayer)
      return;
   BUser *pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUISquad);
      pUser->setBuildProtoID(-1);
      pUser->setUIPowerRadius(0.0f);
      pUser->resetAllInputUIModeData();
      if (squadPlayerID != -1)
         pUser->appendInputUIModePlayerFilter(squadPlayerID);
      if (squadProtoSquadID != -1)
         pUser->appendInputUIModeProtoSquadFilter(squadProtoSquadID);
      if (childrenUnitObjectTypeID != -1)
         pUser->appendInputUIModeObjectTypeFilter(childrenUnitObjectTypeID);
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UISquadVarID);
   }
   else
   {
      BTriggerCommand *pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(userPlayerID, cCommandTrigger);
      pCommand->setSenders(1, &userPlayerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUISquadResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UISquadVarID);
      pCommand->setInputResult(BTriggerVarUISquad::cUISquadResultUILockError);
      pCommand->setInputSquad(cInvalidObjectID);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teInputUISquadV3()
{
   enum 
   { 
      cUserPlayer = 1, 
      cSquadPlayer = 2, 
      cSquadProtoSquad = 3, 
      cOutputUISquad = 4, 
      cChildrenObjectType = 5, 
      cChildrenObjectTypeList = 6,
      cDiplomacy = 7,
      cPower = 8,
   };

   BTriggerVar* pUISquad = getVar(cOutputUISquad);
   pUISquad->asUISquad()->writeResult(BTriggerVarUISquad::cUISquadResultWaiting);

   BPlayerID userPlayerID = getVar(cUserPlayer)->asPlayer()->readVar();
   int triggerScriptID = getParentTriggerScript()->getID();
   BTriggerVarID UISquadVarID = pUISquad->getID();
   BPlayerID squadPlayerID = getVar(cSquadPlayer)->isUsed() ? getVar(cSquadPlayer)->asPlayer()->readVar() : -1;   
   BProtoSquadID squadProtoSquadID = getVar(cSquadProtoSquad)->isUsed() ? getVar(cSquadProtoSquad)->asProtoSquad()->readVar() : -1;
   BObjectTypeID childrenUnitObjectTypeID = getVar(cChildrenObjectType)->isUsed() ? getVar(cChildrenObjectType)->asObjectType()->readVar() : -1;
   BRelationType diplomacy = getVar(cDiplomacy)->isUsed() ? getVar(cDiplomacy)->asRelationType()->readVar() : (BRelationType)cRelationTypeAny;
   long protoPowerID = (getVersion() > 3 && getVar(cPower)->isUsed()) ? getVar(cPower)->asPower()->readVar() : -1;

   BObjectTypeIDArray childrenUnitObjectTypeIDList;
   if (getVar(cChildrenObjectTypeList)->isUsed())
   {
      childrenUnitObjectTypeIDList = getVar(cChildrenObjectTypeList)->asObjectTypeList()->readVar();
   }

   BPlayer *pPlayer = gWorld->getPlayer(userPlayerID);
   if (!pPlayer)
      return;
   BUser *pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUISquad);
      pUser->setBuildProtoID(-1);
      pUser->setUIPowerRadius(0.0f);
      pUser->setUIProtoPowerID(protoPowerID);
      pUser->resetAllInputUIModeData();
      if (squadPlayerID != -1)
      {
         pUser->appendInputUIModePlayerFilter(squadPlayerID);
      }
      if (squadProtoSquadID != -1)
      {
         pUser->appendInputUIModeProtoSquadFilter(squadProtoSquadID);
      }
      if (childrenUnitObjectTypeID != -1)
      {
         pUser->appendInputUIModeObjectTypeFilter(childrenUnitObjectTypeID);
      }
      uint numObjectTypes = (uint)childrenUnitObjectTypeIDList.getNumber();
      for (uint i = 0; i < numObjectTypes; i++)
      {
         pUser->appendInputUIModeObjectTypeFilter(childrenUnitObjectTypeIDList[i]);
      }
      if (diplomacy != cRelationTypeAny)
      {
         pUser->appendInputUIModeRelationFilter(diplomacy);
      }
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UISquadVarID);
   }
   else
   {
      BTriggerCommand* pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(userPlayerID, cCommandTrigger);
      pCommand->setSenders(1, &userPlayerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUISquadResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UISquadVarID);
      pCommand->setInputResult(BTriggerVarUISquad::cUISquadResultUILockError);
      pCommand->setInputSquad(cInvalidObjectID);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// BTriggerEffect::teTransform()
//==============================================================================
void BTriggerEffect::teTransform()
{
   enum { cNewProtoObject = 1, cTransformUnit = 2, cTransformList = 3, };
   bool useTransformUnit = getVar(cTransformUnit)->isUsed();
   bool useTransformList = getVar(cTransformList)->isUsed();
   long newProtoObjectID = getVar(cNewProtoObject)->asProtoObject()->readVar();
   BTRIGGER_ASSERT(gDatabase.getGenericProtoObject(newProtoObjectID));
   BProtoObject *pNewProtoObject = gDatabase.getGenericProtoObject(newProtoObjectID);
   if (!pNewProtoObject)
      return;

   if (useTransformUnit)
   {
      BEntityID unitID = getVar(cTransformUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (pUnit)
         pUnit->transform(newProtoObjectID);
   }

   if (useTransformList)
   {
      const BEntityIDArray& unitList = getVar(cTransformList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
      {
         BUnit *pUnit = gWorld->getUnit(unitList[i]);
         if (pUnit)
            pUnit->transform(newProtoObjectID);
      }
   }
}

//XXXHalwes - 7/17/2007 - Do we need both this and teGetPlayers2?
//==============================================================================
// Select the right version
//==============================================================================
void BTriggerEffect::teGetPlayers()
{
   switch (getVersion())
   {
      case 2:
         teGetPlayersV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;            
   }
}

//==============================================================================
// Get a list of the players
//==============================================================================
void BTriggerEffect::teGetPlayersV2()
{
   enum 
   { 
      cOutputPlayerList = 1, 
      cIncludeGaia = 2, 
      cPlayerState = 4, 
   }; 

   bool includeGaia = getVar(cIncludeGaia)->asBool()->readVar();
   bool usePlayerState = getVar(cPlayerState)->isUsed();
   BPlayerState playerState = (usePlayerState) ? getVar(cPlayerState)->asPlayerState()->readVar() : -1;
   BPlayerIDArray foundPlayers;

   int numPlayers = gWorld->getNumberPlayers();
   for (int i = 0; i < numPlayers; i++)
   {
//-- FIXING PREFIX BUG ID 5336
      const BPlayer* pPlayer = gWorld->getPlayer(i);
//--
      if (!pPlayer)
         continue;
      if (!includeGaia && pPlayer->isGaia())
         continue;
      if (usePlayerState && (pPlayer->getPlayerState() != playerState))
         continue;

      foundPlayers.add(pPlayer->getID());
   }

   getVar(cOutputPlayerList)->asPlayerList()->writeVar(foundPlayers);
}

//=============================================================================
// BTriggerEffect::teGetHealth
//=============================================================================
void BTriggerEffect::teGetHealth()
{
   switch(getVersion())
   {
      case 2:
         teGetHealthV2();
         break;

      case 3:
         teGetHealthV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "Obsolete GetHealth trigger effect version!");
         break;
   }
}

//=============================================================================
// BTriggerEffect::teGetHealthV2
//=============================================================================
void BTriggerEffect::teGetHealthV2()
{
   enum 
   { 
      cSquad        = 1, 
      cSquadList    = 2,
      cHitpoints    = 3,
      cHPPercent    = 4,
      cShieldpoints = 5,
      cSPPercent    = 6,
   };

   float totalHitpoints    = 0.0f;
   float maxHitpoints      = 0.0f;
   float totalShieldpoints = 0.0f;
   float maxShieldpoints   = 0.0f;

   BEntityIDArray unitList;

   // If squad input is used
   if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5305
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child = 0; child < numChildren; child++)
         {
            unitList.uniqueAdd(pSquad->getChild(child));
         }

         maxHitpoints    += pSquad->getHPMax();
         maxShieldpoints += pSquad->getSPMax();
      }
   }

   // If squad list input is used
   if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5306
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long child = 0; child < numChildren; child++)
            {
               unitList.uniqueAdd(pSquad->getChild(child));
            }

            maxHitpoints    += pSquad->getHPMax();
            maxShieldpoints += pSquad->getSPMax();
         }
      }
   }

   // Iterate through units
   long numUnits = unitList.getNumber();
   for (long i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 5339
      const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
      if (pUnit)
      {
         // Add total number of current hit points
         totalHitpoints += pUnit->getHitpoints();         

         // Add total number of current shield points
         totalShieldpoints += pUnit->getShieldpoints();
      }
   }

   // Write number of hit points
   if (getVar(cHitpoints)->isUsed())
   {
      getVar(cHitpoints)->asFloat()->writeVar(totalHitpoints);
   }

   // Calculate and write hit point percentage
   if (getVar(cHPPercent)->isUsed())
   {
      float result = (maxHitpoints == 0.0f) ? 0.0f : (totalHitpoints / maxHitpoints);
      getVar(cHPPercent)->asFloat()->writeVar(result);
   }

   // Write number of shield points
   if (getVar(cShieldpoints)->isUsed())
   {
      getVar(cShieldpoints)->asFloat()->writeVar(totalShieldpoints);
   }

   // Calculate and write shield point percentage
   if (getVar(cSPPercent)->isUsed())
   {
      float result = (maxShieldpoints == 0.0f) ? 0.0f : (totalShieldpoints / maxShieldpoints);
      getVar(cSPPercent)->asFloat()->writeVar(result);
   }
}

//=============================================================================
// BTriggerEffect::teGetHealthV3
//=============================================================================
void BTriggerEffect::teGetHealthV3()
{
   enum 
   { 
      cSquad        = 1, 
      cSquadList    = 2,
      cHitpoints    = 3,
      cHPPercent    = 4,
      cShieldpoints = 5,
      cSPPercent    = 6,
      cUnit         = 7,
      cUnitList     = 8,
   };

   float totalHitpoints    = 0.0f;
   float maxHitpoints      = 0.0f;
   float totalShieldpoints = 0.0f;
   float maxShieldpoints   = 0.0f;
   bool  useSquad          = getVar(cSquad)->isUsed();
   bool  useSquadList      = getVar(cSquadList)->isUsed();
   bool  useUnit           = getVar(cUnit)->isUsed();
   bool  useUnitList       = getVar(cUnitList)->isUsed();


   if (!useSquad && !useSquadList && !useUnit && !useUnitList)
   {
      BTRIGGER_ASSERTM(false, "No targets specified.");
      return;
   }

   BEntityIDArray unitList;
   // If unit list input is used
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();

      long numUnits = unitList.getNumber();
      for (long i = 0; i < numUnits; i++)
      {
//-- FIXING PREFIX BUG ID 5342
         const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
         if (pUnit)
         {
            maxHitpoints    += pUnit->getHPMax();
            maxShieldpoints += pUnit->getSPMax();
         }
      }
   }

   // If unit input is used
   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      unitList.uniqueAdd(unitID);

//-- FIXING PREFIX BUG ID 5326
      const BUnit* pUnit = gWorld->getUnit(unitID);
//--
      if (pUnit)
      {
         maxHitpoints    += pUnit->getHPMax();
         maxShieldpoints += pUnit->getSPMax();
      }      
   }

   // If squad input is used
   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 5315
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child = 0; child < numChildren; child++)
         {
            unitList.uniqueAdd(pSquad->getChild(child));
         }

         maxHitpoints    += pSquad->getHPMax();
         maxShieldpoints += pSquad->getSPMax();
      }
   }

   // If squad list input is used
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5317
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long child = 0; child < numChildren; child++)
            {
               unitList.uniqueAdd(pSquad->getChild(child));
            }

            maxHitpoints    += pSquad->getHPMax();
            maxShieldpoints += pSquad->getSPMax();
         }
      }
   }

   // Iterate through units
   long numUnits = unitList.getNumber();
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         // Add total number of current hit points
         totalHitpoints += pUnit->getHitpoints();         

         // Add total number of current shield points
         totalShieldpoints += pUnit->getShieldpoints();
      }
   }

   // Write number of hit points
   if (getVar(cHitpoints)->isUsed())
   {
      getVar(cHitpoints)->asFloat()->writeVar(totalHitpoints);
   }

   // Calculate and write hit point percentage
   if (getVar(cHPPercent)->isUsed())
   {
      float result = (maxHitpoints >= cFloatCompareEpsilon) ? (totalHitpoints / maxHitpoints) : 0.0f;
      getVar(cHPPercent)->asFloat()->writeVar(result);
   }

   // Write number of shield points
   if (getVar(cShieldpoints)->isUsed())
   {
      getVar(cShieldpoints)->asFloat()->writeVar(totalShieldpoints);
   }

   // Calculate and write shield point percentage
   if (getVar(cSPPercent)->isUsed())
   {
      float result = (maxShieldpoints >= cFloatCompareEpsilon) ? (totalShieldpoints / maxShieldpoints) : 0.0f;
      getVar(cSPPercent)->asFloat()->writeVar(result);
   }
}

//==============================================================================
// BTriggerEffect::teRandomCount()
//==============================================================================
void BTriggerEffect::teRandomCount()
{
   enum { cInputMin = 1, cInputMax = 2, cOutputRandom = 3, };   
   long min = 0;
   long max = BRandomManager::cLongMax;
   if (getVar(cInputMin)->isUsed())
      min = getVar(cInputMin)->asInteger()->readVar();
   if (getVar(cInputMax)->isUsed())
      max = getVar(cInputMax)->asInteger()->readVar();
   BTRIGGER_ASSERT(min < max);
   long value = getRandRange(cSimRand, min, max);
   getVar(cOutputRandom)->asInteger()->writeVar(value);
}


//=============================================================================
// BTriggerEffect::teMathCount
//=============================================================================
void BTriggerEffect::teMathCount()
{
   enum 
   { 
      cParam1 = 1, 
      cOp     = 2,
      cParam2 = 3,
      cOutput = 4
   };

   long param1 = getVar(cParam1)->asInteger()->readVar();
   long op     = getVar(cOp)->asMathOperator()->readVar();
   long param2 = getVar(cParam2)->asInteger()->readVar();

   long result = BTriggerEffect::calculateValuesLong(param1, op, param2);

   getVar(cOutput)->asInteger()->writeVar(result);
}

//=============================================================================
// BTriggerEffect::teMathHitpoints
//=============================================================================
void BTriggerEffect::teMathHitpoints()
{
   enum 
   { 
      cParam1 = 1, 
      cOp     = 2,
      cParam2 = 3,
      cOutput = 4
   };

   float param1 = getVar(cParam1)->asFloat()->readVar();
   long  op     = getVar(cOp)->asMathOperator()->readVar();
   float param2 = getVar(cParam2)->asFloat()->readVar();

   float result = BTriggerEffect::calculateValuesFloat(param1, op, param2);

   if (result < 0.0f)
   {
      result = 0.0f;
   }

   getVar(cOutput)->asFloat()->writeVar(result);
}

//=============================================================================
// BTriggerEffect::teAsString
//=============================================================================
void BTriggerEffect::teAsString()
{
   switch (getVersion())
   {
      case 1:
         teAsStringV1();
         break;

      case 2:
         teAsStringV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is nor supported!");
         break;
   }
}

//=============================================================================
// BTriggerEffect::teAsString
//=============================================================================
void BTriggerEffect::teAsStringV1()
{
   enum 
   { 
      cCount     = 1, 
      cHitpoints = 2,
      cPercent   = 3,
      cOutString = 4
   };

   bool countUsed   = getVar(cCount)->isUsed();
   bool hpUsed      = getVar(cHitpoints)->isUsed();
   bool percentUsed = getVar(cPercent)->isUsed();

   BSimUString result = "";

   if (countUsed)
   {
      long count = getVar(cCount)->asInteger()->readVar();
      result.format(L"%d", count);
   }
   else if (hpUsed)
   {
      float hp = getVar(cHitpoints)->asFloat()->readVar();
      result.format(L"%1.1f", hp);
   }
   else if (percentUsed)
   {
      float percent = getVar(cPercent)->asFloat()->readVar();
      result.format(L"%d%%", (long)(percent * 100.0f));
   }

   getVar(cOutString)->asString()->writeVar(result);
}

//=============================================================================
// BTriggerEffect::teAsString
//=============================================================================
void BTriggerEffect::teAsStringV2()
{
   enum 
   { 
      cCount = 1, 
      cFloat = 2,
      cPercent = 3,
      cOutString = 4,
      cTime = 5,
      cHour = 6,
      cMinute = 7,
      cSecond = 8,
      cDecimal = 9,
   };

   bool countUsed = getVar(cCount)->isUsed();
   bool hpUsed = getVar(cFloat)->isUsed();
   bool percentUsed = getVar(cPercent)->isUsed();
   bool timeUsed = getVar(cTime)->isUsed();

   BSimUString result = "";

   if (countUsed)
   {
      int count = getVar(cCount)->asInteger()->readVar();
      result.format(L"%d", count);
   }
   else if (hpUsed)
   {
      float hp = getVar(cFloat)->asFloat()->readVar();
      result.format(L"%1.1f", hp);
   }
   else if (percentUsed)
   {
      float percent = getVar(cPercent)->asFloat()->readVar();
      result.format(L"%d%%", (long)(percent * 100.0f));
   }
   else if(timeUsed)
   {
      // Format field use
      bool bHourUsed = getVar(cHour)->isUsed();
      bool bMinUsed  = getVar(cMinute)->isUsed();
      bool bSecUsed  = getVar(cSecond)->isUsed();
      bool bDecUsed  = getVar(cDecimal)->isUsed();

      // Init vars
      DWORD time = getVar(cTime)->asTime()->readVar();
      bool bHour = bHourUsed ? getVar(cHour)->asBool()->readVar() : false;
      bool bMin = bMinUsed ? getVar(cMinute)->asBool()->readVar() : false;
      bool bSec = bSecUsed ? getVar(cSecond)->asBool()->readVar() : false;
      bool bDec = bDecUsed ? getVar(cDecimal)->asBool()->readVar() : false;

      // If all format fields are not used then have all fields on by default
      if (!bHourUsed && !bMinUsed && !bSecUsed && !bDecUsed)
      {
         bHour = true;
         bMin = true;
         bSec = true;
         bDec = true;
      }

      BSimUString txtHour = L"";
      BSimUString txtMin = L"";
      BSimUString txtSec = L"";
      BSimUString txtDec = L"";
      float convert = (float)time / 3600000.0f;
      int hours = (int)convert;
      convert = (convert - floor(convert)) * 60.0f;
      int minutes = (int)convert;
      convert = (convert - floor(convert)) * 60.0f;
      int seconds  = (int)convert;
      int cSeconds = (int)((convert - floor(convert)) * 100.0f);

      if (bHour)
      {
         if (hours < 10)
         {
            txtHour.format(L"0%d", hours);
         }
         else
         {
            txtHour.format(L"%d", hours);
         }

         if (bMin)
         {
            txtHour.format(L"%s:", txtHour.getPtr());
         }
      }

      if (bMin)
      {
         if (minutes < 10)
         {
            txtMin.format(L"0%d", minutes);
         }
         else
         {
            txtMin.format(L"%d", minutes);
         }

         if (bSec)
         {
            txtMin.format(L"%s:", txtMin.getPtr());
         }
      }

      if (bSec)
      {
         if (seconds < 10)
         {
            txtSec.format(L"0%d", seconds);
         }
         else
         {
            txtSec.format(L"%d", seconds);
         }

         if (bDec)
         {
            txtSec.format(L"%s.", txtSec.getPtr());
         }
      }

      if (bDec)
      {
         if (cSeconds < 10)
         {
            txtDec.format(L"0%d", cSeconds);
         }
         else
         {
            txtDec.format(L"%d", cSeconds);
         }
      }

      result.format(L"%s%s%s%s", txtHour.getPtr(), txtMin.getPtr(), txtSec.getPtr(), txtDec.getPtr());
   }

   getVar(cOutString)->asString()->writeVar(result);
}

//=============================================================================
// BTriggerEffect::teCalculatePercentCount
//=============================================================================
void BTriggerEffect::teCalculatePercentCount()
{
   enum 
   { 
      cCountTest    = 1, 
      cCountControl = 2,
      cPercentage   = 3,
   };

   long  countTest    = getVar(cCountTest)->asInteger()->readVar();
   long  countControl = getVar(cCountControl)->asInteger()->readVar();
   float result       = 0.0f;  

   #ifndef BUILD_FINAL
   if (countControl == 0)
   {
      BSimString debugDataString;
      BSimString varName = getVar(cCountControl)->getName();
      if (varName.isEmpty())
         varName = "(NONE)";
      debugDataString.format("The ControlCount parameter (ID = %d, Name = %s) in CalculatePercentInt trigger effect cannot be 0!", getVar(cCountControl)->getID(), varName.getPtr());
      BTRIGGER_ASSERTM(false, debugDataString.getPtr());
   }
   #endif

   if (countControl != 0)
   {
      result = (float)countTest / (float)countControl;
   }

   getVar(cPercentage)->asFloat()->writeVar(result);
}

//=============================================================================
// BTriggerEffect::teCalculatePercentHitpoints
//=============================================================================
void BTriggerEffect::teCalculatePercentHitpoints()
{
   enum 
   { 
      cHPTest     = 1, 
      cHPControl  = 2,
      cPercentage = 3,
   };

   float hpTest    = getVar(cHPTest)->asFloat()->readVar();
   float hpControl = getVar(cHPControl)->asFloat()->readVar();
   float result    = 0.0f;  

   if (hpTest < 0.0f)
   {
      hpTest = 0.0f;
   }

   if (hpControl < 0.0f)
   {
      hpControl = 0.0f;
   }

   #ifndef BUILD_FINAL
   if (hpControl == 0.0f)
   {
      BSimString debugDataString;
      BSimString varName = getVar(cHPControl)->getName();
      if (varName.isEmpty())
         varName = "(NONE)";
      debugDataString.format("The ControlHitpoints parameter (ID = %d, Name = %s) in CalculatePercentHitpoints trigger effect cannot be 0.0!", getVar(cHPControl)->getID(), varName.getPtr());
      BTRIGGER_ASSERTM(false, debugDataString.getPtr());
   }
   #endif

   if (hpControl != 0.0f)
   {
      result = hpTest / hpControl;
   }

   getVar(cPercentage)->asFloat()->writeVar(result);
}


//=============================================================================
// BTriggerEffect::teCalculatePercentTime
//=============================================================================
void BTriggerEffect::teCalculatePercentTime()
{
   uint version = getVersion();
   if (version == 1)
   {
      teCalculatePercentTimeV1();
   }
   else if (version == 2)
   {
      teCalculatePercentTimeV2();
   }
   else
   {
      BTRIGGER_ASSERT(false);
   }
}


//=============================================================================
// BTriggerEffect::teCalculatePercentTimeV1
//=============================================================================
void BTriggerEffect::teCalculatePercentTimeV1()
{
   enum 
   { 
      cTimeTest    = 1, 
      cTimeControl = 2,
      cPercentage  = 3,
   };

   DWORD timeTest    = getVar(cTimeTest)->asTime()->readVar();
   DWORD timeControl = getVar(cTimeControl)->asTime()->readVar();
   float result      = 0.0f;  

   BTRIGGER_ASSERTM(timeControl != 0, "The ControlTime parameter in CalculatePercentTime trigger effect cannot be 0!");

   if (timeControl != 0)
   {
      result = (float)timeTest / (float)timeControl;
   }

   getVar(cPercentage)->asFloat()->writeVar(result);
}


//=============================================================================
// BTriggerEffect::teCalculatePercentTimeV2
//=============================================================================
void BTriggerEffect::teCalculatePercentTimeV2()
{
   enum { cTestTime = 1, cControlTime = 2, cPercentage = 3, cStartTime = 4, };
   DWORD startTime = getVar(cStartTime)->isUsed() ? getVar(cStartTime)->asTime()->readVar() : 0;
   DWORD testTime = getVar(cTestTime)->asTime()->readVar();
   DWORD controlTime = getVar(cControlTime)->asTime()->readVar();

   #ifndef BUILD_FINAL
   if (controlTime == 0)
   {
      BSimString debugDataString;
      BSimString varName = getVar(cControlTime)->getName();
      if (varName.isEmpty())
         varName = "(NONE)";
      debugDataString.format("The ControlTime parameter (ID = %d, Name = %s) in CalculatePercentTime trigger effect must be > 0!", getVar(cControlTime)->getID(), varName.getPtr());
      BTRIGGER_ASSERTM(false, debugDataString.getPtr());
   }
   #endif

   if (startTime > testTime)
   {
      // If the start time is after our test time, we are at 0% without doing any division.
      getVar(cPercentage)->asFloat()->writeVar(0.0f);
      return;
   }

   // Do the math.
   float percent = (controlTime > 0) ? ((float)(testTime-startTime)/(float)(controlTime-startTime)) : 0.0f;
   getVar(cPercentage)->asFloat()->writeVar(percent);
}


//=============================================================================
// BTriggerEffect::teLerpCount
//=============================================================================
void BTriggerEffect::teLerpCount()
{
   enum 
   { 
      cCount1  = 1, 
      cCount2  = 2,
      cPercent = 3,
      cResult  = 4,
   };

   long  count1  = getVar(cCount1)->asInteger()->readVar();
   long  count2  = getVar(cCount2)->asInteger()->readVar();
   float percent = getVar(cPercent)->asFloat()->readVar();
   percent = Math::Clamp(percent, 0.0f, 1.0f);

   long result = count1 + (long)(percent * (float)(count2 - count1));

   getVar(cResult)->asInteger()->writeVar(result);
}


//=============================================================================
// BTriggerEffect::teLerpPercent
//=============================================================================
void BTriggerEffect::teLerpPercent()
{
   enum { cPercent1 = 1, cPercent2 = 2, cLerpPercent = 3, cResult = 4, };
   float percent1 = getVar(cPercent1)->asFloat()->readVar();
   float percent2 = getVar(cPercent2)->asFloat()->readVar();
   float lerpPercent = getVar(cLerpPercent)->asFloat()->readVar();
   lerpPercent = Math::Clamp(lerpPercent, 0.0f, 1.0f);
   float result = percent1 + (lerpPercent * (percent2 - percent1));
   getVar(cResult)->asFloat()->writeVar(result);
}


//=============================================================================
// BTriggerEffect::teMathTime
//=============================================================================
void BTriggerEffect::teMathTime()
{
   enum { cFirstTime = 1, cMathOperator = 2, cSecondTime = 3, cResultTime = 4, };
   DWORD firstTime = getVar(cFirstTime)->asTime()->readVar();
   long mathOp = getVar(cMathOperator)->asMathOperator()->readVar();
   DWORD secondTime = getVar(cSecondTime)->asTime()->readVar();
   DWORD resultTime = (mathOp == Math::cOpTypeSubtract && secondTime >= firstTime) ? 0 : BTriggerEffect::calculateValuesDWORD(firstTime, mathOp, secondTime);
   getVar(cResultTime)->asTime()->writeVar(resultTime);
}


//=============================================================================
// BTriggerEffect::teGetGameTime
//=============================================================================
void BTriggerEffect::teGetGameTime()
{
   enum { cDeltaTime = 1, cResultTime = 2, };
   bool useDeltaTime = getVar(cDeltaTime)->isUsed();
   DWORD deltaTime = useDeltaTime ? getVar(cDeltaTime)->asTime()->readVar() : 0;
   DWORD currentGametime = gWorld->getGametime();
   DWORD resultTime = currentGametime + deltaTime;
   getVar(cResultTime)->asTime()->writeVar(resultTime);
}


//=============================================================================
// BTriggerEffect::teGetGameTimeRemaining
//=============================================================================
void BTriggerEffect::teGetGameTimeRemaining()
{
   enum { cTestTime = 1, cGameTimeRemaining = 2, };
   DWORD testTime = getVar(cTestTime)->asTime()->readVar();
   DWORD currentGametime = gWorld->getGametime();
   DWORD gameTimeRemaining = (currentGametime < testTime) ? (testTime - currentGametime) : 0;
   getVar(cGameTimeRemaining)->asTime()->writeVar(gameTimeRemaining);
}


//=============================================================================
// BTriggerEffect::teLerpColor
//=============================================================================
void BTriggerEffect::teLerpColor()
{
   enum 
   { 
      cColor1  = 1, 
      cColor2  = 2,
      cPercent = 3,
      cResult  = 4,
   };
   
   BColor color1  = getVar(cColor1)->asColor()->readVar();
   BColor color2  = getVar(cColor2)->asColor()->readVar();
   float  percent = getVar(cPercent)->asFloat()->readVar();
   percent = Math::Clamp(percent, 0.0f, 1.0f);

   BColor result = color1 + ((color2 - color1) * percent);

   getVar(cResult)->asColor()->writeVar(result);
}

//=============================================================================
// BTriggerEffect::teModifyProtoData
//=============================================================================
void BTriggerEffect::teModifyProtoData()
{
   switch(getVersion())
   {
      case 4:
         teModifyProtoDataV4();
         break;

      case 5:
         teModifyProtoDataV5();
         break;

      default:
         BTRIGGER_ASSERTM(false, "Obsolete modify proto data trigger effect version!");
         break;
   }
}

//=============================================================================
// Modify the proto object data
//=============================================================================
void BTriggerEffect::teModifyProtoDataV4()
{
   enum
   {
      cPlayer = 1,
      cPlayerList = 2,
      cObjectType = 3,
      cAmount = 4,
      cType = 5,
      cRelativity = 6,
      cAllActions = 7,
      cPercent = 8,
      cName = 9,
      cInvert = 10,
      cCommandType = 11,
      cCommandData = 12,
   };

   bool usePlayerList = getVar(cPlayerList)->isUsed();
   bool usePlayer = getVar(cPlayer)->isUsed();
   BTRIGGER_ASSERTM((usePlayer || usePlayerList), "No player or player list parameter was specified in ModifyProtoData trigger effect.");
   if (!usePlayer && !usePlayerList)
      return;

   bool useAmount = getVar(cAmount)->isUsed();
   bool usePercent = getVar(cPercent)->isUsed();
   BTRIGGER_ASSERTM((useAmount || usePercent), "No amount or percent parameter was specified in ModifyProtoData trigger effect.");
   if (!useAmount && !usePercent)
      return;

   // Get our local playerList. Start by assigning the playerList var if used, and unique adding the individual player
   BPlayerIDArray playerList;
   if (usePlayerList)
      playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   if (usePlayer)
      playerList.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());

   uint numPlayers = playerList.getSize();
   if (numPlayers == 0)
      return;

   // Amount takes precedent over percent
   float amount = 0.0f;
   if (useAmount)
      amount = getVar(cAmount)->asFloat()->readVar();
   else if (usePercent)
      amount = getVar(cPercent)->asFloat()->readVar();

   long objectID = getVar(cObjectType)->asObjectType()->readVar();
   long type = getVar(cType)->asObjectDataType()->readVar();
   long relativity = getVar(cRelativity)->asObjectDataRelative()->readVar();
   bool allActions = getVar(cAllActions)->asBool()->readVar();
   bool inv = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   long commandType = getVar(cCommandType)->isUsed() ? getVar(cCommandType)->asTechDataCommandType()->readVar() : -1;
   long commandData = getVar(cCommandData)->isUsed() ? getVar(cCommandData)->asInteger()->readVar() : -1;

   BSimString name = "";
   if (getVar(cName)->isUsed())
      name = getVar(cName)->asString()->readVar();

   // objectID might be an object type so decompose it into proto object types.
   BProtoObjectIDArray *protoObjectIDs = NULL;
   uint numProtoObjects = gDatabase.decomposeObjectType(objectID, &protoObjectIDs);
   if (protoObjectIDs)
   {
      for (uint i=0; i<numProtoObjects; i++)
      {
         BProtoObject* pBaseObject = gDatabase.getGenericProtoObject(protoObjectIDs->get(i));
         for (uint p=0; p<numPlayers; p++)
         {
            BPlayer* pPlayer = gWorld->getPlayer(playerList[p]);      
            if (pPlayer)
            {
               BProtoObject* pCurrentObject = pPlayer->getProtoObject(protoObjectIDs->get(i));

               BTechEffect effect;
               effect.applyProtoObjectEffect(pPlayer, pCurrentObject, pBaseObject, amount, type, relativity, allActions, name, commandType, commandData, inv);
            }
         }
      }
   }
}

//=============================================================================
// Modify the proto object data
//=============================================================================
void BTriggerEffect::teModifyProtoDataV5()
{
   enum
   {
      cPlayer = 1,
      cPlayerList = 2,
      cObjectType = 3,
      cAmount = 4,
      cType = 5,
      cRelativity = 6,
      cAllActions = 7,
      cPercent = 8,
      cName = 9,
      cInvert = 10,
      cCommandType = 11,
      cCommandData = 12,
      cCommandDataTech = 13,
   };

   bool usePlayerList = getVar(cPlayerList)->isUsed();
   bool usePlayer = getVar(cPlayer)->isUsed();
   BTRIGGER_ASSERTM((usePlayer || usePlayerList), "No player or player list parameter was specified in ModifyProtoData trigger effect.");
   if (!usePlayer && !usePlayerList)
      return;

   bool useAmount = getVar(cAmount)->isUsed();
   bool usePercent = getVar(cPercent)->isUsed();
   BTRIGGER_ASSERTM((useAmount || usePercent), "No amount or percent parameter was specified in ModifyProtoData trigger effect.");
   if (!useAmount && !usePercent)
      return;

   // Get our local playerList. Start by assigning the playerList var if used, and unique adding the individual player
   BPlayerIDArray playerList;
   if (usePlayerList)
      playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   if (usePlayer)
      playerList.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());

   uint numPlayers = playerList.getSize();
   if (numPlayers == 0)
      return;

   // Amount takes precedent over percent
   float amount = 0.0f;
   if (useAmount)
      amount = getVar(cAmount)->asFloat()->readVar();
   else if (usePercent)
      amount = getVar(cPercent)->asFloat()->readVar();

   long objectID = getVar(cObjectType)->asObjectType()->readVar();
   long type = getVar(cType)->asObjectDataType()->readVar();
   long relativity = getVar(cRelativity)->asObjectDataRelative()->readVar();
   bool allActions = getVar(cAllActions)->asBool()->readVar();
   bool inv = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   long commandType = getVar(cCommandType)->isUsed() ? getVar(cCommandType)->asTechDataCommandType()->readVar() : -1;

   int commandData = -1;
   if (getVar(cCommandDataTech)->isUsed())
   {
      commandData = getVar(cCommandDataTech)->asTech()->readVar();
   }
   else if (getVar(cCommandData)->isUsed())
   {
      commandData = getVar(cCommandData)->asInteger()->readVar();
   }
   
   BSimString name = "";
   if (getVar(cName)->isUsed())
      name = getVar(cName)->asString()->readVar();

   // objectID might be an object type so decompose it into proto object types.
   BProtoObjectIDArray *protoObjectIDs = NULL;
   uint numProtoObjects = gDatabase.decomposeObjectType(objectID, &protoObjectIDs);
   if (protoObjectIDs)
   {
      for (uint i=0; i<numProtoObjects; i++)
      {
         BProtoObject* pBaseObject = gDatabase.getGenericProtoObject(protoObjectIDs->get(i));
         for (uint p=0; p<numPlayers; p++)
         {
            BPlayer* pPlayer = gWorld->getPlayer(playerList[p]);      
            if (pPlayer)
            {
               BProtoObject* pCurrentObject = pPlayer->getProtoObject(protoObjectIDs->get(i));

               BTechEffect effect;
               effect.applyProtoObjectEffect(pPlayer, pCurrentObject, pBaseObject, amount, type, relativity, allActions, name, commandType, commandData, inv);
            }
         }
      }
   }
}

//==============================================================================
// BTriggerEffect::teGetPlayerCiv()
//==============================================================================
void BTriggerEffect::teGetPlayerCiv()
{
   enum { cPlayer = 1, cCiv = 2, };
   long playerID = getVar(cPlayer)->asPlayer()->readVar();
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   BTRIGGER_ASSERT(pPlayer);
   if (pPlayer)
      getVar(cCiv)->asCiv()->writeVar(pPlayer->getCivID());
}

//==============================================================================
// BTriggerEffect::teEnableAttackNotifications()
//==============================================================================
void BTriggerEffect::teEnableAttackNotifications()
{
   enum { cEnable = 1, };
   bool enable = getVar(cEnable)->asBool()->readVar();

   long numPlayers = gWorld->getNumberPlayers();
   for (long i=0; i<numPlayers; i++)
   {
//-- FIXING PREFIX BUG ID 5343
      const BPlayer* pPlayer = gWorld->getPlayer(i);
//--
      if (pPlayer)
         pPlayer->getAlertManager()->setSquelchAlerts(!enable);
   }
}

//==============================================================================
// BTriggerEffect::teGetIdleDuration()
//==============================================================================
void BTriggerEffect::teGetIdleDuration()
{
   enum { cUnit = 1, cSquad = 2, cDuration = 3, };
   bool useUnit = getVar(cUnit)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   DWORD idleDuration = 0;
   
   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (pUnit)
      {
         BEntityActionIdle *pIdleAction = (BEntityActionIdle*)pUnit->getActionByTypeConst(BAction::cActionTypeEntityIdle);
         if (pIdleAction)
         {
            idleDuration = pIdleAction->getIdleDuration();
         }
         else
         {
            idleDuration = 0;
         }
         getVar(cDuration)->asTime()->writeVar(idleDuration);
         return;
      }
   }

   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         BEntityActionIdle *pIdleAction = (BEntityActionIdle*)pSquad->getActionByTypeConst(BAction::cActionTypeEntityIdle);
         if (pIdleAction)
         {
            idleDuration = pIdleAction->getIdleDuration();
         }
         else
         {
            idleDuration = 0;
         }
         getVar(cDuration)->asTime()->writeVar(idleDuration);
         return;
      }
   }

   BTRIGGER_ASSERTM(false, "GetIdleDuration trigger effect unable to get idle duration from unit or squad.  Returning zero (probably incorrect).");
   getVar(cDuration)->asTime()->writeVar(idleDuration);
}

//XXXHalwes - 7/17/2007 - Invalid?
//=============================================================================
// BTriggerEffect::teGetHitZoneHealth
//=============================================================================
void BTriggerEffect::teGetHitZoneHealth()
{
   enum 
   { 
      cUnit      = 1, 
      cHZName    = 2,
      cHP        = 3,
      cHPPercent = 4,
      cSP        = 5,
      cSPPercent = 6,
   };

   float totalHitpoints    = 0.0f;
   float maxHitpoints      = 0.0f;
   float totalShieldpoints = 0.0f;
   float maxShieldpoints   = 0.0f;

   BEntityID   unitID = getVar(cUnit)->asUnit()->readVar();
   BSimUString uName  = getVar(cHZName)->asString()->readVar();

   BSimString hzName;
   hzName.format("%S", uName.getPtr());

   BUnit* pUnit   = gWorld->getUnit(unitID);
   long   hzIndex = -1;
   if (pUnit && pUnit->getHitZoneIndex(hzName, hzIndex))
   {
      totalHitpoints    = pUnit->getHitZoneList()->get(hzIndex).getHitpoints();
      maxHitpoints      = pUnit->getMaxHitZoneHitpoints(hzIndex);
      totalShieldpoints = pUnit->getHitZoneList()->get(hzIndex).getShieldpoints();
      maxShieldpoints   = pUnit->getMaxHitZoneShieldpoints(hzIndex);
   }

   // Write number of hit points
   if (getVar(cHP)->isUsed())
   {
      getVar(cHP)->asFloat()->writeVar(totalHitpoints);
   }

   // Calculate and write hit point percentage
   if (getVar(cHPPercent)->isUsed())
   {
      float result = (maxHitpoints == 0.0f) ? 0.0f : (totalHitpoints / maxHitpoints);
      getVar(cHPPercent)->asFloat()->writeVar(result);
   }

   // Write number of shield points
   if (getVar(cSP)->isUsed())
   {
      getVar(cSP)->asFloat()->writeVar(totalShieldpoints);
   }

   // Calculate and write shield point percentage
   if (getVar(cSPPercent)->isUsed())
   {
      float result = (maxShieldpoints == 0.0f) ? 0.0f : (totalShieldpoints / maxShieldpoints);
      getVar(cSPPercent)->asFloat()->writeVar(result);
   }
}

//XXXHalwes - 7/17/2007 - Invalid?
//=============================================================================
// BTriggerEffect::teSetHitZoneHealth
//=============================================================================
void BTriggerEffect::teSetHitZoneHealth()
{
   enum 
   { 
      cUnit      = 1, 
      cHZName    = 2,
      cHP        = 3,
      cHPPercent = 4,
      cSP        = 5,
      cSPPercent = 6,
   };

   float       totalHitpoints    = 0.0f;
   float       totalShieldpoints = 0.0f;
   BEntityID   unitID            = getVar(cUnit)->asUnit()->readVar();
   BSimUString uName             = getVar(cHZName)->asString()->readVar();

   BSimString hzName;
   hzName.format("%S", uName.getPtr());

   BUnit* pUnit   = gWorld->getUnit(unitID);
   long   hzIndex = -1;
   if (pUnit && pUnit->getHitZoneIndex(hzName, hzIndex))
   {
      // HP value takes precedent over percent
      if (getVar(cHP)->isUsed())
      {
         totalHitpoints = getVar(cHP)->asFloat()->readVar();
      }
      else if (getVar(cHPPercent)->isUsed())
      {
         totalHitpoints = getVar(cHPPercent)->asFloat()->readVar() * pUnit->getMaxHitZoneHitpoints(hzIndex);
      }

      // SP value takes precedent over percent
      if (getVar(cSP)->isUsed())
      {
         totalShieldpoints = getVar(cSP)->asFloat()->readVar();
      }
      else if (getVar(cSPPercent)->isUsed())
      {         
         totalShieldpoints = getVar(cSPPercent)->asFloat()->readVar() * pUnit->getMaxHitZoneShieldpoints(hzIndex);
      }

      pUnit->getHitZoneList()->get(hzIndex).setHitpoints(totalHitpoints);
      pUnit->getHitZoneList()->get(hzIndex).setShieldpoints(totalShieldpoints);
   }
}

//XXXHalwes - 7/17/2007 - Invalid?
//=============================================================================
// BTriggerEffect::teSetHitZoneActive
//=============================================================================
void BTriggerEffect::teSetHitZoneActive()
{
   enum 
   { 
      cUnit      = 1, 
      cHZName    = 2,
      cActive    = 3,
   };

   BEntityID   unitID = getVar(cUnit)->asUnit()->readVar();
   BSimUString uName  = getVar(cHZName)->asString()->readVar();
   bool        active = getVar(cActive)->asBool()->readVar();

   BSimString hzName;
   hzName.format("%S", uName.getPtr());

   BUnit* pUnit   = gWorld->getUnit(unitID);
   long   hzIndex = -1;
   if (pUnit && pUnit->getHitZoneIndex(hzName, hzIndex))
   {
      pUnit->getHitZoneList()->get(hzIndex).setActive(active);
   }
}


//=============================================================================
// BTriggerEffect::teSetResources()
//=============================================================================
void BTriggerEffect::teSetResources()
{
   enum { cPlayer = 1, cResources = 2, };
   long playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5346
   const BCost& resources = getVar(cResources)->asCost()->readVar();
//--
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPlayer->setResources(&resources);
}


//=============================================================================
// BTriggerEffect::teGetResources()
//=============================================================================
void BTriggerEffect::teGetResources()
{
   enum { cPlayer = 1, cResources = 2, };
   long playerID = getVar(cPlayer)->asPlayer()->readVar();
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      BCost resources = pPlayer->getResources();
      getVar(cResources)->asCost()->writeVar(resources);
   }
}

//=============================================================================
// BTriggerEffect::teSetResourcesTotals()
//=============================================================================
void BTriggerEffect::teSetResourcesTotals()
{
   enum 
   { 
      cPlayer    = 1, 
      cResources = 2, 
   };

   long     playerID  = getVar(cPlayer)->asPlayer()->readVar();
   BCost&   resources = getVar(cResources)->asCost()->readVar();
   BPlayer* pPlayer   = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      pPlayer->setTotalResources(&resources);
   }
}

//=============================================================================
// BTriggerEffect::teGetResourcesTotals()
//=============================================================================
void BTriggerEffect::teGetResourcesTotals()
{
   enum 
   { 
      cPlayer    = 1, 
      cResources = 2, 
   };

   long     playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5349
   const BPlayer* pPlayer  = gWorld->getPlayer(playerID);
//--
   if (pPlayer)
   {
      BCost resources = pPlayer->getTotalResources();
      getVar(cResources)->asCost()->writeVar(resources);
   }
}

//=============================================================================
// BTriggerEffect::teMathResources()
//=============================================================================
void BTriggerEffect::teMathResources()
{
   enum { cResources1 = 1, cOp = 2, cResources2 = 3, cOutput = 4, };
   BCost &resources1 = getVar(cResources1)->asCost()->readVar();
   long op = getVar(cOp)->asMathOperator()->readVar();
   BCost &resources2 = getVar(cResources2)->asCost()->readVar();
   BTRIGGER_ASSERT(op == Math::cOpTypeAdd || op == Math::cOpTypeSubtract);
   BCost resultResources = BTriggerEffect::calculateValuesCost(resources1, op, resources2);
   getVar(cOutput)->asCost()->writeVar(resultResources);
}


//=============================================================================
// BTriggerEffect::teIteratorObjectList()
//=============================================================================
void BTriggerEffect::teIteratorObjectList()
{
   enum { cObjectList = 1, cIterator = 2, };
   BTriggerVarID objectListVarID = getVar(cObjectList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(objectListVarID);
}


//=============================================================================
// BTriggerEffect::teForbid()
//=============================================================================
void BTriggerEffect::teForbid()
{
   enum { cMakeForbidden = 1, cAffectedPlayers = 2, cProtoTechList = 3, cProtoObjectList = 4, cProtoSquadList = 5, };
   bool useProtoTechList = getVar(cProtoTechList)->isUsed();
   bool useProtoObjectList = getVar(cProtoObjectList)->isUsed();
   bool useProtoSquadList = getVar(cProtoSquadList)->isUsed();
   bool makeForbidden = getVar(cMakeForbidden)->asBool()->readVar();
   const BPlayerIDArray& affectedPlayers = getVar(cAffectedPlayers)->asPlayerList()->readVar();
   
   long numAffectedPlayers = affectedPlayers.getNumber();
   for (long p=0; p<numAffectedPlayers; p++)
   {
      BPlayer *pPlayer = gWorld->getPlayer(affectedPlayers[p]);
      if (!pPlayer)
         continue;

      BUser* pUser = pPlayer->getUser();

      if (useProtoTechList)
      {
         const BTechIDArray &protoTechList = getVar(cProtoTechList)->asTechList()->readVar();
         long numProtoTechs = protoTechList.getNumber();
         for (long i=0; i<numProtoTechs; i++)
         {
            BProtoTech *pProtoTech = pPlayer->getProtoTech(protoTechList[i]);
            if (pProtoTech)
            {
               if (makeForbidden != pProtoTech->getFlagForbid() && pUser)
                  pUser->setFlagCommandMenuRefresh(true);

               pProtoTech->setFlagForbid(makeForbidden);
            }
         }
      }

      if (useProtoObjectList)
      {
         const BProtoObjectIDArray &protoObjectList = getVar(cProtoObjectList)->asProtoObjectList()->readVar();
         long numProtoObjects = protoObjectList.getNumber();
         for (long i=0; i<numProtoObjects; i++)
         {
            BProtoObject *pProtoObject = pPlayer->getProtoObject(protoObjectList[i]);
            if (pProtoObject)
            {
               if (makeForbidden != pProtoObject->getFlagForbid() && pUser)
                  pUser->setFlagCommandMenuRefresh(true);

               pProtoObject->setFlagForbid(makeForbidden);
            }
         }
      }

      if (useProtoSquadList)
      {
         const BProtoSquadIDArray &protoSquadList = getVar(cProtoSquadList)->asProtoSquadList()->readVar();
         long numProtoSquads = protoSquadList.getNumber();
         for (long i=0; i<numProtoSquads; i++)
         {
            BProtoSquad *pProtoSquad = pPlayer->getProtoSquad(protoSquadList[i]);
            if (pProtoSquad)
            {
               if (makeForbidden != pProtoSquad->getFlagForbid() && pUser)
                  pUser->setFlagCommandMenuRefresh(true);

               pProtoSquad->setFlagForbid(makeForbidden);
            }
         }
      }
   }
}


//=============================================================================
// BTriggerEffect::teInvertBool()
//=============================================================================
void BTriggerEffect::teInvertBool()
{
   enum { cCurrentBool = 1, cInverseBool = 2, };
   bool currentBool = getVar(cCurrentBool)->asBool()->readVar();
   bool inverseBool = !currentBool;
   getVar(cInverseBool)->asBool()->writeVar(inverseBool);
}


//=============================================================================
// BTriggerEffect::teRevealer()
//=============================================================================
void BTriggerEffect::teRevealer()
{
   switch(getVersion())
   {
      case 2:
         teRevealerV2();
         break;

      case 3:
         teRevealerV3();
         break;

      default:
         BTRIGGER_ASSERT(false);
         break;
   }
}

//=============================================================================
// BTriggerEffect::teRevealerV2()
//=============================================================================
void BTriggerEffect::teRevealerV2()
{
   enum { cPlayer = 1, cLocation = 2, cLifespan = 3, cLOS = 4, cCreatedRevealer = 5, cAddToObjectList = 6, cClearExisting = 7, };
   long playerID = getVar(cPlayer)->asPlayer()->readVar();
   BVector location = getVar(cLocation)->asVector()->readVar();
   float los = getVar(cLOS)->asFloat()->readVar();
   BObject *pRevealerObject = NULL;
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   BTeamID teamID = pPlayer ? pPlayer->getTeamID() : cInvalidTeamID;
   if (getVar(cLifespan)->isUsed())
   {
      DWORD lifespan = getVar(cLifespan)->asTime()->readVar();
      pRevealerObject = gWorld->createRevealer(teamID, location, los, lifespan);
   }
   else
   {
      pRevealerObject = gWorld->createRevealer(teamID, location, los);
   }

   if (pRevealerObject)
   {
      if (getVar(cCreatedRevealer)->isUsed())
      {
         getVar(cCreatedRevealer)->asObject()->writeVar(pRevealerObject->getID());
      }

      if (getVar(cAddToObjectList)->isUsed())
      {
         BTriggerVarObjectList *pVarObjectList = getVar(cAddToObjectList)->asObjectList();
         bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;
         if (clearExisting)
            pVarObjectList->clear();

         pVarObjectList->addObject(pRevealerObject->getID());
      }
   }
   else
   {
      if (getVar(cCreatedRevealer)->isUsed())
      {
         getVar(cCreatedRevealer)->asObject()->writeVar(cInvalidObjectID);
      }
   }
}

//=============================================================================
// Reveal at location(s)
//=============================================================================
void BTriggerEffect::teRevealerV3()
{
   enum 
   { 
      cPlayer = 1, 
      cLocation = 2, 
      cLifespan = 3, 
      cLOS = 4, 
      cCreatedRevealer = 5, 
      cAddToObjectList = 6, 
      cClearExisting = 7, 
      cInLocationList = 8,
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   float los = getVar(cLOS)->asFloat()->readVar();

   BVectorArray locList;
   if (getVar(cInLocationList)->isUsed())
   {
      locList = getVar(cInLocationList)->asVectorList()->readVar();
   }

   if (getVar(cLocation)->isUsed())
   {
      locList.add(getVar(cLocation)->asVector()->readVar());
   }
      
   DWORD lifespan = getVar(cLifespan)->isUsed() ? getVar(cLifespan)->asTime()->readVar() : 0;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   BTeamID teamID = pPlayer ? pPlayer->getTeamID() : cInvalidTeamID;

   BEntityIDArray objectList;
   uint numLocs = locList.getSize();
   for (uint i = 0; i < numLocs; i++)
   {
//-- FIXING PREFIX BUG ID 5350
      const BObject* pRevealerObject = NULL;
//--
      if (lifespan != 0)
      {
         pRevealerObject = gWorld->createRevealer(teamID, locList[i], los, lifespan);
      }
      else
      {
         pRevealerObject = gWorld->createRevealer(teamID, locList[i], los);
      }

      if (pRevealerObject)
      {
         objectList.add(pRevealerObject->getID());
      }
   }


   uint numObjects = objectList.getSize();
   if (getVar(cCreatedRevealer)->isUsed())
   {
      if (numObjects > 0 )
      {
         getVar(cCreatedRevealer)->asObject()->writeVar(objectList[0]);
      }
      else
      {
         getVar(cCreatedRevealer)->asObject()->writeVar(cInvalidObjectID);
      }
   }


   if (getVar(cAddToObjectList)->isUsed())
   {
      BEntityIDArray addToObjectList = getVar(cAddToObjectList)->asObjectList()->readVar();      
      bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;
      if (clearExisting)
      {
         addToObjectList.clear();
      }

      if (numObjects > 0)
      {
         addToObjectList.append(objectList);
      }
      getVar(cAddToObjectList)->asObjectList()->writeVar(addToObjectList);  
   }
}

//=============================================================================
// BTriggerEffect::teMathDistance()
//=============================================================================
void BTriggerEffect::teMathDistance()
{
   enum { cFirstDistance = 1, cMathOperator = 2, cSecondDistance = 3, cResultDistance = 4, };
   float firstDistance = getVar(cFirstDistance)->asFloat()->readVar();
   long mathOperator = getVar(cMathOperator)->asMathOperator()->readVar();
   float secondDistance = getVar(cSecondDistance)->asFloat()->readVar();
   float resultDistance = BTriggerEffect::calculateValuesFloat(firstDistance, mathOperator, secondDistance);
   resultDistance = max(0.0f, resultDistance);
   getVar(cResultDistance)->asFloat()->writeVar(resultDistance);
}


//=============================================================================
// BTriggerEffect::teGroupDeactivate
//=============================================================================
void BTriggerEffect::teGroupDeactivate()
{
   enum { cGroup = 1, };
   //long groupID = getVar(cGroup)->asGroup()->readVar();

   BFAIL("BTriggerEffect::teGroupDeactivate -- This functionality was removed.");
}


//=============================================================================
// BTriggerEffect::teIteratorLocationList
//=============================================================================
void BTriggerEffect::teIteratorLocationList()
{
   enum { cLocationList = 1, cIterator = 2, };
   BTriggerVarID locationListVarID = getVar(cLocationList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(locationListVarID);
}


//=============================================================================
//=============================================================================
void BTriggerEffect::teLocationListAdd()
{
   enum { cDestList = 1, cAddLocation = 2, cAddList = 3, cClearExisting = 4, };
   BTriggerVarVectorList *pDestList = getVar(cDestList)->asVectorList();

   bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;
   if (clearExisting)
      pDestList->clear();

   bool useAddLocation = getVar(cAddLocation)->isUsed();
   if (useAddLocation)
      pDestList->addVector(getVar(cAddLocation)->asVector()->readVar());

   bool useAddList = getVar(cAddList)->isUsed();
   if (useAddList)
   {
      const BVectorArray& addList = getVar(cAddList)->asVectorList()->readVar();
      uint addListCount = addList.getSize();
      for (uint i=0; i<addListCount; i++)
         pDestList->addVector(addList[i]);
   }
}


//=============================================================================
// BTriggerEffect::teLocationListRemove
//=============================================================================
void BTriggerEffect::teLocationListRemove()
{
   enum { cSrcList = 1, cRemoveLocation = 2, cRemoveList = 3, cRemoveAll = 4, };
   BTriggerVarVectorList *pVectorList = getVar(cSrcList)->asVectorList();

   bool removeAll = getVar(cRemoveAll)->isUsed() ? getVar(cRemoveAll)->asBool()->readVar() : false;
   if (removeAll)
   {
      pVectorList->clear();
      return;
   }

   bool useRemoveLocation = getVar(cRemoveLocation)->isUsed();
   if (useRemoveLocation)
      pVectorList->removeVector(getVar(cRemoveLocation)->asVector()->readVar());

   bool useRemoveList = getVar(cRemoveList)->isUsed();
   if (useRemoveList)
   {
      const BVectorArray &removeList = getVar(cRemoveList)->asVectorList()->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i=0; i<numToRemove; i++)
         pVectorList->removeVector(removeList[i]);
   }
}


//=============================================================================
// BTriggerEffect::teLocationListGetSize
//=============================================================================
void BTriggerEffect::teLocationListGetSize()
{
   enum { cLocationList = 1, cSize = 2, };
   const BLocationArray &locationList = getVar(cLocationList)->asVectorList()->readVar();
   long listSize = locationList.getSize();
   getVar(cSize)->asInteger()->writeVar(listSize);
}


//=============================================================================
// BTriggerEffect::teLerpLocation
//=============================================================================
void BTriggerEffect::teLerpLocation()
{
   enum { cLocation1 = 1, cLocation2 = 2, cLerpPercent = 3, cResult = 4, };
   BVector location1 = getVar(cLocation1)->asVector()->readVar();
   BVector location2 = getVar(cLocation2)->asVector()->readVar();
   float lerpPercent = getVar(cLerpPercent)->asFloat()->readVar();
   lerpPercent = Math::Clamp(lerpPercent, 0.0f, 1.0f);
   BVector resultLocation = location1 + (lerpPercent * (location2 - location1));
   getVar(cResult)->asVector()->writeVar(resultLocation);
}


//=============================================================================
// BTriggerEffect::teMathLocation
//=============================================================================
void BTriggerEffect::teMathLocation()
{
   enum { cLocation1 = 1, cOp = 2, cLocation2 = 3, cResult = 4, };
   BVector location1 = getVar(cLocation1)->asVector()->readVar();
   long mathOperator = getVar(cOp)->asMathOperator()->readVar();
   BVector location2 = getVar(cLocation2)->asVector()->readVar();
   BVector result = BTriggerEffect::calculateValuesVector(location1, mathOperator, location2);
   getVar(cResult)->asVector()->writeVar(result);
}


//=============================================================================
// BTriggerEffect::teSquadListPartition
//=============================================================================
void BTriggerEffect::teSquadListPartition()
{
   enum 
   { 
      cSrcList = 1, 
      cPercentToA = 2, 
      cCountToA = 3, 
      cListA = 4, 
      cListB = 5, 
   };

   BEntityIDArray srcList = getVar(cSrcList)->asSquadList()->readVar();
   BTriggerVarSquadList* pListA = getVar(cListA)->asSquadList();
   BTriggerVarSquadList* pListB = getVar(cListB)->asSquadList();
   pListA->clear();
   pListB->clear();
   long numSrcItems = srcList.getSize();

   long countToA = numSrcItems;
   if (getVar(cCountToA)->isUsed())
   {
      countToA = getVar(cCountToA)->asInteger()->readVar();
      countToA = Math::Clamp(countToA, 0L, numSrcItems);
   }
   else if (getVar(cPercentToA)->isUsed())
   {
      float percentToA = getVar(cPercentToA)->asFloat()->readVar();
      percentToA = Math::Clamp(percentToA, 0.0f, 1.0f);
      countToA = Math::FloatToIntRound(percentToA * countToA);
   }

   for (long i = 0; i < numSrcItems; i++)
   {
      if (countToA > 0)
      {
         pListA->addSquad(srcList[i]);
         countToA--;
      }
      else
      {
         pListB->addSquad(srcList[i]);
      }
   }
}


//=============================================================================
// BTriggerEffect::teSquadListShuffle
//=============================================================================
void BTriggerEffect::teSquadListShuffle()
{
   enum { cSquadList = 1, };
   getVar(cSquadList)->asSquadList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teLocationListShuffle
//=============================================================================
void BTriggerEffect::teLocationListShuffle()
{
   enum { cLocationList = 1, };
   getVar(cLocationList)->asVectorList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teEntityListShuffle
//=============================================================================
void BTriggerEffect::teEntityListShuffle()
{
   enum { cEntityList = 1, };
   getVar(cEntityList)->asEntityList()->shuffle();
}


//=============================================================================
// BTriggerEffect::tePlayerListShuffle
//=============================================================================
void BTriggerEffect::tePlayerListShuffle()
{
   enum { cPlayerList = 1, };
   getVar(cPlayerList)->asPlayerList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teTeamListShuffle
//=============================================================================
void BTriggerEffect::teTeamListShuffle()
{
   enum { cTeamList = 1, };
   getVar(cTeamList)->asTeamList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teUnitListShuffle
//=============================================================================
void BTriggerEffect::teUnitListShuffle()
{
   enum { cUnitList = 1, };
   getVar(cUnitList)->asUnitList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teProtoObjectListShuffle
//=============================================================================
void BTriggerEffect::teProtoObjectListShuffle()
{
   enum { cProtoObjectList = 1, };
   getVar(cProtoObjectList)->asProtoObjectList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teObjectTypeListShuffle
//=============================================================================
void BTriggerEffect::teObjectTypeListShuffle()
{
   enum { cObjectTypeList = 1, };
   getVar(cObjectTypeList)->asObjectTypeList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teProtoSquadListShuffle
//=============================================================================
void BTriggerEffect::teProtoSquadListShuffle()
{
   enum { cProtoSquadList = 1, };
   getVar(cProtoSquadList)->asProtoSquadList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teTechListShuffle
//=============================================================================
void BTriggerEffect::teTechListShuffle()
{
   enum { cTechList = 1, };
   getVar(cTechList)->asTechList()->shuffle();
}


//=============================================================================
// BTriggerEffect::teUnitListPartition
//=============================================================================
void BTriggerEffect::teUnitListPartition()
{
   enum { cSrcList = 1, cPercentToA = 2, cCountToA = 3, cListA = 4, cListB = 5, };
   BEntityIDArray srcList = getVar(cSrcList)->asUnitList()->readVar();
   BTriggerVarUnitList *pListA = getVar(cListA)->asUnitList();
   BTriggerVarUnitList *pListB = getVar(cListB)->asUnitList();
   getVar(cSrcList)->asUnitList()->clear();
   pListA->clear();
   pListB->clear();
   long numSrcItems = srcList.getNumber();

   long countToA = numSrcItems;
   if (getVar(cCountToA)->isUsed())
   {
      countToA = getVar(cCountToA)->asInteger()->readVar();
      countToA = Math::Clamp(countToA, 0L, numSrcItems);
   }
   else if (getVar(cPercentToA)->isUsed())
   {
      float percentToA = getVar(cPercentToA)->asFloat()->readVar();
      percentToA = Math::Clamp(percentToA, 0.0f, 1.0f);
      countToA = Math::FloatToIntRound(percentToA * countToA);
   }

   for (long i=0; i<numSrcItems; i++)
   {
      if (countToA > 0)
      {
         pListA->addUnit(srcList[i]);
         countToA--;
      }
      else
      {
         pListB->addUnit(srcList[i]);
      }
   }
}


//=============================================================================
// BTriggerEffect::teLocationListPartition
//=============================================================================
void BTriggerEffect::teLocationListPartition()
{
   enum { cSrcList = 1, cPercentToA = 2, cCountToA = 3, cListA = 4, cListB = 5, };
   BLocationArray srcList = getVar(cSrcList)->asVectorList()->readVar();
   BTriggerVarVectorList *pListA = getVar(cListA)->asVectorList();
   BTriggerVarVectorList *pListB = getVar(cListB)->asVectorList();   
   pListA->clear();
   pListB->clear();
   long numSrcItems = srcList.getNumber();

   long countToA = numSrcItems;
   if (getVar(cCountToA)->isUsed())
   {
      countToA = getVar(cCountToA)->asInteger()->readVar();
      countToA = Math::Clamp(countToA, 0L, numSrcItems);
   }
   else if (getVar(cPercentToA)->isUsed())
   {
      float percentToA = getVar(cPercentToA)->asFloat()->readVar();
      percentToA = Math::Clamp(percentToA, 0.0f, 1.0f);
      countToA = Math::FloatToIntRound(percentToA * countToA);
   }

   for (long i=0; i<numSrcItems; i++)
   {
      if (countToA > 0)
      {
         pListA->addVector(srcList[i]);
         countToA--;
      }
      else
      {
         pListB->addVector(srcList[i]);
      }
   }
}

//=============================================================================
// BTriggerEffect::teRefCountUnitAdd
//=============================================================================
void BTriggerEffect::teRefCountUnitAdd()
{
   enum { cUnit = 1, cUnitList = 2, cSquad = 3, cSquadList = 5, cRefCountType = 6, cMaxCount = 4 };

   short maxCount = 0;

   short refCountType = getVar(cRefCountType)->asRefCountType()->readVar();
   refCountType += BEntityRef::cTypeDesignStart;

   if (refCountType >= BEntityRef::cTypeDesignStart && refCountType <= BEntityRef::cTypeDesignEnd)
   {
      if (getVar(cUnit)->isUsed())
         refCountAddHelper(getVar(cUnit)->asUnit()->readVar(), refCountType, maxCount);

      if (getVar(cUnitList)->isUsed())
      {
         const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
         long numUnits = unitList.getNumber();
         for (long i=0; i<numUnits; i++)
            refCountAddHelper(unitList[i], refCountType, maxCount);
      }

      if (getVar(cSquad)->isUsed())
      {
//-- FIXING PREFIX BUG ID 5331
         const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint i=0; i<numChildren; i++)
               refCountAddHelper(pSquad->getChild(i), refCountType, maxCount);
         }
      }

      if (getVar(cSquadList)->isUsed())
      {
         const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
         long numSquads = squadList.getNumber();
         for (long j=0; j<numSquads; j++)
         {
//-- FIXING PREFIX BUG ID 5333
            const BSquad* pSquad = gWorld->getSquad(squadList[j]);
//--
            if (pSquad)
            {
               uint numChildren = pSquad->getNumberChildren();
               for (uint i=0; i<numChildren; i++)
                  refCountAddHelper(pSquad->getChild(i), refCountType, maxCount);
            }
         }
      }
   }

   if (getVar(cMaxCount)->isUsed())
      getVar(cMaxCount)->asInteger()->writeVar(maxCount);
}


//=============================================================================
// BTriggerEffect::teRefCountUnitRemove
//=============================================================================
void BTriggerEffect::teRefCountUnitRemove()
{
   enum { cUnit = 1, cUnitList = 2, cSquad = 3, cSquadList = 5, cRefCountType = 6, cMaxCount = 4 };

   short maxCount = 0;

   short refCountType = getVar(cRefCountType)->asRefCountType()->readVar();
   refCountType += BEntityRef::cTypeDesignStart;

   if (refCountType >= BEntityRef::cTypeDesignStart && refCountType <= BEntityRef::cTypeDesignEnd)
   {
      if (getVar(cUnit)->isUsed())
         refCountRemoveHelper(getVar(cUnit)->asUnit()->readVar(), refCountType, maxCount);

      if (getVar(cUnitList)->isUsed())
      {
         const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
         long numUnits = unitList.getNumber();
         for (long i=0; i<numUnits; i++)
            refCountRemoveHelper(unitList[i], refCountType, maxCount);
      }

      if (getVar(cSquad)->isUsed())
      {
//-- FIXING PREFIX BUG ID 5334
         const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint i=0; i<numChildren; i++)
               refCountRemoveHelper(pSquad->getChild(i), refCountType, maxCount);
         }
      }

      if (getVar(cSquadList)->isUsed())
      {
         const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
         long numSquads = squadList.getNumber();
         for (long j=0; j<numSquads; j++)
         {
//-- FIXING PREFIX BUG ID 5337
            const BSquad* pSquad = gWorld->getSquad(squadList[j]);
//--
            if (pSquad)
            {
               uint numChildren = pSquad->getNumberChildren();
               for (uint i=0; i<numChildren; i++)
                  refCountRemoveHelper(pSquad->getChild(i), refCountType, maxCount);
            }
         }
      }
   }

   if (getVar(cMaxCount)->isUsed())
      getVar(cMaxCount)->asInteger()->writeVar(maxCount);
}


//=============================================================================
// BTriggerEffect::teRefCountSquadAdd
//=============================================================================
void BTriggerEffect::teRefCountSquadAdd()
{
   enum { cSquad = 1, cSquadList = 2, cRefCountType = 3, cMaxCount = 4 };

   short maxCount = 0;

   short refCountType = getVar(cRefCountType)->asRefCountType()->readVar();
   refCountType += BEntityRef::cTypeDesignStart;

   if (refCountType >= BEntityRef::cTypeDesignStart && refCountType <= BEntityRef::cTypeDesignEnd)
   {
      if (getVar(cSquad)->isUsed())
         refCountAddHelper(getVar(cSquad)->asSquad()->readVar(), refCountType, maxCount);

      if (getVar(cSquadList)->isUsed())
      {
         const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
         long numSquads = squadList.getNumber();
         for (long j=0; j<numSquads; j++)
            refCountAddHelper(squadList[j], refCountType, maxCount);
      }
   }

   if (getVar(cMaxCount)->isUsed())
      getVar(cMaxCount)->asInteger()->writeVar(maxCount);
}


//=============================================================================
// BTriggerEffect::teRefCountSquadRemove
//=============================================================================
void BTriggerEffect::teRefCountSquadRemove()
{
   enum { cSquad = 1, cSquadList = 2, cRefCountType = 3, cMaxCount = 4 };

   short maxCount = 0;

   short refCountType = getVar(cRefCountType)->asRefCountType()->readVar();
   refCountType += BEntityRef::cTypeDesignStart;

   if (refCountType >= BEntityRef::cTypeDesignStart && refCountType <= BEntityRef::cTypeDesignEnd)
   {
      if (getVar(cSquad)->isUsed())
         refCountRemoveHelper(getVar(cSquad)->asSquad()->readVar(), refCountType, maxCount);

      if (getVar(cSquadList)->isUsed())
      {
         const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
         long numSquads = squadList.getNumber();
         for (long j=0; j<numSquads; j++)
            refCountRemoveHelper(squadList[j], refCountType, maxCount);
      }
   }

   if (getVar(cMaxCount)->isUsed())
      getVar(cMaxCount)->asInteger()->writeVar(maxCount);
}

//==============================================================================
// BTriggerEffect::teRepair()
//==============================================================================
void BTriggerEffect::teRepair()
{
   enum 
   { 
      cSquad     = 1, 
      cSquadList = 2, 
      cUnit      = 3,
      cUnitList  = 4,
      cHP        = 5,
      cHPPercent = 6,
      cSP        = 7,
      cSPPercent = 8,
      cSpread    = 9,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useHP = getVar(cHP)->isUsed();
   bool useSP = getVar(cSP)->isUsed();
   bool useHPPercent = getVar(cHPPercent)->isUsed();
   bool useSPPercent = getVar(cSPPercent)->isUsed();
   bool useSpread = getVar(cSpread)->isUsed();
   bool spreadAcrossUnits = useSpread ? getVar(cSpread)->asBool()->readVar() : false;
   BEntityIDArray unitList;

   if (!useHP && !useSP && !useHPPercent && !useSPPercent)
   {
      BTRIGGER_ASSERTM(false, "No values assigned.");
      return;
   }
   if (!useSquad && !useSquadList && !useUnit && !useUnitList)
   {
      BTRIGGER_ASSERTM(false, "No targets specified.");
      return;
   }

   // If unit list input is used
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }
   
   // If unit input is used
   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // If squad input is used
   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 5338
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child = 0; child < numChildren; child++)
         {
            unitList.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   // If squad list input is used
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5340
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long child = 0; child < numChildren; child++)
            {
               unitList.uniqueAdd(pSquad->getChild(child));
            }
         }
      }
   }

   long numUnits = unitList.getNumber();
   if (numUnits == 0)
      return;

   float hpDelta = 0.0f;
   float spDelta = 0.0f;
   float hpPercent = 0.0f;
   float spPercent = 0.0f;
   bool  calcHPPercent = false;
   bool  calcSPPercent = false;

   if (useHP)
   {
      hpDelta = getVar(cHP)->asFloat()->readVar();
      if (spreadAcrossUnits)
      {
         hpDelta /= numUnits;
      }
   }
   else if (useHPPercent)
   {
      calcHPPercent = true;
      hpPercent = getVar(cHPPercent)->asFloat()->readVar();
   }

   if (useSP)
   {
      spDelta = getVar(cSP)->asFloat()->readVar();
      if (spreadAcrossUnits)
      {
         spDelta /= numUnits;
      }
   }
   else if (useSPPercent)
   {
      calcSPPercent = true;
      spPercent = getVar(cSPPercent)->asFloat()->readVar();
   }

   // Iterate through units
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (!pUnit)
      {
         continue;
      }

      if (!pUnit->isAlive())
      {
         continue;
      }

      float unitHP = pUnit->getHitpoints();
      float maxHP  = pUnit->getProtoObject()->getHitpoints();   
      float unitSP = pUnit->getShieldpoints();
      float maxSP  = pUnit->getProtoObject()->getShieldpoints();

      if (calcHPPercent)
      {
         hpDelta = hpPercent * maxHP;
      }
      unitHP += hpDelta;
      if (unitHP > maxHP)
      {
         unitHP = maxHP;
      }

      if (calcSPPercent)
      {
         spDelta = spPercent * maxSP;
      }
      unitSP += spDelta;
      if (unitSP > maxSP)
      {
         unitSP = maxSP;
      }

      #if defined(SYNC_UnitDetail)
         syncUnitDetailData("BTriggerEffect::fireTriggerEffectRepair unitID", pUnit->getID().asLong());
         syncUnitDetailData("BTriggerEffect::fireTriggerEffectRepair hp", unitHP);
         syncUnitDetailData("BTriggerEffect::fireTriggerEffectRepair sp", unitSP);
      #endif

      pUnit->setHitpoints(unitHP);
      pUnit->setShieldpoints(unitSP);
   }
}

//XXXHalwes - 7/17/2007 - Valid?
//=============================================================================
// BTriggerEffect::unitFlagSetHelper
//=============================================================================
void BTriggerEffect::unitFlagSetHelper(BEntityID unitID, long flagType, bool flagValue)
{
   //-- DJBFIXME:  Moving over to bitfields, make this work
   BUnit* pUnit = gWorld->getUnit(unitID);
   if (pUnit)
   {
      switch(flagType)
      {
         case BUnit::cFlagAttackBlocked:
         {
            pUnit->setFlagAttackBlocked(flagValue);
            break;
         }         
      }           
   }
}

//=============================================================================
// BTriggerEffect::squadFlagSetHelper
//=============================================================================
void BTriggerEffect::squadFlagSetHelper(BEntityID squadID, long flagType, bool flagValue)
{
   //-- DJBFIXME:  Moving over to bitfields, make this work
   BSquad* pSquad = gWorld->getSquad(squadID);
   if (pSquad)
   {
      switch(flagType)
      {
         case BSquad::cFlagAttackBlocked:
         {
            pSquad->setFlagAttackBlocked(flagValue);
            break;
         }         
      }           
   }
}

//XXXHalwes - 7/17/2007 - Valid?
//=============================================================================
// BTriggerEffect::teUnitFlagSet
//=============================================================================
void BTriggerEffect::teUnitFlagSet()
{
   enum { cUnit = 1, cUnitList = 2, cSquad = 3, cSquadList = 4, cFlagType = 5, cFlagValue = 6 };

   long flagType = getVar(cFlagType)->asUnitFlag()->readVar();
   if (flagType == -1)
      return;

   bool flagValue = getVar(cFlagValue)->asBool()->readVar();

   if (getVar(cUnit)->isUsed())
      unitFlagSetHelper(getVar(cUnit)->asUnit()->readVar(), flagType, flagValue);

   if (getVar(cUnitList)->isUsed())
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
         unitFlagSetHelper(unitList[i], flagType, flagValue);
   }

   if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5352
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint i=0; i<numChildren; i++)
            unitFlagSetHelper(pSquad->getChild(i), flagType, flagValue);
      }
   }

   if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long j=0; j<numSquads; j++)
      {
//-- FIXING PREFIX BUG ID 5353
         const BSquad* pSquad = gWorld->getSquad(squadList[j]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint i=0; i<numChildren; i++)
               unitFlagSetHelper(pSquad->getChild(i), flagType, flagValue);
         }
      }
   }
}


//=============================================================================
// BTriggerEffect::teGetChildUnits
//=============================================================================
void BTriggerEffect::teGetChildUnits()
{
   enum { cSquad = 1, cSquadList = 2, cUnitList = 3 };
   
   // Clear our list to start with.
   BTriggerVarUnitList *pVarUnitList = getVar(cUnitList)->asUnitList();
   pVarUnitList->clear();

   if (getVar(cSquad)->isUsed())
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long c=0; c<numChildren; c++)
         {
            BEntityID unitID = pSquad->getChild(c);
            BUnit *pUnit = gWorld->getUnit(unitID);
            if (pUnit)
               pVarUnitList->addUnit(unitID);
         }
      }
   }
   
   if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray &squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long s=0; s<numSquads; s++)
      {
         BSquad *pSquad = gWorld->getSquad(squadList[s]);
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long c=0; c<numChildren; c++)
            {
               BEntityID unitID = pSquad->getChild(c);
               BUnit *pUnit = gWorld->getUnit(unitID);
               if (pUnit)
                  pVarUnitList->addUnit(unitID);
            }
         }
      }
   }
}


//=============================================================================
// BTriggerEffect::teDamage
//=============================================================================
void BTriggerEffect::teDamage()
{
   enum { cSquad = 1, cSquadList = 2, cUnit = 3, cUnitList = 4, cHP = 5, cHPPercent = 6, cSP = 7, cSPPercent = 8, cSpread = 9, };
   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useHP = getVar(cHP)->isUsed();
   bool useHPPercent = getVar(cHPPercent)->isUsed();
   bool useSP = getVar(cSP)->isUsed();
   bool useSPPercent = getVar(cSPPercent)->isUsed();
   bool useSpread = getVar(cSpread)->isUsed();
   bool spreadAmongUnits = useSpread ? getVar(cSpread)->asBool()->readVar() : false;
   BEntityIDArray unitList;

   if (!useHP && !useSP && !useHPPercent && !useSPPercent)
   {
      BTRIGGER_ASSERTM(false, "No values assigned.");
      return;
   }
   if (!useSquad && !useSquadList && !useUnit && !useUnitList)
   {
      BTRIGGER_ASSERTM(false, "No targets specified.");
      return;
   }

   // If unit list input is used
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   // If unit input is used
   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // If squad input is used
   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 5354
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child = 0; child < numChildren; child++)
         {
            unitList.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   // If squad list input is used
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5355
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long child = 0; child < numChildren; child++)
            {
               unitList.uniqueAdd(pSquad->getChild(child));
            }
         }
      }
   }

   long numUnits = unitList.getNumber();
   if (numUnits <= 0)
      return;

   float hpDelta = 0.0f;
   float spDelta = 0.0f;
   float hpPercent = 0.0f;
   float spPercent = 0.0f;
   bool calcHPPercent = false;
   bool calcSPPercent = false;

   if (useHP)
   {
      hpDelta = getVar(cHP)->asFloat()->readVar();
      if (spreadAmongUnits)
      {
         hpDelta /= numUnits;
      }
   }
   else if (useHPPercent)
   {
      calcHPPercent = true;
      hpPercent = getVar(cHPPercent)->asFloat()->readVar();
   }

   if (useSP)
   {
      spDelta = getVar(cSP)->asFloat()->readVar();
      if (spreadAmongUnits)
      {
         spDelta /= numUnits;
      }
   }
   else if (useSPPercent)
   {
      calcSPPercent = true;
      spPercent = getVar(cSPPercent)->asFloat()->readVar();
   }

   // Iterate through units
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (!pUnit)
      {
         continue;
      }

      if (!pUnit->isAlive())
      {
         continue;
      }

      float unitHP = pUnit->getHitpoints();
      float minHP = 0.0f;
      float maxHP = pUnit->getProtoObject()->getHitpoints();   
      float unitSP = pUnit->getShieldpoints();
      float minSP = 0.0f;
      float maxSP = pUnit->getProtoObject()->getShieldpoints();

      if (calcHPPercent)
      {
         hpDelta = hpPercent * maxHP;
      }
      unitHP -= hpDelta;
      if (unitHP < minHP)
      {
         unitHP = minHP;
      }

      if (calcSPPercent)
      {
         spDelta = spPercent * maxSP;
      }
      unitSP -= spDelta;
      if (unitSP < minSP)
      {
         unitSP = minSP;
      }

      #if defined(SYNC_UnitDetail)
         syncUnitDetailData("BTriggerEffect::fireTriggerEffectDamage unitID", pUnit->getID().asLong());
         syncUnitDetailData("BTriggerEffect::fireTriggerEffectDamage hp", unitHP);
         syncUnitDetailData("BTriggerEffect::fireTriggerEffectDamage sp", unitSP);
      #endif

      pUnit->setHitpoints(unitHP);
      pUnit->setShieldpoints(unitSP);
   }
}

//=============================================================================
// BTriggerEffect::teUIUnlock
//=============================================================================
void BTriggerEffect::teUIUnlock()
{
   enum { cPlayer = 1, };

   // Get the user for this player (if it exists on this machine.)
   // If there is no user, there's no UI to unlock on this box.
   long playerID = getVar(cPlayer)->asPlayer()->readVar();
   BUser *pUser = gUserManager.getUserByPlayerID(playerID);
   if (!pUser)
      return;

   // If this trigger script has the user locked, unlock it.
   // Do not allow a script to unlock a user that was locked by someone else.
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   BTRIGGER_ASSERTM(!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID), "Script is trying to unlock a User (UI) that is not locked, or was locked by a different script!");
   if (pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->unlockUser();
      pUser->changeMode(BUser::cUserModeNormal);

      // ajl 7/13/07 - Commenting out call to clearSelections because it's causing a unit to get deselected after using or cancelling a 
      // unit ability (such as the hawk over thruster). Marc can't remember why he had originally put the clear in.
      //pUser->getSelectionManager()->clearSelections();
   }
}

//=============================================================================
// BTriggerEffect::teSquadListDiff()
//=============================================================================
void BTriggerEffect::teSquadListDiff()
{
   enum { cListA = 1, cListB = 2, cOnlyInA = 3, cOnlyInB = 4, cInBoth = 5, };
   bool useOnlyInA = getVar(cOnlyInA)->isUsed();
   bool useOnlyInB = getVar(cOnlyInB)->isUsed();
   bool useInBoth = getVar(cInBoth)->isUsed();
   if (!useOnlyInA && !useOnlyInB && !useInBoth)
      return;

   // It's a logic error to use the same variable for two of the outputs because one with bash the other.
   //BTRIGGER_ASSERT(getVar(cOnlyInA) != getVar(cOnlyInB) \
   //   && (getVar(cOnlyInA) != getVar(cInBoth) || !useInBoth) \
   //   && (getVar(cOnlyInB) != getVar(cInBoth) || !useInBoth));

   // Get local copies of our source data.
   BEntityIDArray listA = getVar(cListA)->asSquadList()->readVar();
   BEntityIDArray listB = getVar(cListB)->asSquadList()->readVar();
   long numInA = listA.getNumber();
   long numInB = listB.getNumber();

   // Clear our results lists.
   BTriggerVarSquadList *pOnlyInA = getVar(cOnlyInA)->asSquadList();
   if (useOnlyInA)
      pOnlyInA->clear();
   BTriggerVarSquadList *pOnlyInB = getVar(cOnlyInB)->asSquadList();
   if (useOnlyInB)
      pOnlyInB->clear();
   BTriggerVarSquadList *pInBoth = getVar(cInBoth)->asSquadList();
   if (useInBoth)
      pInBoth->clear();

   // Easiest case:  Both lists empty.
   if (numInA == 0 && numInB == 0)
   {
      return;
   }
   // Easy case:  ListA empty.
   else if (numInA == 0)
   {
      for (long i=0; i<numInB; i++)
      {
         if (useOnlyInB)
            pOnlyInB->addSquad(listB[i]);
         // Halwes - 10/7/2008 - This is incorrect logic since one list is empty none of the entities are in both lists.  If fixing this breaks to much stuff
         //                      we will want to add a flag to change the effect's behavior.
         //if (useInBoth)
         //   pInBoth->addSquad(listB[i]);
      }
   }
   // Easy case:  ListB empty.
   else if (numInB == 0)
   {
      for (long i=0; i<numInA; i++)
      {
         if (useOnlyInA)
            pOnlyInA->addSquad(listA[i]);
         // Halwes - 10/7/2008 - This is incorrect logic since one list is empty none of the entities are in both lists.  If fixing this breaks to much stuff
         //                      we will want to add a flag to change the effect's behavior.
         //if (useInBoth)
         //   pInBoth->addSquad(listA[i]);
      }
   }
   // Both lists have something.
   else
   {
      // Check listA's stuff.
      for (long i=0; i<numInA; i++)
      {
         // Couldn't find it, so it's only in A.
         if (listB.find(listA[i]) == cInvalidIndex)
         {
            if (useOnlyInA)
               pOnlyInA->addSquad(listA[i]);
         }
         // Otherwise, it's in both lists.
         else
         {
            if (useInBoth)
               pInBoth->addSquad(listA[i]);
         }
      }

      // Check listB's stuff.
      for (long i=0; i<numInB; i++)
      {
         // Couldn't find it, so it's only in B.
         if (listA.find(listB[i]) == cInvalidIndex)
         {
            if (useOnlyInB)
               pOnlyInB->addSquad(listB[i]);
         }
         // And we already handled the case of being in both lists.
      }
   }
}


//=============================================================================
// BTriggerEffect::teUnitListDiff()
//=============================================================================
void BTriggerEffect::teUnitListDiff()
{
   enum { cListA = 1, cListB = 2, cOnlyInA = 3, cOnlyInB = 4, cInBoth = 5, };
   bool useOnlyInA = getVar(cOnlyInA)->isUsed();
   bool useOnlyInB = getVar(cOnlyInB)->isUsed();
   bool useInBoth = getVar(cInBoth)->isUsed();
   if (!useOnlyInA && !useOnlyInB && !useInBoth)
      return;

   // It's a logic error to use the same variable for two of the outputs because one with bash the other.
   //BTRIGGER_ASSERT(getVar(cOnlyInA) != getVar(cOnlyInB) \
   //   && (getVar(cOnlyInA) != getVar(cInBoth) || !useInBoth) \
   //   && (getVar(cOnlyInB) != getVar(cInBoth) || !useInBoth));

   // Get local copies of our source data.
   BEntityIDArray listA = getVar(cListA)->asUnitList()->readVar();
   BEntityIDArray listB = getVar(cListB)->asUnitList()->readVar();
   long numInA = listA.getNumber();
   long numInB = listB.getNumber();

   // Clear our results lists.
   BTriggerVarUnitList *pOnlyInA = getVar(cOnlyInA)->asUnitList();
   if (useOnlyInA)
      pOnlyInA->clear();
   BTriggerVarUnitList *pOnlyInB = getVar(cOnlyInB)->asUnitList();
   if (useOnlyInB)
      pOnlyInB->clear();
   BTriggerVarUnitList *pInBoth = getVar(cInBoth)->asUnitList();
   if (useInBoth)
      pInBoth->clear();

   // Easiest case:  Both lists empty.
   if (numInA == 0 && numInB == 0)
   {
      return;
   }
   // Easy case:  ListA empty.
   else if (numInA == 0)
   {
      for (long i=0; i<numInB; i++)
      {
         if (useOnlyInB)
            pOnlyInB->addUnit(listB[i]);
         // Halwes - 10/7/2008 - This is incorrect logic since one list is empty none of the entities are in both lists.  If fixing this breaks to much stuff
         //                      we will want to add a flag to change the effect's behavior.
         //if (useInBoth)
         //   pInBoth->addUnit(listB[i]);
      }
   }
   // Easy case:  ListB empty.
   else if (numInB == 0)
   {
      for (long i=0; i<numInA; i++)
      {
         if (useOnlyInA)
            pOnlyInA->addUnit(listA[i]);
         // Halwes - 10/7/2008 - This is incorrect logic since one list is empty none of the entities are in both lists.  If fixing this breaks to much stuff
         //                      we will want to add a flag to change the effect's behavior.
         //if (useInBoth)
         //   pInBoth->addUnit(listA[i]);
      }
   }
   // Both lists have something.
   else
   {
      // Check listA's stuff.
      for (long i=0; i<numInA; i++)
      {
         // Couldn't find it, so it's only in A.
         if (listB.find(listA[i]) == cInvalidIndex)
         {
            if (useOnlyInA)
               pOnlyInA->addUnit(listA[i]);
         }
         // Otherwise, it's in both lists.
         else
         {
            if (useInBoth)
               pInBoth->addUnit(listA[i]);
         }
      }

      // Check listB's stuff.
      for (long i=0; i<numInB; i++)
      {
         // Couldn't find it, so it's only in B.
         if (listA.find(listB[i]) == cInvalidIndex)
         {
            if (useOnlyInB)
               pOnlyInB->addUnit(listB[i]);
         }
         // And we already handled the case of being in both lists.
      }
   }
}

//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teCombatDamage()
{
   switch (getVersion())
   {
      case 1:
         teCombatDamageV1();
         break;

      case 2:
         teCombatDamageV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Initial combat damage effect
//==============================================================================
void BTriggerEffect::teCombatDamageV1()
{
   enum 
   { 
      cSquad     = 1, 
      cSquadList = 2, 
      cUnit      = 3, 
      cUnitList  = 4, 
      cHP        = 5, 
      cSpread    = 6,
   };

   bool useSquad         = getVar(cSquad)->isUsed();
   bool useSquadList     = getVar(cSquadList)->isUsed();
   bool useUnit          = getVar(cUnit)->isUsed();
   bool useUnitList      = getVar(cUnitList)->isUsed();
   bool useSpread        = getVar(cSpread)->isUsed();
   bool spreadAmongUnits = useSpread ? getVar(cSpread)->asBool()->readVar() : false;   

   if (!useSquad && !useSquadList && !useUnit && !useUnitList)
   {
      BTRIGGER_ASSERTM(false, "No targets specified.");
      return;
   }

   BEntityIDArray unitList;
   // If unit list input is used
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   // If unit input is used
   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // If squad input is used
   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 5356
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child = 0; child < numChildren; child++)
         {
            unitList.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   // If squad list input is used
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5357
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long child = 0; child < numChildren; child++)
            {
               unitList.uniqueAdd(pSquad->getChild(child));
            }
         }
      }
   }

   uint numUnits = unitList.getSize();
   if (numUnits <= 0)
   {
      return;
   }

   float hp = getVar(cHP)->asFloat()->readVar();
   if (spreadAmongUnits)
   {
      hp /= numUnits;
   }

   // Iterate through units
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (!pUnit)
      {
         continue;
      }

      if (!pUnit->isAlive())
      {
         continue;
      }

      BDamage totalDamage;
      totalDamage.mDamage = hp;
      totalDamage.mDamageMultiplier = 1.0f;
      totalDamage.mShieldDamageMultiplier = 1.0f;
      totalDamage.mDirectional = false;
      totalDamage.mHalfKill = false;
      totalDamage.mForceThrow = false;

      // [11/5/2008 xemu] flag our associated squad as non-scoring
      if (pUnit->getSquad() != NULL)
      {
         pUnit->getSquad()->setFlagPreventScoring(true);
      }

      pUnit->damage(totalDamage);

      // [11/5/2008 xemu] now re-clear that flag if we didn't actually destroy the squad
      if ((pUnit->getSquad() != NULL) && (pUnit->getSquad()->isAlive()))
      {
         pUnit->getSquad()->setFlagPreventScoring(false);
      }
   }
}

//==============================================================================
// Added OverrideRevive parameter
//==============================================================================
void BTriggerEffect::teCombatDamageV2()
{
   enum 
   { 
      cSquad = 1, 
      cSquadList = 2, 
      cUnit = 3, 
      cUnitList = 4, 
      cHP = 5, 
      cSpread = 6,
      cOverrideRevive = 7,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useSpread = getVar(cSpread)->isUsed();
   bool spreadAmongUnits = useSpread ? getVar(cSpread)->asBool()->readVar() : false;   
   bool overrideRevive = getVar(cOverrideRevive)->isUsed() ? getVar(cOverrideRevive)->asBool()->readVar(): false;

   if (!useSquad && !useSquadList && !useUnit && !useUnitList)
   {
      BTRIGGER_ASSERTM(false, "No targets specified.");
      return;
   }

   BEntityIDArray unitList;
   // If unit list input is used
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   // If unit input is used
   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // If squad input is used
   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 5358
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long child = 0; child < numChildren; child++)
         {
            unitList.uniqueAdd(pSquad->getChild(child));
         }
      }
   }

   // If squad list input is used
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5359
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long child = 0; child < numChildren; child++)
            {
               unitList.uniqueAdd(pSquad->getChild(child));
            }
         }
      }
   }

   uint numUnits = unitList.getSize();
   if (numUnits <= 0)
   {
      return;
   }

   float hp = getVar(cHP)->asFloat()->readVar();
   if (spreadAmongUnits)
   {
      hp /= numUnits;
   }

   // Iterate through units
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (!pUnit)
      {
         continue;
      }

      if (!pUnit->isAlive())
      {
         continue;
      }

      BDamage totalDamage;
      totalDamage.mDamage = hp;
      totalDamage.mDamageMultiplier = 1.0f;
      totalDamage.mShieldDamageMultiplier = 1.0f;
      totalDamage.mDirectional = false;
      totalDamage.mHalfKill = false;
      totalDamage.mForceThrow = false;
      totalDamage.mOverrideRevive = overrideRevive;

      // [11/5/2008 xemu] flag our associated squad as non-scoring
      if (pUnit->getSquad() != NULL)
      {
         pUnit->getSquad()->setFlagPreventScoring(true);
      }

      pUnit->damage(totalDamage);

      // [11/5/2008 xemu] now re-clear that flag if we didn't actually destroy the squad
      if ((pUnit->getSquad() != NULL) && (pUnit->getSquad()->isAlive()))
      {
         pUnit->getSquad()->setFlagPreventScoring(false);
      }
   }
}

//==============================================================================
// BTriggerEffect::teEntityFilterClear()
//==============================================================================
void BTriggerEffect::teEntityFilterClear()
{
   enum { cEntityFilterSet = 1, };
   getVar(cEntityFilterSet)->asEntityFilterSet()->clearFilters();
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddIsAlive()
//==============================================================================
void BTriggerEffect::teEntityFilterAddIsAlive()
{
   enum { cEntityFilterSet = 1, cInvert = 2, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterIsAlive(invertFilter);
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddInList()
//==============================================================================
void BTriggerEffect::teEntityFilterAddInList()
{
   enum { cEntityFilterSet = 1, cUnitList = 2, cSquadList = 3, cInvert = 4, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   if (getVar(cUnitList)->isUsed())
   {
      const BEntityIDArray &unitList = getVar(cUnitList)->asUnitList()->readVar();
      getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterInList(invertFilter, unitList);
   }
   if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray &squadList = getVar(cSquadList)->asSquadList()->readVar();
      getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterInList(invertFilter, squadList);
   }
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddPlayers()
//==============================================================================
void BTriggerEffect::teEntityFilterAddPlayers()
{
   enum { cEntityFilterSet = 1, cPlayerList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   const BPlayerIDArray &playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterPlayers(invertFilter, playerList);
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddTeams()
//==============================================================================
void BTriggerEffect::teEntityFilterAddTeams()
{
   enum { cEntityFilterSet = 1, cTeamList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   const BTeamIDArray &teamList = getVar(cTeamList)->asTeamList()->readVar();
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterTeams(invertFilter, teamList);
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddProtoObjects()
//==============================================================================
void BTriggerEffect::teEntityFilterAddProtoObjects()
{
   enum { cEntityFilterSet = 1, cProtoObjectList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   const BProtoObjectIDArray &protoObjectList = getVar(cProtoObjectList)->asProtoObjectList()->readVar();
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterProtoObjects(invertFilter, protoObjectList);
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddProtoSquads()
//==============================================================================
void BTriggerEffect::teEntityFilterAddProtoSquads()
{
   enum { cEntityFilterSet = 1, cProtoSquadList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   const BProtoSquadIDArray &protoSquadList = getVar(cProtoSquadList)->asProtoSquadList()->readVar();
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterProtoSquads(invertFilter, protoSquadList);
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddObjectTypes()
//==============================================================================
void BTriggerEffect::teEntityFilterAddObjectTypes()
{
   enum { cEntityFilterSet = 1, cObjectTypeList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   const BObjectTypeIDArray &objectTypeList = getVar(cObjectTypeList)->asObjectTypeList()->readVar();
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterObjectTypes(invertFilter, objectTypeList);
}

//==============================================================================
// BTriggerEffect::teEntityFilterAddObjectTypes()
//==============================================================================
void BTriggerEffect::teEntityFilterAddIsSelected()
{
   enum { cEntityFilterSet = 1, cPlayer = 2, cInvert = 3, };

   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   BPlayerID player = getVar(cPlayer)->asPlayer()->readVar();

   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterIsSelected(invertFilter, player);
}


//==============================================================================
// BTriggerEffect::teEntityFilterAddCanChangeOwner()
//==============================================================================
void BTriggerEffect::teEntityFilterAddCanChangeOwner()
{
   enum { cEntityFilterSet = 1, cInvert = 2, };

   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;

   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterCanChangeOwner(invertFilter);
}

//==============================================================================
// BTriggerEffect::teEntityFilterAddJacking()
//==============================================================================
void BTriggerEffect::teEntityFilterAddJacking()
{
   enum { cEntityFilterSet = 1, cInvert = 2, };

   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;

   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterJacking(invertFilter);
}

//==============================================================================
// BTriggerEffect::tePatherObstructionUpdates
//==============================================================================
void BTriggerEffect::tePatherObstructionUpdates()
{
   enum { cEnablePatherUpdates = 1 };

   gPather.enableQuadUpdate(getVar(cEnablePatherUpdates)->asBool()->readVar());
}

//==============================================================================
// BTriggerEffect::tePatherObstructionRebuild
//==============================================================================
void BTriggerEffect::tePatherObstructionRebuild()
{
   enum { cMinVect = 1, cMaxVect = 2 };

   BVector minVect = getVar(cMinVect)->asVector()->readVar();
   BVector maxVect = getVar(cMaxVect)->asVector()->readVar();

   gPather.updatePathingQuad(&gObsManager, minVect, maxVect);
}

//==============================================================================
// BTriggerEffect::teEntityFilterAddDiplomacy()
//==============================================================================
void BTriggerEffect::teEntityFilterAddDiplomacy()
{
   enum 
   { 
      cEntityFilterSet = 1, 
      cRelationType    = 2, 
      cPlayer          = 3,
      cTeam            = 4,
      cInvert          = 5,
   };

   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   BRelationType relationType = getVar(cRelationType)->asRelationType()->readVar();

   BTeamID teamID = -1;
   if (getVar(cPlayer)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5370
      const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
      if (pPlayer)
      {
         teamID = pPlayer->getTeamID();
      }
   }
   else if (getVar(cTeam)->isUsed())
   {
      teamID = getVar(cTeam)->asTeam()->readVar();
   }

   if (teamID == -1)
   {
      BTRIGGER_ASSERTM(false, "No reference team ID found!");
      return;
   }

   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterRelationType(invertFilter, relationType, teamID);
}

//==============================================================================
// BTriggerEffect::teUnitListFilter()
//==============================================================================
void BTriggerEffect::teUnitListFilter()
{
   enum { cSourceUnitList = 1, cEntityFilterSet = 2, cPassedUnitList = 3, cFailedUnitList = 4, };
   bool usePassedUnitList = getVar(cPassedUnitList)->isUsed();
   bool useFailedUnitList = getVar(cFailedUnitList)->isUsed();
   if (!usePassedUnitList && !useFailedUnitList)
      return;

   BEntityIDArray sourceUnitList = getVar(cSourceUnitList)->asUnitList()->readVar();
   BEntityIDArray passedUnits;
   BEntityIDArray failedUnits;
   BEntityFilterSet *pFilter = getVar(cEntityFilterSet)->asEntityFilterSet()->readVar();
   pFilter->filterUnits(sourceUnitList, &passedUnits, &failedUnits, NULL);

   if (usePassedUnitList)
      getVar(cPassedUnitList)->asUnitList()->writeVar(passedUnits);
   if (useFailedUnitList)
      getVar(cFailedUnitList)->asUnitList()->writeVar(failedUnits);

   BTRIGGER_ASSERTM(pFilter->getNumFilters() < 10, "More than 10 filters.  possible filter leak.  FilterAdd* called without FilterClear  ");
   BTRIGGER_ASSERTM(pFilter->getNumFilters() < 50, "More than 50 filters.  major filter leak.  FilterAdd* called without FilterClear  ");
}


//==============================================================================
// BTriggerEffect::teSquadListFilter()
//==============================================================================
void BTriggerEffect::teSquadListFilter()
{
   enum { cSourceSquadList = 1, cEntityFilterSet = 2, cPassedSquadList = 3, cFailedSquadList = 4, };
   bool usePassedSquadList = getVar(cPassedSquadList)->isUsed();
   bool useFailedSquadList = getVar(cFailedSquadList)->isUsed();
   if (!usePassedSquadList && !useFailedSquadList)
      return;

   BEntityIDArray sourceSquadList = getVar(cSourceSquadList)->asSquadList()->readVar();
   BEntityIDArray passedSquads;
   BEntityIDArray failedSquads;
   BEntityFilterSet *pFilter = getVar(cEntityFilterSet)->asEntityFilterSet()->readVar();
   pFilter->filterSquads(sourceSquadList, &passedSquads, &failedSquads, NULL);

   if (usePassedSquadList)
      getVar(cPassedSquadList)->asSquadList()->writeVar(passedSquads);
   if (useFailedSquadList)
      getVar(cFailedSquadList)->asSquadList()->writeVar(failedSquads);

   BTRIGGER_ASSERTM(pFilter->getNumFilters() < 200, "More than 200 filters.  possible filter leak.  FilterAdd* called without FilterClear  ");
   BTRIGGER_ASSERTM(pFilter->getNumFilters() < 300, "More than 300 filters.  major filter leak.  FilterAdd* called without FilterClear  ");

}


//==============================================================================
// BTriggerEffect::teEntityFilterAddRefCount()
//==============================================================================
void BTriggerEffect::teEntityFilterAddRefCount()
{
   enum { cEntityFilterSet = 1, cRefCountType = 2, cCompareType = 3, cCompareCount = 4, cInvert = 5, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   short refCountType = getVar(cRefCountType)->asRefCountType()->readVar();
   refCountType += BEntityRef::cTypeDesignStart;
   BTRIGGER_ASSERT(refCountType >= BEntityRef::cTypeDesignStart && refCountType <= BEntityRef::cTypeDesignEnd);
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   long compareCount = getVar(cCompareCount)->asInteger()->readVar();
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterRefCount(invertFilter, refCountType, compareType, compareCount);
}

//=============================================================================
// BTriggerEffect::teMathFloat
//=============================================================================
void BTriggerEffect::teMathFloat()
{
   enum 
   { 
      cFloat1 = 1, 
      cOp     = 2, 
      cFloat2 = 3, 
      cOutput = 4, 
   };

   float float1 = getVar(cFloat1)->asFloat()->readVar();
   long  op     = getVar(cOp)->asMathOperator()->readVar();
   float float2 = getVar(cFloat2)->asFloat()->readVar();
   float result = BTriggerEffect::calculateValuesFloat(float1, op, float2);

   getVar(cOutput)->asFloat()->writeVar(result);
}

//=============================================================================
// BTriggerEffect::teTeleport
//=============================================================================
void BTriggerEffect::teTeleport()
{
   switch(getVersion())
   {
      case 2:
         teTeleportV2();
         break;

      case 3:
         teTeleportV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "Obsolete version!");
         break;
   }
}

//=============================================================================
// Teleport objects and squads
//=============================================================================
void BTriggerEffect::teTeleportV2()
{
   enum
   { 
      cUnit       = 1, 
      cUnitList   = 2, 
      cSquad      = 3, 
      cSquadList  = 4, 
      cLocation   = 5,
      cObject     = 6,
      cObjectList = 7,      
   };

   bool useUnit       = getVar(cUnit)->isUsed();
   bool useUnitList   = getVar(cUnitList)->isUsed();
   bool useSquad      = getVar(cSquad)->isUsed();
   bool useSquadList  = getVar(cSquadList)->isUsed();
   bool useObject     = getVar(cObject)->isUsed();
   bool useObjectList = getVar(cObjectList)->isUsed();

   if (!useUnit && !useUnitList && !useSquad && !useSquadList && !useObject && !useObjectList)
   {
      BTRIGGER_ASSERTM(false, "No unit, unit list, squad, squad list, object, or object list assigned!");
      return;
   }

   BEntityIDArray unitList;   

   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   BEntityIDArray objectList;
   if (useObjectList)
   {
      objectList = getVar(cObjectList)->asObjectList()->readVar();
   }

   if (useObject)
   {
      objectList.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   // Collect squads
   uint numUnits = unitList.getSize();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5361
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (pSquad)
         {
            squadList.uniqueAdd(pSquad->getID());
         }
      }
   }

   // Find destination locations
   BVector      location = getVar(cLocation)->asVector()->readVar();
   BWorkCommand command;
   command.addWaypoint(location);
   DWORD flags = BSquadPlotter::cSPFlagIgnorePlayerRestriction;
   gSquadPlotter.plotSquads(squadList, &command, flags);
   const BDynamicSimArray<BSquadPlotterResult>& plotterResults = gSquadPlotter.getResults();

   long searchScale = 2;

   // Teleport squads   
   uint numSquads = squadList.getSize();   
   bool usePlot   = false;
   if (plotterResults.getSize() == numSquads)
   {
      usePlot = true;
   }
   for (uint i = 0; i < numSquads; i++)
   {
      if (usePlot)
      {
         location = plotterResults[i].getDesiredPosition();         
      }         

      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (!pSquad)
         continue;

      // Is this squad an army of one?
      bool armyOfOne = false;
//-- FIXING PREFIX BUG ID 5373
      const BArmy* pArmy = pSquad->getParentArmy();
//--
      if (pArmy)
      {
         armyOfOne = (pArmy->getNumberChildren() == 1);
      }
      // Is this squad a platoon of one?
      if (armyOfOne)
      {
//-- FIXING PREFIX BUG ID 5372
         const BPlatoon* pPlatoon = pSquad->getParentPlatoon();
//--
         if (pPlatoon)
         {
            armyOfOne = (pPlatoon->getNumberChildren() == 1);
         }
         else
         {
            armyOfOne = false;
         }                     
      }

      // If not an army of one add the squad to its own army
      if (!armyOfOne)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = pSquad->getPlayerID();
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (!pArmy)
            continue;
         BEntityIDArray teleportSquads;
         teleportSquads.add(pSquad->getID());
         pArmy->addSquads(teleportSquads, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
      }
      
      if (!pSquad->doTeleport(location, searchScale))
      {
         //BTRIGGER_ASSERTM(false, "The squad teleport destination location is invalid and an alternate location could not be determined!");
         return;
      }
   }

   // Teleport objects
   uint numObjects = objectList.getSize();
   for (uint i = 0; i < numObjects; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject && !pObject->teleport(location, searchScale))
      {
         //BTRIGGER_ASSERTM(false, "The squad teleport destination location is invalid and an alternate location could not be determined!");
         return;
      }
   }
}

//=============================================================================
// Teleport objects and squads
//=============================================================================
void BTriggerEffect::teTeleportV3()
{
   enum
   { 
      cSquad = 3, 
      cSquadList = 4, 
      cLocation = 5,
      cObject = 6,
      cObjectList = 7,      
      cIgnorePlot = 8,
   };

   bool useSquad      = getVar(cSquad)->isUsed();
   bool useSquadList  = getVar(cSquadList)->isUsed();
   bool useObject     = getVar(cObject)->isUsed();
   bool useObjectList = getVar(cObjectList)->isUsed();
   bool ignorePlot    = getVar(cIgnorePlot)->isUsed() ? getVar(cIgnorePlot)->asBool()->readVar() : false;

   if (!useSquad && !useSquadList && !useObject && !useObjectList)
   {
      BTRIGGER_ASSERTM(false, "No squad, squad list, object, or object list assigned!");
      return;
   }

   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   BEntityIDArray objectList;
   if (useObjectList)
   {
      objectList = getVar(cObjectList)->asObjectList()->readVar();
   }

   if (useObject)
   {
      objectList.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   uint numObjects = objectList.getSize();
   uint numSquads = squadList.getSize();   
   if ((numObjects == 0) && (numSquads == 0))
      return;

   // Retrieve destination locations
   BVector location = getVar(cLocation)->asVector()->readVar();
   
   long searchScale = 2;
   if (numSquads > 0)
   {
      // Teleport squads         
      bool usePlot = false;      
      BWorkCommand command;
      command.addWaypoint(location);
      DWORD flags = BSquadPlotter::cSPFlagIgnorePlayerRestriction;
      gSquadPlotter.plotSquads(squadList, &command, flags);
      const BDynamicSimArray<BSquadPlotterResult>& plotterResults = gSquadPlotter.getResults();   
      if (!ignorePlot && (plotterResults.getSize() == numSquads))
      {
         usePlot = true;
      }
      for (uint i = 0; i < numSquads; i++)
      {
         if (usePlot)
         {
            location = plotterResults[i].getDesiredPosition();         
         }         

         BSquad* pSquad = gWorld->getSquad(squadList[i]);
         if (!pSquad)
            continue;

         // Is this squad an army of one?
         bool armyOfOne = false;
//-- FIXING PREFIX BUG ID 5377
         const BArmy* pArmy = pSquad->getParentArmy();
//--
         if (pArmy)
         {
            armyOfOne = (pArmy->getNumberChildren() == 1);
         }
         // Is this squad a platoon of one?
         if (armyOfOne)
         {
//-- FIXING PREFIX BUG ID 5376
            const BPlatoon* pPlatoon = pSquad->getParentPlatoon();
//--
            if (pPlatoon)
            {
               armyOfOne = (pPlatoon->getNumberChildren() == 1);
            }
            else
            {
               armyOfOne = false;
            }                     
         }

         // If not an army of one add the squad to its own army
         if (!armyOfOne)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = pSquad->getPlayerID();
            BArmy* pArmy = gWorld->createArmy(objectParms);
            if (!pArmy)
               continue;
            BEntityIDArray teleportSquads;
            teleportSquads.add(pSquad->getID());
            pArmy->addSquads(teleportSquads, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
         }

         if (!pSquad->doTeleport(location, searchScale))
         {
            BTRIGGER_ASSERTM(false, "The squad teleport destination location is invalid and an alternate location could not be determined!  Verify that the destination location is not obstructed!");
            return;
         }
      }
   }

   // Teleport objects   
   for (uint i = 0; i < numObjects; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject && !pObject->teleport(location, searchScale))
      {
         //BTRIGGER_ASSERTM(false, "The squad teleport destination location is invalid and an alternate location could not be determined!");
         return;
      }
   }
}

//==============================================================================
// BTriggerEffect::teEntityFilterAddIsIdle()
//==============================================================================
void BTriggerEffect::teEntityFilterAddIsIdle()
{
   enum 
   { 
      cEntityFilterSet = 1, 
      cInvert = 2, 
   };

   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterIsIdle(invertFilter);
}

//=============================================================================
// Call the correct version of the function
//=============================================================================
void BTriggerEffect::teAsFloat()
{
   switch (getVersion())
   {
      case 2:
         teAsFloatV2();
         break;

      case 3:
         teAsFloatV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//=============================================================================
// Convert value of assigned data type to a float data type
//=============================================================================
void BTriggerEffect::teAsFloatV2()
{
   enum 
   { 
      cCount = 1, 
      cHitpoints = 2,
      cPercent = 3,
      cOutFloat = 4,
      cDistance = 5,
   };

   bool countUsed = getVar(cCount)->isUsed();
   bool hpUsed = getVar(cHitpoints)->isUsed();
   bool percentUsed = getVar(cPercent)->isUsed();
   bool distanceUsed = getVar(cDistance)->isUsed();

   float result = 0.0f;

   if (countUsed)
   {
      result = (float)getVar(cCount)->asInteger()->readVar();
   }
   else if (hpUsed)
   {
      result = getVar(cHitpoints)->asFloat()->readVar();
   }
   else if (percentUsed)
   {
      result = getVar(cPercent)->asFloat()->readVar();
   }
   else if (distanceUsed)
   {
      result = getVar(cDistance)->asFloat()->readVar();
   }

   getVar(cOutFloat)->asFloat()->writeVar(result);
}

//=============================================================================
// Convert value of assigned data type to a float data type
//=============================================================================
void BTriggerEffect::teAsFloatV3()
{
   enum 
   { 
      cCount = 1, 
      cOutFloat = 4,
   };

   bool countUsed = getVar(cCount)->isUsed();

   float result = 0.0f;

   if (countUsed)
   {
      result = (float)getVar(cCount)->asInteger()->readVar();
   }

   getVar(cOutFloat)->asFloat()->writeVar(result);
}

//XXXHalwes - 7/17/2007 - Valid?
//=============================================================================
// BTriggerEffect::teSettle
//=============================================================================
void BTriggerEffect::teSettle()
{
   enum
   {
      cUnit      = 1,
      cUnitList  = 2,
      cSquad     = 3,
      cSquadList = 4,
   };

   bool unitUsed      = getVar(cUnit)->isUsed();
   bool unitListUsed  = getVar(cUnitList)->isUsed();
   bool squadUsed     = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No unit(s)/squad(s) assigned!");
      return;
   }

   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   BEntityIDArray squadList;
   if (squadListUsed)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (squadUsed)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   // Collect squads
   long numUnits = unitList.getNumber();
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5365
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (pSquad)
         {
            squadList.uniqueAdd(pSquad->getID());
         }
      }
   }

   // Settle squads
   long numSquads = squadList.getNumber();
   for (long i = 0; i < numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         pSquad->settle();
         pSquad->removeAllOrders();
         pSquad->removeActions();
         BUnit* pLeaderUnit = pSquad->getLeaderUnit();
         if (pLeaderUnit)
         {
            BVector pos = pLeaderUnit->getPosition();
            pSquad->setPosition(pos);
            pSquad->setLeashPosition(pos);
            pSquad->setTurnRadiusPos(pos);
            pSquad->updateObstruction();
         }
      }
   }
}



//==============================================================================
// Set unit flag to ignore user input
//==============================================================================
void BTriggerEffect::teSetIgnoreUserInput()
{
   switch(getVersion())
   {
      case 1:
         teSetIgnoreUserInputV1();
         break;

      case 2:
         teSetIgnoreUserInputV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Set unit flag to ignore user input
//==============================================================================
void BTriggerEffect::teSetIgnoreUserInputV1()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cEnable = 5,
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, or SquadList assigned!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5367
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            long numChildren = pSquad->getNumberChildren();
            for (long j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5368
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         long numChildren = pSquad->getNumberChildren();
         for (long i = 0; i < numChildren; i++)
         {
            unitList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Set ignore user input flag on collected units
   long numUnits = unitList.getNumber();
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         pUnit->setIgnoreUserInput(getVar(cEnable)->asBool()->readVar());
      }
   }
}

//==============================================================================
// Set unit flag to ignore user input
//==============================================================================
void BTriggerEffect::teSetIgnoreUserInputV2()
{
   enum
   {
      cSquad = 3,
      cSquadList = 4,
      cEnable = 5,
   };

   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, or SquadList assigned!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = squadList.getSize();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5369
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5371
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            unitList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Set ignore user input flag on collected units
   uint numUnits = unitList.getSize();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         pUnit->setIgnoreUserInput(getVar(cEnable)->asBool()->readVar());
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teSetPlayableBounds()
{
   enum {  cCorner1=1, cCorner2=2, };

   BVector c1=getVar(cCorner1)->asVector()->readVar();
   BVector c2=getVar(cCorner2)->asVector()->readVar();

   float minX, minZ, maxX, maxZ;

   if (c1.x<c2.x)
   {
      minX=c1.x;
      maxX=c2.x;
   }
   else
   {
      minX=c2.x;
      maxX=c1.x;
   }

   if (c1.z<c2.z)
   {
      minZ=c1.z;
      maxZ=c2.z;
   }
   else
   {
      minZ=c2.z;
      maxZ=c1.z;
   }

   gWorld->setSimBounds(minX, minZ, maxX, maxZ);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teResetBlackMap()
{
   for (int i=1; i<gWorld->getNumberTeams(); i++)
      gVisibleMap.resetBlackMap(i);

   //if (!gConfig.isDefined(cConfigFlashGameUI))
   //   gMiniMap.reset();
   //else
   gUIManager->resetMinimap();
}

//==============================================================================
// Enable/Disable fog of war
//==============================================================================
void BTriggerEffect::teEnableFogOfWar()
{
   enum { cEnable = 1, };
   bool useFog = getVar(cEnable)->asBool()->readVar();
   bool useSkirt = (useFog ? false : true);
   bool visualOnly = false;
   gWorld->setFogOfWar(useFog, useSkirt, visualOnly);
}

//==============================================================================
// Change the squad(s) mode
//==============================================================================
void BTriggerEffect::teChangeSquadMode()
{
   switch (getVersion())
   {
      case 1:
         teChangeSquadModeV1();
         break;

      case 2:
         teChangeSquadModeV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Change the squad(s) mode
//==============================================================================
void BTriggerEffect::teChangeSquadModeV1()
{
   enum
   {
      cSquad = 1,
      cSquadList = 2,
      cMode = 3,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   uint numSquads = squadList.getSize();
   if (numSquads == 0)
   {
      return;
   }

   long squadMode = getVar(cMode)->asSquadMode()->readVar();

   // Group squads per player
   // MPB [8/27/2008] - Added the "filterOutSpartanHijackedEntities" function to remove hijacked vehicles from the list
   // of entities to command.  This is a hack to work around the fact that there is no player ID validation on the set
   // of entities that the trigger script wants to command, and because hijacking will change the playerID on a unit/squad.
   // This fixes bug PHX-9580.
   BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(squadList, filterOutSpartanHijackedEntities);
   uint numPlayerEntityIDs = playerEntityIDs.getSize();
   for (uint i = 0; i < numPlayerEntityIDs; i++)
   {
      uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
      if (numEntities > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            // Platoon the squads
            pArmy->addSquads(playerEntityIDs[i].mEntityIDs, !gConfig.isDefined(cConfigClassicPlatoonGrouping));

            // Setup the command
            BWorkCommand tempCommand;
            tempCommand.setSquadMode(squadMode);

            // Set the command to be from a trigger to the army
            tempCommand.setRecipientType(BCommand::cArmy);
            tempCommand.setSenderType(BCommand::cTrigger);

            // Give the command to the army.
            pArmy->queueOrder(&tempCommand);
         }               
      }
   }
}

//==============================================================================
// Change the squad(s) mode
//==============================================================================
void BTriggerEffect::teChangeSquadModeV2()
{
   enum
   {
      cSquad = 1,
      cSquadList = 2,
      cMode = 3,
      cInSetDirectly = 4,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   // Collect squads
   BEntityIDArray squadList;
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   uint numSquads = squadList.getSize();
   if (numSquads == 0)
   {
      return;
   }

   long squadMode = getVar(cMode)->asSquadMode()->readVar();
   
   // Halwes - 9/30/2008 - Added for specific hack to support Scn07 Scarab, not recommended for normal use
   bool setDirectly = getVar(cInSetDirectly)->isUsed() ? getVar(cInSetDirectly)->asBool()->readVar() : false;
   if (setDirectly)
   {
      for (uint i = 0; i < numSquads; i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            BSquadAI* pSquadAI = pSquad->getSquadAI();
            if (pSquadAI)
            {
               pSquadAI->setMode(squadMode);
            }
         }
      }
   }
   else 
   {
      // Group squads per player
      // MPB [8/27/2008] - Added the "filterOutSpartanHijackedEntities" function to remove hijacked vehicles from the list
      // of entities to command.  This is a hack to work around the fact that there is no player ID validation on the set
      // of entities that the trigger script wants to command, and because hijacking will change the playerID on a unit/squad.
      // This fixes bug PHX-9580.
      BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(squadList, filterOutSpartanHijackedEntities);
      uint numPlayerEntityIDs = playerEntityIDs.getSize();
      for (uint i = 0; i < numPlayerEntityIDs; i++)
      {
         uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
         if (numEntities > 0)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
            BArmy* pArmy = gWorld->createArmy(objectParms);
            if (pArmy)
            {
               // Platoon the squads
               pArmy->addSquads(playerEntityIDs[i].mEntityIDs, !gConfig.isDefined(cConfigClassicPlatoonGrouping));

               // Setup the command
               BWorkCommand tempCommand;
               tempCommand.setSquadMode(squadMode);

               // Set the command to be from a trigger to the army
               tempCommand.setRecipientType(BCommand::cArmy);
               tempCommand.setSenderType(BCommand::cTrigger);

               // Give the command to the army.
               pArmy->queueOrder(&tempCommand);
            }               
         }
      }
   }
}

//===================================
// Call the correct version
//===================================
void BTriggerEffect::teGetAmmo()
{
   switch (getVersion())
   {
      case 1:
         teGetAmmoV1();
         break;

      case 2:
         teGetAmmoV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//===================================
// Get the ammo amount from the unit
//===================================
void BTriggerEffect::teGetAmmoV1()
{
   enum
   {
      cUnit = 1,
      cOutAmmo = 2,
      cOutAmmoPercent = 3,
   };

   bool ammoUsed = getVar(cOutAmmo)->isUsed();
   bool ammoPercentUsed = getVar(cOutAmmoPercent)->isUsed();

   if (!ammoUsed && !ammoPercentUsed)
   {
      BTRIGGER_ASSERTM(false, "No Ammo or AmmoPercent output variable assigned!" );
      return;
   }

   BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
   if( !pUnit )
   {
      return;
   }

   if (ammoUsed)
   {
      getVar(cOutAmmo)->asFloat()->writeVar(pUnit->getAmmunition());
   }

   if (ammoPercentUsed)
   {
      getVar(cOutAmmoPercent)->asFloat()->writeVar(pUnit->getAmmoPercentage());
   }
}

//============================================
// Get the ammo amount from the unit or squad
//============================================
void BTriggerEffect::teGetAmmoV2()
{
   enum
   {
      cUnit = 1,
      cOutAmmo = 2,
      cOutAmmoPercent = 3,
      cSquad = 4,
   };

   bool ammoUsed = getVar(cOutAmmo)->isUsed();
   bool ammoPercentUsed = getVar(cOutAmmoPercent)->isUsed();

   if (!ammoUsed && !ammoPercentUsed)
   {
      BTRIGGER_ASSERTM(false, "No Ammo or AmmoPercent output variable assigned!" );
      return;
   }

   float ammo = 0.0f;
   float ammoPercent = 0.0f;
   if (getVar(cUnit)->isUsed())
   {
      BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
      if (!pUnit)
      {
         return;
      }

      ammo = pUnit->getAmmunition();
      ammoPercent = pUnit->getAmmoPercentage();
   }
   else if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5379
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (!pSquad)
      {
         return;
      }

      ammoPercent = pSquad->getAmmoPercentage();
      ammo = ammoPercent * pSquad->getAmmoMax();      
   }

   if (ammoUsed)
   {
      getVar(cOutAmmo)->asFloat()->writeVar(ammo);
   }

   if (ammoPercentUsed)
   {
      getVar(cOutAmmoPercent)->asFloat()->writeVar(ammoPercent);
   }
}

//=================================
// Call the correct version
//=================================
void BTriggerEffect::teSetAmmo()
{
   switch (getVersion())
   {
      case 1:
         teSetAmmoV1();
         break;

      case 2:
         teSetAmmoV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//=================================
// Set the ammo amount on the unit
//=================================
void BTriggerEffect::teSetAmmoV1()
{
   enum
   {
      cUnit = 1,
      cAmmo = 2,
      cAmmoPercent = 3,
   };

   bool ammoUsed = getVar(cAmmo)->isUsed();
   bool ammoPercentUsed = getVar(cAmmoPercent)->isUsed();

   if (!ammoUsed && !ammoPercentUsed)
   {
      BTRIGGER_ASSERTM(false, "No Ammo or AmmoPercent variable assigned!" );
      return;
   }

   BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
   if( !pUnit )
   {
      return;
   }

   if (ammoUsed)
   {
      pUnit->setAmmunition(getVar(cAmmo)->asFloat()->readVar());
   }
   else if (ammoPercentUsed)
   {
      pUnit->setAmmunition(pUnit->getAmmoMax() * getVar(cAmmoPercent)->asFloat()->readVar());
   }
}

//==========================================
// Set the ammo amount on the unit or squad
//==========================================
void BTriggerEffect::teSetAmmoV2()
{
   enum
   {
      cUnit = 1,
      cAmmo = 2,
      cAmmoPercent = 3,
      cSquad = 4,
   };

   bool ammoUsed = getVar(cAmmo)->isUsed();
   bool ammoPercentUsed = getVar(cAmmoPercent)->isUsed();

   if (!ammoUsed && !ammoPercentUsed)
   {
      BTRIGGER_ASSERTM(false, "No Ammo or AmmoPercent variable assigned!" );
      return;
   }
   
   if (getVar(cUnit)->isUsed())
   {
      BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
      if( !pUnit )
      {
         return;
      }

      if (ammoUsed)
      {
         pUnit->setAmmunition(getVar(cAmmo)->asFloat()->readVar());
      }
      else if (ammoPercentUsed)
      {
         pUnit->setAmmunition(pUnit->getAmmoMax() * getVar(cAmmoPercent)->asFloat()->readVar());
      }
   }
   else if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5380
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (!pSquad)
      {
         return;
      }

      float ammoPercent = 0.0f;
      if (ammoUsed)
      {
         if (pSquad->getAmmoMax() < cFloatCompareEpsilon)
         {
            BTRIGGER_ASSERTM(false, "Trying to set ammo on a squad that has a max ammo of 0!");
            return;
         }
         ammoPercent = getVar(cAmmo)->asFloat()->readVar() / pSquad->getAmmoMax();
      }
      else if (ammoPercentUsed)
      {
         ammoPercent = getVar(cAmmoPercent)->asFloat()->readVar();
      }

      uint numUnits = pSquad->getNumberChildren();
      for (uint i = 0; i < numUnits; i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
         {
            pUnit->setAmmunition(pUnit->getAmmoMax() * ammoPercent);
         }
      }
   }
   else
   {
      BTRIGGER_ASSERTM(false, "No unit or squad is assigned!");
      return;
   }
}

//==================================
// Launch an "other" trigger script
//==================================
void BTriggerEffect::teLaunchScript()
{
   switch (getVersion())
   {
   case 4:
      teLaunchScriptV4();
      break;
   default:
      BTRIGGER_ASSERTM( false, "This version is not supported!");
      break;
   }   
}


//==================================
//==================================
void BTriggerEffect::teLaunchScriptV4()
{
   enum
   {      
      cExternalUnit = 2,
      cExternalUnitList = 3,
      cExternalSquad = 4,
      cExternalSquadList = 5,
      cExternalPlayer = 6,
      cExternalPowerID = 7,
      cExternalCost = 8,
      cExternalLocation = 11,
      cExternalLocationList = 12,
      cExternalFloat = 13,
      cScriptName = 15,
   };

   const BSimString& scriptName = getVar(cScriptName)->asString()->readVar();
   BTriggerScriptID newTriggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, scriptName);
   if (newTriggerScriptID == cInvalidTriggerScriptID)
      return;

   if (getVar(cExternalUnit)->isUsed())
   {
      gTriggerManager.addExternalUnitID(newTriggerScriptID, getVar(cExternalUnit)->asUnit()->readVar());
   }

   if (getVar(cExternalUnitList)->isUsed())
   {
      gTriggerManager.addExternalUnitIDs(newTriggerScriptID, getVar(cExternalUnitList)->asUnitList()->readVar());
   }

   if (getVar(cExternalSquad)->isUsed())
   {
      gTriggerManager.addExternalSquadID(newTriggerScriptID, getVar(cExternalSquad)->asSquad()->readVar());
   }

   if (getVar(cExternalSquadList)->isUsed())
   {
      gTriggerManager.addExternalSquadIDs(newTriggerScriptID, getVar(cExternalSquadList)->asSquadList()->readVar());
   }

   if (getVar(cExternalPlayer)->isUsed())
   {
      gTriggerManager.addExternalPlayerID(newTriggerScriptID, getVar(cExternalPlayer)->asPlayer()->readVar());
   }

   if (getVar(cExternalPowerID)->isUsed())
   {
      gTriggerManager.addExternalProtoPowerID(newTriggerScriptID, getVar(cExternalPowerID)->asPower()->readVar());
   }

   if (getVar(cExternalCost)->isUsed())
   {
      gTriggerManager.addExternalCost(newTriggerScriptID, getVar(cExternalCost)->asCost()->readVar());
   }

   if (getVar(cExternalLocation)->isUsed())
   {
      gTriggerManager.addExternalLocation(newTriggerScriptID, getVar(cExternalLocation)->asVector()->readVar());
   }

   if (getVar(cExternalLocationList)->isUsed())
   {
      gTriggerManager.addExternalLocationList(newTriggerScriptID, getVar(cExternalLocationList)->asVectorList()->readVar());
   }

   if (getVar(cExternalFloat)->isUsed())
   {
      gTriggerManager.addExternalFloat(newTriggerScriptID, getVar(cExternalFloat)->asFloat()->readVar());
   }

   gTriggerManager.activateTriggerScript(newTriggerScriptID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPlayerTeam()
{
   enum { cInputPlayer = 1, cOutputTeam = 2, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cInputPlayer)->asPlayer()->readVar());
   if (!pPlayer)
      return;
   // Write out the team.
   getVar(cOutputTeam)->asTeam()->writeVar(pPlayer->getTeamID());
}


//==============================================================================
// Modify a unit's data scalar variable
//==============================================================================
void BTriggerEffect::teModifyDataScalar()
{
   switch (getVersion())
   {
      case 1:
         teModifyDataScalarV1();
         break;

      case 2:
         teModifyDataScalarV2();
         break;

      default:
         BTRIGGER_ASSERTM( false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Modify a unit's data scalar variable
//==============================================================================
void BTriggerEffect::teModifyDataScalarV1()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4, 
      cDataScalar = 5,
      cPercent = 6,
      cAdjust = 7,
      //cNegativeAdjust = 8, ?
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();   

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM( false, "No unit, unit list, squad, or squad list assigned!");
      return;
   }

   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5381
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5382
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            unitList.uniqueAdd(pSquad->getChild(j));
         }
      }      
   }

   float percent = getVar(cPercent)->asFloat()->readVar();
   bool adjust = getVar(cAdjust)->isUsed() ? getVar(cAdjust)->asBool()->readVar() : false;

   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         switch (getVar(cDataScalar)->asDataScalar()->readVar())
         {
         case BTriggerVarDataScalar::cDataScalarAccuracy:
            if (adjust)
            {
               pUnit->adjustAccuracyScalar(percent);
            }
            else
            {
               pUnit->setAccuracyScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarWorkRate:
            if (adjust)
            {
               pUnit->adjustWorkRateScalar(percent);
            }
            else
            {
               pUnit->setWorkRateScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarDamage:
            if (adjust)
            {
               pUnit->adjustDamageModifier(percent);
            }
            else
            {
               pUnit->setDamageModifier(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarLOS:
            if (adjust)
            {
               pUnit->adjustLOSScalar(percent);
            }
            else
            {
               pUnit->setLOSScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarVelocity:
            if (adjust)
            {
               pUnit->adjustVelocityScalar(percent);
            }
            else
            {
               pUnit->setVelocityScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarWeaponRange:
            if (adjust)
            {
               pUnit->adjustWeaponRangeScalar(percent);
            }
            else
            {
               pUnit->setWeaponRangeScalar(percent);
            }
            break;
         }
      }
   }
}

//==============================================================================
// Modify a unit's data scalar variable
//==============================================================================
void BTriggerEffect::teModifyDataScalarV2()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,       
      cPercent = 6,
      cAdjust = 7,
      cDataScalar = 8,
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();   

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM( false, "No unit, unit list, squad, or squad list assigned!");
      return;
   }

   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5384
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5385
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            unitList.uniqueAdd(pSquad->getChild(j));
         }
      }      
   }

   float percent = getVar(cPercent)->asFloat()->readVar();
   bool adjust = getVar(cAdjust)->isUsed() ? getVar(cAdjust)->asBool()->readVar() : false;

   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         switch (getVar(cDataScalar)->asDataScalar()->readVar())
         {
         case BTriggerVarDataScalar::cDataScalarAccuracy:
            if (adjust)
            {
               pUnit->adjustAccuracyScalar(percent);
            }
            else
            {
               pUnit->setAccuracyScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarWorkRate:
            if (adjust)
            {
               pUnit->adjustWorkRateScalar(percent);
            }
            else
            {
               pUnit->setWorkRateScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarDamage:
            if (adjust)
            {
               pUnit->adjustDamageModifier(percent);
            }
            else
            {
               pUnit->setDamageModifier(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarDamageTaken:
            if (adjust)
            {
               pUnit->adjustDamageTakenScalar(percent);
            }
            else
            {
               pUnit->setDamageTakenScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarLOS:
            if (adjust)
            {
               pUnit->adjustLOSScalar(percent);
            }
            else
            {
               pUnit->setLOSScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarVelocity:
            if (adjust)
            {
               pUnit->adjustVelocityScalar(percent);
            }
            else
            {
               pUnit->setVelocityScalar(percent);
            }
            break;

         case BTriggerVarDataScalar::cDataScalarWeaponRange:
            if (adjust)
            {
               pUnit->adjustWeaponRangeScalar(percent);
            }
            else
            {
               pUnit->setWeaponRangeScalar(percent);
            }
            break;
         }
      }
   }
}


//XXXHalwes - 7/17/2007 - This justs sets a flag, should we consolidate to the flag set trigger or have individual triggers and remove the
//                        flag set trigger.
// DMG - 4/29/08 - Is this even relevant to the new cloaking mechanism?
//==============================================================================
// Cloak a unit(s)/squad(s)
//==============================================================================
void BTriggerEffect::teCloak()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cCloak = 5,
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM( false, "No unit, unit list, squad, or squad list assigned!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5386
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5387
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            unitList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Cloak units
   /*
   bool cloak = getVar(cCloak)->asBool()->readVar();
   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 5388
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         pSquad->setFlagCloaked(cloak);
      }
   }
   */
}


//XXXHalwes - 7/17/2007 - Do we need both this and teGetPlayers?
//==============================================================================
//  Select the correct version
//==============================================================================
void BTriggerEffect::teGetPlayers2()
{
   switch (getVersion())
   {
      case 2:
         teGetPlayers2V2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Get a player list
//==============================================================================
void BTriggerEffect::teGetPlayers2V2()
{
   enum 
   { 
      cOutputPlayerList = 1, 
      cIncludeGaia = 2, 
      cIncludeSelf = 3,       
      cTestPlayer = 5, 
      cRelationType = 6,
      cPlayerState = 7, 
   };

   bool includeGaia = getVar(cIncludeGaia)->asBool()->readVar();
   bool includeSelf = getVar(cIncludeSelf)->asBool()->readVar();
   bool usePlayerState = getVar(cPlayerState)->isUsed();
   BPlayerID testPlayerID = getVar(cTestPlayer)->asPlayer()->readVar();
   BRelationType relationType = getVar(cRelationType)->asRelationType()->readVar();

   BPlayerIDArray foundPlayers;
   int numPlayers = gWorld->getNumberPlayers();
   for (int i = 0; i < numPlayers; i++)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(i);
      if (!pPlayer)
         continue;
      if (!includeGaia && pPlayer->isGaia())
         continue;
      if (!includeSelf && (pPlayer->getID() == testPlayerID))
         continue;
      if (usePlayerState)
      {
         BPlayerState playerState = getVar(cPlayerState)->asPlayerState()->readVar();
         if (pPlayer->getPlayerState() != playerState)
            continue;
      }
      if ((relationType == cRelationTypeAlly) && !pPlayer->isAlly(testPlayerID))
         continue;
      if ((relationType == cRelationTypeEnemy) && !pPlayer->isEnemy(testPlayerID))
         continue;
      if ((relationType == cRelationTypeNeutral) && !pPlayer->isNeutral(testPlayerID))
         continue;

      foundPlayers.add(pPlayer->getID());
   }

   getVar(cOutputPlayerList)->asPlayerList()->writeVar(foundPlayers);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayersToTeams()
{
   enum { cInputPlayerList = 1, cOutputTeamList = 2, };
   BTriggerVarTeamList* pTeamList = getVar(cOutputTeamList)->asTeamList();
   pTeamList->clear();

   const BPlayerIDArray& playerList = getVar(cInputPlayerList)->asPlayerList()->readVar();
   uint numPlayers = playerList.getSize();
   for (uint i=0; i<numPlayers; i++)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(playerList[i]);
      if (pPlayer)
         pTeamList->addTeam(pPlayer->getTeamID());
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teTeamsToPlayers()
{
   enum { cInputTeamList = 1, cOutputPlayerList = 2, };
   BTriggerVarPlayerList* pPlayerList = getVar(cOutputPlayerList)->asPlayerList();
   pPlayerList->clear();

   const BTeamIDArray& teamList = getVar(cInputTeamList)->asTeamList()->readVar();
   uint numTeams = teamList.getSize();
   for (uint i=0; i<numTeams; i++)
   {
      const BTeam* pTeam = gWorld->getTeam(teamList[i]);
      if (pTeam)
      {
         long numPlayersOnTeam = pTeam->getNumberPlayers();
         for (long p=0; p<numPlayersOnTeam; p++)
            pPlayerList->addPlayer(pTeam->getPlayerID(p));
      }
   }
}

//=========================
// Create a blocker object
//=========================
void BTriggerEffect::teBlocker()
{
   enum
   {
      cPlayer = 5,
      cLocation = 2,
      cLOS = 3,
      cLifespan = 6,
      cRevealAlly = 4,
      cCreatedBlocker = 7,
      cAddToObjectList = 8,
      cClearExisting = 9,
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   BVector location = getVar(cLocation)->asVector()->readVar();
   float los = getVar(cLOS)->asFloat()->readVar();
   BObject* pBlockerObject = NULL;
   if (getVar(cLifespan)->isUsed())
   {
      DWORD lifespan = getVar(cLifespan)->asTime()->readVar();
      pBlockerObject = gWorld->createBlocker(playerID, location, los, lifespan);
   }
   else
   {
      pBlockerObject = gWorld->createBlocker(playerID, location, los);
   }

   if (pBlockerObject)
   {
      bool revealAlly = getVar(cRevealAlly)->isUsed() ? getVar(cRevealAlly)->asBool()->readVar() : false;
      pBlockerObject->setFlagNoReveal(!revealAlly);

      if (getVar(cCreatedBlocker)->isUsed())
      {
         getVar(cCreatedBlocker)->asObject()->writeVar(pBlockerObject->getID());
      }

      if (getVar(cAddToObjectList)->isUsed())
      {
         BTriggerVarObjectList* pVarObjectList = getVar(cAddToObjectList)->asObjectList();
         bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;
         if (clearExisting)
         {
            pVarObjectList->clear();
         }

         pVarObjectList->addObject(pBlockerObject->getID());
      }
   }
   else
   {
      if (getVar(cCreatedBlocker)->isUsed())
      {
         getVar(cCreatedBlocker)->asObject()->writeVar(cInvalidObjectID);
      }
   }
}

//XXXHalwes - 7/17/2007 - This is specific to a support power so is this still valid?
//==============================================================================
// Set a sensor lock on a squad
//==============================================================================
void BTriggerEffect::teSensorLock()
{
   enum
   {
      cSquad = 1,
      cSquadList = 2,
      cEnable = 3,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No squad or squad list assigned!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5389
         const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }      
   }

   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 5390
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            unitList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Set sensor lock
   bool enable = getVar(cEnable)->asBool()->readVar();
   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
         pUnit->setFlagSensorLockTagged(enable);  
         /*
         // Units tagged with sensor lock cannot cloak
         bool cloaked = pUnit->getFlagCloaked();
         if (enable && cloaked)
         {
            pUnit->setFlagCloakDetected(true);
         }
         else if (!enable && cloaked)
         {
            pUnit->setFlagCloakDetected(false);
         }
         */
      }
   }
}

//====================================
// BTriggerEffect::teDesignFindSphere
//====================================
void BTriggerEffect::teDesignFindSphere()
{
   enum
   {
      cValueType = 6,
      cOperator = 5,
      cValue = 1,
      cLocationList = 4,

   };

   long valueType = 0;
   bool bDoCompare = false;
   long op;
   float value = 0;
   if (getVar(cValueType)->isUsed())
   {
      valueType = getVar(cValueType)->asInteger()->readVar();
   }
   if (getVar(cOperator)->isUsed())
   {
      op = getVar(cOperator)->asOperator()->readVar();
      if (getVar(cValue)->isUsed())
      {
         value = getVar(cValue)->asFloat()->readVar();
         bDoCompare = true;
      }
   }

   if (getVar(cLocationList)->isUsed())
   {
      BTriggerVarVectorList *pOutputList = getVar(cLocationList)->asVectorList();
      pOutputList->clear();

      uint sphereCount = gWorld->getDesignObjectManager()->getDesignSphereCount();


      for (uint i=0; i<sphereCount; i++)
      {
//-- FIXING PREFIX BUG ID 5397
         const BDesignSphere& sphere = gWorld->getDesignObjectManager()->getDesignSphere(i);
//--

         if(sphere.mDesignData.mChokePoint > value)
         {
            pOutputList->addVector(sphere.mPosition);
         }
      }
   }
}

//==============================================================================
// Reinforce a squad's missing units
//==============================================================================
void BTriggerEffect::teReinforceSquad()
{
   switch (getVersion())
   {
      case 1:
         teReinforceSquadV1();
         break;

      case 2:
         teReinforceSquadV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Reinforce a squad's missing units
//==============================================================================
void BTriggerEffect::teReinforceSquadV1()
{
   enum
   {
      cSquad = 1,
      cSquadList = 2,
      cFlyInStart = 3,
      cFlyInEnd = 4,
   };

   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No squad or squad list assigned!");
      return;
   }

   // Collect squads to reinforce
   BEntityIDArray reinforceSquadList;
   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5391
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);         
//--
         if (pSquad)
         {
            const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
            // Don't bother adding the squad if it doesn't need to be reinforced
            if (pProtoSquad && (pSquad->getNumberChildren() != (uint)pProtoSquad->getSquadSize()))
            {
               reinforceSquadList.uniqueAdd(squadList[i]);
            }
         }            
      }
   }

   if (squadUsed)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
//-- FIXING PREFIX BUG ID 5392
      const BSquad* pSquad = gWorld->getSquad(squadID);
//--
      if (pSquad)
      {
         const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
         // Don't bother adding the squad if it doesn't need to be reinforced
         if (pProtoSquad && (pSquad->getNumberChildren() != (uint)pProtoSquad->getSquadSize()))
         {
            reinforceSquadList.uniqueAdd(squadID);
         }
      }
   }

   // Get fly-in data
   BVector flyInStart = getVar(cFlyInStart)->asVector()->readVar();
   BVector flyInEnd = getVar(cFlyInEnd)->asVector()->readVar();

   // Create reinforcement squads for transport
   uint numReinforceSquads = reinforceSquadList.getSize();
   for (uint i = 0; i < numReinforceSquads; i++)
   {
//-- FIXING PREFIX BUG ID 5393
      const BSquad* pSquad = gWorld->getSquad(reinforceSquadList[i]);
//--
      if (!pSquad)
      {
         continue;
      }
      const BProtoObject* pProtoObject = pSquad->getProtoObject();
      if (!pProtoObject)
      {
         continue;
      }

      // Get data from reinforce squad
      BVector dropOffPos = pSquad->getPosition();
      BVector dropOffForward = pSquad->getForward();
      BVector dropOffRight = pSquad->getRight();
      BPlayerID playerID = pSquad->getPlayerID();
      BEntityID reinforceID = pSquad->getID();

//-- FIXING PREFIX BUG ID 5400
      const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
      if (!pPlayer)
      {
         continue;
      }

      // Create a reinforcement squad based on the depleted squad for transport
      BObjectCreateParms objectParms;
      objectParms.mPlayerID = playerID;
      objectParms.mStartBuilt = true;
      objectParms.mNoTieToGround = !pSquad->getFlagTiesToGround();
      objectParms.mPhysicsReplacement = false;
      objectParms.mPosition = flyInStart;
      objectParms.mForward = dropOffForward;
      objectParms.mRight = dropOffRight;
      objectParms.mProtoSquadID = pSquad->getProtoSquadID();
      // Ignore pop because we are reinforcing a squad that already exists
      objectParms.mIgnorePop = true; 
//-- FIXING PREFIX BUG ID 5401
      const BSquad* pReinforcementsSquad = gWorld->createSquad(objectParms);
//--
      if (!pReinforcementsSquad)
      {
         continue;
      }            

      // Create an order
      BSimOrder* pOrder = gSimOrderManager.createOrder();
      // Init order
      if (pOrder)  
      {
         pOrder->setPriority(BSimOrder::cPriorityTrigger);
      }

      BSquadActionReinforce::reinforceSquad(pOrder, reinforceID, pReinforcementsSquad->getID(), flyInStart, flyInEnd);
   }
}

//==============================================================================
// Reinforce a squad's missing units
//==============================================================================
void BTriggerEffect::teReinforceSquadV2()
{
   enum
   {
      cSquad = 1,
      cSquadList = 2,
      cFlyInStart = 3,
      cFlyInEnd = 4,
   };

   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No squad or squad list assigned!");
      return;
   }

   // Collect squads to reinforce
   BEntityIDArray reinforceSquadList;
   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5322
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);         
//--
         if (pSquad)
         {
            const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
            // Don't bother adding the squad if it doesn't need to be reinforced
            if (pProtoSquad && (pSquad->getNumberChildren() != (uint)pProtoSquad->getSquadSize()))
            {
               reinforceSquadList.uniqueAdd(squadList[i]);
            }
         }            
      }
   }

   if (squadUsed)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
//-- FIXING PREFIX BUG ID 5324
      const BSquad* pSquad = gWorld->getSquad(squadID);
//--
      if (pSquad)
      {
         const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
         // Don't bother adding the squad if it doesn't need to be reinforced
         if (pProtoSquad && (pSquad->getNumberChildren() != (uint)pProtoSquad->getSquadSize()))
         {
            reinforceSquadList.uniqueAdd(squadID);
         }
      }
   }

   // Get fly-in data
   BVector flyInStart = getVar(cFlyInStart)->isUsed() ? getVar(cFlyInStart)->asVector()->readVar() : cInvalidVector;
   BVector flyInEnd = getVar(cFlyInEnd)->isUsed() ? getVar(cFlyInEnd)->asVector()->readVar() : cInvalidVector;

   // Create reinforcement squads for transport
   uint numReinforceSquads = reinforceSquadList.getSize();
   for (uint i = 0; i < numReinforceSquads; i++)
   {
//-- FIXING PREFIX BUG ID 5327
      const BSquad* pSquad = gWorld->getSquad(reinforceSquadList[i]);
//--
      if (!pSquad)
      {
         continue;
      }
      const BProtoObject* pProtoObject = pSquad->getProtoObject();
      if (!pProtoObject)
      {
         continue;
      }

      // Get data from reinforce squad
      BVector dropOffPos = pSquad->getPosition();
      BVector dropOffForward = pSquad->getForward();
      BVector dropOffRight = pSquad->getRight();
      BPlayerID playerID = pSquad->getPlayerID();
      BEntityID reinforceID = pSquad->getID();

//-- FIXING PREFIX BUG ID 5404
      const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
      if (!pPlayer)
      {
         continue;
      }

      // Create a reinforcement squad based on the depleted squad for transport
      BObjectCreateParms objectParms;
      objectParms.mPlayerID = playerID;
      objectParms.mStartBuilt = true;
      objectParms.mNoTieToGround = !pSquad->getFlagTiesToGround();
      objectParms.mPhysicsReplacement = false;
      objectParms.mPosition = (flyInStart != cInvalidVector) ? flyInStart : dropOffPos;
      objectParms.mForward = dropOffForward;
      objectParms.mRight = dropOffRight;
      objectParms.mProtoSquadID = pSquad->getProtoSquadID();
      // Ignore pop because we are reinforcing a squad that already exists
      objectParms.mIgnorePop = true; 
//-- FIXING PREFIX BUG ID 5405
      const BSquad* pReinforcementsSquad = gWorld->createSquad(objectParms);
//--
      if (!pReinforcementsSquad)
      {
         continue;
      }            

      // Create an order
      BSimOrder* pOrder = gSimOrderManager.createOrder();
      // Init order
      if (pOrder)  
      {
         pOrder->setPriority(BSimOrder::cPriorityTrigger);
      }

      BSquadActionReinforce::reinforceSquad(pOrder, reinforceID, pReinforcementsSquad->getID(), flyInStart, flyInEnd);
   }
}

//==============================================================================
// Select correct version
//==============================================================================
void BTriggerEffect::teMovePath()
{
   switch (getVersion())
   {
      case 3:
         teMovePathV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Move squads along a list of locations
//==============================================================================
void BTriggerEffect::teMovePathV3()
{
   enum 
   { 
      cSquad = 1, 
      cWaypointList = 2, 
      cSquadList = 3, 
      cAttackMove = 4,
      cQueueOrder = 5,
      cUseClosest = 6,
      cReverseMove = 7,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No Squad or SquadList designated!");
      return;
   }

   bool attackMove = getVar(cAttackMove)->isUsed() ? getVar(cAttackMove)->asBool()->readVar() : false;
   bool queueOrder = getVar(cQueueOrder)->isUsed() ? getVar(cQueueOrder)->asBool()->readVar() : false;

   // Make sure we have some squads to move to the waypoints.
   BEntityIDArray workingSquads;
   if (useSquadList)
   {
      workingSquads = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (useSquad)
   {
      workingSquads.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   if (workingSquads.getSize() == 0)
      return;

   // Make sure we have waypoints to move towards.
   BLocationArray waypointList = getVar(cWaypointList)->asVectorList()->readVar();
   if (waypointList.getSize() == 0)
      return;

#ifndef BUILD_FINAL
   gTriggerManager.incrementWorkPerfCount(workingSquads.getSize());
#endif

   // If use closest flag is used then we need to find the starting point and reduce the way point list appropriately
   if (getVar(cUseClosest)->isUsed() && getVar(cUseClosest)->asBool()->readVar())
   {
      // Find starting point
      BVector startingPoint = cInvalidVector;
      uint numSquads = workingSquads.getSize();
      uint validSquads = 0;
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5398
         const BSquad* pSquad = gWorld->getSquad(workingSquads[i]);
//--
         if (pSquad)
         {
            startingPoint += pSquad->getPosition();
            validSquads++;
         }
      }
      if (startingPoint != cInvalidVector)
      {
         startingPoint /= (float)validSquads;

         // Find closest way point
         float shortestDistSq = startingPoint.xzDistanceSqr(waypointList[0]);
         uint closeLocIndex = 0;
         uint numLocs = waypointList.getSize();
         for (uint i = 1; i < numLocs; i++)
         {
            float distSq = startingPoint.xzDistanceSqr(waypointList[i]);
            if (shortestDistSq > distSq)
            {
               shortestDistSq = distSq;
               closeLocIndex = i;
            }
         }

         // Find closest point from previous and next way point line segments
         BVector prevPoint = cInvalidVector;
         float prevDistSq = 0.0f;
         if (closeLocIndex > 0)
         {
            prevPoint = closestPointOnLine(waypointList[closeLocIndex - 1], waypointList[closeLocIndex], startingPoint);
            prevDistSq = startingPoint.xzDistanceSqr(prevPoint);
         }
         
         BVector nextPoint = cInvalidVector;
         float nextDistSq = 0.0f;
         uint nextIndex = closeLocIndex + 1;
         if (nextIndex < numLocs)
         {
            nextPoint = closestPointOnLine(waypointList[closeLocIndex], waypointList[nextIndex], startingPoint);
            nextDistSq = startingPoint.xzDistanceSqr(nextPoint);
         }

         if ((prevPoint != cInvalidVector) && (nextPoint != cInvalidVector))
         {
            if (prevDistSq < nextDistSq)
            {
               closeLocIndex--;
               waypointList[closeLocIndex] = prevPoint;
            }
            else
            {
               waypointList[closeLocIndex] = nextPoint;
            }
         }
         else if (prevPoint != cInvalidVector)
         {
            closeLocIndex--;
            waypointList[closeLocIndex] = prevPoint;
         }
         else
         {
            waypointList[closeLocIndex] = nextPoint;
         }

         //XXXHalwes - 11/27/2007 - Simply removing the points doesn't work correctly, doing a data copy instead works.
         // Remove the unnecessary way points
         BLocationArray tempLocs;
         for (uint i = closeLocIndex; i < numLocs; i++)
         {
            tempLocs.add(waypointList[i]);
         }
         waypointList.clear();
         waypointList.append(tempLocs);
      }
   }  

   // Reverse move flag
   bool reverseMove = getVar(cReverseMove)->isUsed() ? getVar(cReverseMove)->asBool()->readVar() : false;
   if (reverseMove)
   {
      uint squadCount = workingSquads.getSize();
      for (uint i = 0; i < squadCount; i++)
      {
//-- FIXING PREFIX BUG ID 5399
         const BSquad* pSquad = gWorld->getSquad(workingSquads[i]);
//--
         if (pSquad)
         {
            uint childCount = pSquad->getNumberChildren();
            for (uint j = 0; j < childCount; j++)
            {
               BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
               if (pUnit)
               {
                  pUnit->setFlagReverseMove(true);
               }
            }
         }
      }
   }

   // Group squads per player
   // MPB [8/27/2008] - Added the "filterOutSpartanHijackedEntities" function to remove hijacked vehicles from the list
   // of entities to command.  This is a hack to work around the fact that there is no player ID validation on the set
   // of entities that the trigger script wants to command, and because hijacking will change the playerID on a unit/squad.
   // This fixes bug PHX-9580.
   BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(workingSquads, filterOutSpartanHijackedEntities);
   uint numPlayerEntityIDs = playerEntityIDs.getSize();
   for (uint i = 0; i < numPlayerEntityIDs; i++)
   {
      uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
      if (numEntities > 0)
      {
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            // Platoon the squads
            pArmy->addSquads(playerEntityIDs[i].mEntityIDs, !gConfig.isDefined(cConfigClassicPlatoonGrouping));

            // Setup the command
            BWorkCommand tempCommand;
            const long numWaypoints = waypointList.getNumber();
            tempCommand.setWaypoints(waypointList.getPtr(), numWaypoints);

            // Set the command to be from a trigger to the army
            tempCommand.setRecipientType(BCommand::cArmy);
            tempCommand.setSenderType(BCommand::cTrigger);

            // Set attack move
            tempCommand.setFlag(BWorkCommand::cFlagAttackMove, attackMove);

            // Set queue order
            tempCommand.setFlag(BCommand::cFlagAlternate, queueOrder);

            // Give the command to the army.
            pArmy->queueOrder(&tempCommand);
         }               
      }
   }
}



//==============================================================================
// Select correct version
//==============================================================================
void BTriggerEffect::tePowerGrant()
{
   switch (getVersion())
   {
      case 2:
         tePowerGrantV2();
         break;

      case 3:
         tePowerGrantV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");         
         break;
   }
}

//===============================================================================
// Grant a power to a specific power and place it in the correct power menu slot
//===============================================================================
void BTriggerEffect::tePowerGrantV2()
{
   enum 
   { 
      cPlayer = 1, 
      cPower = 2, 
      cInputIconLoc = 3,
      cInputPlayerList = 4,
      cInputNumUses = 5,
      cInputIgnoreCost = 6,
      cInputIgnoreTech = 7,
      cInputIgnorePop = 8,
   };

   bool usePlayer = getVar(cPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();

   if (!usePlayer && !usePlayerList)
   {
      BTRIGGER_ASSERTM(false, "No Player or PlayerList designated!");
      return;
   }

   BPlayerIDArray playerIDs;
   if (usePlayerList)
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (usePlayer)
   {
      playerIDs.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }
   
   BProtoPowerID protoPowerID = getVar(cPower)->asPower()->readVar();
   int iconLoc = getVar(cInputIconLoc)->isUsed() ? getVar(cInputIconLoc)->asInteger()->readVar() : -1;
   int numUses = getVar(cInputNumUses)->isUsed() ? getVar(cInputNumUses)->asInteger()->readVar() : 1;
   bool ignoreCost = getVar(cInputIgnoreCost)->isUsed() ? getVar(cInputIgnoreCost)->asBool()->readVar() : false;
   bool ignoreTech = getVar(cInputIgnoreTech)->isUsed() ? getVar(cInputIgnoreTech)->asBool()->readVar() : false;
   bool ignorePop = getVar(cInputIgnorePop)->isUsed() ? getVar(cInputIgnorePop)->asBool()->readVar() : false;

   uint numPlayers = playerIDs.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerIDs[i]);
      if (pPlayer)
      {      
         if (iconLoc > -1)
         {
            // Clear the icon location
            pPlayer->removePowerEntryAtIconLocation(iconLoc);
         }
         // Remove duplicates
         pPlayer->removePowerEntry(protoPowerID, cInvalidObjectID);
         pPlayer->addPowerEntry(protoPowerID, cInvalidObjectID, numUses, iconLoc, ignoreCost, ignoreTech, ignorePop);
         BUser* pUser = pPlayer->getUser();
         if (pUser)
         {
            // Force a refresh on the power menu
            pUser->setFlagPowerMenuRefresh(true);
         }
      }
   }
}

//===============================================================================
// Grant a power to a specific power and place it in the correct power menu slot
//===============================================================================
void BTriggerEffect::tePowerGrantV3()
{
   enum 
   { 
      cPlayer = 1, 
      cPower = 2, 
      cInputIconLoc = 3,
      cInputPlayerList = 4,
      cInputNumUses = 5,
      cInputIgnoreCost = 6,
      cInputIgnoreTech = 7,
      cInputIgnorePop = 8,
      cInputPowerSquad = 9,
   };

   bool usePlayer = getVar(cPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();

   if (!usePlayer && !usePlayerList)
   {
      BTRIGGER_ASSERTM(false, "No Player or PlayerList designated!");
      return;
   }

   BPlayerIDArray playerIDs;
   if (usePlayerList)
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (usePlayer)
   {
      playerIDs.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }

   BProtoPowerID protoPowerID = getVar(cPower)->asPower()->readVar();
   int iconLoc = getVar(cInputIconLoc)->isUsed() ? getVar(cInputIconLoc)->asInteger()->readVar() : -1;
   int numUses = getVar(cInputNumUses)->isUsed() ? getVar(cInputNumUses)->asInteger()->readVar() : 1;
   bool ignoreCost = getVar(cInputIgnoreCost)->isUsed() ? getVar(cInputIgnoreCost)->asBool()->readVar() : false;
   bool ignoreTech = getVar(cInputIgnoreTech)->isUsed() ? getVar(cInputIgnoreTech)->asBool()->readVar() : false;
   bool ignorePop = getVar(cInputIgnorePop)->isUsed() ? getVar(cInputIgnorePop)->asBool()->readVar() : false;
   BEntityID powerSquad = getVar(cInputPowerSquad)->isUsed() ? getVar(cInputPowerSquad)->asSquad()->readVar() : cInvalidObjectID;
   if (powerSquad != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 5402
      const BSquad* pSquad = gWorld->getSquad(powerSquad);
//--
      if (!pSquad)
      {
         powerSquad = cInvalidObjectID;
      }
   }

   uint numPlayers = playerIDs.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerIDs[i]);
      if (pPlayer)
      {      
         if (iconLoc > -1)
         {
            // Clear the icon location
            pPlayer->removePowerEntryAtIconLocation(iconLoc);
         }
         // Remove duplicates
         pPlayer->removePowerEntry(protoPowerID, powerSquad);
         pPlayer->addPowerEntry(protoPowerID, powerSquad, numUses, iconLoc, ignoreCost, ignoreTech, ignorePop);
         BUser* pUser = pPlayer->getUser();
         if (pUser)
         {
            // Force a refresh on the power menu
            pUser->setFlagPowerMenuRefresh(true);
         }
      }
   }
}

//==============================================================================
// Choose correct version
//==============================================================================
void BTriggerEffect::tePowerRevoke()
{
   switch (getVersion())
   {
      case 2:
         tePowerRevokeV2();
         break;

      case 3:
         tePowerRevokeV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Revoke a power from the player(s)
//==============================================================================
void BTriggerEffect::tePowerRevokeV2()
{
   enum 
   { 
      cPlayer = 1, 
      cPower = 2, 
      cInputPlayerList = 4,
   };

   bool usePlayer = getVar(cPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();

   if (!usePlayer && !usePlayerList)
   {
      BTRIGGER_ASSERTM(false, "No Player or PlayerList designated!");
      return;
   }

   BPlayerIDArray playerIDs;
   if (usePlayerList)
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (usePlayer)
   {
      playerIDs.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }

   BProtoPowerID protoPowerID = getVar(cPower)->asPower()->readVar();

   uint numPlayers = playerIDs.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerIDs[i]);
      if (pPlayer)
      {
         pPlayer->removePowerEntry(protoPowerID, cInvalidObjectID);

         BUser* pUser = pPlayer->getUser();
         if (pUser)
         {
            // Force refresh on user's power menu
            pUser->setFlagPowerMenuRefresh(true);
         }
      }
   }      
}

//==============================================================================
// Revoke a power from the player(s)
//==============================================================================
void BTriggerEffect::tePowerRevokeV3()
{
   enum 
   { 
      cPlayer = 1, 
      cPower = 2, 
      cInputPlayerList = 4,
      cInputPowerSquad = 5,
   };

   bool usePlayer = getVar(cPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();

   if (!usePlayer && !usePlayerList)
   {
      BTRIGGER_ASSERTM(false, "No Player or PlayerList designated!");
      return;
   }

   BPlayerIDArray playerIDs;
   if (usePlayerList)
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (usePlayer)
   {
      playerIDs.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }

   BProtoPowerID protoPowerID = getVar(cPower)->asPower()->readVar();

   BEntityID powerSquad = getVar(cInputPowerSquad)->isUsed() ? getVar(cInputPowerSquad)->asSquad()->readVar() : cInvalidObjectID;
   if (powerSquad != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 5403
      const BSquad* pSquad = gWorld->getSquad(powerSquad);
//--
      if (!pSquad)
      {
         powerSquad = cInvalidObjectID;
      }
   }

   uint numPlayers = playerIDs.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerIDs[i]);
      if (pPlayer)
      {
         pPlayer->removePowerEntry(protoPowerID, powerSquad);

         BUser* pUser = pPlayer->getUser();
         if (pUser)
         {
            // Force refresh on user's power menu
            pUser->setFlagPowerMenuRefresh(true);
         }
      }
   }      
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetSquadTrainerType()
{
   enum { cPlayer = 1, cProtoObject = 2, cProtoSquad = 3, };
   BProtoObjectID protoObjectID = cInvalidProtoObjectID;
//-- FIXING PREFIX BUG ID 5410
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
   if (pPlayer)
   {
      BProtoSquadID protoSquadID = getVar(cProtoSquad)->asProtoSquad()->readVar();
      protoObjectID = pPlayer->getBuildingProtoIDToTrainSquad(protoSquadID, false);
   }
   getVar(cProtoObject)->asProtoObject()->writeVar(protoObjectID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetTechResearcherType()
{
   enum { cPlayer = 1, cProtoObject = 2, cProtoTech = 3, };
   BProtoObjectID protoObjectID = cInvalidProtoObjectID;
//-- FIXING PREFIX BUG ID 5411
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
   if (pPlayer)
   {
      BProtoTechID protoTechID = getVar(cProtoTech)->asTech()->readVar();
      protoObjectID = pPlayer->getBuildingProtoIDToResearchTech(protoTechID, false);
   }
   getVar(cProtoObject)->asProtoObject()->writeVar(protoObjectID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teDesignLineGetPoints()
{
   enum { cDesignLine = 1, cPoints = 2 };

  
   BDesignLineID lineid = getVar(cDesignLine)->asDesignLine()->readVar();
   
   BDesignLine empty;
   BDesignLine& foundLine = empty;
   uint lineCount = gWorld->getDesignObjectManager()->getDesignLineCount();   
   for(uint i = 0; i<lineCount; i++)
   {
//-- FIXING PREFIX BUG ID 5412
      const BDesignLine& line = gWorld->getDesignObjectManager()->getDesignLine(i);
//--
      if(line.mID == lineid)
      {
         foundLine = line;
      }
   }

   BTriggerVarVectorList *pOutputList = getVar(cPoints)->asVectorList();
   pOutputList->clear();

   //const BLocationArray& addList = getVar(cPoints)->asVectorList()->readVar();
   uint pointCount = foundLine.mPoints.size();
   for (uint i=0; i<pointCount; i++)
      pOutputList->addVector(foundLine.mPoints[i]);
}


//XXXHalwes - 7/17/2007 - This justs sets a flag, should we consolidate to the flag set trigger or have individual triggers and remove the
//                        flag set trigger.
//======================================================
// Enable/Disable the shield on units that have shields
//======================================================
void BTriggerEffect::teEnableShield()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cEnable = 5,
   };

   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useUnit && !useUnitList && !useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No unit, unit list, squad, or squad list assigned!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (useUnitList)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (useUnit)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5406
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 5407
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            unitList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Iterate through units
   bool enable = getVar(cEnable)->asBool()->readVar();
   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (!pUnit)
      {
         continue;
      }

      const BProtoObject* pProtoObject = pUnit->getProtoObject();
      if (!pProtoObject)
      {
         continue;
      }

      if (pProtoObject->getFlagHasShield())
      {
         pUnit->setFlagHasShield(enable);
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPlayerLeader()
{
   enum { cPlayer = 1, cLeader = 2, };
   const BPlayer *pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   getVar(cLeader)->asLeader()->writeVar(pPlayer->getLeaderID());
}


//==============================================================================
// BTriggerEffect::teLaunchCinematic()
//==============================================================================
void BTriggerEffect::teLaunchCinematic()
{
   switch (getVersion())
   {
      case 3:
         teLaunchCinematicV3();
         break;

      case 4:
         teLaunchCinematicV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// BTriggerEffect::teLaunchCinematicV3()
//==============================================================================
void BTriggerEffect::teLaunchCinematicV3()
{
   enum
   {
      cCinematic                 = 1,
      cInputSquadPossess1        = 2,
      cInputSquadPossess2        = 3,
      cInputSquadPossess3        = 4,
      cInputSquadPossess4        = 5,
      cInputSquadPossess5        = 6,
      cInputSquadPossess6        = 7
   };

   if(gModeManager.getModeType() == BModeManager::cModeCinematic)
      return;

   BSmallDynamicSimArray<BEntityID> squadList;

   if (getVar(cInputSquadPossess1)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess1)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess2)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess2)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess3)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess3)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess4)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess4)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess5)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess5)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess6)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess6)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }

   BCinematicManager* pCinematicManager = gWorld->getCinematicManager();
   if (!pCinematicManager)
   {
      BASSERT(pCinematicManager);
      return;
   }

#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigWowRecord))
   {
//-- FIXING PREFIX BUG ID 5416
      const BCinematic* pCinematic = pCinematicManager->getCinematicByID(getVar(cCinematic)->asCinematic()->readVar());
//--
      if (pCinematic)
         gConsole.output(cChannelTriggers, "LaunchCinematic '%s', time %.1f", pCinematic->getFilename().getPtr(), gWorld->getGametimeFloat());
   }
#endif

   pCinematicManager->playCinematicByID(getVar(cCinematic)->asCinematic()->readVar(), &squadList);

   // If subscriber exists reset
   BGeneralEventSubscriber* pSubscription = pCinematicManager->getCinematicCompletedEventSubscriber();
   if (pSubscription)
   {
      pSubscription->mFired = false;
   }   
   // Otherwise create a new subscriber
   else
   {
      pSubscription = gGeneralEventManager.newSubscriber(BEventDefinitions::cCinematicCompleted);
      BASSERT(pSubscription);
      pCinematicManager->setCinematicCompletedEventSubscriber(pSubscription);
   }

   MVinceEventAsync_WowFired(gUserManager.getPrimaryUser());
#if defined(VINCE)
   if (gUserManager.getSecondaryUser())
   {
      MVinceEventAsync_WowFired(gUserManager.getSecondaryUser());
   }
#endif  
}

//==============================================================================
// BTriggerEffect::teLaunchCinematicV4()
//==============================================================================
void BTriggerEffect::teLaunchCinematicV4()
{
   enum
   {
      cCinematic                 = 1,
      cInputSquadPossess1        = 2,
      cInputSquadPossess2        = 3,
      cInputSquadPossess3        = 4,
      cInputSquadPossess4        = 5,
      cInputSquadPossess5        = 6,
      cInputSquadPossess6        = 7,
      cOutputPreRendered         = 14,
   };


   if(gModeManager.getModeType() == BModeManager::cModeCinematic)
      return;

   BSmallDynamicSimArray<BEntityID> squadList;

   if (getVar(cInputSquadPossess1)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess1)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess2)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess2)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess3)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess3)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess4)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess4)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess5)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess5)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }
   if (getVar(cInputSquadPossess6)->isUsed())
   {
      BEntityID entity = getVar(cInputSquadPossess6)->asSquad()->readVar();
      if(!squadList.contains(entity))
         squadList.add(entity);
   }

   BCinematicManager* pCinematicManager = gWorld->getCinematicManager();
   if (!pCinematicManager)
   {
      BASSERT(pCinematicManager);
      return;
   }

#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigWowRecord))
   {
//-- FIXING PREFIX BUG ID 5417
      const BCinematic* pCinematic = pCinematicManager->getCinematicByID(getVar(cCinematic)->asCinematic()->readVar());
//--
      if (pCinematic)
         gConsole.output(cChannelTriggers, "LaunchCinematic '%s', time %.1f", pCinematic->getFilename().getPtr(), gWorld->getGametimeFloat());
   }
#endif

   bool preRendered = false;
   pCinematicManager->playCinematicByID(getVar(cCinematic)->asCinematic()->readVar(), &squadList, &preRendered);

   if (getVar(cOutputPreRendered)->isUsed())
      getVar(cOutputPreRendered)->asBool()->writeVar(preRendered);

   // If subscriber exists reset
   BGeneralEventSubscriber* pSubscription = pCinematicManager->getCinematicCompletedEventSubscriber();
   if (pSubscription)
   {
      pSubscription->mFired = false;
   }   
   // Otherwise create a new subscriber
   else
   {
      pSubscription = gGeneralEventManager.newSubscriber(BEventDefinitions::cCinematicCompleted);
      BASSERT(pSubscription);
      pCinematicManager->setCinematicCompletedEventSubscriber(pSubscription);
   }

   MVinceEventAsync_WowFired(gUserManager.getPrimaryUser());
#if defined(VINCE)
   if (gUserManager.getSecondaryUser())
   {
      MVinceEventAsync_WowFired(gUserManager.getSecondaryUser());
   }
#endif  
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teIteratorProtoObjectList()
{
   enum { cProtoObjectList = 1, cIterator = 2, };
   BTriggerVarID protoObjectListVarID = getVar(cProtoObjectList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(protoObjectListVarID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teIteratorProtoSquadList()
{
   enum { cProtoSquadList = 1, cIterator = 2, };
   BTriggerVarID protoSquadListVarID = getVar(cProtoSquadList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(protoSquadListVarID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teIteratorObjectTypeList()
{
   enum { cObjectTypeList = 1, cIterator = 2, };
   BTriggerVarID objectTypeListVarID = getVar(cObjectTypeList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(objectTypeListVarID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teIteratorTechList()
{
   enum { cTechList = 1, cIterator = 2, };
   BTriggerVarID techListVarID = getVar(cTechList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(techListVarID);
}

//==============================================================================
// Sets the designated unit(s), squad(s), and/or object(s) direction vector
//==============================================================================
void BTriggerEffect::teSetDirection()
{
   switch (getVersion())
   {
      case 1:
         teSetDirectionV1();
         break;

      case 2:
         teSetDirectionV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Sets the designated unit(s), squad(s), and/or object(s) direction vector
//==============================================================================
void BTriggerEffect::teSetDirectionV1()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cObject = 5,
      cObjectList = 6,
      cDirection = 7,
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();
   bool objectUsed = getVar(cObject)->isUsed();
   bool objectListUsed = getVar(cObjectList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed && !objectUsed && !objectListUsed)
   {
      BTRIGGER_ASSERTM(false, "No unit, unit list, squad, squad list, object, or object list assigned!");
      return;
   }

   // Collect entities
   BEntityIDArray entityList;
   if (unitListUsed)
   {
      entityList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (unitUsed)
   {
      entityList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // Collect squads
   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5408
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            entityList.uniqueAdd(squadList[i]);  
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               entityList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5409
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         entityList.uniqueAdd(pSquad->getID());
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            entityList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Collect objects
   if (objectListUsed)
   {
      entityList.append(getVar(cObjectList)->asObjectList()->readVar());
   }

   if (objectUsed)
   {
      entityList.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   BVector forward = getVar(cDirection)->asVector()->readVar();   

   // Iterate through entities
   uint numEntities = (uint)entityList.getNumber();
   for (uint i = 0; i < numEntities; i++)
   {
      BEntity* pEntity = gWorld->getEntity(entityList[i]);
      if (pEntity)
      {
         pEntity->setForward(forward);
         pEntity->calcRight();
         pEntity->calcUp();
         pEntity->updateObstruction();
      }
   }
}

//==============================================================================
// Sets the designated unit(s), squad(s), and/or object(s) direction vector
//==============================================================================
void BTriggerEffect::teSetDirectionV2()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cObject = 5,
      cObjectList = 6,
      cDirection = 7,
      cUseTurrets = 8,
   };

   // Collect objects
   BEntityIDArray objectList;
   if (getVar(cObjectList)->isUsed())
   {
      objectList = getVar(cObjectList)->asObjectList()->readVar();
   }

   if (getVar(cObject)->isUsed())
   {
      objectList.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   // Collect units
   if (getVar(cUnitList)->isUsed())
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      uint numUnits = unitList.getSize();
      for (uint i = 0; i < numUnits; i++)
      {
         BUnit* pUnit = gWorld->getUnit(unitList[i]);
         if (pUnit)
         {
//-- FIXING PREFIX BUG ID 5420
            const BObject* pObject = pUnit->getObject();
//--
            if (pObject)
            {
               objectList.uniqueAdd(pObject->getID());
            }
         }
      }
   }

   if (getVar(cUnit)->isUsed())
   {
      BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5421
         const BObject* pObject = pUnit->getObject();
//--
         if (pObject)
         {
            objectList.uniqueAdd(pObject->getID());
         }
      }
   }

   // Collect squads
   if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = squadList.getSize();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5413
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
               if (pUnit)
               {
//-- FIXING PREFIX BUG ID 5422
                  const BObject* pObject = pUnit->getObject();
//--
                  if (pObject)
                  {
                     objectList.uniqueAdd(pObject->getID());
                  }
               }
            }
         }
      }
   }

   if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5414
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
            if (pUnit)
            {
//-- FIXING PREFIX BUG ID 5424
               const BObject* pObject = pUnit->getObject();
//--
               if (pObject)
               {
                  objectList.uniqueAdd(pObject->getID());
               }
            }
         }
      }
   }

   BVector forward = getVar(cDirection)->asVector()->readVar();   
   bool useTurrets = getVar(cUseTurrets)->isUsed() ? getVar(cUseTurrets)->asBool()->readVar() : false;

   // Iterate through objects
   uint numObjects = objectList.getSize();
   for (uint i = 0; i < numObjects; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject)
      {                  
         int turretHardpointID = -1;
         const BProtoObject* pProtoObject = pObject->getProtoObject();
         if (pProtoObject)
         {
            turretHardpointID = pProtoObject->findHardpoint("Turret");            
         }

         if (useTurrets && (turretHardpointID != -1))
         {
            const BHardpoint* pHardpoint = pObject->getHardpoint(pProtoObject->findHardpoint("Turret"));
            BVector targetLocation = pObject->getPosition() + (forward * (pObject->getObstructionRadius() * 2.0f));
            float forceTime = 1000.0f;
            if (pHardpoint->getFlagCombined())
            {
               pObject->orientHardpointToWorldPos(turretHardpointID, targetLocation, forceTime);
            }
            else
            {
               pObject->yawHardpointToWorldPos(turretHardpointID, targetLocation, forceTime);
               pObject->pitchHardpointToWorldPos(turretHardpointID, targetLocation, forceTime);
            }
         }
         else
         {
            pObject->setForward(forward);
            pObject->calcRight();
            pObject->calcUp();
            pObject->updateObstruction();
         }
      }
   }
}


//==============================================================================
// Gets the direction value from two location vectors
//==============================================================================
void BTriggerEffect::teGetDirectionFromLocations()
{
   enum
   {
      cLoc1 = 1,
      cLoc2 = 2,
      cDirection = 4,
   };

   BVector loc1 = getVar(cLoc1)->asVector()->readVar();
   BVector loc2 = getVar(cLoc2)->asVector()->readVar();

   BVector dir = loc2 - loc1;
   dir.normalize();

   getVar(cDirection)->asVector()->writeVar(dir);
}

//==============================================================================
// Outputs the value of the assigned data type to a count data type
//==============================================================================
void BTriggerEffect::teAsCount()
{
   enum 
   { 
      cFloat = 1, 
      cHitpoints = 2,
      cDistance = 3,
      cOutCount = 4,
   };

   bool floatUsed = getVar(cFloat)->isUsed();
   bool hpUsed = getVar(cHitpoints)->isUsed();
   bool distanceUsed = getVar(cDistance)->isUsed();

   int result = 0;

   if (floatUsed)
   {
      result = (int)getVar(cFloat)->asFloat()->readVar();
   }
   else if (hpUsed)
   {
      result = (int)getVar(cHitpoints)->asFloat()->readVar();
   }
   else if (distanceUsed)
   {
      result = (int)getVar(cDistance)->asFloat()->readVar();
   }

   getVar(cOutCount)->asInteger()->writeVar(result);
}

//==============================================================
// Outputs the normalized forward vector of the provided entity
//==============================================================
void BTriggerEffect::teGetDirection()
{
   switch (getVersion())
   {
      case 1:
         teGetDirectionV1();
         break;

      case 2:
         teGetDirectionV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================
// Outputs the normalized forward vector of the provided entity
//==============================================================
void BTriggerEffect::teGetDirectionV1()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
      cObject = 3,
      cDir = 4,
   };   

   BEntityID entity = cInvalidObjectID;
   if (getVar(cUnit)->isUsed())
   {
      entity = getVar(cUnit)->asUnit()->readVar();
   }
   else if (getVar(cSquad)->isUsed())
   {
      entity = getVar(cSquad)->asSquad()->readVar();
   }
   else if (getVar(cObject)->isUsed())
   {
      entity = getVar(cObject)->asObject()->readVar();
   }

   BVector dir = cInvalidVector;
   if (entity != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 5426
      const BEntity* pEntity = gWorld->getEntity(entity);
//--
      if (pEntity)
      {
         dir = pEntity->getForward();
         dir.normalize();
      }
   }
   else
   {
      BTRIGGER_ASSERTM( false, "No unit, squad, or object assigned!  This will result in an invalid direction vector!");
   }

   getVar(cDir)->asVector()->writeVar(dir);
}

//==============================================================
// Outputs the normalized forward vector of the provided entity
//==============================================================
void BTriggerEffect::teGetDirectionV2()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
      cObject = 3,
      cDir = 4,
      cRight = 5,
      cUp = 6,
   };   

   BEntityID entity = cInvalidObjectID;
   if (getVar(cUnit)->isUsed())
   {
      entity = getVar(cUnit)->asUnit()->readVar();
   }
   else if (getVar(cSquad)->isUsed())
   {
      entity = getVar(cSquad)->asSquad()->readVar();
   }
   else if (getVar(cObject)->isUsed())
   {
      entity = getVar(cObject)->asObject()->readVar();
   }

   BVector dir = cInvalidVector;
   BVector right = cInvalidVector;
   BVector up = cInvalidVector;
   if (entity != cInvalidObjectID)
   {
      BEntity* pEntity = gWorld->getEntity(entity);
      if (pEntity)
      {
         dir = pEntity->getForward();
         dir.normalize();
         right = pEntity->getRight();
         right.normalize();
         up = pEntity->getUp();
         up.normalize();
      }
   }
   else
   {
      BTRIGGER_ASSERTM( false, "No unit, squad, or object assigned!  This will result in an invalid direction vector!");
   }

   if (getVar(cDir)->isUsed())
   {
      getVar(cDir)->asVector()->writeVar(dir);
   }   

   if (getVar(cRight)->isUsed())
   {
      getVar(cRight)->asVector()->writeVar(right);
   }

   if (getVar(cUp)->isUsed())
   {
      getVar(cUp)->asVector()->writeVar(up);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetLegalSquads()
{
   enum { cPlayer = 1, cProtoSquadList = 2, cCount = 3, };

   BProtoSquadIDArray legalSquads;
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (pPlayer)
      pPlayer->getCurrentlyLegalSquadsToTrain(legalSquads);

   getVar(cProtoSquadList)->asProtoSquadList()->writeVar(legalSquads);
   BTriggerVarInteger *pInt = getVar(cCount)->asInteger();
   if (pInt->isUsed())
      pInt->writeVar(legalSquads.getNumber());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetLegalTechs()
{
   enum { cPlayer = 1, cProtoTechList = 2, cCount = 3, };

   BTechIDArray legalTechs;
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (pPlayer)
      pPlayer->getCurrentlyLegalTechsToResearch(legalTechs);

   getVar(cProtoTechList)->asTechList()->writeVar(legalTechs);
   BTriggerVarInteger *pInt = getVar(cCount)->asInteger();
   if (pInt->isUsed())
      pInt->writeVar(legalTechs.getNumber());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetLegalBuildings()
{
   enum { cPlayer = 1, cProtoObjectList = 2, cCount = 3, };

   BProtoObjectIDArray legalBuildings;
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (pPlayer)
      pPlayer->getCurrentlyLegalBuildingsToConstruct(legalBuildings);

   getVar(cProtoObjectList)->asProtoObjectList()->writeVar(legalBuildings);
   BTriggerVarInteger *pInt = getVar(cCount)->asInteger();
   if (pInt->isUsed())
      pInt->writeVar(legalBuildings.getNumber());
}

//==============================================================================
// Pick the correct version
//==============================================================================
void BTriggerEffect::teSetMobile()
{
   switch (getVersion())
   {
      case 1:
         teSetMobileV1();
         break;

      case 2:
         teSetMobileV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//XXXHalwes - 7/17/2007 - This justs sets a flag, should we consolidate to the flag set trigger or have individual triggers and remove the
//                        flag set trigger.
//==============================================================================
// Set the unit(s) or squad(s) mobility
//==============================================================================
void BTriggerEffect::teSetMobileV1()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cMobile = 5,
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No unit, unit list, squad, or squad list assigned!");
      return;
   }

   // Collect Units
   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();      
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // Collect Squads
   BEntityIDArray squadList;
   if (squadListUsed)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (squadUsed)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5418
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (pSquad)
         {
            squadList.uniqueAdd(pSquad->getID());
         }
      }
   }

   bool mobile = getVar(cMobile)->asBool()->readVar();

   // Iterate through squads
   uint numSquads = (uint)squadList.getNumber();
   for (uint i = 0; i < numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         // Don't modify mobility if the protoobject says we're nonmobile (i.e. a building)
         if (pSquad->getProtoObject() && pSquad->getProtoObject()->getFlagNonMobile())
            continue;

         if (!mobile)
         {
            //pSquad->doTotalStop();
            //pSquad->removeCurrentCommandActions();
            //pSquad->clearCommands();            
            pSquad->removeAllOrders();
         }
         pSquad->setFlagNonMobile(!mobile);
      }
   }
}

//XXXHalwes - 7/17/2007 - This justs sets a flag, should we consolidate to the flag set trigger or have individual triggers and remove the
//                        flag set trigger.
//==============================================================================
// Set the unit(s) or squad(s) mobility
//==============================================================================
void BTriggerEffect::teSetMobileV2()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cMobile = 5,
      cTemporary = 6,
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM(false, "No unit, unit list, squad, or squad list assigned!");
      return;
   }

   // Collect Units
   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();      
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   // Collect Squads
   BEntityIDArray squadList;
   if (squadListUsed)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (squadUsed)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      if (pUnit)
      {                  
//-- FIXING PREFIX BUG ID 5423
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (pSquad)
         {
            squadList.uniqueAdd(pSquad->getID());
         }
      }
   }

   bool mobile = getVar(cMobile)->asBool()->readVar();

   // Iterate through squads
   uint numSquads = (uint)squadList.getNumber();
   for (uint i = 0; i < numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {         
         // Don't modify mobility if the protoobject says we're nonmobile (i.e. a building)
         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (pProtoObject && pProtoObject->getFlagNonMobile())
            continue;

         if (!mobile)
         {
            bool isTemp = getVar(cTemporary)->isUsed() ? getVar(cTemporary)->asBool()->readVar() : false;
            if (!isTemp)
            {
               //pSquad->removeCurrentCommandActions();
               //pSquad->clearCommands();            
               pSquad->removeAllOrders();
            }
         }
         pSquad->setFlagNonMobile(!mobile);
      }
   }
}


//==============================================================================
// Get the distance between the locations
//==============================================================================
void BTriggerEffect::teGetDistanceLocationLocation()
{
   enum
   {
      cLoc1 = 1,
      cLoc2 = 2,
      cDist = 3,
   };

   BVector loc1 = getVar(cLoc1)->asVector()->readVar();
   BVector loc2 = getVar(cLoc2)->asVector()->readVar();
   float dist = loc1.distance(loc2);

   getVar(cDist)->asFloat()->writeVar(dist);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayWorldSoundAtPosition()
{
   enum { cSound = 1, cLocation = 2, cQueue = 3, };
   const BSimString &sound = getVar(cSound)->asSound()->readVar();
   BVector location = getVar(cLocation)->asVector()->readVar();
   bool queueSound = false;
//-- FIXING PREFIX BUG ID 5430
   const BTriggerVarBool* pVarQueue = getVar(cQueue)->asBool();
//--
   if (pVarQueue->isUsed())
      queueSound = pVarQueue->readVar();

   BCueIndex cueIndex = gSoundManager.getCueIndex(sound);
   gWorld->getWorldSoundManager()->addSound(location, cueIndex, false, cInvalidCueIndex, false, false);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayWorldSoundOnObject()
{
   switch (getVersion())
   {
      case 1:
         tePlayWorldSoundOnObjectV1();
         break;

      case 2:
         tePlayWorldSoundOnObjectV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayWorldSoundOnObjectV1()
{
   enum { cSound = 1, cObject = 2, cQueue = 3, };
   const BSimString &sound = getVar(cSound)->asSound()->readVar();
   BEntityID objectID = getVar(cObject)->asObject()->readVar();
   bool queueSound = false;
//-- FIXING PREFIX BUG ID 5432
   const BTriggerVarBool* pVarQueue = getVar(cQueue)->asBool();
//--
   if (pVarQueue->isUsed())
      queueSound = pVarQueue->readVar();
   BObject* pObject = gWorld->getObject(objectID);
   if (pObject)
   {
      BCueIndex cueIndex = gSoundManager.getCueIndex(sound);
      gWorld->getWorldSoundManager()->addSound(pObject, -1, cueIndex, false, cInvalidCueIndex, false, false);
   }
   else
   {      
      // Do something else? No object specified, no sounds for you!
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayWorldSoundOnObjectV2()
{
   enum 
   { 
      cSound = 1, 
      cObject = 2, 
      cQueue = 3, 
      cInUnit = 4,
      cInSquad = 5,
   };

   const BSimString& sound = getVar(cSound)->asSound()->readVar();
   BEntityID entityID = cInvalidObjectID;
   if (getVar(cObject)->isUsed())
   {
      entityID = getVar(cObject)->asObject()->readVar();
   }
   else if (getVar(cInUnit)->isUsed())
   {
      entityID = getVar(cInUnit)->asUnit()->readVar();
   }
   else if (getVar(cInSquad)->isUsed())
   {
      entityID = getVar(cInSquad)->asSquad()->readVar();
   }

   // Halwes - 8/26/2008 - This was in version 1 but is not even used?
   //bool queueSound = getVar(cQueue)->isUsed() ? getVar(cQueue)->asBool()->readVar() : false;
   BEntity* pEntity = gWorld->getEntity(entityID);
   if (pEntity)
   {
      BCueIndex cueIndex = gSoundManager.getCueIndex(sound);
      gWorld->getWorldSoundManager()->addSound(pEntity, -1, cueIndex, false, cInvalidCueIndex, false, false);
   }
}

//==============================================================================
// Return a random time
//==============================================================================
void BTriggerEffect::teRandomTime()
{
   enum 
   { 
      cInputMin = 1, 
      cInputMax = 2, 
      cOutputRandom = 3, 
   };   

   DWORD min = 0;
   DWORD max = (DWORD)BRandomManager::cLongMax;
   if (getVar(cInputMin)->isUsed())
   {
      min = getVar(cInputMin)->asTime()->readVar();
   }
   if (getVar(cInputMax)->isUsed())
   {
      max = Math::Min(getVar(cInputMax)->asTime()->readVar(), max);
   }
   BTRIGGER_ASSERT(min < max);
   DWORD value = getRandRangeDWORD(cSimRand, min, max);

   getVar(cOutputRandom)->asTime()->writeVar(value);
}

//==============================================================================
// Return a unit's parent squad
//==============================================================================
void BTriggerEffect::teGetParentSquad()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
   };

   BEntityID unit = getVar(cUnit)->asUnit()->readVar();
//-- FIXING PREFIX BUG ID 5433
   const BUnit* pUnit = gWorld->getUnit(unit);
//--
   if (!pUnit)
   {
      BTRIGGER_ASSERTM(false, "A NULL unit has been provided!");
      return;
   }

   getVar(cSquad)->asSquad()->writeVar(pUnit->getParentID());
}

//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teCreateIconObject()
{
   switch (getVersion())
   {
      case 2:
         teCreateIconObjectV2();
         break;

      case 3:
         teCreateIconObjectV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Create an object of Icon object type
//==============================================================================
void BTriggerEffect::teCreateIconObjectV2()
{
   enum
   {
      cPlayer = 2,
      cLocation = 3,
      cIconColor = 4,      
      cClearExisting = 5,
      cOutputObject = 6,
      cOutputObjectList = 7,
      cIconType = 9,
   };

   bool useOutputObject = getVar(cOutputObject)->isUsed();
   bool useOutputObjectList = getVar(cOutputObjectList)->isUsed();
   bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;

   BEntityID createdIconObjectID = cInvalidObjectID;

   int protoID = getVar(cIconType)->asIconType()->readVar();

   // Verify the proto object is an icon object type
//-- FIXING PREFIX BUG ID 5295
   const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
//--
   bool pass = true;
   if (!pProtoObject)
   {
      BTRIGGER_ASSERTM(false, "The specified proto object is NULL!");
      pass = false;
   }
   if (!pProtoObject->isType(gDatabase.getOTIDIcon()))
   {
      BTRIGGER_ASSERTM(false, "The specified proto object is not an icon object type!");
      pass = false;
   }

   if (pass)
   {
      // Seed our object create parms & create our object.
      BObjectCreateParms parms;
      parms.mProtoObjectID = protoID;
      parms.mPlayerID = getVar(cPlayer)->asPlayer()->readVar();
      parms.mPosition = getVar(cLocation)->asVector()->readVar();
      BObject* pObject = gWorld->createObject(parms);
      if (pObject)
      {
         createdIconObjectID = pObject->getID();
         if (getVar(cIconColor)->isUsed())
         {
            pObject->setIconColor(getVar(cIconColor)->asColor()->readVar());
         }         
      }      
   }

   if (useOutputObject)
   {
      getVar(cOutputObject)->asObject()->writeVar(createdIconObjectID);
   }

   if (useOutputObjectList)
   {
      if (clearExisting)
      {
         getVar(cOutputObjectList)->asObjectList()->clear();
      }

      if (createdIconObjectID != cInvalidObjectID)
         getVar(cOutputObjectList)->asObjectList()->addObject(createdIconObjectID);
   }
}

//==============================================================================
// Create an object of Icon object type
//==============================================================================
void BTriggerEffect::teCreateIconObjectV3()
{
   enum
   {
      cPlayer = 2,
      cLocation = 3,
      cIconColor = 4,      
      cClearExisting = 5,
      cOutputObject = 6,
      cOutputObjectList = 7,
      cIconType = 9,
      cVisibleToAll = 10,
   };

   bool useOutputObject = getVar(cOutputObject)->isUsed();
   bool useOutputObjectList = getVar(cOutputObjectList)->isUsed();
   bool clearExisting = getVar(cClearExisting)->isUsed() ? getVar(cClearExisting)->asBool()->readVar() : false;

   BEntityID createdIconObjectID = cInvalidObjectID;

   int protoID = getVar(cIconType)->asIconType()->readVar();

   // Verify the proto object is an icon object type
//-- FIXING PREFIX BUG ID 5298
   const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
//--
   bool pass = true;
   if (!pProtoObject)
   {
      BTRIGGER_ASSERTM(false, "The specified proto object is NULL!");
      pass = false;
   }
   if (!pProtoObject->isType(gDatabase.getOTIDIcon()))
   {
      BTRIGGER_ASSERTM(false, "The specified proto object is not an icon object type!");
      pass = false;
   }

   if (pass)
   {
      // Seed our object create parms & create our object.
      BObjectCreateParms parms;
      parms.mProtoObjectID = protoID;
      parms.mPlayerID = getVar(cPlayer)->asPlayer()->readVar();
      parms.mPosition = getVar(cLocation)->asVector()->readVar();
      BObject* pObject = gWorld->createObject(parms);
      if (pObject)
      {
         createdIconObjectID = pObject->getID();
         if (getVar(cIconColor)->isUsed())
         {
            pObject->setIconColor(getVar(cIconColor)->asColor()->readVar());
         }         

         bool visibleToAll = getVar(cVisibleToAll)->isUsed() ? getVar(cVisibleToAll)->asBool()->readVar() : false;
         if (visibleToAll)
         {
            pObject->setFlagVisibleForOwnerOnly(false);
            pObject->setFlagVisibleToAll(true);
         }         
      }      
   }

   if (useOutputObject)
   {
      getVar(cOutputObject)->asObject()->writeVar(createdIconObjectID);
   }

   if (useOutputObjectList)
   {
      if (clearExisting)
      {
         getVar(cOutputObjectList)->asObjectList()->clear();
      }

      if (createdIconObjectID != cInvalidObjectID)
         getVar(cOutputObjectList)->asObjectList()->addObject(createdIconObjectID);
   }
}

//==============================================================================
// Provide the parameters for the follow camera mode
//==============================================================================
void BTriggerEffect::teSetFollowCam()
{
   enum
   {
      cFollowUnit = 1,
      cFollowSquad = 2,
      cFollowObject = 3,
      cDirection = 4,
      cEnable = 5,
   };

   bool unitUsed = getVar(cFollowUnit)->isUsed();
   bool squadUsed = getVar(cFollowSquad)->isUsed();
   bool objectUsed = getVar(cFollowObject)->isUsed();

   if (!unitUsed && !squadUsed && !objectUsed)
   {
      BTRIGGER_ASSERTM(false, "No unit, squad, or object assigned!");
      return;
   }

   // Collect entity ID
   BEntityID entity = cInvalidObjectID;
   if (unitUsed)
   {
      entity = getVar(cFollowUnit)->asUnit()->readVar();
   }
   else if(squadUsed)
   {
      entity = getVar(cFollowSquad)->asSquad()->readVar();
   }
   else if(objectUsed)
   {
      entity = getVar(cFollowObject)->asObject()->readVar();
   }
   BEntity* pEntity = gWorld->getEntity(entity);
   if (!pEntity)
   {
      BTRIGGER_ASSERTM(false, "Invalid entity pointer!");
      return;
   }

   // Collect direction
   BVector dir = getVar(cDirection)->isUsed() ? getVar(cDirection)->asVector()->readVar() : cInvalidVector;   
   int numUsers = gUserManager.getUserCount();
   for (int i = 0; i < numUsers; i++)
   {      
      BUser* pUser = gUserManager.getUser(i);
      if (!pUser || !pUser->getFlagUserActive())
      {
         return;
      }

      // Set follow entity
      pUser->setFollowEntity(entity);

      BCamera* pCamera = pUser->getCamera();
      if (!pCamera)
      {
         BTRIGGER_ASSERTM(false, "Invalid camera pointer!");
         return;
      }   

      // Set direction
      if (dir != cInvalidVector)
      {
         const BVector pos = pEntity->getPosition();
         const BVector& camDir = pCamera->getCameraDir();
         BVector tempDir = camDir;
         tempDir.y = 0.0f;
         dir.y = 0.0f;
         tempDir.normalize();
         dir.normalize();      
         float cosAngle = dir.dot(tempDir);
         float angle = acosf(cosAngle);
         pCamera->yawWorldAbout(angle, pos);            
      }

      // Enable follow mode
      bool follow = getVar(cEnable)->isUsed() ? getVar(cEnable)->asBool()->readVar() : false;
      if (follow)
      {
         pUser->changeMode(BUser::cUserModeFollow);
      }
      else
      {
         pUser->changeMode(BUser::cUserModeNormal);
      }
   }
}

//==============================================================================
// Enable/Disable the follow camera mode
//==============================================================================
void BTriggerEffect::teEnableFollowCam()
{
   enum
   {
      cEnable = 1,
   };

   int numUsers = gUserManager.getUserCount();
   for (int i = 0; i < numUsers; i++)
   {
      BUser* pUser = gUserManager.getUser(i);
      if (!pUser || !pUser->getFlagUserActive())
      {
         return;
      }

      bool follow = getVar(cEnable)->asBool()->readVar();
      if (follow)
      {
         pUser->changeMode(BUser::cUserModeFollow);
      }
      else
      {
         pUser->changeMode(BUser::cUserModeNormal);
      }
   }
}

//XXXHalwes - 7/17/2007 - Shouldn't we just be working on the garrisoned squads?
//==============================================================================
//==============================================================================
void BTriggerEffect::teGetGarrisonedUnits()
{
   enum { cContainerUnit = 1, cGarrisonedUnits = 2, cNumGarrisoned = 3, };
   BEntityID containerUnitID = getVar(cContainerUnit)->asUnit()->readVar();
   BTriggerVar* pGarrisonedUnits = getVar(cGarrisonedUnits);
   BTriggerVar* pNumGarrisoned = getVar(cNumGarrisoned);
   BTRIGGER_ASSERTM(pGarrisonedUnits->isUsed() || pNumGarrisoned->isUsed(), "No output parameters selected for GetGarrisonedUnits effect.");

   BEntityIDArray garrisonedUnits;
   BUnit* pContainerUnit = gWorld->getUnit(containerUnitID);
   if (pContainerUnit)
   {
      long numEntityRefs = pContainerUnit->getNumberEntityRefs();
      for (long i=0; i<numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 5437
         const BEntityRef* pEntityRef = pContainerUnit->getEntityRefByIndex(i);
//--
         if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
         {
//-- FIXING PREFIX BUG ID 5436
            const BUnit* pContainedUnit = gWorld->getUnit(pEntityRef->mID);
//--
            if (pContainedUnit)
               garrisonedUnits.add(pContainedUnit->getID());
         }
      }
   }

   if (pGarrisonedUnits)
      pGarrisonedUnits->asUnitList()->writeVar(garrisonedUnits);

   if (pNumGarrisoned)
      pNumGarrisoned->asInteger()->writeVar(garrisonedUnits.getNumber());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetDifficulty()
{
   switch (getVersion())
   {
      case 1:
      {
         teGetDifficultyV1();
         break;
      }
      case 2:
      {
         teGetDifficultyV2();
         break;
      }
      default:
      {
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetDifficultyV1()
{
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetDifficultyV2()
{
   enum { cPlayer = 1, cDifficultyLevel = 2, cDifficultyFloat = 3, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   BTriggerVarFloat* pDiffFloat = getVar(cDifficultyFloat)->asFloat();
   if (pDiffFloat->isUsed())
      pDiffFloat->writeVar(pPlayer->getDifficulty());
}


//=============================================================================
// BTriggerEffect::teHUDToggle
//=============================================================================
void BTriggerEffect::teHUDToggle()
{
   enum { cHUDItem = 1, cOnOff = 2, };

   int hudItem = getVar(cHUDItem)->asHUDItem()->readVar();
   if (hudItem == -1)
      return;

   bool onoff = getVar(cOnOff)->asBool()->readVar();

   gUserManager.getPrimaryUser()->setHUDItemEnabled(hudItem, onoff);
   if (gGame.isSplitScreen())
      gUserManager.getSecondaryUser()->setHUDItemEnabled(hudItem, onoff);
}

//==============================================================================
// BTriggerEffect::teInputUIButton()
//==============================================================================
void BTriggerEffect::teInputUIButton()
{
   enum { cInputPlayer=1, cInputControlType=2, cOutputUIButton=3, cSpeedModifier=4, cActionModifier=5, cOverride=6, cOnRelease=7, cContinuous=8 };

   getVar(cOutputUIButton)->asUIButton()->writeResult(BTriggerVarUIButton::cUIButtonResultWaiting, 0.0f, 0.0f, 0.0f, false, false);

   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   int controlType = getVar(cInputControlType)->asControlType()->readVar();
   BTriggerVarID uiButtonVarID = getVar(cOutputUIButton)->getID();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();

   bool speedModifier=false;
   bool ignoreSpeedModifier=true;
   bool actionModifier=false;
   bool ignoreActionModifier=true;
   bool overrideGameInput=false;
   bool onRelease=false;
   bool continuous=false;

   if (getVersion() > 1)
   {
      if (getVar(cSpeedModifier)->isUsed())
      {
         speedModifier = getVar(cSpeedModifier)->asBool()->readVar();
         ignoreSpeedModifier=false;
      }
      if (getVar(cActionModifier)->isUsed())
      {
         actionModifier = getVar(cActionModifier)->asBool()->readVar();
         ignoreActionModifier=false;
      }
      if (getVar(cOverride)->isUsed())
         overrideGameInput = getVar(cOverride)->asBool()->readVar();
      if (getVar(cOnRelease)->isUsed())
         onRelease = getVar(cOnRelease)->asBool()->readVar();
      if (getVar(cContinuous)->isUsed())
         continuous = getVar(cContinuous)->asBool()->readVar();
   }

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      BUser* pUser = pPlayer->getUser();
      if (pUser)
         pUser->addTriggerUIButtonRequest(triggerScriptID, uiButtonVarID, controlType, speedModifier, ignoreSpeedModifier, actionModifier, ignoreActionModifier, overrideGameInput, onRelease, continuous);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teSetRenderTerrainSkirt()
{
   enum { cRender = 1, };

   bool onoff = getVar(cRender)->asBool()->readVar();
   gTerrain.setRenderSkirt(onoff);
   gWorld->setFlagShowSkirt(onoff);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teSetOccluded()
{
   enum { cLocation=1, cDistance=2, cBool=3, };

   BVector location = getVar(cLocation)->asVector()->readVar();
   float distance = getVar(cDistance)->asFloat()->readVar();
   bool occluded = getVar(cBool)->asBool()->readVar();

   gWorld->setOccluded(location, distance, occluded);
}

//==============================================================================
// Set the position of an object
//==============================================================================
void BTriggerEffect::teSetPosition()
{
   switch (getVersion())
   {
      case 2:
         teSetPositionV2();
         break;

      case 3:
         teSetPositionV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Set the position of an object
//==============================================================================
void BTriggerEffect::teSetPositionV2()
{
   enum
   {      
      cPos = 2,
      cObject = 3,
   };

   BObject* pObject = gWorld->getObject(getVar(cObject)->asObject()->readVar());
   if (pObject)
   {
      #ifdef SYNC_Unit
         if (pObject->isClassType(BEntity::cClassTypeUnit))
            syncUnitData("BTriggerEffect::teSetPositionV2", getVar(cPos)->asVector()->readVar());
      #endif
      pObject->setPosition(getVar(cPos)->asVector()->readVar());
   }
}

//==============================================================================
// Set the position of an object or unit
//==============================================================================
void BTriggerEffect::teSetPositionV3()
{
   enum
   {      
      cPos = 2,
      cObject = 3,
      cUnit = 4,
   };

   BEntityID entityID = cInvalidObjectID;
   if (getVar(cObject)->isUsed())
      entityID = getVar(cObject)->asObject()->readVar();
   else if (getVar(cUnit)->isUsed())
      entityID = getVar(cUnit)->asUnit()->readVar();

   BEntity* pEntity = gWorld->getEntity(entityID);
   if (pEntity)
   {
      #ifdef SYNC_Unit
         if (pEntity->isClassType(BEntity::cClassTypeUnit))
            syncUnitData("BTriggerEffect::teSetPositionV2", getVar(cPos)->asVector()->readVar());
      #endif
      pEntity->setPosition(getVar(cPos)->asVector()->readVar());
   }
}

//==============================================================================
// Sets light animation
//==============================================================================
void BTriggerEffect::teLightsetAnimate()
{
   enum { cLightsetID = 1, cTime = 2};

   DWORD duration = getVar(cTime)->asTime()->readVar();
   DWORD currentGametime = gWorld->getGametime();
   //BSimUString pString = getVar(cFilename)->asString()->readVar();
   uint count = getVar(cLightsetID)->asInteger()->readVar();  
   const BRawLightSettings* pA = &gSimSceneLightManager.getCurrentLightSet(); ///= gSimSceneLightManager.getCurrentRawSettings()
   const BRawLightSettings* pB = gSimSceneLightManager.getLightSet(count);

   
   if(pA != NULL && pB != NULL)
   {
      gSimSceneLightManager.setAnimation(*pA, *pB, currentGametime, duration);     
   }

}

//XXXHalwes - 7/17/2007 - Added during E3, but not presently used.  May be used in the future.  Marked as dev only.
//==============================================================================
// sets the flag near layer
//==============================================================================
void BTriggerEffect::teSetFlagNearLayer()
{
   enum { cUnit = 1, cState = 2};
   
   bool enable = getVar(cState)->asBool()->readVar();   
   BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());

   if (pUnit)
   {
      pUnit->setFlagNearLayer(enable);
   }
}





//==============================================================================
//==============================================================================
void BTriggerEffect::teLocationAdjustDir()
{
   enum { cSourceLocation = 1, cDirection = 2, cDistance = 3, cResultLocation = 4, };
   BVector sourceLocation = getVar(cSourceLocation)->asVector()->readVar();
   BVector direction = getVar(cDirection)->asVector()->readVar();
   direction.normalize();
   float distance = getVar(cDistance)->asFloat()->readVar();
   BVector resultLocation = sourceLocation + (direction * distance);
   // Validate result
   if ((resultLocation.x < gWorld->getSimBoundsMinX()) || (resultLocation.x > gWorld->getSimBoundsMaxX()) ||
       (resultLocation.z < gWorld->getSimBoundsMinZ()) || (resultLocation.z > gWorld->getSimBoundsMaxZ()))
   {
      BVector clampLoc = resultLocation;
      // Check for case where both -x, and -z
      if ((clampLoc.x < gWorld->getSimBoundsMinX()) && (clampLoc.z < gWorld->getSimBoundsMinZ()))
      {
         // Clamp to z
         if (clampLoc.x < clampLoc.z)
            clampLoc.z = gWorld->getSimBoundsMinZ();
         // Clamp to x
         else
            clampLoc.x = gWorld->getSimBoundsMinX();
      }
      else if ((clampLoc.x > gWorld->getSimBoundsMaxX()) && (clampLoc.z > gWorld->getSimBoundsMaxZ()))
      {
         // Clamp to z
         if (clampLoc.x > clampLoc.z)
            clampLoc.z = gWorld->getSimBoundsMaxZ();
         // Clamp to x
         else 
            clampLoc.x = gWorld->getSimBoundsMaxX();
      }
      else
      {
         clampLoc.x = Math::Clamp(clampLoc.x, gWorld->getSimBoundsMinX(), gWorld->getSimBoundsMaxX());
         clampLoc.z = Math::Clamp(clampLoc.z, gWorld->getSimBoundsMinZ(), gWorld->getSimBoundsMaxZ());
      }
      BVector clampDir = clampLoc - resultLocation;
      float clampDist = clampDir.length();
      clampDir.normalize();
      BVector newResultDir = -direction;
      float angle = newResultDir.dot(clampDir);
      float delta = clampDist / angle;
      delta += 0.01f; // Fudge
      resultLocation = sourceLocation + (direction * (distance - delta));
   }
   getVar(cResultLocation)->asVector()->writeVar(resultLocation);
}

//XXXHalwes - 7/17/2007 - This justs sets a flag, should we consolidate to the flag set trigger or have individual triggers and remove the
//                        flag set trigger.
// DMG - 4/29/08 - Is this even relevant to the new cloaking mechanism?
//============================================================================================
// Sets whether the assigned unit, unit list, squad, or squad list has had its cloak detected
//============================================================================================
void BTriggerEffect::teCloakDetected()
{
   enum
   {
      cUnit = 1,
      cUnitList = 2,
      cSquad = 3,
      cSquadList = 4,
      cDetected = 5,
   };

   bool unitUsed = getVar(cUnit)->isUsed();
   bool unitListUsed = getVar(cUnitList)->isUsed();
   bool squadUsed = getVar(cSquad)->isUsed();
   bool squadListUsed = getVar(cSquadList)->isUsed();

   if (!unitUsed && !unitListUsed && !squadUsed && !squadListUsed)
   {
      BTRIGGER_ASSERTM( false, "No unit, unit list, squad, or squad list assigned!");
      return;
   }

   // Collect units
   BEntityIDArray unitList;
   if (unitListUsed)
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (unitUsed)
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   if (squadListUsed)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      uint numSquads = (uint)squadList.getNumber();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5427
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint j = 0; j < numChildren; j++)
            {
               unitList.uniqueAdd(pSquad->getChild(j));
            }
         }
      }
   }

   if (squadUsed)
   {
//-- FIXING PREFIX BUG ID 5428
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            unitList.uniqueAdd(pSquad->getChild(i));
         }
      }
   }

   // Detect units
   /*
   bool detected = getVar(cDetected)->asBool()->readVar();
   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 5438
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         pSquad->notify(BEntity::cEventDetected,NULL, NULL, NULL );
      }
   }
   */
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCountToInt()
{
   enum { cCount = 1, cInt = 2, };
   int32 intval = getVar(cCount)->asInteger()->readVar();
   getVar(cInt)->asInteger()->writeVar(intval);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teIntToCount()
{
   enum { cInt = 1, cCount = 2, };
   long countval = getVar(cInt)->asInteger()->readVar();
   getVar(cCount)->asInteger()->writeVar(countval);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teProtoObjectListAdd()
{
   enum { cAddProto = 1, cAddList = 2, cClearExisting = 3, cDestList = 4, };
//-- FIXING PREFIX BUG ID 5440
   const BTriggerVarProtoObject* pAddProto = getVar(cAddProto)->asProtoObject();
//--
   BTriggerVarProtoObjectList* pAddList = getVar(cAddList)->asProtoObjectList();
//-- FIXING PREFIX BUG ID 5441
   const BTriggerVarBool* pClearExisting = getVar(cClearExisting)->asBool();
//--
   BTriggerVarProtoObjectList* pDestList = getVar(cDestList)->asProtoObjectList();
   BProtoObjectIDArray workingList = pDestList->readVar();
   if (pClearExisting->isUsed() && pClearExisting->readVar())
      workingList.clear();
   if (pAddList->isUsed())
      workingList.append(pAddList->readVar());
   if (pAddProto->isUsed())
      workingList.add(pAddProto->readVar());
   pDestList->writeVar(workingList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teProtoObjectListRemove()
{
   enum { cRemoveOne = 1, cRemoveList = 2, cRemoveAll = 3, cModifyList = 4, };
//-- FIXING PREFIX BUG ID 5442
   const BTriggerVarProtoObject* pRemoveOne = getVar(cRemoveOne)->asProtoObject();
//--
   BTriggerVarProtoObjectList* pRemoveList = getVar(cRemoveList)->asProtoObjectList();
//-- FIXING PREFIX BUG ID 5443
   const BTriggerVarBool* pRemoveAll = getVar(cRemoveAll)->asBool();
//--
   BTriggerVarProtoObjectList* pModifyList = getVar(cModifyList)->asProtoObjectList();
   // EASY!
   if (pRemoveAll->isUsed() && pRemoveAll->readVar() == true)
   {
      pModifyList->clear();
      return;
   }
   if (pRemoveOne->isUsed())
      pModifyList->removeProtoObjectID(pRemoveOne->readVar());
   if (pRemoveList->isUsed())
   {
      const BProtoObjectIDArray& removeList = pRemoveList->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i=0; i<numToRemove; i++)
         pModifyList->removeProtoObjectID(removeList[i]);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teProtoSquadListAdd()
{
   enum 
   { 
      cAddProto = 1, 
      cAddList = 2, 
      cClearExisting = 3, 
      cDestList = 4, 
   };

//-- FIXING PREFIX BUG ID 5444
   const BTriggerVarProtoSquad* pAddProto = getVar(cAddProto)->asProtoSquad();
//--
   BTriggerVarProtoSquadList* pAddList = getVar(cAddList)->asProtoSquadList();
//-- FIXING PREFIX BUG ID 5445
   const BTriggerVarBool* pClearExisting = getVar(cClearExisting)->asBool();
//--
   BTriggerVarProtoSquadList* pDestList = getVar(cDestList)->asProtoSquadList();
   BProtoSquadIDArray workingList = pDestList->readVar();
   if (pClearExisting->isUsed() && pClearExisting->readVar())
      workingList.clear();
   
   if (pAddList->isUsed())
      workingList.append(pAddList->readVar());
   if (pAddProto->isUsed())
      workingList.add(pAddProto->readVar());
   
   pDestList->writeVar(workingList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teProtoSquadListRemove()
{
   enum { cRemoveOne = 1, cRemoveList = 2, cRemoveAll = 3, cModifyList = 4, };
//-- FIXING PREFIX BUG ID 5446
   const BTriggerVarProtoSquad* pRemoveOne = getVar(cRemoveOne)->asProtoSquad();
//--
   BTriggerVarProtoSquadList* pRemoveList = getVar(cRemoveList)->asProtoSquadList();
//-- FIXING PREFIX BUG ID 5447
   const BTriggerVarBool* pRemoveAll = getVar(cRemoveAll)->asBool();
//--
   BTriggerVarProtoSquadList* pModifyList = getVar(cModifyList)->asProtoSquadList();
   // EASY!
   if (pRemoveAll->isUsed() && pRemoveAll->readVar() == true)
   {
      pModifyList->clear();
      return;
   }
   if (pRemoveOne->isUsed())
      pModifyList->removeProtoSquadID(pRemoveOne->readVar());
   if (pRemoveList->isUsed())
   {
      const BProtoSquadIDArray& removeList = pRemoveList->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i=0; i<numToRemove; i++)
         pModifyList->removeProtoSquadID(removeList[i]);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teTechListAdd()
{
   enum { cAddTech = 1, cAddList = 2, cClearExisting = 3, cDestList = 4, };
//-- FIXING PREFIX BUG ID 5448
   const BTriggerVarTech* pAddTech = getVar(cAddTech)->asTech();
//--
   BTriggerVarTechList* pAddList = getVar(cAddList)->asTechList();
//-- FIXING PREFIX BUG ID 5449
   const BTriggerVarBool* pClearExisting = getVar(cClearExisting)->asBool();
//--
   BTriggerVarTechList* pDestList = getVar(cDestList)->asTechList();
   BTechIDArray workingList = pDestList->readVar();
   // The easy case.
   if (pClearExisting->isUsed() && pClearExisting->readVar())
      workingList.clear();
   if (pAddList->isUsed())
      workingList.append(pAddList->readVar());
   if (pAddTech->isUsed())
      workingList.add(pAddTech->readVar());
   pDestList->writeVar(workingList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teTechListRemove()
{
   enum { cRemoveOne = 1, cRemoveList = 2, cRemoveAll = 3, cModifyList = 4, };
//-- FIXING PREFIX BUG ID 5450
   const BTriggerVarTech* pRemoveOne = getVar(cRemoveOne)->asTech();
//--
   BTriggerVarTechList* pRemoveList = getVar(cRemoveList)->asTechList();
//-- FIXING PREFIX BUG ID 5451
   const BTriggerVarBool* pRemoveAll = getVar(cRemoveAll)->asBool();
//--
   BTriggerVarTechList* pModifyList = getVar(cModifyList)->asTechList();
   // EASY!
   if (pRemoveAll->isUsed() && pRemoveAll->readVar() == true)
   {
      pModifyList->clear();
      return;
   }
   if (pRemoveOne->isUsed())
      pModifyList->removeTechID(pRemoveOne->readVar());
   if (pRemoveList->isUsed())
   {
      const BTechIDArray& removeList = pRemoveList->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i=0; i<numToRemove; i++)
         pModifyList->removeTechID(removeList[i]);
   }
}


//==============================================================================
// Add an integer to the integer list
//==============================================================================
void BTriggerEffect::teIntegerListAdd()
{
   enum 
   { 
      cAddInt = 1, 
      cAddList = 2, 
      cClearExisting = 3, 
      cDestList = 4, 
   };

   BInt32Array destList = getVar(cDestList)->asIntegerList()->readVar();

   if (getVar(cClearExisting)->isUsed())
   {
      if(getVar(cClearExisting)->asBool()->readVar() == true)
         destList.clear();
   }

   if (getVar(cAddInt)->isUsed())
   {
      destList.add(getVar(cAddInt)->asInteger()->readVar());
   }

   if (getVar(cAddList)->isUsed())
   {
      BInt32Array addList = getVar(cAddList)->asIntegerList()->readVar();
      uint numInts = addList.getSize();
      for (uint i = 0; i < numInts; i++)
      {
         destList.add(addList[i]);
      }
   }

   getVar(cDestList)->asIntegerList()->writeVar(destList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teIntegerListRemove()
{
   enum { cRemoveOne = 1, cRemoveList = 2, cRemoveAll = 3, cRemoveDupes = 4, cModifyList = 5, };
//-- FIXING PREFIX BUG ID 5452
   const BTriggerVarInteger* pRemoveOne = getVar(cRemoveOne)->asInteger();
//--
   BTriggerVarIntegerList* pRemoveList = getVar(cRemoveList)->asIntegerList();
//-- FIXING PREFIX BUG ID 5453
   const BTriggerVarBool* pRemoveAll = getVar(cRemoveAll)->asBool();
//--
//-- FIXING PREFIX BUG ID 5454
   const BTriggerVarBool* pRemoveDupes = getVar(cRemoveDupes)->asBool();
//--
   BTriggerVarIntegerList* pModifyList = getVar(cModifyList)->asIntegerList();
   bool removeDupes = (pRemoveDupes->isUsed() && pRemoveDupes->readVar() == true);

   // EASY!
   if (pRemoveAll->isUsed() && pRemoveAll->readVar() == true)
   {
      pModifyList->clear();
      return;
   }
   if (pRemoveOne->isUsed())
   {
      if (removeDupes)
         pModifyList->removeIntAll(pRemoveOne->readVar());
      else
         pModifyList->removeInt(pRemoveOne->readVar());
   }
   if (pRemoveList->isUsed())
   {
      const BInt32Array& removeList = pRemoveList->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i=0; i<numToRemove; i++)
      {
         if (removeDupes)
            pModifyList->removeIntAll(removeList[i]);
         else
            pModifyList->removeInt(removeList[i]);
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teIntegerListGetSize()
{
   enum { cIntList = 1, cSize = 2, };
   const BInt32Array& intList = getVar(cIntList)->asIntegerList()->readVar();
   getVar(cSize)->asInteger()->writeVar(intList.getSize());
}


//==============================================================================
// Use correct version
//==============================================================================
void BTriggerEffect::teBuildingCommand()
{
   switch (getVersion())
   {
      case 3:
         teBuildingCommandV3();
         break;

      case 4:
         teBuildingCommandV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Train, research, or build from a building
//==============================================================================
void BTriggerEffect::teBuildingCommandV3()
{
   enum { cUnit = 1, cProtoSquad = 2, cCount = 3, cNoCost = 4, cOutputState = 5, cTech = 6 };

   BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
   BProtoSquadID protoSquadID = (getVar(cProtoSquad)->isUsed() ? getVar(cProtoSquad)->asProtoSquad()->readVar() : -1);
   int protoTechID = (getVar(cTech)->isUsed() ? getVar(cTech)->asTech()->readVar() : -1);
   int count = (getVar(cCount)->isUsed() ? getVar(cCount)->asInteger()->readVar() : 1);
   bool noCost = (getVar(cNoCost)->isUsed() ? getVar(cNoCost)->asBool()->readVar() : false);
   BTriggerScriptID triggerScriptID = cInvalidTriggerScriptID;
   BTriggerVarID triggerVarID = cInvalidTriggerVarID;

   if (getVar(cOutputState)->isUsed())
   {
      BTriggerVarBuildingCommandState* pState = getVar(cOutputState)->asBuildingCommandState();
      pState->writeResult(BTriggerVarBuildingCommandState::cResultWaiting);
      pState->clearTrainedSquadIDs();
      triggerScriptID = getParentTriggerScript()->getID();
      triggerVarID = pState->getID();
   }

   bool okay=false;

   BUnit* pUnit = gWorld->getUnit(unitID);
   if (pUnit)
   {
      if (protoSquadID != -1)
         okay = pUnit->doTrain(pUnit->getPlayerID(), protoSquadID, count, true, noCost, triggerScriptID, triggerVarID);
      else if (protoTechID != -1)
         okay = pUnit->doResearch(pUnit->getPlayerID(), protoTechID, count, noCost, triggerScriptID, triggerVarID);
   }

   if (!okay)
   {
      if (getVar(cOutputState)->isUsed())
      {
         BTriggerVarBuildingCommandState* pState = getVar(cOutputState)->asBuildingCommandState();
         pState->writeResult(BTriggerVarBuildingCommandState::cResultDone);
      }
   }
}

//==============================================================================
// Train, research, or build from a building
//==============================================================================
void BTriggerEffect::teBuildingCommandV4()
{
   enum { cUnit = 1, cProtoSquad = 2, cCount = 3, cNoCost = 4, cOutputState = 5, cTech = 6, cProtoBulding = 7, cPlayer = 8, };

   BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
   BProtoSquadID protoSquadID = (getVar(cProtoSquad)->isUsed() ? getVar(cProtoSquad)->asProtoSquad()->readVar() : -1);
   int protoTechID = (getVar(cTech)->isUsed() ? getVar(cTech)->asTech()->readVar() : -1);
   BProtoObjectID protoBuildingID = (getVar(cProtoBulding)->isUsed() ? getVar(cProtoBulding)->asProtoObject()->readVar() : -1);
   int count = (getVar(cCount)->isUsed() ? getVar(cCount)->asInteger()->readVar() : 1);
   bool noCost = (getVar(cNoCost)->isUsed() ? getVar(cNoCost)->asBool()->readVar() : false);
   BPlayerID playerID = (getVar(cPlayer)->isUsed() ? getVar(cPlayer)->asPlayer()->readVar() : cInvalidPlayerID);
   BTriggerScriptID triggerScriptID = cInvalidTriggerScriptID;
   BTriggerVarID triggerVarID = cInvalidTriggerVarID;

   if (getVar(cOutputState)->isUsed())
   {
      BTriggerVarBuildingCommandState* pState = getVar(cOutputState)->asBuildingCommandState();
      pState->writeResult(BTriggerVarBuildingCommandState::cResultWaiting);
      pState->clearTrainedSquadIDs();
      triggerScriptID = getParentTriggerScript()->getID();
      triggerVarID = pState->getID();
   }

   bool okay=false;

   BUnit* pUnit = gWorld->getUnit(unitID);
   if (pUnit)
   {
      if (playerID == cInvalidPlayerID)
         playerID = pUnit->getPlayerID();
      if (protoSquadID != -1)
         okay = pUnit->doTrain(playerID, protoSquadID, count, true, noCost, triggerScriptID, triggerVarID);
      else if (protoTechID != -1)
         okay = pUnit->doResearch(playerID, protoTechID, count, noCost, triggerScriptID, triggerVarID);
      else if (protoBuildingID != -1)
      {
         // Base socket support. Allow the main base to be built from the main base socket. Allow child buildings to be built
         // by specifying either the main base socket or the main base building and auto selecting a socket.
         BUnit* pBuildFromUnit = NULL;
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         for (uint i=0; i<pProtoObject->getNumberCommands(); i++)
         {
            BProtoObjectCommand cmd = pProtoObject->getCommand(i);
            if (cmd.getType() == BProtoObjectCommand::cTypeBuildOther && cmd.getID() == protoBuildingID)
            {
               pBuildFromUnit = pUnit;
               break;
            }
         }
         if (!pBuildFromUnit)
         {
//-- FIXING PREFIX BUG ID 5458
            const BUnit* pBaseSocket = NULL;
//--
            if (pUnit->getProtoObject()->isType(gDatabase.getOTIDSettlement()))
               pBaseSocket = pUnit;
            if (pUnit->getProtoObject()->isType(gDatabase.getOTIDBase()))
            {
//-- FIXING PREFIX BUG ID 5455
               const BEntityRef* pBaseSocketRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeAssociatedSettlement);
//--
               if (pBaseSocketRef)
                  pBaseSocket = gWorld->getUnit(pBaseSocketRef->mID);
            }
            if (pBaseSocket)
            {
//-- FIXING PREFIX BUG ID 5457
               const BEntityRef* pBaseBuildingRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug);
//--
               if (pBaseBuildingRef)
               {
                  BUnit* pBaseBuilding = gWorld->getUnit(pBaseBuildingRef->mID);
                  uint numEntityRefs = pBaseBuilding->getNumberEntityRefs();
                  for (uint j=0; j<numEntityRefs; j++)
                  {
//-- FIXING PREFIX BUG ID 5456
                     const BEntityRef* pRef = pBaseBuilding->getEntityRefByIndex(j);
//--
                     if (pRef && pRef->mType == BEntityRef::cTypeAssociatedSocket)
                     {
                        BUnit* pChildSocket = gWorld->getUnit(pRef->mID);
                        if (pChildSocket && !pChildSocket->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug))
                        {
                           const BProtoObject* pProtoObject = pChildSocket->getProtoObject();
                           for (uint i=0; i<pProtoObject->getNumberCommands(); i++)
                           {
                              BProtoObjectCommand cmd = pProtoObject->getCommand(i);
                              if (cmd.getType() == BProtoObjectCommand::cTypeBuildOther && cmd.getID() == protoBuildingID)
                              {
                                 pBuildFromUnit = pChildSocket;
                                 break;
                              }
                           }
                           break;
                        }
                     }
                  }
               }
            }
         }
         if (pBuildFromUnit)
            okay = pBuildFromUnit->doBuildOther(playerID, playerID, protoBuildingID, false, noCost, false, triggerScriptID, triggerVarID);
      }
   }

   if (!okay)
   {
      if (getVar(cOutputState)->isUsed())
      {
         BTriggerVarBuildingCommandState* pState = getVar(cOutputState)->asBuildingCommandState();
         pState->writeResult(BTriggerVarBuildingCommandState::cResultDone);
      }
   }
}

//==============================================================================
// Select correct version
//==============================================================================
void BTriggerEffect::tePlayAnimationObject()
{
   switch (getVersion())
   {
      case 2:
         tePlayAnimationObjectV2();
         break;

      case 3:
         tePlayAnimationObjectV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Play an animation on an object
//==============================================================================
void BTriggerEffect::tePlayAnimationObjectV2()
{
   enum 
   { 
      cObject = 1, 
      cAnimType = 2, 
      cOutputAnimDuration = 6, 
   };

   long animType = getVar(cAnimType)->asAnimType()->readVar();
   BEntityID objectID = getVar(cObject)->asObject()->readVar();
   DWORD duration = 0;
   BObject* pObject = gWorld->getObject(objectID);
   if (pObject)
   {
      // Remove idle action so that idle durations are not calculated incorrectly, objects do NOT automatically go idle so an idle animation must be set manually
      pObject->removeAllActionsOfType(BAction::cActionTypeEntityIdle);
      pObject->unlockAnimation();
      pObject->setAnimationState(BObjectAnimationState::cAnimationStateMisc, animType, true);
      duration = pObject->getAnimationDurationDWORD(cActionAnimationTrack);
      pObject->lockAnimation(duration, true);      
   }

   if (getVar(cOutputAnimDuration)->isUsed())
   {
      getVar(cOutputAnimDuration)->asTime()->writeVar(duration);
   }
}

//==============================================================================
// Play an animation on an object
//==============================================================================
void BTriggerEffect::tePlayAnimationObjectV3()
{
   enum 
   { 
      cObject = 1, 
      cAnimType = 2, 
      cOutputAnimDuration = 6, 
      cInputAnimDuration = 7, 
   };

   long animType = getVar(cAnimType)->asAnimType()->readVar();
   BEntityID objectID = getVar(cObject)->asObject()->readVar();
   DWORD duration = 0;
   BObject* pObject = gWorld->getObject(objectID);
   if (pObject)
   {
      // Remove idle action so that idle durations are not calculated incorrectly, objects do NOT automatically go idle so an idle animation must be set manually
      pObject->removeAllActionsOfType(BAction::cActionTypeEntityIdle);
      pObject->unlockAnimation();
      pObject->setAnimationState(BObjectAnimationState::cAnimationStateMisc, animType, true);
      duration = getVar(cInputAnimDuration)->isUsed() ? getVar(cInputAnimDuration)->asTime()->readVar() : pObject->getAnimationDurationDWORD(cActionAnimationTrack);
      pObject->lockAnimation(duration, true);      
   }

   if (getVar(cOutputAnimDuration)->isUsed())
   {
      getVar(cOutputAnimDuration)->asTime()->writeVar(duration);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetDeadUnitCount()
{
   enum { cPlayer = 1, cObjectType = 2, cCount = 3, };

   int32 numDeadUnits = 0;
//-- FIXING PREFIX BUG ID 5461
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
   if (pPlayer)
   {
//-- FIXING PREFIX BUG ID 5460
      const BTriggerVarObjectType* pVarObjectType = getVar(cObjectType)->asObjectType();
//--
      if (pVarObjectType->isUsed())
         numDeadUnits = pPlayer->getDeadUnitCount(pVarObjectType->readVar());
      else
         numDeadUnits = pPlayer->getTotalDeadUnitCount();
   }

   getVar(cCount)->asInteger()->writeVar(numDeadUnits);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetDeadSquadCount()
{
   enum { cPlayer = 1, cProtoSquad = 2, cCount = 3, };

   int32 numDeadSquads = 0;
//-- FIXING PREFIX BUG ID 5463
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
   if (pPlayer)
   {
//-- FIXING PREFIX BUG ID 5462
      const BTriggerVarProtoSquad* pVarProtoSquad = getVar(cProtoSquad)->asProtoSquad();
//--
      if (pVarProtoSquad->isUsed())
         numDeadSquads = pPlayer->getDeadSquadCount(pVarProtoSquad->readVar());
      else
         numDeadSquads = pPlayer->getTotalDeadSquadCount();
   }

   getVar(cCount)->asInteger()->writeVar(numDeadSquads);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBreakpoint()
{
#ifndef BUILD_FINAL
   if (DmIsDebuggerPresent())
   {
      // SINGLE STEP HERE ONCE BEFORE STOPPING, OTHERWISE THE DEVKIT WILL HARD REBOOT!
      breakpoint;
   }
#endif
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPrimaryUser()
{
   enum { cPlayer = 1, };
/*
   const BPlayer* pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   BTRIGGER_ASSERT(pPlayer);
   const BAI* pAI = pPlayer->getAI();
   BTRIGGER_ASSERT(pAI);
   const BPlayerID playerID = pAI->getPlayerID();

   getVar(cPlayer)->asPlayer()->writeVar(playerID);*/
}


//==============================================================================
// Enable the user message index
//==============================================================================
void BTriggerEffect::teEnableUserMessage()
{
   enum
   {
      cIndex      = 1,
      cEnabled    = 2,
   };

   long index = getVar(cIndex)->asMessageIndex()->readVar();
   bool enabled = getVar(cEnabled)->asBool()->readVar();

   BPlayerIDArray playerList;
   playerList.clear();
   BSimUString string = "NULL";
   BColor nullColor;
   nullColor.set(-1.0f, -1.0f, -1.0f);

   BUser::setUserMessage(index, playerList, &string, -1.0f, -1.0f, BUser::cUserMessageJustify_NULL, -1.0f, -1.0f, nullColor, enabled);
}

//================================================================================
// Get the proto squad ID for the provided squad
//================================================================================
void BTriggerEffect::teGetProtoSquad()
{
   enum
   {
      cInputSquad = 1,
      cOutputProtoSquad = 2,
   };

   BProtoSquadID protoSquadID = cInvalidProtoSquadID;
//-- FIXING PREFIX BUG ID 5439
   const BSquad* pSquad = gWorld->getSquad(getVar(cInputSquad)->asSquad()->readVar());
//--
   if (pSquad)
   {
      protoSquadID = pSquad->getProtoSquadID();
   }

   getVar(cOutputProtoSquad)->asProtoSquad()->writeVar(protoSquadID);      
}

//================================================================================
// Get the proto object ID for the provided unit or object
//================================================================================
void BTriggerEffect::teGetProtoObject()
{
   switch(getVersion())
   {
      case 1:
         teGetProtoObjectV1();
         break;

      case 2:
         teGetProtoObjectV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//================================================================================
// Get the proto object ID for the provided unit or object
//================================================================================
void BTriggerEffect::teGetProtoObjectV1()
{
   enum
   {
      cInputUnit = 1,
      cInputObject = 2,
      cOutputProtoObject = 3,
   };

   BProtoObjectID protoObjectID = cInvalidObjectID;
   if (getVar(cInputUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5465
      const BUnit* pUnit = gWorld->getUnit(getVar(cInputUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         protoObjectID = pUnit->getProtoID();
      }
   }
   else if (getVar(cInputObject)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5466
      const BObject* pObject = gWorld->getObject(getVar(cInputObject)->asObject()->readVar());
//--
      if (pObject)
      {
         protoObjectID = pObject->getProtoID();
      }
   }
   else
   {
      BTRIGGER_ASSERTM(false, "No Unit or Object designated!");
   }

   getVar(cOutputProtoObject)->asProtoObject()->writeVar(protoObjectID);
}

//================================================================================
// Get the proto object ID for the provided unit, object, or proto squad
//================================================================================
void BTriggerEffect::teGetProtoObjectV2()
{
   enum
   {
      cInputUnit = 1,
      cInputObject = 2,
      cOutputProtoObject = 3,
      cInputProtoSquad = 4,
   };

   BProtoObjectID protoObjectID = cInvalidObjectID;
   if (getVar(cInputUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5467
      const BUnit* pUnit = gWorld->getUnit(getVar(cInputUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         protoObjectID = pUnit->getProtoID();
      }
   }
   else if (getVar(cInputObject)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5468
      const BObject* pObject = gWorld->getObject(getVar(cInputObject)->asObject()->readVar());
//--
      if (pObject)
      {
         protoObjectID = pObject->getProtoID();
      }
   }
   else if (getVar(cInputProtoSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5469
      const BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(getVar(cInputProtoSquad)->asProtoSquad()->readVar());
//--
      if (pProtoSquad)
      {
         protoObjectID = pProtoSquad->getProtoObjectID();
      }
   }
   else
   {
      BTRIGGER_ASSERTM(false, "No Unit or Object designated!");
   }

   getVar(cOutputProtoObject)->asProtoObject()->writeVar(protoObjectID);
}

//==============================================================================
// Shake the camera based on an intensity and duration
//==============================================================================
void BTriggerEffect::teCameraShake()
{
   switch (getVersion())
   {
      case 1:
         teCameraShakeV1();
         break;

      case 2:
         teCameraShakeV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Shake the camera based on an intensity and duration
//==============================================================================
void BTriggerEffect::teCameraShakeV1()
{
   enum
   {
      cInputDuration = 1,
      cInputIntensity = 2,
      cInputTrailOffDuration = 3,
      cInputConservationFactor = 4,
   };

   int numUsers = gUserManager.getUserCount();
   for (int i = 0; i < numUsers; i++)
   {
      BUser* pUser = gUserManager.getUser(i);
      if (pUser && pUser->getFlagUserActive())
      {
         BCamera* pCamera = pUser->getCamera();
         if (pCamera)
         {
            DWORD duration = getVar(cInputDuration)->asTime()->readVar();
            float intensity = Math::Max(getVar(cInputIntensity)->asFloat()->readVar(), 0.0f);

            DWORD trailOffDuration = cDefaultShakeTrailOffDurationDWORD;
            if (getVar(cInputTrailOffDuration)->isUsed())
            {
               trailOffDuration = getVar(cInputTrailOffDuration)->asTime()->readVar();
            }

            float conservationFactor = cDefaultShakeConservationFactor;
            if (getVar(cInputConservationFactor)->isUsed())
            {
               conservationFactor = Math::Clamp(getVar(cInputConservationFactor)->asFloat()->readVar(), 0.0f, 1.0f);
            }

            pCamera->clearShake();
            pCamera->beginShake(duration, intensity, trailOffDuration, conservationFactor);
         }
      }
   }
}

//==============================================================================
// Shake the camera based on an intensity and duration
//==============================================================================
void BTriggerEffect::teCameraShakeV2()
{
   enum
   {
      cInputDuration = 1,
      cInputIntensity = 2,
      cInputTrailOffDuration = 3,
      cInputConservationFactor = 4,
      cInputPlayer = 5,
      cInputPlayerList = 6,
   };

   BPlayerIDArray playerIDs;
   if (getVar(cInputPlayerList)->isUsed())
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cInputPlayer)->isUsed())
   {
      playerIDs.uniqueAdd(getVar(cInputPlayer)->asPlayer()->readVar());
   }

   uint numPlayers = playerIDs.getSize();
   bool allUsers = false;
   if (numPlayers == 0)
   {
      numPlayers = (uint)gUserManager.getUserCount();
      allUsers = true;
   }

   for (uint i = 0; i < numPlayers; i++)
   {
      BUser* pUser = allUsers ? gUserManager.getUser(i) : gUserManager.getUserByPlayerID(playerIDs[i]);
      if (pUser && pUser->getFlagUserActive())
      {
         BCamera* pCamera = pUser->getCamera();
         if (pCamera)
         {
            DWORD duration = getVar(cInputDuration)->asTime()->readVar();
            float intensity = Math::Max(getVar(cInputIntensity)->asFloat()->readVar(), 0.0f);

            DWORD trailOffDuration = cDefaultShakeTrailOffDurationDWORD;
            if (getVar(cInputTrailOffDuration)->isUsed())
            {
               trailOffDuration = getVar(cInputTrailOffDuration)->asTime()->readVar();
            }

            float conservationFactor = cDefaultShakeConservationFactor;
            if (getVar(cInputConservationFactor)->isUsed())
            {
               conservationFactor = Math::Clamp(getVar(cInputConservationFactor)->asFloat()->readVar(), 0.0f, 1.0f);
            }

            pCamera->clearShake();
            pCamera->beginShake(duration, intensity, trailOffDuration, conservationFactor);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCustomCommandAdd()
{
   switch (getVersion())
   {
      case 1:
         teCustomCommandAddV1();
         break;

      case 2:
         teCustomCommandAddV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCustomCommandAddV1()
{
   enum { cSquad=1, cIconPosition=2, cIconName=3, cCost=4, cTimer=5, cLimit=6, cNameStringID=8, cInfoStringID=9, cHelpStringID=10,
      cQueue=11, cAllowMultiple=12, cShowLimit=13, cCloseMenu=14, cPersistent=15, cUnavailable=16, cCommandIDOut=17, cAllowCancel=18, };

   BCustomCommand c;

//-- FIXING PREFIX BUG ID 5464
   const BSquad* pSquad=gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
   if (!pSquad)
   {
      getVar(cCommandIDOut)->asInteger()->writeVar(-1);
      return;
   }
//-- FIXING PREFIX BUG ID 5471
   const BUnit* pUnit=pSquad->getLeaderUnit();
//--
   if (!pUnit)
   {
      getVar(cCommandIDOut)->asInteger()->writeVar(-1);
      return;
   }
   c.mUnitID=pUnit->getID();

   c.mPosition=getVar(cIconPosition)->asInteger()->readVar();
   if (getVar(cCost)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5470
      const BCost& cost=getVar(cCost)->asCost()->readVar();
//--
      c.mCost.set(&cost);
   }
   if (getVar(cLimit)->isUsed())
      c.mLimit=getVar(cLimit)->asInteger()->readVar();
   if (getVar(cTimer)->isUsed())
      c.mTimer=getVar(cTimer)->asFloat()->readVar();

   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      if (getVar(cIconName)->isUsed())
      {
         BSimUString iconName=getVar(cIconName)->asString()->readVar();
         // Format of the icon name is "type,name".
         int split=iconName.findLeft(",");
         if (split != -1)
         {
            BSimString type=iconName;
            BSimString name=iconName;
            type.substring(0, split);
            name.substring(split+1, name.length());
            c.mPrimaryUserIconID = gUserManager.getPrimaryUser()->getUIContext()->lookupIconID(type,name);
            c.mSecondaryUserIconID = -1;
            if (gGame.isSplitScreen())
               c.mSecondaryUserIconID = gUserManager.getSecondaryUser()->getUIContext()->lookupIconID(type,name);
         }
      }
      else
      {
         c.mPrimaryUserIconID=gUserManager.getPrimaryUser()->getUIContext()->lookupIconID("miscicon", "placeholder");
         c.mSecondaryUserIconID = -1;
         if (gGame.isSplitScreen())
            c.mSecondaryUserIconID=gUserManager.getSecondaryUser()->getUIContext()->lookupIconID("miscicon", "placeholder");
      }
   }

   if (getVar(cNameStringID)->isUsed())
      c.mNameStringIndex=gDatabase.getLocStringIndex(getVar(cNameStringID)->asInteger()->readVar());
   if (getVar(cInfoStringID)->isUsed())
      c.mInfoStringIndex=gDatabase.getLocStringIndex(getVar(cInfoStringID)->asInteger()->readVar());
   if (getVar(cHelpStringID)->isUsed())
      c.mHelpStringIndex=gDatabase.getLocStringIndex(getVar(cHelpStringID)->asInteger()->readVar());
   if (getVar(cQueue)->isUsed())
      c.mFlagQueue=getVar(cQueue)->asBool()->readVar();
   if (getVar(cAllowMultiple)->isUsed())
      c.mFlagAllowMultiple=getVar(cAllowMultiple)->asBool()->readVar();
   if (getVar(cShowLimit)->isUsed())
      c.mFlagShowLimit=getVar(cShowLimit)->asBool()->readVar();
   if (getVar(cCloseMenu)->isUsed())
      c.mFlagCloseMenu=getVar(cCloseMenu)->asBool()->readVar();
   if (getVar(cPersistent)->isUsed())
      c.mFlagPersistent=getVar(cPersistent)->asBool()->readVar();
   if (getVar(cUnavailable)->isUsed())
      c.mFlagUnavailable=getVar(cUnavailable)->asBool()->readVar();
   if (getVar(cAllowCancel)->isUsed())
      c.mFlagAllowCancel=getVar(cAllowCancel)->asBool()->readVar();

   int commandID=gWorld->addCustomCommand(c);
   getVar(cCommandIDOut)->asInteger()->writeVar(commandID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCustomCommandAddV2()
{
   enum { cUnit=1, cIconPosition=2, cIconName=3, cCost=4, cTimer=5, cLimit=6, cNameStringID=8, cInfoStringID=9, cHelpStringID=10,
      cQueue=11, cAllowMultiple=12, cShowLimit=13, cCloseMenu=14, cPersistent=15, cUnavailable=16, cCommandIDOut=17, cAllowCancel=18, };

   BCustomCommand c;

   c.mUnitID=getVar(cUnit)->asUnit()->readVar();
   c.mPosition=getVar(cIconPosition)->asInteger()->readVar();
   if (getVar(cCost)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5472
      const BCost& cost=getVar(cCost)->asCost()->readVar();
//--
      c.mCost.set(&cost);
   }
   if (getVar(cLimit)->isUsed())
      c.mLimit=getVar(cLimit)->asInteger()->readVar();
   if (getVar(cTimer)->isUsed())
      c.mTimer=getVar(cTimer)->asFloat()->readVar();

   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      if (getVar(cIconName)->isUsed())
      {
         BSimUString iconName=getVar(cIconName)->asString()->readVar();
         // Format of the icon name is "type,name".
         int split=iconName.findLeft(",");
         if (split != -1)
         {
            BSimString type=iconName;
            BSimString name=iconName;
            type.substring(0, split);
            name.substring(split+1, name.length());

            c.mPrimaryUserIconID = gUserManager.getPrimaryUser()->getUIContext()->lookupIconID(type,name);
            c.mSecondaryUserIconID = -1;
            if (gGame.isSplitScreen())
               c.mSecondaryUserIconID = gUserManager.getSecondaryUser()->getUIContext()->lookupIconID(type,name);            
         }
      }
      else
      {
         c.mPrimaryUserIconID=gUserManager.getPrimaryUser()->getUIContext()->lookupIconID("miscicon", "placeholder");
         c.mSecondaryUserIconID = -1;
         if (gGame.isSplitScreen())
            c.mSecondaryUserIconID=gUserManager.getSecondaryUser()->getUIContext()->lookupIconID("miscicon", "placeholder");
      }
   }

   if (getVar(cNameStringID)->isUsed())
      c.mNameStringIndex=gDatabase.getLocStringIndex(getVar(cNameStringID)->asInteger()->readVar());
   if (getVar(cInfoStringID)->isUsed())
      c.mInfoStringIndex=gDatabase.getLocStringIndex(getVar(cInfoStringID)->asInteger()->readVar());
   if (getVar(cHelpStringID)->isUsed())
      c.mHelpStringIndex=gDatabase.getLocStringIndex(getVar(cHelpStringID)->asInteger()->readVar());
   if (getVar(cQueue)->isUsed())
      c.mFlagQueue=getVar(cQueue)->asBool()->readVar();
   if (getVar(cAllowMultiple)->isUsed())
      c.mFlagAllowMultiple=getVar(cAllowMultiple)->asBool()->readVar();
   if (getVar(cShowLimit)->isUsed())
      c.mFlagShowLimit=getVar(cShowLimit)->asBool()->readVar();
   if (getVar(cCloseMenu)->isUsed())
      c.mFlagCloseMenu=getVar(cCloseMenu)->asBool()->readVar();
   if (getVar(cPersistent)->isUsed())
      c.mFlagPersistent=getVar(cPersistent)->asBool()->readVar();
   if (getVar(cUnavailable)->isUsed())
      c.mFlagUnavailable=getVar(cUnavailable)->asBool()->readVar();
   if (getVar(cAllowCancel)->isUsed())
      c.mFlagAllowCancel=getVar(cAllowCancel)->asBool()->readVar();

   int commandID=gWorld->addCustomCommand(c);
   getVar(cCommandIDOut)->asInteger()->writeVar(commandID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCustomCommandRemove()
{
   enum { cCommandID=1, };
   gWorld->removeCustomCommand(getVar(cCommandID)->asInteger()->readVar());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teConnectHitpointBar()
{
   // This was cut.
}


//==============================================================================
// Launch an air strike action
//==============================================================================
void BTriggerEffect::teAirStrike()
{
   //Cut.
}

//==============================================================================
// Order the mega turret to attack
//==============================================================================
void BTriggerEffect::teMegaTurretAttack()
{
   enum 
   { 
      cInputTurretSquad = 1,
      cInputTargetLocation = 2, 
      cOutputSuccess = 3,
   };

   BVector targetLocation = cInvalidVector;
   targetLocation = getVar(cInputTargetLocation)->asVector()->readVar();

   bool success = false;

   BEntityID turretSquadID = getVar(cInputTurretSquad)->asSquad()->readVar();
   BSquad* pTurretSquad = gWorld->getSquad(turretSquadID);
   if (pTurretSquad)
   {
      // Create an order
      BSimOrder* pOrder = gSimOrderManager.createOrder();
      BASSERT(pOrder);

      // Create the target
      BSimTarget target;
      target.setPosition(targetLocation);

      // Init order
      pOrder->setTarget(target);
      pOrder->setPriority(BSimOrder::cPriorityTrigger);

      // Attack!
      pTurretSquad->doAttack(pOrder, NULL, &target);

      success = true;
   }

   // Return success
   if (getVar(cOutputSuccess)->isUsed())
      getVar(cOutputSuccess)->asBool()->writeVar(success);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPlayerPop()
{
   enum { cPlayer=1, cTotalPop=2, cMaxPop=3, cFuturePop=4, cExistingPop=5 };

   long playerID = getVar(cPlayer)->asPlayer()->readVar();
   long unitPopID = gDatabase.getPop("Unit");

//-- FIXING PREFIX BUG ID 5476
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return;

   float futurePop = pPlayer->getPopFuture(unitPopID);
   float existingPop = pPlayer->getPopCount(unitPopID);
   float totalPop = futurePop + existingPop;
   float popMax = min(pPlayer->getPopCap(unitPopID), pPlayer->getPopMax(unitPopID));  

   if(getVar(cTotalPop)->isUsed())
   {
      getVar(cTotalPop)->asFloat()->writeVar(totalPop);
   }
   if(getVar(cMaxPop)->isUsed())
   {
      getVar(cMaxPop)->asFloat()->writeVar(popMax);
   }
   if(getVar(cFuturePop)->isUsed())
   {
      getVar(cFuturePop)->asFloat()->writeVar(futurePop);
   }
   if(getVar(cExistingPop)->isUsed())
   {
      getVar(cExistingPop)->asFloat()->writeVar(existingPop);
   }

 
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPlayerEconomy()
{
   enum { cPlayer=1, cSupplies=2, cSupplyRate=3, cPower=4, cPowerRate=5, cLeaderPower=6, cLeaderPowerRate=7, cLeaderPowerCharges=8 };

   long playerID = getVar(cPlayer)->asPlayer()->readVar();

//-- FIXING PREFIX BUG ID 5477
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return;

   long suppliesRateID = gDatabase.getRate("Supplies");
   long powerRateID = gDatabase.getRate("Power");
   long leaderPowerRateID = gDatabase.getRate("LeaderPowerCharge");

   long suppliesID = gDatabase.getRIDSupplies();
   long powerID = gDatabase.getRIDPower();
   long leaderPowerID = gDatabase.getRIDLeaderPowerCharge();

   float supplies = BCost::isValidResourceID(suppliesID) ? pPlayer->getResource(suppliesID) : 0.0f;
   float power = BCost::isValidResourceID(powerID) ? pPlayer->getResource(powerID) : 0.0f;
   float leaderPower = BCost::isValidResourceID(leaderPowerID) ? pPlayer->getResource(leaderPowerID) : 0.0f;
   
   float suppliesRate = BCost::isValidResourceID(suppliesRateID) ? pPlayer->getRate(suppliesRateID) : 0.0f;
   float powerRate = BCost::isValidResourceID(powerRateID) ? pPlayer->getRate(powerRateID) : 0.0f;
   float leaderPowerRate = BCost::isValidResourceID(leaderPowerRateID) ? pPlayer->getRate(leaderPowerRateID) : 0.0f;

   float chargeCount = (float)pPlayer->getLeaderPowerChargeCount();

   if(getVar(cSupplies)->isUsed())
   {
      getVar(cSupplies)->asFloat()->writeVar(supplies);
   }
   if(getVar(cSupplyRate)->isUsed())
   {
      getVar(cSupplyRate)->asFloat()->writeVar(suppliesRate);
   }
   if(getVar(cPower)->isUsed())
   {
      getVar(cPower)->asFloat()->writeVar(power);
   }
   if(getVar(cPowerRate)->isUsed())
   {
      getVar(cPowerRate)->asFloat()->writeVar(powerRate);
   }
   if(getVar(cLeaderPower)->isUsed())
   {
      getVar(cLeaderPower)->asFloat()->writeVar(leaderPower);
   }
   if(getVar(cLeaderPowerRate)->isUsed())
   {
      getVar(cLeaderPowerRate)->asFloat()->writeVar(leaderPowerRate);
   }
   if(getVar(cLeaderPowerCharges)->isUsed())
   {
      getVar(cLeaderPowerCharges)->asFloat()->writeVar(chargeCount);
   }

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPlayerScore()
{
   enum { cPlayer=1, cScore=2 };

   long playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5478
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return;

   int32 score = pPlayer->getStrength();
   getVar(cScore)->asInteger()->writeVar(score);

}

//==============================================================================
// Switch user mode to input multiple squads
//==============================================================================
void BTriggerEffect::teInputUISquadList()
{
   enum 
   { 
      cUserPlayer = 1, 
      cOutputUISquadList = 2,
      cInputSelectionRadius = 3,
      cInputEntityFilter = 4,
   };

   BTriggerVar* pUISquadList = getVar(cOutputUISquadList);
   pUISquadList->asUISquadList()->writeResult(BTriggerVarUISquadList::cUISquadListResultWaiting);

   BPlayerID userPlayerID = getVar(cUserPlayer)->asPlayer()->readVar();
   int triggerScriptID = getParentTriggerScript()->getID();
   BTriggerVarID UISquadVarID = pUISquadList->getID();

   BPlayer* pPlayer = gWorld->getPlayer(userPlayerID);
   if (!pPlayer)
      return;
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUISquadList);
      pUser->setBuildProtoID(-1);         
      pUser->setUIPowerRadius(getVar(cInputSelectionRadius)->asFloat()->readVar());
      pUser->resetAllInputUIModeData();
      if (getVar(cInputEntityFilter)->isUsed())
      {
         pUser->setInputUIModeEntityFilterSet(getVar(cInputEntityFilter)->asEntityFilterSet()->readVar());
      }
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UISquadVarID);
   }
   else
   {
      BTriggerCommand* pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(userPlayerID, cCommandTrigger);
      pCommand->setSenders(1, &userPlayerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUISquadListResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UISquadVarID);
      pCommand->setInputResult(BTriggerVarUISquad::cUISquadResultUILockError);
      BEntityIDArray empty;
      empty.clear();
      pCommand->setInputSquadList(empty);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// Add an entity filter based on max number of object types
//==============================================================================
void BTriggerEffect::teEntityFilterAddMaxObjectType()
{
   enum 
   { 
      cEntityFilterSet = 1, 
      cObjectType = 2, 
      cMaxCount = 3,
      cApplyToSquads = 4,
      cApplyToUnits = 5,
      cInvert = 6,
   };

   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   bool applyToSquads = getVar(cApplyToSquads)->isUsed() ? getVar(cApplyToSquads)->asBool()->readVar() : true;
   bool applyToUnits = getVar(cApplyToUnits)->isUsed() ? getVar(cApplyToUnits)->asBool()->readVar() : false;
   BObjectTypeID objectTypeID = getVar(cObjectType)->asObjectType()->readVar();
   uint maxCount = (uint)getVar(cMaxCount)->asInteger()->readVar();

   getVar(cEntityFilterSet)->asEntityFilterSet()->addEntityFilterMaxObjectType(invertFilter, objectTypeID, maxCount, applyToSquads, applyToUnits);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCreateTimer()
{
   switch (getVersion())
   {
      case 4:
         teCreateTimerV4();
         break;

      case 5:
         teCreateTimerV5();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCreateTimerV4()
{
   enum 
   { 
      cCountUp = 1, 
      cStartTime = 2, 
      cStopTime = 3, 
      cTimerID = 4, 
      cLabel = 6,
   };

   bool countUp = getVar(cCountUp)->asBool()->readVar();
   DWORD startTime = getVar(cStartTime)->asTime()->readVar();
   DWORD stopTime = getVar(cStopTime)->asTime()->readVar();

   int timerID = gTimerManager.createTimer(countUp, startTime, stopTime);
   BTRIGGER_ASSERTM(timerID != -1, "Invalid timer ID!");
   getVar(cTimerID)->asInteger()->writeVar(timerID);
   
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser)
   {
      BUIWidgets* pWidgets = gUIManager->getWidgetUI();
      BTRIGGER_ASSERT(pWidgets);
      if (!pWidgets)
         return;

      if (getVar(cLabel)->isUsed())
         pWidgets->setTimerVisible(true, timerID, getVar(cLabel)->asLocStringID()->readVar());
      else
         pWidgets->setTimerVisible(true, timerID);         
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCreateTimerV5()
{
   enum 
   { 
      cCountUp = 1, 
      cStartTime = 2, 
      cStopTime = 3, 
      cTimerID = 4, 
      cLabel = 6,
      cPlayer = 7,
      cPlayerList = 8,
   };

   bool countUp = getVar(cCountUp)->asBool()->readVar();
   DWORD startTime = getVar(cStartTime)->asTime()->readVar();
   DWORD stopTime = getVar(cStopTime)->asTime()->readVar();

   int timerID = gTimerManager.createTimer(countUp, startTime, stopTime);
   BTRIGGER_ASSERTM(timerID != -1, "Invalid timer ID!");
   getVar(cTimerID)->asInteger()->writeVar(timerID);

   BPlayerIDArray playerIDs;
   if (getVar(cPlayerList)->isUsed())
   {
      playerIDs = getVar(cPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cPlayer)->isUsed())
   {
      playerIDs.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }

   BUser* pUser = NULL;
   uint numPlayers = playerIDs.getSize();
   if (numPlayers > 0)
   {
      for (uint i = 0; i < numPlayers; i++)
      {
         pUser = gUserManager.getUserByPlayerID(playerIDs[i]);
         if (pUser)
         {
            break;
         }
      }
   }
   else
   {
      pUser = gUserManager.getPrimaryUser();
   }
   
   if (pUser)
   {
      BUIWidgets* pWidgets = gUIManager->getWidgetUI();
      BTRIGGER_ASSERT(pWidgets);
      if (!pWidgets)
         return;

      if (getVar(cLabel)->isUsed())
         pWidgets->setTimerVisible(true, timerID, getVar(cLabel)->asLocStringID()->readVar());
      else
         pWidgets->setTimerVisible(true, timerID);         
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teDestroyTimer()
{
   enum 
   { 
      cTimerID = 1 
   };

   int timerID = getVar(cTimerID)->asInteger()->readVar();
   gTimerManager.destroyTimer(timerID);

   BUIWidgets* pWidgets = gUIManager->getWidgetUI();
   if (!pWidgets)
      return;

   pWidgets->setTimerVisible(false, timerID);
}


//==============================================================================
// Get the mean location of the designated units, squads, or objects
//==============================================================================
void BTriggerEffect::teGetMeanLocation()
{
   switch (getVersion())
   {
      case 1:
         teGetMeanLocationV1();
         break;

      case 2:
         teGetMeanLocationV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Get the mean location of the designated units, squads, or objects
//==============================================================================
void BTriggerEffect::teGetMeanLocationV1()
{
   enum 
   { 
      cInputUnitList = 1, 
      cInputSquadList = 2, 
      cInputObjectList = 3,
      cOutputLocation = 4,       
   };

   bool useUnitList = getVar(cInputUnitList)->isUsed();
   bool useSquadList = getVar(cInputSquadList)->isUsed();
   bool useObjectList = getVar(cInputObjectList)->isUsed();
   if(!useUnitList && !useSquadList && !useObjectList)
   {
      BTRIGGER_ASSERTM(false, "No UnitList, SquadList, or ObjectList designated!");
      return;
   }

   BVector meanLoc = cInvalidVector;
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cInputUnitList)->asUnitList()->readVar();
      uint numUnits = unitList.getSize();
      if (numUnits > 0)
      {
         meanLoc = cOriginVector;
         for (uint i = 0; i < numUnits; i++)
         {
//-- FIXING PREFIX BUG ID 5479
            const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
            if (pUnit)
            {
               meanLoc += pUnit->getPosition();
            }
         }
         meanLoc /= (float)numUnits;
      }
   }
   else if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cInputSquadList)->asSquadList()->readVar();
      uint numSquads = squadList.getSize();
      if (numSquads > 0)
      {
         meanLoc = cOriginVector;
         for (uint i = 0; i < numSquads; i++)
         {
            BSquad* pSquad = gWorld->getSquad(squadList[i]);
            if (pSquad)
            {
               meanLoc += pSquad->getAveragePosition();
            }
         }
         meanLoc /= (float)numSquads;
      }
   }
   else if (useObjectList)
   {
      const BEntityIDArray& objectList = getVar(cInputObjectList)->asObjectList()->readVar();
      uint numObjects = objectList.getSize();
      if (numObjects > 0)
      {
         meanLoc = cOriginVector;
         for (uint i = 0; i < numObjects; i++)
         {
//-- FIXING PREFIX BUG ID 5480
            const BObject* pObject = gWorld->getObject(objectList[i]);
//--
            if (pObject)
            {
               meanLoc += pObject->getPosition();
            }
         }
         meanLoc /= (float)numObjects;
      }
   }

   getVar(cOutputLocation)->asVector()->writeVar(meanLoc);
}

//==============================================================================
// Get the mean location of the designated units, squads, or objects
//==============================================================================
void BTriggerEffect::teGetMeanLocationV2()
{
   enum 
   { 
      cInputUnitList = 1, 
      cInputSquadList = 2, 
      cInputObjectList = 3,
      cOutputLocation = 4,       
      cInputLocList = 5,
   };

   bool useUnitList = getVar(cInputUnitList)->isUsed();
   bool useSquadList = getVar(cInputSquadList)->isUsed();
   bool useObjectList = getVar(cInputObjectList)->isUsed();
   bool useLocList = getVar(cInputLocList)->isUsed();
   if(!useUnitList && !useSquadList && !useObjectList && !useLocList)
   {
      BTRIGGER_ASSERTM(false, "No UnitList, SquadList, or ObjectList designated!");
      return;
   }

   BVector meanLoc = cInvalidVector;
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cInputUnitList)->asUnitList()->readVar();
      uint numUnits = unitList.getSize();
      if (numUnits > 0)
      {
         uint numValid = 0;
         meanLoc = cOriginVector;
         for (uint i = 0; i < numUnits; i++)
         {
//-- FIXING PREFIX BUG ID 5497
            const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
            if (pUnit)
            {
               meanLoc += pUnit->getPosition();
               numValid++;
            }
         }
         meanLoc /= (float)numValid;
      }
   }
   else if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cInputSquadList)->asSquadList()->readVar();
      uint numSquads = squadList.getSize();
      if (numSquads > 0)
      {
         uint numValid = 0;
         meanLoc = cOriginVector;
         for (uint i = 0; i < numSquads; i++)
         {
            BSquad* pSquad = gWorld->getSquad(squadList[i]);
            if (pSquad)
            {
               meanLoc += pSquad->getAveragePosition();
               numValid++;
            }
         }
         meanLoc /= (float)numValid;
      }
   }
   else if (useObjectList)
   {
      const BEntityIDArray& objectList = getVar(cInputObjectList)->asObjectList()->readVar();
      uint numObjects = objectList.getSize();
      if (numObjects > 0)
      {
         uint numValid = 0;
         meanLoc = cOriginVector;
         for (uint i = 0; i < numObjects; i++)
         {
//-- FIXING PREFIX BUG ID 5498
            const BObject* pObject = gWorld->getObject(objectList[i]);
//--
            if (pObject)
            {
               meanLoc += pObject->getPosition();
               numValid++;
            }
         }
         meanLoc /= (float)numValid;
      }
   }
   else if (useLocList)
   {
      BVectorArray& locList = getVar(cInputLocList)->asVectorList()->readVar();
      uint numLocs = locList.getSize();
      if (numLocs > 0 )
      {
         meanLoc = cOriginVector;
         for (uint i = 0; i < numLocs; i++)
         {
            meanLoc += locList[i];
         }
         meanLoc /= (float)numLocs;
      }
   }

   getVar(cOutputLocation)->asVector()->writeVar(meanLoc);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teShowGarrisonedCount()
{
   enum { cWidgetID=1, cVisible=2, cInputGarrisonContainerUnit=3, };

   // grab the variables
   int garrisonWidgetID = getVar(cWidgetID)->asInteger()->readVar();
   garrisonWidgetID--;
   if ( (garrisonWidgetID < 0) || (garrisonWidgetID >= BUIWidgets::cNumGarrisonContainers) )
      return;

   bool bVisible = getVar(cVisible)->asBool()->readVar();
   BEntityID targetUnitID = cInvalidObjectID;
   if (getVar(cInputGarrisonContainerUnit)->isUsed())
      targetUnitID = getVar(cInputGarrisonContainerUnit)->asUnit()->readVar();

   // get the widget UI
   BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
   if (!uiWidgets)
   {
      BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
      return;
   }

   // connect them.
   uiWidgets->setGarrisonedVisible(garrisonWidgetID, bVisible, targetUnitID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teShowCitizensSaved()
{
   enum { cVisible=1, };

   // grab the variables
   bool bVisible = getVar(cVisible)->asBool()->readVar();

   // get the widget UI
   BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
   if (!uiWidgets)
   {
      BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
      return;
   }

   uiWidgets->setCitizensSavedVisible(bVisible);

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teSetCitizensSaved()
{
   switch (getVersion())
   {
   case 1:
      teSetCitizensSavedV1();
      break;

   case 2:
      teSetCitizensSavedV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }

}


//==============================================================================
//==============================================================================
void BTriggerEffect::teSetCitizensSavedV1()
{
   enum { cCitizenCount=1, };

   // grab the variables
   int count = getVar(cCitizenCount)->asInteger()->readVar();

   // get the widget UI
   BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
   if (!uiWidgets)
   {
      BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
      return;
   }

   uiWidgets->setCitizensSaved(count, 300);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teSetCitizensSavedV2()
{
   enum { cCitizenCount=1, cCitizenCountMax=2,};

   // grab the variables
   int count = getVar(cCitizenCount)->asInteger()->readVar();
   int max = getVar(cCitizenCountMax)->asInteger()->readVar();

   // get the widget UI
   BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
   if (!uiWidgets)
   {
      BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
      return;
   }

   uiWidgets->setCitizensSaved(count, max);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teCostToFloat()
{
   enum { cCost=1, cSupplyCoefficient=2, cPowerCoefficient=3, cChargesCoefficient=4, cResult=5 };

   BCost &resources = getVar(cCost)->asCost()->readVar();

   float supplyCoefficient = 1.0f;
   float powerCoefficient = 1.0f;
   float chargesCoefficient = 1.0f;

   float total = 0.0f;
   if(getVar(cSupplyCoefficient)->isUsed())
   {
      supplyCoefficient = getVar(cSupplyCoefficient)->asFloat()->readVar();
   }
   if(getVar(cPowerCoefficient)->isUsed())
   {
      powerCoefficient = getVar(cPowerCoefficient)->asFloat()->readVar();
   }
   if(getVar(cChargesCoefficient)->isUsed())
   {
      chargesCoefficient = getVar(cChargesCoefficient)->asFloat()->readVar();
   }


   long suppliesID = gDatabase.getRIDSupplies();
   long powerID = gDatabase.getRIDPower();
   long leaderPowerID = gDatabase.getRIDLeaderPowerCharge();

   if (BCost::isValidResourceID(suppliesID))
      total += (resources.get(suppliesID) * supplyCoefficient);
   if (BCost::isValidResourceID(powerID))
      total += (resources.get(powerID) * powerCoefficient);
   if (BCost::isValidResourceID(leaderPowerID))
      total += (resources.get(leaderPowerID) * chargesCoefficient);

   getVar(cResult)->asFloat()->writeVar(total);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetCost()
{
   enum { cPlayer=1, cProtoSquad=2, cTech=3, cProtoObject=4,  cCost=5};

   BPlayer* pPlayer = NULL;
   const BCost* pCost = NULL;
   if(getVar(cPlayer)->isUsed())
   {
      pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   }
   if(getVar(cProtoSquad)->isUsed())
   {
      long protosquadID = getVar(cProtoSquad)->asProtoSquad()->readVar();
//-- FIXING PREFIX BUG ID 5500
      const BProtoSquad* pSquad = NULL;
//--
      if(!pPlayer)
      {
         pSquad = gDatabase.getGenericProtoSquad(protosquadID);
      }
      else
      {
         pSquad = pPlayer->getProtoSquad(protosquadID);
      }
      if(pSquad)     
         pCost = pSquad->getCost();
   }
   else if(getVar(cTech)->isUsed())
   {
      long techID = getVar(cTech)->asTech()->readVar();
//-- FIXING PREFIX BUG ID 5501
      const BProtoTech* pTech = NULL;
//--
      if(!pPlayer)
      {
         pTech = gDatabase.getProtoTech(techID);
      }
      else
      {
         pTech = pPlayer->getProtoTech(techID);
      }
      if(pTech)        
         pCost = pTech->getCost();
   }
   else if(getVar(cProtoObject)->isUsed())
   {
      long protoID = getVar(cProtoObject)->asProtoObject()->readVar();
//-- FIXING PREFIX BUG ID 5502
      const BProtoObject* pObject = NULL;
//--
      if(!pPlayer)
      {
         pObject = gDatabase.getGenericProtoObject(protoID);
      }
      else
      {
         pObject = pPlayer->getProtoObject(protoID);
      }
      if(pObject)
         pCost = pObject->getCost();
   }
   else 
   {
      return;
   }
   if(pCost)
   {
      BCost output;
      output.set(pCost);
      getVar(cCost)->asCost()->writeVar(output);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPrimaryHealthComponent()
{
   //APF 8/29 Hmm I am on the fence on this one.  may scrap it

   //enum { cProtoSquad=1, cProtoObject=2, cHealthComponent=3 };

   //if(getVar(cProtoSquad)->isUsed())
   //{
   //   long protosquadID = getVar(cProtoSquad)->asProtoSquad()->readVar();
   //   BProtoSquad* pSquad = NULL;
   //   pSquad = gDatabase.getProtoSquad(protosquadID);
   //   pSquad->getDamageType();
   //}
   //else if(getVar(cProtoObject)->isUsed())
   //{
   //   long protoID = getVar(cProtoObject)->asProtoObject()->readVar();
   //   BProtoObject* pObject = NULL;
   //   pObject = gDatabase.getProtoObject(protoID);
   //   pObject->getDamageType();
   //}
}

//==============================================================================
// Show an in-world objective pointer
//==============================================================================
void BTriggerEffect::teShowObjectivePointer()
{
   switch (getVersion())
   {
      case 4:
         teShowObjectivePointerV4();
         break;

      case 5:
         teShowObjectivePointerV5();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported");
         break;
   }
}

//==============================================================================
// Show an in-world objective pointer
//==============================================================================
void BTriggerEffect::teShowObjectivePointerV4()
{
   enum 
   { 
      cWidgetID = 1, 
      cVisible = 2, 
      cInputPointerUnit = 3, 
      cInputPointerSquad = 4, 
      cInputPointerLocation = 5, 
      cInputPlayer = 6,
      cInputPlayerList = 7,
      cInputUseTarget = 8,
   };

   // grab the variables
   int widgetPointerID = getVar(cWidgetID)->asInteger()->readVar();
   bool bVisible = getVar(cVisible)->asBool()->readVar();
   bool useTarget = getVar(cInputUseTarget)->isUsed() ? getVar(cInputUseTarget)->asBool()->readVar() : true;
   BVector targetPos = cInvalidVector;
   if (getVar(cInputPointerUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5504
      const BUnit* pUnit = gWorld->getUnit(getVar(cInputPointerUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         targetPos = pUnit->getPosition();
      }
   }
   else if (getVar(cInputPointerSquad)->isUsed())
   {
      BSquad* pSquad = gWorld->getSquad(getVar(cInputPointerSquad)->asSquad()->readVar());
      if (pSquad)
      {
         targetPos = pSquad->getAveragePosition();
      }
   }
   else if (getVar(cInputPointerLocation)->isUsed())
   {
      targetPos = getVar(cInputPointerLocation)->asVector()->readVar();
   }

   BPlayerIDArray playerIDs;
   if (getVar(cInputPlayerList)->isUsed())
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cInputPlayer)->isUsed())
   {
      playerIDs.uniqueAdd(getVar(cInputPlayer)->asPlayer()->readVar());
   }

   uint numPlayers = playerIDs.getSize();
   bool allPlayers = false;
   if (numPlayers == 0)
   {
      numPlayers = gUserManager.getUserCount();
      allPlayers = true;
   }

   for (uint i = 0; i < numPlayers; i++)
   {
      BUser* pUser = allPlayers ? gUserManager.getUser(i) : gUserManager.getUserByPlayerID(playerIDs[i]);
      if (pUser && pUser->getFlagUserActive())
      {
         pUser->setObjectiveArrow(widgetPointerID, pUser->getHoverPoint(), targetPos, bVisible, useTarget);
      }
   }
}

//==============================================================================
// Show an in-world objective pointer
//==============================================================================
void BTriggerEffect::teShowObjectivePointerV5()
{
   enum 
   { 
      cWidgetID = 1, 
      cVisible = 2, 
      cInputPointerUnit = 3, 
      cInputPointerSquad = 4, 
      cInputPointerLocation = 5, 
      cInputPlayer = 6,
      cInputPlayerList = 7,
      cInputUseTarget = 8,
      cInputForceTargetVisible = 9,
   };

   // grab the variables
   int widgetPointerID = getVar(cWidgetID)->asInteger()->readVar();
   bool bVisible = getVar(cVisible)->asBool()->readVar();
   bool useTarget = getVar(cInputUseTarget)->isUsed() ? getVar(cInputUseTarget)->asBool()->readVar() : true;
   bool forceTargetVisible = getVar(cInputForceTargetVisible)->isUsed() ? getVar(cInputForceTargetVisible)->asBool()->readVar() : false;
   BVector targetPos = cInvalidVector;
   if (getVar(cInputPointerUnit)->isUsed())
   {
      BUnit* pUnit = gWorld->getUnit(getVar(cInputPointerUnit)->asUnit()->readVar());
      if (pUnit)
      {
         targetPos = pUnit->getPosition();
      }
   }
   else if (getVar(cInputPointerSquad)->isUsed())
   {
      BSquad* pSquad = gWorld->getSquad(getVar(cInputPointerSquad)->asSquad()->readVar());
      if (pSquad)
      {
         targetPos = pSquad->getAveragePosition();
      }
   }
   else if (getVar(cInputPointerLocation)->isUsed())
   {
      targetPos = getVar(cInputPointerLocation)->asVector()->readVar();
   }

   BPlayerIDArray playerIDs;
   if (getVar(cInputPlayerList)->isUsed())
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cInputPlayer)->isUsed())
   {
      playerIDs.uniqueAdd(getVar(cInputPlayer)->asPlayer()->readVar());
   }

   uint numPlayers = playerIDs.getSize();
   bool allPlayers = false;
   if (numPlayers == 0)
   {
      numPlayers = gUserManager.getUserCount();
      allPlayers = true;
   }

   for (uint i = 0; i < numPlayers; i++)
   {
      BUser* pUser = allPlayers ? gUserManager.getUser(i) : gUserManager.getUserByPlayerID(playerIDs[i]);
      if (pUser && pUser->getFlagUserActive())
      {
         pUser->setObjectiveArrow(widgetPointerID, pUser->getHoverPoint(), targetPos, bVisible, useTarget, forceTargetVisible);
      }
   }
}

//==============================================================================
void BTriggerEffect::teObjectiveIncrementCounter()
{
   enum 
   { 
      cInputObjective = 1, 
      cInputAmount    = 2,

      cOutputCurrentCount = 3
   };

   long objectiveID = getVar(cInputObjective)->asObjective()->readVar();
   bool bAmount     = getVar(cInputAmount)->isUsed();
   bool bCurCount   = getVar(cOutputCurrentCount)->isUsed();

   int curCount = gWorld->getObjectiveManager()->getObjectiveCurrentCount(objectiveID);
   curCount = max(curCount, 0); // don't allow negatives

   if (!bAmount)
      curCount++;
   else
      curCount += getVar(cInputAmount)->asInteger()->readVar();

   // Cap at final count
   int finalCount = gWorld->getObjectiveManager()->getObjectiveFinalCount(objectiveID);
   curCount = min(curCount, finalCount);

   gWorld->getObjectiveManager()->setObjectiveCurrentCount(objectiveID, curCount);

   if (bCurCount)
      getVar(cOutputCurrentCount)->asInteger()->writeVar(curCount);
}

//==============================================================================
void BTriggerEffect::teObjectiveDecrementCounter()
{
   enum 
   { 
      cInputObjective = 1, 
      cInputAmount    = 2,

      cOutputCurrentCount = 3
   };

   long objectiveID = getVar(cInputObjective)->asObjective()->readVar();
   bool bAmount     = getVar(cInputAmount)->isUsed();
   bool bCurCount   = getVar(cOutputCurrentCount)->isUsed();

   int curCount = gWorld->getObjectiveManager()->getObjectiveCurrentCount(objectiveID);
   if (!bAmount)
      curCount--;
   else
      curCount -= getVar(cInputAmount)->asInteger()->readVar();

   curCount = max(curCount, 0); // don't allow negatives

   gWorld->getObjectiveManager()->setObjectiveCurrentCount(objectiveID, curCount);

   if (bCurCount)
      getVar(cOutputCurrentCount)->asInteger()->writeVar(curCount);
}

//==============================================================================
void BTriggerEffect::teObjectiveGetCurrentCounter()
{
   enum 
   { 
      cInputObjective = 1, 

      cOutputCurrentCount = 2
   };

   long objectiveID = getVar(cInputObjective)->asObjective()->readVar();
   int curCount = gWorld->getObjectiveManager()->getObjectiveCurrentCount(objectiveID);

   getVar(cOutputCurrentCount)->asInteger()->writeVar(curCount);
}


//==============================================================================
void BTriggerEffect::teObjectiveGetFinalCounter()
{
   enum 
   { 
      cInputObjective = 1, 

      cOutputFinalCount = 2
   };

   long objectiveID = getVar(cInputObjective)->asObjective()->readVar();
   int finalCount = gWorld->getObjectiveManager()->getObjectiveFinalCount(objectiveID);

   getVar(cOutputFinalCount)->asInteger()->writeVar(finalCount);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetLOS()
{
   enum
   {
      cInputSquad = 1,
      cOutputLOS = 2,
   };

   float LOS = 0.0f;
   BSquad* pSquad = gWorld->getSquad(getVar(cInputSquad)->asSquad()->readVar());
   if (pSquad)
   {
      LOS = pSquad->getLOS();
   }

   getVar(cOutputLOS)->asFloat()->writeVar(LOS);
}

//==============================================================================
// Set the attacking unit's target
//==============================================================================
void BTriggerEffect::teSetUnitAttackTarget()
{
   enum
   {
      cInputUnit = 1,
      cInputTargetUnit = 2,
      cInputTargetLoc = 3,
   };

   // Valid unit?
   BUnit* pUnit = gWorld->getUnit(getVar(cInputUnit)->asUnit()->readVar());
   if (!pUnit)
   {
      return;
   }

   // Is unit currently attacking?
   BUnitActionRangedAttack* pAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
   if (!pAttackAction)
   {
      return;
   }

   // Setup target
   BSimTarget target;
   if (getVar(cInputTargetUnit)->isUsed())
   {
      BEntityID unitID = getVar(cInputTargetUnit)->asUnit()->readVar();
      BUnit* pUnit = gWorld->getUnit(unitID);
      if (pUnit)
      {
         target.setID(unitID);
      }
   }
   else if (getVar(cInputTargetLoc)->isUsed())
   {
      target.setPosition(getVar(cInputTargetLoc)->asVector()->readVar());
   }
   else
   {
      BTRIGGER_ASSERTM(false, "No TargetUnit or TargetLocation designated!");
      return;
   }

   // Set attack action's new target
   pAttackAction->changeTarget(&target, pAttackAction->getOppID(), pAttackAction->getOrder());
}

//==============================================================================
// Convert and int or float to a time variable type
//==============================================================================
void BTriggerEffect::teAsTime()
{
   enum
   {
      cInputInt = 1,
      cInputFloat = 2,
      cOutputTime = 3,
   };

   DWORD time = 0;
   if (getVar(cInputInt)->isUsed())
   {
      time = (DWORD)getVar(cInputInt)->asInteger()->readVar();
   }
   else if (getVar(cInputFloat)->isUsed())
   {
      time = (DWORD)getVar(cInputFloat)->asFloat()->readVar();
   }
#ifndef BUILD_FINAL
   else
   {      
      BSimString debugDataString;
      debugDataString.format("No integer or float designated!  Trigger Effect ID: %d", gTriggerManager.getCurrentTriggerID());
      BTRIGGER_ASSERTM(false, debugDataString.getPtr());      
   }
#endif

   getVar(cOutputTime)->asTime()->writeVar(time);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRallyPointSet()
{
   switch (getVersion())
   {
   case 1:
      teRallyPointSetV1();
      break;

   case 2:
      teRallyPointSetV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRallyPointSetV1()
{
   enum { cInputPlayer=1, cInputLocation=2, };

   int playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BVector location = getVar(cInputLocation)->asVector()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPlayer->setRallyPoint(location, cInvalidObjectID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRallyPointSetV2()
{
   enum { cInputPlayer=1, cInputLocation=2, cBase=3, cRallyTargetUnit=4};

   int playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BVector location = getVar(cInputLocation)->asVector()->readVar();

   BEntityID base = getVar(cBase)->isUsed()?getVar(cBase)->asUnit()->readVar():cInvalidObjectID;
   BEntityID rallyTarget = getVar(cRallyTargetUnit)->isUsed()?getVar(cRallyTargetUnit)->asUnit()->readVar():cInvalidObjectID;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      if(getVar(cBase)->isUsed() == false)
      {
         pPlayer->setRallyPoint(location, rallyTarget);
      }
      else if(base != cInvalidObjectID)
      {
         BUnit* pbase = gWorld->getUnit(base);
         pbase->setRallyPoint(location, rallyTarget);
      }
   }
}
//==============================================================================
//==============================================================================
void BTriggerEffect::teRallyPointClear()
{
   switch (getVersion())
   {
   case 1:
      teRallyPointClearV1();
      break;

   case 2:
      teRallyPointClearV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRallyPointClearV1()
{
   enum { cInputPlayer=1, };

   int playerID = getVar(cInputPlayer)->asPlayer()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPlayer->clearRallyPoint();
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRallyPointClearV2()
{
   enum { cInputPlayer=1, cBase=2};

   int playerID = getVar(cInputPlayer)->asPlayer()->readVar();

   BEntityID base = getVar(cBase)->isUsed()?getVar(cBase)->asUnit()->readVar():cInvalidObjectID;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      if(getVar(cBase)->isUsed() == false)
      {    
         pPlayer->clearRallyPoint();
      }
      else if(base != cInvalidObjectID)
      {
         BUnit* pbase = gWorld->getUnit(base);
         pbase->clearRallyPoint();
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRallyPointGet()
{
   enum { cInputPlayer=1, cOutputHasRallyPoint=2, cOutputLocation=3, };

   int playerID = getVar(cInputPlayer)->asPlayer()->readVar();

//-- FIXING PREFIX BUG ID 5507
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (pPlayer && pPlayer->haveRallyPoint())
   {
      getVar(cOutputHasRallyPoint)->asBool()->writeVar(true);
      getVar(cOutputLocation)->asVector()->writeVar(pPlayer->getRallyPoint());
   }
   else
   {
      getVar(cOutputHasRallyPoint)->asBool()->writeVar(false);
      getVar(cOutputLocation)->asVector()->writeVar(cOriginVector);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teUITogglePowerOverlay()
{
   enum { cPowerID = 1, cOnOff = 2, cPlayerID = 3, };
   //BProtoPowerID powerID = getVar(cPowerID)->asPower()->readVar();
   bool onOff = getVar(cOnOff)->asBool()->readVar();
   BPlayerID playerID = getVar(cPlayerID)->asPlayer()->readVar();

   if (onOff)
   {
      BFAIL("This should not be used to turn the UI on since the power system doesn't use triggers now and is being repurposed (hacked) to only turn the UI off");
      return;
   }

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // this user isn't using a power!
   if (pUser->getPowerUser() == NULL)
      return;

   pUser->getPowerUser()->cancelPower();
}

//==============================================================================
//==============================================================================
void BTriggerEffect::tePlayChat()
{
   switch (getVersion())
   {
      case 3:
         tePlayChatV3();
         break;

      case 4:
         tePlayChatV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Play a chat
//==============================================================================
void BTriggerEffect::tePlayChatV3()
{
   enum 
   { 
      cChatSound = 1, 
      cQueue = 2,
      cChatStringID = 3,
      cChatSpeaker = 4,
      cChatDuration = 6,
   };
   
   BSimString soundCue;
   soundCue.empty();
   bool queueSound = false;
   int stringID = getVar(cChatStringID)->isUsed() ? getVar(cChatStringID)->asLocStringID()->readVar() : -1;
   //int chatSpeaker = getVar(cChatSpeaker)->isUsed() ? getVar(cChatSpeaker)->asChatSpeaker()->readVar() : -1;
   int chatSpeaker = -1;
   DWORD duration = getVar(cChatDuration)->isUsed() ? getVar(cChatDuration)->asTime()->readVar() : 10000;
   if (getVar(cChatSound)->isUsed())
   {
      // play the chat sound
      soundCue = getVar(cChatSound)->asSound()->readVar();
      queueSound = getVar(cQueue)->isUsed() ? getVar(cQueue)->asBool()->readVar() : false;
      // gSoundManager.playCue(soundCue, -1, queue);
   }

   bool allPlayers = true;
   BPlayerIDArray recipients;

   // queue this chat up.
   BChatManager* pChatManager = gWorld->getChatManager();
   if (!pChatManager)
   {
      BASSERT(pChatManager);
      return;
   }
   pChatManager->addChat(stringID, soundCue, queueSound, allPlayers, chatSpeaker, recipients, (float)duration * 0.001f);

   // If subscriber exists reset
   BGeneralEventSubscriber* pSubscription = pChatManager->getChatCompletedEventSubscriber();
   if (pSubscription)
   {
      pSubscription->mFired = false;
   }   
   // Otherwise create a new subscriber
   else
   {
      pSubscription = gGeneralEventManager.newSubscriber(BEventDefinitions::cChatCompleted);
      BASSERT(pSubscription);
      pChatManager->setChatCompletedEventSubscriber(pSubscription);
   }
}

//==============================================================================
void BTriggerEffect::tePlayChatV4()
{
   enum 
   { 
      cChatSound = 1, 
      cQueue = 2,
      cChatStringID = 3,
      cChatSpeaker = 4,
      cChatDuration = 6,
      cChatTalkingHead = 7,
   };


   int TalkingHeadID = -1;
   BSimString soundCue;
   soundCue.empty();
   bool queueSound = false;
   int stringID = getVar(cChatStringID)->isUsed() ? getVar(cChatStringID)->asLocStringID()->readVar() : -1;
   //int chatSpeaker = getVar(cChatSpeaker)->isUsed() ? getVar(cChatSpeaker)->asChatSpeaker()->readVar() : -1;
   DWORD duration = getVar(cChatDuration)->isUsed() ? getVar(cChatDuration)->asTime()->readVar() : 10000;
   if (getVar(cChatSound)->isUsed())
   {
      // play the chat sound
      soundCue = getVar(cChatSound)->asSound()->readVar();
      queueSound = getVar(cQueue)->isUsed() ? getVar(cQueue)->asBool()->readVar() : false;
      // gSoundManager.playCue(soundCue, -1, queue);
   }

#ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigDisableTalkingHeads) && getVar(cChatTalkingHead)->isUsed())
#else
   if (getVar(cChatTalkingHead)->isUsed())
#endif
      TalkingHeadID = getVar(cChatTalkingHead)->asTalkingHead()->readVar();

   bool allPlayers = true;
   BPlayerIDArray recipients;

   // queue this chat up.
   BChatManager* pChatManager = gWorld->getChatManager();
   if (!pChatManager)
   {
      BASSERT(pChatManager);
      return;
   }
   pChatManager->addChat(stringID, soundCue, queueSound, allPlayers, TalkingHeadID, recipients, (float)duration * 0.001f);

   // If subscriber exists reset
   BGeneralEventSubscriber* pSubscription = pChatManager->getChatCompletedEventSubscriber();
   if (pSubscription)
   {
      pSubscription->mFired = false;
   }   
   // Otherwise create a new subscriber
   else
   {
      pSubscription = gGeneralEventManager.newSubscriber(BEventDefinitions::cChatCompleted);
      BASSERT(pSubscription);
      pChatManager->setChatCompletedEventSubscriber(pSubscription);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teShowProgressBar()
{
   // This was cut.
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teUpdateProgressBar()
{
   // This was cut.
}

// -----------------

//==============================================================================
//==============================================================================
void BTriggerEffect::teShowObjectCounter()
{
   enum { cVisible=1,  cMaxValue=2, cInitialValue=3, };

   bool visible = getVar(cVisible)->asBool()->readVar();
   bool maxValueUsed = getVar(cMaxValue)->isUsed();
   bool initialValueUsed = getVar(cInitialValue)->isUsed();

   if (visible)
   {
      if ( (!maxValueUsed) || (!initialValueUsed) )
      {
         BTRIGGER_ASSERTM(false, "The max and initial values must be set.");
         return;
      }

      int maxValue = getVar(cMaxValue)->asInteger()->readVar();
      int initialValue = getVar(cInitialValue)->asInteger()->readVar();

      if (maxValue <=0)
      {
         BTRIGGER_ASSERTM(false, "The max value must be greater than 0.");
         return;
      }

      if (maxValue > 10)
      {
         BTRIGGER_ASSERTM(false, "The max value must be 10 or less.");
         return;
      }

      if (initialValue>maxValue)
      {
         BTRIGGER_ASSERTM(false, "The initial value cannot be greater than the max value.");
         return;
      }

      // enable the UI widget and give the squad to the widget
      BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
      if (!uiWidgets)
      {
         BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
         return;
      }

      uiWidgets->setCounterVisible(visible, maxValue, initialValue);
   }
   else
   {
      // disconnect the widget
      // enable the UI widget and give the squad to the widget
      BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
      if (!uiWidgets)
      {
         BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
         return;
      }

      uiWidgets->setCounterVisible(visible, -1, -1);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teUpdateObjectCounter()
{
   enum { cNewValue=1, };

   int newValue = getVar(cNewValue)->asInteger()->readVar();

   if (newValue < 0)
   {
      BTRIGGER_ASSERTM(false, "The new value must be greater than or equal to 0.");
      return;
   }

   // enable the UI widget and give the squad to the widget
   BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
   if (!uiWidgets)
   {
      BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
      return;
   }

   if (newValue > uiWidgets->getCounterMax())
   {
      BTRIGGER_ASSERTM(false, "The new value must be less than or equal to the max value.");
      return;
   }

   uiWidgets->updateCounter(newValue);
}


//==============================================================================
// Get pop cost of a protosquad or protounit
//==============================================================================
void BTriggerEffect::teGetPop()
{
   uint version = getVersion();
   if (version == 2)
   {
      teGetPopV2();
   }
   else
   {
      teGetPopV1();
   }
}

//==============================================================================
// Get pop cost of a protosquad or protounit
//==============================================================================
void BTriggerEffect::teGetPopV1()
{
   enum { cProtoObject = 1, cProtoSquad = 2, cPop = 3 };
   float pop = 0;
   long unitPopID = gDatabase.getPop("Unit");

   if(getVar(cProtoObject)->isUsed())
   {
      long id = getVar(cProtoObject)->asProtoObject()->readVar();
      // TODO - This should be getting the player version of the proto
      BProtoObject* pObject = gDatabase.getGenericProtoObject(id);
      if(pObject != NULL)
      {
         BPopArray* pPops = pObject->getPops(); 
         if(pPops->size() > (uint)unitPopID)
            pop = (float)(pPops->get(unitPopID).mCount);
      }
   }
   if(getVar(cProtoSquad)->isUsed())
   {
      long id = getVar(cProtoSquad)->asProtoSquad()->readVar();
      // TODO - This should be getting the player version of the proto
//-- FIXING PREFIX BUG ID 5508
      const BProtoSquad* pSquad = gDatabase.getGenericProtoSquad(id);
//--
      if(pSquad != NULL)
      {
         BPopArray pops;
         pSquad->getPops(pops);  
         if(pops.size() > (uint)unitPopID)
            pop = (float)(pops[unitPopID].mCount);
      }
   }

   getVar(cPop)->asFloat()->writeVar(pop);

}

//==============================================================================
// Get pop cost of a protosquad or protounit
//==============================================================================
void BTriggerEffect::teGetPopV2()
{
   enum { cProtoObject = 1, cProtoSquad = 2, cPop = 3, cIsUnitPop = 4, cPopId = 5, };
   float popValue = 0;
   bool isUnitPop = false;
   int popID = -1;

   long unitPopID = gDatabase.getPop("Unit");

   if(getVar(cProtoObject)->isUsed())
   {
      long id = getVar(cProtoObject)->asProtoObject()->readVar();
      // TODO - This should be getting the player version of the proto
      BProtoObject* pObject = gDatabase.getGenericProtoObject(id);
      if(pObject != NULL)
      {
         BPopArray& pops = *(pObject->getPops()); 
         for(uint i=0; i < pops.size(); i++)
         {
            BPop pop = pops.get(i);            
            long id = pop.mID;
            float popCount = pop.mCount;
            
            if(popCount == 0)
               continue;

            if(id == unitPopID)
            {
               isUnitPop = true;
            }
            popID = id;
            popValue = popCount;
         }
         
      }
   }
   if(getVar(cProtoSquad)->isUsed())
   {
      long id = getVar(cProtoSquad)->asProtoSquad()->readVar();
      // TODO - This should be getting the player version of the proto
//-- FIXING PREFIX BUG ID 5509
      const BProtoSquad* pSquad = gDatabase.getGenericProtoSquad(id);
//--
      if(pSquad != NULL)
      {
         BPopArray pops;
         pSquad->getPops(pops);           
         for(uint i=0; i < pops.size(); i++)
         {
            BPop pop = pops.get(i);            
            long id = pop.mID;
            float popCount = pop.mCount;
            
            if(popCount == 0)
               continue;

            if(id == unitPopID)
            {
               isUnitPop = true;
            }
            popID = id;
            popValue = popCount;
         }

      }
   }

   if(getVar(cPop)->isUsed())
   {
      getVar(cPop)->asFloat()->writeVar(popValue);
   }
   if(getVar(cIsUnitPop)->isUsed())
   {
      getVar(cIsUnitPop)->asBool()->writeVar(isUnitPop);
   }
   if(getVar(cPopId)->isUsed())
   {
      getVar(cPopId)->asInteger()->writeVar(popID);
   }
}

//==============================================================================
// Round float  
//==============================================================================
void BTriggerEffect::teRoundFloat()
{
   enum { cInputFloat = 1, cTruncate = 2, cOutputFloat = 3, cOutputInteger = 4 };

   float input = getVar(cInputFloat)->asFloat()->readVar();
   bool truncate = getVar(cTruncate)->isUsed()? getVar(cTruncate)->asBool()->readVar() : false;

   float output;
   int outputInt;
   if(truncate)
   {
      outputInt = Math::FloatToIntTrunc(input);
   }
   else
   {
      outputInt = Math::FloatToIntRound(input);
   }
   output = (float)outputInt;
   
   if(getVar(cOutputFloat)->isUsed())
   {
      getVar(cOutputFloat)->asFloat()->writeVar(output);
   }
   if(getVar(cOutputInteger)->isUsed())
   {
      getVar(cOutputInteger)->asInteger()->writeVar(outputInt);
   }

}

//==============================================================================
// Clear the power entry list
//==============================================================================
void BTriggerEffect::tePowerClear()
{
   enum
   {
      cInputPlayer = 1,
      cInputPlayerList = 2,
   };

   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();

   if (!usePlayer && !usePlayerList)
   {
      BTRIGGER_ASSERTM(false, "No Player or PlayerList is designated!");
      return;
   }

   BPlayerIDArray playerIDs;
   if (usePlayerList)
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (usePlayer)
   {
      playerIDs.uniqueAdd(getVar(cInputPlayer)->asPlayer()->readVar());
   }

   uint numPlayers = playerIDs.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerIDs[i]);
      if (pPlayer)
      {
         // Clear the player's power entries
         pPlayer->clearPowerEntries();

         BUser* pUser = pPlayer->getUser();
         if (pUser)
         {
            // Set the player's user power menu to override mode
            pUser->setFlagPowerMenuOverride(true);
            // Force a refresh on the power menu
            pUser->setFlagPowerMenuRefresh(true);
         }
      }
   }
}

//==============================================================================
// Version control
//==============================================================================
void BTriggerEffect::tePowerInvoke()
{
   switch (getVersion())
   {
      case 6:
         tePowerInvokeV6();
         break;

      case 7:
         tePowerInvokeV7();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Invoke the designated power for the specific player
//==============================================================================
void BTriggerEffect::tePowerInvokeV6()
{
   enum
   {
      cInputPlayer = 1,
      cInputPower = 2,
      cInputDirection = 4,      
      cInputIgnoreAllReqs = 7,
      cInputPowerLevel = 9,
      cInputTargetList = 10,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
   {
      return;
   }

   BProtoPowerID protoPowerID = getVar(cInputPower)->asPower()->readVar();
   BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
   {
      return;
   }

   BPowerLevel powerLevel = getVar(cInputPowerLevel)->isUsed() ? getVar(cInputPowerLevel)->asInteger()->readVar() : pPlayer->getPowerLevel(protoPowerID);
   powerLevel = Math::Min(pProtoPower->getMaxPowerLevel(), powerLevel);
   BVector direction = getVar(cInputDirection)->isUsed() ? getVar(cInputDirection)->asVector()->readVar() : cInvalidVector;
   BVectorArray targetLocations = getVar(cInputTargetList)->asVectorList()->readVar();      

   // Get proper position
   bool useDirection = false;
   bool useOK = false;
   bool usePos = false;
   bool usePath = false;
   uint numTargets = targetLocations.getSize();
   BVector targetLoc = cInvalidVector;
   BPowerType powerType = pProtoPower->getPowerType();
   switch (powerType)
   {
      // One shot positional
      case PowerType::cOrbital:
      case PowerType::cCryo:
      case PowerType::cRage:
      case PowerType::cWave:
      case PowerType::cRepair:
      case PowerType::cDisruption:
         if (numTargets > 0)
         {
            targetLoc = targetLocations[0];
         }            

         useOK = true;
         break;

      // One shot positional with direction
      case PowerType::cCarpetBombing:
         if (numTargets > 0)
         {
            targetLoc = targetLocations[0];
         }            

         if (direction.almostEqual(cInvalidVector))
         {
            BTRIGGER_ASSERTM(false, "Direction not assigned for a power that needs a direction!");
            direction = cZAxisVector;
         }

         useDirection = true;
         usePos = true;
         break;

      case PowerType::cCleansing:
         if (numTargets > 0)
         {
            targetLoc = targetLocations[0];
         }   

         usePath = true;
         break;

      default:
         BTRIGGER_ASSERTM(false, "Power type not defined!");
         break;
   }

   BTRIGGER_ASSERTM(!targetLoc.almostEqual(cInvalidVector), "Invalid target location!");

   bool inputIgnoreAllReqs = getVar(cInputIgnoreAllReqs)->isUsed() ? getVar(cInputIgnoreAllReqs)->asBool()->readVar() : false;
   BPowerManager* pPowerManager = gWorld->getPowerManager();
   if (pPowerManager)
   {
      BPower* pPower = pPowerManager->createNewPower(protoPowerID);
      if (pPower)
      {
         if (usePath && (powerType == PowerType::cCleansing))
         {
            ((BPowerCleansing*)pPower)->setFlagUsePath(true);
         }

         // Initialize power and provide inputs         
         if (pPower->init(playerID, powerLevel, cInvalidPowerUserID, cInvalidObjectID, targetLoc, inputIgnoreAllReqs))
         {
            // Input direction
            BPowerInput powerInput;
            if (useDirection)
            {
               powerInput.mType = PowerInputType::cDirection;
               powerInput.mVector = direction;
               pPower->submitInput(powerInput);
            }

            // Input position
            if (usePos)
            {
               powerInput.mType = PowerInputType::cPosition;
               powerInput.mVector = targetLoc;
               pPower->submitInput(powerInput);
            }

            // Input path
            if (usePath)
            {
               powerInput.mType = PowerInputType::cPosition;
               uint numTargetLocs = targetLocations.getSize();
               if (numTargetLocs > 1)
               {
                  for (uint i = 1; i < numTargetLocs; i++)
                  {
                     powerInput.mVector = targetLocations[i];
                     pPower->submitInput(powerInput);
                  }
               }               
            }

            // Input user ok and target location
            if (useOK)
            {
               powerInput.mType = PowerInputType::cUserOK;
               powerInput.mVector = targetLoc;
               pPower->submitInput(powerInput);
            }            
         }
      }
   }
}

//==============================================================================
// Invoke the designated power for the specific player
//==============================================================================
void BTriggerEffect::tePowerInvokeV7()
{
   enum
   {
      cInputPlayer = 1,
      cInputPower = 2,
      cInputDirection = 4,      
      cInputIgnoreAllReqs = 7,
      cInputPowerLevel = 9,
      cInputTargetList = 10,
      cInputForPlayer = 11,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
   {
      return;
   }

   BProtoPowerID protoPowerID = getVar(cInputPower)->asPower()->readVar();

   BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
   {
      return;
   }

   BPowerLevel powerLevel = getVar(cInputPowerLevel)->isUsed() ? getVar(cInputPowerLevel)->asInteger()->readVar() : pPlayer->getPowerLevel(protoPowerID);
   powerLevel = Math::Min(pProtoPower->getMaxPowerLevel(), powerLevel);
   BVector direction = getVar(cInputDirection)->isUsed() ? getVar(cInputDirection)->asVector()->readVar() : cInvalidVector;
   BVectorArray targetLocations = getVar(cInputTargetList)->asVectorList()->readVar();      
   bool inputIgnoreAllReqs = getVar(cInputIgnoreAllReqs)->isUsed() ? getVar(cInputIgnoreAllReqs)->asBool()->readVar() : false;

   bool forPlayer = getVar(cInputForPlayer)->isUsed() ? getVar(cInputForPlayer)->asBool()->readVar() : false;
   if (forPlayer)
   {
      BUser* pUser = gUserManager.getUserByPlayerID(playerID);
      if (pUser)
      {
         pUser->invokePower(protoPowerID, cInvalidObjectID, inputIgnoreAllReqs, true, powerLevel);
      }

      return;
   }

   // Get proper position
   bool useDirection = false;
   bool useOK = false;
   bool usePos = false;
   bool usePath = false;
   uint numTargets = targetLocations.getSize();
   BVector targetLoc = cInvalidVector;
   BPowerType powerType = pProtoPower->getPowerType();
   switch (powerType)
   {
      // no input, just create
      case PowerType::cDisruption:
      case PowerType::cCryo:
      case PowerType::cRepair:
         if (numTargets > 0)
         {
            targetLoc = targetLocations[0];
         }            

         break;

      // One shot positional
      case PowerType::cOrbital:
      case PowerType::cRage:
      case PowerType::cWave:
         if (numTargets > 0)
         {
            targetLoc = targetLocations[0];
         }            

         useOK = true;
         break;

      // One shot positional with direction
      case PowerType::cCarpetBombing:
         if (numTargets > 0)
         {
            targetLoc = targetLocations[0];
         }            

         if (direction.almostEqual(cInvalidVector))
         {
            BTRIGGER_ASSERTM(false, "Direction not assigned for a power that needs a direction!");
            direction = cZAxisVector;
         }

         useDirection = true;
         usePos = true;
         break;

      case PowerType::cCleansing:
         if (numTargets > 0)
         {
            targetLoc = targetLocations[0];
         }   

         usePath = true;
         break;

      default:
         BTRIGGER_ASSERTM(false, "Power type not defined!");
         break;
   }

   BTRIGGER_ASSERTM(!targetLoc.almostEqual(cInvalidVector), "Invalid target location!");
   
   BPowerManager* pPowerManager = gWorld->getPowerManager();
   if (pPowerManager)
   {
      BPower* pPower = pPowerManager->createNewPower(protoPowerID);
      if (pPower)
      {
         if (usePath && (powerType == PowerType::cCleansing))
         {
            ((BPowerCleansing*)pPower)->setFlagUsePath(true);
         }

         // Initialize power and provide inputs         
         if (pPower->init(playerID, powerLevel, cInvalidPowerUserID, cInvalidObjectID, targetLoc, inputIgnoreAllReqs))
         {
            // Input direction
            BPowerInput powerInput;
            if (useDirection)
            {
               powerInput.mType = PowerInputType::cDirection;
               powerInput.mVector = direction;
               pPower->submitInput(powerInput);
            }

            // Input position
            if (usePos)
            {
               powerInput.mType = PowerInputType::cPosition;
               powerInput.mVector = targetLoc;
               pPower->submitInput(powerInput);
            }

            // Input path
            if (usePath)
            {
               powerInput.mType = PowerInputType::cPosition;
               uint numTargetLocs = targetLocations.getSize();
               if (numTargetLocs > 1)
               {
                  for (uint i = 1; i < numTargetLocs; i++)
                  {
                     powerInput.mVector = targetLocations[i];
                     pPower->submitInput(powerInput);
                  }
               }               
            }

            // Input user ok and target location
            if (useOK)
            {
               powerInput.mType = PowerInputType::cUserOK;
               powerInput.mVector = targetLoc;
               pPower->submitInput(powerInput);
            }            
         }
      }
   }
}

//==============================================================================
// Set the player's pop statistics
//==============================================================================
void BTriggerEffect::teSetPlayerPop()
{
   enum 
   { 
      cPlayer = 1, 
      cCapPop = 2, 
      cMaxPop = 3, 
      cFuturePop = 4, 
      cExistingPop = 5 
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   int unitPopID = gDatabase.getPop("Unit");

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   if (getVar(cCapPop)->isUsed())
   {  
      pPlayer->setPopCap(unitPopID, getVar(cCapPop)->asFloat()->readVar());      
   }
   if (getVar(cMaxPop)->isUsed())
   {
      pPlayer->setPopMax(unitPopID, getVar(cMaxPop)->asFloat()->readVar());      
   }
   if (getVar(cFuturePop)->isUsed())
   {
      pPlayer->setPopFuture(unitPopID, getVar(cFuturePop)->asFloat()->readVar());
   }
   if (getVar(cExistingPop)->isUsed())
   {
      pPlayer->setPopCount(unitPopID, getVar(cExistingPop)->asFloat()->readVar());
   }
}


//==============================================================================
// Get the military stats for a player
//==============================================================================
void BTriggerEffect::teGetPlayerMilitaryStats()
{
   enum
   {
      cInputPlayer = 1,
      cOutputBuilt = 2,
      cOutputLost = 3,
      cOutputDestroyed = 4,
   };

   int built = 0;
   int lost = 0;
   int destroyed = 0;

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cInputPlayer)->asPlayer()->readVar());
   if (pPlayer)
   {
//-- FIXING PREFIX BUG ID 5516
      const BStatsPlayer* pStats = pPlayer->getStats();
//--
      if (pStats)
      {
         const BStatRecorderArray& statRecorders = pStats->getStatRecorders();
         uint numStatRecorders = statRecorders.getSize();
         for (uint i = 0; i < numStatRecorders; i++)
         {
            const BStatRecorder* pStatRecorder = statRecorders[i];
            if (!pStatRecorder)
               continue;

            const BStatDefinition* pStatDef = pStatRecorder->getStatDefiniion();
            if (!pStatDef)
               continue;

            // Is this recorder the military squad total recorder as per stats.xml?
            if (pStatDef->getID() == 3)
            {
               const BStatRecorderTotal* pStatRecorderTotal = reinterpret_cast<const BStatRecorderTotal*>(pStatRecorder);
               BASSERT(pStatRecorderTotal);

               const BStatRecorder::BStatTotal& statTotal = pStatRecorderTotal->getStatTotal();
               built = statTotal.mBuilt;
               lost = statTotal.mLost;
               destroyed = statTotal.mDestroyed;
               break;               
            }
         }
      }
   }

   if (getVar(cOutputBuilt)->isUsed())
   {
      getVar(cOutputBuilt)->asInteger()->writeVar(built);
   }

   if (getVar(cOutputLost)->isUsed())
   {
      getVar(cOutputLost)->asInteger()->writeVar(lost);
   }

   if (getVar(cOutputDestroyed)->isUsed())
   {
      getVar(cOutputDestroyed)->asInteger()->writeVar(destroyed);
   }
}

//==============================================================================
// Get the objective stats for a particular player
//==============================================================================
void BTriggerEffect::teGetObjectiveStats()
{
   enum
   {
      cOutputNumCompleted = 1,
      cOutputNumIncomplete = 2,
      cOutputNumTotal = 3,      
      cOutputScore = 4,
      cOutputMaxScore = 5,
   };

   int numCompleted = 0;
   int numIncomplete = 0;
   int numTotal = 0;
   int score = 0;
   int maxScore = 0;
   BObjectiveManager* pObjectiveManager = gWorld->getObjectiveManager();
   if (pObjectiveManager)
   {
      numTotal = pObjectiveManager->getNumberObjectives();
      for (int i = 0; i < numTotal; i++)
      {
         uint objectiveScore = pObjectiveManager->getObjectiveScore(i);
         if (pObjectiveManager->getObjectiveCompleted(i))
         {
            numCompleted++;
            score += objectiveScore;
         }          
         maxScore += objectiveScore;
      }
      numIncomplete = numTotal - numCompleted;
   }

   if (getVar(cOutputNumCompleted)->isUsed())
   {
      getVar(cOutputNumCompleted)->asInteger()->writeVar(numCompleted);
   }

   if (getVar(cOutputNumIncomplete)->isUsed())
   {
      getVar(cOutputNumIncomplete)->asInteger()->writeVar(numIncomplete);
   }

   if (getVar(cOutputNumTotal)->isUsed())
   {
      getVar(cOutputNumTotal)->asInteger()->writeVar(numTotal);
   }

   if (getVar(cOutputScore)->isUsed())
   {
      getVar(cOutputScore)->asInteger()->writeVar(score);
   }

   if (getVar(cOutputMaxScore)->isUsed())
   {
      getVar(cOutputMaxScore)->asInteger()->writeVar(maxScore);
   }
}

//==============================================================================
// Linear interpolation between two time values based on a control percentage
//==============================================================================
void BTriggerEffect::teLerpTime()
{
   enum 
   { 
      cTime1  = 1, 
      cTime2  = 2,
      cPercent = 3,
      cResult  = 4,
   };

   DWORD time1 = getVar(cTime1)->asTime()->readVar();
   DWORD time2 = getVar(cTime2)->asTime()->readVar();
   float percent = getVar(cPercent)->asFloat()->readVar();
   percent = Math::Clamp(percent, 0.0f, 1.0f);

   DWORD result = time1 + (DWORD)(percent * (time2 - time1));

   getVar(cResult)->asTime()->writeVar(result);
}

//==============================================================================
// Combine two strings into one
//==============================================================================
void BTriggerEffect::teCombineString()
{
   enum
   {
      cInputFirstString = 1,
      cInputSecondString = 2,
      cOutputString = 3,
   };

   BSimUString first = getVar(cInputFirstString)->asString()->readVar();
   BSimUString second = getVar(cInputSecondString)->asString()->readVar();
   first.append(second);   
   getVar(cOutputString)->asString()->writeVar(first);
}

//==============================================================================
// convert time to a float
//==============================================================================
void BTriggerEffect::teTimeToFloat()
{
   enum
   {
      cInputTime = 1,
      cOutputTotalSeconds = 2,
      cOutputTotalMS = 3,
   };

   DWORD time = getVar(cInputTime)->asTime()->readVar();
   if(getVar(cOutputTotalSeconds)->isUsed())
   {
      getVar(cOutputTotalSeconds)->asFloat()->writeVar((float)(time / 1000.0));
      
   }
   if(getVar(cOutputTotalMS)->isUsed())
   {
      getVar(cOutputTotalMS)->asFloat()->writeVar((float)time);

   }
}

//==============================================================================
// Block or unblock the minimap
//==============================================================================
void BTriggerEffect::teBlockMinimap()
{
   enum { cInputPlayer=1, cInputBlock=2, };

   bool block = getVar(cInputBlock)->asBool()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cInputPlayer)->asPlayer()->readVar());
   if (pPlayer)
      pPlayer->setFlagMinimapBlocked(block);
}

//==============================================================================
// Block or unblock leader powers
//==============================================================================
void BTriggerEffect::teBlockLeaderPowers()
{
   enum { cInputPlayer=1, cInputBlock=2, };

   bool block = getVar(cInputBlock)->asBool()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cInputPlayer)->asPlayer()->readVar());
   if (pPlayer)
      pPlayer->setFlagLeaderPowersBlocked(block);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetGarrisonedSquads()
{
   enum { cContainerUnit = 1, cGarrisonedSquads = 2, cNumGarrisoned = 3, cOnlyCompletelyGarrisoned = 4, };
   BEntityID containerUnitID = getVar(cContainerUnit)->asUnit()->readVar();
   BTriggerVar* pGarrisonedSquads = getVar(cGarrisonedSquads);
   BTriggerVar* pNumGarrisoned = getVar(cNumGarrisoned);
   BTRIGGER_ASSERTM(pGarrisonedSquads->isUsed() || pNumGarrisoned->isUsed(), "No output parameters selected for GetGarrisonedSquads effect.");

   bool onlyCompletelyGarrisoned = (getVar(cOnlyCompletelyGarrisoned)->isUsed() ? getVar(cOnlyCompletelyGarrisoned)->asBool()->readVar() : false);

   BEntityIDArray garrisonedSquads;
   BUnit* pContainerUnit = gWorld->getUnit(containerUnitID);
   if (pContainerUnit)
   {
      long numEntityRefs = pContainerUnit->getNumberEntityRefs();
      for (long i=0; i<numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 5518
         const BEntityRef* pEntityRef = pContainerUnit->getEntityRefByIndex(i);
//--
         if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
         {
            BUnit* pContainedUnit = gWorld->getUnit(pEntityRef->mID);
            if (pContainedUnit)
            {
               BSquad* pSquad = pContainedUnit->getParentSquad();
               if (pSquad)
               {
                  BEntityID squadID = pSquad->getID();
                  if (garrisonedSquads.find(squadID) != -1)
                     continue;

                  if (onlyCompletelyGarrisoned)
                  {
                     if (pSquad->getActionByType(BAction::cActionTypeSquadGarrison) != NULL)
                        continue;
                     bool fullyContained = true;
                     uint childCount = pSquad->getNumberChildren();
                     for (uint j=0; j<childCount; j++)
                     {
//-- FIXING PREFIX BUG ID 5314
                        const BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(i));
//--
                        if (pChildUnit && !pChildUnit->isGarrisoned())
                        {
                           fullyContained = false;
                           break;
                        }
                     }
                     if (!fullyContained)
                        continue;
                  }

                  garrisonedSquads.add(squadID);
               }
            }
         }
      }
   }

   if (pGarrisonedSquads)
      pGarrisonedSquads->asSquadList()->writeVar(garrisonedSquads);

   if (pNumGarrisoned)
      pNumGarrisoned->asInteger()->writeVar(garrisonedSquads.getNumber());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teTransferGarrisonedSquad()
{
   enum { cFromContainerUnit = 1, cToContainerUnit = 2, cGarrisonedSquad = 3, };
   BEntityID fromContainerUnitID = getVar(cFromContainerUnit)->asUnit()->readVar();
   BEntityID toContainerUnitID = getVar(cToContainerUnit)->asUnit()->readVar();
   BEntityID garrisonedSquadID = getVar(cGarrisonedSquad)->asSquad()->readVar();

   BUnit* pFromContainerUnit = gWorld->getUnit(fromContainerUnitID);
   BUnit* pToContainerUnit = gWorld->getUnit(toContainerUnitID);
   if (pFromContainerUnit && pToContainerUnit)
   {
      int numEntityRefs = pFromContainerUnit->getNumberEntityRefs();
      for (int i=numEntityRefs; i>=0; i--)
      {
//-- FIXING PREFIX BUG ID 5520
         const BEntityRef* pEntityRef = pFromContainerUnit->getEntityRefByIndex(i);
//--
         if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
         {
            BUnit* pUnit = gWorld->getUnit(pEntityRef->mID);
            if (pUnit)
            {
//-- FIXING PREFIX BUG ID 5519
               const BSquad* pSquad = pUnit->getParentSquad();
//--
               if (pSquad && pSquad->getID() == garrisonedSquadID)
               {
                  pFromContainerUnit->unloadUnit(i, false);
                  pToContainerUnit->containUnit(pUnit->getID());
               }
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teTransferGarrisoned()
{
   enum { cFromContainerUnit = 1, cToContainerUnit = 2, cGarrisonedSquad = 3, cGarrisonedSquadList = 4, };

   BEntityID fromContainerUnitID = getVar(cFromContainerUnit)->asUnit()->readVar();
   BEntityID toContainerUnitID = getVar(cToContainerUnit)->asUnit()->readVar();

   BEntityIDArray squadList;

   if (getVar(cGarrisonedSquadList)->isUsed())
      squadList = getVar(cGarrisonedSquadList)->asSquadList()->readVar();

   if (getVar(cGarrisonedSquad)->isUsed())
      squadList.add(getVar(cGarrisonedSquad)->asSquad()->readVar());

   if (squadList.isEmpty())
      return;

   BUnit* pFromContainerUnit = gWorld->getUnit(fromContainerUnitID);
   BUnit* pToContainerUnit = gWorld->getUnit(toContainerUnitID);
   if (pFromContainerUnit && pToContainerUnit)
   {
      int numEntityRefs = pFromContainerUnit->getNumberEntityRefs();
      for (int i=numEntityRefs; i>=0; i--)
      {
//-- FIXING PREFIX BUG ID 5522
         const BEntityRef* pEntityRef = pFromContainerUnit->getEntityRefByIndex(i);
//--
         if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
         {
            BUnit* pUnit = gWorld->getUnit(pEntityRef->mID);
            if (pUnit)
            {
//-- FIXING PREFIX BUG ID 5521
               const BSquad* pSquad = pUnit->getParentSquad();
//--
               if (pSquad && squadList.find(pSquad->getID()) != -1)
               {
                  pFromContainerUnit->unloadUnit(i, false);
                  pToContainerUnit->containUnit(pUnit->getID());
               }
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teRumbleStart()
{
   switch (getVersion())
   {
      case 1:
         teRumbleStartV1();
         break;

      case 2:
         teRumbleStartV2();
         break;

      case 3:
         teRumbleStartV3();
         break;

      case 4:
         teRumbleStartV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRumbleStartV1()
{
   enum { cLeftRumbleType=1, cRightRumbleType=2, cDuration=3, cStrength=5, cLoop=6, cRumbleID=7, cPlayer=8, };

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(-1);

   if (!gDatabase.isRumbleEventEnabled(BRumbleEvent::cTypeTrigger))
      return;

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5523
   const BUser* pUser = gUserManager.getUserByPlayerID(playerID);
//--
   if (!pUser)
      return;

   BGamepad& gamepad = gInputSystem.getGamepad(pUser->getPort());

   int leftRumbleType = BGamepad::cRumbleTypeNone;
   if (getVar(cLeftRumbleType)->isUsed())
      leftRumbleType = getVar(cLeftRumbleType)->asRumbleType()->readVar();

   int rightRumbleType = BGamepad::cRumbleTypeNone;
   if (getVar(cRightRumbleType)->isUsed())
      rightRumbleType = getVar(cRightRumbleType)->asRumbleType()->readVar();

   float duration = 1.0f;
   if (getVar(cDuration)->isUsed())
      duration = getVar(cDuration)->asFloat()->readVar();

   float strength = 1.0f;
   if (getVar(cStrength)->isUsed())
      strength = getVar(cStrength)->asFloat()->readVar();

   bool loop = false;
   if (getVar(cLoop)->isUsed())
      loop = getVar(cLoop)->asBool()->readVar();

   int rumbleID = gamepad.playRumble(leftRumbleType, strength, rightRumbleType, strength, duration, loop);

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(rumbleID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRumbleStartV2()
{
   enum { cRumbleMotor=1, cRumbleType=2, cDuration=3, cStrength=5, cLoop=6, cRumbleID=7, cPlayer=8, };

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(-1);

   if (!gDatabase.isRumbleEventEnabled(BRumbleEvent::cTypeTrigger))
      return;

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5524
   const BUser* pUser = gUserManager.getUserByPlayerID(playerID);
//--
   if (!pUser)
      return;

   BGamepad& gamepad = gInputSystem.getGamepad(pUser->getPort());

   int rumbleMotor = -1;
   if (getVar(cRumbleMotor)->isUsed())
      rumbleMotor = getVar(cRumbleMotor)->asRumbleMotor()->readVar();

   int rumbleType = BGamepad::cRumbleTypeNone;
   if (getVar(cRumbleType)->isUsed())
      rumbleType = getVar(cRumbleType)->asRumbleType()->readVar();

   float duration = 1.0f;
   if (getVar(cDuration)->isUsed())
      duration = getVar(cDuration)->asFloat()->readVar();

   float strength = 1.0f;
   if (getVar(cStrength)->isUsed())
      strength = getVar(cStrength)->asFloat()->readVar();

   bool loop = false;
   if (getVar(cLoop)->isUsed())
      loop = getVar(cLoop)->asBool()->readVar();

   int rumbleID = -1;
   
   if (rumbleMotor == BGamepad::cRumbleMotorBoth)
      rumbleID = gamepad.playRumble(rumbleType, strength, rumbleType, strength, duration, loop);
   else if (rumbleMotor == BGamepad::cRumbleMotorLeft)
      rumbleID = gamepad.playRumble(rumbleType, strength, BGamepad::cRumbleTypeNone, 0.0f, duration, loop);
   else if (rumbleMotor == BGamepad::cRumbleMotorRight)
      rumbleID = gamepad.playRumble(BGamepad::cRumbleTypeNone, 0.0f, rumbleType, strength, duration, loop);

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(rumbleID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRumbleStartV3()
{
   enum { cPlayer=8, cLeftRumbleType=2, cLeftStrength=5, cRightRumbleType=9, cRightStrength=11, cDuration=3, cLoop=6, cRumbleID=7, };

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(-1);

   if (!gDatabase.isRumbleEventEnabled(BRumbleEvent::cTypeTrigger))
      return;

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5525
   const BUser* pUser = gUserManager.getUserByPlayerID(playerID);
//--
   if (!pUser)
      return;

   BGamepad& gamepad = gInputSystem.getGamepad(pUser->getPort());

   int leftRumbleType = BGamepad::cRumbleTypeNone;
   if (getVar(cLeftRumbleType)->isUsed())
      leftRumbleType = getVar(cLeftRumbleType)->asRumbleType()->readVar();

   float leftStrength = 1.0f;
   if (getVar(cLeftStrength)->isUsed())
      leftStrength = getVar(cLeftStrength)->asFloat()->readVar();

   int rightRumbleType = BGamepad::cRumbleTypeNone;
   if (getVar(cRightRumbleType)->isUsed())
      rightRumbleType = getVar(cRightRumbleType)->asRumbleType()->readVar();

   float rightStrength = 1.0f;
   if (getVar(cRightStrength)->isUsed())
      rightStrength = getVar(cRightStrength)->asFloat()->readVar();

   float duration = 1.0f;
   if (getVar(cDuration)->isUsed())
      duration = getVar(cDuration)->asFloat()->readVar();

   bool loop = false;
   if (getVar(cLoop)->isUsed())
      loop = getVar(cLoop)->asBool()->readVar();

   int rumbleID = gamepad.playRumble(leftRumbleType, leftStrength, rightRumbleType, rightStrength, duration, loop);

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(rumbleID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRumbleStartV4()
{
   enum { cPlayer=8, cLeftRumbleType=2, cLeftStrength=5, cRightRumbleType=9, cRightStrength=11, cDuration=3, cPattern=12, cLoop=6, cRumbleID=7, };

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(-1);

   if (!gDatabase.isRumbleEventEnabled(BRumbleEvent::cTypeTrigger))
      return;

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5526
   const BUser* pUser = gUserManager.getUserByPlayerID(playerID);
//--
   if (!pUser)
      return;

   BGamepad& gamepad = gInputSystem.getGamepad(pUser->getPort());

   int rumblePattern = -1;
   if (getVar(cPattern)->isUsed())
   {
      BSimString patternName = getVar(cPattern)->asString()->readVar();
      rumblePattern = BGamepad::getRumblePattern(patternName);
   }

   bool loop = false;
   if (getVar(cLoop)->isUsed())
      loop = getVar(cLoop)->asBool()->readVar();

   int rumbleID = -1;

   if (rumblePattern == -1)
   {
      int leftRumbleType = BGamepad::cRumbleTypeNone;
      if (getVar(cLeftRumbleType)->isUsed())
         leftRumbleType = getVar(cLeftRumbleType)->asRumbleType()->readVar();

      float leftStrength = 1.0f;
      if (getVar(cLeftStrength)->isUsed())
         leftStrength = getVar(cLeftStrength)->asFloat()->readVar();

      int rightRumbleType = BGamepad::cRumbleTypeNone;
      if (getVar(cRightRumbleType)->isUsed())
         rightRumbleType = getVar(cRightRumbleType)->asRumbleType()->readVar();

      float rightStrength = 1.0f;
      if (getVar(cRightStrength)->isUsed())
         rightStrength = getVar(cRightStrength)->asFloat()->readVar();

      float duration = 1.0f;
      if (getVar(cDuration)->isUsed())
         duration = getVar(cDuration)->asFloat()->readVar();

      rumbleID = gamepad.playRumble(leftRumbleType, leftStrength, rightRumbleType, rightStrength, duration, loop);
   }
   else
      rumbleID = gamepad.playRumblePattern(rumblePattern, loop);

   if (getVar(cRumbleID)->isUsed())
      getVar(cRumbleID)->asInteger()->writeVar(rumbleID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teRumbleStop()
{
   enum { cPlayer=1, cRumbleID=2, };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5527
   const BUser* pUser = gUserManager.getUserByPlayerID(playerID);
//--
   if (!pUser)
      return;

   BGamepad& gamepad = gInputSystem.getGamepad(pUser->getPort());

   int rumbleID = getVar(cRumbleID)->asInteger()->readVar();

   gamepad.stopRumble(rumbleID);
}

//==============================================================================
// Put user in a mode that allows placement of provided squads
//==============================================================================
void BTriggerEffect::teInputUIPlaceSquads()
{
   enum 
   { 
      cInputPlayer = 1,    
      cInputSquad = 2,
      cInputSquadList = 3, 
      cLosType = 4,
      cOutputUILocation = 5, 
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();

   // Collect squads
   BEntityIDArray squadList;
   if (getVar(cInputSquadList)->isUsed())
   {
      squadList = getVar(cInputSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInputSquad)->isUsed())
   {
      squadList.uniqueAdd(getVar(cInputSquad)->asSquad()->readVar());
   }
   BTRIGGER_ASSERTM((squadList.getSize() > 0), "A Squad or SquadList MUST be designated!");
   
   int losType = getVar(cLosType)->isUsed() ? getVar(cLosType)->asLOSType()->readVar() : BWorld::cCPLOSDontCare;
   getVar(cOutputUILocation)->asUILocation()->writeResult(BTriggerVarUILocation::cUILocationResultWaiting);
   BTriggerVarID UILocationVarID = getVar(cOutputUILocation)->getID();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();

   // Add squads to an army so that they are all in one platoon
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = playerID;
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   pArmy->addSquads(squadList);         
   BPlatoon* pPlatoon = gWorld->getPlatoon(pArmy->getChild(0), !gConfig.isDefined(cConfigClassicPlatoonGrouping));
   BASSERT(pPlatoon);                  
   int numPlatoons = pArmy->getNumberChildren();
   for (int i = (numPlatoons - 1); i >= 1; i--)
   {
      BPlatoon* pNextPlatoon = gWorld->getPlatoon(pArmy->getChild(i));
      if (pNextPlatoon)
      {
         pPlatoon->mergePlatoon(pNextPlatoon);
      }
   }
   BFormation2* pFormation = (BFormation2*)pPlatoon->getFormation();
   BASSERT(pFormation);
   pFormation->setUseRandom(false);
   pFormation->setMakeNeeded(true);
   pPlatoon->updateFormation();                

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->changeMode(BUser::cUserModeInputUIPlaceSquadList);
      pUser->setUIPowerRadius(0.0f);
      pUser->setBuildProtoID(cInvalidObjectID);
      pUser->setUILocationCheckObstruction(false);
      pUser->setUILocationLOSType(losType);
      pUser->setUILocationPlacementRuleIndex(-1);
      pUser->setInputUIModeSquadList(squadList);
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UILocationVarID);
   }
   else
   {
      BTriggerCommand* pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandTrigger);
      pCommand->setSenders(1, &playerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUIPlaceSquadListResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UILocationVarID);
      pCommand->setInputResult(BTriggerVarUILocation::cUILocationResultUILockError);
      pCommand->setInputLocation(cInvalidVector);         
      pCommand->setInputSquadList(squadList);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// Get the number of transports required for a given set of squads
//==============================================================================
void BTriggerEffect::teGetNumTransports()
{
   switch (getVersion())
   {
      case 1:
         teGetNumTransportsV1();
         break;

      case 2:
         teGetNumTransportsV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Get the number of transports required for a given set of squads
//==============================================================================
void BTriggerEffect::teGetNumTransportsV1()
{
   enum
   {
      cInputPlayer = 1,
      cInputSquad = 2,
      cInputSquadList = 3,
      cOutputNumTransports = 4,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   playerID;

   // Collect squads
   BEntityIDArray squadList;
   if (getVar(cInputSquadList)->isUsed())
   {
      squadList = getVar(cInputSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInputSquad)->isUsed())
   {
      squadList.uniqueAdd(getVar(cInputSquad)->asSquad()->readVar());
   }
   BTRIGGER_ASSERTM((squadList.getSize() > 0), "A Squad or SquadList MUST be designated!");

   uint numTransports = BSquadActionTransport::calculateNumTransports(squadList/*, playerID*/);

   getVar(cOutputNumTransports)->asInteger()->writeVar((int)numTransports);
}

//==============================================================================
// Get the number of transports required for a given set of squads
//==============================================================================
void BTriggerEffect::teGetNumTransportsV2()
{
   enum
   {
      cInputSquad = 2,
      cInputSquadList = 3,
      cOutputNumTransports = 4,
   };

   // Collect squads
   BEntityIDArray squadList;
   if (getVar(cInputSquadList)->isUsed())
   {
      squadList = getVar(cInputSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInputSquad)->isUsed())
   {
      squadList.uniqueAdd(getVar(cInputSquad)->asSquad()->readVar());
   }
   BTRIGGER_ASSERTM((squadList.getSize() > 0), "A Squad or SquadList MUST be designated!");

   uint numTransports = BSquadActionTransport::calculateNumTransports(squadList);

   getVar(cOutputNumTransports)->asInteger()->writeVar((int)numTransports);
}



//==============================================================================
// Set the override tint for the squads, unit, and/or objects
//==============================================================================
void BTriggerEffect::teSetOverrideTint()
{
   enum
   {
      cInputUnit = 1,
      cInputUnitList = 2,
      cInputSquad = 3,
      cInputSquadList = 4,
      cInputObject = 5,
      cInputObjectList = 6,
      cInputTint = 7,
      cInputAlpha = 8,
      cInputEnable = 9,
   };

   // Collect objects
   BEntityIDArray objectList;
   if (getVar(cInputObjectList)->isUsed())
   {
      objectList = getVar(cInputObjectList)->asObjectList()->readVar();
   }

   if (getVar(cInputObject)->isUsed())
   {
      objectList.uniqueAdd(getVar(cInputObject)->asObject()->readVar());
   }

   // Collect units
   BEntityIDArray tempList;
   if (getVar(cInputUnitList)->isUsed())
   {
      tempList = getVar(cInputUnitList)->asUnitList()->readVar();
   }

   if (getVar(cInputUnit)->isUsed())
   {
      tempList.uniqueAdd(getVar(cInputUnit)->asUnit()->readVar());
   }

   // Collect objects from units
   uint count = tempList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BUnit* pUnit = gWorld->getUnit(tempList[i]);
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5528
         const BObject* pObject = pUnit->getObject();
//--
         if (pObject)
         {
            objectList.uniqueAdd(pObject->getID());
         }
      }
   }

   // Collect squads
   tempList.clear();
   if (getVar(cInputSquadList)->isUsed())
   {
      tempList = getVar(cInputSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInputSquad)->isUsed())
   {
      tempList.uniqueAdd(getVar(cInputSquad)->asSquad()->readVar());
   }

   // Collect objects from squads
   count = tempList.getSize();
   for (uint i = 0; i < count; i++)
   {
//-- FIXING PREFIX BUG ID 5530
      const BSquad* pSquad = gWorld->getSquad(tempList[i]);
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
            if (pUnit)
            {
//-- FIXING PREFIX BUG ID 5529
               const BObject* pObject = pUnit->getObject();
//--
               if (pObject)
               {
                  objectList.uniqueAdd(pObject->getID());
               }
            }
         }
      }
   }

   BColor color = getVar(cInputTint)->asColor()->readVar();
   float alpha = getVar(cInputAlpha)->isUsed() ? getVar(cInputAlpha)->asFloat()->readVar() : 1.0f;
   bool enable = getVar(cInputEnable)->isUsed() ? getVar(cInputEnable)->asBool()->readVar() : false;

   // Set override tint
   count = objectList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject)
      {
         int a = Math::iClampToByte((int)(255.0f * alpha));
         int r = Math::iClampToByte((int)(255.0f * color.r));
         int g = Math::iClampToByte((int)(255.0f * color.g));
         int b = Math::iClampToByte((int)(255.0f * color.b));
         DWORD tintColor = D3DCOLOR_ARGB(a, r, g, b);
         
         pObject->setOverrideTintColor(tintColor);
         pObject->setFlagOverrideTint(enable);
      }
   }
}

//==============================================================================
// Enable/Disable the override tint for the squads, unit, and/or objects
//==============================================================================
void BTriggerEffect::teEnableOverrideTint()
{
   enum
   {
      cInputUnit = 1,
      cInputUnitList = 2,
      cInputSquad = 3,
      cInputSquadList = 4,
      cInputObject = 5,
      cInputObjectList = 6,
      cInputEnable = 7,
   };

   // Collect objects
   BEntityIDArray objectList;
   if (getVar(cInputObjectList)->isUsed())
   {
      objectList = getVar(cInputObjectList)->asObjectList()->readVar();
   }

   if (getVar(cInputObject)->isUsed())
   {
      objectList.uniqueAdd(getVar(cInputObject)->asObject()->readVar());
   }

   // Collect units
   BEntityIDArray tempList;
   if (getVar(cInputUnitList)->isUsed())
   {
      tempList = getVar(cInputUnitList)->asUnitList()->readVar();
   }

   if (getVar(cInputUnit)->isUsed())
   {
      tempList.uniqueAdd(getVar(cInputUnit)->asUnit()->readVar());
   }

   // Collect objects from units
   uint count = tempList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BUnit* pUnit = gWorld->getUnit(tempList[i]);
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5531
         const BObject* pObject = pUnit->getObject();
//--
         if (pObject)
         {
            objectList.uniqueAdd(pObject->getID());
         }
      }
   }

   // Collect squads
   tempList.clear();
   if (getVar(cInputSquadList)->isUsed())
   {
      tempList = getVar(cInputSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInputSquad)->isUsed())
   {
      tempList.uniqueAdd(getVar(cInputSquad)->asSquad()->readVar());
   }

   // Collect objects from squads
   count = tempList.getSize();
   for (uint i = 0; i < count; i++)
   {
//-- FIXING PREFIX BUG ID 5533
      const BSquad* pSquad = gWorld->getSquad(tempList[i]);
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
            if (pUnit)
            {
//-- FIXING PREFIX BUG ID 5532
               const BObject* pObject = pUnit->getObject();
//--
               if (pObject)
               {
                  objectList.uniqueAdd(pObject->getID());
               }
            }
         }
      }
   }

   bool enable = getVar(cInputEnable)->isUsed() ? getVar(cInputEnable)->asBool()->readVar() : false;

   // Set override tint
   count = objectList.getSize();
   for (uint i = 0; i < count; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject)
      {
         pObject->setFlagOverrideTint(enable);
      }
   }
}

//==========================================================================================
// Updates the transport squads' positions/orientations to the calculated pick up locations
//==========================================================================================
void BTriggerEffect::teSetTransportPickUpLocations()
{
   enum
   {
      cInputTransportingSquads = 1,
      cInputPlayer = 2,
      cInputTransportSquads = 3,
   };

   BEntityIDArray squadsToTransport = getVar(cInputTransportingSquads)->asSquadList()->readVar();
   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BEntityIDArray transportSquads = getVar(cInputTransportSquads)->asSquadList()->readVar();

   BTransportDataArray transportData;
   BSquadActionTransport::calculateTransportData(squadsToTransport, playerID, transportSquads, cOriginVector, true, transportData);
}

//==============================================================================
// Set the timer
//==============================================================================
void BTriggerEffect::teTimerSet()
{
   enum 
   { 
      cTimerID = 1, 
      cCountUp = 2, 
      cStartTime = 3, 
      cStopTime = 4,
   };

   int timerID = getVar(cTimerID)->asInteger()->readVar();
   BGameTimer* pTimer = gTimerManager.getTimer(timerID);
   if (!pTimer)
   {
      return;
   }

   bool countUp = getVar(cCountUp)->asBool()->readVar();
   DWORD startTime = getVar(cStartTime)->asTime()->readVar();
   DWORD stopTime = getVar(cStopTime)->asTime()->readVar();

   pTimer->reset();
   pTimer->initialize(countUp, startTime, stopTime, timerID);
   pTimer->start();
   pTimer->setActive(true);
}

//==============================================================================
// Get the timer
//==============================================================================
void BTriggerEffect::teTimerGet()
{
   enum
   {
      cInputTimerID = 1,
      cOutputCurrentTime = 3,
      cOutputActive = 4,
   };

   DWORD currentTime = 0;
   bool active = false;

   int timerID = getVar(cInputTimerID)->asInteger()->readVar();
   BGameTimer* pTimer = gTimerManager.getTimer(timerID);
   if (pTimer)
   {
      currentTime = pTimer->getCurrentTime();
      active = pTimer->getActive();
   }

   getVar(cOutputCurrentTime)->asTime()->writeVar(currentTime);

   if (getVar(cOutputActive)->isUsed())
   {
      getVar(cOutputActive)->asBool()->writeVar(active);
   }
}

//==============================================================================
// Start/Stop the timer
//==============================================================================
void BTriggerEffect::teTimerSetPaused()
{
   enum
   {
      cInputTimerID = 1,
      cInputPaused = 2,
   };

   int timerID = getVar(cInputTimerID)->asInteger()->readVar();
   BGameTimer* pTimer = gTimerManager.getTimer(timerID);
   if (pTimer)
   {
      bool paused = getVar(cInputPaused)->asBool()->readVar();
      pTimer->setPaused(paused);
   }
}


//==============================================================================
// Select squads as if the player had manually selected them
//==============================================================================
void BTriggerEffect::tePlayerSelectSquads()
{
   enum
   {
      cInputPlayer = 1,
      cInputSquad = 2,
      cInputSquadList = 3,
   };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cInputPlayer)->asPlayer()->readVar());
   if (pPlayer)
   {
      BUser* pUser = pPlayer->getUser();
      if (pUser)
      {
         BSelectionManager* pSelectionManager = pUser->getSelectionManager();
         if (pSelectionManager)
         {
            BEntityIDArray selectedSquads;
            if (getVar(cInputSquadList)->isUsed())
            {
               selectedSquads = getVar(cInputSquadList)->asSquadList()->readVar();
            }

            if (getVar(cInputSquad)->isUsed())
            {
               selectedSquads.add(getVar(cInputSquad)->asSquad()->readVar());
            }

            pSelectionManager->clearSelections();
            pSelectionManager->selectSquads(selectedSquads);
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teSetResourceHandicap()
{
   enum { cPlayerID = 1, cMultiplier=2};
   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;
   float multiplier = getVar(cMultiplier)->asFloat()->readVar();
   pPlayer->setResourceHandicap(multiplier);

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teParkingLotSet()
{
   enum { cBuilding = 1, cParkingLot = 2};

   BEntityID parkingLot = cInvalidObjectID;
   if (getVar(cParkingLot)->isUsed())
      parkingLot = getVar(cParkingLot)->asUnit()->readVar();

   BUnit* pBuilding = gWorld->getUnit(getVar(cBuilding)->asUnit()->readVar());
   if (pBuilding)
      pBuilding->setAssociatedParkingLot(parkingLot);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teUnpack()
{
   enum { cSquad = 1, cLocation = 2};

   BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
//-- FIXING PREFIX BUG ID 5536
   const BSquad* pSquad =  gWorld->getSquad(squadID);
//--
   BVector targetLocation = getVar(cLocation)->asVector()->readVar();

   if(pSquad == NULL)
     return;

   long playerID = pSquad->getPlayerID();
   long unpackAbilityID = -1;
   bool validAbility = false;
   if (pSquad && pSquad->getNumberChildren() > 0)
   {

//-- FIXING PREFIX BUG ID 5535
      const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
//--
      if (pUnit && !pUnit->getIgnoreUserInput())
      {
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         long abilityID = pProtoObject->getAbilityCommand();
         const BAbility* pAbility=gDatabase.getAbilityFromID(abilityID);
         if (pAbility && pAbility->getType()==cAbilityUnpack)
         {
            unpackAbilityID = gDatabase.getAIDCommand();
            validAbility = true;
         }

      }
   }

   if(validAbility == false)
   {
      return;
   }

   BWorkCommand* c = (BWorkCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandWork);
   if (!c)
   {
      return;
   }

   c->setSenders(1, &playerID);
   c->setSenderType(BCommand::cPlayer);
   c->setRecipientType(BCommand::cArmy);
   if (!c->setRecipients(1, &squadID))
   {
      delete c;
      return;
   }
   c->setFlag(BCommand::cFlagAlternate, false);//getFlagModifierAction());   
   //c->setBuildProtoID(mBuildProtoID);
   c->setAbilityID(unpackAbilityID);
   c->setWaypoints(&targetLocation, 1);

   //float angle = (mCameraYaw == mBuildBaseYaw ? 0.0f : mCameraYaw - mBuildBaseYaw);
   //c->setAngle(angle);   
   c->setAngle(0.0f);

   gWorld->getCommandManager()->addCommandToExecute(c);
   //pArmy->queueOrder(&tempCommand);


}


//==============================================================================
//==============================================================================
void BTriggerEffect::teHintMessageShow()
{
   enum { cPlayerList=1, cStringID=2, cDuration=3, };

   bool isDurationUsed = getVar(cDuration)->isUsed();

   long stringID = stringID = getVar(cStringID)->asLocStringID()->readVar();
   
   DWORD duration = 0;

   if (isDurationUsed)
   {
      duration = getVar(cDuration)->asTime()->readVar();
      duration /= 1000;    // convert ms to sec
   }

   const BPlayerIDArray& playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   bool allPlayers = false;


   gWorld->getHintManager()->addHint(stringID, allPlayers, playerList, (float)duration);


}

//==============================================================================
// Return the color for the particular player
//==============================================================================
void BTriggerEffect::teGetPlayerColor()
{
   enum
   {
      cPlayer = 1,
      cColor1 = 2,
      cColor2 = 3,
      cColor3 = 4,
      cMinMapColor = 5,
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 5537
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (pPlayer)
   {
      const BPlayerColor& playerColor = gDatabase.getPlayerColor(gWorld->getPlayerColorCategory(), pPlayer->getColorIndex());
      // rg [6/3/08] - FIXME - the player colors have changed since this trigger code was written.
      BColor color1(playerColor.mObjects);
      BColor color2(playerColor.mObjects);
      BColor color3(playerColor.mObjects);
      BColor minMapColor(playerColor.mMinimap);
      if (getVar(cColor1)->isUsed())
      {
         getVar(cColor1)->asColor()->writeVar(color1);
      }
      
      if (getVar(cColor2)->isUsed())
      {
         getVar(cColor2)->asColor()->writeVar(color2);
      }

      if (getVar(cColor3)->isUsed())
      {
         getVar(cColor3)->asColor()->writeVar(color3);
      }

      if (getVar(cMinMapColor)->isUsed())
      {
         getVar(cMinMapColor)->asColor()->writeVar(minMapColor);
      }
   }
}

//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teHintGlowToggle()
{
   switch (getVersion())
   {
      case 2:
         teHintGlowToggleV2();
         break;

      case 3:
         teHintGlowToggleV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Toggle on the hint glow
//==============================================================================
void BTriggerEffect::teHintGlowToggleV2()
{
   enum { cPlayerList=4,  cObject=6,  cOnOff=2,  };


   bool turnOn = getVar(cOnOff)->asBool()->readVar();

   BEntityID entityID=getVar(cObject)->asObject()->readVar();

   BObject* pObject = gWorld->getObject(entityID);
//-- FIXING PREFIX BUG ID 5538
   const BSquad* pSquad = NULL;
//--
   BEntity* pEntity = NULL;
   if (!pObject)
   {
      pObject = gWorld->getUnit(entityID);
   }
   if (!pObject)
   {
      pSquad = gWorld->getSquad(entityID);
      if(pSquad)
      {
         BEntityID child = pSquad->getChild(0);
         pObject = gWorld->getObject(child);         
      }
   }
   if (!pObject)
   {
      pEntity = gWorld->getEntity(entityID);
      if(pEntity)
         pObject = pEntity->getObject();
   }

   if (!pObject)
   {
      BTRIGGER_ASSERTM(false, "A unit is not specified in the HintGlowToggle trigger.");
      return;
   }

   if (turnOn)
      pObject->setHighlightIntensity(1.0f);
   else
      pObject->setHighlightIntensity(0.0f);

#if 0
   // this is a hack for the time being and will grow as a hack as we add more units until we get a better
   // materials solution.
   BProtoObjectID pUnitProtoID = gDatabase.getProtoObject("unsc_bldg_barracks_01");
   if (pObject->getProtoID() == pUnitProtoID)
   {
      BProtoObjectID pAttachmentProtoID = gDatabase.getProtoObject("test_temphighlight_01");
      if (turnOn)
         pObject->addAttachment(pAttachmentProtoID);
      else
         pObject->removeAttachmentsOfType(pAttachmentProtoID);
      return;
   }

   pUnitProtoID = gDatabase.getProtoObject("env_generic_wallshield_01");
   if (pObject->getProtoID() == pUnitProtoID)
   {
      BProtoObjectID pAttachmentProtoID = gDatabase.getProtoObject("env_generic_wallshieldShell_01");
      if (turnOn)
         pObject->addAttachment(pAttachmentProtoID);
      else
         pObject->removeAttachmentsOfType(pAttachmentProtoID);
      return;
   }

   pUnitProtoID = gDatabase.getProtoObject("env_generic_explosives_01");
   if (pObject->getProtoID() == pUnitProtoID)
   {
      BProtoObjectID pAttachmentProtoID = gDatabase.getProtoObject("env_generic_explosivesShell_01");
      if (turnOn)
         pObject->addAttachment(pAttachmentProtoID);
      else
         pObject->removeAttachmentsOfType(pAttachmentProtoID);
      return;
   }

   pUnitProtoID = gDatabase.getProtoObject("env_harvest_foreconsole_01");
   if (pObject->getProtoID() == pUnitProtoID)
   {
      BProtoObjectID pAttachmentProtoID = gDatabase.getProtoObject("env_harvest_foreconsoleShell_01");
      if (turnOn)
         pObject->addAttachment(pAttachmentProtoID);
      else
         pObject->removeAttachmentsOfType(pAttachmentProtoID);
      return;
   }

   pUnitProtoID = gDatabase.getProtoObject("env_generic_detonator_01");
   if (pObject->getProtoID() == pUnitProtoID)
   {
      BProtoObjectID pAttachmentProtoID = gDatabase.getProtoObject("env_generic_detonatorShell_01");
      if (turnOn)
         pObject->addAttachment(pAttachmentProtoID);
      else
         pObject->removeAttachmentsOfType(pAttachmentProtoID);
      return;
   }

   BTRIGGER_ASSERTM(false, "The specified unit is not supported by the HintGlowToggle trigger.");
#endif
}

//==============================================================================
// Toggle on the hint glow
//==============================================================================
void BTriggerEffect::teHintGlowToggleV3()
{
   enum 
   { 
      cUnit = 7,
      cUnitList = 8,
      cSquad = 9,
      cSquadList = 10,      
      cObject = 6,  
      cObjectList = 11,
      cOnOff = 2,  
   };

   bool turnOn = getVar(cOnOff)->asBool()->readVar();
   BEntityIDArray unitIDs;
   if (getVar(cUnitList)->isUsed())
   {
      unitIDs = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (getVar(cUnit)->isUsed())
   {
      unitIDs.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   BEntityIDArray squadIDs;
   if (getVar(cSquadList)->isUsed())
   {
      squadIDs = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (getVar(cSquad)->isUsed())
   {
      squadIDs.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   BEntityIDArray objectIDs;   
   if (getVar(cObjectList)->isUsed())
   {
      objectIDs = getVar(cObjectList)->asObjectList()->readVar();
   }

   if (getVar(cObject)->isUsed())
   {
      objectIDs.uniqueAdd(getVar(cObject)->asObject()->readVar());
   }

   uint numIDs = squadIDs.getSize();
   for (uint i = 0; i < numIDs; i++)
   {
//-- FIXING PREFIX BUG ID 5539
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            unitIDs.uniqueAdd(pSquad->getChild(j));
         }
      }
   }

   numIDs = unitIDs.getSize();
   for (uint i = 0; i < numIDs; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitIDs[i]);
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5540
         const BObject* pObject = pUnit->getObject();
//--
         if (pObject)
         {
            objectIDs.uniqueAdd(pObject->getID());
         }
      }
   }         

   numIDs = objectIDs.getSize();
   for (uint i = 0; i < numIDs; i++)
   {
      BObject* pObject = gWorld->getObject(objectIDs[i]);
      if (pObject)
      {
         float intensity = turnOn ? 1.0f : 0.0f;         
         pObject->setHighlightIntensity(intensity);
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teHintCalloutCreate()
{
   enum { cPlayerList=1, cLocStringID=2, cLocation=3, cSquad=5, cUnit=6, cID=4, };

   bool isLocationUsed = getVar(cLocation)->isUsed();
   bool isSquadUsed = getVar(cSquad)->isUsed();
   bool isUnitUsed = getVar(cUnit)->isUsed();

   long stringID = getVar(cLocStringID)->asLocStringID()->readVar();

   BEntityID entityID;
   int calloutID = -1;
   
   if(isSquadUsed)
   {
      entityID = getVar(cSquad)->asSquad()->readVar();
      calloutID = gUIManager->getCalloutUI()->addCallout(entityID, BUICallout::cCalloutTypeSquad, stringID);
   }
   else if (isUnitUsed)
   {
      entityID = getVar(cUnit)->asUnit()->readVar();
      calloutID = gUIManager->getCalloutUI()->addCallout(entityID, BUICallout::cCalloutTypeObject, stringID);
   }
   else if(isLocationUsed)
   {
      BVector vec = getVar(cLocation)->asVector()->readVar();
      calloutID = gUIManager->getCalloutUI()->addCallout(vec, BUICallout::cCalloutTypeLocation, stringID);
   }

   getVar(cID)->asInteger()->writeVar(calloutID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teHintCalloutDestroy()
{
   enum { cID=1, };
   int id = getVar(cID)->asInteger()->readVar();

   gUIManager->getCalloutUI()->removeCallout(id);
}

//==============================================================================
void BTriggerEffect::teEventSubscribe()
{
   switch (getVersion())
   {
   case 1:
      teEventSubscribeV1();
      break;

   case 2:
      teEventSubscribeV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teEventSubscribeV1()
{
   enum { cEventType = 1, cPlayer=3, cSourceEntity=4, cTargetEntity=5, cEventID=2 };

   bool isPlayerUsed = getVar(cPlayer)->isUsed();
   bool isSourceEntityUsed = getVar(cSourceEntity)->isUsed();
   bool isTargetEntityUsed = getVar(cTargetEntity)->isUsed();

   int subscriptionID;
   int eventType = getVar(cEventType)->asEventType()->readVar();

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.newSubscriber(eventType);
   subscriptionID = pSubscription->mID;

   BPlayerID playerID;
   if(isPlayerUsed)
   {
      playerID = getVar(cPlayer)->asPlayer()->readVar();
      pSubscription->mCheckPlayer = true;
      pSubscription->mPlayer = playerID;
   }

   //BProtoObjectID protoObjectID = cInvalidProtoObjectID;
   //if(isProtoObjectUsed)
   //{
   //   protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   //   pSubscription->mCheckProtoObject = true;
   //   pSubscription->mProtoObject = protoObjectID;
   //}
   //BProtoSquadID squadProtoSquadID = -1;
   //if(isProtoSquadUsed)
   //{
   //   squadProtoSquadID = getVar(cProtoSquad)->asProtoSquad()->readVar();
   //   pSubscription->mCheckProtoSquad = true;
   //   pSubscription->mProtoSquad = squadProtoSquadID;
   //}


   getVar(cEventID)->asInteger()->writeVar(subscriptionID);

   //if(isProtoObjectUsed || !isProtoSquadUsed || !isPlayerUsed)
   //{
   //   return;
   //}


   //BEventFilterBase* pFilter = pSubscription->addFilter<BPlayerID>();
   //pFilter->mOperator = BEventFilterBase::cEquals;
   //pFilter->mOtherValue1 = (DWORD)(getVar(cPlayer)->asPlayer()->readVar());
   //pFilter->mParamID = 1;
   //BEventFilterBase* pFilter2 = pSubscription->addFilter<BProtoSquadID>();
   //pFilter2->mOperator = BEventFilterBase::cEquals;;
   //pFilter2->mOtherValue1 = (DWORD)(getVar(cProtoSquad)->asProtoSquad()->readVar());
   //pFilter2->mParamID = 2 + (1 << 16);


   //BEventFilterBase* pFilter = pSubscription->addFilter2();
   //pFilter->mParamID = makeParamID(cSource, cPlayer);
   //pFilter->mOperator = BEventFilterBase::cEquals;
   //pFilter->mOtherValue1 = (DWORD)(getVar(cPlayer)->asPlayer()->readVar());

   //BEventFilterBase* pFilter2 = pSubscription->addFilter2();
   //pFilter2->mParamID = makeParamID(cSource, cEntity);
   //pFilter2->mProp1 = cEntity_ProtoSquad;
   //pFilter2->mOperator = BEventFilterBase::cEquals;
   //pFilter2->mOtherValue1 = (DWORD)(getVar(cProtoSquad)->asProtoSquad()->readVar());

   if(isSourceEntityUsed)
   {
      BEventFilterEntity* pFilter = pSubscription->addEventFilterEntity();
      pFilter->mParamID = BEventParameters::cSource;
      pFilter->mFilterSet.copyFilterSet( getVar(cSourceEntity)->asEntityFilterSet()->readVar() );
   }
   if(isTargetEntityUsed)
   {
      BEventFilterEntity* pFilter = pSubscription->addEventFilterEntity();
      pFilter->mParamID = BEventParameters::cTarget;
      pFilter->mFilterSet.copyFilterSet( getVar(cTargetEntity)->asEntityFilterSet()->readVar() );
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teEventSubscribeV2()
{
   enum { cEventType = 1, cPlayer=3, cSourceEntity=4, cTargetEntity=5, cEventID=2 };

   bool isPlayerUsed = getVar(cPlayer)->isUsed();

   int subscriptionID;
   int eventType = getVar(cEventType)->asEventType()->readVar();

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.newSubscriber(eventType);
   subscriptionID = pSubscription->mID;

   BPlayerID playerID;
   if(isPlayerUsed)
   {
      playerID = getVar(cPlayer)->asPlayer()->readVar();
      pSubscription->mCheckPlayer = true;
      pSubscription->mPlayer = playerID;
   }

   getVar(cEventID)->asInteger()->writeVar(subscriptionID);

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventSetFilter()
{
   enum { cEventID=1 };

   long id  = getVar(cEventID)->asInteger()->readVar();
   //not impl
   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)      
      pSubscription->mFired = false;


}

//==============================================================================
//==============================================================================
void BTriggerEffect::teHintMessageDestroy()
{
   switch (getVersion())
   {
      case 1:
         teHintMessageDestroyV1();
         break;

      case 2:
         teHintMessageDestroyV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teHintMessageDestroyV1()
{
   BHintMessage* pHint = gWorld->getHintManager()->getHint();

   if(pHint != NULL)
   {
      gWorld->getHintManager()->removeHint(pHint);
      gUIManager->hideHint();
   }
   
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teHintMessageDestroyV2()
{
   enum { cLocStringID=1 };


   BHintMessage* pHint = gWorld->getHintManager()->getHint();

   if(pHint != NULL)
   {
      bool doHide = true;
      if(getVar(cLocStringID)->isUsed())
      {
         long id  = getVar(cLocStringID)->asLocStringID()->readVar();
         if(pHint->getHintStringID() != id)
            doHide = false;
      }
      if(doHide)
      {
         gWorld->getHintManager()->removeHint(pHint);
         gUIManager->hideHint();
      }
    }
   
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teSetLevel()
{
   enum { cSquad=1, cSquadList=2, cLevel=3, };
   int level = getVar(cLevel)->asInteger()->readVar();
   if (getVar(cSquad)->isUsed())
   {
      BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
      if (pSquad)
         pSquad->upgradeLevel(level, true);
   }
   else if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      for (uint i=0; i<squadList.getSize(); i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadList[i]);
         if (pSquad)
            pSquad->upgradeLevel(level, true);
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetLevel()
{
   enum { cSquad=1, cSquadList=2, cUnit=5, cUnitList=6, cLevelOut=4, };

   int level=0;

   if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5541
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
         level=pSquad->getLevel();
   }
   else if (getVar(cSquadList)->isUsed())
   {
      int lowestLevel=999999999;
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      for (uint i=0; i<squadList.getSize(); i++)
      {
//-- FIXING PREFIX BUG ID 5542
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            if (pSquad->getLevel()<lowestLevel)
               lowestLevel=pSquad->getLevel();
         }
      }
      if (lowestLevel<999999999)
         level=lowestLevel;
   }
   else if (getVar(cUnit))
   {
      BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 5543
         const BSquad* pSquad = pUnit->getParentSquad();
//--
         if (pSquad)
            level=pSquad->getLevel();
      }
   }
   else if (getVar(cUnitList)->isUsed())
   {
      int lowestLevel=999999999;
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      for (uint i=0; i<unitList.getSize(); i++)
      {
         BUnit* pUnit = gWorld->getUnit(unitList[i]);
         if (pUnit)
         {
//-- FIXING PREFIX BUG ID 5544
            const BSquad* pSquad = pUnit->getParentSquad();
//--
            if (pSquad)
            {
               if (pSquad->getLevel()<lowestLevel)
                  lowestLevel=pSquad->getLevel();
            }
         }
      }
      if (lowestLevel<999999999)
         level=lowestLevel;
   }

   getVar(cLevelOut)->asInteger()->writeVar(level);
}



//==============================================================================
// Modify the ProtoSquad's data
//==============================================================================
void BTriggerEffect::teModifyProtoSquadData()
{
   enum
   {
      cPlayer = 1,
      cPlayerList = 2,
      cProtoSquad = 3,
      cAmount = 4,
      cPercent = 5,
      cName = 6,
      cType = 7,
      cRelativity = 8,
      cCommandType = 9,
      cCommandData = 10,
      cInvert = 11,
      cAllActions = 12,      
   };

   bool usePlayerList = getVar(cPlayerList)->isUsed();
   bool usePlayer = getVar(cPlayer)->isUsed();
   BTRIGGER_ASSERTM((usePlayer || usePlayerList), "No player or player list parameter was specified in ModifyProtoData trigger effect.");
   if (!usePlayer && !usePlayerList)
      return;

   bool useAmount = getVar(cAmount)->isUsed();
   bool usePercent = getVar(cPercent)->isUsed();
   BTRIGGER_ASSERTM((useAmount || usePercent), "No amount or percent parameter was specified in ModifyProtoData trigger effect.");
   if (!useAmount && !usePercent)
      return;

   // Get our local playerList. Start by assigning the playerList var if used, and unique adding the individual player
   BPlayerIDArray playerList;
   if (usePlayerList)
      playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   if (usePlayer)
      playerList.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());

   uint numPlayers = playerList.getSize();
   if (numPlayers == 0)
      return;

   // Amount takes precedent over percent
   float amount = 0.0f;
   if (useAmount)
      amount = getVar(cAmount)->asFloat()->readVar();
   else if (usePercent)
      amount = getVar(cPercent)->asFloat()->readVar();

   BProtoSquadID protoSquadID = getVar(cProtoSquad)->asProtoSquad()->readVar();
   long type = getVar(cType)->asSquadDataType()->readVar();
   long relativity = getVar(cRelativity)->asObjectDataRelative()->readVar();
   bool allActions = getVar(cAllActions)->isUsed() ? getVar(cAllActions)->asBool()->readVar() : true;
   bool inv = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   long commandType = getVar(cCommandType)->isUsed() ? getVar(cCommandType)->asTechDataCommandType()->readVar() : -1;
   long commandData = getVar(cCommandData)->isUsed() ? getVar(cCommandData)->asInteger()->readVar() : -1;

   BSimString name = "";
   if (getVar(cName)->isUsed())
      name = getVar(cName)->asString()->readVar();

   BProtoSquad* pBaseSquad = gDatabase.getGenericProtoSquad(protoSquadID);
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerList[i]);      
      if (pPlayer)
      {
         BProtoSquad* pCurrentSquad = pPlayer->getProtoSquad(protoSquadID);

         BTechEffect effect;
         effect.applyProtoSquadEffect(pPlayer, pCurrentSquad, pBaseSquad, amount, type, relativity, allActions, name, commandType, commandData, inv);
      }
   }
}

//==============================================================================
// Set the score for the scenario
//
// This whole trigger is a hack to get something working for the 
//==============================================================================
void BTriggerEffect::teSetScenarioScore()
{
   switch (getVersion())
   {
      case 1:
         teSetScenarioScore1();
         break;

      case 2:
         teSetScenarioScore2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Set the score for the scenario
//
// This whole trigger is a hack to get something working for the 
// Fixme - remove this on a clean up pass.
//==============================================================================
void BTriggerEffect::teSetScenarioScore1()
{
   enum
   {
      cPlayer = 1,
      cKillLossRatio = 2,
      cParTime = 3,
      cCompletionTime = 4,
      cDifficulty = 5,
      cOptObjectiveCount = 6,
      cOptObjectiveScore = 7,
      cTotalScore = 8,
      cMedalEarned = 9,
      cBaseScore = 10,
   };
}

//==============================================================================
// Set the score for the scenario
//
// This whole trigger is a hack to get something working for the 
// Fixme - remove this on a cleanup pass.
//==============================================================================
void BTriggerEffect::teSetScenarioScore2()
{
   enum
   {
      cPlayer = 1,
      cMilitaryModifier = 2,
      cParTime = 3,
      cCompletionTime = 4,
      cDifficulty = 5,
      cOptObjectiveCount = 6,
      cOptObjectiveScore = 7,
      cTotalScore = 8,
      cMedalEarned = 9,
      cBaseScore = 10,
      cHiddenFound = 11,
      cHiddenBonus = 12,
      cTimeModifier = 13,
   };
   return;
}

//==============================================================================
// Send the scenario score information to the score manager.
//==============================================================================
void BTriggerEffect::teSetScenarioScoreInfo()
{
   enum
   {
      cCombatBonusMinMultiplier = 1,
      cCombatBonusMaxMultiplier = 2,
      cMinParTime = 3,
      cMaxParTime = 4,
      cScoreNeededForGradeA = 5,
      cScoreNeededForGradeB = 6,
      cScoreNeededForGradeC = 7,
      cScoreNeededForGradeD = 8,
   };

   float combatBonusMin = getVar(cCombatBonusMinMultiplier)->asFloat()->readVar();
   float combatBonusMax = getVar(cCombatBonusMaxMultiplier)->asFloat()->readVar();
   DWORD missionMinParTime = getVar(cMinParTime)->asTime()->readVar();
   DWORD missionMaxParTime = getVar(cMaxParTime)->asTime()->readVar();
   long gradeA = getVar(cScoreNeededForGradeA)->asInteger()->readVar();
   long gradeB = getVar(cScoreNeededForGradeB)->asInteger()->readVar();
   long gradeC = getVar(cScoreNeededForGradeC)->asInteger()->readVar();
   long gradeD = getVar(cScoreNeededForGradeD)->asInteger()->readVar();
   //long scenarioID = getVar(cScenarioID)->asInteger()->readVar();

   gScoreManager.setScenarioData( -1, combatBonusMin, combatBonusMax, missionMinParTime, missionMaxParTime, gradeA, gradeB, gradeC, gradeD );
}


//==============================================================================
// Creates an obstruction unit
//==============================================================================
void BTriggerEffect::teCreateObstructionUnit()
{
   enum
   {
      cInputPos = 1,
      cInputDir = 2,
      cInputRadiusX = 3,
      cInputRadiusY = 4,
      cInputRadiusZ = 5,
      cInputClearList = 6,
      cOutputUnit = 7,
      cOutputUnitList = 8,
   };

   // Seed our object create parms & create our unit.
   BObjectCreateParms parms;
   parms.mProtoObjectID = gDatabase.getPOIDObstruction();
   parms.mPlayerID = 0;
   parms.mPosition = getVar(cInputPos)->asVector()->readVar();
   parms.mForward = getVar(cInputDir)->asVector()->readVar();
   parms.mRight.assignCrossProduct(cYAxisVector, parms.mForward);
   parms.mRight.normalize();   
   
   BUnit* pUnit = gWorld->createUnit(parms);
   BEntityID createdUnitID = pUnit ? pUnit->getID() : cInvalidObjectID;
   if (pUnit)
   {
      pUnit->setObstructionRadiusX(getVar(cInputRadiusX)->asFloat()->readVar());
      pUnit->setObstructionRadiusY(getVar(cInputRadiusY)->asFloat()->readVar());
      pUnit->setObstructionRadiusZ(getVar(cInputRadiusZ)->asFloat()->readVar());
      pUnit->updateObstruction();
   }

   if (getVar(cOutputUnit)->isUsed())
   {
      getVar(cOutputUnit)->asUnit()->writeVar(createdUnitID);
   }

   if (getVar(cOutputUnitList)->isUsed())
   {
      bool clearExisting = getVar(cInputClearList)->isUsed() ? getVar(cInputClearList)->asBool()->readVar() : false;
      if (clearExisting)
      {
         getVar(cOutputUnitList)->asUnitList()->clear();
      }

      if (createdUnitID != cInvalidObjectID)
      {
         getVar(cOutputUnitList)->asUnitList()->addUnit(createdUnitID);
      }
   }
}

//==============================================================================
// Creates an obstruction unit
//==============================================================================
void BTriggerEffect::tePlayVideo()
{
   enum
   {
      cVideoName = 1,
      cxScale = 2,
      cyScale = 3,
      cxOffset = 4,
      cyOffset = 5,
   }; 

   BSimString videoName = getVar(cVideoName)->asString()->readVar();

   float xScale = getVar(cxScale)->asFloat()->readVar();
   float yScale = getVar(cyScale)->asFloat()->readVar();
   float xOffset = getVar(cxOffset)->asFloat()->readVar();
   float YOffset = getVar(cyOffset)->asFloat()->readVar();

   BBinkInterface::BLoadParams lp;
   lp.mFilename.set(videoName);
   lp.mCaptionDirID = cDirData;
   lp.mFullScreen = false;
   lp.mXOffset = xOffset;
   lp.mYOffset = YOffset;
   lp.mWidthScale = xScale;
   lp.mHeightScale = yScale;
   gBinkInterface.loadActiveVideo(lp);//gBinkInterface.loadActiveVideo(videoName, cDirData, "", "", xScale,yScale,xOffset,YOffset);
}

//==============================================================================
// Remove an Object from an ObjectList
//==============================================================================
void BTriggerEffect::teObjectListRemove()
{
   enum 
   { 
      cSrcList = 1, 
      cRemoveObject = 2, 
      cRemoveList = 3, 
      cRemoveAll = 4, 
   };

   BTriggerVarObjectList* pObjectList = getVar(cSrcList)->asObjectList();

   bool removeAll = getVar(cRemoveAll)->asBool()->readVar();
   if (removeAll)
   {
      pObjectList->clear();
      return;
   }

   bool useRemoveObject = getVar(cRemoveObject)->isUsed();
   if (useRemoveObject)
      pObjectList->removeObject(getVar(cRemoveObject)->asObject()->readVar());

   bool useRemoveList = getVar(cRemoveList)->isUsed();
   if (useRemoveList)
   {
      const BEntityIDArray& removeList = getVar(cRemoveList)->asObjectList()->readVar();
      for (uint i = 0; i < removeList.getSize(); i++)
      {
         pObjectList->removeObject(removeList[i]);
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventReset()
{
   enum { cEventID=1 };

   long id  = getVar(cEventID)->asInteger()->readVar();
   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)
   {
      if (pSubscription->useCount())
      {
         pSubscription->mFiredCount = Math::ClampLow(pSubscription->mFiredCount - 1, 0);
         if (pSubscription->mFiredCount <= 0)
            pSubscription->mFired = false;
      }
      else
         pSubscription->mFired = false;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventFilterCamera()
{
     switch (getVersion())
   {
      case 1:
         teEventFilterCameraV1();
         break;

      case 2:
         teEventFilterCameraV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventFilterCameraV1()
{
   enum { cEventID=1, cViewAreaRadius=2, cLocation=5, cUnit=3, cObject=4  };

   long id  = getVar(cEventID)->asInteger()->readVar();

   float viewAreaRadius = (getVar(cViewAreaRadius)->isUsed())? getVar(cViewAreaRadius)->asFloat()->readVar() : 100.0f;

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)    
   {
      BEventFilterCamera* pFilter = pSubscription->addEventFilterCamera();
      pFilter->mParamID = BEventParameters::cSource;

      pFilter->mViewAreaRadius = viewAreaRadius;

      if(getVar(cUnit)->isUsed())
      {
         pFilter->mbFilterUnit = true;
         pFilter->mUnit = getVar(cUnit)->asUnit()->readVar();
      }
      if(getVar(cObject)->isUsed())
      {
         pFilter->mbFilterUnit = true;
         pFilter->mUnit = getVar(cObject)->asObject()->readVar();
      }
      if(getVar(cLocation)->isUsed())
      {
         pFilter->mbFilterLocation = true;
         pFilter->mLocation = getVar(cLocation)->asVector()->readVar();
      }
   } 
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventFilterCameraV2()
{
   enum { cEventID=1, cViewAreaRadius=2, cLocation=5, cUnit=3, cObject=4, cInvert=6  };

   long id  = getVar(cEventID)->asInteger()->readVar();

   float viewAreaRadius = (getVar(cViewAreaRadius)->isUsed())? getVar(cViewAreaRadius)->asFloat()->readVar() : 100.0f;

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)    
   {
      BEventFilterCamera* pFilter = pSubscription->addEventFilterCamera();
      pFilter->mParamID = BEventParameters::cSource;

      pFilter->mViewAreaRadius = viewAreaRadius;

      if(getVar(cUnit)->isUsed())
      {
         pFilter->mbFilterUnit = true;
         pFilter->mUnit = getVar(cUnit)->asUnit()->readVar();
      }
      if(getVar(cObject)->isUsed())
      {
         pFilter->mbFilterUnit = true;
         pFilter->mUnit = getVar(cObject)->asObject()->readVar();
      }
      if(getVar(cLocation)->isUsed())
      {
         pFilter->mbFilterLocation = true;
         pFilter->mLocation = getVar(cLocation)->asVector()->readVar();
      }
      if(getVar(cInvert)->isUsed())
      {
         pFilter->mbInvert = getVar(cInvert)->asBool()->readVar();
      }
   } 
}



//==============================================================================
//==============================================================================
void BTriggerEffect::teEventFilterEntity()
{
   enum { cEventID=1, cSourceFilter=2, cTargetFilter=3  };

   long id  = getVar(cEventID)->asInteger()->readVar();

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)    
   {
      if(getVar(cSourceFilter)->isUsed())
      {
         BEventFilterEntity* pFilter = pSubscription->addEventFilterEntity();
         pFilter->mParamID = BEventParameters::cSource;
         pFilter->mFilterSet.copyFilterSet( getVar(cSourceFilter)->asEntityFilterSet()->readVar() );
      }
      if(getVar(cTargetFilter)->isUsed())
      {
         BEventFilterEntity* pFilter = pSubscription->addEventFilterEntity();
         pFilter->mParamID = BEventParameters::cTarget;
         pFilter->mFilterSet.copyFilterSet( getVar(cTargetFilter)->asEntityFilterSet()->readVar() );
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventFilterEntityList()
{
   enum { cEventID=1, cSourceFilter=2, cSourceMinMatches=3, cSourceAllMustMatch=4  };

   long id  = getVar(cEventID)->asInteger()->readVar();

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)    
   {
      BEventFilterEntityList* pFilter = pSubscription->addEventFilterEntityList();
      if(getVar(cSourceFilter)->isUsed())
      {
         pFilter->mParamID = BEventParameters::cSource;
         pFilter->mFilterSet.copyFilterSet( getVar(cSourceFilter)->asEntityFilterSet()->readVar() );
      }
      if(getVar(cSourceMinMatches)->isUsed())
         pFilter->mMinimumMatchesRequired = getVar(cSourceMinMatches)->asInteger()->readVar();
      if(getVar(cSourceAllMustMatch)->isUsed())
         pFilter->mAllMustMatch = getVar(cSourceAllMustMatch)->asBool()->readVar();
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teEnableChats()
{
   enum { cEnableChats=1 };

   bool enable = getVar(cEnableChats)->asBool()->readVar();

   gWorld->getChatManager()->setChatsEnabled(enable);
}

//==============================================================================
// Add an object to an object list
//==============================================================================
void BTriggerEffect::teObjectListAdd()
{
   enum 
   { 
      cDestList = 1, 
      cAddObject = 2, 
      cAddList = 3, 
      cAddMoreList = 4, 
      cClearExisting = 5, 
   };

   BTriggerVarObjectList* pDestList = getVar(cDestList)->asObjectList();

   bool clearExisting = getVar(cClearExisting)->asBool()->readVar();
   if (clearExisting)
      pDestList->clear();

   bool useAddObject = getVar(cAddObject)->isUsed();
   if (useAddObject)
   {
      BEntityID objectID = getVar(cAddObject)->asObject()->readVar();
      if (gWorld->getObject(objectID))
         pDestList->addObject(objectID);
   }

   bool useAddList = getVar(cAddList)->isUsed();
   if (useAddList)
   {
      const BEntityIDArray& addList = getVar(cAddList)->asObjectList()->readVar();
      uint addListCount = addList.getSize();
      for (uint i = 0; i < addListCount; i++)
      {
         pDestList->addObject(addList[i]);
      }
   }

   bool useAddMoreList = getVar(cAddMoreList)->isUsed();
   if (useAddMoreList)
   {
      const BEntityIDArray& addMoreList = getVar(cAddMoreList)->asObjectList()->readVar();
      uint addMoreListCount = addMoreList.getSize();
      for (uint i = 0; i < addMoreListCount; i++)
      {
         pDestList->addObject(addMoreList[i]);
      }
   }
}

//==============================================================================
// Get the number of objects in an object list
//==============================================================================
void BTriggerEffect::teObjectListGetSize()
{
   enum 
   { 
      cObjectList = 1, 
      cOutputSize = 2, 
   };

   const BEntityIDArray& objectList = getVar(cObjectList)->asObjectList()->readVar();
   long listSize = objectList.getNumber();
   getVar(cOutputSize)->asInteger()->writeVar(listSize);
}

//==============================================================================
// Clear the corpse units
//==============================================================================
void BTriggerEffect::teClearCorpseUnits()
{
   enum
   {
      cUnits = 1,
      cLeaveEffect = 2,
   };

   BEntityIDArray corpses = getVar(cUnits)->asUnitList()->readVar();
   bool leaveEffect = getVar(cLeaveEffect)->isUsed() ? getVar(cLeaveEffect)->asBool()->readVar() : false;
   uint numCorpses = corpses.getSize();
   for (uint i = 0; i < numCorpses; i++)
   {
      gCorpseManager.removeCorpse(corpses[i], leaveEffect);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAddXP()
{
   enum { cSquad=1, cSquadList=2, cXP=3, };
   float xp = getVar(cXP)->asFloat()->readVar();
   if (getVar(cSquad)->isUsed())
   {
      BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
      if (pSquad)
      {
         pSquad->addBankXP(xp);
         pSquad->applyBankXP();
      }
   }
   else if (getVar(cSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      for (uint i=0; i<squadList.getSize(); i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            pSquad->addBankXP(xp);
            pSquad->applyBankXP();
         }
      }
   }
}

//==============================================================================
// add an event filter that checks on game state predicates
//==============================================================================
void BTriggerEffect::teEventFilterGameState()
{
   enum  {	cEventID = 1, cPlayer = 8, cGameStatePredicate = 2, cInvert = 3, cEntityFilter = 4, cEntityMinMatches = 5, cTech = 6, cCost = 7,   };

   long id  = getVar(cEventID)->asInteger()->readVar();
   long pred = getVar(cGameStatePredicate)->asGameStatePredicate()->readVar();
   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)    
   {
      BEventFilterGameState* pFilter = pSubscription->addEventFilterGameState();
      pFilter->mGameState = pred;
      pFilter->mPlayerID = playerID;
      pFilter->mInvert = getVar(cInvert)->isUsed()?getVar(cInvert)->asBool()->readVar():false;

      if(getVar(cEntityFilter)->isUsed())
      {
         pFilter->mParamID = BEventParameters::cSource;
         pFilter->mFilterSet.copyFilterSet( getVar(cEntityFilter)->asEntityFilterSet()->readVar() );
      }
      if(getVar(cEntityMinMatches)->isUsed())
      {
         pFilter->mMinimumMatchesRequired = getVar(cEntityMinMatches)->asInteger()->readVar();
      }
      //if(getVar(cSourceAllMustMatch)->isUsed())
      {
         pFilter->mAllMustMatch = false; //getVar(cSourceAllMustMatch)->asBool()->readVar();
      }
      if(getVar(cTech)->isUsed())
      {
         pFilter->mTechID = getVar(cTech)->asTech()->readVar();
      }
      if(getVar(cCost)->isUsed())
      {
         pFilter->mCost = getVar(cCost)->asCost()->readVar();
      }
   }
}


//==============================================================================
// add an event filter that checks compares game types
//==============================================================================
void BTriggerEffect::teEventFilterType()
{
   enum  {	cEventID = 1, cInvert = 2, cObjectType = 4, cProtoSquad = 5, cProtoObject = 6, cTech = 7 };

   long id  = getVar(cEventID)->asInteger()->readVar();
   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);

   if(pSubscription)    
   {
      BEventFilterType* pFilter = pSubscription->addEventFilterType();
      
      pFilter->mInvert = getVar(cInvert)->isUsed()?getVar(cInvert)->asBool()->readVar():false;
      if(getVar(cObjectType)->isUsed())
      {
         pFilter->mObjectType = getVar(cObjectType)->asObjectType()->readVar();
      }
      if(getVar(cProtoSquad)->isUsed())
      {
         pFilter->mProtoSquad = getVar(cProtoSquad)->asProtoSquad()->readVar();
      }
      if(getVar(cProtoObject)->isUsed())
      {
         pFilter->mProtoObject = getVar(cProtoObject)->asProtoObject()->readVar();
      }
      if(getVar(cTech)->isUsed())
      {
         pFilter->mTech = getVar(cTech)->asTech()->readVar();
      }
   }
}

//==============================================================================
// add an event filter that checks compares numbers
//==============================================================================
void BTriggerEffect::teEventFilterNumeric()
{
   enum  {	cEventID = 1, cCompare = 2, cInteger = 3, cFloat = 4 };

   long id  = getVar(cEventID)->asInteger()->readVar();
   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);

   if(pSubscription)    
   {
      BEventFilterNumeric* pFilter = pSubscription->addEventFilterNumeric();
      pFilter->mCompareType = Math::cEqualTo;//default
      
      if(getVar(cCompare)->isUsed())
      {
         pFilter->mCompareType = getVar(cCompare)->asOperator()->readVar();
      }
      if(getVar(cInteger)->isUsed())
      {
         pFilter->mInteger = getVar(cInteger)->asInteger()->readVar();
      }
      if(getVar(cFloat)->isUsed())
      {
         pFilter->mFloat = getVar(cFloat)->asFloat()->readVar();
      }
   }
}
//==============================================================================
// 
//==============================================================================
void BTriggerEffect::teGetBuildingTrainQueue()
{
   enum  { cBuilding = 1, cCount = 2, cTime = 3};

//-- FIXING PREFIX BUG ID 5545
   const BUnit* pBuilding = gWorld->getUnit(getVar(cBuilding)->asUnit()->readVar());
//--
   
   BPlayerID player;
   
   player = pBuilding->getPlayerID();  //make player optional input for special buildings / map hooks ?

   long count;
   float time;
   pBuilding->getTrainQueue(player, &count, &time);

   if(getVar(cCount)->isUsed())
   {
      getVar(cCount)->asInteger()->writeVar((int)count);
   }
   if(getVar(cTime)->isUsed())
   {
      getVar(cTime)->asTime()->writeVar((DWORD)(time * 1000));
   }

}


//==============================================================================
// Get the closest squad out of a squad list relative to a location
//==============================================================================
void BTriggerEffect::teGetClosestSquad()
{
   enum
   {
      cInSquadList = 1,
      cInTestLoc = 2,
      cOutSquadList = 3,
   };

   BEntityIDArray inSquads = getVar(cInSquadList)->asSquadList()->readVar();
   BVector inTestLoc = getVar(cInTestLoc)->asVector()->readVar();
   BEntityID closestSquad = cInvalidObjectID;
   float closeDistSq = 0.0f;
   uint numSquads = inSquads.getSize();
   for (uint i = 0; i < numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 5546
      const BSquad* pSquad = gWorld->getSquad(inSquads[i]);
//--
      if (pSquad)
      {
         float distSqr = pSquad->calculateXZDistanceSqr(inTestLoc);
         if ((i == 0) || (closeDistSq > distSqr))
         {
            closeDistSq = distSqr; 
            closestSquad = inSquads[i];
         }
      }
   }

   getVar(cOutSquadList)->asSquad()->writeVar(closestSquad);
}

//==============================================================================
// Move the squad to face the target
//==============================================================================
//void BTriggerEffect::teMoveToFace()
//{
//   enum
//   {
//      cInSquad = 1,
//      cInSquadList = 2,
//      cInTargetUnit = 3,
//      cInTargetSquad = 4,
//      cInTargetLoc = 5,
//      cInIgnoreTurrets = 6,
//   };
//
//   // Collect working squads
//   BEntityIDArray workingSquads;
//   if (getVar(cInSquadList)->isUsed())
//   {
//      workingSquads = getVar(cInSquadList)->asSquadList()->readVar();
//   }
//
//   if (getVar(cInSquad)->isUsed())
//   {
//      workingSquads.add(getVar(cInSquad)->asSquad()->readVar());
//   }
//
//   // Collect target location
//   BVector targetLoc = cInvalidVector;
//   if (getVar(cInTargetUnit)->isUsed())
//   {
//      BUnit* pUnit = gWorld->getUnit(getVar(cInTargetUnit)->asUnit()->readVar());
//      if (pUnit)
//      {
//         targetLoc = pUnit->getPosition();
//      }
//   }
//   else if (getVar(cInTargetSquad)->isUsed())
//   {
//      BSquad* pSquad = gWorld->getSquad(getVar(cInTargetSquad)->asSquad()->readVar());
//      if (pSquad)
//      {
//         targetLoc = pSquad->getAveragePosition();
//      }
//   }
//   else if (getVar(cInTargetLoc)->isUsed())
//   {
//      targetLoc = getVar(cInTargetLoc)->asVector()->readVar();
//   }
//
//   if (targetLoc == cInvalidVector)
//   {
//      return;
//   }
//
//   uint numSquads = workingSquads.getSize();
//   for (uint i = 0; i < numSquads; i++)
//   {
//      BSquad* pSquad = gWorld->getSquad(workingSquads[i]);
//      if (pSquad)
//      {
//         // Move the turrets
//         bool ignoreTurrets = getVar(cInIgnoreTurrets)->isUsed() ? getVar(cInIgnoreTurrets)->asBool()->readVar() : false;
//         bool move = ignoreTurrets;
//         if (!ignoreTurrets)
//         {
//            const BProtoObject* pProtoObject = pSquad->getProtoObject();
//            if (pProtoObject)
//            {
//               long hardPointID = pProtoObject->findHardpoint("Turret");
//               if (hardPointID != -1)
//               {
//                  
//               }
//               else
//               {
//                  move = true;
//               }
//            }
//         }
//
//         // Move the squads
//         if (move)
//         {
//
//         }
//      }
//   }
//}


//==============================================================================
// Add a time to a time list
//==============================================================================
void BTriggerEffect::teTimeListAdd()
{
   enum 
   { 
      cAddTime = 1, 
      cAddList = 2, 
      cClearExisting = 3, 
      cDestList = 4, 
   };

   BDWORDArray destList = getVar(cDestList)->asTimeList()->readVar();

   if (getVar(cClearExisting)->isUsed())
   {
      if(getVar(cClearExisting)->asBool()->readVar() == true)
         destList.clear();
   }

   if (getVar(cAddTime)->isUsed())
   {
      destList.add(getVar(cAddTime)->asTime()->readVar());
   }

   if (getVar(cAddList)->isUsed())
   {
      BDWORDArray addList = getVar(cAddList)->asTimeList()->readVar();
      destList.append(addList);
   }

   getVar(cDestList)->asTimeList()->writeVar(destList);
}

//==============================================================================
// Call correct version
//==============================================================================
void BTriggerEffect::teTimeListRemove()
{
   switch (getVersion())
   {
      case 2:
         teTimeListRemoveV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Remove a time from a time list
//==============================================================================
void BTriggerEffect::teTimeListRemoveV2()
{
   enum 
   { 
      cRemoveOne = 1,       
      cRemoveAll = 3, 
      cRemoveDupes = 4, 
      cModifyList = 5,
      cRemoveList = 6, 
   };

//-- FIXING PREFIX BUG ID 5547
   const BTriggerVarTime* pRemoveOne = getVar(cRemoveOne)->asTime();
//--
   BTriggerVarTimeList* pRemoveList = getVar(cRemoveList)->asTimeList();
//-- FIXING PREFIX BUG ID 5548
   const BTriggerVarBool* pRemoveAll = getVar(cRemoveAll)->asBool();
//--
//-- FIXING PREFIX BUG ID 5549
   const BTriggerVarBool* pRemoveDupes = getVar(cRemoveDupes)->asBool();
//--
   BTriggerVarTimeList* pModifyList = getVar(cModifyList)->asTimeList();
   bool removeDupes = (pRemoveDupes->isUsed() && pRemoveDupes->readVar() == true);

   // EASY!
   if (pRemoveAll->isUsed() && pRemoveAll->readVar() == true)
   {
      pModifyList->clear();
      return;
   }
   if (pRemoveOne->isUsed())
   {
      if (removeDupes)
         pModifyList->removeTimeAll(pRemoveOne->readVar());
      else
         pModifyList->removeTime(pRemoveOne->readVar());
   }
   if (pRemoveList->isUsed())
   {
      const BDWORDArray& removeList = pRemoveList->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i = 0; i < numToRemove; i++)
      {
         if (removeDupes)
            pModifyList->removeTimeAll(removeList[i]);
         else
            pModifyList->removeTime(removeList[i]);
      }
   }
}

//==============================================================================
// Get the size of a time list
//==============================================================================
void BTriggerEffect::teTimeListGetSize()
{
   enum 
   { 
      cTimeList = 1, 
      cSize = 2, 
   };

   const BDWORDArray& timeList = getVar(cTimeList)->asTimeList()->readVar();
   getVar(cSize)->asInteger()->writeVar(timeList.getSize());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teClearBlackMap()
{
   gWorld->clearBlackMap();
}


//==============================================================================
// Add a design line to a design line list
//==============================================================================
void BTriggerEffect::teDesignLineListAdd()
{
   enum 
   { 
      cAddDesignLine = 1, 
      cAddDesignLineList = 2, 
      cClearExisting = 3, 
      cDestList = 4, 
   };

   BDesignLineIDArray destList = getVar(cDestList)->asDesignLineList()->readVar();

   if (getVar(cClearExisting)->isUsed())
   {
      if(getVar(cClearExisting)->asBool()->readVar() == true)
         destList.clear();
   }

   if (getVar(cAddDesignLine)->isUsed())
   {
      destList.add(getVar(cAddDesignLine)->asDesignLine()->readVar());
   }

   if (getVar(cAddDesignLineList)->isUsed())
   {
      BDesignLineIDArray addList = getVar(cAddDesignLineList)->asDesignLineList()->readVar();
      destList.append(addList);
   }

   getVar(cDestList)->asDesignLineList()->writeVar(destList);
}

//==============================================================================
// Remove a design line from a design line list
//==============================================================================
void BTriggerEffect::teDesignLineListRemove()
{
   enum 
   { 
      cRemoveOne = 1,       
      cRemoveList = 2, 
      cRemoveAll = 3, 
      cRemoveDupes = 4, 
      cModifyList = 5,      
   };

//-- FIXING PREFIX BUG ID 5550
   const BTriggerVarDesignLine* pRemoveOne = getVar(cRemoveOne)->asDesignLine();
//--
   BTriggerVarDesignLineList* pRemoveList = getVar(cRemoveList)->asDesignLineList();
//-- FIXING PREFIX BUG ID 5551
   const BTriggerVarBool* pRemoveAll = getVar(cRemoveAll)->asBool();
//--
//-- FIXING PREFIX BUG ID 5552
   const BTriggerVarBool* pRemoveDupes = getVar(cRemoveDupes)->asBool();
//--
   BTriggerVarDesignLineList* pModifyList = getVar(cModifyList)->asDesignLineList();
   bool removeDupes = (pRemoveDupes->isUsed() && pRemoveDupes->readVar() == true);

   if (pRemoveAll->isUsed() && pRemoveAll->readVar() == true)
   {
      pModifyList->clear();
      return;
   }

   if (pRemoveOne->isUsed())
   {
      if (removeDupes)
         pModifyList->removeLineAll(pRemoveOne->readVar());
      else
         pModifyList->removeLine(pRemoveOne->readVar());
   }

   if (pRemoveList->isUsed())
   {
      const BDesignLineIDArray& removeList = pRemoveList->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i = 0; i < numToRemove; i++)
      {
         if (removeDupes)
            pModifyList->removeLineAll(removeList[i]);
         else
            pModifyList->removeLine(removeList[i]);
      }
   }
}

//==============================================================================
// Get the number of design lines in a design line list
//==============================================================================
void BTriggerEffect::teDesignLineListGetSize()
{
   enum 
   { 
      cDesignLineList = 1, 
      cSize = 2, 
   };

   const BDesignLineIDArray& desigLineList = getVar(cDesignLineList)->asDesignLineList()->readVar();
   getVar(cSize)->asInteger()->writeVar(desigLineList.getSize());
}




//==============================================================================
// Get the number of ProtoSquads in the list
//==============================================================================
void BTriggerEffect::teProtoSquadListGetSize()
{
   enum 
   { 
      cProtoSquadList = 1, 
      cSize = 2, 
   };

   const BProtoSquadIDArray& protoSquadList = getVar(cProtoSquadList)->asProtoSquadList()->readVar();
   getVar(cSize)->asInteger()->writeVar(protoSquadList.getSize());   
}

//==============================================================================
// Get the obstruction radius for the unit, squad, or object
//==============================================================================
void BTriggerEffect::teGetObstructionRadius()
{
   switch (getVersion())
   {
      case 2:
         teGetObstructionRadiusV2();
         break;

      case 3:
         teGetObstructionRadiusV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Get the obstruction radius for the unit, squad, or object
//==============================================================================
void BTriggerEffect::teGetObstructionRadiusV2()
{
   enum
   {
      cInUnit = 1,
      cInSquad = 2,
      cInObject = 3,
      cOutRadius = 4,
      cInProtoObject = 5,
   };

   BEntityID id = cInvalidObjectID;
   BProtoObjectID protoID = cInvalidProtoObjectID;
   float result = 0.0f;
   if (getVar(cInUnit)->isUsed())
   {
      id = getVar(cInUnit)->asUnit()->readVar();
   }
   else if (getVar(cInSquad)->isUsed())
   {
      id = getVar(cInSquad)->asSquad()->readVar();
   }
   else if (getVar(cInObject)->isUsed())
   {
      id = getVar(cInObject)->asObject()->readVar();
   }
   else if (getVar(cInProtoObject)->isUsed())
   {
      protoID = getVar(cInProtoObject)->asProtoObject()->readVar();
   }

   if (id != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 5554
      const BEntity* pEntity = gWorld->getEntity(id);
//--
      if (pEntity)
      {
         result = pEntity->getObstructionRadius();
      }
   }
   else if (protoID != cInvalidProtoObjectID)
   {
//-- FIXING PREFIX BUG ID 5330
      const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
//--
      if (pProtoObject)
      {
         result = pProtoObject->getObstructionRadius();
      }
   }

   getVar(cOutRadius)->asFloat()->writeVar(result);
}

//==============================================================================
// Get the obstruction radius for the unit, squad, or object
//==============================================================================
void BTriggerEffect::teGetObstructionRadiusV3()
{
   enum
   {
      cInUnit = 1,
      cInSquad = 2,
      cInObject = 3,
      cOutRadius = 4,
      cInProtoObject = 5,
      cOutRadiusX = 6,
      cOutRadiusY = 7,
      cOutRadiusZ = 8,
   };

   BEntityID id = cInvalidObjectID;
   BProtoObjectID protoID = cInvalidProtoObjectID;
   float result = 0.0f;
   float resultX = 0.0f;
   float resultY = 0.0f;
   float resultZ = 0.0f;
   if (getVar(cInUnit)->isUsed())
   {
      id = getVar(cInUnit)->asUnit()->readVar();
   }
   else if (getVar(cInSquad)->isUsed())
   {
      id = getVar(cInSquad)->asSquad()->readVar();
   }
   else if (getVar(cInObject)->isUsed())
   {
      id = getVar(cInObject)->asObject()->readVar();
   }
   else if (getVar(cInProtoObject)->isUsed())
   {
      protoID = getVar(cInProtoObject)->asProtoObject()->readVar();
   }

   if (id != cInvalidObjectID)
   {
      BEntity* pEntity = gWorld->getEntity(id);
      if (pEntity)
      {
         result = pEntity->getObstructionRadius();
         resultX = pEntity->getObstructionRadiusX();
         resultY = pEntity->getObstructionRadiusY();
         resultZ = pEntity->getObstructionRadiusZ();
      }
   }
   else if (protoID != cInvalidProtoObjectID)
   {
//-- FIXING PREFIX BUG ID 5332
      const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
//--
      if (pProtoObject)
      {
         result = pProtoObject->getObstructionRadius();
         resultX = pProtoObject->getObstructionRadiusX();
         resultY = pProtoObject->getObstructionRadiusY();
         resultZ = pProtoObject->getObstructionRadiusZ();
      }
   }

   if (getVar(cOutRadius)->isUsed())
   {
      getVar(cOutRadius)->asFloat()->writeVar(result);
   }

   if (getVar(cOutRadiusX)->isUsed())
   {
      getVar(cOutRadiusX)->asFloat()->writeVar(resultX);
   }

   if (getVar(cOutRadiusY)->isUsed())
   {
      getVar(cOutRadiusY)->asFloat()->writeVar(resultY);
   }

   if (getVar(cOutRadiusZ)->isUsed())
   {
      getVar(cOutRadiusZ)->asFloat()->writeVar(resultZ);
   }
}

//==============================================================================
// Create and fly-in multiple squads
//==============================================================================
void BTriggerEffect::teCreateSquads()
{
   enum 
   { 
      cInProtoSquads = 1, 
      cInPlayer = 2, 
      cInCreatePosition = 3,       
      cInFacing = 4,
      cInFlyInStart = 5,
      cInFlyInEnd = 6,      
      cInRallyPoint = 7,
      cInAttackMoveRallyPoint = 8,      
      cOutputCreatedSquads = 9, 
      cOutputAddToList = 10, 
      cOutputClearExisting = 11, 
   };

   BProtoSquadIDArray flyInProtoSquads = getVar(cInProtoSquads)->asProtoSquadList()->readVar();
   BPlayerID playerID = getVar(cInPlayer)->asPlayer()->readVar();
   BVector createPos = getVar(cInCreatePosition)->asVector()->readVar();   
   bool useFacing = getVar(cInFacing)->isUsed();
   BVector facing = useFacing ? getVar(cInFacing)->asVector()->readVar() : cZAxisVector;
   bool useFlyInStart = getVar(cInFlyInStart)->isUsed();
   bool useFlyInEnd = getVar(cInFlyInEnd)->isUsed();
   bool useRallyPoint = getVar(cInRallyPoint)->isUsed();
   BVector rallyPoint = useRallyPoint ? getVar(cInRallyPoint)->asVector()->readVar() : cInvalidVector;
   bool attackMove = getVar(cInAttackMoveRallyPoint)->isUsed() ? getVar(cInAttackMoveRallyPoint)->asBool()->readVar() : false;
   bool useOutputCreatedSquads = getVar(cOutputCreatedSquads)->isUsed();
   bool useOutputAddToList = getVar(cOutputAddToList)->isUsed();
   bool clearExisting = getVar(cOutputClearExisting)->isUsed() ? getVar(cOutputClearExisting)->asBool()->readVar() : false;      
   BVector right; 
   right.assignCrossProduct(cYAxisVector, facing);
   right.normalize();
   bool flyInSuccess = false;

   // Create squads
   BEntityIDArray workingSquads;
   uint numFlyInProtoSquads = flyInProtoSquads.getSize();
   for (uint i = 0; i < numFlyInProtoSquads; i++)
   {
      BEntityID createdSquadID = gWorld->createEntity(flyInProtoSquads[i], true, playerID, createPos, facing, right, true);
//-- FIXING PREFIX BUG ID 5556
      const BSquad* pSquad = gWorld->getSquad(createdSquadID);
//--
      if (pSquad)
      {
         workingSquads.uniqueAdd(createdSquadID);
      }
   }   
   
   uint numWorkingSquads = workingSquads.getSize();
   if (numWorkingSquads > 0)
   {      
#ifndef BUILD_FINAL
      gTriggerManager.incrementCreatePerfCount(numWorkingSquads);
#endif

      if (useFlyInStart || useFlyInEnd)
      {
         BPlayer* pPlayer = gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 5557
         const BCiv* pCiv = pPlayer ? pPlayer->getCiv() : NULL;
//--
         BProtoObjectID transportProtoID = pCiv ? pCiv->getTransportTriggerProtoID() : cInvalidProtoID;
         if (transportProtoID != cInvalidProtoID)
         {
            const BVector start = useFlyInStart ? getVar(cInFlyInStart)->asVector()->readVar() : cInvalidVector;
            const BVector end = useFlyInEnd ? getVar(cInFlyInEnd)->asVector()->readVar() : cInvalidVector;
            const BVector* pStart = (start == cInvalidVector) ? NULL : &start;
            const BVector* pEnd = (end == cInvalidVector) ? NULL : &end;

            // Create an order
            BSimOrder* pOrder = gSimOrderManager.createOrder();
            // Init order
            if (pOrder)  
            {
               pOrder->setPriority(BSimOrder::cPriorityTrigger);
            }

            flyInSuccess = BSquadActionTransport::flyInSquads(pOrder, workingSquads, pStart, createPos, cZAxisVector, cXAxisVector, pEnd, playerID, transportProtoID, useRallyPoint, rallyPoint, attackMove, cInvalidCueIndex, true, useFacing, facing);
#ifndef BUILD_FINAL
            // Account for transport squads
            if (flyInSuccess)
            {               
               gTriggerManager.incrementCreatePerfCount(BSquadActionTransport::calculateNumTransports(workingSquads));
            }
#endif
         }
      }

      if(!flyInSuccess && useRallyPoint)
      {
         BVectorArray wayPoints;
         wayPoints.add(createPos);

         if (useRallyPoint)
         {
            wayPoints.add(rallyPoint);
         }

         float obsRadius = 0.0f;
         for (uint i = 0; i < numWorkingSquads; i++)
         {
//-- FIXING PREFIX BUG ID 5558
            const BSquad* pSquad = gWorld->getSquad(workingSquads[i]);
//--
            if (pSquad && (obsRadius < pSquad->getObstructionRadius()))
            {
               obsRadius = pSquad->getObstructionRadius();
            }
         }

         facing.normalize();
         facing *= (0.5f + obsRadius);
         facing += wayPoints[wayPoints.getSize() - 1];
         wayPoints.add(facing);

         // Build the move command.
         BWorkCommand tempCommand;         
         tempCommand.setWaypoints(wayPoints.getPtr(), wayPoints.getNumber());

         // Creat an army for the squads
         BObjectCreateParms objectParms;
         objectParms.mPlayerID = playerID;
         BArmy* pArmy = gWorld->createArmy(objectParms);
         if (pArmy)
         {
            pArmy->addSquads(workingSquads, !gConfig.isDefined(cConfigClassicPlatoonGrouping));
         }

         // Set the command to be from a trigger to the army
         tempCommand.setRecipientType(BCommand::cArmy);
         tempCommand.setSenderType(BCommand::cTrigger);

         // Set the attack move
         tempCommand.setFlag(BWorkCommand::cFlagAttackMove, attackMove);

         //Give the command to the army.
         pArmy->queueOrder(&tempCommand);

#ifndef BUILD_FINAL
         gTriggerManager.incrementWorkPerfCount(numWorkingSquads);
#endif
      }
   }

   if (useOutputCreatedSquads)
   {
      getVar(cOutputCreatedSquads)->asSquadList()->writeVar(workingSquads);
   }

   if (useOutputAddToList)
   {
      BEntityIDArray outAddToList = getVar(cOutputAddToList)->asSquadList()->readVar();
      if (clearExisting)
      {
         outAddToList.clear();
      }

      outAddToList.append(workingSquads);
      getVar(cOutputAddToList)->asSquadList()->writeVar(outAddToList);
   }
}


//==============================================================================
// Enable camera functionality
//==============================================================================
void BTriggerEffect::teSetCamera()
{
   switch (getVersion())
   {
      case 2:
         teSetCameraV2();
         break;

      case 3:
         teSetCameraV3();
         break;

      case 4:
         teSetCameraV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Enable camera functionality
//==============================================================================
void BTriggerEffect::teSetCameraV2()
{
   enum
   {
      cInEnableScroll = 1,
      cInEnableYaw = 2,
      cInEnableZoom = 3,
      cInLocation = 4,
      cInDirection = 5,
      cInPlayer = 6,
      cInPlayerList = 7,
   };

   bool enableScroll = getVar(cInEnableScroll)->asBool()->readVar();
   bool enableYaw = getVar(cInEnableYaw)->asBool()->readVar();
   bool enableZoom = getVar(cInEnableZoom)->asBool()->readVar();

   bool locUsed = getVar(cInLocation)->isUsed();
   bool dirUsed = getVar(cInDirection)->isUsed();
   BVector loc = locUsed ? getVar(cInLocation)->asVector()->readVar() : cInvalidVector;
   BVector dir = dirUsed ? getVar(cInDirection)->asVector()->readVar() : cInvalidVector;

   BPlayerIDArray players;
   if (getVar(cInPlayerList)->isUsed())
   {
      players = getVar(cInPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cInPlayer)->isUsed())
   {
      players.uniqueAdd(getVar(cInPlayer)->asPlayer()->readVar());
   }
   
   uint numPlayers = players.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = (BPlayer*)gWorld->getPlayer(players[i]);
      if (pPlayer)
      {
         BUser* pUser = pPlayer->getUser();
         if (pUser && (pUser->getUserMode() == BUser::cUserModeNormal))
         {
            pUser->setFlagCameraScrollEnabled(enableScroll);
            pUser->setFlagCameraYawEnabled(enableYaw);
            pUser->setFlagCameraZoomEnabled(enableZoom);

            BCamera* pCamera = pUser->getCamera();
            if (pCamera)
            {         
               if (locUsed)
               {
                  /*
                  BVector offsetPos = loc - (pCamera->getCameraDir() * pUser->getCameraZoom());
                  pCamera->setCameraLoc(offsetPos);
                  */

                  // Plant to camera height field
                  pUser->computeClosestCameraHoverPointVertical(loc, loc);

                  BVector hoverPointPos(loc);
                  pUser->setCameraHoverPoint(hoverPointPos);

                  BVector cameraPos = hoverPointPos - (pCamera->getCameraDir() * pUser->getCameraZoom());
                  pCamera->setCameraLoc(cameraPos);

                  pUser->setFlagTeleportCamera(true);
               }

               if (dirUsed)
               {
                  BVector pos = locUsed ? loc : pUser->getCameraHoverPoint();
                  const BVector& camDir = pCamera->getCameraDir();
                  BVector tempDir = camDir;
                  tempDir.y = 0.0f;
                  dir.y = 0.0f;
                  tempDir.normalize();
                  dir.normalize();      
                  float cosAngle = dir.dot(tempDir);
                  float angle = acosf(cosAngle);
                  pCamera->yawWorldAbout(angle, pos);            
               }

               pUser->setFlagUpdateHoverPoint(true);
            }
         }
      }
   }
}

//==============================================================================
// Enable camera functionality
//==============================================================================
void BTriggerEffect::teSetCameraV3()
{
   enum
   {
      cInEnableScroll = 1,
      cInEnableYaw = 2,
      cInEnableZoom = 3,
      cInLocation = 4,
      cInDirection = 5,
      cInPlayer = 6,
      cInPlayerList = 7,
      cAbsolutePos = 8,
      cAbsoluteDir = 9,      
   };

   bool enableScroll = getVar(cInEnableScroll)->asBool()->readVar();
   bool enableYaw = getVar(cInEnableYaw)->asBool()->readVar();
   bool enableZoom = getVar(cInEnableZoom)->asBool()->readVar();   

   bool locUsed = getVar(cInLocation)->isUsed();
   bool dirUsed = getVar(cInDirection)->isUsed();
   BVector loc = locUsed ? getVar(cInLocation)->asVector()->readVar() : cInvalidVector;
   BVector dir = dirUsed ? getVar(cInDirection)->asVector()->readVar() : cInvalidVector;
   bool absPos = getVar(cAbsolutePos)->isUsed() ? getVar(cAbsolutePos)->asBool()->readVar() : false;
   bool absDir = getVar(cAbsoluteDir)->isUsed() ? getVar(cAbsoluteDir)->asBool()->readVar() : false;

   BPlayerIDArray players;
   if (getVar(cInPlayerList)->isUsed())
   {
      players = getVar(cInPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cInPlayer)->isUsed())
   {
      players.uniqueAdd(getVar(cInPlayer)->asPlayer()->readVar());
   }

   uint numPlayers = players.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BUser* pUser = gUserManager.getUserByPlayerID(players[i]);
      if (pUser && (pUser->getUserMode() == BUser::cUserModeNormal))
      {
         pUser->setFlagCameraScrollEnabled(enableScroll);
         pUser->setFlagCameraYawEnabled(enableYaw);
         pUser->setFlagCameraZoomEnabled(enableZoom);

         BCamera* pCamera = pUser->getCamera();
         if (pCamera)
         {    
            if ((locUsed && absPos) || (dirUsed && absDir))
            {
               pUser->setFlagCameraAutoZoomInstantEnabled(false);
               pUser->setFlagCameraAutoZoomEnabled(false);
            }
            else
            {
               bool resetCam = (!pUser->getFlagCameraAutoZoomInstantEnabled() || !pUser->getFlagCameraAutoZoomEnabled());
               pUser->setFlagCameraAutoZoomInstantEnabled(gConfig.isDefined(cConfigCameraAutoZoomInstant));
               pUser->setFlagCameraAutoZoomEnabled(true);
               if (resetCam)
               {
                  pUser->resetCameraSettings(false);
               }
            }

            if (locUsed)
            {
               /*
               BVector offsetPos = cInvalidVector;
               if (absPos)
               {
                  offsetPos = loc;
               }
               else
               {
                  offsetPos = loc - (pCamera->getCameraDir() * pUser->getCameraZoom());
               }
               pCamera->setCameraLoc(offsetPos);
               */

               // Plant to camera height field
               pUser->computeClosestCameraHoverPointVertical(loc, loc);

               BVector hoverPointPos(loc);
               pUser->setCameraHoverPoint(hoverPointPos);

               BVector cameraPos = hoverPointPos - (pCamera->getCameraDir() * pUser->getCameraZoom());
               pCamera->setCameraLoc(cameraPos);

               pUser->setFlagTeleportCamera(true);
            }

            if (dirUsed)
            {
               if (absDir)
               {  
                  // Yaw
                  if (!dir.xzEqualTo(cOriginVector))
                  {
                     BVector xzDir = dir;
                     xzDir.y = 0.0f;
                     xzDir.normalize();

                     pCamera->setCameraDir(xzDir);
                     pCamera->setCameraUp(cYAxisVector);
                     pCamera->calcCameraRight();
                  }
                  else
                  {
                     BVector xzDir = pCamera->getCameraDir();
                     xzDir.y = 0.0f;
                     xzDir.normalize();

                     pCamera->setCameraDir(xzDir);
                     pCamera->setCameraUp(cYAxisVector);
                     pCamera->calcCameraRight();
                  }

                  // Pitch     
                  if (Math::fAbs(dir.y) > cFloatCompareEpsilon)
                  {
                     BVector yawDir = pCamera->getCameraDir();                        
                     float angle = dir.angleBetweenVector(yawDir);
                     angle = (dir.y > 0.0f) ? -angle : angle;
                     bool pitchLimit = pCamera->getPitchLimitOn();
                     pCamera->setPitchLimitOn(false);
                     pCamera->pitch(angle);
                     pCamera->setPitchLimitOn(pitchLimit);
                  }
               }
               else
               {                  
                  BVector pos = locUsed ? loc : pUser->getCameraHoverPoint();
                  BVector camDir = (BVector)pCamera->getCameraDir();
                  camDir.y = 0.0f;
                  dir.y = 0.0f;
                  camDir.normalize();
                  dir.normalize();      
                  float cosAngle = dir.dot(camDir);
                  float angle = acosf(cosAngle);
                  pCamera->yawWorldAbout(angle, pos);            
               }
            }

            pUser->setFlagUpdateHoverPoint(true);
         }
      }
   }
}



//==============================================================================
// Enable camera functionality
//==============================================================================
void BTriggerEffect::teSetCameraV4()
{
   enum
   {
      cInEnableScroll = 1,
      cInEnableYaw = 2,
      cInEnableZoom = 3,
      cInLocation = 4,
      cInDirection = 5,
      cInPlayer = 6,
      cInPlayerList = 7,
      cAbsolutePos = 8,
      cAbsoluteDir = 9,     
      cHeightOffset = 10,
   };

   bool enableScroll = getVar(cInEnableScroll)->asBool()->readVar();
   bool enableYaw = getVar(cInEnableYaw)->asBool()->readVar();
   bool enableZoom = getVar(cInEnableZoom)->asBool()->readVar();   

   bool locUsed = getVar(cInLocation)->isUsed();
   bool dirUsed = getVar(cInDirection)->isUsed();
   BVector loc = locUsed ? getVar(cInLocation)->asVector()->readVar() : cInvalidVector;
   BVector dir = dirUsed ? getVar(cInDirection)->asVector()->readVar() : cInvalidVector;
   bool absPos = getVar(cAbsolutePos)->isUsed() ? getVar(cAbsolutePos)->asBool()->readVar() : false;
   bool absDir = getVar(cAbsoluteDir)->isUsed() ? getVar(cAbsoluteDir)->asBool()->readVar() : false;
   bool heightOffsetUsed = getVar(cHeightOffset)->isUsed();
   float heightOffset = heightOffsetUsed ? getVar(cHeightOffset)->asFloat()->readVar() : 0.0f;



   BPlayerIDArray players;
   if (getVar(cInPlayerList)->isUsed())
   {
      players = getVar(cInPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cInPlayer)->isUsed())
   {
      players.uniqueAdd(getVar(cInPlayer)->asPlayer()->readVar());
   }

   uint numPlayers = players.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BUser* pUser = gUserManager.getUserByPlayerID(players[i]);
      if (pUser && (pUser->getUserMode() == BUser::cUserModeNormal))
      {
         pUser->setFlagCameraScrollEnabled(enableScroll);
         pUser->setFlagCameraYawEnabled(enableYaw);
         pUser->setFlagCameraZoomEnabled(enableZoom);

         BCamera* pCamera = pUser->getCamera();
         if (pCamera)
         {    
            if (heightOffsetUsed)
            {
               pUser->setCameraHoverPointOffsetHeight(heightOffset);
            }

            // absPos and absDir are not implement here since they can cause bad things,
            // I don't think they are used anywhere though.
            // 
            BASSERTM(!absPos, "BTriggerEffect::teSetCameraV4 - absolute position no longer supported");
            BASSERTM(!absDir, "BTriggerEffect::teSetCameraV4 - absolute direction no longer supported");

            if ((locUsed && absPos) || (dirUsed && absDir))
            {
               pUser->setFlagCameraAutoZoomInstantEnabled(false);
               pUser->setFlagCameraAutoZoomEnabled(false);
            }
            else
            {
               bool resetCam = (!pUser->getFlagCameraAutoZoomInstantEnabled() || !pUser->getFlagCameraAutoZoomEnabled());
               pUser->setFlagCameraAutoZoomInstantEnabled(gConfig.isDefined(cConfigCameraAutoZoomInstant));
               pUser->setFlagCameraAutoZoomEnabled(true);
               if (resetCam)
               {
                  pUser->resetCameraSettings(false);
               }
            }

            if (dirUsed)
            {
               BVector camDir = (BVector)pCamera->getCameraDir();
               camDir.y = 0.0f;
               dir.y = 0.0f;
               camDir.normalize();
               dir.normalize();      
               float cosAngle = dir.dot(camDir);
               float angle = acosf(cosAngle);
               pCamera->yawWorld(-angle);            
            }

            if (locUsed)
            {
               // Plant to camera height field
               pUser->computeClosestCameraHoverPointVertical(loc, loc);

               BVector hoverPointPos(loc);
               pUser->setCameraHoverPoint(hoverPointPos);

               BVector cameraPos = hoverPointPos - (pCamera->getCameraDir() * pUser->getCameraZoom());
               pCamera->setCameraLoc(cameraPos);

               pUser->setFlagTeleportCamera(true);
            }

            pUser->setFlagUpdateHoverPoint(true);
         }
      }
   }
}


//==============================================================================
// Add a float to a list
//==============================================================================
void BTriggerEffect::teFloatListAdd()
{
   enum 
   { 
      cAddFloat = 1, 
      cAddList = 2, 
      cClearExisting = 3, 
      cDestList = 4, 
   };

   BFloatArray destList = getVar(cDestList)->asFloatList()->readVar();

   if (getVar(cClearExisting)->isUsed())
   {
      if(getVar(cClearExisting)->asBool()->readVar() == true)
         destList.clear();
   }

   if (getVar(cAddFloat)->isUsed())
   {
      destList.add(getVar(cAddFloat)->asFloat()->readVar());
   }

   if (getVar(cAddList)->isUsed())
   {
      BFloatArray addList = getVar(cAddList)->asFloatList()->readVar();
      uint numFloats = addList.getSize();
      for (uint i = 0; i < numFloats; i++)
      {
         destList.add(addList[i]);
      }
   }

   getVar(cDestList)->asFloatList()->writeVar(destList);
}

//==============================================================================
// Remove a float from a list
//==============================================================================
void BTriggerEffect::teFloatListRemove()
{
   enum 
   { 
      cRemoveOne = 1, 
      cRemoveList = 2, 
      cRemoveAll = 3, 
      cRemoveDupes = 4, 
      cModifyList = 5, 
   };

//-- FIXING PREFIX BUG ID 5559
   const BTriggerVarFloat* pRemoveOne = getVar(cRemoveOne)->asFloat();
//--
   BTriggerVarFloatList* pRemoveList = getVar(cRemoveList)->asFloatList();
//-- FIXING PREFIX BUG ID 5560
   const BTriggerVarBool* pRemoveAll = getVar(cRemoveAll)->asBool();
//--
//-- FIXING PREFIX BUG ID 5561
   const BTriggerVarBool* pRemoveDupes = getVar(cRemoveDupes)->asBool();
//--
   BTriggerVarFloatList* pModifyList = getVar(cModifyList)->asFloatList();
   bool removeDupes = (pRemoveDupes->isUsed() && pRemoveDupes->readVar() == true);

   // EASY!
   if (pRemoveAll->isUsed() && pRemoveAll->readVar() == true)
   {
      pModifyList->clear();
      return;
   }

   if (pRemoveOne->isUsed())
   {
      if (removeDupes)
         pModifyList->removeFloatAll(pRemoveOne->readVar());
      else
         pModifyList->removeFloat(pRemoveOne->readVar());
   }

   if (pRemoveList->isUsed())
   {
      const BFloatArray& removeList = pRemoveList->readVar();
      uint numToRemove = removeList.getSize();
      for (uint i = 0; i < numToRemove; i++)
      {
         if (removeDupes)
            pModifyList->removeFloatAll(removeList[i]);
         else
            pModifyList->removeFloat(removeList[i]);
      }
   }
}

//==============================================================================
// Get the number of floats in a list
//==============================================================================
void BTriggerEffect::teFloatListGetSize()
{
   enum 
   { 
      cFloatList = 1, 
      cSize = 2, 
   };

   const BFloatArray& floatList = getVar(cFloatList)->asFloatList()->readVar();
   getVar(cSize)->asInteger()->writeVar(floatList.getSize());
}

//==============================================================================
// Repairs a list of squads using combat value
//==============================================================================
void BTriggerEffect::teRepairByCombatValue()
{
   switch (getVersion())
   {
   case 1:
      teRepairByCombatValueV1();
      break;

   case 2:
      teRepairByCombatValueV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}

//==============================================================================
// Repairs a list of squads using combat value
//==============================================================================
void BTriggerEffect::teRepairByCombatValueV2()
{
   enum 
   { 
      cSquad     = 1, 
      cSquadList = 2, 
      cUnit      = 3,
      cUnitList  = 4,
      cCVP       = 5,
      cCVPPercent= 6,
      cSpread    = 7,
      cDamageTakenScalar = 8,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useCVP = getVar(cCVP)->isUsed();
   bool useCVPPercent = getVar(cCVPPercent)->isUsed();
   bool useSpread = getVar(cSpread)->isUsed();
   bool spreadAcrossUnits = useSpread ? getVar(cSpread)->asBool()->readVar() : false;
   bool useDamageTakenScalar = getVar(cDamageTakenScalar)->isUsed();

   //BEntityIDArray unitList;
   BEntityIDArray squadList;

   if (!useCVP && !useCVPPercent)
   {
      BTRIGGER_ASSERTM(false, "No values assigned.");
      return;
   }
   if (!useSquad && !useSquadList && !useUnit && !useUnitList)
   {
      BTRIGGER_ASSERTM(false, "No targets specified.");
      return;
   }

   float damageTakenScalar = useDamageTakenScalar ? getVar(cDamageTakenScalar)->asFloat()->readVar() : 1.0f;

   // If unit list input is used
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long                  numUnits = unitList.getNumber();
      for (long i = 0; i < numUnits; i++)
      {
//-- FIXING PREFIX BUG ID 5562
         const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
         if (pUnit)
         {
            if (pUnit->getParentSquad())
               squadList.uniqueAdd(pUnit->getParentSquad()->getID());
         }
      }
   }

   // If unit input is used
   if (useUnit)
   {
//-- FIXING PREFIX BUG ID 5563
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit && pUnit->getParentSquad())
         squadList.uniqueAdd(pUnit->getParentSquad()->getID());
   }

   // If squad input is used
   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   // If squad list input is used
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   long numSquads = squadList.getNumber();
   if (numSquads == 0)
      return;

   // Spread combat value repairing over each squad proportional to the 
   // combat value of that squad.  Excess combat value spills over and
   // is divided up to remaining squads.
   if (useCVP && spreadAcrossUnits)
   {
      float totalRepairCombatValue = getVar(cCVP)->asFloat()->readVar();
      if (totalRepairCombatValue <= 0.0f)
         return;

      // Make list of squads that need repairing and total
      // up their combat values
      BDynamicSimFloatArray repairCosts;
      BDynamicSimFloatArray baseCombatValues;
      float totalCombatValue = 0.0f;
      BEntityIDArray squads;
      for (long i = 0; i < numSquads; i++)
      {
         const BSquad* pSquad=gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            BCost squadCost;
            float squadCombatValueRepairCost = 0.0f;
            if (pSquad->getRepairCost(squadCost, squadCombatValueRepairCost))
            {
               squads.add(pSquad->getID());

               float baseCV = pSquad->getCombatValue();
               baseCombatValues.add(baseCV);
               repairCosts.add(squadCombatValueRepairCost);
               totalCombatValue += baseCV;
            }
         }
      }

      long numSquadsToHeal = squads.getNumber();
      if (numSquadsToHeal <= 0)
         return;

      if (totalCombatValue < cFloatCompareEpsilon)
         return;

      // Sort squads by excess repair cost (repair slice - repair cost
      BDynamicSimFloatArray sortByRepairDiff;
      BDynamicSimFloatArray sortedBaseCombatValues;
      BEntityIDArray sortedSquads;

      for (long i = 0; i < numSquadsToHeal; i++)
      {
         // Calculate amount of heal cost proportional to squad's combat value
         float squadRepairPortion = (baseCombatValues[i] / totalCombatValue) * totalRepairCombatValue;
         float repairDiff = squadRepairPortion - repairCosts[i];

         // Insert in sorted order
         long j = 0;
         while ((j < sortByRepairDiff.getNumber()) && (repairDiff < sortByRepairDiff[j]))
            j++;
         sortByRepairDiff.insertAtIndex(repairDiff, j);
         sortedBaseCombatValues.insertAtIndex(baseCombatValues[i], j);
         sortedSquads.insertAtIndex(squads[i], j);
      }

      // Heal squads base on cost percentage
      float remainingRepair = totalRepairCombatValue;
      float remainingCombatValue = totalCombatValue;
      for (long i = 0; i < numSquadsToHeal; i++)
      {
         BSquad* pSquad=gWorld->getSquad(sortedSquads[i]);
         float squadHealCost = (sortedBaseCombatValues[i] / remainingCombatValue) * remainingRepair;

         float excessRepair = 0.0f;
         pSquad->repairCombatValue(squadHealCost, true, excessRepair, damageTakenScalar);

         remainingRepair -= (squadHealCost - excessRepair);
         remainingCombatValue -= sortedBaseCombatValues[i];
      }
   }
   // Just repair each squad by the specified combat value
   // or combat value percent
   else
   {
      float repairCV = 0.0f;
      if (useCVP)
         repairCV = getVar(cCVP)->asFloat()->readVar();
      else if (useCVPPercent)
         repairCV = getVar(cCVPPercent)->asFloat()->readVar();
      if (repairCV <= 0.0f)
         return;

      for (long i = 0; i < numSquads; i++)
      {
         BSquad* pSquad=gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            float cvDelta = 0.0f;
            if (useCVP)
               cvDelta = repairCV;
            else if (useCVPPercent)
               cvDelta = repairCV * pSquad->getCombatValue();

            float excess;
            pSquad->repairCombatValue(cvDelta, true, excess, damageTakenScalar);
         }
      }
   }
}

//==============================================================================
// Repairs a list of squads using combat value
//==============================================================================
void BTriggerEffect::teRepairByCombatValueV1()
{
   enum 
   { 
      cSquad     = 1, 
      cSquadList = 2, 
      cUnit      = 3,
      cUnitList  = 4,
      cCVP       = 5,
      cCVPPercent= 6,
      cSpread    = 7,
   };

   bool useSquad = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();
   bool useUnit = getVar(cUnit)->isUsed();
   bool useUnitList = getVar(cUnitList)->isUsed();
   bool useCVP = getVar(cCVP)->isUsed();
   bool useCVPPercent = getVar(cCVPPercent)->isUsed();
   bool useSpread = getVar(cSpread)->isUsed();
   bool spreadAcrossUnits = useSpread ? getVar(cSpread)->asBool()->readVar() : false;

   //BEntityIDArray unitList;
   BEntityIDArray squadList;

   if (!useCVP && !useCVPPercent)
   {
      BTRIGGER_ASSERTM(false, "No values assigned.");
      return;
   }
   if (!useSquad && !useSquadList && !useUnit && !useUnitList)
   {
      BTRIGGER_ASSERTM(false, "No targets specified.");
      return;
   }

   // If unit list input is used
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long                  numUnits = unitList.getNumber();
      for (long i = 0; i < numUnits; i++)
      {
//-- FIXING PREFIX BUG ID 5564
         const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
         if (pUnit)
         {
            if (pUnit->getParentSquad())
               squadList.uniqueAdd(pUnit->getParentSquad()->getID());
         }
      }
   }
   
   // If unit input is used
   if (useUnit)
   {
//-- FIXING PREFIX BUG ID 5565
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit && pUnit->getParentSquad())
         squadList.uniqueAdd(pUnit->getParentSquad()->getID());
   }

   // If squad input is used
   if (useSquad)
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   // If squad list input is used
   if (useSquadList)
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   long numSquads = squadList.getNumber();
   if (numSquads == 0)
      return;

   // Spread combat value repairing over each squad proportional to the 
   // combat value of that squad.  Excess combat value spills over and
   // is divided up to remaining squads.
   if (useCVP && spreadAcrossUnits)
   {
      float totalRepairCombatValue = getVar(cCVP)->asFloat()->readVar();
      if (totalRepairCombatValue <= 0.0f)
         return;

      // Make list of squads that need repairing and total
      // up their combat values
      BDynamicSimFloatArray repairCosts;
      BDynamicSimFloatArray baseCombatValues;
      float totalCombatValue = 0.0f;
      BEntityIDArray squads;
      for (long i = 0; i < numSquads; i++)
      {
         const BSquad* pSquad=gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            BCost squadCost;
            float squadCombatValueRepairCost = 0.0f;
            if (pSquad->getRepairCost(squadCost, squadCombatValueRepairCost))
            {
               squads.add(pSquad->getID());

               float baseCV = pSquad->getCombatValue();
               baseCombatValues.add(baseCV);
               repairCosts.add(squadCombatValueRepairCost);
               totalCombatValue += baseCV;
            }
         }
      }

      long numSquadsToHeal = squads.getNumber();
      if (numSquadsToHeal <= 0)
         return;

      if (totalCombatValue < cFloatCompareEpsilon)
         return;

      // Sort squads by excess repair cost (repair slice - repair cost
      BDynamicSimFloatArray sortByRepairDiff;
      BDynamicSimFloatArray sortedBaseCombatValues;
      BEntityIDArray sortedSquads;

      for (long i = 0; i < numSquadsToHeal; i++)
      {
         // Calculate amount of heal cost proportional to squad's combat value
         float squadRepairPortion = (baseCombatValues[i] / totalCombatValue) * totalRepairCombatValue;
         float repairDiff = squadRepairPortion - repairCosts[i];

         // Insert in sorted order
         long j = 0;
         while ((j < sortByRepairDiff.getNumber()) && (repairDiff < sortByRepairDiff[j]))
            j++;
         sortByRepairDiff.insertAtIndex(repairDiff, j);
         sortedBaseCombatValues.insertAtIndex(baseCombatValues[i], j);
         sortedSquads.insertAtIndex(squads[i], j);
      }

      // Heal squads base on cost percentage
      float remainingRepair = totalRepairCombatValue;
      float remainingCombatValue = totalCombatValue;
      for (long i = 0; i < numSquadsToHeal; i++)
      {
         BSquad* pSquad=gWorld->getSquad(sortedSquads[i]);
         float squadHealCost = (sortedBaseCombatValues[i] / remainingCombatValue) * remainingRepair;

         float excessRepair = 0.0f;
         pSquad->repairCombatValue(squadHealCost, true, excessRepair);

         remainingRepair -= (squadHealCost - excessRepair);
         remainingCombatValue -= sortedBaseCombatValues[i];
      }
   }
   // Just repair each squad by the specified combat value
   // or combat value percent
   else
   {
      float repairCV = 0.0f;
      if (useCVP)
         repairCV = getVar(cCVP)->asFloat()->readVar();
      else if (useCVPPercent)
         repairCV = getVar(cCVPPercent)->asFloat()->readVar();
      if (repairCV <= 0.0f)
         return;

      for (long i = 0; i < numSquads; i++)
      {
         BSquad* pSquad=gWorld->getSquad(squadList[i]);
         if (pSquad)
         {
            float cvDelta = 0.0f;
            if (useCVP)
               cvDelta = repairCV;
            else if (useCVPPercent)
               cvDelta = repairCV * pSquad->getCombatValue();

            float excess;
            pSquad->repairCombatValue(cvDelta, true, excess);
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetUTechBldings()
{
   enum { cPlayer = 3, cTech = 1, cAvailableBuildings = 2 };
//-- FIXING PREFIX BUG ID 5566
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
   if (!pPlayer)
      return;
   long techId = getVar(cTech)->asTech()->readVar();

   uint count = pPlayer->getAvailableUniqueTechBuildings(techId);

   getVar(cAvailableBuildings)->asInteger()->writeVar(count);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teInputUILocationMinigame()
{
   enum 
   { 
      cInputPlayer = 1, 
      cOutputUILocationMinigame = 2, 
      cInputProtoObject = 3, 
      cInputCheckObstruction = 4, 
      cLosType = 5,
      cInputPlacementRule = 6,
      cInputLOSCenteryOnly = 7,
      cInputPower = 8,
      cInputPowerMinigame = 9,
      cInputPowerMinigameTimeFactor = 10,
      cInputRestartMinigame = 11
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BProtoObjectID protoObjectID = getVar(cInputProtoObject)->isUsed() ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   bool checkObstruction = getVar(cInputCheckObstruction)->isUsed() ? getVar(cInputCheckObstruction)->asBool()->readVar() : false;
   long losType = getVar(cLosType)->isUsed() ? getVar(cLosType)->asLOSType()->readVar() : BWorld::cCPLOSDontCare;
   getVar(cOutputUILocationMinigame)->asUILocationMinigame()->writeResult(BTriggerVarUILocation::cUILocationResultWaiting);
   BTriggerVarID UILocationMinigameVarID = getVar(cOutputUILocationMinigame)->getID();
   BTriggerScriptID triggerScriptID = getParentTriggerScript()->getID();
   long placementRuleIndex = getVar(cInputPlacementRule)->isUsed() ? getVar(cInputPlacementRule)->asPlacementRule()->readVar() : -1;
   bool losCenterOnly = getVar(cInputLOSCenteryOnly)->isUsed() ? getVar(cInputLOSCenteryOnly)->asBool()->readVar() : false;
   long protoPowerID = getVar(cInputPower)->isUsed() ? getVar(cInputPower)->asPower()->readVar() : -1;
   bool minigame = getVar(cInputPowerMinigame)->isUsed() ? getVar(cInputPowerMinigame)->asBool()->readVar() : false;
   float timeFactor = getVar(cInputPowerMinigameTimeFactor)->isUsed() ? getVar(cInputPowerMinigameTimeFactor)->asFloat()->readVar() : 1.0f;
   bool restartMinigame = getVar(cInputRestartMinigame)->isUsed() ? getVar(cInputRestartMinigame)->asBool()->readVar() : true;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // Lock the user and change modes if possible.
   if (!pUser->isUserLocked() || pUser->isUserLockedByTriggerScript(triggerScriptID))
   {
      pUser->lockUser(triggerScriptID);
      pUser->setUIPowerRadius(0.0f);
      pUser->setBuildProtoID(protoObjectID);
      pUser->setUILocationCheckObstruction(checkObstruction);
      pUser->setUILocationLOSType(losType);
      pUser->setUILocationLOSCenterOnly(losCenterOnly);
      pUser->setUILocationCheckMoving(false);
      pUser->setUILocationPlacementSuggestion(false);
      pUser->setUILocationPlacementRuleIndex(placementRuleIndex);
      pUser->setTriggerScriptID(triggerScriptID);
      pUser->setTriggerVarID(UILocationMinigameVarID);
      pUser->setUIProtoPowerID(protoPowerID);
      pUser->setFlagUIPowerMinigame(minigame);
      pUser->setMinigameTimeFactor(timeFactor);
      pUser->setFlagUIRestartMinigame(restartMinigame);
      pUser->changeMode(BUser::cUserModeInputUILocationMinigame);
   }
   else
   {
      BTriggerCommand* pCommand = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandTrigger);
      pCommand->setSenders(1, &playerID);
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setRecipientType(BCommand::cPlayer);
      pCommand->setType(BTriggerCommand::cTypeBroadcastInputUILocationMinigameResult);
      pCommand->setTriggerScriptID(triggerScriptID);
      pCommand->setTriggerVarID(UILocationMinigameVarID);
      pCommand->setInputResult(BTriggerVarUILocation::cUILocationResultUILockError);
      pCommand->setInputLocation(cInvalidVector);
      pCommand->setInputQuality(0.0f);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}


//==================================================================================
// Find the closest path out of a list of design lines to a given squad or location
//==================================================================================
void BTriggerEffect::teGetClosestPath()
{
   enum
   {
      cInSquad = 1,
      cInLoc = 2,
      cInPathList = 3,
      cOutPath = 4,
   };

   BDesignLineID resultPath = -1;
   BVector testLoc = cInvalidVector;
   if (getVar(cInSquad)->isUsed())
   {
      BSquad* pSquad = gWorld->getSquad(getVar(cInSquad)->asSquad()->readVar());
      if (pSquad)
      {
         testLoc = pSquad->getAveragePosition();
      }
   }
   else if (getVar(cInLoc)->isUsed())
   {
      testLoc = getVar(cInLoc)->asVector()->readVar();
   }

   if (testLoc != cInvalidVector)
   {
      BDesignLineIDArray paths = getVar(cInPathList)->asDesignLineList()->readVar();   
      BDesignObjectManager* pDesignObjectManager = gWorld->getDesignObjectManager();
      if (pDesignObjectManager)
      {
         BDesignLine& firstPath = pDesignObjectManager->getDesignLine(paths[0]);
         float shortestDistSq = testLoc.xzDistanceSqr(firstPath.mPoints[0]);
         resultPath = 0;
         bool newDistFound = false;
         uint numPaths = paths.getSize();
         for (uint i = 0; i < numPaths; i++)
         {            
            BDesignLine& path = pDesignObjectManager->getDesignLine(paths[i]);

            // Find closest way point
            uint closeLocIndex = 0;
            uint numPathPoints = path.mPoints.getSize();
            for (uint j = 0; j < numPathPoints; j++)
            {               
               float distSq = testLoc.xzDistanceSqr(path.mPoints[j]);
               if (distSq < shortestDistSq)
               {
                  shortestDistSq = distSq;
                  closeLocIndex = j; 
                  newDistFound = true;
               }
            }

            // Find closest point from previous and next way point line segments
            BVector prevPoint = cInvalidVector;
            float prevDistSq = 0.0f;
            if (closeLocIndex > 0)
            {
               prevPoint = closestPointOnLine(path.mPoints[closeLocIndex - 1], path.mPoints[closeLocIndex], testLoc);
               prevDistSq = testLoc.xzDistanceSqr(prevPoint);
            }

            BVector nextPoint = cInvalidVector;
            float nextDistSq = 0.0f;
            uint nextIndex = closeLocIndex + 1;
            if (nextIndex < numPathPoints)
            {
               nextPoint = closestPointOnLine(path.mPoints[closeLocIndex], path.mPoints[nextIndex], testLoc);
               nextDistSq = testLoc.xzDistanceSqr(nextPoint);
            }

            if ((prevPoint != cInvalidVector) && (nextPoint != cInvalidVector))
            {
               float minDistSq = Math::Min(prevDistSq, nextDistSq);
               if (minDistSq < shortestDistSq)
               {
                  shortestDistSq = minDistSq;
                  newDistFound = true;
               }
            }
            else if ((prevPoint != cInvalidVector) && (prevDistSq < shortestDistSq))
            {
               shortestDistSq = prevDistSq;
               newDistFound = true;
            }
            else if ((nextPoint != cInvalidVector) && (nextDistSq < shortestDistSq))
            {
               shortestDistSq = nextDistSq;
               newDistFound = true;
            }

            if (newDistFound)
            {
               resultPath = paths[i];
               newDistFound = false;
            }
         }
      }
   }

   getVar(cOutPath)->asDesignLine()->writeVar(resultPath);
}

//==============================================================================
// Set the selectable override flag
//==============================================================================
void BTriggerEffect::teSetSelectable()
{
   enum
   {
      cInUnit = 1,
      cInUnitList = 2,
      cInSquad = 3,
      cInSquadList = 4,
      cInObject = 5,
      cInObjectList = 6,
      cInSelectable = 7,
   };

   BEntityIDArray entities;
   if (getVar(cInUnitList)->isUsed())
   {
      entities = getVar(cInUnitList)->asUnitList()->readVar();
   }

   if (getVar(cInUnit)->isUsed())
   {
      entities.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }

   BEntityIDArray squads;
   if (getVar(cInSquadList)->isUsed())
   {      
      squads = getVar(cInSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInSquad)->isUsed())
   {
      squads.uniqueAdd(getVar(cInSquad)->asSquad()->readVar());
   }

   uint numSquads = squads.getSize();
   for (uint i = 0; i < numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 5568
      const BSquad* pSquad = gWorld->getSquad(squads[i]);
//--
      if (pSquad)
      {
         entities.uniqueAdd(squads[i]);
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            BEntityID unitID = pSquad->getChild(j);
//-- FIXING PREFIX BUG ID 5567
            const BUnit* pUnit = gWorld->getUnit(unitID);
//--
            if (pUnit)
            {
               entities.uniqueAdd(unitID);
            }
         }
      }
   }

   if (getVar(cInObjectList)->isUsed())
   {
      entities.append(getVar(cInObjectList)->asObjectList()->readVar());
   }

   if (getVar(cInObject)->isUsed())
   {
      entities.uniqueAdd(getVar(cInObject)->asObject()->readVar());
   }

   bool selectable = getVar(cInSelectable)->asBool()->readVar();
   uint numEntities = entities.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
      BEntity* pEntity = gWorld->getEntity(entities[i]);
      if (pEntity)
      {
         pEntity->setFlagSelectable(selectable);
      }
   }
}

//==============================================================================
// Set the player(s) trickle rate
//==============================================================================
void BTriggerEffect::teSetTrickleRate()
{
   enum
   {
      cInPlayer = 1,
      cInPlayerList = 2,
      cInCost = 3,
   };

   BPlayerIDArray players;
   if (getVar(cInPlayerList)->isUsed())
   {
      players = getVar(cInPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cInPlayer)->isUsed())
   {
      players.uniqueAdd(getVar(cInPlayer)->asPlayer()->readVar());
   }

   BCost cost = getVar(cInCost)->asCost()->readVar();

   uint numPlayers = players.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BPlayer* pPlayer = (BPlayer*)gWorld->getPlayer(players[i]);
      if (pPlayer)
      {
#ifdef SYNC_Player
         syncPlayerData("BTriggerEffect::teSetTrickleRate i", (DWORD)i);
#endif
         pPlayer->setResourceTrickleRate(cost);
      }
   }
}

//==============================================================================
// Get the player(s) trickle rate
//==============================================================================
void BTriggerEffect::teGetTrickleRate()
{
   enum
   {
      cInPlayer = 1,
      cOutCost = 2,
   };

   BCost cost;
   cost.zero();
   BPlayerID player = getVar(cInPlayer)->asPlayer()->readVar();
   const BPlayer* pPlayer = gWorld->getPlayer(player);
   if (pPlayer)
   {
      cost = pPlayer->getResourceTrickleRate();
   }

   getVar(cOutCost)->asCost()->writeVar(cost);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teChatDestroy()
{
   BChatMessage* pMessage =gWorld->getChatManager()->getChat();

   if (pMessage != NULL)
   {
      gWorld->getChatManager()->removeChat(pMessage);
      gUIManager->setChatVisible(false);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teShowMessage()
{
   enum 
   { 
      cPlayer = 1, 
      cPlayerList = 2, 
      cSoundString = 3, 
      cQueueSound = 4, 
      cLocStringID = 5, 
      cDuration = 6, 
   };  

   long stringID = stringID = getVar(cLocStringID)->asLocStringID()->readVar();
   DWORD duration = getVar(cDuration)->asTime()->readVar();

   duration /= 1000;    // convert ms to sec

   BSimString soundCue;
   soundCue.empty();
   bool queueSound = false;
   if (getVar(cSoundString)->isUsed())
   {
      soundCue = getVar(cSoundString)->asSound()->readVar();
      queueSound = getVar(cQueueSound)->isUsed() ? getVar(cQueueSound)->asBool()->readVar() : false;
   }

   BPlayerIDArray playerIDs;
   if (getVar(cPlayerList)->isUsed())
   {
      playerIDs = getVar(cPlayerList)->asPlayerList()->readVar();
   }

   if (getVar(cPlayer)->isUsed())
   {
      playerIDs.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());
   }

   uint numPlayers = playerIDs.getSize();
   bool allUsers = false;
   if (numPlayers == 0)
   {
      numPlayers = gUserManager.getUserCount();
      allUsers = true;
   }

   for (uint i = 0; i < numPlayers; i++)
   {
      BUser* pUser = allUsers ? gUserManager.getUser(i) : gUserManager.getUserByPlayerID(playerIDs[i]);
      if (pUser && pUser->getFlagUserActive())
      {
         pUser->addFlashUserMessage(stringID, soundCue, queueSound, (float)duration);
      }               
   }
}

//==============================================================================
// Fade in/out a color
//==============================================================================
void BTriggerEffect::teFadeToColor()
{
   enum
   {
      cInColor = 1,      
      cInDuration = 2,
      cInFadeIn = 3,
   };

   BColor color = getVar(cInColor)->asColor()->readVar();
   bool fadeIn = getVar(cInFadeIn)->isUsed() ? getVar(cInFadeIn)->asBool()->readVar() : false;
   DWORD duration = getVar(cInDuration)->asTime()->readVar();

   // If subscriber exists reset
   BGeneralEventSubscriber* pSubscription = gWorld->getTransitionManager()->getCompletedEventSubscriber();
   if (pSubscription)
      pSubscription->mFired = false;
   // Otherwise create a new subscriber
   else
      pSubscription = gGeneralEventManager.newSubscriber(BEventDefinitions::cFadeCompleted);

   BASSERT(pSubscription);      
   if (fadeIn)
      gWorld->getTransitionManager()->doFadeUp((float)duration * 0.001f, 0.0f, 0.0f, color, pSubscription);
   else
      gWorld->getTransitionManager()->doFadeDown((float)duration * 0.001f, 0.0f, 0.0f, color, pSubscription);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teSquadFlagSet()
{
   enum { cSquad = 1, cSquadList = 2, cFlagType = 3, cFlagValue = 4, };

   long flagType = getVar(cFlagType)->asSquadFlag()->readVar();
   if (flagType == -1)
      return;
   bool flagValue = getVar(cFlagValue)->asBool()->readVar();

//-- FIXING PREFIX BUG ID 5569
   const BTriggerVarSquad* pVarSquad = getVar(cSquad)->asSquad();
//--
   if (pVarSquad->isUsed())
      squadFlagSetHelper(pVarSquad->readVar(), flagType, flagValue);

//-- FIXING PREFIX BUG ID 5570
   const BTriggerVarSquadList* pVarSquadList = getVar(cSquadList)->asSquadList();
//--
   if (pVarSquadList->isUsed())
   {
      const BEntityIDArray& squadList = pVarSquadList->readVar();
      uint numSquads = squadList.getSize();
      for (uint i=0; i<numSquads; i++)
         squadFlagSetHelper(squadList[i], flagType, flagValue);
   }
}


//==============================================================================
// Set whether the squad(s), unit(s), or object(s) are auto attackable
//==============================================================================
void BTriggerEffect::teSetAutoAttackable()
{
   enum
   {
      cInUnit = 1,
      cInUnitList = 2,
      cInSquad = 3,
      cInSquadList = 4,
      cInAutoAttackable = 7,
   };

   BEntityIDArray objectList;
   if (getVar(cInUnitList)->isUsed())
   {
      objectList.append(getVar(cInUnitList)->asUnitList()->readVar());
   }

   if (getVar(cInUnit)->isUsed())
   {
      objectList.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }

   if (getVar(cInSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cInSquadList)->asSquadList()->readVar();
      uint numSquads = squadList.getSize();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 5571
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            objectList.append(pSquad->getChildList());
         }
      }
   }

   if (getVar(cInSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 5572
      const BSquad* pSquad = gWorld->getSquad(getVar(cInSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         objectList.append(pSquad->getChildList());
      }
   }

   bool autoAttackable = getVar(cInAutoAttackable)->asBool()->readVar();

   uint numObjects = objectList.getSize();
   for (uint i = 0; i < numObjects; i++)
   {
      BObject* pObject = gWorld->getObject(objectList[i]);
      if (pObject)
      {
         pObject->setFlagDontAutoAttackMe(!autoAttackable);
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teFadeTransition()
{
   enum
   {
      cInFadeDown = 1,
      cInHold = 2,
      cInFadeUp = 3,
      cInReverse = 4,
      cInColor = 5
   };

   BColor color = getVar(cInColor)->asColor()->readVar();
   bool reverse = getVar(cInReverse)->isUsed() ? getVar(cInReverse)->asBool()->readVar() : false;
   DWORD fadeDown = getVar(cInFadeDown)->asTime()->readVar();
   DWORD hold = getVar(cInHold)->asTime()->readVar();
   DWORD fadeUp = getVar(cInFadeUp)->asTime()->readVar();

   // If subscriber exists reset
   BGeneralEventSubscriber* pSubscription = gWorld->getTransitionManager()->getCompletedEventSubscriber();
   if (pSubscription)
      pSubscription->mFired = false;
   // Otherwise create a new subscriber
   else
      pSubscription = gGeneralEventManager.newSubscriber(BEventDefinitions::cFadeCompleted);

   BASSERT(pSubscription);      
   if (reverse)
      gWorld->getTransitionManager()->doFadeUpHoldFadeDown((float)fadeUp * 0.001f, (float)hold * 0.001f, (float)fadeDown * 0.001f, color, pSubscription);
   else
      gWorld->getTransitionManager()->doFadeDownHoldFadeUp((float)fadeDown * 0.001f, (float)hold * 0.001f, (float)fadeUp * 0.001f, color, pSubscription);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teFlashUIElement()
{
   switch (getVersion())
   {
      case 1:
         teFlashUIElementV1();
         break;

      case 2:
         teFlashUIElementV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teFlashUIElementV2()
{
   enum
   {
      cUIElement = 1,
      cFlashOn = 2,
      cInputPlayer = 3,
   };

   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   int uiElement = getVar(cUIElement)->asUIItem()->readVar();
   bool flashOn = getVar(cFlashOn)->asBool()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;

   // call the context to flash (or stop flashing) the UI.
   BUIContext* pContext = pUser->getUIContext();
   pContext->flashUIElement(uiElement, flashOn);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teFlashUIElementV1()
{
   enum
   {
      cUIElement = 1,
      cFlashOn = 2,
   };

   //int uiElement = getVar(cUIElement)->asUIItem()->readVar();
   //bool flashOn = getVar(cFlashOn)->asBool()->readVar();

   // already not implemented
}

//==============================================================================
// BTriggerEffect::teTeamSetDiplomacy
//==============================================================================
void BTriggerEffect::teTeamSetDiplomacy()
{
   enum { cSourceTeam = 1, cSourceTeamList, cDiplomacy, cTargetTeam, cTargetTeamList };

   bool bSrcTeam = getVar(cSourceTeam)->isUsed();
   bool bSrcTeamList = getVar(cSourceTeamList)->isUsed();
   bool bTgtTeam = getVar(cTargetTeam)->isUsed();
   bool bTgtTeamList = getVar(cTargetTeamList)->isUsed();

   BTeamIDArray srcTeams;
   BTeamIDArray tgtTeams;

   if (bSrcTeamList)
      srcTeams = getVar(cSourceTeamList)->asTeamList()->readVar();

   if (bSrcTeam)
      srcTeams.add(getVar(cSourceTeam)->asTeam()->readVar());

   if (bTgtTeamList)
      tgtTeams = getVar(cTargetTeamList)->asTeamList()->readVar();

   if (bTgtTeam)
      tgtTeams.add(getVar(cTargetTeam)->asTeam()->readVar());

   BRelationType relationType = getVar(cDiplomacy)->asRelationType()->readVar();
   for (int src=0; src<srcTeams.getNumber(); src++)
   {
      for (int tgt=0; tgt<tgtTeams.getNumber(); tgt++)
      {
         gWorld->setTeamRelationType(srcTeams[src], tgtTeams[tgt], relationType);
      }
   }
}
   
//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptGetParameters()
{
   enum { cPlayerID = 1, cConcept = 2, cPage = 3, cVector = 4, cIsVectorSet = 5, cSquadList = 6, cIsSquadListSet = 7, cUnitList = 8, 
      cIsUnitListSet = 9, cEntityFilterSet = 10, cIsEntityFilterSet = 11, cFloat = 12, cIsFloatSet = 13, cObjectType = 14, cIsObjectTypeSet = 15,
      cLocStringID = 16, cIsLocStringIDSet = 17};

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;
   
   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;
  
   int page = getVar(cPage)->asInteger()->readVar();
   int concept = getVar(cConcept)->asConcept()->readVar();

   BParameterPage* pParamPage = pHintEngine->getConceptPage(concept, page);
   if(!pParamPage)
      return;


   if(getVar(cVector)->isUsed())// && pParamPage->mHasVector)
   {      
      if (pParamPage->mHasVector)
         getVar(cVector)->asVector()->writeVar(pParamPage->mVector);
      else
         getVar(cVector)->asVector()->writeVar(BVector(0.0f,0.0f,0.0f));
   }
   if(getVar(cIsVectorSet)->isUsed())
   {      
      getVar(cIsVectorSet)->asBool()->writeVar(pParamPage->mHasVector);
   }  

   if(getVar(cSquadList)->isUsed())// && pParamPage->mHasSquadList)
   {
      if (pParamPage->mHasSquadList)
         getVar(cSquadList)->asSquadList()->writeVar(pParamPage->mSquadList);
      else 
         getVar(cSquadList)->asSquadList()->writeVar(BEntityIDArray());
   }
   if(getVar(cIsSquadListSet)->isUsed())
   {      
      getVar(cIsSquadListSet)->asBool()->writeVar(pParamPage->mHasSquadList);
   }  

   if(getVar(cUnitList)->isUsed())// && pParamPage->mHasUnitList)
   {
      if (pParamPage->mHasUnitList)
         getVar(cUnitList)->asUnitList()->writeVar(pParamPage->mUnitList);
      else
         getVar(cUnitList)->asUnitList()->writeVar(BEntityIDArray());
   }
   if(getVar(cIsUnitListSet)->isUsed())
   {      
      getVar(cIsUnitListSet)->asBool()->writeVar(pParamPage->mHasUnitList);
   }  

   if(getVar(cEntityFilterSet)->isUsed())// && pParamPage->mHasEntityFilterSet)
   {
      if (pParamPage->mHasEntityFilterSet)
         getVar(cEntityFilterSet)->asEntityFilterSet()->writeVar(&(pParamPage->mEntityFilterSet));
      else 
      {
         BEntityFilterSet emptySet;
         getVar(cEntityFilterSet)->asEntityFilterSet()->writeVar(&emptySet);
      }
   }
   if(getVar(cIsEntityFilterSet)->isUsed())
   {      
      getVar(cIsEntityFilterSet)->asBool()->writeVar(pParamPage->mHasEntityFilterSet);
   }  

   if(getVar(cFloat)->isUsed())// && pParamPage->mHasFloat)
   {
      if (pParamPage->mHasFloat)
         getVar(cFloat)->asFloat()->writeVar(pParamPage->mFloat);
      else
         getVar(cFloat)->asFloat()->writeVar(0.0f);
   }
   if(getVar(cIsFloatSet)->isUsed())
   {      
      getVar(cIsFloatSet)->asBool()->writeVar(pParamPage->mHasFloat);
   }  
   if(getVar(cObjectType)->isUsed())
   {
      if (pParamPage->mHasObjectType)
         getVar(cObjectType)->asObjectType()->writeVar(pParamPage->mObjectType);
      else
         getVar(cObjectType)->asObjectType()->writeVar(0);
   }
   if(getVar(cIsObjectTypeSet)->isUsed())
   {      
      getVar(cIsObjectTypeSet)->asBool()->writeVar(pParamPage->mHasObjectType);
   }  
   if(getVar(cLocStringID)->isUsed())
   {
      if (pParamPage->mHasLocStringID)
         getVar(cLocStringID)->asLocStringID()->writeVar(pParamPage->mLocStringID);
      else
         getVar(cLocStringID)->asLocStringID()->writeVar(0);
   }
   if(getVar(cIsLocStringIDSet)->isUsed())
   {      
      getVar(cIsLocStringIDSet)->asBool()->writeVar(pParamPage->mHasLocStringID);
   }  
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptSetttings()
{
      //NOT IMPL
}
   
//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptPermission()
{
   enum { cPlayerID = 1, cAllowAll = 3, cAllowList = 2 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;

   bool allowAll = false;
   BInt32Array list;
   if(getVar(cAllowAll)->isUsed())
   {
      allowAll = getVar(cAllowAll)->asBool()->readVar();
   }
   if(getVar(cAllowList)->isUsed())
   {
      list = getVar(cAllowList)->asConceptList()->readVar();
   }
   pHintEngine->setPermission(allowAll, list);

}
   
//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptSetState()
{
   enum { cPlayerID = 1, cConcept = 2, cState = 3 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;

   int state = getVar(cState)->asInteger()->readVar();
   int concept = getVar(cConcept)->asConcept()->readVar();

   pHintEngine->onStateChange(concept, state);


}
   
//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptSetPrecondition()
{
   enum { cPlayerID = 1, cConcept = 2, cPreCondition = 3 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;

   int precond = getVar(cPreCondition)->asInteger()->readVar();
   int concept = getVar(cConcept)->asConcept()->readVar();

   pHintEngine->onPrecondition(concept, precond);

}
   
//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptStartSub()
{
   enum { cPlayerID = 1, cConcept = 2, cParentConcept = 3, cWaitTime = 4 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;
   
   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;

   int parentconcept = getVar(cParentConcept)->asConcept()->readVar();
   int concept = getVar(cConcept)->asConcept()->readVar();

   float delay = -1;
   if(getVar(cWaitTime)->isUsed())
   {
      delay = getVar(cWaitTime)->asTime()->readVar() / 1000.0f;
   }

   pHintEngine->startSubHint(concept, parentconcept, delay);

}
   
//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptClearSub()
{
   enum { cPlayerID = 1, cConcept = 2  };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;


   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;

   int concept = getVar(cConcept)->asConcept()->readVar();

   pHintEngine->stopSubHints(concept);
}
   
//==============================================================================
//==============================================================================
void BTriggerEffect::teConceptSetParameters()
{
   enum { cPlayerID = 1, cConcept = 2, cPage = 3, cVector = 4, cSquadList = 5, cUnitList = 6, 
      cEntityFilterSet = 7, cFloat = 9, cObjectType = 10 , cLocStringID = 11 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;
  
   int page = getVar(cPage)->asInteger()->readVar();
   int concept = getVar(cConcept)->asConcept()->readVar();

   BParameterPage* pParamPage = pHintEngine->getConceptPage(concept, page);
   if(!pParamPage)
      return;

   
   pParamPage->clear();//zap the page.

   if(getVar(cVector)->isUsed())
   {
      pParamPage->mVector = getVar(cVector)->asVector()->readVar();
      pParamPage->mHasVector = true;
   }
   if(getVar(cSquadList)->isUsed())
   {
      pParamPage->mSquadList = getVar(cSquadList)->asSquadList()->readVar();
      pParamPage->mHasSquadList = true;
   }
   if(getVar(cUnitList)->isUsed())
   {
      pParamPage->mUnitList = getVar(cUnitList)->asUnitList()->readVar();
      pParamPage->mHasUnitList = true;
   }
   if(getVar(cEntityFilterSet)->isUsed())
   {
      pParamPage->mEntityFilterSet.copyFilterSet( getVar(cEntityFilterSet)->asEntityFilterSet()->readVar() );
      pParamPage->mHasEntityFilterSet = true;
   }
   if(getVar(cFloat)->isUsed())
   {
      pParamPage->mFloat = getVar(cFloat)->asFloat()->readVar();
      pParamPage->mHasFloat = true;
   }
   if(getVar(cObjectType)->isUsed())
   {
      pParamPage->mObjectType = getVar(cObjectType)->asObjectType()->readVar();
      pParamPage->mHasObjectType = true;
   }
   if(getVar(cLocStringID)->isUsed())
   {
      pParamPage->mLocStringID = getVar(cLocStringID)->asLocStringID()->readVar();
      pParamPage->mHasLocStringID = true;
   }   
}
//==============================================================================
//==============================================================================
void BTriggerEffect::teGetNPCPlayersByName()
{
   enum { cNPCPlayerName = 1, cNPCPlayerState = 2, cNPCPlayerList = 3, };

   BPlayerIDArray foundPlayers;

   BSimString name = getVar(cNPCPlayerName)->asString()->readVar();

   long numPlayers = gWorld->getNumberPlayers();
   for (long i=0; i<numPlayers; i++)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(i);
      // No bad players.
      if (!pPlayer)
         continue;
      // No non-npc players.
      if (!pPlayer->isNPC())
         continue;
      // No players not in our desired player state (if we care.)
      if (getVar(cNPCPlayerState)->isUsed())
      {
         if (pPlayer->getPlayerState() != getVar(cNPCPlayerState)->asPlayerState()->readVar())
            continue;
      }
      // No players with a different name.
      if (pPlayer->getName() != name)
         continue;
      // Good enough.
      foundPlayers.add(pPlayer->getID());
   }

   getVar(cNPCPlayerList)->asPlayerList()->writeVar(foundPlayers);
}


//==============================================================================
// Get the closest unit to the provided location
//==============================================================================
void BTriggerEffect::teGetClosestUnit()
{
   enum
   {
      cInUnitList = 1,
      cInTestLoc = 2,
      cOutUnit = 4,
   };

   BEntityIDArray inUnits = getVar(cInUnitList)->asUnitList()->readVar();
   BVector inTestLoc = getVar(cInTestLoc)->asVector()->readVar();
   BEntityID closestUnit = cInvalidObjectID;
   float closeDistSq = 0.0f;
   uint numUnits = inUnits.getSize();
   for (uint i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 5573
      const BUnit* pUnit = gWorld->getUnit(inUnits[i]);
//--
      if (pUnit)
      {
         float distSqr = pUnit->calculateXZDistanceSqr(inTestLoc);
         if ((i == 0) || (closeDistSq > distSqr))
         {
            closeDistSq = distSqr; 
            closestUnit = inUnits[i];
         }
      }
   }

   getVar(cOutUnit)->asUnit()->writeVar(closestUnit);
}

//==============================================================================
// Clear the building state to done
//==============================================================================
void BTriggerEffect::teClearBuildingCommandState()
{
   enum
   {
      cOutBuildingState = 1,
   };

   BTriggerVarBuildingCommandState* pState = getVar(cOutBuildingState)->asBuildingCommandState();
   pState->writeResult(BTriggerVarBuildingCommandState::cResultDone);
   pState->clearTrainedSquadIDs();   
}

//==============================================================================
// Make the given tentacle active
//==============================================================================
void BTriggerEffect::teActivateTentacle()
{
   enum { cSquad = 1, cTime = 2 };

   const BEntityID squad = getVar(cSquad)->asSquad()->readVar();
   float time = (getVar(cTime)->asTime()->readVar()) / 1000.0f;

   BSquad* pSquad = gWorld->getSquad(squad);
   if (pSquad)
   {
      long dormantId = gDatabase.getProtoSquad("fld_inf_tentacleDormant_01");

      // If dormant, kill to make it normal
      if (pSquad->getProtoSquadID() == dormantId)
      {
         pSquad->kill(false);
         return;
      }

      // If normal, kick into active state via activity
      long normalId = gDatabase.getProtoSquad("fld_inf_tentacle_01");
      if (pSquad->getProtoSquadID() == normalId)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
         BASSERT(pUnit);

         BUnitActionTentacleDormant* pAction = static_cast<BUnitActionTentacleDormant*>(pUnit->getActionByType(BAction::cActionTypeUnitTentacleDormant));
         if (pAction)
         {
            pAction->forceActive(time);
         }
         return;
      }
   }
}

//==============================================================================
// Recycle a building or its sub buildings.  
// It has options to let you choose which sub buildings to kill/recycle
//==============================================================================
void BTriggerEffect::teRecycleBuilding()
{
   enum { cUnit = 1, cKillSubsOnly = 2, cKillQueued = 3, cKillBuilding = 4, cKillBuilt = 5 };
   bool recycleSubsOnly = false; 
   bool KillQueued = false;
   bool KillBuilding = false;
   bool KillBuilt = false;

   const BEntityID unit = getVar(cUnit)->asUnit()->readVar();

   if(getVar(cKillSubsOnly)->isUsed())
   {
      recycleSubsOnly = getVar(cKillSubsOnly)->asBool()->readVar();
   }
   if(getVar(cKillQueued)->isUsed())
   {
      KillQueued = getVar(cKillQueued)->asBool()->readVar();
   }   
   if(getVar(cKillBuilding)->isUsed())
   {
      KillBuilding = getVar(cKillBuilding)->asBool()->readVar();
   }
   if(getVar(cKillBuilt)->isUsed())
   {
      KillBuilt = getVar(cKillBuilt)->asBool()->readVar();
   }

   BUnit* pUnit = gWorld->getUnit(unit);
   if (pUnit)
   {
      if(recycleSubsOnly == false)
      {
         pUnit->doSelfDestruct(false);      
      }
      else
      {     
         uint numEntityRefs = pUnit->getNumberEntityRefs();
         for(uint j=0; j < numEntityRefs; j++)
         {
//-- FIXING PREFIX BUG ID 5574
            const BEntityRef* pRef = pUnit->getEntityRefByIndex(j);
//--
            if(pRef && pRef->mType == BEntityRef::cTypeBuildQueueChild)
            {
               BUnit* pSocket = gWorld->getUnit(pRef->mID);
               //Kill queued?
               if(KillQueued)
                  pSocket->doSelfDestruct(false);     
            }
            if (pRef && pRef->mType == BEntityRef::cTypeAssociatedBuilding)
            {
               BUnit* pBuilding = gWorld->getUnit(pRef->mID);
               if(pBuilding->getFlagBuilt())
               {
                  //built building?
                  if(KillBuilt)
                     pBuilding->doSelfDestruct(false);  

               }
               else 
               {
                  //building being constructed?
                  if(KillBuilding)
                     pBuilding->doSelfDestruct(false);  
               }
            }           
         }
      }
   }
}

//==============================================================================
// Set the destination for a hook
//==============================================================================
void BTriggerEffect::teSetTeleporterDestination()
{
   enum { cSource = 1, cTarget = 2 };

   BEntityID source = getVar(cSource)->asSquad()->readVar();
   BEntityID target = getVar(cTarget)->asSquad()->readVar();

   BSquad* pSource = gWorld->getSquad(source);
   BASSERT(pSource);
   if (!pSource)
      return;

//-- FIXING PREFIX BUG ID 5578
   const BSquad* pTarget = gWorld->getSquad(target);
//--
   BASSERT(pTarget);
   if (!pTarget)
      return;

   BUnitActionHotDrop* pHotdropSource = static_cast<BUnitActionHotDrop*>(pSource->getLeaderUnit()->getActionByType(BAction::cActionTypeUnitHotDrop));
   BASSERT(pHotdropSource);
   if (!pHotdropSource)
      return;

   pHotdropSource->setHotdropTarget(target);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teSetTowerWallDestination()
{
   enum { cSource = 1, cTarget = 2 };

   BEntityID source = getVar(cSource)->asSquad()->readVar();
   BEntityID target = getVar(cTarget)->asSquad()->readVar();

   BSquad* pSource = gWorld->getSquad(source);
   BASSERT(pSource);
   if (!pSource)
      return;

   BSquad* pTarget = gWorld->getSquad(target);
   BASSERT(pTarget);
   if (!pTarget)
      return;

   BUnitActionTowerWall* pTowerWall = static_cast<BUnitActionTowerWall*>(pSource->getLeaderUnit()->getActionByType(BAction::cActionTypeUnitTowerWall));
   BASSERT(pTowerWall);
   if (!pTowerWall)
      return;

   BSimTarget simTarget(target);
   pTowerWall->setTarget(simTarget);

   //-- Add entity 
   pSource->addEntityRef(BEntityRef::cTypeAssociatedWallTower, pTarget->getID(), 0, 0);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEnableMusicManager()
{
   enum { cEnabled = 1};
   bool enabled = getVar(cEnabled)->asBool()->readVar();
   gWorld->getWorldSoundManager()->getMusicManager()->setEnabled(enabled);
}

//==============================================================================
// Set the number of garrisoned civilians for the custom UI widget
//==============================================================================
void BTriggerEffect::teSetGarrisonedCount()
{
   enum 
   { 
      cWidgetID = 1, 
      cVisible = 2, 
      cInNumGarrisoned = 3, 
   };

   // grab the variables
   int garrisonWidgetID = getVar(cWidgetID)->asInteger()->readVar();
   garrisonWidgetID--;
   if ( (garrisonWidgetID < 0) || (garrisonWidgetID >= BUIWidgets::cNumGarrisonContainers) )
      return;

   bool bVisible = getVar(cVisible)->asBool()->readVar();   
   int numGarrisoned = getVar(cInNumGarrisoned)->isUsed() ? getVar(cInNumGarrisoned)->asInteger()->readVar() : 0;

   // get the widget UI
   BUIWidgets* uiWidgets = gUIManager->getWidgetUI();
   if (!uiWidgets)
   {
      BTRIGGER_ASSERTM(false, "Could not find the Widget UI!");
      return;
   }

   // connect them.
   uiWidgets->setGarrisonedVisible(garrisonWidgetID, bVisible, numGarrisoned);
}

//==============================================================================
// Get the number of players in the list
//==============================================================================
void BTriggerEffect::tePlayerListGetSize()
{
   enum 
   { 
      cInPlayerList = 1, 
      cOutSize = 2, 
   };
   
   const BPlayerIDArray& players = getVar(cInPlayerList)->asPlayerList()->readVar();
   int listSize = players.getNumber();
   getVar(cOutSize)->asInteger()->writeVar(listSize);
}

//==============================================================================
// ConceptResetCooldown
//==============================================================================
void BTriggerEffect::teConceptResetCooldown()
{
   enum 
   { 
      cPlayerID = 1, 
      cConcept = 2, 
      cResetIncrementedTime = 3, 
   };
   
   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;


   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return;
  
   int concept = getVar(cConcept)->asConcept()->readVar();
   bool resetAccumlator = getVar(cResetIncrementedTime)->asBool()->readVar();

   pHintEngine->resetConceptCoolDown(concept, resetAccumlator);

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetPowerRadius()
{
   enum { cPlayerID = 1, cPowerID = 2, cRadius = 3 };

   getVar(cRadius)->asFloat()->writeVar(0.0f);

//-- FIXING PREFIX BUG ID 5579
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
//--
   if (!pPlayer)
      return;

   BPowerID powerID = getVar(cPowerID)->asPower()->readVar();
//-- FIXING PREFIX BUG ID 5580
   const BProtoPower* pPower = gDatabase.getProtoPowerByID(powerID);
//--
   if (!pPower)
      return;

   getVar(cRadius)->asFloat()->writeVar(pPower->getUIRadius());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teTransportSquads2()
{
   enum { cPlayer = 1, cSquadList = 2, cDropoff = 3, };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   const BEntityIDArray& squadsToTransport = getVar(cSquadList)->asSquadList()->readVar();
   long protoObject = gWorld->getPlayer(playerID)->getCiv()->getTransportProtoID();
   BVector dropoffLoc = getVar(cDropoff)->asVector()->readVar();

   BSimOrder* pOrder = gSimOrderManager.createOrder();
   if (!pOrder)  
      return;
   pOrder->setPriority(BSimOrder::cPriorityTrigger);

   BSquadActionTransport::transportSquads(pOrder, squadsToTransport, dropoffLoc, playerID, protoObject);
}

//==============================================================================
// Get a location from a list by list index
//==============================================================================
void BTriggerEffect::teLocationListGetByIndex()
{
   enum
   {
      cInLocs = 1,
      cInIndex = 2,
      cOutPrevLoc = 3,
      cOutCurrentLoc = 4,
      cOutNextLoc = 5,
   };

   BVectorArray locs = getVar(cInLocs)->asVectorList()->readVar();   
   BVector prevLoc = cInvalidVector;
   BVector currentLoc = cInvalidVector;
   BVector nextLoc = cInvalidVector;
   int numLocs = locs.getNumber();
   int index = getVar(cInIndex)->asInteger()->readVar();      
   if (numLocs > 0)
   {
      int maxIndex = numLocs - 1;
      index = Math::Max(0, Math::Min(maxIndex, index));      
      int prevIndex = Math::Max(0, index - 1);
      int nextIndex = Math::Min(maxIndex, index + 1);
      prevLoc = locs[prevIndex];
      currentLoc = locs[index];
      nextLoc = locs[nextIndex];      
   }

   if (getVar(cOutPrevLoc)->isUsed())
   {      
      getVar(cOutPrevLoc)->asVector()->writeVar(prevLoc);
   }

   if (getVar(cOutCurrentLoc)->isUsed())
   {
      getVar(cOutCurrentLoc)->asVector()->writeVar(currentLoc);
   }

   if (getVar(cOutNextLoc)->isUsed())
   {      
      getVar(cOutNextLoc)->asVector()->writeVar(nextLoc);
   }
}

//==============================================================================
// Find the closest location from or along the list to the test location
//==============================================================================
void BTriggerEffect::teLocationListGetClosest()
{
   enum
   {
      cInLocs = 1,
      cInTestLoc = 2,
      cInOnPath = 3,
      cOutClosestLoc = 4,
      cOutClosestIndex = 5,
   };

   BVectorArray locs = getVar(cInLocs)->asVectorList()->readVar();
   BVector testLoc = getVar(cInTestLoc)->asVector()->readVar();
   bool asPath = getVar(cInOnPath)->isUsed() ? getVar(cInOnPath)->asBool()->readVar() : false;
   BVector closestLoc = cInvalidVector;
   int closestIndex = -1;   
   uint numLocs = locs.getSize();

   if (numLocs > 0)
   {
      if (asPath)
      {
         // Find closest way point
         float minDist = testLoc.xzDistanceSqr(locs[0]);
         closestIndex = 0;
         closestLoc = locs[0];
         for (uint i = 1; i < numLocs; i++)
         {
            float testDist = testLoc.xzDistanceSqr(locs[i]);
            if (testDist < minDist)
            {
               minDist = testDist;
               closestIndex = i;
               closestLoc = locs[i];
            }
         }

         // Find closest point from previous and next way point line segments
         BVector prevPoint = cInvalidVector;
         float prevDist = 0.0f;
         if (closestIndex > 0)
         {
            prevPoint = closestPointOnLine(locs[closestIndex - 1], locs[closestIndex], testLoc);
            prevDist = testLoc.xzDistanceSqr(prevPoint);
         }

         BVector nextPoint = cInvalidVector;
         float nextDist = 0.0f;
         uint nextIndex = closestIndex + 1;
         if (nextIndex < numLocs)
         {
            nextPoint = closestPointOnLine(locs[closestIndex], locs[nextIndex], testLoc);
            nextDist = testLoc.xzDistanceSqr(nextPoint);
         }

         if ((prevPoint != cInvalidVector) && (nextPoint != cInvalidVector))
         {
            if (prevDist < nextDist)
            {
               closestIndex--;
               closestLoc = prevPoint;
            }
            else
            {
               closestLoc = nextPoint;
            }
         }
         else if (prevPoint != cInvalidVector)
         {
            closestIndex--;
            closestLoc = prevPoint;
         }
         else
         {
            closestLoc = nextPoint;
         }
      }
      else
      {         
         float minDist = testLoc.distanceSqr(locs[0]);
         closestLoc = locs[0];
         closestIndex = 0;
         for (uint i = 1; i < numLocs; i++)
         {
            float testDist = testLoc.distanceSqr(locs[i]);
            if (testDist < minDist)
            {
               minDist = testDist;
               closestLoc = locs[i];
               closestIndex = i;
            }
         }
      }
   }

   if (getVar(cOutClosestLoc)->isUsed())
   {
      getVar(cOutClosestLoc)->asVector()->writeVar(closestLoc);
   }

   if (getVar(cOutClosestIndex)->isUsed())
   {
      getVar(cOutClosestIndex)->asInteger()->writeVar(closestIndex);
   }
}

//==============================================================================
// Infect the provided squads and/or units.
//==============================================================================
void BTriggerEffect::teInfect()
{
   enum
   {      
      cInFloodPlayer = 1,
      cInSquad = 2,
      cInSquadList = 3,
      cInUnit = 4,
      cInUnitList = 5,
      cInKill = 6,
   };

   BPlayerID floodPlayer = getVar(cInFloodPlayer)->asPlayer()->readVar();
   BEntityIDArray units;
   if (getVar(cInUnitList)->isUsed())
   {
      units = getVar(cInUnitList)->asUnitList()->readVar();
   }

   if (getVar(cInUnit)->isUsed())
   {
      units.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }

   BEntityIDArray squads;
   if (getVar(cInSquadList)->isUsed())
   {
      squads = getVar(cInSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInSquad)->isUsed())
   {
      squads.uniqueAdd(getVar(cInSquad)->asSquad()->readVar());
   }

   uint numEntities = squads.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
//-- FIXING PREFIX BUG ID 5583
      const BSquad* pSquad = gWorld->getSquad(squads[i]);
//--
      if (pSquad)
      {
         units.append(pSquad->getChildList());
      }
   }

   bool killUnit = getVar(cInKill)->isUsed() ? getVar(cInKill)->asBool()->readVar() : false;
   numEntities = units.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
      BUnit* pUnit = gWorld->getUnit(units[i]);
      if (pUnit && !pUnit->getFlagInfected() && !pUnit->getFlagTakeInfectionForm() && pUnit->isInfectable() && !pUnit->getFlagFatalityVictim() && !pUnit->getFlagDoingFatality())
      {
         if (killUnit)
         {
            pUnit->infect(floodPlayer);
            pUnit->kill(false);            
         }
         else
         {
            pUnit->infect(floodPlayer);
            pUnit->takeInfectionForm();
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventDelete()
{
   //This is put on hold in favor of just clearing the filters on a subscriber.

   //enum { cEventID=1 };
   //long id  = getVar(cEventID)->asInteger()->readVar();
   //BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   //if(pSubscription)     
   //{
   //   gGeneralEventManager.removeSubscriber(id);
   //}
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teEventClearFilters()
{
   enum { cEventID=1 };
   long id  = getVar(cEventID)->asInteger()->readVar();
   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.getSubscriber(id);
   if(pSubscription)     
   {
      pSubscription->clearFilters();
   }
}

//==============================================================================
// Flash an entity with a custom color
//==============================================================================
void BTriggerEffect::teFlashEntity()
{
   enum
   {
      cInUnit = 1,
      cInUnitList = 2,
      cInSquad = 3,
      cInSquadList = 4,
      cInObject = 5,
      cInObjectList = 6,
      cInIntervalTime = 7,
      cInDurationTime = 8,
      cInColor = 9,
      cInIntensity = 10,
   };

   // Collect objects
   BEntityIDArray workingObjects;
   if (getVar(cInObjectList)->isUsed())
   {
      workingObjects = getVar(cInObjectList)->asObjectList()->readVar();
   }

   if (getVar(cInObject)->isUsed())
   {
      workingObjects.uniqueAdd(getVar(cInObject)->asObject()->readVar());
   }

   // Collect units
   if (getVar(cInUnitList)->isUsed())
   {
      workingObjects.append(getVar(cInUnitList)->asUnitList()->readVar());
   }

   if (getVar(cInUnit)->isUsed())
   {
      workingObjects.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }

   // Collect squads
   BEntityIDArray squads;
   if (getVar(cInSquadList)->isUsed())
   {
      squads = getVar(cInSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInSquad)->isUsed())
   {
      squads.uniqueAdd(getVar(cInSquad)->asSquad()->readVar());
   }

   uint numStuff = squads.getSize();
   for (uint i = 0; i < numStuff; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squads[i]);
      if (pSquad)
      {
         workingObjects.append(pSquad->getChildList());
      }
   }

   // Set parameters
   DWORD intervalTime = getVar(cInIntervalTime)->asTime()->readVar();
   DWORD durationTime = getVar(cInDurationTime)->asTime()->readVar();
   BColor color = getVar(cInColor)->asColor()->readVar();
   float intensity = ((getVersion() > 1) && (getVar(cInIntensity)->isUsed())) ? getVar(cInIntensity)->asFloat()->readVar() : 20.0f;

   // Iterate list
   numStuff = workingObjects.getSize();
   for (uint i = 0; i < numStuff; i++)
   {
      BObject* pObject = gWorld->getObject(workingObjects[i]);
      if (pObject)
      {
         /*
         pObject->setFlagOverrideTint(true);
         pObject->setOverrideFlashInterval(intervalTime);
         pObject->setOverrideFlashDuration(durationTime);
         */
         // Tint color is used by the selection stuff
         pObject->setOverrideTintColor(color.asDWORD());

         float speed = -2.0f;
         if (intervalTime > 0)
            speed /= (intervalTime * 0.001f);
         float scale = 1.0f;
         float minY;
         if (pObject->getVisualBoundingBox())
         {
            BVector minCorner, maxCorner;
            pObject->getVisualBoundingBox()->computeWorldCorners(minCorner, maxCorner);
            float yExtent = maxCorner.y - minCorner.y;
            if (yExtent > cFloatCompareEpsilon || yExtent < -cFloatCompareEpsilon)
               scale = -1.0f / yExtent;
            minY = minCorner.y;
         }
         else
            minY = pObject->getPosition().y;
         float heightOffset = 2.0f - (minY * scale);

         pObject->setTargettingSelection(true, scale, heightOffset, heightOffset, durationTime * 0.001f, speed, true, true, intensity);
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teMissionResult()
{
   enum { cPlayerState = 1, };

   BUser* pUser = gUserManager.getPrimaryUser();
   if( pUser && pUser->getPlayerState() != BPlayer::cPlayerStatePlaying )
      return;

   BPlayerState result = getVar(cPlayerState)->asPlayerState()->readVar();

   gUIManager->setScenarioResult( result );
   gUIManager->showNonGameUI( BUIManager::cEndGameScreen, gUserManager.getPrimaryUser() );
}

//==============================================================================
// Reset the dopples
//==============================================================================
void BTriggerEffect::teResetDopple()
{
   enum
   {
      cInUnit = 1,
      cInUnitList = 2,
      cInSquad = 3,
      cInSquadList = 4,
      cInObject = 5,
      cInObjectList = 6,
      cInGrayMapDopples = 7,
      cInDopples = 8,
   };

   BEntityIDArray objects;
   if (getVar(cInObjectList)->isUsed())
   {
      objects = getVar(cInObjectList)->asObjectList()->readVar();
   }

   if (getVar(cInObject)->isUsed())
   {
      objects.uniqueAdd(getVar(cInObject)->asObject()->readVar());
   }

   if (getVar(cInUnitList)->isUsed())
   {
      objects.append(getVar(cInUnitList)->asUnitList()->readVar());
   }

   if (getVar(cInUnit)->isUsed())
   {
      objects.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }

   BEntityIDArray squads;
   if (getVar(cInSquadList)->isUsed())
   {
      squads = getVar(cInSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInSquad)->isUsed())
   {
      squads.uniqueAdd(getVar(cInSquad)->asSquad()->readVar());
   }

   uint count = squads.getSize();
   for (uint i = 0; i < count; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squads[i]);
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint j = 0; j < numChildren; j++)
         {
            objects.uniqueAdd(pSquad->getChild(j));
         }
      }
   }

   bool grayMapDopples = getVar(cInGrayMapDopples)->asBool()->readVar();
   bool dopples = getVar(cInDopples)->asBool()->readVar();
   int numTeams = gWorld->getNumberTeams();
   count = objects.getSize();
   for (uint i = 0; i < count; i++)
   {
      BObject* pObject = gWorld->getObject(objects[i]);
      if (pObject)
      {
         pObject->setFlagGrayMapDopples(grayMapDopples);
         pObject->setFlagDopples(dopples);
         BDopple* pDopple = pObject->getDopple();
         if (pDopple)
         {
            pDopple->kill(true);
         }

         for (int t = 1; t < numTeams; t++)
         {
            pObject->clearDoppleObject(t);
         }

         pObject->setFlagForceVisibilityUpdateNextFrame(true);
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teSaveGame()
{
   enum { cFileName = 1, };

   BSimString fileName;
   if (getVar(cFileName)->isUsed())
      fileName = getVar(cFileName)->asString()->readVar();

   gSaveGame.saveGame(fileName);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teLoadGame()
{
   enum { cFileName = 1, };

   BSimString fileName;
   if (getVar(cFileName)->isUsed())
      fileName = getVar(cFileName)->asString()->readVar();

   gSaveGame.loadGame(fileName);
}

//==============================================================================
// Issue a reverse hot drop command from a hot drop pad
//==============================================================================
void BTriggerEffect::teReverseHotDrop()
{
   enum
   {
      cInPadUnit = 1,
   };

   BEntityID padUnitID = getVar(cInPadUnit)->asUnit()->readVar();
   BUnit* pPadUnit = gWorld->getUnit(padUnitID);
   if (pPadUnit)
   {
      BPlayerID playerID = pPadUnit->getPlayerID();
      BPlayer* pPlayer = gWorld->getPlayer(playerID);
      if (pPlayer)
      {         
         BEntityHandle h = cInvalidObjectID;
         BUnit* pLeaderUnit = gWorld->getNextUnit(h);
         while (pLeaderUnit)
         {
            if (!pLeaderUnit->isAlive())
            {
               pLeaderUnit = gWorld->getNextUnit(h);
               continue;
            }

            if (pLeaderUnit->isType(gDatabase.getOTIDLeader()))
            {
               break;
            }

            pLeaderUnit = gWorld->getNextUnit(h);
         }

         if (pLeaderUnit)
         {
            BGameCommand* pCommand = (BGameCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGame);
            if (pCommand)
            {
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setSenders(1, &playerID);
               pCommand->setRecipientType(BCommand::cGame);
               pCommand->setType(BGameCommand::cTypeReverseHotDrop);
               pCommand->setData(padUnitID);
               pCommand->setData2(pLeaderUnit->getID());
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
            }
         }
      }
   }
}

//==============================================================================
// This is identical to teEventSubscribe, but also sets the useCount bool to
// true.  This could be an optional parameter, but we want to make sure
// nobody accidentally sets it.
//==============================================================================
void BTriggerEffect::teEventSubscribeUseCount()
{
   enum { cEventType = 1, cPlayer=3, cEventID=2 };

   bool isPlayerUsed = getVar(cPlayer)->isUsed();

   int subscriptionID;
   int eventType = getVar(cEventType)->asEventType()->readVar();

   BGeneralEventSubscriber* pSubscription = gGeneralEventManager.newSubscriber(eventType);
   subscriptionID = pSubscription->mID;

   BPlayerID playerID;
   if(isPlayerUsed)
   {
      playerID = getVar(cPlayer)->asPlayer()->readVar();
      pSubscription->mCheckPlayer = true;
      pSubscription->mPlayer = playerID;
   }

   // Set to use the fired count
   pSubscription->mUseCount = true;

   getVar(cEventID)->asInteger()->writeVar(subscriptionID);

}

//==============================================================================
// teObjectTypeToProtoObjects
//==============================================================================
void BTriggerEffect::teObjectTypeToProtoObjects()
{
   enum { cObjectType = 1, cProtoObjectList=2, cCount=3 };

   long type = getVar(cObjectType)->asObjectType()->readVar();

   static BProtoObjectIDArray ids;
   ids.resize(0);
   BProtoObjectIDArray *pTempIDs = NULL;
   uint count = gDatabase.decomposeObjectType(type, &pTempIDs);
   if (pTempIDs)
      ids.assignNoDealloc(*pTempIDs);

   if(getVar(cProtoObjectList)->isUsed())
   {
      getVar(cProtoObjectList)->asProtoObjectList()->writeVar(ids);
   }
   if(getVar(cCount)->isUsed())
   {
      getVar(cCount)->asInteger()->writeVar(count);
   }
}

//==============================================================================
// teShowInfoDialog
//==============================================================================
void BTriggerEffect::teShowInfoDialog()
{
   enum { cTitleTextID = 1, cBodyTextID = 2, cVOCue = 3, cImagePath = 4 };

   uint32 titleTextID = getVar(cTitleTextID)->asLocStringID()->readVar();
   uint32 bodyTextID = getVar(cBodyTextID)->asLocStringID()->readVar();

   BSimString voCue;
   BSimString imagePath;

   if(getVar(cVOCue)->isUsed())
   {
      voCue = getVar(cVOCue)->asString()->readVar();
   }

   if(getVar(cImagePath)->isUsed())
   {
      imagePath = getVar(cImagePath)->asString()->readVar();
   }

   BUIInfoDialog* pInfoDialog = (BUIInfoDialog*)gUIManager->getNonGameUIScreen( BUIManager::cCampaignInfoDialog );
   if( pInfoDialog )
   {
      pInfoDialog->setData( gDatabase.getLocStringFromID(titleTextID), gDatabase.getLocStringFromID(bodyTextID), voCue, imagePath );
      gUIManager->showNonGameUI( BUIManager::cCampaignInfoDialog, gUserManager.getPrimaryUser() );
   }
}

//==============================================================================
// Shutdown the user power
//==============================================================================
void BTriggerEffect::tePowerUserShutdown()
{
   enum
   {
      cInPlayer = 1,
      cInPower = 2,
   };

   BPlayerID playerID = getVar(cInPlayer)->asPlayer()->readVar();
   BProtoPowerID powerID = getVar(cInPower)->asPower()->readVar();
   BUser* pUser = gUserManager.getUserByPlayerID(playerID);
   if (pUser)
   {
      BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(powerID);
      BPowerUser* pPowerUser = pUser->getPowerUser();
      if (pProtoPower && pPowerUser && (pProtoPower->getPowerType() == pPowerUser->getType()))
      {
         // We've already invoked the power, so send across the shutdown command.
         BPowerInputCommand* pCommand = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandPowerInput);
         BASSERT(pCommand);
         if (pCommand)
         {            
            pCommand->setSenders(1, &playerID);
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setRecipientType(BCommand::cPlayer);
            pCommand->setType(BPowerInputCommand::cTypeShutdown);
            pCommand->setPowerUserID(pPowerUser->getID());
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
         }

         pPowerUser->shutdown();
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::tePowerToInt()
{
   enum { cPower = 1, cInt = 2, };
   long intval = getVar(cPower)->asPower()->readVar();
   getVar(cInt)->asInteger()->writeVar(intval);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teIntToPower()
{
   enum { cInt = 1, cPower = 2, };
   long countval = getVar(cInt)->asInteger()->readVar();
   getVar(cPower)->asPower()->writeVar(countval);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teCustomCommandExecute()
{
   enum { cUnit = 1, cSlot = 2, cPlayer = 3 };

   int slot = 1;
   bool checkSlot = getVar(cSlot)->isUsed();
   if(checkSlot)
      slot = getVar(cSlot)->asInteger()->readVar();

   BEntityID unit = getVar(cUnit)->asUnit()->readVar();
   long playerId = getVar(cPlayer)->asPlayer()->readVar();

   BSmallDynamicSimArray<BCustomCommand>& commands = gWorld->getCustomCommands();
   uint count=commands.getSize();
   for (uint i=0; i<count; i++)
   {
      BCustomCommand& item=commands[i];
      if (item.mUnitID == unit)
      {
         //check slot?
         if(checkSlot && item.mPosition != slot)
            continue;

         //Can we execute the command? This may not be robust in all situations.  That is why this effect is dev only.
         if(gWorld->getPlayer(playerId)->checkCost(&(item.mCost)))
         {
            long type=BProtoObjectCommand::cTypeCustomCommand;
            long id=item.mID;

            BBuildingCommand* pCommand=(BBuildingCommand*)gWorld->getCommandManager()->createCommand(playerId, cCommandBuilding);
            if(pCommand)
            {
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setSenders(1, &playerId);
               pCommand->setRecipientType(BCommand::cUnit);
               pCommand->setRecipients(1, &unit);
               pCommand->setType(type);
               pCommand->setTargetID(id);
               pCommand->setCount(1);
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
            }
         }
         return;
      }
   }
}

//==============================================================================
// Set the player to ignore Dpad user inputs
//==============================================================================
void BTriggerEffect::teIgnoreDpad()
{
   enum 
   {
      cInPlayer = 1,
      cInPlayers = 2,
      cInIgnore = 3,
   };

   BPlayerIDArray playerIDs;
   if (getVar(cInPlayers)->isUsed())
   {
      playerIDs = getVar(cInPlayers)->asPlayerList()->readVar();
   }

   if (getVar(cInPlayer)->isUsed())
   {
      playerIDs.uniqueAdd(getVar(cInPlayer)->asPlayer()->readVar());
   }

   bool ignoreDpad = getVar(cInIgnore)->asBool()->readVar();

   uint numPlayers = playerIDs.getSize();
   for (uint i = 0; i < numPlayers; i++)
   {
      BUser* pUser = gUserManager.getUserByPlayerID(playerIDs[i]);
      if (pUser)
      {
         pUser->setFlagIgnoreDpad(ignoreDpad);
      }
   }
}

//==============================================================================
// Turn letter box on and off
//==============================================================================
void BTriggerEffect::teEnableLetterBox()
{
   enum
   {
      cInEnable = 1,
   };

   bool enable = getVar(cInEnable)->asBool()->readVar();
  
   if (enable)
   {
      gWorld->getCinematicManager()->fadeInLetterBox();
   }
   else
   {
      gWorld->getCinematicManager()->fadeOutLetterBox();
   }
}

//==============================================================================
// Turn a screen blur effect on and off
//==============================================================================
void BTriggerEffect::teEnableScreenBlur()
{
   enum
   {
      cInEnable = 1,
   };

   BUser* pPrimaryUser = gUserManager.getPrimaryUser();

   bool enable = getVar(cInEnable)->asBool()->readVar();
   if (enable)
   {
      pPrimaryUser->applyScreenBlur(true);
   }
   else
   {
      pPrimaryUser->applyScreenBlur(false);
   }
}

//==============================================================================
// Set the blocked flag for a given player / power
//==============================================================================
void BTriggerEffect::teSetPowerAvailableTime()
{
   enum { cInputPlayer = 1, cInputPower = 2, cInputTime = 3 };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long protoPowerID = getVar(cInputPower)->asPower()->readVar();
   DWORD intervalTime = getVar(cInputTime)->asTime()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPlayer->setPowerAvailableTime(protoPowerID, intervalTime);
}

//==============================================================================
// Set the blocked flag for a given player / power
//==============================================================================
void BTriggerEffect::tePowerMenuEnable()
{
   enum { cEnabled = 1, cPlayer = 2, cPlayerList = 3 };

   BPlayerIDArray players;

   bool enable = getVar(cEnabled)->asBool()->readVar();

   if (getVar(cPlayer)->isUsed())
      players.uniqueAdd(getVar(cPlayer)->asPlayer()->readVar());

   if (getVar(cPlayerList)->isUsed())
   {
      const BPlayerIDArray& playerList = getVar(cPlayerList)->asPlayerList()->readVar();
      uint playerListSize = (uint)playerList.getNumber();
      for (uint idx = 0; idx < playerListSize; idx++)
         players.uniqueAdd(playerList[idx]);
   }

   uint count = players.getNumber();
   for (uint idx = 0; idx < count; idx++)
   {
      BUser* pUser = gUserManager.getUserByPlayerID(players[idx]);
      if (pUser == NULL)
         continue;

      pUser->setFlagPowerMenuEnable(enable);
   }
}

//==============================================================================
// GetPopularSquadType
//==============================================================================
void BTriggerEffect::teGetPopularSquadType()
{
   enum { cSquadList = 1, cProtoSquad = 2, cCount = 3 };

   const BEntityIDArray& squads = getVar(cSquadList)->asSquadList()->readVar();

   BHashMap<long, long> protoSquadCounts;

   long maxCount = 0;
   long topProtoSquad = cInvalidProtoSquadID;
   for(uint i=0; i<squads.size(); i++)
   {
      BSquad* pSquad = gWorld->getSquad(squads[i]);
      long squad = pSquad->getProtoSquadID();

      BHashMap<long, long>::iterator it(protoSquadCounts.find(squad));
      long count = 0;
      if (it == protoSquadCounts.end())
      {  
         protoSquadCounts.insert(squad, 1);
         count = 1;
      }
      else
      {
         count = (long)it->second;
         count++;
         it->second = count;
      }
      if(count >maxCount)
      {
         maxCount = count;
         topProtoSquad = squad;
      }
   }

   getVar(cProtoSquad)->asProtoSquad()->writeVar(topProtoSquad);
   getVar(cCount)->asInteger()->writeVar(maxCount);
}

//==============================================================================
// TeleportUnitsOffObstruction
//==============================================================================
void BTriggerEffect::teTeleportUnitsOffObstruction()
{
   enum { cUnit = 1 };

   // This function based on BWorld::moveSquadsFromObstruction

   // Get unit and squad that define the obstruction
   BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
   BUnit* pObsUnit = gWorld->getUnit(unitID);
   if (!pObsUnit)
      return;
   BSquad* pObsSquad = pObsUnit->getParentSquad();
   if (!pObsSquad)
      return;

   // Get obstruction
   const BOPObstructionNode* pObsNode = pObsUnit->getObstructionNode();
   if (!pObsNode || pObsNode->mType == BObstructionManager::cObsTypeNonCollidableUnit)
      return;
   BOPQuadHull* pObsHull = (BOPQuadHull*) pObsNode->getHull();
   if (!pObsHull)
      return;

   // Look for other squads overlapping the obstruction squad
   BEntityIDArray results(0, 20);
   BUnitQuery query1(pObsHull);
   query1.setFlagIgnoreDead(true);
   int numTargets = gWorld->getSquadsInArea(&query1, &results);
   if (numTargets == 0)
      return;

   BEntityIDArray moveSquads(0, numTargets);
   BSmallDynamicSimArray<BPlayerID> movePlayers(0, 1);

   BVector aveSquadDir;
   aveSquadDir.zero();

   for (int i = 0; i < numTargets; i++)
   {
      BEntityID squadID = results[i];

      // Skip over the obstruction's squad
      if (squadID == pObsSquad->getID())
         continue;      

      BSquad* pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (!pProtoObject)
            continue;

         // Ignore permanently non-mobile units such as a socket 
         if (pProtoObject->getFlagNonMobile())
            continue;

         // squads that are in a modal power won't be able to be told to move
         if (pSquad->getSquadMode() == BSquadAI::cModePower)
         {
            continue;
            //return true;
         }

         // Hitched units or tow trucks won't move off obstructions
         if (pSquad->getFlagHitched() || (const_cast<BSquad*>(pSquad))->hasHitchedSquad())
         {
            continue;
            //return true;
         }

         // TRB 8/28/08 - Because the base obstruction can rotate but moving squad's obstructions don't, the above
         // obstruction check can return squads that have a corner of an obstruction overlapping, which is ok according
         // to the movement system.  Do the same obstruction to radius check the movement system uses to confirm the squad
         // really needs to move.
         if (pObsSquad->calculateXZDistance(pSquad) > cFloatCompareEpsilon)
            continue;


         /////////////////////////////////////
         // Move the squad

         static BDynamicSimVectorArray instantiatePositions;
         instantiatePositions.setNumber(0);
         long closestDesiredPositionIndex;

         if (BSimHelper::findInstantiatePositions(1, instantiatePositions, pObsSquad->getObstructionRadius(), pObsSquad->getPosition(),
               pObsSquad->getForward(), pObsSquad->getRight(), pSquad->getObstructionRadius(), pSquad->getPosition(), closestDesiredPositionIndex, 4))
         {
            BVector newPosition;
            if ((closestDesiredPositionIndex >= 0) && (closestDesiredPositionIndex < instantiatePositions.getNumber()))
               newPosition = instantiatePositions[closestDesiredPositionIndex];
            else if (instantiatePositions.getNumber() > 0)
               newPosition = instantiatePositions[0];
            else
            {
               // Failed to find valid position
               continue;
            }

            // Remove the squad from it's parent platoon
            BPlatoon* pParentPlatoon = pSquad->getParentPlatoon();
            if (pParentPlatoon)
               pParentPlatoon->removeChild(pSquad);

            // Preserve height for air units.  Maybe not exactly the correct new height but a good approximation.
            const BProtoObject *pProtoObject = pSquad->getProtoObject();
            if (pProtoObject && pProtoObject->getMovementType() == cMovementTypeAir)
               newPosition.y = pSquad->getPosition().y;

            // Set new squad and unit position
            pSquad->setPosition(newPosition);
            pSquad->setLeashPosition(newPosition);
            pSquad->setTurnRadiusPos(newPosition);
            pSquad->settle();
            pSquad->updateObstruction();
            bool squadIsAPhysicsVehicle = pSquad->isSquadAPhysicsVehicle();
            for (uint unitIndex = 0; unitIndex < pSquad->getNumberChildren(); unitIndex++)
            {
               BUnit* pUnit = gWorld->getUnit(pSquad->getChild(unitIndex));
               if (pUnit)
               {
                  if (squadIsAPhysicsVehicle && pUnit->getFlagPhysicsControl())
                  {
                     pUnit->setPosition(pSquad->getPosition());
                  }
                  pUnit->updateObstruction();
               }
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teLockPlayerUser()
{
   enum { cPlayerID = 1, cLock = 2, };

   BUser* pUser = gUserManager.getUserByPlayerID(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pUser)
      return;

   BTriggerScriptID triggerScriptID = mpParentTriggerScript->getID();
   bool bLock = getVar(cLock)->asBool()->readVar();
   if (bLock)
   {
      if (!pUser->isUserLocked())
         pUser->lockUser(triggerScriptID);
   }
   else
   {
      if (pUser->isUserLockedByTriggerScript(triggerScriptID))
         pUser->unlockUser();
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetGameMode()
{
   enum { cIsSkirmish = 1, cIsDeathmatch = 2, };
   
   bool bIsSkirmish = false;
   bool bIsDeathmatch = false;
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (pSettings)
   {
      long gameMode = -1;
      if (pSettings->getLong(BGameSettings::cGameMode, gameMode))
      {
         const BGameMode* pGameMode = gDatabase.getGameModeByID(gameMode);
         if (pGameMode && pGameMode->getName() == "Skirmish")
            bIsSkirmish = true;
         if (pGameMode && pGameMode->getName() == "Deathmatch")
            bIsDeathmatch = true;
      }
   }

   if (getVar(cIsSkirmish)->asBool()->isUsed())
      getVar(cIsSkirmish)->asBool()->writeVar(bIsSkirmish);

   if (getVar(cIsDeathmatch)->asBool()->isUsed())
      getVar(cIsDeathmatch)->asBool()->writeVar(bIsDeathmatch);
}



// NEWTRIGGEREFFECT
// Add your trigger effect function here, following the same conventions.
// Any questions, ask Marc.


//==============================================================================
// BTriggerEffect::calculateValuesFloat(float val1, long op, float val2)
//==============================================================================
float BTriggerEffect::calculateValuesFloat(float val1, long op, float val2)
{
   switch (op)
   {
      case Math::cOpTypeAdd:
         return (val1 + val2);
      case Math::cOpTypeSubtract:
         return (val1 - val2);
      case Math::cOpTypeMultiply:
         return (val1 * val2);
      case Math::cOpTypeDivide:
         if (val2 != 0.0f)
         {
            return (val1 / val2);
         }
         else
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is trying to divide by 0!");
            return (val1);
         }
      case Math::cOpTypeModulus:
         BTRIGGER_ASSERTM(false, "A trigger effect is trying to modulate a float!");
         return (0.0f);
      default:
         BFATAL_ASSERT(false);
         return (false);
   }
}

//==============================================================================
// BTriggerEffect::calculateValuesLong(long val1, long op, long val2)
//==============================================================================
long BTriggerEffect::calculateValuesLong(long val1, long op, long val2)
{
   switch (op)
   {
      case Math::cOpTypeAdd:
         return (val1 + val2);
      case Math::cOpTypeSubtract:
         return (val1 - val2);
      case Math::cOpTypeMultiply:
         return (val1 * val2);
      case Math::cOpTypeDivide:
         if (val2 != 0)
         {
            return (val1 / val2);
         }
         else
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is tyring to divide by 0!");
            return (val1);
         }
      case Math::cOpTypeModulus:
         if (val2 != 0)
         {
            return (val1 % val2);
         }
         else
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is tyring to modulate by 0!");
            return (val1);
         }
      default:
         BFATAL_ASSERT(false);
         return (false);
   }
}

//==============================================================================
// BTriggerEffect::calculateValuesDWORD(float val1, long op, float val2)
//==============================================================================
DWORD BTriggerEffect::calculateValuesDWORD(DWORD val1, long op, DWORD val2)
{
   switch (op)
   {
      case Math::cOpTypeAdd:
         return (val1 + val2);
      case Math::cOpTypeSubtract:
         return (val1 - val2);
      case Math::cOpTypeMultiply:
         return (val1 * val2);
      case Math::cOpTypeDivide:
         if (val2 != 0)
         {
            return (val1 / val2);
         }
         else
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is tyring to divide by 0!");
            return (val1);
         }
      case Math::cOpTypeModulus:
         if (val2 != 0)
         {
            return (val1 % val2);
         }
         else
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is tyring to modulate by 0!");
            return (val1);
         }
      default:
         BFATAL_ASSERT(false);
         return (false);
   }
}

//==============================================================================
// BTriggerEffect::calculateValuesCost()
//==============================================================================
BCost BTriggerEffect::calculateValuesCost(BCost &cost1, long op, BCost &cost2)
{
   BCost resultCost;
   switch (op)
   {
      case Math::cOpTypeAdd:
         {
            resultCost = cost1;
            resultCost.add(&cost2);
            return (resultCost);
         }
      case Math::cOpTypeSubtract:
         {
            resultCost = cost1;
            resultCost.subtract(&cost2);
            return (resultCost);
         }
      case Math::cOpTypeMultiply:
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is trying to multiply a resource!");
            return (resultCost);
         }
      case Math::cOpTypeDivide:
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is trying to divide a resource!");
            return (resultCost);
         }
      case Math::cOpTypeModulus:
         {
            BTRIGGER_ASSERTM(false, "A trigger effect is trying to modulate a resource!");
            return (resultCost);
         }
      default:
         {
            BTRIGGER_ASSERTM(false, "An invalid math operator was used.");
            return (resultCost);
         }
   }
}

//==============================================================================
// BTriggerEffect::calculateValuesVector()
//==============================================================================
BVector BTriggerEffect::calculateValuesVector(BVector val1, long op, BVector val2)
{
   BVector resultVector;
   switch (op)
   {
      case Math::cOpTypeAdd:
         {
            resultVector = val1 + val2;
            return (resultVector);
         }
      case Math::cOpTypeSubtract:
         {
            resultVector = val1 - val2;
            return (resultVector);
         }
      case Math::cOpTypeMultiply:
      case Math::cOpTypeDivide:
      case Math::cOpTypeModulus:
      default:
         {
            BTRIGGER_ASSERTM(false, "An invalid math operator was used.");
            return (resultVector);
         }
   }
}

//=============================================================================
// BTriggerEffect::refCountAddHelper
//=============================================================================
void BTriggerEffect::refCountAddHelper(BEntityID entityID, short refCountType, short& maxCount)
{
   BEntity* pEntity = gWorld->getEntity(entityID);
   if (!pEntity)
      return;

   BEntityRef* pEntityRef = pEntity->getFirstEntityRefByType(refCountType);

   // If we already have this entity ref, increment the refcount on it (stored in mData1)
   if (pEntityRef)
   {
      pEntityRef->mData1 += 1;
      if (pEntityRef->mData1 > maxCount)
         maxCount = pEntityRef->mData1;
   }
   // Otherwise, add a new entity ref with refcount of 1 (stored in mData1)
   else
   {      
      pEntity->addEntityRef(refCountType, cInvalidObjectID, 1, 0);
      if (maxCount < 1)
         maxCount = 1;
   }
}

//=============================================================================
// Appropriately remove the ref count of the designated type
//=============================================================================
void BTriggerEffect::refCountRemoveHelper(BEntityID entityID, short refCountType, short& maxCount)
{
   BEntity* pEntity = gWorld->getEntity(entityID);
   if (!pEntity)
      return;
   if (pEntity->getFlagDestroy())
      return;
   const BUnit* pUnit = pEntity->getUnit();
   if (pUnit && !pUnit->getFlagAlive())
      return;

   BEntityRef* pEntityRef = pEntity->getFirstEntityRefByType(refCountType);
   if (!pEntityRef)
      return;

   // We are removing our last entity ref so actually remove it.
   if (pEntityRef->mData1 <= 1)
   {
      pEntity->removeEntityRef(pEntityRef->mType, pEntityRef->mID);
   }
   // Just decrement the ref count since there's > 1 of them.
   else
   {
      pEntityRef->mData1 -= 1;
      if (pEntityRef->mData1 > maxCount)
         maxCount = pEntityRef->mData1;
      BTRIGGER_ASSERT(pEntityRef->mData1 >= 0);
   }
}

//=============================================================================
void BTriggerEffect::tePowerUsed()
{
   enum { cPower = 1, cPlayer = 2 };

   long powerID = getVar(cPower)->asPower()->readVar();
   long playerID = getVar(cPlayer)->asPlayer()->readVar();

   gGeneralEventManager.eventTrigger(BEventDefinitions::cUsedPower, playerID, cInvalidObjectID, powerID);
}

//==============================================================================
void BTriggerEffect::teChangeControlledPlayer()
{
   enum
   {
      cFromPlayer = 1,
      cToPlayer = 2,
   };

   // [8/22/2008 xemu] cribbed largely from user.cpp
   if ((gLiveSystem != NULL) && gLiveSystem->isMultiplayerGameActive())
   {
      // [8/22/2008 xemu] this trigger should not be used in multiplayer games!
      BASSERT(0);
      return;
   }

   long fromPlayerID = getVar(cFromPlayer)->asPlayer()->readVar();
   long toPlayerID = getVar(cToPlayer)->asPlayer()->readVar();

   // [8/22/2008 xemu] get the user associated with the "from" player 
   BUser* pUser = gUserManager.getUserByPlayerID(fromPlayerID);
   if (pUser == NULL)
   {
      BASSERT(0);
      return;
   }

   // Switch player
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(fromPlayerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &fromPlayerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeSwitchPlayer);
      pCommand->setData(toPlayerID);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
   pUser->switchPlayer(toPlayerID);
}

//==============================================================================
void BTriggerEffect::teGrantAchievement()
{
   enum
   {
      cAchievementName = 1,
   };

   BString name = "";
   if (getVar(cAchievementName)->isUsed())
      name = getVar(cAchievementName)->asString()->readVar();

   // [8/22/2008 xemu] get the user associated with the "from" player 
   BUser *pUser = gUserManager.getPrimaryUser();
   if (pUser)
      gAchievementManager.reportTriggerAchievement(pUser, name);

   pUser = gUserManager.getSecondaryUser();
   if (pUser)
      gAchievementManager.reportTriggerAchievement(pUser, name);

}

//==============================================================================
void BTriggerEffect::teResetAbilityTimer()
{
   enum 
   { 
      cPlayerIDList = 1,
      cObjectTypeList = 2, 
   };

   if (!getVar(cPlayerIDList)->isUsed() || !getVar(cObjectTypeList)->isUsed())
   {
      BTRIGGER_ASSERTM(false, "Bad data, eek!");
      return;
   }

   const BPlayerIDArray &players = getVar(cPlayerIDList)->asPlayerList()->readVar();
   const BObjectTypeIDArray &objectTypeIDList = getVar(cObjectTypeList)->asObjectTypeList()->readVar();
   BUnitQuery query;    // default to query type none because we don't care about space

   uint count = players.getNumber();
   for (uint idx=0; idx<count; idx++)
      query.addPlayerFilter(players[idx]);

   count = objectTypeIDList.getNumber();
   for (uint idx=0; idx<count; idx++)
      query.addObjectTypeFilter(objectTypeIDList[idx]);

   BEntityIDArray squadList(0,50);
   gWorld->getSquadsInArea(&query, &squadList);

   // Reset recover time for squads
   count = squadList.getNumber();
   for (uint idx = 0; idx < count; idx++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[idx]);
      if (pSquad)
         pSquad->setRecover(-1, 0.0f, -1);
   }
}

//==============================================================================
void BTriggerEffect::teSetMinimapNorthPointerRotation()
{
   enum
   {
      cRotationDegrees = 1,
   };

   float degrees = getVar(cRotationDegrees)->asFloat()->readVar();

   if (gUIManager)
      gUIManager->setMinimapRotationOffset(degrees);
}

//==============================================================================
void BTriggerEffect::teSetMinimapSkirtMirroring()
{
   enum
   {
      cMirrorSkirt = 1,
   };

   bool value = getVar(cMirrorSkirt)->asBool()->readVar();

   if (gUIManager)
      gUIManager->setMinimapSkirtMirroring(value);
}

//==============================================================================
void BTriggerEffect::teHideCircleMenu()
{
   BUser *pUser = gUserManager.getPrimaryUser();
   if (pUser)
      pUser->resetUserMode();

   pUser = gUserManager.getSecondaryUser();
   if (pUser)
      pUser->resetUserMode();
}

//==============================================================================
void BTriggerEffect::teChatForceSubtitles()
{
   enum
   {
      cEnabled = 1,
   };

   bool enable = getVar(cEnabled)->asBool()->readVar();

   if (gWorld->getChatManager())
      gWorld->getChatManager()->setForceSubtitlesOn(enable);
}

//==============================================================================
//==============================================================================
bool BTriggerEffect::save(BStream* pStream, int saveType) const
{
   uint8 varCount = (uint8)mEffectVars.size();
   GFVERIFYCOUNT(varCount, 200);
   GFWRITEVAR(pStream, uint8, varCount);
   for (uint8 i=0; i<varCount; i++)
   {
//-- FIXING PREFIX BUG ID 5584
      const BTriggerVar* pVar = mEffectVars[i];
//--
      BTriggerVarID varID = (pVar ? pVar->getID() : UINT16_MAX);
      GFWRITEVAR(pStream, BTriggerVarID, varID);
   }

   GFWRITEVAR(pStream, long, mDBID);
   GFWRITEVAR(pStream, int8, mVersion);

   return true;
}

//==============================================================================
//==============================================================================
bool BTriggerEffect::load(BStream* pStream, int saveType)
{
   mEffectVars.clear();
   uint8 varCount;
   GFREADVAR(pStream, uint8, varCount);
   GFVERIFYCOUNT(varCount, 200);

   if (mGameFileVersion < 2 && varCount > 0)
      mEffectVars.reserve(varCount-1);
   else
      mEffectVars.reserve(varCount);

   for (uint8 i=0; i<varCount; i++)
   {
      BTriggerVarID varID;
      GFREADVAR(pStream, BTriggerVarID, varID);
      
      if (mGameFileVersion == 1 && i==0)   // the first value in version 1 was always NULL
         continue;

      BTriggerVar* pVar = (varID != UINT16_MAX)?mpParentTriggerScript->getTriggerVar(varID):NULL;
      mEffectVars.add(pVar);
   }

   GFREADVAR(pStream, long, mDBID);
   GFREADVAR(pStream, int8, mVersion);

   return true;
}
