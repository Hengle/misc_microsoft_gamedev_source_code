//==============================================================================
// squadactionreinforce.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "squadactionreinforce.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "actionmanager.h"
#include "soundmanager.h"
#include "squadactionungarrison.h"
#include "visualitem.h"
#include "protosquad.h"
#include "SimOrderManager.h"
#include "configsgame.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionReinforce, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionReinforce::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

//-- FIXING PREFIX BUG ID 1696
   const BSquad* pReinforceSquad = gWorld->getSquad(mReinforceSquad);
//--
   BASSERT(pReinforceSquad);
   if (pReinforceSquad)
      mFlagNonMobileCache = pReinforceSquad->getFlagNonMobile();

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionReinforce::disconnect()
{   
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   removeOpp();

   // Remove the child action.
   pSquad->removeActionByID(mChildActionID);

   if (mpChildOrder)
   {
      mpChildOrder->decrementRefCount();
      if (mpChildOrder->getRefCount() == 0)
         gSimOrderManager.markForDelete(mpChildOrder);
      mpChildOrder = NULL;
   }

   BSquad* pReinforceSquad = gWorld->getSquad(mReinforceSquad);
   if (pReinforceSquad)
   {
      pReinforceSquad->setFlagNonMobile(mFlagNonMobileCache);
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionReinforce::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mFlagFlyInFrom = false;
   mFlagUnloadSquad = false;
   mFlagFlyOffTo = false;
   mFlagUseMaxHeight = false;
   mTarget.reset();   
   mChildActionID = cInvalidActionID;
   mFutureState = cStateNone;
   mFlagActionFailed = false;
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
   mFlagAnyFailed = false;
   mFlagNonMobileCache = false;
   mpChildOrder = NULL;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionReinforce::setState(BActionState state)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   switch (state)
   {
      // Fly in animation
      case cStateIncoming:
         if (!flyInTransport())
         {
            setState(cStateFailed);
            return (false);
         }
         break;

      // Fly in from location
      case cStateFlyIn:
         if (!mFlagFlyInFrom)
         {
            setState(cStateUnloading);
            return (true);
         }

         if (!flyInFrom())
         {
            setState(cStateFailed);
            return (false);
         }         
         break;

      // Unload squads
      case cStateUnloading:
         if (!mFlagUnloadSquad)
         {
            setState(cStateFlyOff);
            return (true);
         }

         if (prepReinforcements())
         {            
            //XXXHalwes - 6/19/2007 - Revisit this.
            BSquad* pReinforceSquad = gWorld->getSquad(mReinforceSquad);
            if (pReinforceSquad)
            {
               // Make reinforce squad stop moving while unload happens               
               pReinforceSquad->setFlagNonMobile(true);
            }

            if (!unloadSquad())
            {
               setState(cStateFailed);
               return (false);
            }
         }
         else
         {
            // Abort!  Kill the reinforcements
            BSquad* pSquad = gWorld->getSquad(mReinforcementsSquad);
            if (pSquad)
            {
               pSquad->kill(true);
            }            
            setState(cStateFlyOff);
            return (true);
         }
         break;

      // Fly off to location
      case cStateFlyOff:
         if (!mFlagFlyOffTo)
         {
            setState(cStateReturning);
            return (true);
         }

         if (!flyOffTo())
         {
            setState(cStateFailed);
            return (false);
         }
         break;

      // Transports leave area
      case cStateReturning:         
         if (!flyOffTransport())
         {
            setState(cStateFailed);
            return (false);
         }
         break;

         // Done/Failed.
      case cStateDone:
      case cStateFailed:
         // Remove the child action.
         removeOpp();
         pSquad->removeActionByID(mChildActionID);

         selfDestruct();
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionReinforce::update(float elapsed)
{
   // If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
      {
         return (true);
      }

      mFutureState = cStateNone;
   }

   switch (mState)
   {
      // First update
      case cStateNone:
         setState(cStateIncoming);
         break;

      // Fly in animation
      case cStateIncoming:
         break;

      // Fly in from location
      case cStateFlyIn:
         break;

      // Unload squads
      case cStateUnloading:
         break;

      // Fly off to location
      case cStateFlyOff:
         break;

      // Transports leave area
      case cStateReturning:
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionReinforce::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   bool actionsDone = false;
   switch (eventType)
   {
      case BEntity::cEventActionFailed:
         if (mChildActionID == (BActionID)data1)
         {
            mFlagActionFailed = true;
            actionsDone = true;
         }
         break;

      case BEntity::cEventActionDone:
         if (mChildActionID == (BActionID)data1)
         {
            mFlagActionFailed = false;
            actionsDone = true;
         }
         break;

      // Check if this squad's units have completed their animation opportunities
      case BEntity::cEventOppComplete:
         {
            //Data1:  OppID.
            //Data2:  Success.
            if (data1 == mUnitOppID)
            {                  
               if (!data2)
               {                  
                  mFlagAnyFailed = true;                  
               }

               mUnitOppIDCount--;
               if (mUnitOppIDCount == 0)
               {
                  actionsDone = true;
                  if (mFlagAnyFailed)
                  {
                     mFlagActionFailed = true;
                  }
                  else
                  {
                     mFlagActionFailed = false;
                  }
                  mFlagAnyFailed = false;

                  mUnitOppID = BUnitOpp::cInvalidID;
               }
            }            
         }
         break;
   }    

   if (actionsDone && mFlagActionFailed)
   {
      switch (mState)
      {
         // Fly in animation
         case cStateIncoming:
            mFutureState = cStateFailed;
            break;

         // Fly in from location
         case cStateFlyIn:
            mFutureState = cStateFailed;
            break;

         // Unload squads
         case cStateUnloading:
            mFutureState = cStateFlyOff;
            break;

         // Fly off to location
         case cStateFlyOff:
            mFutureState = cStateFailed;
            break;

         // Transports leave area
         case cStateReturning:
            mFutureState = cStateDone;
            break;
      } 
      mFlagActionFailed = false;
   }
   else if (actionsDone && !mFlagActionFailed)
   {
      switch (mState)
      {
         // Fly in animation
         case cStateIncoming:
            mFutureState = cStateFlyIn;
            break;

         // Fly in from location
         case cStateFlyIn:
            mFutureState = cStateUnloading;
            break;

         // Unload squads
         case cStateUnloading:
            {
               // Transfer reinforcements
               BSquad* pReinforcementsSquad = gWorld->getSquad(mReinforcementsSquad);
               BSquad* pReinforceSquad = gWorld->getSquad(mReinforceSquad);
               if (pReinforcementsSquad && pReinforceSquad && pReinforceSquad->isAlive())
               {                  
                  pReinforcementsSquad->transferChildren(pReinforceSquad);
               }
               // Abort!
               else
               {               
                  pReinforcementsSquad->kill(true);
               }

               //XXXHalwes - 6/19/2007 - Revisit this.
               if (pReinforceSquad)
               {                  
                  pReinforceSquad->setFlagNonMobile(mFlagNonMobileCache);
               }                     
               mFutureState = cStateFlyOff;
            }
            break;

         // Fly off to location
         case cStateFlyOff:
            mFutureState = cStateReturning;
            break;

         // Transports leave area
         case cStateReturning:
            mFutureState = cStateDone;
            break;
      }       
   }
}

//==============================================================================
// Static function used to fly in a transport to drop off a reinforcement squad
//==============================================================================
bool BSquadActionReinforce::reinforceSquad(BSimOrder* pOrder, BEntityID reinforceSquad, BEntityID reinforcementsSquad, const BVector flyInLocation /*= cInvalidVector*/, const BVector flyOffLocation /*= cInvalidVector*/, BCueIndex soundCue /*= cInvalidCueIndex*/, bool useMaxHeight /*= true*/)
{
   if ((reinforcementsSquad == cInvalidObjectID) && (reinforceSquad == cInvalidObjectID))
   {
      return (false);
   }

   BSquad* pReinforceSquad = gWorld->getSquad(reinforceSquad);
   BSquad* pReinforcementsSquad = gWorld->getSquad(reinforcementsSquad);
   if (!pReinforceSquad || !pReinforcementsSquad)
   {
      return (false);
   }
   
   pReinforcementsSquad->setFlagIgnoreLeash(true);

   BPlayerID reinforcePlayerID = pReinforceSquad->getPlayerID();
   BPlayerID reinforcementsPlayerID = pReinforcementsSquad->getPlayerID();
   if (reinforcePlayerID != reinforcementsPlayerID)
   {
      return (false);
   }

   BPlayer* pPlayer = gWorld->getPlayer(reinforcePlayerID);
//-- FIXING PREFIX BUG ID 1694
   const BCiv* pCiv = pPlayer ? pPlayer->getCiv() : NULL;
//--
   int transportProtoID = pCiv ? pCiv->getTransportProtoID() : cInvalidProtoID;
   if (transportProtoID == cInvalidProtoID)
   {
      return (false);
   }            

   BObjectCreateParms objectParms;
   objectParms.mPlayerID = reinforcePlayerID;
   objectParms.mStartBuilt = true;
   objectParms.mProtoObjectID = transportProtoID;
   objectParms.mProtoSquadID = -1;

   BVector reinforceLoc = pReinforceSquad->getPosition();
   bool flyIn = (flyInLocation != cInvalidVector) ? true : false;
   if (flyIn)
   {
      objectParms.mPosition = flyInLocation;
      objectParms.mForward = reinforceLoc - flyInLocation;
      objectParms.mForward.y = 0.0f;
      if (!objectParms.mForward.safeNormalize())
      {
         objectParms.mForward = cZAxisVector;
      }
      objectParms.mRight = XMVector3Cross(cYAxisVector, objectParms.mForward);
   }
   else
   {
      objectParms.mPosition = reinforceLoc;
      objectParms.mForward = pReinforceSquad->getForward();
      objectParms.mRight = pReinforceSquad->getRight();
   }

   BSquad* pTransportSquad = gWorld->createSquad(objectParms);
   if (pTransportSquad)
   {
      BSquadActionReinforce* pAction = (BSquadActionReinforce*)gActionManager.createAction(BAction::cActionTypeSquadReinforce);
      if (pAction)
      {
         pReinforcementsSquad->removeAllOrders();
         pReinforceSquad->removeAllOrders();

         pAction->setFlyInFrom(flyIn);
         pAction->setUnloadSquad(true);
         pAction->setUseMaxHeight(useMaxHeight);
         pAction->setFlyOffTo((flyOffLocation != cInvalidVector) ? true : false);
         pAction->setReinforcementsSquad(reinforcementsSquad);
         pAction->setReinforceSquad(reinforceSquad);            
         pAction->setFlyInLocation(flyInLocation);
         pAction->setFlyOffLocation(flyOffLocation);
         pAction->setCompletionSoundCue(soundCue);         

         pTransportSquad->addAction(pAction, pOrder);

         pAction->preLoadSquad();

         return (true);
      }
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BSquadActionReinforce::selfDestruct()
{
//-- FIXING PREFIX BUG ID 1698
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
   if (pUnit)
   {
      pUnit->kill(true);
   }
}

//==============================================================================
// Load the squad into the transport
//==============================================================================
void BSquadActionReinforce::preLoadSquad()
{
//-- FIXING PREFIX BUG ID 1699
   const BSquad* pSquad = mpOwner->getSquad();
//--

   // Garrison reinforcements
//-- FIXING PREFIX BUG ID 1700
   const BSquad* pSquadToGarrison = gWorld->getSquad(mReinforcementsSquad);
//--
   if (pSquadToGarrison)
   {
      // Iterate through the squad's units
      uint unitCount = pSquadToGarrison->getNumberChildren();
      for (uint j = 0; j < unitCount; j++)
      {
         BEntityID unitID = pSquadToGarrison->getChild(j);
         BUnit* pUnitToGarrison = gWorld->getUnit(unitID);
         if (pUnitToGarrison)
         {
            BUnit* pTransportUnit = gWorld->getUnit(pSquad->getChild(0));

            // Stop unit and garrison it
            pUnitToGarrison->setFlagUseMaxHeight(mFlagUseMaxHeight);
            pTransportUnit->containUnit(unitID);
         }
      }
   }
}

//==============================================================================
// Fly in from specified location
//==============================================================================
bool BSquadActionReinforce::flyInFrom()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   
   removeOpp();

   bool platoonMove = false;
   BEntityID parentID = pSquad->getParentID();
   if (mpOrder && (parentID != cInvalidObjectID) && (mpOrder->getOwnerID() == parentID))
   {
      platoonMove = true;
   }

   BSquad* pReinforceSquad = gWorld->getSquad(mReinforceSquad);
   BSimTarget target(mReinforceSquad);
   if (pReinforceSquad)
   {
      const BVector pos = pReinforceSquad->getPosition();
      target.setPosition(pos);      
   }
   setupChildOrder(mpOrder);
   if (mpChildOrder)
   {
      mpChildOrder->setTarget(target);
   }
   // Have our squad move.  If it can't, fail.
   mChildActionID = pSquad->doMove(mpChildOrder, this, &target, platoonMove, true);
   if (mChildActionID == cInvalidActionID)
   {
      return (false);
   }

   return (true);  
}

//==============================================================================
// Fly off to specified location
//==============================================================================
bool BSquadActionReinforce::flyOffTo()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   removeOpp();

   playCompletionSoundCue();
   
   bool platoonMove = false;
   BEntityID parentID = pSquad->getParentID();
   if (mpOrder && (parentID != cInvalidObjectID) && (mpOrder->getOwnerID() == parentID))
   {
      platoonMove = true;
   }

   BSimTarget target(mFlyOffLocation);
   setupChildOrder(mpOrder);
   if (mpChildOrder)
   {
      mpChildOrder->setTarget(target);
   }
   // Have our squad move.  If it can't, fail.
   mChildActionID = pSquad->doMove(mpChildOrder, this, &target, platoonMove, true);
   if (mChildActionID == cInvalidActionID)
   {
      return (false);
   }

   return (true);
}

//==============================================================================
// Ungarrison the squad from the transport
//==============================================================================
bool BSquadActionReinforce::unloadSquad()
{
//-- FIXING PREFIX BUG ID 1701
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   removeOpp();

   // Create a new army for the squads that are going to be ungarrisoned
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pSquad->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   BEntityIDArray tempList;
   tempList.add(mReinforcementsSquad);
   if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
      pArmy->addSquads(tempList, false);
   else
      pArmy->addSquads(tempList);

   // Ungarrison squad
   BSimTarget target(pSquad->getLeader());
   setupChildOrder(mpOrder);
   if (mpChildOrder)
   {
      mpChildOrder->setTarget(target);
   }
   BSquad* pSquadToUngarrison = gWorld->getSquad(mReinforcementsSquad);
   if (pSquadToUngarrison)
   {
      BSimTarget target(pSquad->getChild(0));
      mChildActionID = pSquadToUngarrison->doUngarrison(mpChildOrder, this, &target);
      if (mChildActionID == cInvalidActionID)
      {
         return (false);
      }                  
   }

   return (true);
}

//==============================================================================
// Fly transport off world
//==============================================================================
bool BSquadActionReinforce::flyOffTransport()
{
//-- FIXING PREFIX BUG ID 1703
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   BUnitOpp opp;
   opp.init();
   opp.setTarget(mTarget);
   opp.setType(BUnitOpp::cTypeAnimation);
   opp.setSource(pSquad->getID());
   opp.setUserData(cAnimTypeOutgoing);
   opp.generateID();
   if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
      opp.setTrigger(true);
   else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
      opp.setPriority(BUnitOpp::cPriorityCommand);
   else
      opp.setPriority(BUnitOpp::cPrioritySquad);

   bool result = addOpp(opp);

   return (result);
}

//==============================================================================
// Fly transport into the world
//==============================================================================
bool BSquadActionReinforce::flyInTransport()
{
//-- FIXING PREFIX BUG ID 1704
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   BUnitOpp opp;
   opp.init();
   opp.setTarget(mTarget);
   opp.setType(BUnitOpp::cTypeAnimation);
   opp.setSource(pSquad->getID());
   opp.setUserData(cAnimTypeIncoming);
   opp.generateID();
   if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
      opp.setTrigger(true);
   else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
      opp.setPriority(BUnitOpp::cPriorityCommand);
   else
      opp.setPriority(BUnitOpp::cPrioritySquad);

   bool result = addOpp(opp);

   return (result);
}

//==============================================================================
//==============================================================================
void BSquadActionReinforce::playCompletionSoundCue(void)
{
   if (mCompletionSoundCue != cInvalidCueIndex)
   {
      gSoundManager.playCue(mCompletionSoundCue);
   }
}

//==============================================================================
// Prepare the reinforcements for unloading
//==============================================================================
bool BSquadActionReinforce::prepReinforcements()
{
//-- FIXING PREFIX BUG ID 1706
   const BSquad* pReinforcementsSquad = gWorld->getSquad(mReinforcementsSquad);
//--
//-- FIXING PREFIX BUG ID 1707
   const BSquad* pReinforceSquad = gWorld->getSquad(mReinforceSquad);
//--
   if (!pReinforcementsSquad || !pReinforceSquad || !pReinforceSquad->isAlive())
   {
      return (false);
   }

   // We only want to unload the units that the squad needs
   const BProtoSquad* pProtoSquad = pReinforceSquad->getProtoSquad();
   if (!pProtoSquad)
   {
      return (false);
   }

   uint numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (uint i = 0; i < numUnitNodes; i++)
   {
      // What units does the squad need?
      const BProtoSquadUnitNode& unitNode = pProtoSquad->getUnitNode(i);
      uint unitTypeCount = 0;
      uint numUnits = pReinforceSquad->getNumberChildren();
      for (uint j = 0; j < numUnits; j++)
      {
//-- FIXING PREFIX BUG ID 1705
         const BUnit* pUnit = gWorld->getUnit(pReinforceSquad->getChild(j));
//--
         if (!pUnit)
         {
            return (false);
         }
         if (pUnit->getProtoID() == unitNode.mUnitType)
         {
            unitTypeCount++;
         }
      }

      // Kill the excess units
      int numTransportUnits = pReinforcementsSquad->getNumberChildren();
      for (int j = numTransportUnits - 1; j >= 0; j--)
      {
         BUnit* pUnit = gWorld->getUnit(pReinforcementsSquad->getChild(j));
         if (!pUnit)
         {
            return (false);
         }
         if ((pUnit->getProtoID() == unitNode.mUnitType) && (unitTypeCount > 0))
         {
            pUnit->kill(true);
            unitTypeCount--;
         }
      }
   }

   return (true);
}

//==============================================================================
// Add the opportunity to the children units
//==============================================================================
bool BSquadActionReinforce::addOpp(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 1709
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   if (pSquad->addOppToChildren(opp, mUnitOppIDCount))
   {      
      mUnitOppID = opp.getID();      
      return (true);
   }

   return (false);
}

//==============================================================================
// Remove the opportunity to the children units
//==============================================================================
void BSquadActionReinforce::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      return;
   }

   //Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 1710
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
}

//==============================================================================
// Setup the child order based on the parent order
//==============================================================================
void BSquadActionReinforce::setupChildOrder(BSimOrder* pOrder)
{
   if (!mpChildOrder)
   {
      mpChildOrder = gSimOrderManager.createOrder();
      BASSERT(mpChildOrder);
      mpChildOrder->incrementRefCount();
      mpChildOrder->setOwnerID(pOrder->getOwnerID());
      mpChildOrder->setPriority(pOrder->getPriority());
   }
   mpChildOrder->setAttackMove(false);
}

//==============================================================================
//==============================================================================
bool BSquadActionReinforce::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);      
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFWRITEVECTOR(pStream, mFlyInLocation);
   GFWRITEVECTOR(pStream, mFlyOffLocation);
   GFWRITEVAR(pStream, BEntityID, mReinforcementsSquad);
   GFWRITEVAR(pStream, BEntityID, mReinforceSquad);
   GFWRITEVAR(pStream, BCueIndex, mCompletionSoundCue);      
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);  
   GFWRITEVAR(pStream, BActionState, mFutureState);      
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEBITBOOL(pStream, mFlagFlyInFrom);
   GFWRITEBITBOOL(pStream, mFlagUnloadSquad);
   GFWRITEBITBOOL(pStream, mFlagFlyOffTo);
   GFWRITEBITBOOL(pStream, mFlagUseMaxHeight);
   GFWRITEBITBOOL(pStream, mFlagActionFailed);
   GFWRITEBITBOOL(pStream, mFlagAnyFailed);      
   GFWRITEBITBOOL(pStream, mFlagNonMobileCache);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionReinforce::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);      
   GFREADFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFREADVECTOR(pStream, mFlyInLocation);
   GFREADVECTOR(pStream, mFlyOffLocation);
   GFREADVAR(pStream, BEntityID, mReinforcementsSquad);
   GFREADVAR(pStream, BEntityID, mReinforceSquad);
   GFREADVAR(pStream, BCueIndex, mCompletionSoundCue);      
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);  
   GFREADVAR(pStream, BActionState, mFutureState);      
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADBITBOOL(pStream, mFlagFlyInFrom);
   GFREADBITBOOL(pStream, mFlagUnloadSquad);
   GFREADBITBOOL(pStream, mFlagFlyOffTo);
   GFREADBITBOOL(pStream, mFlagUseMaxHeight);
   GFREADBITBOOL(pStream, mFlagActionFailed);
   GFREADBITBOOL(pStream, mFlagAnyFailed);      
   GFREADBITBOOL(pStream, mFlagNonMobileCache);
   return true;
}
