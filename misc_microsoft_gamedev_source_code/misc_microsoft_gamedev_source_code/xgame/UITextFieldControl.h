//============================================================================
// UITextFieldControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUITextFieldControl : public BUIControl
{
public:
   enum Events
   {
      eTextFieldEvent = UITextFieldControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UITextFieldControl );
public:

   BUITextFieldControl( void );
   virtual ~BUITextFieldControl( void );

   // overrides
   virtual void show( bool force=false );
   virtual void hide( bool force=false );


   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }

   virtual void setColor(DWORD color) { mUseColor=true; mColor = color; }
   virtual const DWORD getColor() const { return mColor; }

   virtual void setUseColor(bool value) { mUseColor=value; }
   virtual const bool getUseColor() const {return mUseColor;}

protected:
   BUString mText;
   DWORD    mColor;
   bool     mUseColor:1;
};