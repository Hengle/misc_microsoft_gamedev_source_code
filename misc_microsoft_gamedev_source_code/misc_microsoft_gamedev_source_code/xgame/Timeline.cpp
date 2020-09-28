//==============================================================================
// achievementmanager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "Timeline.h"
#include "gamedirectories.h"
#include "usermanager.h"
#include "skullmanager.h"


//==============================================================================
//==============================================================================
BTimeline::BTimeline()
{
}

//==============================================================================
//==============================================================================
BTimeline::~BTimeline()
{
   removeEvents();
}


//==============================================================================
//==============================================================================
void BTimeline::update(float elapsedTime)
{
   // did we get rid of needing this method?
}


//==============================================================================
//==============================================================================
BTimelineEvent* BTimeline::getEvent(int i)
{
   if ( (i<0) || (i>=mEvents.getNumber()) )
      return NULL;

   return mEvents[i];
}


//==============================================================================
//==============================================================================
bool BTimeline::loadEvents()
{
   BXMLReader reader;
   if (!reader.load(cDirData, "HaloTimeline.xml"))
      return(false);   

   BXMLNode rootNode(reader.getRootNode());

   BASSERT(rootNode.getName() == "Timeline");

   //Create an achievement rule for each entry.
   for (long i=0; i < rootNode.getNumberChildren(); i++)
   {
      //Get child node.
      BXMLNode node(rootNode.getChild(i));

      if (node.getName().compare("LockedImage") == 0)
      {
         node.getText(mLockedImageURL);
      }
      else if (node.getName().compare("Event") == 0)
      {
         BTimelineEvent* pEvent = new BTimelineEvent();;

         // Load the achievement
         pEvent->loadXML(node);

         // check to see if the event is unlocked.
         if ((pEvent->getStatus() == BTimelineEvent::cStatusLocked) && (gUserManager.getPrimaryUser() != NULL))
         {
            bool unlocked = gCollectiblesManager.hasTimeLineEventBeenUnlocked(gUserManager.getPrimaryUser(), pEvent->getEventName());
            if (unlocked)
               pEvent->mStatus = BTimelineEvent::cStatusUnlocked;

            bool isNew = gCollectiblesManager.hasTimeLineEventBeenSeen(gUserManager.getPrimaryUser(), pEvent->getEventName());
            pEvent->setIsNew(isNew);
         }

         // Add an event to the list.
         mEvents.add(pEvent);
      }
   }

   return(true);
}

//=============================================================================
//=============================================================================
void BTimeline::removeEvents(void)
{
   for(int i=0; i<mEvents.getNumber(); i++)
      delete mEvents[i];
   mEvents.setNumber(0);
}

