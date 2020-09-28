//============================================================================
// UIImageViewerControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIImageViewerControl.h"

//============================================================================
//============================================================================
BUIImageViewerControl::BUIImageViewerControl( void ) : 
   mViewDuration(5000.0f),
   mImageWidth(200),
   mImageHeight(100),
   mbAutoTransition(true)
{
   mControlType.set("UIImageViewerControl");
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUIImageViewerControl::~BUIImageViewerControl( void )
{
}

//============================================================================
//============================================================================
bool BUIImageViewerControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   return true;
}

//============================================================================
//============================================================================
void BUIImageViewerControl::setAutoTransition(bool bAutoTransition)
{
   mbAutoTransition = bAutoTransition;

   GFxValue value;
   value.SetBoolean(mbAutoTransition);

   invokeActionScript( "setAutoTransition", &value, 1);
}

//============================================================================
//============================================================================
void BUIImageViewerControl::showNextPicture()
{
   invokeActionScript( "next");
}

//============================================================================
//============================================================================
void BUIImageViewerControl::addImage(const char* url)
{
   GFxValue value;
   value.SetString(url);

   invokeActionScript( "addImage", &value, 1);
}

//============================================================================
//============================================================================
void BUIImageViewerControl::setImageSize(int width, int height)
{
   GFxValue value[2];
   value[0].SetNumber(width);
   value[1].SetNumber(height);

   invokeActionScript( "setImageSize", value, 2);
}

//============================================================================
//============================================================================
void BUIImageViewerControl::reset()
{
   invokeActionScript( "reset");
}

//============================================================================
//============================================================================
void BUIImageViewerControl::start()
{
   // This is deprecated
}

//============================================================================
//============================================================================
void BUIImageViewerControl::clearImages()
{
   invokeActionScript( "clearImages");
}

//============================================================================
//============================================================================
void BUIImageViewerControl::setViewDuration(float duration)
{
   GFxValue value;
   value.SetNumber(duration);

   invokeActionScript( "setViewDuration", &value, 1);
}
