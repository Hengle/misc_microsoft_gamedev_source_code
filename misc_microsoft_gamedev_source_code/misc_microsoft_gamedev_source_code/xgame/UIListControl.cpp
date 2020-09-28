//============================================================================
// UIListControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIListControl.h"
#include "UIInputHandler.h"

//==============================================================================
//==============================================================================
BUIListControl::BUIListControl( void ) :
   mWrap( false ),
   mIgnoreNextNext( false ),
   mIgnoreNextPrev( false ),
   mAlignment( eVertical ),
   mSelectedIndex( -1 )
{
   mControlType.set("UIListControl");
   INIT_EVENT_MAP();
}

//==============================================================================
//==============================================================================
BUIListControl::~BUIListControl( void )
{
}

//============================================================================
//============================================================================
bool BUIListControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID , BXMLNode* initData )
{
   bool result = __super::init( parent, controlPath, controlID, initData );
   
   if( result )
      setAlignment( mAlignment );

   return result;
}

//==============================================================================
//==============================================================================
void BUIListControl::setWrap( bool wrap )
{
   mWrap = wrap;
}

//==============================================================================
//==============================================================================
bool BUIListControl::getWrap( void ) const
{
   return mWrap;
}

//==============================================================================
//==============================================================================
void BUIListControl::setAlignment( BUIListControl::EListAlignment alignment )
{
   mAlignment = alignment;
 
   if( mpInputHandler )
   {
      switch( mAlignment )
      {
         case eHorizontal: mpInputHandler->enterContext( "Horizontal" ); break;
         case eVertical: mpInputHandler->enterContext( "Main" ); break;
      }
   }
}

//==============================================================================
//==============================================================================
BUIListControl::EListAlignment BUIListControl::getAlignment( void ) const
{
   return mAlignment;
}

//==============================================================================
//==============================================================================
void BUIListControl::clearControls()
{
   mControls.clear();
   mSelectedIndex = -1;
   invokeActionScript( "onClearControls" );
}

//==============================================================================
//==============================================================================
void BUIListControl::addControl( BUIControl* control )
{
   mControls.add( control );

   GFxValue args[1];
   args[0].SetString( control->getControlPath() );
   invokeActionScript( "onAddControl", args, 1 );
}

//==============================================================================
//==============================================================================
bool BUIListControl::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   if (command=="prev")
   {
      if (mIgnoreNextPrev)
      {
         mIgnoreNextPrev=false;
         return true;
      }
      prev();
      return true;
      // fixme - bubble the event up too.
   }
   else if (command=="next")
   {
      if (mIgnoreNextNext)
      {
         mIgnoreNextNext=false;
         return true;
      }
      next();
      // fixme - bubble the event up too.
      return true;
   }
   
   fireUIControlEvent( command.getPtr() );
   return false;
}

//==============================================================================
//==============================================================================
BUIControl* BUIListControl::getControl( int index )
{
   BUIControl* control = NULL;

   if( index >= 0 && index < (int)mControls.size() )
   {
      return mControls[index];
   }

   return control;
}

//==============================================================================
//==============================================================================
BUIControl* BUIListControl::getSelectedControl( void )
{
   return getControl( mSelectedIndex );
}

//============================================================================
//============================================================================
int BUIListControl::getControlCount( void ) const
{
   return mControls.size();
}

//==============================================================================
//==============================================================================
bool BUIListControl::next( void )
{
   // determine the next one that get's focus
   int newIndex = getNextFocusIndex(mSelectedIndex);

   // did we get a new position?
   if (newIndex == mSelectedIndex)
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eStopEnd );
      // fireUIControlEvent( eListBottomEvent );
      return true;
   }

   // fire off the "onNext" events
   invokeActionScript( "onNext" );
   fireUIControlEvent( eNext );

   // change our selection.
   bool result = setIndex( newIndex );
   if (result)
      fireUIControlEvent( eItemSelected );

   return result;
}


