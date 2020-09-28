//============================================================================
// UIScrollableTextFieldControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIScrollableTextFieldControl: public BUIControl
{
public:
   enum Events
   {
      eBounced = UIScrollableTextFieldControlID,
   };
   
   BEGIN_EVENT_MAP( UIScrollableTextFieldControl )
      MAP_CONTROL_EVENT( Bounced )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIScrollableTextFieldControl );

public:
   BUIScrollableTextFieldControl( void );
   virtual ~BUIScrollableTextFieldControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }

   virtual void startScrollUp();
   virtual void startScrollDown();
   virtual void stopScroll();

   virtual void setBounce(bool bBounce);
   virtual bool getBounce() const { return mBounce; }


protected:
   BUString mText;
   bool     mBounce : 1;
};