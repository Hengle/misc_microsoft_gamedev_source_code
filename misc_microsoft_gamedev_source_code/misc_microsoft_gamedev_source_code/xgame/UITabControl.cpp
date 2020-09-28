//============================================================================
// UITabControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UITabControl.h"

//============================================================================
//============================================================================
BUITabControl::BUITabControl( void )
{
   mControlType.set("UITabControl");
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUITabControl::~BUITabControl( void )
{

}

//============================================================================
//============================================================================
bool BUITabControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   bool result = __super::init( parent, controlPath, controlID, initData );
   
   if( result )
      mTabList.init( this, "", 0, initData );

   return result;
}

//============================================================================
// IUIControlEventHandler
//============================================================================
bool BUITabControl::handleUIControlEvent( BUIControlEvent& event )
{
   if( event.getControl() == &mTabList && event.getID() == BUIListControl::eSelectionChanged )
   {
      updateContentVisiblity();
      fireUIControlEvent( eTabChanged );
      return true;
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BUITabControl::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   if (command=="prev")
   {
      prev();
      return true;
      // fixme - bubble the event up too.
   }
   else if (command=="next")
   {
      next();
      return true;
      // fixme - bubble the event up too.
   }

   return false;
}


//============================================================================
//============================================================================
void BUITabControl::addTab( BUIControl* tab, BUIControl* pane )
{
   BASSERT( tab );
   BASSERT( pane );

   mTabList.addControl( tab );
   mPanes.add( pane );
}

//============================================================================
//============================================================================
void BUITabControl::setWrap( bool wrap )
{
   mTabList.setWrap( wrap );
}

//============================================================================
//============================================================================
bool BUITabControl::getWrap( void ) const
{
   return mTabList.getWrap();
}

//============================================================================
//============================================================================
bool BUITabControl::next( void )
{
   if( mTabList.next() )
   {
      invokeActionScript( "onNext" );
      return true;
   }
   return false;
}

//============================================================================
//============================================================================
bool BUITabControl::prev( void )
{
   if( mTabList.prev() )
   {
      invokeActionScript( "onPrev" );
      return true;
   }
   return false;
}

//============================================================================
//============================================================================
BUIControl* BUITabControl::getPane( int index )
{
   BUIControl* pane = NULL;

   if( index >= 0 && index < (int)mPanes.getSize() )
   {
      pane = mPanes[index];
   }

   return pane;
}

//============================================================================
//============================================================================
BUIControl* BUITabControl::getActivePane( void )
{
   return getPane( getActiveTab() );
}

//============================================================================
//============================================================================
void BUITabControl::setActiveTab(int index)
{
   mTabList.setIndex(index);
}
//============================================================================
//============================================================================
int BUITabControl::getActiveTab()
{
   return mTabList.getSelectedIndex();
}


//============================================================================
//============================================================================
void BUITabControl::updateContentVisiblity( void )
{
   int selectedIndex = mTabList.getSelectedIndex();
   
   for( int i = 0; i < (int)mPanes.size(); ++i )
   {
      if( mPanes[i]->isShown() && i != selectedIndex )
         mPanes[i]->hide();
   }

   for( int i = 0; i < (int)mPanes.size(); ++i )
   {
      if( !mPanes[i]->isShown() && i == selectedIndex )
         mPanes[i]->show();
   }
}