//==============================================================================
// squadai.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "squadai.h"
#include "unit.h"
#include "tactic.h"
#include "squad.h"
#include "world.h"
#include "protosquad.h"
#include "team.h"
#include "action.h"
#include "entityscheduler.h"
#include "pather.h"
#include "syncmacros.h"
#include "unitquery.h"
#include "commands.h"
#include "squadplotter.h"
#include "squadactionattack.h"
#include "squadactionwork.h"
#include "UnitOpportunity.h"
#include "visual.h"
#include "protovisual.h"
#include "ability.h"

//#define DEBUGOPPS


BOpportunityList BSquadAI::mOpportunityList;


GFIMPLEMENTVERSION(BSquadAI, 4);
enum 
{
   cSaveMarkerSquadAI1=10000,
};


//==============================================================================
//==============================================================================
BSquadAI::BSquadAI()
{
}

//==============================================================================
//==============================================================================
BSquadAI::~BSquadAI()
{
}

//==============================================================================
//==============================================================================
void BSquadAI::init(BSquad* pSquad)
{
   mpOwner=pSquad;
   mMode=cModeNormal;

   mWorkOpp.init();

   //-- Flags
   mFlagCanWork=true;
   mFlagSearchToken=false;
   mFlagWorkToken=false;
}

//==============================================================================
//==============================================================================
void BSquadAI::update()
{
   BASSERT(mpOwner);
   BASSERT(mpOwner->getPlayer());
   
   //If we have a search token, use it.
   if (mFlagSearchToken)
      mpOwner->refreshVisibleSquads();
   
   //If we have a work token and we can work, use it.
   if (mFlagWorkToken && mFlagCanWork)
   {
      removeExpiredIgnores();
      searchForWork();
   }

   mFlagSearchToken=false;
   mFlagWorkToken=false;
}

