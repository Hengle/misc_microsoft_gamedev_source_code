//============================================================================
// UIInfoDialog.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "UIScreen.h"
#include "UILabelControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UITextFieldControl.h"

class BUIInfoDialog : public BUIScreen
{
public:

   BUIInfoDialog( void );
   virtual ~BUIInfoDialog( void );

   virtual bool init( BXMLNode dataNode );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

   void setData( const BUString& titleText, const BUString& bodyText, const BString& voCue, const BString& imagePath );

   virtual void enter( void );
   virtual void update( float dt );
   virtual void leave( void );

   enum EResult { eResult_Close };

protected:

   float mButtonDelay;
   float mButtonDelayCounter;
   BUILabelControl mTitleText;
   BUIScrollableTextFieldControl mBodyTextNoImage;
   BUIScrollableTextFieldControl mBodyTextWithImage;
   BUIScrollableTextFieldControl* pBodyText;
   BUIImageViewerControl mImage;
   BUITextFieldControl mButtonText;
};