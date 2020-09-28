//============================================================================
// UIButtonControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIButtonControl: public BUIControl
{
public:
   enum Events
   {
      ePress = UIButtonControlID,
   };

   BEGIN_EVENT_MAP( UIButtonControl )
      MAP_CONTROL_EVENT( Press )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIButtonControl );
   
public:
   BUIButtonControl( void );
   virtual ~BUIButtonControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }

   virtual void setImage(const char* pImage);
   virtual const char * getImage() const { return mImage.getPtr(); }

   virtual void onPress();

protected:
   BString  mImage;
   BUString mText;
};