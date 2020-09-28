//============================================================================
// UISliderControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "UIControl.h"

class BUISliderControl : public BUIControl
{
public:
   enum Events
   {
      eSteppedUp = UISliderControlID,
      eSteppedDown,
      eValueChanged,
      eStopBegin,
      eStopEnd
   };

   BEGIN_EVENT_MAP( UISliderControl )
      MAP_CONTROL_EVENT( ValueChanged )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UISliderControl );

public:
   BUISliderControl( void );
   virtual ~BUISliderControl( void );

   virtual void setMinText( const BUString& minText );
   virtual void setMaxText( const BUString& maxText );

   virtual void setMin( int minVal );
   virtual int getMin( void ) const;

   virtual void setMax( int maxVal );
   virtual int getMax( void ) const;

   virtual void setStep( int step );
   virtual int getStep( void ) const;

   virtual void setValue( int value );
   virtual int getValue( void ) const;

   virtual void stepUp( void );
   virtual void stepDown( void );
   virtual void step( bool bUp );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   
protected:

   int mMin, mMax, mStep, mValue;

};