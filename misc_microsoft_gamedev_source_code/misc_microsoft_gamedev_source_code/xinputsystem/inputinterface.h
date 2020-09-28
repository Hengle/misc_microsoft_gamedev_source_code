//==============================================================================
// inputinterface.h
//
// A mapping of functional game inputs to controller keys
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#ifndef __INPUTINTERFACE_H__
#define __INPUTINTERFACE_H__
#pragma once

#include "xmlreader.h"
#include "inputcontrolenum.h"

class BInputEventDetail;

struct BControllerKeyTimes
{
   int64 mDCTime;
   int64 mHTime;

   inline void reset()
   { 
      mDCTime = 0;
      mHTime  = 0;
   }
};

//==============================================================================
// BInputInterface
//==============================================================================
class BInputInterface
{
   public:

      // Controller buttons
      enum BInputControlFlag
      {
         cInputStickLeft           = ( 1 << 0 ),
         cInputStickRight          = ( 1 << 1 ),
         cInputDpadUp              = ( 1 << 2 ),
         cInputDpadDown            = ( 1 << 3 ),
         cInputDpadLeft            = ( 1 << 4 ),
         cInputDpadRight           = ( 1 << 5 ),
         cInputButtonA             = ( 1 << 6 ),
         cInputButtonB             = ( 1 << 7 ),
         cInputButtonX             = ( 1 << 8 ),
         cInputButtonY             = ( 1 << 9 ),
         cInputButtonStart         = ( 1 << 10 ),
         cInputButtonBack          = ( 1 << 11 ),
         cInputButtonShoulderRight = ( 1 << 12 ),
         cInputButtonShoulderLeft  = ( 1 << 13 ),
         cInputButtonThumbLeft     = ( 1 << 14 ),
         cInputButtonThumbRight    = ( 1 << 15 ),
         cInputTriggerLeft         = ( 1 << 16 ),
         cInputTriggerRight        = ( 1 << 17 ),
         cInputDoubleClick         = ( 1 << 18 ),
         cInputHold                = ( 1 << 19 ),
         cInputPrecise             = ( 1 << 20 ),
      };

      // Input functions - Be sure to match enumerations to string map gFunctionNames for proper XML parsing.
      typedef enum BInputFunctions
      {
         cInputActionModifier = 0,
         cInputFlare,
         cInputSpeedModifier,
         cInputStart,
         cInputBack,
         cInputTranslation,
         cInputPan,
         cInputTilt,
         cInputZoom,
         cInputSelection,
         cInputDoubleClickSelect,
         cInputMultiSelect,
         cInputClear,
         cInputDoWork,
         cInputDoWorkQueue,
         cInputAbility,
         cInputAbilityQueue,
         cInputPowers,
         cInputResetCamera,
         cInputAssignGroup1,
         cInputAssignGroup2,
         cInputAssignGroup3,
         cInputAssignGroup4,
         cInputSelectGroup1,
         cInputSelectGroup2,
         cInputSelectGroup3,
         cInputSelectGroup4,
         cInputGotoGroup1,
         cInputGotoGroup2,
         cInputGotoGroup3,
         cInputGotoGroup4,
         cInputGotoBase,
         cInputGotoAlert,
         cInputGotoScout,
         cInputGotoArmy,
         cInputGotoNode,
         cInputGotoHero,
         cInputGotoRally,
         cInputGotoSelected,
         cInputScreenSelect,
         cInputGlobalSelect,
         cInputScreenSelectPrev,
         cInputScreenSelectNext,
         cInputGlobalSelectPrev,
         cInputGlobalSelectNext,
         cInputScreenCyclePrev,
         cInputScreenCycleNext,
         cInputTargetPrev,
         cInputTargetNext,
         cInputSubSelectPrev,
         cInputSubSelectNext,
         cInputSubSelectSquad,
         cInputSubSelectType,
         cInputSubSelectTag,
         cInputSubSelectSelect,
         cInputSubSelectGoto,
         cInputModeGoto,
         cInputModeSubSelectRight,
         cInputModeSubSelectLeft,
         cInputModeGroup,
         cInputModeFlare,
         cInputGroupAdd,
         cInputGroupNext,
         cInputGroupPrev,
         cInputGroupGoto,
         cInputFlareLook,
         cInputFlareHelp,
         cInputFlareMeet,
         cInputFlareAttack,
         cInputMapZoom,
         cInputAttackMove,
         cInputSetRally,
         cInputNoAction1,
         cInputNoAction2,
         cInputDisplayExtraInfo,
         cInputFunctionNum,
      };

      BInputInterface();
      ~BInputInterface(){}

      // Assignment operator.
      BInputInterface& operator = ( const BInputInterface& cSrcII );

      // Equality operator
      bool operator == ( const BInputInterface& cSrcII ) const;      

      // Get function control types
      //void setFunctionControl( BInputFunctions inputFunc, long controlType, bool modifier );
      void getFunctionControl( BInputFunctions inputFunc, long& controlType, bool& modifier, bool& doubleClick, bool& hold );

      // Does the input function match the control type and modifier flag
      bool isFunctionControl( BInputFunctions inputFunc, long controlType, bool modifier, int64 time = 0, BControllerKeyTimes* keyTimes = NULL, bool start = false, bool repeat = false, bool stop = false, bool forceClickOnStop = false, float doubleClickTime = -1.0f, float holdTime = -1.0f, BInputEventDetail* pDetail = NULL );

      // Does this input function need the action modifier flag to be active
      inline bool usesActionModifier( BInputFunctions inputFunc ){ return( ( mInputFuncs[inputFunc] & mInputFuncs[cInputActionModifier] ) ? true : false ); }

      // Does this input function need a double click
      inline bool usesDoubleClick( BInputFunctions inputFunc ){ return( ( mInputFuncs[inputFunc] & cInputDoubleClick ) ? true : false ); }

      // Does this input function need a hold
      inline bool usesHold( BInputFunctions inputFunc ){ return( ( mInputFuncs[inputFunc] & cInputHold ) ? true : false ); }

      bool controlhasModifierFunc(int controlType) const { if (controlType<0 || controlType>=cControlCount) return false; else return mHasModifierFunc[controlType]; }

      // Parse XML
      bool parseXML( BXMLNode configNode );

      // Get configuration name
      inline BSimString getConfigName(){ return( mName ); }

      // Look up function enumeration
      static long lookupFunction( const BCHAR_T* name );

   protected:

      // Translate from control type enum to keys
      DWORD controlTypeToKeys( long controlType, bool modifier, bool doubleClick = false, bool hold = false, bool precise = false );

      // Translate from keys to control type enum
      void keysToControlType( DWORD keys, long& controlType, bool& modifier, bool& doubleClick, bool& hold, bool& precise );

      BSimString mName;
      int64      mTimerFrequency;
      double     mTimerFrequencyFloat;
      DWORD      mInputFuncs[cInputFunctionNum];
      bool       mHasDoubleClickFunc[cControlCount];
      bool       mHasModifierFunc[cControlCount];
};

typedef BSmallDynamicSimArray<BInputInterface> BInputInterfaceArray;

#endif //__INPUTINTERFACE_H__