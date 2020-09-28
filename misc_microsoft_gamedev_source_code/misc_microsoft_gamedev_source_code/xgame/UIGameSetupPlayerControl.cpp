//============================================================================
// UIGameSetupPlayerControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIGameSetupPlayerControl.h"

//============================================================================
//============================================================================
BUIGameSetupPlayerControl::BUIGameSetupPlayerControl( void )
{
   mControlType.set("UIGameSetupPlayerControl");
}

//============================================================================
//============================================================================
BUIGameSetupPlayerControl::~BUIGameSetupPlayerControl( void )
{
}

//============================================================================
//============================================================================
bool BUIGameSetupPlayerControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData /*= NULL*/ )
{
   if (!__super::init( parent, controlPath, controlID, initData))
      return false;

   // init the player control
   mGamerTag.init(parent, this->getControlPath()+".mPlayer");

   return true;
}

//============================================================================
//============================================================================
void BUIGameSetupPlayerControl::focus( bool force)
{
   __super::focus(force);

   mGamerTag.focus(force);
}

//============================================================================
//============================================================================
void BUIGameSetupPlayerControl::unfocus( bool force)
{
   __super::unfocus(force);

   mGamerTag.unfocus(force);
}


//============================================================================
//============================================================================
void BUIGameSetupPlayerControl::playTransition(const char * transitionName)
{
   GFxValue values[1];
   values[0].SetString(transitionName);

   invokeActionScript("playTransition", values, 1);
}

//============================================================================
//============================================================================
void BUIGameSetupPlayerControl::showSpinner(bool bShow)
{
   GFxValue values[1];
   values[0].SetBoolean(bShow);

   invokeActionScript("showSpinner", values, 1);
}

