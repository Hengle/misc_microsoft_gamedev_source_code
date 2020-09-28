//============================================================================
// UIListControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "UIListControl.h"
#include "xcore.h"

class BUIScrollingListControl : public BUIListControl
{
public:
   enum Events
   {
      eListBottomEvent = UIScrollingListControlID,
      eListTopEvent,
      ePrev,
      eStopBegin,
      eStopEnd,
      eItemSelected,
      eSelectionChanged
   };

   BEGIN_EVENT_MAP( UIScrollingListControl )
      MAP_CONTROL_EVENT( ListBottomEvent )
      MAP_CONTROL_EVENT( ListTopEvent )
      MAP_CONTROL_EVENT( Next )
      MAP_CONTROL_EVENT( Prev )
      MAP_CONTROL_EVENT( StopBegin )
      MAP_CONTROL_EVENT( StopEnd )
      MAP_CONTROL_EVENT( ItemSelected )
      MAP_CONTROL_EVENT( SelectionChanged )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIScrollingListControl );

public:
   BUIScrollingListControl( void );
   virtual ~BUIScrollingListControl( void );


   virtual bool next( void );
   virtual bool prev( void );
   virtual bool setIndex( int index );

   //----- IInputControlEventHandler 
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );


   virtual int getMaxItems() { return mMaxItems; }
   virtual void setMaxItems(int maxItems) { mMaxItems=maxItems; };

   virtual int getViewSize() { return mViewSize; }
   virtual void setViewSize(int viewSize) { mViewSize=viewSize; };

   virtual int getFirstVisibleIndex() { return mFirstVisibleIndex; }

   void scrollList(int scrollCount);

   // methods that are candidates for base UIListControl
   int getNumVisibleControls();
   int getLastVisible();                     // Index of the last visible control

   void reset();


protected:

   virtual bool getEventName( int eventID, BString& eventName );
  
   int mMaxItems;          // list size - number of elements in our list
   int mViewSize;          // view size - number of elements the view can show
   int mFirstVisibleIndex; // first visible item - the index of the first viewable item.
};