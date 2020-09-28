//==============================================================================
// inputinterface.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "config.h"
#include "configsinput.h"
#include "inputinterface.h"
#include "inputcontrolenum.h"
#include "inputsystem.h"

// Function names
const BCHAR_T* gFunctionNames[] =
{
   B( "ActionModifier" ),
   B( "Flare" ),
   B( "SpeedModifier" ),
   B( "Start" ),
   B( "Back" ),
   B( "Translation" ),
   B( "Pan" ),
   B( "Tilt" ),
   B( "Zoom" ),
   B( "Selection" ),
   B( "DoubleClickSelect" ),
   B( "MultiSelect" ),
   B( "Clear" ),
   B( "DoWork" ),
   B( "DoWorkQueue" ),
   B( "Ability" ),
   B( "AbilityQueue" ),
   B( "Powers" ),
   B( "ResetCamera" ),
   B( "AssignGroup1" ),
   B( "AssignGroup2" ),
   B( "AssignGroup3" ),
   B( "AssignGroup4" ),
   B( "SelectGroup1" ),
   B( "SelectGroup2" ),
   B( "SelectGroup3" ),
   B( "SelectGroup4" ),
   B( "GotoGroup1" ),
   B( "GotoGroup2" ),
   B( "GotoGroup3" ),
   B( "GotoGroup4" ),
   B( "GotoBase" ),
   B( "GotoAlert" ),
   B( "GotoScout" ),
   B( "GotoArmy" ),
   B( "GotoNode" ),
   B( "GotoHero" ),
   B( "GotoRally" ),
   B( "GotoSelected" ),
   B( "ScreenSelect" ),
   B( "GlobalSelect" ),
   B( "ScreenSelectPrev" ),
   B( "ScreenSelectNext" ),
   B( "GlobalSelectPrev" ),
   B( "GlobalSelectNext" ),
   B( "ScreenCyclePrev" ),
   B( "ScreenCycleNext" ),
   B( "TargetPrev" ),
   B( "TargetNext" ),
   B( "SubSelectPrev" ),
   B( "SubSelectNext" ),
   B( "SubSelectSquad" ),
   B( "SubSelectType" ),
   B( "SubSelectTag" ),
   B( "SubSelectSelect" ),
   B( "SubSelectGoto" ),
   B( "ModeGoto" ),
   B( "ModeSubSelectRight" ),
   B( "ModeSubSelectLeft" ),
   B( "ModeGroup" ),
   B( "ModeFlare" ),
   B( "GroupAdd" ),
   B( "GroupNext" ),
   B( "GroupPrev" ),
   B( "GroupGoto" ),
   B( "FlareLook" ),
   B( "FlareHelp" ),
   B( "FlareMeet" ),
   B( "FlareAttack" ),
   B( "MapZoom" ),
   B( "AttackMove" ),
   B( "SetRally" ),
   B( "NoAction1" ),
   B( "NoAction2" ),
   B( "ExtraInfo" )
};

//==============================================================================
// BInputInterface::BInputInterface
//==============================================================================
BInputInterface::BInputInterface()
{ 
   mName.format( "" );

   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);
   mTimerFrequency = freq.QuadPart;
   mTimerFrequencyFloat = (double)mTimerFrequency;

   memset( mInputFuncs, 0, sizeof( DWORD ) * cInputFunctionNum ); 
   memset( mHasDoubleClickFunc, 0, sizeof( bool ) * cControlCount );
   memset( mHasModifierFunc, 0, sizeof( bool ) * cControlCount );
}

//==============================================================================
// BInputInterface::BInputInterface operator =
//==============================================================================
BInputInterface& BInputInterface::operator = ( const BInputInterface& cSrcII )
{
   mName = cSrcII.mName;

   mTimerFrequency = cSrcII.mTimerFrequency;
   mTimerFrequencyFloat = cSrcII.mTimerFrequencyFloat;

   memcpy( mInputFuncs, cSrcII.mInputFuncs, sizeof( DWORD ) * cInputFunctionNum );
   memcpy( mHasDoubleClickFunc, cSrcII.mHasDoubleClickFunc, sizeof( bool ) * cControlCount );
   memcpy( mHasModifierFunc, cSrcII.mHasModifierFunc, sizeof( bool ) * cControlCount );

   return( *this );
}

