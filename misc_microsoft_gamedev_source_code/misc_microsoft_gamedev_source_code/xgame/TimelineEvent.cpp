//============================================================================
// TimelineEvent.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "TimelineEvent.h"

//==============================================================================
//==============================================================================
BTimelineEvent::BTimelineEvent():
mStatus(cStatusLocked),
mIsNew(false),
mTimelinePostition(0.0f)
{
   mTitle.set(L"");
   mDetailText.set(L"");
   mDateString.set(L"");
   mUnlockedImage.set("");
   mDetailImage.set("");
   mEventName.set("");
}


//==============================================================================
//==============================================================================
bool BTimelineEvent::loadXML(BXMLNode root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {

/*
      <Name>EventOne</Name>
         <Status>Unlocked</Status>
         <Position>10.0</Position>
         <Date>24 Aug 2533</Date>
         <Title>This is the title (loc me)</Title>
         <DetailText>This is detail text</DetailText>
         <UnlockedImage>img://art\ui\....</UnlockedImage>
      <DetailImage>img://art\ui\...</DetailImage>

*/

      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());
      if (name=="Name")
      {
         node.getText(mEventName);
      }
      else if (name=="Status")
      {
         BString temp;
         mStatus = cStatusLocked;
         node.getText(temp);
         if (temp=="Unlocked")
            mStatus=cStatusUnlocked;
      }
      else if (name=="Position")
      {
         node.getTextAsFloat(mTimelinePostition);
      }
      else if (name=="Date")
      {
         node.getText(mDateString);
      }
      else if (name=="Title")
      {
         node.getText(mTitle);
      }
      else if (name=="DetailText")
      {
         node.getText(mDetailText);
      }
      else if (name=="UnlockedImage")
      {
         node.getText(mUnlockedImage);
      }
      else if (name=="DetailImage")
      {
         node.getText(mDetailImage);
      }
   }
      
   return true;
}
