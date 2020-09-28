//============================================================================
// UIGridControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIGridControl.h"

//==============================================================================
//==============================================================================
BUIGridControl::BUIGridControl( int rows, int columns ) :
   mWrap(false),
   mSelectedRow(0),
   mSelectedColumn(0),
   mAllowNavigation(true),
   mIgnoreNextDown(false),
   mIgnoreNextUp(false)
{
   mControlType.set("UIGridControl");
   INIT_EVENT_MAP();

   mGridControls.resize(rows);

   // pre-allocate space for our controls
   for (int i=0; i<mGridControls.getNumber(); i++)
   {
      mGridControls[i].resize(columns);
      for (int j=0; j<mGridControls[i].getNumber(); j++)
      {
         mGridControls[i][j]=NULL;           // clear out this pointer
      }
   }
}

//==============================================================================
//==============================================================================
BUIGridControl::~BUIGridControl( void )
{
}

//==============================================================================
//==============================================================================
void BUIGridControl::setWrap( bool wrap )
{
   mWrap = wrap;
}

//==============================================================================
//==============================================================================
bool BUIGridControl::getWrap( void ) const
{
   return mWrap;
}

//==============================================================================
//==============================================================================
void BUIGridControl::clearControls()
{
   for (int i=0; mGridControls.getNumber(); i++)
   {
      for (int j=0; j<mGridControls[i].getNumber(); j++)
      {
         mGridControls[i].clear();
      }
   }

   mGridControls.clear();
}

//==============================================================================
//==============================================================================
bool BUIGridControl::isValidCoordinate(int row, int column)
{
   // validate the row
   if ( (row<0) || (row>=mGridControls.getNumber()) )
      return false;

   // validate the column
   if ( (column<0) || (column>=mGridControls[row].getNumber()) )
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUIGridControl::setControl( BUIControl* control, int row, int column )
{
   if (!isValidCoordinate(row, column))
      return false;

   // add the control
   mGridControls[row][column]=control;

   return true;
}

//==============================================================================
//==============================================================================
bool BUIGridControl::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   if (command=="up")
   {
      if (mIgnoreNextUp)
      {
         mIgnoreNextUp=false;
         return true;
      }
      if (mAllowNavigation)
         up();
      return true;
   }
   else if (command=="down")
   {
      if (mIgnoreNextDown)
      {
         mIgnoreNextDown=false;
         return true;
      }
      if (mAllowNavigation)
         down();
      return true;
   }
   else if (command=="left")
   {
      if (mAllowNavigation)
         left();
      return true;
   }
   else if (command=="right")
   {
      if (mAllowNavigation)
         right();
      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
BUIControl* BUIGridControl::getControl( int row, int column )
{
   if (!isValidCoordinate(row, column))
      return NULL;

   return mGridControls[row][column];
}

//==============================================================================
//==============================================================================
BUIControl* BUIGridControl::getSelectedControl( void )
{
   return getControl(mSelectedRow, mSelectedColumn);
}

//============================================================================
//============================================================================
int BUIGridControl::getRowCount( void ) const
{
   return mGridControls.getNumber();
}

//============================================================================
//============================================================================
int BUIGridControl::getColumnCount( void ) const
{
   if (mGridControls.getNumber()<=0)
      return 0;

   // the grid is evenly built, so returning the count of row 0 is sufficient
   return mGridControls[0].getNumber();
}

//==============================================================================
//==============================================================================
bool BUIGridControl::up( void )
{
   // determine the next one that get's focus
   int newRow=mSelectedRow;
   int newColumn=mSelectedColumn;

   getNextUpFocusIndex(newRow, newColumn);
   if ( (newRow==mSelectedRow) && (newColumn==mSelectedColumn) )
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eStopTop );
      return true;
   }

   // change our selection.
   bool result = setSelectedCell( newRow, newColumn );
   if (result)
   {
      fireUIControlEvent( ePrevVertical );
      fireUIControlEvent( eItemSelected );
   }

   return result;
}


