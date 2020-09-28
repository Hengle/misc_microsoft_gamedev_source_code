//============================================================================
// UIListControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIScrollingListControl.h"

//==============================================================================
//==============================================================================
BUIScrollingListControl::BUIScrollingListControl( void ) : 
   mMaxItems(0),
   mViewSize(0),
   mFirstVisibleIndex(0)
{
   mWrap=false;
   mAlignment=eVertical;
   mSelectedIndex=0;
   mControlType.set("UIScrollingListControl");
   INIT_EVENT_MAP();
}

//==============================================================================
//==============================================================================
BUIScrollingListControl::~BUIScrollingListControl( void )
{
}

//==============================================================================
//==============================================================================
bool BUIScrollingListControl::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   // fixme - do we need to handle this or can I just override the next/prev calls below?
   if (command=="prev")
   {
      prev();
      return true;
      // fixme - bubble the event up too.
   }
   else if (command=="next")
   {
      next();
      // fixme - bubble the event up too.
      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
int BUIScrollingListControl::getNumVisibleControls()
{
   int count = 0;
   for (int i=mControls.getNumber()-1; i>=0; i--)
   {
      if (mControls[i]->isShown())
         count++;
   }

   return count;
}



//==============================================================================
//==============================================================================
int BUIScrollingListControl::getLastVisible()
{
   int index = 0;
   for (int i=mControls.getNumber()-1; i>=0; i--)
   {
      if (mControls[i]->isShown())
      {
         index = i;
         break;
      }
   }

   return index;
}

//==============================================================================
//==============================================================================
bool BUIScrollingListControl::next( void )
{
   // determine the next one that get's focus
   int newIndex = getNextFocusIndex(mSelectedIndex);

   if (newIndex == mSelectedIndex)
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eListBottomEvent );
      return true;
   }

   // determine how many items we moved down.
   int numMoved = newIndex-mSelectedIndex;

   // number in the list - the bottom index
   int numNotVisibleBelowList = getLastVisible()+1-(mFirstVisibleIndex+mViewSize);

   numNotVisibleBelowList=max(numNotVisibleBelowList, 0);   // cap this in case we have less in the list than it can show.

   // how much do we need to scroll the list?
   int scrollCount = min(numMoved, numNotVisibleBelowList);

   // update the first visible index
   mFirstVisibleIndex += scrollCount;

   // scroll the list up for 'scrollCount' items
   scrollList(scrollCount);

   invokeActionScript( "onNext" );
   bool result = setIndex( newIndex );
   if (result)
   {
      fireUIControlEvent( eItemSelected );
      fireUIControlEvent( eNext );
   }

   return result;
}

//==============================================================================
//==============================================================================
bool BUIScrollingListControl::prev( void )
{
   int newIndex = getPrevFocusIndex(mSelectedIndex);

   if (newIndex == mSelectedIndex)
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eListTopEvent );
      return true;
   }

   if (newIndex < mFirstVisibleIndex)
   {
      // do a scroll up
      int scrollCount = mFirstVisibleIndex-newIndex;
      mFirstVisibleIndex -= scrollCount;
      scrollList(-scrollCount);   // this should be negative
   }

   invokeActionScript( "onPrev" );
   bool result=setIndex( newIndex );
   if (result)
   {
      fireUIControlEvent( eItemSelected );
      fireUIControlEvent( ePrev );
   }

   return result;
}

//==============================================================================
//==============================================================================
void BUIScrollingListControl::reset()
{
   mSelectedIndex=0;
   mFirstVisibleIndex=0;
   invokeActionScript("reset");
}


//==============================================================================
//==============================================================================
void BUIScrollingListControl::scrollList(int scrollCount)
{
   if (scrollCount == 0)
      return;

   GFxValue value;

   value.SetNumber(scrollCount);
   invokeActionScript("scrollList", &value, 1);
}

//==============================================================================
//==============================================================================
bool BUIScrollingListControl::setIndex( int index )
{
   int numControls = (int)mControls.size();
   if( getWrap() || ( index >= 0 && index < numControls ) )
   {
      if( mControls[mSelectedIndex]->isFocused() )
      {
         mControls[mSelectedIndex]->unfocus();
      }
      
      mSelectedIndex = index;
      
      if( mSelectedIndex >= numControls )
      {
         mSelectedIndex = mSelectedIndex % numControls;
      }
      else if( mSelectedIndex < 0 )
      {
         while( mSelectedIndex < 0 )
            mSelectedIndex += numControls;
      }

      if( !mControls[mSelectedIndex]->isFocused() )
      {
         mControls[mSelectedIndex]->focus();
      }

      return true;
   }

   return false;
}

//============================================================================
//============================================================================
bool BUIScrollingListControl::getEventName( int eventID, BString& eventName )
{
   return BUIControl::getEventName( eventID, eventName ) || BUIScrollingListControl::getEventName( eventID, eventName );
}