//==============================================================================
// BInputInterface::operator ==
//==============================================================================
bool BInputInterface::operator == ( const BInputInterface& cSrcII ) const
{
   if( mName != cSrcII.mName )
   {
      return( false );
   }

   for( long i = 0; i < cInputFunctionNum; i++ )
   {
      if( mInputFuncs[i] != cSrcII.mInputFuncs[i] )
      {
         return( false );
      }
   }

   return( true );
}

//==============================================================================
// BInputInterface::setFunctionControl
//==============================================================================
//void BInputInterface::setFunctionControl( BInputFunctions inputFunc, long controlType, bool modifier )
//{
//   mInputFuncs[inputFunc] = controlTypeToKeys( controlType, modifier );
//}

//==============================================================================
// BInputInterface::getFunctionControl
//==============================================================================
void BInputInterface::getFunctionControl( BInputFunctions inputFunc, long& controlType, bool& modifier, bool& doubleClick, bool& hold )
{
   bool precise = false;
   keysToControlType( mInputFuncs[inputFunc], controlType, modifier, doubleClick, hold, precise );
}

//==============================================================================
// BInputInterface::isFunctionControl
//==============================================================================
bool BInputInterface::isFunctionControl( BInputFunctions inputFunc, long controlType, bool modifier, int64 time /*= 0*/, BControllerKeyTimes* keyTimes /*= NULL*/, bool start /*= false*/, bool repeat /*= false*/, bool stop /*= false*/, bool forceClickOnStop /*= false*/, float doubleClickTime /*= -1.0f*/, float holdTime /*= -1.0f*/, BInputEventDetail* pDetail /*= NULL*/ )
{
   long testControlType = -1;
   bool testModifier    = false;
   bool testDoubleClick = false;
   bool testHold        = false;
   bool testPrecise     = false;
   bool result          = false;   

   // Extract the test control type, and the modifier, the double click, and the hold flags from the function's keys
   keysToControlType( mInputFuncs[inputFunc], testControlType, testModifier, testDoubleClick, testHold, testPrecise );

   // Does this function match the key and modifier?
   if( ( controlType == testControlType ) && ( modifier == testModifier || !mHasModifierFunc[controlType] ) )
   {
      bool isPrecise = true;
      if (testPrecise && pDetail)
      {
         switch (controlType)
         {
            case cDpadLeft: 
            case cDpadRight:
               if (pDetail->mY != 0.0f) 
                  isPrecise = false; 
               break;
            case cDpadUp:
            case cDpadDown:
               if (pDetail->mX != 0.0f)
                  isPrecise = false; 
               break;
         }
      }

      long  index      = getDoubleClickFromControlType( controlType ); // Extract proper enumerated index for keys that accept double clicks and holds
      bool  validTimes = ( keyTimes && ( index != -1 ) );

      // Do we need to test for a double click?
      if( validTimes && testDoubleClick && start && ( keyTimes[index].mDCTime != 0 ) )
      {
         keyTimes[index].mHTime = time;

         float elapsed = (float)((time - keyTimes[index].mDCTime)/mTimerFrequencyFloat);
         float dcTime = 0.25f;
         if( doubleClickTime < 0.0f )
            gConfig.get( cConfigGamepadDoubleClickTime, &dcTime );
         else
            dcTime = doubleClickTime;
         if( elapsed <= dcTime )
         {
            keyTimes[index].mDCTime = 0;
            result = true;
         }         
      }
      // Do we need to test for a hold?
      else if( validTimes && testHold && repeat && ( keyTimes[index].mHTime != 0 ) )
      {
         float elapsed = (float)((time - keyTimes[index].mHTime)/mTimerFrequencyFloat);
         float hTime = 0.25f;
         if( holdTime < 0.0f )
            gConfig.get( cConfigGamepadHoldTime, &hTime );
         else
            hTime = holdTime;
         if( elapsed > hTime )
         {
            keyTimes[index].mHTime = 0;
            result = true;
         }         
      }
      // On a start set times and register a click if valid
      else if( validTimes && !testHold && !testDoubleClick && start )
      {
         keyTimes[index].mHTime = time;

         if( mHasDoubleClickFunc[controlType] ) // is ANY function assigned to a double click for this control type?
         {
            // If click on start and double click time has been reset
            if( !forceClickOnStop && ( keyTimes[index].mDCTime == 0 ) )
            {
               result = true;
            }
            // If click on start and valid double click time
            else if( !forceClickOnStop )
            {
               float elapsed = (float)((time - keyTimes[index].mDCTime)/mTimerFrequencyFloat);
               float dcTime = 0.25f;
               if( doubleClickTime < 0.0f )
                  gConfig.get( cConfigGamepadDoubleClickTime, &dcTime );
               else
                  dcTime = doubleClickTime;
               // If double click time has expired
               if( elapsed > dcTime )
               {
                  result = true;   
               }
            }
         }
         else if( !forceClickOnStop )
         {
            result = true;
         }

         keyTimes[index].mDCTime = time;
      }      
      // On a stop set times and register a click if valid
      else if( validTimes && !testHold && !testDoubleClick && stop && ( keyTimes[index].mHTime != 0 ) )
      {         
         keyTimes[index].mHTime  = 0;

         // If click on stop
         if( forceClickOnStop )
         {
            result = true;
         }
      }
      // Ignore time on key
      else if( !validTimes )
      {
         result = true;
      }

      if (result && testPrecise && !isPrecise)
         result = false;
   }

   return( result );
}

