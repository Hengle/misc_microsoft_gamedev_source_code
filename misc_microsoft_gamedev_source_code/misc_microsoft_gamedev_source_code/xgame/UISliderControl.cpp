//============================================================================
// UISliderControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UISliderControl.h"

//============================================================================
//============================================================================
BUISliderControl::BUISliderControl( void ) :
   mMin( 0 ),
   mMax( 0 ),
   mStep( 0 ),
   mValue( 0 )
{
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUISliderControl::~BUISliderControl( void )
{
}

//============================================================================
//============================================================================
void BUISliderControl::setMinText( const BUString& minText )
{
   GFxValue args[1];
   args[0].SetStringW( minText.getPtr() );
   invokeActionScript( "setMinText", args, 1 );
}

//============================================================================
//============================================================================
void BUISliderControl::setMaxText( const BUString& maxText )
{
   GFxValue args[1];
   args[0].SetStringW( maxText.getPtr() );
   invokeActionScript( "setMaxText", args, 1 );
}

//============================================================================
//============================================================================
void BUISliderControl::setMin( int minVal )
{
   mMin = minVal;
   if( mValue < mMin )
      setValue( mMin );

   GFxValue values[1];
   values[0].SetNumber( mMin );
   invokeActionScript( "setMin", values, 1 );
}
//============================================================================
//============================================================================
int BUISliderControl::getMin( void ) const
{
   return mMin;
}

//============================================================================
//============================================================================
void BUISliderControl::setMax( int maxVal )
{
   mMax = maxVal;
   if( mValue > mMax )
      setValue( mMax );

   GFxValue values[1];
   values[0].SetNumber( mMax );
   invokeActionScript( "setMax", values, 1 );
}

//============================================================================
//============================================================================
int BUISliderControl::getMax( void ) const
{
   return mMax;
}

//============================================================================
//============================================================================
void BUISliderControl::setStep( int step )
{
   mStep = step;
   if( mValue % mStep != 0 )
      mValue -= mValue % mStep;

   GFxValue values[1];
   values[0].SetNumber( mStep);
   invokeActionScript( "setStep", values, 1 );
}

//============================================================================
//============================================================================
int BUISliderControl::getStep( void ) const
{
   return mStep;
}

//============================================================================
//============================================================================
void BUISliderControl::setValue( int value )
{
   mValue = max( mMin, min( mMax, value ) );
   
   GFxValue values[1];
   values[0].SetNumber( mValue );
   invokeActionScript( "setValue", values, 1 );
   fireUIControlEvent( eValueChanged );
}

//============================================================================
//============================================================================
int BUISliderControl::getValue( void ) const
{
   return mValue;
}

//============================================================================
//============================================================================
void BUISliderControl::stepUp( void )
{
   if( mValue < mMax )
   {
      step( true );
      fireUIControlEvent( eSteppedUp );
   }
}

//============================================================================
//============================================================================
void BUISliderControl::stepDown( void )
{
   if( mValue > mMin )
   {
      step( false );
      fireUIControlEvent( eSteppedDown );
   }
}

//============================================================================
//============================================================================
void BUISliderControl::step( bool bUp )
{
   if( bUp )
   {
      setValue( mValue + mStep );
   }
   else
   {
      setValue( mValue - mStep );
   }
}

//============================================================================
//============================================================================
bool BUISliderControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   bool handled = false;
   
   if( command == "stepUp" )
   {
      stepUp();
      handled = true;
   }
   else if( command == "stepDown" )
   {
      stepDown();
      handled = true;
   }

   return handled;
}