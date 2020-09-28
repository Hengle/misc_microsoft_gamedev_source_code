//============================================================================
// UIObjectiveControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIObjectiveControl : public BUIControl
{
public:
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIObjectiveControl );

public:
   enum 
   {
      cObjectiveTypePrimary=0,
      cObjectiveTypeSecondary,
      cObjectiveTypeTitle,
   };

   enum
   {
      cObjectiveStatusActive,
      cObjectiveStatusCompleted,
      cObjectiveStatusFailed,
   };

   BUIObjectiveControl( void );
   virtual ~BUIObjectiveControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setText(const BUString& text);
   virtual const BUString& getText() const { return mText; }

   virtual void setType(int type);
   virtual int  getType() { return mType; }

   virtual void setStatus(int status);
   virtual int  getStatus() { return mStatus; }

protected:
   int      mType;
   int      mStatus;
   BUString mText;
};