//==============================================================================
// BInputInterface::parseXML
//==============================================================================
bool BInputInterface::parseXML( BXMLNode configNode )
{
   BSimString configName;
   if( configNode.getAttribValue( "name", &configName ) )
   {
      mName = configName;
   }

   long numChildren = configNode.getNumberChildren();
   for( long i = 0; i < numChildren; i++ )
   {
      const BXMLNode      child( configNode.getChild( i ) );
      const BPackedString nodeName( child.getName() );
      if( nodeName == B( "Function" ) )
      {
         BSimString funcName;
         if( child.getAttribValue( "name", &funcName ) )
         {
            long funcIndex = lookupFunction( funcName.getPtr() );            
            if( ( funcIndex < 0 ) || ( funcIndex >= cInputFunctionNum ) )
            {
               BASSERTM( 0, "Function name not recognized! Is the data in your work directory up to date with code?" );
               return( false );
            }
            mInputFuncs[funcIndex] = 0;

            long numKeys = child.getNumberChildren();
            for( long j = 0; j < numKeys; j++ )
            {
               const BXMLNode  keyNode( child.getChild( j ) );               
               BSimString      temp;
               BSimString      keyName     = keyNode.getTextPtr( temp );
               long            controlType = lookupControl( keyName );               
               DWORD           key         = 0;
               if( controlType == -1 )
               {
                  if( keyName == "DoubleClick" )
                  {
                     key = cInputDoubleClick;
                  }
                  else if( keyName == "Hold" )
                  {
                     key = cInputHold;
                  }
                  else if( keyName == "Precise" )
                  {
                     key = cInputPrecise;
                  }
               }
               else
               {
                  key = controlTypeToKeys( controlType, false );
               }
               
               if( key == 0 )
               {
                  BASSERTM( 0, "Control type not recognized!" );
                  return( false );
               }

               mInputFuncs[funcIndex] |= key;
            }            
         }
      }      
   }

   for( uint i = 0; i < cInputFunctionNum; i++ )
   {
      long controlType = -1;
      bool modifier = false, doubleClick = false, hold = false, precise = false;
      keysToControlType( mInputFuncs[i], controlType, modifier, doubleClick, hold, precise );
      if( controlType != -1 && doubleClick )
         mHasDoubleClickFunc[controlType] = true;
      if( controlType != -1 && modifier )
         mHasModifierFunc[controlType] = true;
   }

   return( true );
}

