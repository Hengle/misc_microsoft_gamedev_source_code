//============================================================================
// TimelineEvent.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 

class BTimelineEvent
{
public:
   enum
   {
      cStatusLocked,
      cStatusUnlocked,
   };

   BTimelineEvent();

   virtual bool loadXML(BXMLNode root);

   const float getPosition() const { return mTimelinePostition; }
   const int getStatus() const { return mStatus; }

   const BUString& getTitle() const { return mTitle; }
   const BUString& getDetailText() const  { return mDetailText; }
   const BUString& getDateString() const { return mDateString; }

   const BString& getUnlockedImage() const { return mUnlockedImage; }
   const BString& getDetailImage() const { return mDetailImage; }
   const BString& getEventName() const { return mEventName; }

   const bool isNew() const { return mIsNew; }
   void setIsNew(bool isNew) { mIsNew = isNew; }

   BUString    mDateString;
   BUString    mTitle;
   BUString    mDetailText;
   BString     mUnlockedImage;
   BString     mDetailImage;
   BString     mEventName;             // Used to look up in another system to determine if it's locked or not.

   int         mStatus;
   float       mTimelinePostition;
   bool        mIsNew:1;
};

