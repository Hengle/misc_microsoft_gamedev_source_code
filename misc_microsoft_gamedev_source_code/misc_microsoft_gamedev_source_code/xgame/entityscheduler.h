//==============================================================================
// entityscheduler.h
//
// entityscheduler manages actions
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once 

#include "entity.h"

//==============================================================================
//Forward Declarations
//==============================================================================

class BScheduleQueue
{
public:
   BScheduleQueue::BScheduleQueue() : mEventType(-1), mProcessPerUpdate(-1), mProcessInstant(0) {}

   bool processQueue(void);
   void notifyEntity(BEntityID id, long eventID);
   
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BDynamicSimArray<BEntityID>      mQueue;
   long                             mEventType;
   long                             mProcessPerUpdate;
   long                             mProcessInstant;
};



class BEntityScheduler
{
public:

   BEntityScheduler();
   virtual ~BEntityScheduler();
   
   bool                    init( void );
   bool                    reset( void );
   bool                    update( void );
   bool                    addRequest(long eventID, BEntityID entityID);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   BScheduleQueue          mScheduleQueue;
};

extern BEntityScheduler gEntityScheduler;