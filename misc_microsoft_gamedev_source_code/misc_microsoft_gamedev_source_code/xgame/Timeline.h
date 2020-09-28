//==============================================================================
// timeline.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "TimelineEvent.h"


//==============================================================================
// BTimeline
//==============================================================================
class BTimeline
{
public:
   BTimeline();
   ~BTimeline();

   void        update(float elapsedTime);
   bool        loadEvents();

   const int   getNumberEvents() const { return mEvents.getNumber(); }
   BTimelineEvent* getEvent(int i); 

   const BString& getLockedImage() const { return mLockedImageURL; }

protected:

   void        removeEvents();

   BString     mLockedImageURL;

   BDynamicSimArray<BTimelineEvent*>   mEvents;
};
