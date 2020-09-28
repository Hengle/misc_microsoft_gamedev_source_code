//==============================================================================
// objectAnimEventTagQueue.h
//
// Copyright (c) 2004-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// class BStoredAnimEvent
// This class stores an instance of a fired tag
//==============================================================================
class BStoredAnimEvent
{
public:
   BStoredAnimEvent():
      mpUserData(0),
      mUserDatLen(0)
   {

   }
   ~BStoredAnimEvent()
   {
      destroy();
   }
      
   BStoredAnimEvent(const BStoredAnimEvent& other) :
      mTag(other.mTag),
      mEventLocation(other.mEventLocation),
      mEventType(other.mEventType),
      mUserDatLen(other.mUserDatLen),
      mpUserData(NULL)
   {
      if (mUserDatLen)
      {
         mpUserData = new byte[mUserDatLen];
         memcpy(mpUserData, other.mpUserData, mUserDatLen);
      }
   }
   
   BStoredAnimEvent& operator= (const BStoredAnimEvent& rhs)
   {
      if (this != &rhs)
      {
         destroy();
         
         mTag = rhs.mTag;
         mEventLocation = rhs.mEventLocation;
         mEventType = rhs.mEventType;
         mUserDatLen = rhs.mUserDatLen;
         
         if (mUserDatLen)
         {
            mpUserData = new byte[mUserDatLen];
            memcpy(mpUserData, rhs.mpUserData, mUserDatLen);
         }         
      }
      
      return *this;
   }
   
   void destroy()
   {
      if (mpUserData)
      {
         delete [] mpUserData;
         mpUserData = NULL;
      }
   }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BProtoVisualTag   mTag;       //a copy of the event tag that occured

   BVector           mEventLocation;      //the object which spawned this event may no longer exist when this is fired
                                          //so we need to store the physical location in worldspace that it occured at

   int8              mEventType;          //eg cAnimEventAlphaTerrain

   uint              mUserDatLen;
   byte*             mpUserData; //we're responsible for deleting this
};

//==============================================================================
// class BObjectAnimEventItem
// This class stores per-object information to queue up past tags.
//==============================================================================
class BObjectAnimEventItem
{
public:
   BObjectAnimEventItem()
   {
      mStoredAnimEvents.setNumber(0);
   }
   ~BObjectAnimEventItem()
   {
      destroy();
   }
   
   BObjectAnimEventItem(const BObjectAnimEventItem& other) :
      mEntityID(0)
   {
      mStoredAnimEvents.resize(other.mStoredAnimEvents.getSize());
      for (uint i = 0; i < other.mStoredAnimEvents.getSize(); i++)
         mStoredAnimEvents[i] = new BStoredAnimEvent(*other.mStoredAnimEvents[i]);
   }
   
   BObjectAnimEventItem& operator= (const BObjectAnimEventItem& rhs)
   {
      if (this != &rhs)
      {
         destroy();
         
         mStoredAnimEvents.resize(rhs.mStoredAnimEvents.getSize());
         for (uint i = 0; i < rhs.mStoredAnimEvents.getSize(); i++)
            mStoredAnimEvents[i] = new BStoredAnimEvent(*rhs.mStoredAnimEvents[i]);
      }
      
      return *this;
   }
   
   void destroy()
   {
      for(uint i=0;i<mStoredAnimEvents.size();i++)
      {
         delete mStoredAnimEvents[i];
         mStoredAnimEvents[i] = NULL;
      }
      
      mStoredAnimEvents.clear();
   }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BEntityID                        mEntityID;     //Does a dopple have a different entity ID?

   BDynamicSimArray<BStoredAnimEvent*>  mStoredAnimEvents;
};

//==============================================================================
// class BStoredAnimEventManager
//==============================================================================
class BStoredAnimEventManager
{
   BStoredAnimEventManager(const BStoredAnimEventManager&);
   BStoredAnimEventManager& operator= (const BStoredAnimEventManager&);
   
public:
   BStoredAnimEventManager();
   ~BStoredAnimEventManager();

   void addEventTag(BEntityID objectID, int8 EventType, BVector  EventLocation, BProtoVisualTag* pTag, void* userData, uint userDatLen);
   void fireAllEventTags(BEntityID objectID, bool removeAfterFinished= true);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

private:
   BDynamicArray<BObjectAnimEventItem*> mObjects;
   
   void clearObjects();
};