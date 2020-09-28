//============================================================================
// BUITwoColumnScrollTextFieldControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUITwoColumnScrollTextFieldControl: public BUIControl
{
public:
   enum Events
   {
      eBounced = UITwoColumnScrollTextFieldControlID,
   };
   
   BEGIN_EVENT_MAP( UITwoColumnScrollTextFieldControl )
      MAP_CONTROL_EVENT( Bounced )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UITwoColumnScrollTextFieldControl );

public:
   BUITwoColumnScrollTextFieldControl( void );
   virtual ~BUITwoColumnScrollTextFieldControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setText(const BUString& textColumn0, const BUString& textColum1);   
   virtual void appendTextColumn0(const BUString& textColumn);
   virtual void appendTextColumn1(const BUString& textColumn);
   virtual void updateTextBegin();
   virtual void updateTextEnd();
   virtual const BUString& getTextColumn0() const { return mTextColumn0; }
   virtual const BUString& getTextColumn1() const { return mTextColumn1; }

   virtual void startScrollUp();
   virtual void startScrollDown();
   virtual void stopScroll();

   virtual void setBounce(bool bBounce);
   virtual bool getBounce() const { return mBounce; }


protected:
   BUString mTextColumn0;
   BUString mTextColumn1;
   bool     mBounce : 1;
};