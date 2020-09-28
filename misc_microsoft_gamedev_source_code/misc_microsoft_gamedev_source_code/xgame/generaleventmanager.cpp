//==============================================================================
// GeneralEventManager.h
//
// 
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "generaleventmanager.h"
#include "command.h"
#include "commands.h"
#include "commandmanager.h"
#include "mpSimDataObject.h"
#include "selectionmanager.h"
#include "techtree.h"
#include "transitionManager.h"
#include "user.h"
#include "world.h"
#include "modemanager.h"
#include "modegame.h"

GFIMPLEMENTVERSION(BGeneralEventManager, 1);
enum
{
   cSaveMarkerSubscribers=10000,
   cSaveMarkerEvents,
   cSaveMarkerQueue,
};

//==============================================================================
//==============================================================================
//Filters
IMPLEMENT_FREELIST(BEventFilterEntity, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEventFilterEntityList, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEventFilterLocation, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEventFilterCamera, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEventFilterGameState, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEventFilterType, 4, &gSimHeap);
IMPLEMENT_FREELIST(BEventFilterNumeric, 4, &gSimHeap);

//Subscribers
IMPLEMENT_FREELIST(BGeneralEventSubscriber, 5, &gSimHeap);


BGeneralEventManager gGeneralEventManager;

//==============================================================================
// Constants
//==============================================================================

inline BGeneralEventSubscriberID makeSubscriberID(uint index, BGeneralEventID eventID)
{
   return (index << 16) + eventID;
}
inline uint getSubscriberIndex(BGeneralEventSubscriberID id)
{
   return  (id  & 0xffff0000) >> 16;
}
inline BGeneralEventID getSubscriberEventID(BGeneralEventSubscriberID id)
{
   return (id & 0x0000ffff);
}

////==============================================================================
//// BGeneralEventManager::newSubscriber
////==============================================================================
BGeneralEventSubscriber* BGeneralEventManager::newSubscriber(BGeneralEventID id)
{
   BGeneralEvent& thisEvent = mGeneralEvents[id];
   BGeneralEventSubscriber* pSubscriber = BGeneralEventSubscriber::getInstance();   
   thisEvent.addSubscriber(pSubscriber);
   pSubscriber->mID = makeSubscriberID(thisEvent.getSubscriberCount()-1, id); 
   return pSubscriber;
}

////==============================================================================
//// BGeneralEventManager::getSubscriber
////==============================================================================
BGeneralEventSubscriber* BGeneralEventManager::getSubscriber(BGeneralEventSubscriberID id)
{
   uint index = getSubscriberIndex(id);
   BGeneralEventID eventId = getSubscriberEventID(id);
   if (index >= mGeneralEvents[eventId].mGeneralEventSubscribers.size())
      return NULL;
   return mGeneralEvents[eventId].getSubscriber(index);
}

////==============================================================================
//// BGeneralEventManager::removeSubscriber
////==============================================================================
void BGeneralEventManager::removeSubscriber(BGeneralEventSubscriberID id)
{
   //This is put on hold in favor of just clearing the filters on a subscriber.
   //See Andrew for details
    BASSERTM(false, "BGeneralEventManager::removeSubscriber disabled");
   //uint index = getSubscriberIndex(id);
   //BGeneralEventID eventId = getSubscriberEventID(id);
   //if (index >= mGeneralEvents[eventId].mGeneralEventSubscribers.size())
   //   return;
   //BGeneralEventSubscriber* pSub = mGeneralEvents[eventId].getSubscriber(index);
   //if(pSub == NULL)
   //   return;
   ////mGeneralEvents[eventId].removeSubscriber(pSub);
   //mGeneralEvents[eventId].mGeneralEventSubscribers[index] = NULL;
   //pSub->clearFilters();
   //BGeneralEventSubscriber::releaseInstance( pSub );
}

////==============================================================================
//// BGeneralEventManager::init
////==============================================================================

bool BGeneralEventManager::init()
{

   return true;
}


////==============================================================================
//// BGeneralEventManager::reset
////==============================================================================
void BGeneralEventManager::reset()
{
   mGeneralEvents[BEventDefinitions::cSize];

   for (uint i=0; i<BEventDefinitions::cSize; i++)
   {
      mGeneralEvents[i].clearSubscribers();         
   }
   mSubscriberFiredQueue.clear();
}

////==============================================================================
//// BGeneralEvent::clearSubscribers
////==============================================================================
void BGeneralEvent::clearSubscribers()
{
   uint numSubscribers = mGeneralEventSubscribers.size();
   for (uint i=0; i<numSubscribers; i++)
   {
      BGeneralEventSubscriber::releaseInstance( mGeneralEventSubscribers[i] );
   }
   mGeneralEventSubscribers.clear();
}

////==============================================================================
//// BGeneralEventManager::QueueSubscriberFired
////==============================================================================
void BGeneralEventManager::queueSubscriberFired( BGeneralEventSubscriberID id)
{
   BGeneralEventSubscriber* sub = getSubscriber(id);
   // subscribers that use the ref counting variable can't be executed through a command because it will be executed multiple times for each player (incorrectly incrementing the count)
   // so we just fire it locally and immediately
   if(sub != NULL && sub->useCount())
      subscriberFired(id);
   else
      mSubscriberFiredQueue.add(id);
}

////==============================================================================
//// BGeneralEventManager::SubscriberFired
////==============================================================================
void BGeneralEventManager::subscriberFired( BGeneralEventSubscriberID id)
{
   BGeneralEventSubscriber* sub = getSubscriber(id);
   if(sub != NULL)
   {
      sub->setFired();
   }
}

////==============================================================================
//// BGeneralEventManager::SubscriberFired
////==============================================================================
void BGeneralEventManager::flushQueue(long playerID, float elapsedTime)
{
   mOneSecondTimer += elapsedTime;
   if(mOneSecondTimer > 1)
   {
      gGeneralEventManager.eventTrigger(BEventDefinitions::cTimer1Sec, playerID);
      mOneSecondTimer = 0;
   }

   if(mSubscriberFiredQueue.size() == 0)
      return;

   if (!gModeManager.getModeGame() || !gModeManager.getModeGame()->getPaused())
   {
      BGeneralEventCommand* pCommand = (BGeneralEventCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGeneralEvent);

      if (!pCommand)
      {
         return;
      }
      
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &playerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGeneralEventCommand::cEventFired);

      for(uint i=0; i< mSubscriberFiredQueue.size(); i++)
      {
         pCommand->addSubscriber(mSubscriberFiredQueue[i]);
      }

      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }

   mSubscriberFiredQueue.clear();

}

