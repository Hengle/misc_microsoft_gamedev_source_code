//==============================================================================
// unitactionroar.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionroar.h"
#include "protoobject.h"
#include "selectionmanager.h"
#include "tactic.h"
#include "unit.h"
#include "unitquery.h"
#include "usermanager.h"
#include "world.h"
#include "pather.h"
#include "protosquad.h"

const static float cRoarTimerMin = 5;
const static float cRoarTimerMax = 60;

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionRoar, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionRoar::init()
{
   if (!BAction::init())
      return (false);

   mRoarTimer = 1.0f;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionRoar::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   return true;
}
//==============================================================================
//==============================================================================
void BUnitActionRoar::disconnect()
{
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionRoar::update(float elapsed)
{
//-- FIXING PREFIX BUG ID 1726
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);   

   switch (mState)
   {
      case cStateNone:
      {                  
         rollTimer();
         setState(cStateWait);         
         break;
      }
      case cStateWait:
      {
         mRoarTimer -= elapsed;
         if(mRoarTimer < 0.0f)
            mRoarTimer = 0.0f;

         if(mRoarTimer == 0.0f)
         {
            roar();                        
            setState(cStateWorking);
         }                  
         break;
      }

      case cStateWorking:
      {         
         if(pUnit->getOppByID(mRoarOppID) == NULL)
         {
            mRoarOppID = BUnitOpp::cInvalidID;
            setState(cStateNone);         
         }
         break;
      }
   }   

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRoar::rollTimer()
{
   mRoarTimer = getRandRangeFloat(cSimRand, cRoarTimerMin, cRoarTimerMax);
}

//==============================================================================
//==============================================================================
void BUnitActionRoar::roar()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);   

   BUnitOpp* pNewOpp=BUnitOpp::getInstance();
   pNewOpp->init();
   BSimTarget target(pUnit->getID());   
   pNewOpp->setType(BUnitOpp::cTypeAnimation);
   pNewOpp->setSource(pUnit->getID());
   pNewOpp->setUserData(static_cast<uint16>(mpProtoAction->getAnimType()));
   pNewOpp->generateID();   
   pNewOpp->setPriority(BUnitOpp::cPriorityHigh); 
   pNewOpp->setMustComplete(true);
   bool result = pUnit->addOpp(pNewOpp);
   if(result)
      mRoarOppID = pNewOpp->getID();
   else
      mRoarOppID = BUnitOpp::cInvalidID;
}

//==============================================================================
//==============================================================================
void BUnitActionRoar::scurryNearbyUnits()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);   

   //-- Query for nearby squads
   BEntityIDArray results(0, 100);
   BVector ownerPos=pUnit->getPosition();
   BUnitQuery gatherQuery(ownerPos, pUnit->getLOS(), false);
   gatherQuery.setUnitVisibility(pUnit->getPlayer()->getID());
   gatherQuery.setFlagIgnoreDead(true);   
   gWorld->getSquadsInArea(&gatherQuery, &results);
   if (results.getNumber() <= 0)
      return;

   for (uint i=0; i < results.getSize(); i++)
   {
      BSquad* pTargetSquad=gWorld->getSquad(results[i]);
      if (!pTargetSquad)
         continue;

      if(pTargetSquad->containsChild(pUnit->getID()))
         continue;     

      //-- Tell the Squad to run!
      scurrySquad(pTargetSquad);

      uint numChildren = pTargetSquad->getNumberChildren();
      for(uint j=0; j<numChildren; j++)
      {  
         BUnit* pChildUnit= gWorld->getUnit(pTargetSquad->getChild(j));
         if(pChildUnit)
            cowerUnit(pChildUnit); //-- Tell the unit to cower!
      }
    }
}

//==============================================================================
//==============================================================================
void BUnitActionRoar::scurrySquad(BSquad* pSquad)
{
   /*if(pSquad->getProtoSquad()->getFlagScaredByRoar())
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);   

      BVector myPos = pUnit->getPosition();
      BVector targetPos = pSquad->getPosition();

      BVector dir = targetPos - myPos;
      dir.normalize();

      dir.scale(pSquad->getObstructionRadius() * 3);
      BVector newPos = targetPos + dir;
      gPather.findClosestPassableTile(newPos, newPos);

      BSimTarget target;
      target.setPosition(newPos);
      pSquad->queueMove(target);
      pSquad->queueMove(targetPos);
   }*/
}

//==============================================================================
//==============================================================================
void BUnitActionRoar::cowerUnit(BUnit* pUnit)
{
   if(!pUnit)
      return;

   if(pUnit->hasAnimation(cAnimTypeCower))
   {
      BUnitOpp* pNewOpp=BUnitOpp::getInstance();
      pNewOpp->init();
      BSimTarget target(pUnit->getID());   
      pNewOpp->setType(BUnitOpp::cTypeAnimation);
      pNewOpp->setSource(pUnit->getID());
      pNewOpp->setUserData(cAnimTypeCower);
      pNewOpp->generateID();   
      pNewOpp->setPriority(BUnitOpp::cPriorityHigh);
      pNewOpp->setMustComplete(true);
      pUnit->addOpp(pNewOpp);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRoar::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{   
   //-- Tag validation
   if(mState != cStateWorking)
      return;

//-- FIXING PREFIX BUG ID 1727
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);   

   if (senderID != pUnit->getID())
      return;

   if(eventType == cAnimEventCameraShake)
   {
      //-- Make sure this is our camera shake      
      const BUnitOpp* pOpp = pUnit->getOppByID(mRoarOppID);
      if(pOpp && pOpp->getEvaluated() == true)
      {
         scurryNearbyUnits();
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionRoar::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mRoarTimer);
   GFWRITEVAR(pStream, BUnitOppID, mRoarOppID);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRoar::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mRoarTimer);
   GFREADVAR(pStream, BUnitOppID, mRoarOppID);
   return true;
}
