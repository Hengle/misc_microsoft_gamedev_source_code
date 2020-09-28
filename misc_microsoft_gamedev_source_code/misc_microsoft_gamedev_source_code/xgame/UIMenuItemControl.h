//============================================================================
// UIMenuItemControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIMenuItemControl: public BUIControl
{
   enum Events
   {
      eMenuItemControlEvent = UIMenuItemControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIMenuItemControl );

public:
   BUIMenuItemControl( void );
   virtual ~BUIMenuItemControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }

   virtual int getData() const { return mData; }
   virtual void setData(int data) { mData = data; }

   virtual void setHelpText(const BUString& helpText) { mHelpText = helpText; }
   virtual const BUString& getHelpText() const { return mHelpText; }

protected:
   BUString mText;
   BUString mHelpText;
   int      mData;
};