//==============================================================================
//==============================================================================
int BUIListControl::getNextFocusIndex(int currentIndex)
{
   // we may need to check all controls
   int nextIndex = currentIndex+1;

   for (int i=0; i<mControls.getNumber(); i++)
   {
      if (nextIndex == currentIndex)
         return nextIndex;                      // we didn't change our position

      // check list bounds
      if (nextIndex >= mControls.getNumber())
      {
         if (mWrap)
         {
            nextIndex=0;                        // if we wrap, just go to index 0
            continue;
         }
         else
         {
            // if we don't wrap and haven't found a new index, then just return the index passed in.
            return currentIndex;                // we went off the end of the list
         }
      }

      // check the control at this index.
//-- FIXING PREFIX BUG ID 2276
      const BUIControl* c=NULL;
//--
      c = getControl(nextIndex);
      if (c && c->isEnabled() && c->isShown())  // skip it if null, not enabled, not visible
      {
         return nextIndex;                      // we found a valid control, store off the index
      }

      nextIndex++;
   }

   // in case we get back to here.
   return currentIndex;
}


//==============================================================================
//==============================================================================
bool BUIListControl::prev( void )
{
   int newIndex = getPrevFocusIndex(mSelectedIndex);

   if (newIndex == mSelectedIndex)
   {
      // invoke a UI event to let our parent know we hit the bottom of the list
      fireUIControlEvent( eStopBegin );
      // fireUIControlEvent( eListBottomEvent );
      return true;
   }

   // fire off the "onPrev" events
   invokeActionScript( "onPrev" );
   fireUIControlEvent( ePrev );

   // change our selection.
   bool result = setIndex( newIndex );
   if (result)
      fireUIControlEvent( eItemSelected );

   return result;
}

//==============================================================================
//==============================================================================
int BUIListControl::getPrevFocusIndex(int currentIndex)
{

   // we may need to check all controls
   int nextIndex = currentIndex-1;
   for (int i=0; i<mControls.getNumber(); i++)
   {
      if (nextIndex == currentIndex)
         return nextIndex;                      // we didn't change our position

      // check list bounds
      if (nextIndex<0)
      {
         if (mWrap)
         {
            nextIndex=mControls.getNumber()-1;                        // if we wrap, just go to the last index
            continue;
         }
         else
         {
            // if we don't wrap and haven't found a new index, then just return the index passed in.
            return currentIndex;                // we went off the end of the list
         }
      }

      // check the control at this index.
//-- FIXING PREFIX BUG ID 2277
      const BUIControl* c=NULL;
//--
      c = getControl(nextIndex);
      if (c && c->isEnabled() && c->isShown())  // skip it if null, not enabled, not visible
      {
         return nextIndex;                      // we found a valid control, store off the index
      }

      nextIndex--;
   }

   // in case we get back to here.
   return currentIndex;
}

//==============================================================================
//==============================================================================
bool BUIListControl::setIndex( int index )
{
   if (index==mSelectedIndex)
      return false;

   BUIControl* cNew = getControl(index);
   BUIControl* cOld = getControl(mSelectedIndex);

   // turn off the old one if necessary
   if (cOld && cOld->isFocused())
      cOld->unfocus();

   if (cNew && cNew->isEnabled() && cNew->isShown())
      cNew->focus();

   mSelectedIndex=index;

   GFxValue args[1];
   args[0].SetNumber( mSelectedIndex );
   invokeActionScript( "onSelectedIndexChanged", args, 1 );
   fireUIControlEvent( eSelectionChanged );

   return true;
}

//==============================================================================
//==============================================================================
bool BUIListControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   BUIControl* pSelectedControl = getSelectedControl();

   if( pSelectedControl )
      handled = pSelectedControl->handleInput( port, event, controlType, detail );

   if (!handled)
      handled = __super::handleInput( port, event, controlType, detail );

   return handled;
}

//============================================================================
//============================================================================
int BUIListControl::getSelectedIndex( void )
{
   return mSelectedIndex;
}

//============================================================================
//============================================================================
void BUIListControl::focus( bool force )
{
   if( ( !isFocused() || force ) && getSelectedControl() )
      getSelectedControl()->focus( force );

   __super::focus( force );
}

//============================================================================
//============================================================================
void BUIListControl::unfocus( bool force )
{
   if( ( isFocused() || force ) && getSelectedControl() )
      getSelectedControl()->unfocus( force );

   __super::unfocus( force );
}