//==============================================================================
//==============================================================================
bool BGeneralEventManager::save(BStream* pStream, int saveType) const
{
   GFWRITEFREELIST(pStream, saveType, BGeneralEventSubscriber, BGeneralEventSubscriber::mFreeList, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerSubscribers);

   uint16 genEventCount = BEventDefinitions::cSize;
   GFWRITEVAR(pStream, uint16, genEventCount);
   GFVERIFYCOUNT(genEventCount, 1000);
   for (uint16 i=0; i<genEventCount; i++)
   {
      const BGeneralEvent& genEvent = mGeneralEvents[i];
      uint16 subCount = (uint16)genEvent.mGeneralEventSubscribers.size();
      GFWRITEVAR(pStream, uint16, subCount);
      GFVERIFYCOUNT(genEventCount, 1000);
      for (uint16 j=0; j<subCount; j++)
      {
//-- FIXING PREFIX BUG ID 1522
         const BGeneralEventSubscriber* pSub = genEvent.mGeneralEventSubscribers[j];
//--
         GFWRITEFREELISTITEMPTR(pStream, BGeneralEventSubscriber, pSub);
      }
   }
   GFWRITEMARKER(pStream, cSaveMarkerEvents);

   GFWRITEARRAY(pStream, uint, mSubscriberFiredQueue, int16, 10000);
   GFWRITEMARKER(pStream, cSaveMarkerQueue);

//-- FIXING PREFIX BUG ID 1524
   const BGeneralEventSubscriber* pSub = gWorld->getCinematicManager()->getCinematicCompletedEventSubscriber();
//--
   BGeneralEventSubscriberID subID = (pSub ? pSub->mID : UINT_MAX);
   GFWRITEVAR(pStream, BGeneralEventSubscriberID, subID);

   pSub = gWorld->getChatManager()->getChatCompletedEventSubscriber();
   subID = (pSub ? pSub->mID : UINT_MAX);
   GFWRITEVAR(pStream, BGeneralEventSubscriberID, subID);

   pSub = gWorld->getTransitionManager()->getCompletedEventSubscriber();
   subID = (pSub ? pSub->mID : UINT_MAX);
   GFWRITEVAR(pStream, BGeneralEventSubscriberID, subID);

   return true;
}

//==============================================================================
//==============================================================================
bool BGeneralEventManager::load(BStream* pStream, int saveType)
{
   GFREADFREELIST(pStream, saveType, BGeneralEventSubscriber, BGeneralEventSubscriber::mFreeList, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerSubscribers);

   uint16 genEventCount;
   GFREADVAR(pStream, uint16, genEventCount);
   GFVERIFYCOUNT(genEventCount, BEventDefinitions::cSize);
   for (uint16 i=0; i<genEventCount; i++)
   {
      BGeneralEvent& genEvent = mGeneralEvents[i];
      uint16 subCount;
      GFREADVAR(pStream, uint16, subCount);
      GFVERIFYCOUNT(genEventCount, 1000);
      for (uint16 j=0; j<subCount; j++)
      {
         BGeneralEventSubscriber* pSub = NULL;
         GFREADFREELISTITEMPTR(pStream, BGeneralEventSubscriber, pSub);
         genEvent.mGeneralEventSubscribers.add(pSub);
      }
   }
   GFREADMARKER(pStream, cSaveMarkerEvents);

   GFREADARRAY(pStream, uint, mSubscriberFiredQueue, int16, 10000);
   GFREADMARKER(pStream, cSaveMarkerQueue);

   BGeneralEventSubscriber* pSub;
   BGeneralEventSubscriberID subID;
   GFREADVAR(pStream, BGeneralEventSubscriberID, subID);
   if (subID != UINT_MAX)
   {
      pSub = getSubscriber(subID);
      if (!pSub)
      {
         GFERROR("GameFile Error: can't find cinematic sub %u", subID);
         return false;
      }
   }
   else
      pSub = NULL;
   gWorld->getCinematicManager()->setCinematicCompletedEventSubscriber(pSub);

   GFREADVAR(pStream, BGeneralEventSubscriberID, subID);
   if (subID != UINT_MAX)
   {
      pSub = getSubscriber(subID);
      if (!pSub)
      {
         GFERROR("GameFile Error: can't find chat sub %u", subID);
         return false;
      }
   }
   else
      pSub = NULL;
   // Halwes - 10/30/2008 - If a chat had already fired then the "has fired" event will never fire because the save happened while the chat was being played.
   if (pSub && pSub->hasFired() == false)
      pSub->setFired();
   gWorld->getChatManager()->setChatCompletedEventSubscriber(pSub);

   GFREADVAR(pStream, BGeneralEventSubscriberID, subID);
   if (subID != UINT_MAX)
   {
      pSub = getSubscriber(subID);
      if (!pSub)
      {
         GFERROR("GameFile Error: can't find transition sub %u", subID);
         return false;
      }
   }
   else
      pSub = NULL;
   gWorld->getTransitionManager()->setCompletedEventSubscriber(pSub);

   return true;
}

//==============================================================================
//==============================================================================
bool BGeneralEventSubscriber::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, DWORD, mFireTime);
   GFWRITEVAR(pStream, int, mFiredCount);
   GFWRITEVAR(pStream, bool, mFired);
   GFWRITEVAR(pStream, bool, mUseCount);
   GFWRITEVAR(pStream, BGeneralEventSubscriberID, mID);
   GFWRITEVAR(pStream, bool, mCheckPlayer);
   GFWRITEVAR(pStream, long, mPlayer);

   // mEventFilters
   uint16 filterCount = (uint16)mEventFilters.size();
   GFWRITEVAR(pStream, uint16, filterCount);
   GFVERIFYCOUNT(filterCount, 1000);
   for (uint16 i=0; i<filterCount; i++)
   {
//-- FIXING PREFIX BUG ID 1526
      const BEventFilterBase* pFilter = mEventFilters[i];
//--
      GFWRITEVAL(pStream, BYTE, pFilter->getType());
      if (!mEventFilters[i]->save(pStream, saveType))
         return false;
   }

   return true;
}

