//============================================================================
// UIGridCellControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIGridCellControl : public BUIControl
{
public:
   enum Events
   {
      ePress = UIGridCellControlID,
   };
   
   BEGIN_EVENT_MAP( UIGridCellControl )
      MAP_CONTROL_EVENT( Press )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIGridCellControl );
public:

   BUIGridCellControl( void );
   virtual ~BUIGridCellControl( void );

   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }

   virtual void setCalloutText(const BUString& text) { mCalloutText = text; }
   virtual const BUString& getCalloutText() const { return mCalloutText; }

   virtual void setCalloutTitle(const BUString& text) { mCalloutTitle = text; }
   virtual const BUString& getCalloutTitle() const { return mCalloutTitle; }


   virtual void setDetailIndicator(bool enable);
   virtual const bool getDetailIndicator() const { return mHasDetail; }


   bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

protected:
   void onPress();

   bool     mHasDetail;
   BUString mCalloutTitle;
   BUString mCalloutText;
   BUString mText;
};