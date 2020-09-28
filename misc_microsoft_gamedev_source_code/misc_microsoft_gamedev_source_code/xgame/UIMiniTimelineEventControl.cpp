//============================================================================
// UIMiniTimelineEventControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIMiniTimelineEventControl.h"


//============================================================================
//============================================================================
BUIMiniTimelineEventControl::BUIMiniTimelineEventControl( void ) :
   mState(cMiniTimelineStateOff)
{
}

//============================================================================
//============================================================================
BUIMiniTimelineEventControl::~BUIMiniTimelineEventControl( void )
{
}


//============================================================================
//============================================================================
bool BUIMiniTimelineEventControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   bool result = false;

   result = __super::init( parent, controlPath, controlID, initData );

   return result;
}


//============================================================================
//============================================================================
void BUIMiniTimelineEventControl::setState(int state)
{
   if (mState == state)
      return;

   mState = state;

   switch (mState)
   {
      case cMiniTimelineStateOff:
         mKeyframe.set("off");
         break;
      case cMiniTimelineStateUnlocked:
         mKeyframe.set("unlocked");
         break;
      case cMiniTimelineStateNew:
         mKeyframe.set("new");
         break;
      default:
         mKeyframe.set("off");
   }

   GFxValue value;
   value.SetString(mKeyframe.getPtr());

   invokeActionScript("gotoAndPlay", &value, 1);
}