////==============================================================================
////==============================================================================
bool BGeneralEventSubscriber::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, DWORD, mFireTime);
   GFREADVAR(pStream, int, mFiredCount);
   GFREADVAR(pStream, bool, mFired);
   GFREADVAR(pStream, bool, mUseCount);
   GFREADVAR(pStream, BGeneralEventSubscriberID, mID);
   GFREADVAR(pStream, bool, mCheckPlayer);
   GFREADVAR(pStream, long, mPlayer);

   // mEventFilters
   uint16 filterCount;
   GFREADVAR(pStream, uint16, filterCount);
   GFVERIFYCOUNT(filterCount, 1000);
   for (uint16 i=0; i<filterCount; i++)
   {
      BYTE filterType;
      GFREADVAR(pStream, BYTE, filterType);
      BEventFilterBase* pFilter = NULL;
      switch (filterType)
      {
         case BEventFilterBase::cFilterTypeEntity     : pFilter = BEventFilterEntity::getInstance(); break; 
         case BEventFilterBase::cFilterTypeEntityList : pFilter = BEventFilterEntityList::getInstance(); break;
         case BEventFilterBase::cFilterTypeLocation   : pFilter = BEventFilterLocation::getInstance(); break;
         case BEventFilterBase::cFilterTypeCamera     : pFilter = BEventFilterCamera::getInstance(); break;
         case BEventFilterBase::cFilterTypeGameState  : pFilter = BEventFilterGameState::getInstance(); break;
         case BEventFilterBase::cFilterTypeType       : pFilter = BEventFilterType::getInstance(); break;
         case BEventFilterBase::cFilterTypeNumeric    : pFilter = BEventFilterNumeric::getInstance(); break;
         default: return false;
      }
      if (!pFilter)
         return false;
      if (!pFilter->load(pStream, saveType))
         return false;
      mEventFilters.add(pFilter);
   }

   return true;
}

////==============================================================================
//// BGeneralEventSubscriber::eventTrigger  
////==============================================================================

void BGeneralEventSubscriber::setFired() 
{
   mFired = true; 
   mFireTime=gWorld->getGametime(); 
   if (mUseCount)
      mFiredCount++;
}

void BGeneralEventSubscriber::eventTrigger(BPlayerID playerID)
{
   if(mFired == true && !mUseCount) return;
   
   if(mCheckPlayer)
   {
      if(playerID != mPlayer)
         return;
   }
   for(uint i=0; i< mEventFilters.size(); i++)
   {
      BEventFilterBase* pFilter = mEventFilters[i];
      bool res = false;
	   if(pFilter->isType(BEventFilterBase::cFilterTypeGameState))
	   {
         BEventFilterGameState* pFilterGameState = static_cast<BEventFilterGameState*>(pFilter);
		   res = pFilterGameState->testGameState();
	   }
      if(res == false)
         return;  
   }
   //queue fire...
   gGeneralEventManager.queueSubscriberFired(mID);
}

void BGeneralEventSubscriber::eventTrigger(BPlayerID playerID, BEntityID sourceEntity, BEntityID targetEntity)
{
   if(mFired == true && !mUseCount) return;

   if(mCheckPlayer)
   {
      if(playerID != mPlayer)
         return;
   }

   for(uint i=0; i< mEventFilters.size(); i++)
   {
      BEventFilterBase* pFilter = mEventFilters[i];
      bool res = false;
            
      if((pFilter->mParamID == BEventParameters::cSource)
      && (pFilter->isType(BEventFilterBase::cFilterTypePlayer)))
      {
         //No player filter type yet
      }
      else if((pFilter->mParamID == BEventParameters::cSource)
           && (pFilter->isType(BEventFilterBase::cFilterTypeEntity)))
      {
         if(sourceEntity == cInvalidObjectID) 
         {
            res = false;  //cInvalidObjectID can be valid, but this should still cause us to fail the filter
         }
         else
         {
            BEventFilterEntity* pEnityFilter = static_cast<BEventFilterEntity*>(pFilter);
            res = pEnityFilter->testEntity(sourceEntity);
         }
      }    
      else if((pFilter->mParamID == BEventParameters::cTarget)
           && (pFilter->isType(BEventFilterBase::cFilterTypeEntity)))
      {
         if(targetEntity == cInvalidObjectID) 
         {
            res = false;  //cInvalidObjectID can be valid, but this should still cause us to fail the filter
         }
         else
         {
            BEventFilterEntity* pEnityFilter = static_cast<BEventFilterEntity*>(pFilter);
            res = pEnityFilter->testEntity(targetEntity);
         }
      } 
		else if(pFilter->isType(BEventFilterBase::cFilterTypeGameState))
		{
         BEventFilterGameState* pFilterGameState = static_cast<BEventFilterGameState*>(pFilter);
			res = pFilterGameState->testGameState();
		}
      if(res == false)
         return;      
   }

   gGeneralEventManager.queueSubscriberFired(mID);
}