//==============================================================================
//==============================================================================
bool BUIGridControl::down( void )
{
   // determine the next one that get's focus
   int newRow=mSelectedRow;
   int newColumn=mSelectedColumn;

   getNextDownFocusIndex(newRow, newColumn);
   if ( (newRow==mSelectedRow) && (newColumn==mSelectedColumn) )
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eStopBottom );
      return true;
   }

   // change our selection.
   bool result = setSelectedCell( newRow, newColumn );
   if (result)
   {
      fireUIControlEvent( eNextVertical );
      fireUIControlEvent( eItemSelected );
   }

   return result;
}

//==============================================================================
//==============================================================================
bool BUIGridControl::left( void )
{
   // determine the next one that get's focus
   int newRow=mSelectedRow;
   int newColumn=mSelectedColumn;

   getNextLeftFocusIndex(newRow, newColumn);
   if ( (newRow==mSelectedRow) && (newColumn==mSelectedColumn) )
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eStopLeft );
      return true;
   }

   // change our selection.
   bool result = setSelectedCell( newRow, newColumn );
   if (result)
   {
      fireUIControlEvent( ePrevHorizontal );
      fireUIControlEvent( eItemSelected );
   }

   return result;
}

//==============================================================================
//==============================================================================
bool BUIGridControl::right( void )
{
   // determine the next one that get's focus
   int newRow=mSelectedRow;
   int newColumn=mSelectedColumn;

   getNextRightFocusIndex(newRow, newColumn);
   if ( (newRow==mSelectedRow) && (newColumn==mSelectedColumn) )
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eStopRight );
      return true;
   }

   // change our selection.
   bool result = setSelectedCell( newRow, newColumn );
   if (result)
   {
      fireUIControlEvent( eNextHorizontal );
      fireUIControlEvent( eItemSelected );
   }

   return result;
}


//==============================================================================
//==============================================================================
bool BUIGridControl::getNextDownFocusIndex(int& row, int& column)
{
   if (!isValidCoordinate(row, column))
      return false;

   int newRow = row+1;
   for (int i=0; i<mGridControls.getNumber(); i++)
   {
      if (newRow == row)
         return false;                      // we didn't change our position

      // check list bounds
      if (newRow >= mGridControls.getNumber())
      {
         if (mWrap)
         {
            newRow=0;                        // if we wrap, just go to index 0
            continue;
         }
         else
         {
            // if we don't wrap and haven't found a new index, then just return the index passed in.
            return false;                // we went off the end of the list
         }
      }

      // check the control at this index.
//-- FIXING PREFIX BUG ID 2279
      const BUIControl* c=NULL;
//--
      c = getControl(newRow, column);
      if (c && c->isEnabled() && c->isShown())  // skip it if null, not enabled, not visible
      {
         row = newRow;
         return true;                      // we found a valid control, store off the index
      }

      newRow++;
   }

   // in case we get back to here.
   return false;
}


//==============================================================================
//==============================================================================
bool BUIGridControl::getNextUpFocusIndex(int& row, int& column)
{
   if (!isValidCoordinate(row, column))
      return false;

   // we may need to check all controls
   int newRow = row-1;
   for (int i=0; i<mGridControls.getNumber(); i++)
   {
      if (newRow == row)
         return false;                      // we didn't change our position

      // check list bounds
      if (newRow<0)
      {
         if (mWrap)
         {
            newRow=mGridControls.getNumber()-1;                        // if we wrap, just go to the last index
            continue;
         }
         else
         {
            // if we don't wrap and haven't found a new index, then just return the index passed in.
            return false;                // we went off the end of the list
         }
      }

      // check the control at this index.
//-- FIXING PREFIX BUG ID 2280
      const BUIControl* c=NULL;
//--
      c = getControl(newRow, column);
      if (c && c->isEnabled() && c->isShown())  // skip it if null, not enabled, not visible
      {
         row=newRow;
         return true;                      // we found a valid control, store off the index
      }

      newRow--;
   }

   // in case we get back to here.
   return false;
}


