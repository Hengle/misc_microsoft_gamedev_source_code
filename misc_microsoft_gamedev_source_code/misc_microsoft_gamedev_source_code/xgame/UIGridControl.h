//============================================================================
// UIGridControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "xcore.h"

class BUIGridControl : public BUIControl
{
public:
   enum Events
   {
      eNextHorizontal = UIGridControlID,
      ePrevHorizontal,
      eNextVertical,
      ePrevVertical,
      eStopTop,
      eStopBottom,
      eStopLeft,
      eStopRight,
      eItemSelected,
      eSelectionChanged
   };
   BEGIN_EVENT_MAP( UIGridControl )
      MAP_CONTROL_EVENT( NextHorizontal )
      MAP_CONTROL_EVENT( PrevHorizontal )
      MAP_CONTROL_EVENT( NextVertical )
      MAP_CONTROL_EVENT( PrevVertical )
      MAP_CONTROL_EVENT( StopTop )
      MAP_CONTROL_EVENT( StopBottom )
      MAP_CONTROL_EVENT( StopLeft )
      MAP_CONTROL_EVENT( StopRight )
      MAP_CONTROL_EVENT( ItemSelected )
      MAP_CONTROL_EVENT( SelectionChanged )
   END_EVENT_MAP()
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIGridControl );

public:
   BUIGridControl( int rows, int columns );
   virtual ~BUIGridControl( void );

   virtual void setWrap( bool wrap );
   virtual bool getWrap( void ) const;

   virtual bool setControl( BUIControl* control, int row, int column );
   virtual void clearControls();
   virtual BUIControl* getControl( int row, int column );
   virtual BUIControl* getSelectedControl( void );
   virtual int getRowCount( void ) const;
   virtual int getColumnCount( void ) const;

   virtual bool up(void);
   virtual bool down(void);
   virtual bool left(void);
   virtual bool right(void);

   virtual bool setSelectedCell(int row, int column);
   virtual bool getSelectedCell( int& row, int& column);

   //----- IInputControlEventHandler 
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

   bool isValidCoordinate(int row, int column);

   bool getAllowNavigation() { return mAllowNavigation; }
   void setAllowNavigation(bool value) { mAllowNavigation = value; }

   void setIgnoreNextDown(bool v) { mIgnoreNextDown=v; }
   void setIgnoreNextUp(bool v) { mIgnoreNextUp=v; }

protected:
   bool getNextUpFocusIndex(int& row, int& column);
   bool getNextDownFocusIndex(int& row, int& column);
   bool getNextRightFocusIndex(int& row, int& column);
   bool getNextLeftFocusIndex(int& row, int& column);
   
   bool  mWrap:1;
   bool  mAllowNavigation:1;
   bool  mIgnoreNextDown:1;
   bool  mIgnoreNextUp:1;
   int   mSelectedRow;
   int   mSelectedColumn;

   BDynamicArray<BDynamicArray<BUIControl*>> mGridControls;

};
