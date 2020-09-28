//==============================================================================
// entityscheduler.cpp
//
// entityscheduler
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "common.h"
#include "entityscheduler.h"
#include "entity.h"
#include "world.h"


BEntityScheduler gEntityScheduler;
GFIMPLEMENTVERSION(BEntityScheduler, 2);


//==============================================================================
//==============================================================================
bool BScheduleQueue::processQueue()
{
   //-- Send notifies out for however many we're suppose to process per update
   long numToProcess = min(mProcessPerUpdate, mQueue.getNumber());
   for(long i = 0; i < mQueue.getNumber(); i++)
   {
      if(i < numToProcess)
      {
         notifyEntity(mQueue[i], mEventType);         
      }

      if((i + numToProcess) < mQueue.getNumber())
      {
         mQueue[i] = mQueue[i + numToProcess];
      }
   }

   //-- Shrink the queue
   mQueue.resize(mQueue.getNumber() - numToProcess);

   return true;
}

//==============================================================================
// BEntityScheduler::notifyEntity
//==============================================================================
void BScheduleQueue::notifyEntity(BEntityID id, long eventID)
{
   BEntity *pEntity = gWorld->getEntity(id);
   if(!pEntity)
      return;

   //-- Send the notify
   pEntity->notify(eventID, id, (DWORD)-1, 0);
}



//==============================================================================
//==============================================================================
BEntityScheduler::BEntityScheduler()
{
}

//==============================================================================
//==============================================================================
BEntityScheduler::~BEntityScheduler()
{
}

//==============================================================================
//==============================================================================
bool BEntityScheduler::init()
{
   //-- Squad AI Search Scheduler
   mScheduleQueue.mEventType=BEntity::cEventSquadAISearch;
   mScheduleQueue.mProcessPerUpdate = 5;

   return true;
}

//==============================================================================
//==============================================================================
bool BEntityScheduler::reset()
{
   mScheduleQueue.mQueue.clear();
   return true;
}

//==============================================================================
//==============================================================================
bool BEntityScheduler::addRequest(long eventID, BEntityID entityID)
{
   #ifndef BUILD_FINAL
   //-- Go through all our ID's and make sure we're not adding a dupe
   long numItems = mScheduleQueue.mQueue.getNumber();
   for(long i = 0; i < numItems; i++)
   {
      BASSERT(mScheduleQueue.mQueue[i] != entityID);
   }
   #endif

   //Queue it up.
   mScheduleQueue.mQueue.pushBack(entityID);

   return true;
}

//==============================================================================
//==============================================================================
bool BEntityScheduler::update()
{
   mScheduleQueue.processQueue();
   return true;
}

//==============================================================================
//==============================================================================
bool BEntityScheduler::save(BStream* pStream, int saveType) const
{
   GFWRITECLASS(pStream, saveType, mScheduleQueue);
   return true;
}

//==============================================================================
//==============================================================================
bool BEntityScheduler::load(BStream* pStream, int saveType)
{
   if (mGameFileVersion < 2)
   {
      int queueTotal = 0;
      GFREADVAR(pStream, int, queueTotal);
      GFVERIFYCOUNT(queueTotal, 20);
      for (int i=0; i<queueTotal; i++)
      {
         if (i < 1)
            GFREADCLASS(pStream, saveType, mScheduleQueue)
         else
            GFREADTEMPCLASS(pStream, saveType, BScheduleQueue)
      }
      int eventCount = 0;
      GFREADVAR(pStream, int, eventCount);
      GFVERIFYCOUNT(eventCount, 200);
      for (int i=0; i<eventCount; i++)
      {
         char val = -1;
         GFREADVAR(pStream, char, val);
      }
   }
   else
      GFREADCLASS(pStream, saveType, mScheduleQueue)
   
   return true;
}

//==============================================================================
//==============================================================================
bool BScheduleQueue::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BEntityID, mQueue, uint16, 5000);
   GFWRITEVAR(pStream, long, mEventType);
   GFWRITEVAR(pStream, long, mProcessPerUpdate);
   GFWRITEVAR(pStream, long, mProcessInstant);
   return true;
}

//==============================================================================
//==============================================================================
bool BScheduleQueue::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, BEntityID, mQueue, uint16, 5000);
   GFREADVAR(pStream, long, mEventType);
   GFREADVAR(pStream, long, mProcessPerUpdate);
   GFREADVAR(pStream, long, mProcessInstant);
   return true;
}