void BGeneralEventSubscriber::eventTrigger(BPlayerID playerID, BEntityID sourceEntity, long value)
{
   if(mFired == true && !mUseCount) return;

   if(mCheckPlayer)
   {
      if(playerID != mPlayer)
         return;
   }

   for(uint i=0; i< mEventFilters.size(); i++)
   {
      BEventFilterBase* pFilter = mEventFilters[i];
      bool res = false;
            
      if((pFilter->mParamID == BEventParameters::cSource)
      && (pFilter->isType(BEventFilterBase::cFilterTypePlayer)))
      {
         //No player filter type yet
      }
      else if((pFilter->mParamID == BEventParameters::cSource)
           && (pFilter->isType(BEventFilterBase::cFilterTypeEntity)))
      {
         if(sourceEntity == cInvalidObjectID) 
         {
            res = false;  //cInvalidObjectID can be valid, but this should still cause us to fail the filter
         }
         else
         {
            BEventFilterEntity* pEnityFilter = static_cast<BEventFilterEntity*>(pFilter);
            res = pEnityFilter->testEntity(sourceEntity);
         }
      }    
      else if(pFilter->isType(BEventFilterBase::cFilterTypeType))
      {
         BEventFilterType* pFilterType = static_cast<BEventFilterType*>(pFilter);
         res = pFilterType->testType(value);
      } 
      else if(pFilter->isType(BEventFilterBase::cFilterTypeNumeric))
      {
         BEventFilterNumeric* pFilterNum = static_cast<BEventFilterNumeric*>(pFilter);
         res = pFilterNum->testInt((int)value);
      } 
		else if(pFilter->isType(BEventFilterBase::cFilterTypeGameState))
		{
         BEventFilterGameState* pFilterGameState = static_cast<BEventFilterGameState*>(pFilter);
			res = pFilterGameState->testGameState();
		}
      if(res == false)
         return;      
   }

   gGeneralEventManager.queueSubscriberFired(mID);
}
void BGeneralEventSubscriber::eventTrigger(BPlayerID playerID, BEntityIDArray* pSources, BEntityID sourceEntity, BEntityIDArray* pTargets, BEntityID targetEntity)
{
   if(mFired == true && !mUseCount) return;

   if(mCheckPlayer)
   {
      if(playerID != mPlayer)
         return;
   }

   for(uint i=0; i< mEventFilters.size(); i++)
   {
      BEventFilterBase* pFilter = mEventFilters[i];
      bool res = false;
            
      if((pFilter->mParamID == BEventParameters::cSource)
      && (pFilter->isType(BEventFilterBase::cFilterTypePlayer)))
      {
         //No player filter type yet
      }
      else if((pFilter->mParamID == BEventParameters::cSource)
           && (pFilter->isType(BEventFilterBase::cFilterTypeEntity)))
      {
         if(sourceEntity == cInvalidObjectID) 
         {
            res = false;  //cInvalidObjectID can be valid, but this should still cause us to fail the filter
         }
         else
         {
            BEventFilterEntity* pEnityFilter = static_cast<BEventFilterEntity*>(pFilter);
            res = pEnityFilter->testEntity(sourceEntity);
         }
      }    
      else if((pFilter->mParamID == BEventParameters::cTarget)
           && (pFilter->isType(BEventFilterBase::cFilterTypeEntity)))
      {
         if(targetEntity == cInvalidObjectID) 
         {
            res = false;  //cInvalidObjectID can be valid, but this should still cause us to fail the filter
         }
         else
         {
            BEventFilterEntity* pEnityFilter = static_cast<BEventFilterEntity*>(pFilter);
            res = pEnityFilter->testEntity(targetEntity);
         }
      } 

      else if((pFilter->mParamID == BEventParameters::cSource)
           && (pFilter->isType(BEventFilterBase::cFilterTypeEntityList)))
      {

         BEventFilterEntityList* pEnityFilter = static_cast<BEventFilterEntityList*>(pFilter);
         res = pEnityFilter->testEntityList(pSources);
      }    
      else if((pFilter->mParamID == BEventParameters::cTarget)
           && (pFilter->isType(BEventFilterBase::cFilterTypeEntityList)))
      {

         BEventFilterEntityList* pEnityFilter = static_cast<BEventFilterEntityList*>(pFilter);
         res = pEnityFilter->testEntityList(pTargets);
      } 
		else if(pFilter->isType(BEventFilterBase::cFilterTypeGameState))
		{
         BEventFilterGameState* pFilterGameState = static_cast<BEventFilterGameState*>(pFilter);
			res = pFilterGameState->testGameState();
		}
      if(res == false)
         return;      
   }

   gGeneralEventManager.queueSubscriberFired(mID);


}


//default/all
//filter per type
//
//onther filter types
//entities??
//camera
//locations

//how to make it transparant/ foolproof
void BGeneralEventSubscriber::eventTrigger(BPlayerID playerID, BCamera* pCamera)
{
   if(mFired == true && !mUseCount) return;

   if(mCheckPlayer)
   {
      if(playerID != mPlayer)
         return;
   }

   for(uint i=0; i< mEventFilters.size(); i++)
   {
      bool res = false;

      BEventFilterBase* pFilter = mEventFilters[i];
            
      if((pFilter->mParamID == BEventParameters::cSource)
         && (pFilter->isType(BEventFilterBase::cFilterTypeCamera)))
      {
         BEventFilterCamera* pCameraFilter = static_cast<BEventFilterCamera*>(pFilter);
         res = pCameraFilter->testCamera(pCamera);         
      }
		else if(pFilter->isType(BEventFilterBase::cFilterTypeGameState))
		{
         BEventFilterGameState* pFilterGameState = static_cast<BEventFilterGameState*>(pFilter);
			res = pFilterGameState->testGameState();
		}
      if(res == false)
         return;   
   }
   
   gGeneralEventManager.queueSubscriberFired(mID);

}

////==============================================================================
//// BGeneralEventSubscriber::clearFilters
////==============================================================================
void BGeneralEventSubscriber::clearFilters()
{
   int numEventFilters = mEventFilters.getNumber();
   for (int i = (numEventFilters - 1); i >= 0; i--)
   {
      BEventFilterBase::releaseFilter(mEventFilters[i]);
   }
   mEventFilters.clear();
}



////==============================================================================
//// BGeneralEvent::addSubscriber
////==============================================================================
void BGeneralEvent::addSubscriber(BGeneralEventSubscriber* pSubscriber)
{
   mGeneralEventSubscribers.add(pSubscriber); 
}

////==============================================================================
//// BGeneralEvent::removeSubscriber
////==============================================================================
void BGeneralEvent::removeSubscriber(BGeneralEventSubscriber* pSubscriber)
{ 
   mGeneralEventSubscribers.remove(pSubscriber);
}

