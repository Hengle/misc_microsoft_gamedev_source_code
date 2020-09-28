//============================================================================
//
//  objectAnimEventTagQueue.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================


//xgame
#include "common.h"
#include "world.h"
#include "objectAnimEventTagQueue.h"

GFIMPLEMENTVERSION(BStoredAnimEventManager, 2);

//============================================================================
// BStoredAnimEventManager::BStoredAnimEventManager
//============================================================================
BStoredAnimEventManager::BStoredAnimEventManager()
{

}

//============================================================================
// BStoredAnimEventManager::clearObjects
//============================================================================
void BStoredAnimEventManager::clearObjects()
{
   for(uint i=0;i<mObjects.size();i++)
   {
      delete mObjects[i];
      mObjects[i] = NULL;
   }
   
   mObjects.clear();
}

//============================================================================
// BStoredAnimEventManager::BStoredAnimEventManager
//============================================================================
BStoredAnimEventManager::~BStoredAnimEventManager()
{
   clearObjects();
}

//============================================================================
// BStoredAnimEventManager::addEventTag
//============================================================================
void BStoredAnimEventManager::addEventTag(BEntityID objectID, int8 EventType, BVector  EventLocation, BProtoVisualTag* pTag, void* userData, uint userDatLen)
{
   BObjectAnimEventItem* targetItem = NULL;
   //does this object already have a position in the list?
   for(uint i=0;i<mObjects.size();i++)
   {
      if(mObjects[i]->mEntityID == objectID)
      {
         targetItem = mObjects[i];
      }
   }

   //we don't exit in the list prior.. add us
   if(targetItem == NULL)
   { 
      mObjects.push_back(new BObjectAnimEventItem());
      targetItem = mObjects[mObjects.size()-1];

      targetItem->mEntityID = objectID;
   }

   BDEBUG_ASSERT(targetItem);

   //add our event to the owning container
   targetItem->mStoredAnimEvents.pushBack(new BStoredAnimEvent());
   BStoredAnimEvent* sae = targetItem->mStoredAnimEvents[targetItem->mStoredAnimEvents.size()-1];

   sae->mEventType = EventType;
   sae->mEventLocation = EventLocation;
   sae->mTag = (*pTag); //according to the operator, this will copy.
   if ((userData != NULL) && (userDatLen > 0))
   {
      sae->mUserDatLen = userDatLen;
      sae->mpUserData = new byte[userDatLen];
      BASSERT(sae->mpUserData);
      memcpy(sae->mpUserData, userData, userDatLen);
   }
   else
   {
      sae->mUserDatLen = 0;
      sae->mpUserData = NULL;
   }
}
//============================================================================
// BStoredAnimEventManager::fireAllEventTags
//============================================================================
void BStoredAnimEventManager::fireAllEventTags(BEntityID objectID, bool removeAfterFinished/*= true*/)
{
   BObjectAnimEventItem* targetItem = NULL;
   //does this object already have a position in the list?
   uint itemIndex =0;
   for(itemIndex=0;itemIndex<mObjects.size();itemIndex++)
   {
      BEntityID id = mObjects[itemIndex]->mEntityID;
      if(id == objectID)
      {
         targetItem = mObjects[itemIndex];
         break;
      }
   }
   //The item does not exist in our database.
   if(targetItem == NULL)
      return;


   for(uint i=0;i<targetItem->mStoredAnimEvents.size();i++)
   {
      BStoredAnimEvent* animEvent = targetItem->mStoredAnimEvents[i];
      
      gWorld->handleCachedAnimEvent(animEvent->mEventType, 
                                    &animEvent->mTag, 
                                    animEvent->mEventLocation, 
                                    animEvent->mpUserData);
   }

   if(removeAfterFinished)
   {
      delete targetItem;
      mObjects[itemIndex] = NULL;
      
      mObjects.removeIndex(itemIndex); //remove us from the queue
   }
}

//=============================================================================
// BStoredAnimEventManager::save
//=============================================================================
bool BStoredAnimEventManager::save(BStream* pStream, int saveType) const
{
   GFWRITECLASSPTRARRAY(pStream, saveType, BObjectAnimEventItem, mObjects, uint16, 2500);

   return true;
}

//=============================================================================
// BStoredAnimEventManager::load
//=============================================================================
bool BStoredAnimEventManager::load(BStream* pStream, int saveType)
{
   clearObjects();
   
   GFREADCLASSPTRARRAY(pStream, saveType, BObjectAnimEventItem, mObjects, uint16, 2500);

   return true;
}

//=============================================================================
// BObjectAnimEventItem::save
//=============================================================================
bool BObjectAnimEventItem::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BEntityID, mEntityID);
   GFWRITECLASSPTRARRAY(pStream, saveType, BStoredAnimEvent, mStoredAnimEvents, uint16, 10000);

   return true;
}

//=============================================================================
// BObjectAnimEventItem::load
//=============================================================================
bool BObjectAnimEventItem::load(BStream* pStream, int saveType)
{
   destroy();
   
   GFREADVAR(pStream, BEntityID, mEntityID);
   if (BStoredAnimEventManager::mGameFileVersion < 2)
      GFREADCLASSPTRARRAY(pStream, saveType, BStoredAnimEvent, mStoredAnimEvents, uint8, 50)
   else
      GFREADCLASSPTRARRAY(pStream, saveType, BStoredAnimEvent, mStoredAnimEvents, uint16, 10000)

   return true;
}

//=============================================================================
// BStoredAnimEvent::save
//=============================================================================
bool BStoredAnimEvent::save(BStream* pStream, int saveType) const
{
   GFWRITECLASS(pStream, saveType, mTag);
   GFWRITEVECTOR(pStream, mEventLocation);
   GFWRITEVAR(pStream, int8, mEventType);
   GFWRITEVAL(pStream, uint8, mUserDatLen);
   if (mUserDatLen > 0)
   {
      BASSERT(mpUserData);
      GFWRITEPTR(pStream, mUserDatLen, mpUserData);
   }

   return true;
}

//=============================================================================
// BStoredAnimEvent::load
//=============================================================================
bool BStoredAnimEvent::load(BStream* pStream, int saveType)
{
   destroy();
   
   GFREADCLASS(pStream, saveType, mTag);
   GFREADVECTOR(pStream, mEventLocation);
   GFREADVAR(pStream, int8, mEventType);
   GFREADVAL(pStream, uint8, uint, mUserDatLen);
   if (mUserDatLen > 0)
   {
      mpUserData = new byte[mUserDatLen];
      BASSERT(mpUserData);
      GFREADPTR(pStream, mUserDatLen, mpUserData);
   }
   else
   {
      mpUserData = NULL;
   }

   return true;
}
