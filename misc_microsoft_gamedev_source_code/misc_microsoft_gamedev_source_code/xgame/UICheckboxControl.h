//============================================================================
// UICheckboxControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUICheckboxControl : public BUIControl
{
public:
   enum Events
   {
      eCheck = UICheckboxControlID,
      eUncheck
   };

   BEGIN_EVENT_MAP( UICheckboxControl )
      MAP_CONTROL_EVENT( Check )
      MAP_CONTROL_EVENT( Uncheck )
   END_EVENT_MAP()
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UICheckboxControl )

public:
   BUICheckboxControl( void );
   virtual ~BUICheckboxControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void check( bool bCheck = true );
   virtual void uncheck( void );

   virtual bool isChecked( void ) const;

protected:
   bool mChecked;
};