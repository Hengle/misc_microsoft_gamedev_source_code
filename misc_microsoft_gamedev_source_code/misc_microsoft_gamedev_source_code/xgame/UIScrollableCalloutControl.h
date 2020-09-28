//============================================================================
// UIScrollableTextFieldControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIScrollableCalloutControl : public BUIControl
{
public:
   enum Events
   {
      eBounced = UIScrollableCalloutControlID,
      eCancel,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIScrollableCalloutControl );

public:
   BUIScrollableCalloutControl( void );
   virtual ~BUIScrollableCalloutControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }

   virtual void setTitle(const BUString& text);
   virtual const BUString& getTitle() const { return mText; }

   virtual void startScrollUp();
   virtual void startScrollDown();
   virtual void stopScroll();

   virtual void setBounce(bool bBounce);
   virtual bool getBounce() const { return mBounce; }

protected:
   BUString mText;
   BUString mTitle;
   bool     mBounce : 1;
};