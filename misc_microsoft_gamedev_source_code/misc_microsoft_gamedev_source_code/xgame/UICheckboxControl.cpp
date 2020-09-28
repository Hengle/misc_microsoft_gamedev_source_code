//============================================================================
// UICheckboxControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UICheckboxControl.h"

BUICheckboxControl::BUICheckboxControl( void ) : 
   mChecked( false )
{
   INIT_EVENT_MAP();
}

BUICheckboxControl::~BUICheckboxControl( void )
{
}

bool BUICheckboxControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "toggle" )
   {
      check( !isChecked() );
      return true;
   }
   return false;
}

void BUICheckboxControl::check( bool bCheck /* = true */ )
{
   if( bCheck )
   {
      mChecked = true;
      invokeActionScript( "onCheck" );
      fireUIControlEvent( eCheck );
   }
   else
   {
      uncheck();
   }
}

void BUICheckboxControl::uncheck( void )
{
   mChecked = false;
   invokeActionScript( "onUncheck" );
   fireUIControlEvent( eUncheck );
}

bool BUICheckboxControl::isChecked( void ) const
{
   return mChecked;
}