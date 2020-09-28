//============================================================================
// UILabelControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUILabelControl : public BUIControl
{
public:
   enum Events
   {
      eLabelEvent = UILabelControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UILabelControl );
public:

   BUILabelControl( void );
   virtual ~BUILabelControl( void );

   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }


protected:
   BUString mText;
};