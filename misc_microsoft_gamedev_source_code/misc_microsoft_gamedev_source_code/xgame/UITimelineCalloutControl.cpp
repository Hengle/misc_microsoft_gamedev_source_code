//============================================================================
// UITimelineCalloutControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UITimelineCalloutControl.h"

#include "player.h"
#include "usermanager.h"
#include "user.h"
#include "scoremanager.h"


//============================================================================
//============================================================================
BUITimelineCalloutControl::BUITimelineCalloutControl( void )
{
}

//============================================================================
//============================================================================
BUITimelineCalloutControl::~BUITimelineCalloutControl( void )
{
}


//============================================================================
//============================================================================
bool BUITimelineCalloutControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   bool result = false;

   result = __super::init( parent, controlPath, controlID, initData );

   mTitle.init(this, mScriptPrefix+"mTitle", -1, NULL);
   // mImage.init(this, mScriptPrefix+"mImage", -1, NULL);



   return result;
}

//============================================================================
//============================================================================
bool BUITimelineCalloutControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = __super::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   return handled;
}

//============================================================================
//============================================================================
bool BUITimelineCalloutControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="scrollUp")
   {
      if (event == cInputEventCommandStart)
         startScrollDown();      // reversed to be consistent with the rest of the UI
      else if (event == cInputEventCommandStop)
         stopScroll();

      return true;
   }
   else if (command=="scrollDown")
   {
      if (event == cInputEventCommandStart)
         startScrollUp();        // reversed to be consistent with the rest of the UI
      else if (event == cInputEventCommandStop)
         stopScroll();
      return true;
   }

   return false;
}

//============================================================================
//============================================================================
void BUITimelineCalloutControl::startScrollUp()
{
   invokeActionScript("startScrollUp");
}

//============================================================================
//============================================================================
void BUITimelineCalloutControl::startScrollDown()
{
   invokeActionScript("startScrollDown");
}

//============================================================================
//============================================================================
void BUITimelineCalloutControl::stopScroll()
{
   invokeActionScript("stopScroll");
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUITimelineCalloutControl::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   bool handled = false;

   return handled;

}


//============================================================================
//============================================================================
void BUITimelineCalloutControl::setTitle(const BUString& title)
{
   mTitle.setText(title);
}


//============================================================================
//============================================================================
void BUITimelineCalloutControl::setDescription(const BUString& description)
{

   mDescription=description;

   GFxValue value;
   value.SetStringW(mDescription.getPtr());
   invokeActionScript("setText", &value, 1);
}

//============================================================================
//============================================================================
/*
void BUITimelineCalloutControl::setImage(const char* imageURL)
{
   mImage.clearImages();
   mImage.addImage(imageURL);
   mImage.start();
}
*/