//==============================================================================
// BInputInterface::controlTypeToKeys
//==============================================================================
DWORD BInputInterface::controlTypeToKeys( long controlType, bool modifier, bool doubleClick /*= false*/, bool hold /*= false*/, bool precise /*= false*/ )
{
   DWORD keys = 0;

   if( modifier )
   {
      keys = mInputFuncs[cInputActionModifier];
   }

   if( doubleClick )
   {
      keys |= cInputDoubleClick;
   }

   if( hold )
   {
      keys |= cInputHold;
   }

   switch( controlType )
   {
      case cStickLeft:
         keys |= cInputStickLeft;
         break;

      case cStickRight:
         keys |= cInputStickRight;
         break;

      case cDpadUp:
         keys |= cInputDpadUp;
         break;

      case cDpadDown:
         keys |= cInputDpadDown;
         break;

      case cDpadLeft:
         keys |= cInputDpadLeft;
         break;

      case cDpadRight:
         keys |= cInputDpadRight;
         break;

      case cButtonA:
         keys |= cInputButtonA;
         break;

      case cButtonB:
         keys |= cInputButtonB;
         break;

      case cButtonX:
         keys |= cInputButtonX;
         break;

      case cButtonY:
         keys |= cInputButtonY;
         break;

      case cButtonStart:
         keys |= cInputButtonStart;
         break;

      case cButtonBack:
         keys |= cInputButtonBack;
         break;

      case cButtonShoulderRight:
         keys |= cInputButtonShoulderRight;
         break;

      case cButtonShoulderLeft:
         keys |= cInputButtonShoulderLeft;
         break;

      case cButtonThumbLeft:
         keys |= cInputButtonThumbLeft;
         break;

      case cButtonThumbRight:
         keys |= cInputButtonThumbRight;
         break;

      case cTriggerLeft:
         keys |= cInputTriggerLeft;
         break;

      case cTriggerRight:
         keys |= cInputTriggerRight;
         break;
   }

   return( keys );
}

//==============================================================================
// BInputInterface::keysToControlType
//==============================================================================
void BInputInterface::keysToControlType( DWORD keys, long& controlType, bool& modifier, bool& doubleClick, bool& hold, bool& precise )
{   
   DWORD controlKey = 0;
   
   if( mInputFuncs[cInputActionModifier] == keys )
   {
      modifier    = false;
      doubleClick = false;
      hold        = false;
      precise     = false;
      controlKey  = mInputFuncs[cInputActionModifier];
   }
   else
   {      
      modifier    =  ( keys & mInputFuncs[cInputActionModifier] ) ? true : false;
      controlKey  =  keys & ~mInputFuncs[cInputActionModifier];      
      doubleClick =  ( keys & cInputDoubleClick ) ? true : false;
      controlKey  &= ~cInputDoubleClick;
      hold        =  ( keys & cInputHold ) ? true : false;
      controlKey  &= ~cInputHold;
      precise     =  ( keys & cInputPrecise ) ? true : false;
      controlKey  &= ~cInputPrecise;
   }

   controlType = -1;

   switch( controlKey )
   {
      case cInputStickLeft:
         controlType = cStickLeft;
         break;

      case cInputStickRight:
         controlType = cStickRight;
         break;

      case cInputDpadUp:
         controlType = cDpadUp;
         break;

      case cInputDpadDown:
         controlType = cDpadDown;
         break;

      case cInputDpadLeft:
         controlType = cDpadLeft;
         break;

      case cInputDpadRight:
         controlType = cDpadRight;
         break;

      case cInputButtonA:
         controlType = cButtonA;
         break;

      case cInputButtonB:
         controlType = cButtonB;
         break;

      case cInputButtonX:
         controlType = cButtonX;
         break;

      case cInputButtonY:
         controlType = cButtonY;
         break;

      case cInputButtonStart:
         controlType = cButtonStart;
         break;

      case cInputButtonBack:
         controlType = cButtonBack;
         break;

      case cInputButtonShoulderRight:
         controlType = cButtonShoulderRight;
         break;

      case cInputButtonShoulderLeft:
         controlType = cButtonShoulderLeft;
         break;

      case cInputButtonThumbLeft:
         controlType = cButtonThumbLeft;
         break;

      case cInputButtonThumbRight:
         controlType = cButtonThumbRight;
         break;

      case cInputTriggerLeft:
         controlType = cTriggerLeft;
         break;

      case cInputTriggerRight:
         controlType = cTriggerRight;
         break;
   }
}

//==============================================================================
// BInputInterface::lookupFunction
//==============================================================================
long BInputInterface::lookupFunction( const BCHAR_T* name )
{
   // Look through list for a match.
   for( long i = 0; i < cInputFunctionNum; i++ )
   {
      if( strCompare( gFunctionNames[i], 100, name, 100 ) == 0 )
      {
         return( i );
      }
   }

   // Didn't find a match, so return -1.
   return( -1 );
}