//==============================================================================
// BEventFilterBase::releaseFilter
//==============================================================================
void BEventFilterBase::releaseFilter(BEventFilterBase *pEventFilter)
{
   BASSERT(pEventFilter);
   BYTE filterType = pEventFilter->getType();
   switch(filterType)
   {
   case cFilterTypeEntity: 
      {
         //BEventFilterEntity* pASDF = reinterpret_cast<BEventFilterEntity*>(pEventFilter);
         //BEventFilterEntity* pASDF2 = (BEventFilterEntity*)(pEventFilter);
         BEventFilterEntity::releaseInstance((BEventFilterEntity*)(pEventFilter)); 
         break; 
      }
   case cFilterTypeEntityList: { BEventFilterEntityList::releaseInstance((BEventFilterEntityList*)(pEventFilter)); break; }
   case cFilterTypeLocation: { BEventFilterLocation::releaseInstance((BEventFilterLocation*)(pEventFilter)); break; }
   case cFilterTypeCamera: { BEventFilterCamera::releaseInstance((BEventFilterCamera*)(pEventFilter)); break; }
   case cFilterTypeGameState: { BEventFilterGameState::releaseInstance((BEventFilterGameState*)(pEventFilter)); break; }
   case cFilterTypeType: { BEventFilterType::releaseInstance((BEventFilterType*)(pEventFilter)); break; }
   case cFilterTypeNumeric: { BEventFilterNumeric::releaseInstance((BEventFilterNumeric*)(pEventFilter)); break; }
   default: BASSERTM(false, "You are trying to release an instance of an unknown event filter type.  Botched.");
   }
}


//==============================================================================
// BEventFilterEntity::testEntity
//==============================================================================

bool BEventFilterEntity::testEntity(BEntityID id)
{
   bool res = false;
   if(id.getType() == BEntity::cClassTypeUnit)
   {
      BUnit *pUnit = gWorld->getUnit(id);
      if(pUnit)
         res = mFilterSet.testUnit(pUnit);
   }
   else if(id.getType() == BEntity::cClassTypeSquad)
   {
      BSquad *pSquad = gWorld->getSquad(id);
      if(pSquad)
         res = mFilterSet.testSquad(pSquad);
   }
   else if(id.getType() == BEntity::cClassTypeProjectile)
   {
      BProjectile *pProj = gWorld->getProjectile(id);
      if(pProj)
         res = mFilterSet.testProjectile(pProj);
   }
   return res;

}

bool BEventFilterCamera::testCamera(BCamera* pCamera)
{  
   bool positiveResult = true;
   if(mbInvert)
      positiveResult = false;


   BVector intersectionPt;
   BVector dir(pCamera->getCameraDir());
   bool hit=gTerrainSimRep.rayIntersects(pCamera->getCameraLoc(), dir, intersectionPt);
   if (!hit)
      return !positiveResult;

   BVector pos = intersectionPt;   
   
   if(mbFilterLocation == true)
   {
      float dist = pos.distance(mLocation);

      if(dist < mViewAreaRadius)
      {
         return positiveResult;
      }
   }

   if(mbFilterUnit == true)
   {      
//-- FIXING PREFIX BUG ID 1517
      const BEntity* pUnit = gWorld->getEntity(mUnit);
//--
      if(pUnit)
      {                   
         float dist = pos.distance(pUnit->getPosition());

         if(dist < mViewAreaRadius)
         {
            return positiveResult;
         }
      }      
   }
   return !positiveResult;
}


////==============================================================================
//// BEventFilterEntity::testEntity
////==============================================================================
//
bool BEventFilterEntityList::testEntityList(BEntityIDArray *pIds)
{
   bool res = false;

   if(pIds == NULL || pIds->size() <= 0)
      return false;

   const BEntityIDArray& source = *pIds;
   BEntityIDArray passedSquads;
   BEntityIDArray failedSquads;

   uint type = pIds->get(0).getType(); 
   if(type == BEntity::cClassTypeUnit)
   {
      mFilterSet.filterUnits(source, &passedSquads, &failedSquads, NULL);
   }
   else if(type == BEntity::cClassTypeSquad)
   {
      mFilterSet.filterSquads(source, &passedSquads, &failedSquads, NULL);
   }
   else if(type == BEntity::cClassTypeProjectile)
   {
      mFilterSet.filterProjectiles(source, &passedSquads, &failedSquads, NULL);
   }

   if(mAllMustMatch)
   {
      if(passedSquads.size() == 0 || failedSquads.size() > 0)
      {
         return false;
      }
   }
   
   if(mMinimumMatchesRequired <= passedSquads.size())
   {
      return true;
   }

   return res;

}


////==============================================================================
//// BEventFilterGameState::testGameState  WARNING ONLY RETURN EARLY WITH VALUE = error
////==============================================================================
bool BEventFilterGameState::testGameState()
{
	bool res = false;
   bool error = false;

   //only inverts non errors?
   if(mInvert == true)
   {
      res = !res;
   }

	BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
	if (!pPlayer)
		return (error);
//-- FIXING PREFIX BUG ID 1519
	const BUser* pUser = pPlayer->getUser();
//--
	if (!pUser)
		return (error);

   //WARNING ONLY RETURN EARLY WITH VALUE = error
	if(mGameState == BGameStateDefinitions::cSquadsSelected)
	{
		const BEntityIDArray &ugIDs = pUser->getSelectionManager()->getSelectedSquads();

		res = evaluateEntityList(&ugIDs);
	}
	else if(mGameState == BGameStateDefinitions::cHasSquads)
	{
		BEntityIDArray squadIDs;
      pPlayer->getSquads(squadIDs);
		res = evaluateEntityList(&squadIDs);
	}
	else if(mGameState == BGameStateDefinitions::cHasBuildings)
	{
		BObjectTypeID otidBuilding = gDatabase.getOTIDBuilding();

		//needs default filter or name change
		BEntityIDArray unitIDs;
      pPlayer->getUnitsOfType(otidBuilding,unitIDs);
		res = evaluateEntityList(&unitIDs);
	}
	else if(mGameState == BGameStateDefinitions::cHasResources)
	{
		res = pPlayer->checkCost(&mCost);
	}
	else if(mGameState == BGameStateDefinitions::cHasTech)
	{
	   BTechTree *pTechTree = pPlayer->getTechTree();
		if (!pTechTree)
			return (error);
		long actualTechStatus = pTechTree->getTechStatus(mTechID, mTechID);
		res = (BTechTree::cStatusActive == actualTechStatus);
	}
	else if(mGameState == BGameStateDefinitions::cGameTime)
	{
		//gWorld->getGametime()
	}

   if(mInvert == true)
   {
      res = !res;
   }

	return res;
}

