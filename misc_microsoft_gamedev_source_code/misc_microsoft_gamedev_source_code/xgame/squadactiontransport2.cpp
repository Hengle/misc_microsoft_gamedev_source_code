//==============================================================================
// squadactiontransport2.cpp
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "squadactiontransport2.h"
#include "actionmanager.h"
#include "commands.h"
#include "SimOrderManager.h"
#include "squad.h"
#include "unit.h"
#include "world.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionTransport2, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionTransport2::connect(BEntity* pEntity, BSimOrder* pOrder)
{
//-- FIXING PREFIX BUG ID 4938
   const BSquad* pSquad = reinterpret_cast<BSquad*>(pEntity);
//--
   BASSERT(pSquad);

   uint numChildren = pSquad->getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
         pUnit->setFlagSelectTypeTarget(true);
   }

   if (!BAction::connect(pEntity, pOrder))
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BSquadActionTransport2::disconnect()
{
   removeOpp();
   removeChildActions();

   if (mpChildOrder)
   {
      mpChildOrder->decrementRefCount();
      if (mpChildOrder->getRefCount() == 0)
         gSimOrderManager.markForDelete(mpChildOrder);
      mpChildOrder = NULL;
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport2::init()
{
   if (!BAction::init())
      return false;

   mSquadList.clear();
   mSquadOffsets.clear();

   mChildActionIDs.clear();

   mPickupLocation = cInvalidVector;
   mDropOffLocation = cInvalidVector;
   mFlyOffLocation = cInvalidVector;
   mMovementPos = cInvalidVector;

   mpChildOrder = NULL;
   mUnitOppID = BUnitOpp::cInvalidID;
   mFutureState = cStateNone;
   mMovementTimer = 0.0f;

   mUnitOppIDCount = 0;

   mFlagActionFailed = false;
   mFlagAnyFailed = false;
   mFlagUseSquadPosition = false;

   mFlagConflictsWithIdle = true;

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport2::setState(BActionState state)
{
//-- FIXING PREFIX BUG ID 4939
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   switch (state)
   {
      // Fly in from location
      case cStateFlyIn:
      {
         mFlagUseSquadPosition = true;
         BVector pos = mPickupLocation;
         BVector dir = mDropOffLocation - mPickupLocation;
         dir.y = 0.0f;
         if (dir.safeNormalize())
         {
            static float cOffset = 20.0f;
            pos += dir * cOffset;
         }
         if (!moveTransport(pos))
         {
            setState(cStateFlyOff);
            return true;
         }         
         break;
      }

      // Wait for the transport to land when flying in
      case cStateFlyInWait:
         mMovementTimer = 0.0f;
         break;

      // Load squads
      case cStateLoading:
         if (!loadSquads())
         {
            setState(cStateFlyOff);
            return true;
         }
         else
         {
            // ajl 6/16/08 - load happens instantly for now
            setState(cStateMoving);
            return true;
         }
         break;

      // Transport squads
      case cStateMoving:
      {
         mFlagUseSquadPosition = true;
         BVector pos = mDropOffLocation;
         BVector dir = mDropOffLocation - mPickupLocation;
         dir.y = 0.0f;
         if (dir.safeNormalize())
         {
            static float cOffset = 20.0f;
            pos += dir * cOffset;
         }
         if (!moveTransport(pos))
         {
            setState(cStateFlyOff);
            return true;
         }
         break;
      }

      // Wait for transport to get in position to unload
      case cStateUnloadingWait:
         mMovementTimer = 0.0f;
         break;

      // Unload squads
      case cStateUnloading:
      {
         // Unload the squads
         if (!unloadSquads())
         {
            setState(cStateFlyOff);
            return true;
         }
         else
         {
            // ajl 6/16/08 - unload happens instantly for now
            setState(cStateFlyOff);
            return true;
         }
         break;
      }

      // Fly off to location
      case cStateFlyOff:
         if (!moveTransport(mFlyOffLocation))
         {
            setState(cStateDone);
            return true;
         }
         break;

      // Wait for the transport to fly to desired height
      case cStateFlyOffWait:
         mMovementTimer = 0.0f;
         break;

      // Done/Failed.      
      case cStateDone:
      case cStateFailed:
         removeOpp();
         removeChildActions();            
         selfDestruct();
         break;
   }

   return BAction::setState(state);
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport2::update(float elapsed)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

//-- FIXING PREFIX BUG ID 4940
   const BUnit* pUnit = pSquad->getLeaderUnit();
//--
   if (!pUnit)
   {
      mFutureState = cStateNone;
      setState(cStateFailed);
      return true;
   }

   // If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return true;
      mFutureState = cStateNone;
   }

   switch (mState)
   {
      // First update
      case cStateNone:
         positionSquads();
         mFutureState = cStateFlyIn;
         break;

      // Fly in from location
      case cStateFlyIn:
      {
         static float cThreshold = 2.0f;
         if (pSquad->getPosition().xzDistance(mPickupLocation) < cThreshold)
         {
            mFutureState = cStateFlyInWait;
            mFlagUseSquadPosition = false;
         }
         break;
      }

      // Wait for the transport to land when flying in
      case cStateFlyInWait:
      {
         static float cThreshold = 2.0f;
         float dist = pUnit->getPosition().xzDistance(mFlagUseSquadPosition ? pSquad->getPosition() : mPickupLocation);
         if (dist <= cThreshold || updateMovementStuck(elapsed))
            mFutureState = cStateLoading;
         break;
      }

      // Load squads
      case cStateLoading:
         break;

      // Transport squads
      case cStateMoving:
      {
         static float cThreshold = 2.0f;
         if (pSquad->getPosition().xzDistance(mDropOffLocation) < cThreshold)
         {
            mFutureState = cStateUnloadingWait;
            mFlagUseSquadPosition = false;
         }
         break;
      }

      // Wait for transport to get in position to unload
      case cStateUnloadingWait:
      {
         static float cThreshold = 2.0f;
         float dist = pUnit->getPosition().xzDistance(mFlagUseSquadPosition ? pSquad->getPosition() : mDropOffLocation);
         if (dist <= cThreshold || updateMovementStuck(elapsed))
            mFutureState = cStateUnloading;
         break;
      }

      // Unload squads
      case cStateUnloading:
         break;

      // Fly off to location. Wait for the transport to fly to desired height
      case cStateFlyOff:
      case cStateFlyOffWait:
      {
//-- FIXING PREFIX BUG ID 4941
         const BUnit* pUnit = pSquad->getLeaderUnit();
//--
         if (!pUnit)
         {
            mFutureState = cStateFailed;
            break;
         }
         static float cThreshold = 1.0f;
         float dist = mFlyOffLocation.y - pUnit->getPosition().y;
         if (dist <= cThreshold || updateMovementStuck(elapsed))
            mFutureState = cStateDone;
         break;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport2::getHoverOffset(float& offset) const
{
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   switch (mState)
   {
      case cStateFlyInWait:
      case cStateLoading:
         offset = gDatabase.getTransportPickupHeight();
         return true;

      case cStateMoving:
      {
         const BUnit* pUnit = pSquad->getLeaderUnit();
         if (pUnit)
         {
            static float cApproachDist = 60.0f;
            float dist = pUnit->getPosition().xzDistance(mDropOffLocation);
            if (dist <= cApproachDist)
            {
               offset = gDatabase.getTransportDropoffHeight();
               return true;
            }
         }
         break;
      }

      case cStateUnloadingWait:
      case cStateUnloading:
         offset = gDatabase.getTransportDropoffHeight();
         return true;

      case cStateFlyOff:
      case cStateFlyOffWait:
         offset = gDatabase.getTransportOutgoingHeight();
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BSquadActionTransport2::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   bool actionsDone = false;
   switch (eventType)
   {
      case BEntity::cEventActionFailed:
      {
         uint index = 0;
         if (mChildActionIDs.find((BActionID)data1, index))
            mFlagActionFailed = true;
         actionsDone = actionsComplete((BActionID)data1);
         break;
      }

      case BEntity::cEventActionDone:
         actionsDone = actionsComplete((BActionID)data1);
         break;

      // Check if this squad's units have completed their opps
      case BEntity::cEventOppComplete:
      {
         //Data1:  OppID.
         //Data2:  Success.
         if (data1 == mUnitOppID)
         {                  
            if (!data2)
               mFlagAnyFailed = true;                  
            mUnitOppIDCount--;
            if (mUnitOppIDCount == 0)
            {
               actionsDone = true;
               if (mFlagAnyFailed)
                  mFlagActionFailed = true;
               else
                  mFlagActionFailed = false;
               mFlagAnyFailed = false;
               mUnitOppID = BUnitOpp::cInvalidID;
            }
         }            
         break;
      }
   }    

   if (actionsDone && mFlagActionFailed)
   {
      switch (mState)
      {
         // Fly in from location
         case cStateFlyIn:
            mFutureState = cStateFlyOff;
            break;

         // Transport squads
         case cStateMoving:
            mFutureState = cStateUnloadingWait;
            break;

         // Fly off to location
         case cStateFlyOff:
            mFutureState = cStateFlyOffWait;
            break;
      } 
      mFlagActionFailed = false;
   }
   else if (actionsDone && !mFlagActionFailed)
   {
      switch (mState)
      {
         // Fly in from location
         case cStateFlyIn:
            mFutureState = cStateFlyInWait;
            break;

         // Transport squads
         case cStateMoving:
            mFutureState = cStateUnloadingWait;
            break;

         // Fly off to location
         case cStateFlyOff:
            mFutureState = cStateFlyOffWait;
            break;
      }       
   }
}

//==============================================================================
// Position squads to get ready for transporting
//==============================================================================
bool BSquadActionTransport2::positionSquads()
{
//-- FIXING PREFIX BUG ID 4943
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   // Create a new army and add the transporting squads
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = pSquad->getPlayerID();
   BArmy* pArmy = gWorld->createArmy(objectParms);
   BASSERT(pArmy);
   pArmy->addSquads(mSquadList);   

   BWorkCommand tempCommand;
   tempCommand.setWaypoints(&mPickupLocation, 1);
   tempCommand.setRecipientType(BCommand::cArmy);
   tempCommand.setFlag(BWorkCommand::cFlagAlternate, false);
   tempCommand.setFlag(BWorkCommand::cFlagAttackMove, false);
   pArmy->queueOrder(&tempCommand);

   return true;
}

//==============================================================================
// Move the transport
//==============================================================================
bool BSquadActionTransport2::moveTransport(BVector loc)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (pSquad->getPosition().almostEqual(loc))
      return setState(cStateFlyInWait);

   removeOpp();

   BSimTarget target(loc);
   setupChildOrder(mpOrder);
   if (!mpChildOrder)
      return false;
   mpChildOrder->setTarget(target);

   BActionID childActionID = pSquad->doMove(mpChildOrder, this, &target, false, true, false, true);
   if (childActionID == cInvalidActionID)
      return false;
   mChildActionIDs.add(childActionID);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionTransport2::updateMovementStuck(float elapsed)
{
   const float cMaxStillTime = 1.0f;
   const float cMinMovementDist = 1.0f;
   mMovementTimer += elapsed;
   if (mMovementTimer >= cMaxStillTime)
   {
      BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//-- FIXING PREFIX BUG ID 4945
      const BUnit* pUnit = pSquad->getLeaderUnit();
//--
      if (!pUnit)
         return true;
      float dist = mMovementPos.distance(pUnit->getPosition());
      if (dist < cMinMovementDist)
         return true;
      mMovementPos = pUnit->getPosition();
   }
   return false;
}

//==============================================================================
// Garrison the squads into the transports
//==============================================================================
bool BSquadActionTransport2::loadSquads()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // ajl 6/16/08 instantly load the squads for now since units seem to have a hard time getting in when 
   // there is more than one squad being loaded.
   BUnit* pTransport = pSquad->getLeaderUnit();
   if (!pTransport)
      return false;

   uint numSquadsToTransport = mSquadList.getSize();
   mSquadOffsets.setNumber(numSquadsToTransport);

   for (uint i=0; i<numSquadsToTransport; i++)
   {
      mSquadOffsets[i] = cOriginVector;

      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (!pSquadToTransport)
         continue;

      mSquadOffsets[i] = mPickupLocation - pSquadToTransport->getPosition();

      // Clear the transporting squad's current orders
      pSquadToTransport->removeOrders(true, true, true, false);

      uint numChildren = pSquadToTransport->getNumberChildren();
      for (uint j=0; j<numChildren; j++)
         pTransport->containUnit(pSquadToTransport->getChild(j));
   }
   return true;
}

//==============================================================================
// Ungarrison the squads from the transports
//==============================================================================
bool BSquadActionTransport2::unloadSquads()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // ajl 6/16/08 instantly load the squads for now since units seem to have a hard time getting in when 
   // there is more than one squad being loaded.
   BUnit* pTransport = pSquad->getLeaderUnit();
   if (!pTransport)
      return false;

   uint numSquadsToTransport = mSquadList.getSize();
   for (uint i=0; i<numSquadsToTransport; i++)
   {
      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (!pSquadToTransport)
         continue;
      pSquadToTransport->setPosition(mPickupLocation + mSquadOffsets[i]);
   }

   BDynamicSimVectorArray waypoints;
   waypoints.add(pTransport->getPosition());
   gSquadPlotter.plotSquads(mSquadList, pSquad->getPlayerID(), cInvalidObjectID, waypoints, pTransport->getPosition(), -1, BSquadPlotter::cSPFlagForceLandMovementType | BSquadPlotter::cSPFlagNoSync);
   const BDynamicSimArray<BSquadPlotterResult>& dropOffPlotterResults = gSquadPlotter.getResults();      
   uint numDropOffPlotterResults = dropOffPlotterResults.getSize();

   for (uint i=0; i<numSquadsToTransport; i++)
   {
      BSquad* pSquadToTransport = gWorld->getSquad(mSquadList[i]);
      if (!pSquadToTransport)
         continue;

      // Clear the transporting squad's current orders
      pSquadToTransport->removeOrders(true, true, true, false);

      BVector pos = (i < numDropOffPlotterResults ? dropOffPlotterResults[i].getDesiredPosition() : pTransport->getPosition());
      pSquadToTransport->setPosition(pos);
      pSquadToTransport->setLeashPosition(pos);

      uint numChildren = pSquadToTransport->getNumberChildren();
      for (uint j=0; j<numChildren; j++)
      {
         BUnit* pChildUnit = gWorld->getUnit(pSquadToTransport->getChild(j));
         if (!pChildUnit)
            continue;

         #ifdef SYNC_Unit
            syncUnitData("BSquadActionTransport2::unloadSquads", pSquadToTransport->getPosition());
         #endif
         pChildUnit->setPosition(pSquadToTransport->getPosition());
         BVector forward = pTransport->getForward();
         forward.y = 0.0f;
         if (!forward.safeNormalize())
            forward = cZAxisVector;
         pChildUnit->setForward(forward);
         pChildUnit->setUp(cYAxisVector);
         pChildUnit->calcRight();
         pChildUnit->clearGoalVector();
         pTransport->unloadUnit(pChildUnit->getID(), false);
      }

      pSquadToTransport->settle();
   }
   return true;
}

//==============================================================================
// Destroy transports
//==============================================================================
void BSquadActionTransport2::selfDestruct()
{
//-- FIXING PREFIX BUG ID 4946
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   uint numChildren = pSquad->getNumberChildren();
   for (uint i  = 0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
         pUnit->kill(true);
   }
}

//==============================================================================
// Setup the child order based on the parent order
//==============================================================================
void BSquadActionTransport2::setupChildOrder(BSimOrder* pOrder)
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
// Remove any cached child actions from the squads and the transports
//==============================================================================
void BSquadActionTransport2::removeChildActions()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   uint numChildActions = (uint)mChildActionIDs.getNumber();
   for (uint i = 0; i < numChildActions; i++)
   {
      // Remove the child action.
      pSquad->removeActionByID(mChildActionIDs[i]);
      
      uint numSquadsTransported = (uint)mSquadList.getNumber();
      for (uint j = 0; j < numSquadsTransported; j++)
      {
         BSquad* pSquadTransprted = gWorld->getSquad(mSquadList[i]);
         if (pSquadTransprted)
         {
            // Remove the child action.
            pSquadTransprted->removeActionByID(mChildActionIDs[i]);
         }
      }
   }
}

//===================================================================================================================
// Check action against cached child actions and remove it when found.  When all actions are gone they are complete.
//===================================================================================================================
bool BSquadActionTransport2::actionsComplete(BActionID id)
{
   uint index = 0;
   if (mChildActionIDs.find(id, index))
   {
      if (mChildActionIDs.removeIndex(index))
      {
         int numChildActions = mChildActionIDs.getNumber();
         return (numChildActions == 0);
      }
   }
   return false;
}

//==============================================================================
// Add the opportunity to the children units
//==============================================================================
bool BSquadActionTransport2::addOpp(BUnitOpp opp)
{
   //Give our opp to our units.
//-- FIXING PREFIX BUG ID 4948
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   if (pSquad->addOppToChildren(opp, mUnitOppIDCount))
   {      
      mUnitOppID = opp.getID();      
      return true;
   }

   return false;
}

//==============================================================================
// Remove the opportunity to the children units
//==============================================================================
void BSquadActionTransport2::removeOpp()
{
   if (mUnitOppID == BUnitOpp::cInvalidID)
      return;

   //Remove the opportunity that we've given the units.  That's all we do here.
//-- FIXING PREFIX BUG ID 4949
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
}

//===========================================================================================================================
// Calculate the number of transports required to transport the provided squads
//===========================================================================================================================
uint BSquadActionTransport2::calculateNumTransports(BEntityIDArray squadList, BPlayerID playerID, BProtoObjectID transportProtoObjectID)
{
   if (squadList.getSize() <= 0)
      return 0;

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return 0;

//-- FIXING PREFIX BUG ID 4950
   const BProtoObject* pTransportProtoObject = pPlayer->getProtoObject(transportProtoObjectID);
//--
   if (!pTransportProtoObject)
      return 0;

   // ajl 6/18/08 - Temp hard code to 1 transport. Need to make data driven to support multiple transports. Specs states up to 3.
   return 1;
}

//===========================================================================================================================
// Static function used to fly in transports, pick up troops and drop em off at a different location before flying off again
//===========================================================================================================================
class BTransportData2
{
   public:
      BEntityIDArray mSquadsToTransport;
      BVector mPickUpLoc;
      BVector mDropOffLoc;
      BVector mPos;
      BVector mForward;
      BVector mRight;
      BEntityID mTransportSquad;
};

bool BSquadActionTransport2::transportSquads(BSimOrder* pSimOrder, BEntityIDArray squadList, BVector dropOffLocation, BPlayerID playerID, BProtoObjectID transportProtoObjectID)
{   
   if (squadList.getSize() == 0)
      return false;

   // Get total number of transports needed
   const long cMaxTransports = 1;
   uint totalNumTransports = calculateNumTransports(squadList, playerID, transportProtoObjectID);
   if (totalNumTransports == 0 || totalNumTransports > cMaxTransports)
      return false;

   // Setup parameters for transports   
   BObjectCreateParms objectParms;
   objectParms.mPlayerID = playerID;
   objectParms.mStartBuilt = true;
   objectParms.mProtoObjectID = transportProtoObjectID;
   objectParms.mProtoSquadID = -1;
   objectParms.mPosition = cOriginVector;
   objectParms.mForward = cZAxisVector;
   objectParms.mRight = XMVector3Cross(cYAxisVector, objectParms.mForward);

   // Create the transports
   BEntityIDArray transports;
   for (uint i = 0; i < totalNumTransports; i++)
   {
//-- FIXING PREFIX BUG ID 4952
      const BSquad* pTransportSquad = gWorld->createSquad(objectParms);
//--
      if (pTransportSquad)
         transports.add(pTransportSquad->getID());
   }

   // Fill out the transport data array.
   BTransportData2 transportData[cMaxTransports];

   // Compute pickup location
   BVector pickupLoc = cOriginVector;
   uint numSquads = 0;
   for (uint i=0; i<squadList.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 4953
      const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
      if (pSquad)
      {
         pickupLoc += pSquad->getPosition();
         numSquads++;
      }
   }
   if (numSquads == 0)
      return false;
   pickupLoc /= (float)numSquads;
   gTerrainSimRep.getHeight(pickupLoc, false);

   // Compute direction
   BVector travelDirection = dropOffLocation - pickupLoc;
   BVector transportForward = travelDirection;
   transportForward.y = 0.0f;
   if (!transportForward.safeNormalize())
      transportForward = cZAxisVector;
   BVector transportRight = XMVector3Cross(cYAxisVector, transportForward);

   // Fill out transport data
   BTransportData2& data = transportData[0];
   data.mTransportSquad = transports[0];
   data.mPos = pickupLoc;
   data.mForward = transportForward;
   data.mRight = transportRight;
   data.mSquadsToTransport = squadList;
   data.mPickUpLoc = pickupLoc;
   data.mDropOffLoc = dropOffLocation;

   // Compute initial position for transport to fly in from
   BVector flyInPos = pickupLoc;
   BVector offset = transportForward * -gDatabase.getTransportIncomingOffset();
   flyInPos += offset;
   flyInPos.y += gDatabase.getTransportIncomingHeight();

   for (uint j = 0; j < totalNumTransports; j++)
   {
      BSquad* pTransportSquad = gWorld->getSquad(transportData[j].mTransportSquad);
      if (pTransportSquad)
      {
         // Update transport squad, unit, and physics object position and orientation
         pTransportSquad->setPosition(flyInPos);
         pTransportSquad->setForward(transportForward);
         pTransportSquad->setRight(transportRight);
         pTransportSquad->setLeashPosition(flyInPos);

         BUnit* pTransportUnit = pTransportSquad->getLeaderUnit();
         if (pTransportUnit)
         {
            #ifdef SYNC_Unit
               syncUnitData("BSquadActionTransport2::transportSquads", flyInPos);
            #endif
            pTransportUnit->setPosition(flyInPos, true);
            BMatrix rot;
            pTransportSquad->getWorldMatrix(rot);
            rot.setTranslation(cOriginVector);
            pTransportUnit->setRotation(rot, true);

            // Don't allow the user to garrison any units on this transport.
            pTransportUnit->setFlagBlockContain(true);
         }

         // Create the transport action
         BSquadActionTransport2* pAction = reinterpret_cast<BSquadActionTransport2*>(gActionManager.createAction(BAction::cActionTypeSquadTransport2));
         if (pAction)
         {
            pAction->setSquadList(transportData[j].mSquadsToTransport);
            pAction->setPickupLocation(transportData[j].mPickUpLoc);
            pAction->setDropOffLocation(transportData[j].mDropOffLoc);

            BVector flyOffLoc = cOriginVector;
            if (gDatabase.getTransportOutgoingOffset() > 0.0f)
            {
               BVector dir = transportData[j].mDropOffLoc - transportData[j].mPickUpLoc;
               dir.y = 0.0f;
               dir.safeNormalize();
               flyOffLoc = transportData[j].mDropOffLoc + (dir * gDatabase.getTransportOutgoingOffset());
               flyOffLoc.y += gDatabase.getTransportOutgoingHeight();
            }
            pAction->setFlyOffLocation(flyOffLoc);

            pTransportSquad->addAction(pAction, pSimOrder);                        
         }         
      }
   }

   return true;
}
