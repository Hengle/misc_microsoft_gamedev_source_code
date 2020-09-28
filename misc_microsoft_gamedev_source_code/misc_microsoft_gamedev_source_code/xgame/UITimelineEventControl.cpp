//============================================================================
// UITimelineCalloutControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UITimelineEventControl.h"


//============================================================================
//============================================================================
BUITimelineEventControl::BUITimelineEventControl( void ) : 
mTimelinePosition(0.0f),
mState(cTimelineEventStateOff),
mEventIndex(0)
{
}

//============================================================================
//============================================================================
BUITimelineEventControl::~BUITimelineEventControl( void )
{
}


//============================================================================
//============================================================================
bool BUITimelineEventControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   bool result = false;

   result = __super::init( parent, controlPath, controlID, initData );

   mImage.init(this, mScriptPrefix+"mImage", -1, NULL);

   return result;
}

//============================================================================
//============================================================================
void BUITimelineEventControl::initMiniEvent(const char * path)
{
   mMiniEvent.init(this, path);
}


//============================================================================
//============================================================================
bool BUITimelineEventControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = __super::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   return handled;
}

//============================================================================
//============================================================================
bool BUITimelineEventControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   fireUIControlEvent( command.getPtr() );
   return true;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUITimelineEventControl::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   bool handled = false;

   return handled;
}

//============================================================================
//============================================================================
void BUITimelineEventControl::setImage(const char* imageURL)
{
   mImageURL.set(imageURL);

   mImage.clearImages();
   mImage.addImage(imageURL);
   mImage.start();
}

//============================================================================
//============================================================================
void BUITimelineEventControl::setState(int state)
{
   if (mState == state)
      return;

   mState = state;

   switch (mState)
   {
   case cTimelineEventStateOff:
      mKeyframe.set("off");
      break;
   case cTimelineEventStateOn:
      break;
   case cTimelineEventStateNew:
      mKeyframe.set("new");
      break;
   default:
      mKeyframe.set("on");
      break;
   }

   GFxValue value;
   value.SetString(mKeyframe.getPtr());

   invokeActionScript("gotoAndPlay", &value, 1);
}