////==============================================================================
//// BEventFilterGameState::evaluateEntityList
////==============================================================================
bool BEventFilterGameState::evaluateEntityList(const BEntityIDArray *pIds )
{
   bool res = false;

   if(pIds == NULL || pIds->size() <= 0)
      return false;

   const BEntityIDArray& source = *pIds;
   BEntityIDArray passedSquads;
   BEntityIDArray failedSquads;

   uint type = pIds->get(0).getType(); 
   if(type == BEntity::cClassTypeUnit)
   {
      mFilterSet.filterUnits(source, &passedSquads, &failedSquads, NULL);
   }
   else if(type == BEntity::cClassTypeSquad)
   {
      mFilterSet.filterSquads(source, &passedSquads, &failedSquads, NULL);
   }
   else if(type == BEntity::cClassTypeProjectile)
   {
      mFilterSet.filterProjectiles(source, &passedSquads, &failedSquads, NULL);
   }

   if(mAllMustMatch)
   {
      if(passedSquads.size() == 0 || failedSquads.size() > 0)
      {
         return false;
      }
   }
   
   if(mMinimumMatchesRequired <= passedSquads.size())
   {
      return true;
   }

   return res;
}

bool BEventFilterNumeric::testInt(int value)
{
   int val1 = value;
   int val2 = mInteger;
   switch(mCompareType)
   {
   case Math::cNotEqualTo:
      return (val1 != val2);
   case Math::cLessThan:
      return (val1 < val2);
   case Math::cLessThanOrEqualTo:
      return (val1 <= val2);
   case Math::cEqualTo:
      return (val1 == val2);
   case Math::cGreaterThanOrEqualTo:
      return (val1 >= val2);
   case Math::cGreaterThan:
      return (val1 > val2);
   default:
      BASSERT(false);
      return (false);
   }
   return false;
}
bool BEventFilterNumeric::testFloat(float value)
{
    


   return false;
}
bool BEventFilterType::testType(long type)
{
   bool res = false;

   if(mObjectType != -1)
   {
      //TODO needs a real comparison
      if(mProtoSquad == type)
         res = true;
   }
   if(mProtoSquad != -1)
   {
      if(mProtoSquad == type)
         res = true;
   }
   if(mProtoObject != -1)
   {
      if(mProtoObject == type)
         res = true;
   }
   if(mTech != -1) 
   {
      if(mTech == type)
         res = true;
   }


   if(mInvert == true)
   {
      res = !res;
   }

	return res;
}
void BEventFilterType::onAcquire()
{
   mInvert = false; 
   mObjectType = -1;
   mProtoSquad = -1;
   mProtoObject = -1;
   mTech = -1;   
}

