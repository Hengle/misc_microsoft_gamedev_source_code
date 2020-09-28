//============================================================================
// UICheckeMenuItemControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UICheckedMenuItemControl.h"

//============================================================================
//============================================================================
BUICheckedMenuItemControl::BUICheckedMenuItemControl( void ) :
   mData(-1)
{
   mControlType.set("UICheckedMenuItemControl");
   mText.set("");
}

//============================================================================
//============================================================================
BUICheckedMenuItemControl::~BUICheckedMenuItemControl( void )
{
}

//============================================================================
//============================================================================
bool BUICheckedMenuItemControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   bool result = __super::init( parent, controlPath, controlID, NULL );
   if (result)
   {
      result = mCheckbox.init(this, mScriptPrefix+"mCheckbox", -1, NULL);
   }
      
   return result;
}

//============================================================================
//============================================================================
bool BUICheckedMenuItemControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "toggle" )
   {
      check( !isChecked() );
      return true;
   }

   fireUIControlEvent( command.getPtr() );
   return true;
}

//============================================================================
//============================================================================
void BUICheckedMenuItemControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}

//============================================================================
//============================================================================
void BUICheckedMenuItemControl::check( bool bCheck /* = true */ )
{
   mCheckbox.check(bCheck);   
}

//============================================================================
//============================================================================
void BUICheckedMenuItemControl::uncheck( void )
{
   mCheckbox.uncheck();
}

//============================================================================
//============================================================================
bool BUICheckedMenuItemControl::isChecked( void ) const
{
   return mCheckbox.isChecked();
}

//============================================================================
//============================================================================
void BUICheckedMenuItemControl::toggleChecked( void )
{
   mCheckbox.check(!mCheckbox.isChecked());
}
