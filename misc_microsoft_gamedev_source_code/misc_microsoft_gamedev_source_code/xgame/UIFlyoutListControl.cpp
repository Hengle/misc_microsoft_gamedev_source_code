//============================================================================
// UIFlyoutListControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIFlyoutListControl.h"

//============================================================================
//============================================================================
BUIFlyoutListControl::BUIFlyoutListControl( void ) :
   mbOpen( false )
{
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUIFlyoutListControl::~BUIFlyoutListControl( void )
{
}

//============================================================================
//============================================================================
bool BUIFlyoutListControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData /* = NULL */ )
{
   bool result = __super::init( parent, controlPath, controlID, initData );
   
   if( result )
      result = mOpen.init( this, mScriptPrefix + "mOpen" );

   if( result )
      result = mClosed.init( this, mScriptPrefix + "mClosed" );

   BString itemPath;
   for( int i = 0; result && i < MAX_ITEMS; ++i )
   {
      itemPath.format( "mOpen.mItem%d", i + 1 );
      result = mItems[i].init( this, mScriptPrefix + itemPath );
      if( result )
         mItems[i].hide();
   }

   if( result )
      mOpen.hide();

   return result;
}

//============================================================================
//============================================================================
bool BUIFlyoutListControl::handleUIControlEvent( BUIControlEvent& event )
{
   if( event.getString() == "accept" )
   {
      if( !isOpen() )
      {
         open();
      }
      else
      {
         BUIMenuItemControl* pSelectedItem = (BUIMenuItemControl*)mOpen.getSelectedControl();
         if( pSelectedItem->getControlID() != mClosed.getControlID() )
         {
            mClosed.setText( pSelectedItem->getText() );
            mClosed.setControlID( pSelectedItem->getControlID() );
            fireUIControlEvent( eSelectionChanged );
         }
         close();
      }
      return true;
   }
   else
      return __super::handleUIControlEvent( event );
}

//============================================================================
//============================================================================
bool BUIFlyoutListControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "select" )
   {
      if( !isOpen() )
      {
         open();
      }
      else
      {
         BUIMenuItemControl* pSelectedItem = (BUIMenuItemControl*)mOpen.getSelectedControl();
         mClosed.setText( pSelectedItem->getText() );
         mClosed.setControlID( pSelectedItem->getControlID() );
         close();
         fireUIControlEvent( eSelectionChanged );
      }
      return true;
   }
   else if( isOpen() && command == "cancel" )
   {
      for( int i = 0; i < mOpen.getControlCount(); ++i )
      {
         if( mOpen.getControl( i )->getControlID() == mClosed.getControlID() )
         {
            mOpen.setIndex( i );
            break;
         }
      }

      close();
      return true;
   }

   return false;
}

//============================================================================
//============================================================================
bool BUIFlyoutListControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool result = false;

   if( isOpen() )
   {
      result = mOpen.handleInput( port, event, controlType, detail );
      
      if( !result )
         result = __super::handleInput( port, event, controlType, detail );
   }
   else
      result = mClosed.handleInput( port, event, controlType, detail );

   return result;
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::focus( bool force )
{
   if( !isFocused() || force )
   {
      if( isOpen() )
         mOpen.focus( force );
      else
         mClosed.focus( force );
   }

   __super::focus( force );
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::unfocus( bool force )
{
   if( isFocused() || force )
   {
      if( isOpen() )
         mOpen.unfocus( force );
      else
         mClosed.unfocus( force );
   }

   __super::unfocus( force );
}


//============================================================================
//============================================================================
void BUIFlyoutListControl::show( bool force /* = false */ )
{
   if( !isShown() || force )
   {
      if( isOpen() )
         mOpen.show( force );
      else
         mClosed.show( force );
   }

   __super::show( force );
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::hide( bool force /* = false */ )
{
   if( isShown() || force )
   {
      if( isOpen() )
         mOpen.hide( force );
      else
         mClosed.hide( force );
   }

   __super::hide( force );
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::open( void )
{
   if( !isOpen() )
   {
      mbOpen = true;
      for( int i = 0; i < mOpen.getControlCount(); ++i )
      {
         if( i != mOpen.getSelectedIndex() )
         {
            BUIControl* pItem = mOpen.getControl( i );
            if( pItem )
               pItem->unfocus();
         }
      }
      mOpen.show();
      mClosed.hide();
      fireUIControlEvent( eOpen );
   }
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::close( void )
{
   if( isOpen() )
   {
      mbOpen = false;
      mOpen.hide();
      mClosed.show();
      fireUIControlEvent( eClose );
   }
}

//============================================================================
//============================================================================
bool BUIFlyoutListControl::isOpen( void ) const
{
   return mbOpen;
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::addItem( const BUString& itemText, int itemValue )
{
   int numItems = mOpen.getControlCount();
   if( numItems < MAX_ITEMS )
   {
      BUIMenuItemControl* pItem = &(mItems[numItems]);
      pItem->setText( itemText );
      pItem->setControlID( itemValue );
      pItem->show();
      mOpen.addControl( &mItems[numItems]);
   }
}

//============================================================================
//============================================================================
int BUIFlyoutListControl::getValue( void )
{
   return mOpen.getSelectedControl()->getControlID();
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::setValue( int value )
{
   for( int i = 0; i < mOpen.getControlCount(); ++i )
   {
      BUIMenuItemControl* pItem = (BUIMenuItemControl*)mOpen.getControl( i );
      if( pItem->getControlID() == value )
      {
         setIndex( i );
         break;
      }
   }
}

//============================================================================
//============================================================================
int BUIFlyoutListControl::getValueByIndex( int index )
{
   return mOpen.getControl( index )->getControlID();
}

//============================================================================
//============================================================================
int BUIFlyoutListControl::getSelectedIndex( void )
{
   return mOpen.getSelectedIndex();
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::setIndex( int index )
{
   mOpen.setIndex( index );
   BUIMenuItemControl* pSelectedItem = ((BUIMenuItemControl*)mOpen.getControl( index ));
   mClosed.setText( pSelectedItem->getText() );
   mClosed.setControlID( pSelectedItem->getControlID() );
}

//============================================================================
//============================================================================
int BUIFlyoutListControl::getNumValues( void )
{
   return mOpen.getControlCount();
}

//============================================================================
//============================================================================
void BUIFlyoutListControl::clear( void )
{
   for( int i = 0; i < MAX_ITEMS; ++i )
      mItems[i].hide();
   mOpen.clearControls();
}