//==============================================================================
//==============================================================================
void BSquadAI::searchForWork(void)
{  
   SCOPEDSAMPLE(BSquadAISearchForWork);
   syncSquadData("BSquadAI::searchForWork mWorkOpp ID", (int)mWorkOpp.getID());
   syncSquadData("BSquadAI::searchForWork mWorkOpp Type", (int)mWorkOpp.getType());
   // SLB: Don't search for work once the game is over
   if (gWorld->getFlagGameOver())
      return;

   //Get our current attack target (if any).
   bool isStrafing=false;
   BEntityID currentAttackTarget=cInvalidObjectID;
   for (int actionIndex = 0; actionIndex < mpOwner->getNumberActions(); actionIndex++)
   {
      BAction* pAction = mpOwner->getActionByIndex(actionIndex);
      if (pAction && (pAction->getType() == BAction::cActionTypeSquadAttack))
      {
         const BSquadActionAttack* pAttackAction = reinterpret_cast<BSquadActionAttack*>(pAction);
         currentAttackTarget=pAttackAction->getTarget()->getID();
         isStrafing=pAttackAction->isStrafing();

         // Stop searching if this is a non-paused attack action
         if (pAttackAction->getState() != BAction::cStatePaused)
            break;
      }
   }

   //Pull our unit opp.
   BUnitOpp newOpp;
   newOpp.init();

   //-- Set the list's behavior. Right now this just plugs into default squad behavior, but it can call different methods based on squadAI state later
   mOpportunityList.setBehavior(reinterpret_cast<BEntity*>(mpOwner));

   //-- Update our opportunity list
   mOpportunityList.findOpportunities(mpOwner, isStrafing);
   syncSquadData("BSquadAI::searchForWork mOpportunityList.getNumOpportunities()", (int)mOpportunityList.getNumOpportunities());
   if (mOpportunityList.getNumOpportunities() == 0)
   {
      if (mWorkOpp.getID() != BUnitOpp::cInvalidID)
      {
         mpOwner->removeOppFromChildren(mWorkOpp.getID(), true);
         mWorkOpp.init();
      }
      return;
   }

   //-- Sort our opportunity list
   mOpportunityList.calculatePriorities(mpOwner);

   //-- Do the top thing in our list
   BOpportunity newOpportunity;
   bool newOpportunityExists = mOpportunityList.getTopOpportunity(newOpportunity);
   bool didSquadAttack=false;
   int maxNumUnitsPerformAction = -1;
   if (newOpportunityExists && (newOpportunity.mTargetID != cInvalidObjectID))
   {
      
      syncSquadData("BSquadAI::searchForWork newOpportunity.mTargetID", newOpportunity.mTargetID.asLong());
      syncSquadData("BSquadAI::searchForWork newOpportunity.mType", (int)newOpportunity.mType);
      
      short type = newOpportunity.mType;
      switch(type)
      {         
         //Straight-up attack kills current orders and just queues an order to the squad.
         case BOpportunity::eOpportunityTypeAttack:
         {
            //Check to see if we can even do or want to do an attack first.
            if (currentAttackTarget != newOpportunity.mTargetID)
            {
               if (doesTargetObeyLeash(newOpportunity.mTargetID))
               {
                  mpOwner->removeOrders(true, true, true, false, true);
                  mpOwner->queueAttack(BSimTarget(newOpportunity.mTargetID));
                  didSquadAttack=true;
               }
               else
                  ignoreSquad(newOpportunity.mTargetID, cIgnoreSquadTime);
            }
            break;
         }

         //Attack move queues a prepend order to the squad.
         case BOpportunity::eOpportunityTypeAttackMove:
         {
            if (doesTargetObeyLeash(newOpportunity.mTargetID))
            {
               if (newOpportunity.mTargetID != currentAttackTarget)
               {
                  mpOwner->removeAutoGeneratedAttackMoveOrders();
                  mpOwner->queueAttack(BSimTarget(newOpportunity.mTargetID), true, true, true);
               }
               didSquadAttack=true;
            }
            else
               ignoreSquad(newOpportunity.mTargetID, cIgnoreSquadTime);
            break;
         }

         //Secondary Attack just gives an opp to the units.
         case BOpportunity::eOpportunityTypeAttackSecondary:
         {
            if (newOpportunity.mTargetID != currentAttackTarget)
            {
               BSimTarget target(newOpportunity.mTargetID);
               float range = 0.0f;
               BProtoAction *pProtoAction = NULL;
               if (mpOwner->calculateRange(&target, range, &pProtoAction, NULL))
                  target.setRange(range);
               if (pProtoAction)
                  maxNumUnitsPerformAction = pProtoAction->getMaxNumUnitsPerformAction();
               newOpp.setType(BUnitOpp::cTypeSecondaryAttack);
               newOpp.setTarget(target);
               newOpp.setSource(mpOwner->getID());
               newOpp.setPriority(BUnitOpp::cPrioritySquad);
               newOpp.setAllowComplete(false);
                                                                           
               syncSquadData("BSquadAI::searchForWork newOpp.mTarget", (int)newOpp.getTarget().getID().asLong());
            }
            break;
         }         
             
         case BOpportunity::eOpportunityTypeMove:
         {            
            mpOwner->queueMove(BSimTarget(newOpportunity.mTargetPosition));
            break;
         }

         case BOpportunity::eOpportunityTypeGather:
         {
            //If we have a gather, see if we're already gathering the same thing.
            BEntityID currentWorkTarget=cInvalidObjectID;
            const BSquadActionWork* pWorkAction=reinterpret_cast<BSquadActionWork*>(mpOwner->getActionByType(BAction::cActionTypeSquadWork));
            if (pWorkAction)
               currentWorkTarget=pWorkAction->getTarget()->getID();
            //If we're not, queue the new thing.
            if (currentWorkTarget != newOpportunity.mTargetID)
            {
               BSimTarget target(newOpportunity.mTargetID);
               mpOwner->queueWork(BSimTarget(target));
            }
            break;
         }

         case BOpportunity::eOpportunityTypeAutoRepair:
         {
            BSimTarget target(newOpportunity.mTargetID);
            mpOwner->queueWork(BSimTarget(target));
            break;
         }
      }
   }

   //If we have a new unit opp that's different than our old one, remove the
   //old one and add the new one.
   if (newOpp.getTarget() != mWorkOpp.getTarget())
   {      
      //If we're canceling an old secondary opp but we queued an attack for
      //the same target, don't nuke the actions.
      bool removeActions=true;
      if (didSquadAttack &&
         (newOpp.getType() == BUnitOpp::cTypeNone) &&
         (mWorkOpp.getTarget().getID() == newOpportunity.mTargetID))
         removeActions=false;
   
      #ifdef DEBUGOPPS
      if (mpOwner->getPlayerID() == 1)
      {
         mpOwner->debug("Changing Opps: RemoveActions=%d.", removeActions);
         mpOwner->debug("  OldOppID=%d, Target=%d.", mWorkOpp.getID(), mWorkOpp.getTarget().getID());
         mpOwner->debug("  NewOppID=%d, Target=%d.", newOpp.getID(), newOpp.getTarget().getID());
      }
      #endif

      //Nuke the old one.
      if (mWorkOpp.getID() != BUnitOpp::cInvalidID)
      {
         mpOwner->removeOppFromChildren(mWorkOpp.getID(), removeActions);
         mWorkOpp.init();
      }
      //Add the new one if it's valid.
      if (newOpp.getType() != BUnitOpp::cTypeNone)
      {
         newOpp.generateID();

         // Only add opp to the number of children that can perform the action.  This should only affect
         // auto attacking flood swarm.  All the other limited attacks are ability attacks which don't
         // go through this auto attack code.
         bool addOppSuccessful = true;
         if (maxNumUnitsPerformAction == -1)
            addOppSuccessful = mpOwner->addOppToChildren(newOpp);
         else
            addOppSuccessful = mpOwner->addOppToMaxNumChildren(newOpp, maxNumUnitsPerformAction);

         if (addOppSuccessful)
         {
            mWorkOpp=newOpp;
            #ifdef DEBUGOPPS
            mpOwner->debug("  Saved OppID.");
            #endif
         }
         #ifdef DEBUGOPPS
         else if (mpOwner->getPlayerID() == 1)
            mpOwner->debug("  Did not save OppID.");
         #endif
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadAI::setMode(int mode)
{
   //UGH.  Too many compile warnings to fix at once.
   uint8 fooMode=static_cast<uint8>(mode);
   setMode(fooMode);
}

//==============================================================================
//==============================================================================
void BSquadAI::setMode(uint8 mode)
{
   if (mMode == mode)
      return;

   if (mMode == cModeLockdown)
   {
      mpOwner->setFlagLockedDown(false);

      BUnit* pLeaderUnit = mpOwner->getLeaderUnit();
      if (pLeaderUnit && pLeaderUnit->getProtoObject()->getFlagLockdownMenu())
      {
         if (pLeaderUnit->getProtoObject()->getGotoType() != cGotoTypeBase)
         {
            pLeaderUnit->clearRallyPoint(mpOwner->getPlayerID());
            mpOwner->getPlayer()->removeGotoBase(pLeaderUnit->getID());
         }
         // If all existing bases have a rally point set, then clear the global rally point.
         mpOwner->getPlayer()->checkRallyPoint();
      }
   }
   else if (mMode == cModeSniper)
      mpOwner->setFlagInSniper(false);
   else if (mMode == cModeCarryingObject)
   {
      // if we are transitioning away from the carrying object state, 
      // all units must dispose of objects they are carrying
      for (uint i=0; i<mpOwner->getNumberChildren(); i++)
      {
         BUnit* pUnit=gWorld->getUnit(mpOwner->getChild(i));
         if (pUnit)
            pUnit->destroyCarriedObject();
      }
   }

   mMode=mode;

   if (mode == cModeLockdown)
   {
      mpOwner->setFlagLockedDown(true);

      BUnit* pLeaderUnit = mpOwner->getLeaderUnit();
      if (pLeaderUnit && pLeaderUnit->getProtoObject()->getFlagLockdownMenu() && pLeaderUnit->getProtoObject()->getGotoType() != cGotoTypeBase)
      {
         mpOwner->getPlayer()->addGotoBase(pLeaderUnit->getID());
         
         // [8/27/2008 JRuediger] Checking against gotovehicle because I want to catch instances where the Elephant is locking down in order to set its rally point.
         if(pLeaderUnit->getProtoObject()->getGotoType() == cGotoTypeVehicle)
         {
            pLeaderUnit->calculateDefaultRallyPoint();            
         }

      }
         
   }
   else if (mode == cModeSniper)
      mpOwner->setFlagInSniper(true);

   if (mMode == cModeNormal)
   {
      float speedModifier = mpOwner->getModeMovementModifier();
      if (speedModifier != 0.0f)
         mpOwner->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, speedModifier, true);
   }

   for (uint i=0; i<mpOwner->getNumberChildren(); i++)
   {
      BUnit* pUnit=gWorld->getUnit(mpOwner->getChild(i));
      if (pUnit)
         gWorld->notify(BEntity::cEventSquadModeChanaged, pUnit->getID(), mode, 0);
   }
}

//==============================================================================
//==============================================================================
bool BSquadAI::canMoveToAttack(bool allowAutoUnlock)
{
   if (mMode==cModeStandGround)
      return false;
   if (mMode == cModeLockdown && !allowAutoUnlock)
      return false;
   return (mpOwner->canMove());
}


//==============================================================================
//==============================================================================
bool BSquadAI::isSquadIgnored(BEntityID squadID) const
{
   uint numIgnored = mIgnoreList.getSize();
   for (uint i=0; i<numIgnored; i++)
   {
      if (mIgnoreList[i].first == squadID)
         return (true);
   }
   return (false);
}


//==============================================================================
//==============================================================================
void BSquadAI::removeExpiredIgnores()
{
   DWORD currentGameTime = gWorld->getGametime();
   uint i = 0;
   while (i < mIgnoreList.getSize())
   {
      if (currentGameTime >= mIgnoreList[i].second)
         mIgnoreList.removeIndex(i);
      else
         i++;
   }
}


//==============================================================================
//==============================================================================
void BSquadAI::ignoreSquad(BEntityID squadID, DWORD ignoreDuration)
{
   DWORD ignoreTimeout = gWorld->getGametime() + ignoreDuration;
   uint numIgnored = mIgnoreList.getSize();
   for (uint i=0; i<numIgnored; i++)
   {
      if (mIgnoreList[i].first == squadID)
      {
         mIgnoreList[i].second = ignoreTimeout;
         return;
      }
   }
   mIgnoreList.add(std::make_pair(squadID, ignoreTimeout));
   numIgnored;
}


//==============================================================================
//==============================================================================
bool BSquadAI::doesTargetObeyLeash(BEntityID squadID) const
{
   // No target, then 
   const BSquad* pTargetSquad = gWorld->getSquad(squadID);
   if (!pTargetSquad)
      return (false);

   // Get lots of data
   BVector targetPos = pTargetSquad->getPosition();
   BVector leashPos = mpOwner->getLeashPosition();
   BVector startPos = mpOwner->getPosition();
   float pathingRadius = mpOwner->getPathingRadius();
   float leashDistance = mpOwner->getLeashDistance() + mpOwner->getObstructionRadius();  // We need to add the obstruction radius here because in updateLeash() it does an obstruction to leashPt distance check.
   float leashDistanceSqr = leashDistance * leashDistance;

   // Use the squad plotter to figure out where we are going to stand if we attack this opportunity.
   static BEntityIDArray squadIDs;
   squadIDs.resize(1);
   squadIDs[0] = mpOwner->getID();
   static BDynamicSimVectorArray waypoints;
   waypoints.resize(1);
   waypoints[0] = mpOwner->getPosition();
   bool haveValidPlots = gSquadPlotter.plotSquads(squadIDs, mpOwner->getPlayerID(), squadID, waypoints, targetPos, -1, BSquadPlotter::cSPFlagIgnorePlayerRestriction);
   if (haveValidPlots && gSquadPlotter.getResults().getSize() == 1)
      targetPos = gSquadPlotter.getResults()[0].getDesiredPosition();

   bool canJump = mpOwner ? mpOwner->canJump() : false;

   static BPath path;
   path.reset();
   long result = gPather.findPath(&gObsManager, startPos, targetPos, pathingRadius, 0.0f, NULL, &path, false, true, canJump, squadID, BPather::cLongRange);
   if (result == BPath::cFull || result == BPath::cPartial || result == BPath::cInRangeAtStart)
   {
      long numWaypoints = path.getNumberWaypoints();
      for (long i=0; i<numWaypoints; i++)
      {
         if (path.getWaypoint(i).xzDistanceSqr(leashPos) > leashDistanceSqr)
            return (false);
      }
      return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BSquadAI::deinit()
{
   // Actually free the memory
   mOpportunityList.deinit();
}

//==============================================================================
//==============================================================================
bool BSquadAI::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BSquadAIIgnoreItem, mIgnoreList, uint16, 2000);
   //BSquad* mpOwner;
   GFWRITECLASS(pStream, saveType, mWorkOpp);
   GFWRITEVAR(pStream, uint8, mMode);

   GFWRITEBITBOOL(pStream, mFlagCanWork);
   GFWRITEBITBOOL(pStream, mFlagSearchToken);
   GFWRITEBITBOOL(pStream, mFlagWorkToken);

   GFWRITEMARKER(pStream, cSaveMarkerSquadAI1);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadAI::load(BStream* pStream, int saveType)
{
   // Opportunity list is now shared.  For old versions read the opportunity list for each squad AI.
   if (BSquadAI::mGameFileVersion < 4)
   {
      mOpportunityList.clearOpportunities();                // Make sure data cleared before read
      GFREADCLASS(pStream, saveType, mOpportunityList);
      mOpportunityList.clearOpportunities();                // Don't need to save data so might as well reset it here too
   }

   GFREADARRAY(pStream, BSquadAIIgnoreItem, mIgnoreList, uint16, 2000);
   //BSquad* mpOwner;
   GFREADCLASS(pStream, saveType, mWorkOpp);

   if (BSquadAI::mGameFileVersion >= 3)
   {
      GFREADVAR(pStream, uint8, mMode);
   }
   else
   {
      long oldMode;
      GFREADVAR(pStream, long, oldMode);
      mMode=static_cast<uint8>(oldMode);
   }

   if (BSquadAI::mGameFileVersion < 3)
   {
      uint8 oldSearchCounter;
      GFREADVAR(pStream, uint8, oldSearchCounter);
      bool oldFlagSearchPending;
      GFREADBITBOOL(pStream, oldFlagSearchPending);
      bool oldFlagSearchRetry;
      GFREADBITBOOL(pStream, oldFlagSearchRetry);
   }

   if (BSquadAI::mGameFileVersion >= 2)
   {
      GFREADBITBOOL(pStream, mFlagCanWork);
   }

   if (BSquadAI::mGameFileVersion >= 3)
   {
      GFREADBITBOOL(pStream, mFlagSearchToken);
      GFREADBITBOOL(pStream, mFlagWorkToken);
   }
   else
   {
      mFlagSearchToken=false;
      mFlagWorkToken=false;
   }
   GFREADMARKER(pStream, cSaveMarkerSquadAI1);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadAI::savePtr(BStream* pStream) const
{
   GFWRITEVAL(pStream, BEntityID, mpOwner->getID());
   return true;
}
