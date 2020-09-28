//==============================================================================
// eventlistener.cpp
//
// Interface that defines the notify method for passing simulation events
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

#include "common.h"
#include "eventlistener.h"
#include "gamefilemacros.h"
#include "entity.h"
#include "world.h"
#include "actionmanager.h"
#include "powermanager.h"
#include "power.h"

//==============================================================================
//==============================================================================
bool IEventListener::savePtr(BStream* pStream, const IEventListener* pListener)
{
   int type = (pListener ? pListener->getEventListenerType() : -1);
   GFWRITEVAL(pStream, int8, type);
   if (pListener)
      return pListener->savePtr(pStream);
   else
      return true;
   /*
   switch (type)
   {
      case cEventListenerTypeEntity:
      {
         const BEntity* pEntity = (BEntity*)pListener;
         GFWRITEVAL(pStream, BEntityID, pEntity->getID());
         break;
      }

      case cEventListenerTypeSquadAI:
      {
         const BSquadAI* pSquadAI = (BSquadAI*)pListener;
         BSquad* pOwner = pSquadAI->getOwner();
         GFWRITEVAL(pStream, BEntityID, pOwner ? pOwner->getID() : cInvalidObjectID);
         break;
      }

      case cEventListenerTypeWorld:
         break;

      case cEventListenerTypeAction:
      {
         const BAction* pAction = (const BAction*)pListener;
         GFWRITEACTIONPTR(pStream, pAction);
         break;
      }

      case cEventListenerTypePower:
      {
         const BPower* pPower = (const BPower*)pListener;
         GFWRITEVAL(pStream, BPowerID, pPower->getID());
         break;
      }

      case -1:
         break;

      default:
         BASSERT(0);
   }
   return true;
   */
}

//==============================================================================
//==============================================================================
bool IEventListener::loadPtr(BStream* pStream, IEventListener** ppListener)
{
   (*ppListener) = NULL;
   int type;
   GFREADVAL(pStream, int8, int, type);
   switch (type)
   {
      case cEventListenerTypeEntity:
      {
         BEntityID entityID;
         //GFREADVAR(pStream, BEntityID, entityID);
         if (pStream->readBytes(&(entityID), sizeof(BEntityID)) != sizeof(BEntityID))
         {
            {GFERROR("GameFile Error: read var %s, type %s, on line %s, %d", "entityID", "BEntityID", __FILE__,__LINE__);}
            return false;
         }
         (*ppListener) = (IEventListener*)gWorld->getEntity(entityID);
         break;
      }

      case cEventListenerTypeSquadAI:
      {
         BEntityID squadID;
         GFREADVAR(pStream, BEntityID, squadID);
         BSquad* pSquad = gWorld->getSquad(squadID);
         if (pSquad)
            (*ppListener) = (IEventListener*)(pSquad->getSquadAI());
         break;
      }

      case cEventListenerTypeWorld:
         (*ppListener) = (IEventListener*)gWorld;
         break;

      case cEventListenerTypeAction:
      {
         BAction* pAction = NULL;
         GFREADACTIONPTR(pStream, pAction);
         (*ppListener) = (IEventListener*)pAction;
         break;
      }

      case cEventListenerTypePower:
      {
         BPowerID powerID;
         GFREADVAR(pStream, BPowerID, powerID);
         BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
         (*ppListener) = (IEventListener*)pPower;
         break;
      }

      case -1:
         break;
   }
   return true;
}