////Old versions
////
/////////////////
//////==============================================================================
////// GeneralEventManager.h
//////
////// 
//////
////// Copyright (c) 2006 Ensemble Studios
//////==============================================================================
////
////// Includes
////#include "common.h"
//////#include "ai.h"
////#include "generaleventmanager.h"
//////#include "player.h"
//////#include "world.h"
//////#include "game.h"
//////#include "SegIntersect.h"
//////#include "usermanager.h"
//////#include "user.h"
////#include "command.h"
////#include "commands.h"
////#include "commandmanager.h"
////#include "mpSimDataObject.h"
////#include "world.h"
////
////
////
////IMPLEMENT_FREELIST(BGeneralEventSubscriber, 5, &gSimHeap);
////
////
////BGeneralEventManager gGeneralEventManager;
////
//////==============================================================================
////// Constants
//////==============================================================================
////
////
////inline BGeneralEventSubscriberID makeSubscriberID(uint index, BGeneralEventID eventID)
////{
////   return (index << 16) + eventID;
////}
////inline uint getSubscriberIndex(BGeneralEventSubscriberID id)
////{
////   return  (id  & 0xffff0000) >> 16;
////}
////inline BGeneralEventID getSubscriberEventID(BGeneralEventSubscriberID id)
////{
////   return (id & 0x0000ffff);
////}
////
////
////////==============================================================================
//////// BGeneralEventManager::newSubscriber
////////==============================================================================
////BGeneralEventSubscriber* BGeneralEventManager::newSubscriber(BGeneralEventID id)
////{
////   BGeneralEvent& thisEvent = mGeneralEvents[id];
////   BGeneralEventSubscriber* pSubscriber = BGeneralEventSubscriber::getInstance();   
////   thisEvent.addSubscriber(pSubscriber);
////   pSubscriber->mID = makeSubscriberID(thisEvent.getSubscriberCount()-1, id); 
////   return pSubscriber;
////}
////
////////==============================================================================
//////// BGeneralEventManager::getSubscriber
////////==============================================================================
////BGeneralEventSubscriber* BGeneralEventManager::getSubscriber(BGeneralEventSubscriberID id)
////{
////   uint index = getSubscriberIndex(id);
////   BGeneralEventID eventId = getSubscriberEventID(id);
////   return mGeneralEvents[eventId].getSubscriber(index);
////}
////
////////==============================================================================
//////// BGeneralEventManager::QueueSubscriberFired
////////==============================================================================
////void BGeneralEventManager::QueueSubscriberFired( BGeneralEventSubscriberID id)
////{
////   mSubscriberFiredQueue.add(id);
////}
////
////////==============================================================================
//////// BGeneralEventManager::SubscriberFired
////////==============================================================================
////void BGeneralEventManager::SubscriberFired( BGeneralEventSubscriberID id)
////{
////   BGeneralEventSubscriber* sub = getSubscriber(id);
////   if(sub != NULL)
////   {
////      sub->setFired();
////   }
////}
////
//////TODO .... who sends/recieves... ...????
////
////////==============================================================================
//////// BGeneralEventManager::SubscriberFired
////////==============================================================================
////void BGeneralEventManager::FlushQueue()
////{
////   if(mSubscriberFiredQueue.size() == 0)
////      return;
////
////   //PlayerID playerID = 1;
////   long playerID = 1;
////
////   BGeneralEventCommand* pCommand = (BGeneralEventCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGeneralEvent);
////
////   if (!pCommand)
////   {
////      return;
////   }
////   
////   pCommand->setSenderType(BCommand::cPlayer);
////   pCommand->setSenders(1, &playerID);
////   pCommand->setRecipientType(BCommand::cGame);
////   pCommand->setType(BGeneralEventCommand::cEventFired);
////
////   for(uint i=0; i< mSubscriberFiredQueue.size(); i++)
////   {
////      pCommand->addSubscriber(mSubscriberFiredQueue[i]);
////   }
////
////   gWorld->getCommandManager()->addCommandToExecute(pCommand);
////
////   mSubscriberFiredQueue.clear();
////
////}
////////==============================================================================
//////// BGeneralEvent::eventTrigger
////////==============================================================================
////
////
////void BGeneralEvent::eventTrigger(long targetPlayerID, long targetProtoUnitID, long targetProtoSquadID)
////{
////   for(uint i=0; i< mGeneralEventSubscribers.size(); i++)
////   {
////      mGeneralEventSubscribers[i]->eventTrigger(targetPlayerID, targetProtoUnitID, targetProtoSquadID);
////   }
////}
////
//////template<class T1, class T2> void BGeneralEvent::eventTrigger(T1* v1, T2* v2)
//////{
//////   for(uint i=0; i< mGeneralEventSubscribers.size(); i++)
//////   {
//////      mGeneralEventSubscribers[i]->eventTrigger<T1,T2>(v1, v2);
//////   }
//////}
////
////////==============================================================================
//////// BGeneralEventSubscriber::eventTrigger  
////////==============================================================================
////
//////void BGeneralEventSubscriber::eventTrigger(long targetPlayerID, long targetProtoUnitID, long targetProtoSquadID)
//////{
//////   if(mFired == true) return;
//////   
//////   if(mCheckPlayer)
//////   {
//////      if(targetPlayerID != mPlayer)
//////         return;
//////   }
//////   if(mCheckProtoObject)
//////   {
//////      if(targetProtoUnitID != mProtoObject)
//////         return;
//////   }
//////   if(mCheckProtoSquad)
//////   {
//////      if(targetProtoSquadID != mProtoSquad)
//////         return;
//////   }
//////   
//////   //mFired = true;
//////
//////   //queue fire...
//////   gGeneralEventManager.QueueSubscriberFired(mID);
//////}
////
////
////void BGeneralEventSubscriber::eventTrigger(BPlayerID playerID)
////{
////   if(mFired == true) return;
////   
////   if(mCheckPlayer)
////   {
////      if(playerID != mPlayer)
////         return;
////   }
////   //mFired = true;
////
////   //queue fire...
////   gGeneralEventManager.QueueSubscriberFired(mID);
////}
////
////
////   //BEventFilterBase* pFilter = pSubscription->addFilter2();
////   //pFilter->mParamID = makeParamID(cSource, cPlayer);
////   //pFilter->mOperator = BEventFilterBase::cEquals;
////   //pFilter->mOtherValue1 = (DWORD)(getVar(cPlayer)->asPlayer()->readVar());
////
////   //BEventFilterBase* pFilter2 = pSubscription->addFilter2();
////   //pFilter2->mParamID = makeParamID(cSource, cEntity);
////   //pFilter2->mProp1 = cEntity_ProtoSquad;
////   //pFilter2->mOperator = BEventFilterBase::cEquals;
////   //pFilter2->mOtherValue1 = (DWORD)(getVar(cProtoSquad)->asProtoSquad()->readVar());
////
////
//////   static BPropertyRes masterConvert(BPropertyID id, long& in, long& out); //variant
////
////
////void BGeneralEventSubscriber::eventTrigger(BPlayerID playerID, BEntityID sourceEntity, BEntityID targetEntity)
////{
////   if(mFired == true) return;
////
////   if(mCheckPlayer)
////   {
////      if(playerID != mPlayer)
////         return;
////   }
////
////   for(uint i=0; i< mEventFilters.size(); i++)
////   {
////      BEventFilterBase* pFilter = mEventFilters[i];
////      bool res = false;
////            
////      if(pFilter->mParamID == BEventFilterBase::makeParamID(cSource, cPlayer))
////      {
////
////      }
////      else if(pFilter->mParamID == BEventFilterBase::makeParamID(cSource, cEntity))
////      {
////         //Box value?
////         BTriggerVarEntity* sourceEntityVar = BTriggerVarEntity::getInstance();
////         sourceEntity->writeVar(sourceEntity);
////
////         /////////////////////////////////
////         BTriggerVar* pLeft;
////         //follow propery chain////////////////
////         //...make template versions to avoid boxing?
////         //if(ObjectProperty<BEntityID>(pFilter->mProp1, sourceEntityVar, mNewValue))
////         if(ObjectProperty(pFilter->mProp1, sourceEntityVar, mNewValue))
////         {
////            pLeft = mNewValue;
////            BTriggerVar* mNewValue2;
////            if(ObjectProperty(pFilter->mProp2, mNewValue, mNewValue2))
////            {
////               pLeft = mNewValue2;
////            }
////         }
////         ////////////////////////////////
////
////          
////         //Equals... or special function.
////
////         //COMPARE...
////         if(Compare(pFilter->mOperator, pLeft, pFilter->mValue1, pFilter->mValue2))
////         {
////            //map inputs to condition..
////
////            //check contition true/false.
////
////            //is point in area?
////            //is point in box?
////
////            //pLeft->            
////            //? condition id?
////            res = true;
////         }
////
////         BTriggerVarEntity::releaseInstance(sourceEntityVar);
////      }    
////
////      if(res == false)
////         return;      
////   }
////
////   //trivial case
////   //mFired = true;
////
////   gGeneralEventManager.QueueSubscriberFired(mID);
////
////
////
////}
////
////
//////template< class T> BEventFilterBase* BGeneralEventSubscriber::addFilter() 
//////{ 
//////   BEventFilterBase* pFilter = new BEventFilter<T>(); 
//////   mEventFilters.add(pFilter); 
//////   return pFilter; 
//////}
//////
//////
//////template<class T1, class T2> void BGeneralEventSubscriber::eventTrigger(T1* v1, T2* v2)
//////{
//////   if(mFired == true) return;
//////   for(uint i=0; i< mEventFilters.size(); i++)
//////   {
//////      BEventFilter pFilter = mEventFilters[i];
//////      bool res;
//////   
////////......support derivative values.... how to expand them?????
//////      
//////      int paramId = pFilter->mParamID;
//////      int convertID = 0;
//////      if(paramId > 0xff)
//////      {
//////         convertID = (id  & 0xffff0000) >> 16;
//////         paramID = (id & 0x0000ffff);
//////
//////
//////      }
//////      //DO convert????
//////
//////      if(paramId == 1)
//////      {
//////         if(convertID > 0)
//////         {
//////            //BGameObjectAccessProtocol::Property<>(convertID, v1);
//////         }  
//////         
//////         pFilter->Compare<T1>(convertID, v1);
//////
//////         //res = pFilter->Compare<T1>(v1);
//////      }
//////      else if(paramId == 2)
//////      {
//////         
//////         pFilter->Compare<T2>(convertID, v2);
//////
//////
//////         //res = pFilter->Compare<T2>(v2);
//////      }
//////
//////      //else if(pFilter->mParamID == 50)
//////      //{
//////      //   //void* val = Convert[50]->convert<T2>(v2);
//////      //   
//////      //   //compare .... ->  how to get type back out.
//////      //   //a
//////      //   
//////      //   //compare<T2>(v2, mOtherValue1); //??
//////      //   res = compare<T2>(v2, pFilter); //??
//////      //   
//////      //   //res = pFilter->Compare<T2>(v2);
//////      //}
//////
//////      if(res == false)
//////         return;      
//////   }
//////
//////   
//////
//////   //trivial case
//////   mFired = true;
//////}
////
////
////
//////template<class R>
//////template<class T> 
//////bool BEventFilter<R>::Compare(T input)
//////{
//////   return ((T)(*mOtherValue1) == input); //?  bit compare
//////
//////   return true;
//////}
//////
//////
//////template<class R>
//////bool BEventFilter<R>::CompareInner(R input)
//////{
//////   return ((T)(*mOtherValue1) == input); 
//////
//////   return true;
//////}
//////
//////
//////template<class R>
//////template<class T> 
//////bool BEventFilter<R>::Compare(BPropertyID i, T input)
//////{
//////   int convertID = 3;
//////
//////   R val2;
//////   if(BGameObjectAccessProtocol::Property2(convertID, input, val2))
//////   {
//////      //  now we can compare val2  to   (R)mOtherValue1
//////      
//////   }
//////   return true;
//////}
////
//////template<>
//////template<class T> 
//////bool BEventFilter<BVector>::Compare(BPropertyID i, T input)
//////{
//////   int convertID = 3;
//////
//////   BVector val2;
//////   if(BGameObjectAccessProtocol::Property2(convertID, input, val2))
//////   {
//////      //  now we can compare val2  to   (R)mOtherValue1
//////      
//////   }
//////   return true;
//////}
////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////
////
//////Abstract Base:
//////template<class T> bool BEventFilterBase::Compare(T input)
//////{
//////   return ((T)(*mOtherValue1) == input); //?  bit compare
//////
//////   return true;
//////}
////
//////template<class R> 
//////template<class T> 
//////bool BEventFilter<R>::Compare3(T input)
//////{
//////   return ((T)(*mOtherValue1) == input); //?  bit compare
//////
//////   return true;
//////}
////
//////Try compare with in/out. ...just cast R
//////template<class R>
//////bool BEventFilter<R>::Compare2()
//////{
//////   //R adsf;
//////   int convertID = 3;
//////   R value = BGameObjectAccessProtocol::Property<R>(convertID, v1);
//////
//////   //R val2;
//////   //BGameObjectAccessProtocol::Property2(convertID, input ,val2); 
//////
//////   //if(BGameObjectAccessProtocol::Property(v1
//////   
//////   return BEventFilterBase::Compare<R>()
//////
//////   return true;
//////}
////
//////
//////template<> bool BEventFilter<BPlayerID>::Compare2()
//////{
//////   //R adsf;
//////   return true;
//////}
////
//////   bool,   in, out  ...
////
//////template<class T> static T BGameObjectAccessProtocol::Property(BPropertyID id, BEntityID prop)
//////{
//////
//////   
//////}
//////template<> static BVector BGameObjectAccessProtocol::Property(BPropertyID id, BEntityID prop)
//////{
//////   BVector v;
//////   return v;
//////}
////
////BPropertyRes BGameObjectAccessProtocol::Property2(BPropertyID id, long in, long &out)
////{
////   if(id == cBEntityID_ProtoSquadID)
////   {
////
////
////   }
////   return cFail;
////}
////
////
////BPropertyRes BGameObjectAccessProtocol::Property2(BPropertyID id, long in, BVector &out)
////{
////   if(id == cBEntityID_Location)
////   {
////
////
////   }
////   return cFail;
////}
////
//////template<class T2> static T2 BGameObjectAccessProtocol::Property<BPlayerID, T2> (BPropertyID id, BPlayerID prop)
//////{
//////   T2 asdf;
//////
//////
//////   return asdf;
//////}
//////template<class T1, class T2> static T2 BGameObjectAccessProtocol::Property(BPropertyID id, T1 prop)
//////{
//////   T2 asdf;
//////
//////   return asdf;
//////}
////
////
////
//////template<> bool BEventFilter<BProtoSquadID>::Compare2()
//////{
//////   //R adsf;
//////   return true;
//////}
////
//////specialization for more operators...
////
//////expand first then no-op them.?
////
//////template<> bool BEventFilterBase::Compare<BVector>(BVector input)
//////{
//////   //if(mOperator == BVector.ASDF)
//////   //{
//////   //   //specialize?
//////   //}
//////   //else
//////   //{
//////   //   //non specialized..?
//////   //}
//////
//////   return true;
//////}
////
////
////
////////==============================================================================
//////// BGeneralEvent::addSubscriber
////////==============================================================================
////void BGeneralEvent::addSubscriber(BGeneralEventSubscriber* pSubscriber)
////{
////   mGeneralEventSubscribers.add(pSubscriber); 
////}
////
////////==============================================================================
//////// BGeneralEvent::removeSubscriber
////////==============================================================================
////void BGeneralEvent::removeSubscriber(BGeneralEventSubscriber* pSubscriber)
////{ 
////   mGeneralEventSubscribers.remove(pSubscriber);
////}
////