//==============================================================================
//==============================================================================
bool BUIGridControl::getNextRightFocusIndex(int& row, int& column)
{
   if (!isValidCoordinate(row, column))
      return false;

   int newColumn=column+1;
   for (int i=0; i<mGridControls[row].getNumber(); i++)
   {
      if (newColumn==column)
         return false;                      // we didn't change our position

      // check list bounds
      if (newColumn>=mGridControls[row].getNumber())
      {
         if (mWrap)
         {
            newColumn=0;                        // if we wrap, just go to index 0
            continue;
         }
         else
         {
            // if we don't wrap and haven't found a new index, then just return the index passed in.
            return false;                // we went off the end of the list
         }
      }

      // check the control at this index.
//-- FIXING PREFIX BUG ID 2281
      const BUIControl* c=NULL;
//--
      c = getControl(row, newColumn);
      if (c && c->isEnabled() && c->isShown())  // skip it if null, not enabled, not visible
      {
         column=newColumn;
         return true;                      // we found a valid control, store off the index
      }

      newColumn++;
   }

   // in case we get back to here.
   return false;
}

//==============================================================================
//==============================================================================
bool BUIGridControl::getNextLeftFocusIndex(int& row, int& column)
{
   if (!isValidCoordinate(row, column))
      return false;

   // we may need to check all controls
   int newColumn=column-1;
   for (int i=0; i<mGridControls[row].getNumber(); i++)
   {
      if (newColumn==column)
         return false;                      // we didn't change our position

      // check list bounds
      if (newColumn<0)
      {
         if (mWrap)
         {
            newColumn=mGridControls[row].getNumber()-1;                        // if we wrap, just go to the last index
            continue;
         }
         else
         {
            // if we don't wrap and haven't found a new index, then just return the index passed in.
            return false;                // we went off the end of the list
         }
      }

      // check the control at this index.
//-- FIXING PREFIX BUG ID 2282
      const BUIControl* c=NULL;
//--
      c = getControl(row, newColumn);
      if (c && c->isEnabled() && c->isShown())  // skip it if null, not enabled, not visible
      {
         column=newColumn;
         return true;                      // we found a valid control, store off the index
      }

      newColumn--;
   }

   // in case we get back to here.
   return false;
}

//==============================================================================
//==============================================================================
/*
bool BUIGridControl::setSelectedCell(int row, int column)
{
   // did the focus change?
   if ( (mSelectedRow==row) && (mSelectedColumn==column) )
      return false;

   if (!isValidCoordinate(row, column))
      return false;

   // it's a valid coord, but a valid control?
   BUIControl* cNew=mGridControls[row][column];
   if (!cNew || !cNew->isEnabled() || !cNew->isShown())
      return false;

   // deal with the old 
   if (isValidCoordinate(mSelectedRow, mSelectedColumn))
   {
      BUIControl* cOld=mGridControls[mSelectedRow][mSelectedColumn];
      if (cOld && cOld->isFocused())
      {
         cOld->unfocus();
      }
   }

   // focus the new control
   cNew->focus();
   mSelectedRow=row;
   mSelectedColumn=column;

   // fire the events
   fireUIControlEvent( eSelectionChanged );

   return true;
}
*/

//==============================================================================
//==============================================================================
bool BUIGridControl::setSelectedCell(int row, int column)
{
   // did the focus change?
   if ( (mSelectedRow==row) && (mSelectedColumn==column) )
      return false;

   BUIControl* cNew=NULL;
   if (isValidCoordinate(row, column))
   {
      // it's a valid coord, but a valid control?
      cNew=mGridControls[row][column];
   }

   // deal with the old 
   if (isValidCoordinate(mSelectedRow, mSelectedColumn))
   {
      BUIControl* cOld=mGridControls[mSelectedRow][mSelectedColumn];
      if (cOld && cOld->isFocused())
      {
         cOld->unfocus();
      }
   }

   // focus the new control
   if (cNew && cNew->isEnabled() && cNew->isShown())
      cNew->focus();

   mSelectedRow=row;
   mSelectedColumn=column;

   // fire the events
   fireUIControlEvent( eSelectionChanged );

   return true;
}


//==============================================================================
//==============================================================================
bool BUIGridControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = BUIControl::handleInput(port, event, controlType, detail);

   // If we didn't handle it, then try the selected item.
   if (!handled)
   {
      BUIControl* selectedControl = getSelectedControl();
      if (selectedControl)
         handled = selectedControl->handleInput(port, event, controlType, detail);
   }
   return handled;
}

//============================================================================
//============================================================================
bool BUIGridControl::getSelectedCell( int& row, int& column)
{
   row = mSelectedRow;
   column = mSelectedColumn;

   return isValidCoordinate(row, column);
}
