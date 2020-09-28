//==============================================================================
// inputcontrolenum.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

#pragma once

// Base controls
enum BInputBaseType
{
   cBaseControlX,
   cBaseControlY,
   cBaseControlZ,
   cBaseControlRX,
   cBaseControlRY,
   cBaseControlRZ,
   cBaseControlPOV,
   cBaseControlSlider,
   cBaseControlButton,
   cBaseControlCount
};

// POV directions
enum BInputPovType
{
   cPovDirUp,
   cPovDirRight,
   cPovDirDown,
   cPovDirLeft,
   cPovDirCount
};

// Input controls
enum BInputControlType
{
   // Gamepad controls
   cStickLeftUp,
   cStickLeftDown,
   cStickLeftLeft,
   cStickLeftRight,
   cStickRightUp,
   cStickRightDown,
   cStickRightLeft,
   cStickRightRight,
   cDpadUp,
   cDpadDown,
   cDpadLeft,
   cDpadRight,
   cButtonA,
   cButtonB,
   cButtonX,
   cButtonY,
   cButtonStart,
   cButtonBack,
   cButtonShoulderRight,
   cButtonShoulderLeft,
   cButtonThumbLeft,
   cButtonThumbRight,
   cTriggerLeft,
   cTriggerRight,

   // Combo controls
   cStickLeft,
   cStickRight,
   cDpad,
   cTriggers,

   // Gamepad insert and remove
   cGamepadInsert,
   cGamepadRemove,

   // Keyboard controls
   cKeyBackSpace,
   cKeyTab,
   cKeyEnter,
   cKeyShift,
   cKeyCtrl,
   cKeyAlt,
   cKeyPause,
   cKeyEscape,
   cKeySpace,
   cKeyPageUp,
   cKeyPageDown,
   cKeyEnd,
   cKeyHome,
   cKeyLeft,
   cKeyUp,
   cKeyRight,
   cKeyDown,
   cKeyPrtSc,
   cKeyInsert,
   cKeyDelete,
   cKey0,
   cKey1,
   cKey2,
   cKey3,
   cKey4,
   cKey5,
   cKey6,
   cKey7,
   cKey8,
   cKey9,
   cKeyA,
   cKeyB,
   cKeyC,
   cKeyD,
   cKeyE,
   cKeyF,
   cKeyG,
   cKeyH,
   cKeyI,
   cKeyJ,
   cKeyK,
   cKeyL,
   cKeyM,
   cKeyN,
   cKeyO,
   cKeyP,
   cKeyQ,
   cKeyR,
   cKeyS,
   cKeyT,
   cKeyU,
   cKeyV,
   cKeyW,
   cKeyX,
   cKeyY,
   cKeyZ,
   cKeyAccent,
   cKeyMultiply,
   cKeyAdd,
   cKeySeparator,
   cKeySubtract,
   cKeyDecimal,
   cKeyDivide,
   cKeyF1,
   cKeyF2,
   cKeyF3,
   cKeyF4,
   cKeyF5,
   cKeyF6,
   cKeyF7,
   cKeyF8,
   cKeyF9,
   cKeyF10,
   cKeyF11,
   cKeyF12,
   cKeyF16,
   cKeyShiftLeft,
   cKeyShiftRight,
   cKeyCtrlLeft,
   cKeyCtrlRight,
   cKeyAltLeft,
   cKeyAltRight,
   cKeyGreenModifier,
   cKeyOrangeModifier,

   // Control count
   cControlCount,

   // First, last, and count of various types of controls
   cGamepadControlCount=cGamepadRemove+1,

   cFirstButton=cButtonA,
   cLastButton=cButtonThumbRight,
   cButtonCount=cLastButton-cFirstButton+1,

   cFirstKey=cKeyBackSpace,
   cLastKey=cKeyOrangeModifier,
   cKeyCount=cLastKey-cFirstKey+1,

   cFirstSingleControl=cStickLeftUp,
   cLastSingleControl=cTriggerRight,
   cSingleControlCount=cLastSingleControl-cFirstSingleControl+1,

   cFirstComboControl=cStickLeft,
   cLastComboControl=cTriggers,
   cComboControlCount=cLastComboControl-cFirstComboControl+1,
};

// Control keys that double-clicks and press-and-holds are listened on
enum BDoubleClickControlType
{
   cButtonShoulderLeftDC = 0,
   cButtonShoulderRightDC,
   cDPadUpDC,
   cDPadRightDC,
   cDPadDownDC,
   cDPadLeftDC,
   cButtonYDC,
   cButtonBDC,
   cButtonADC,
   cButtonXDC,
   cButtonThumbLeftDC,
   cButtonThumbRightDC,
   cTriggerLeftDC,
   cTriggerRightDC,
   cNumDC,
};

// Functions
long lookupBaseControl(const BCHAR_T* name);
long lookupPovDir(const BCHAR_T* name);
long lookupControl(const BCHAR_T* name);
long getDoubleClickFromControlType( long controlType );
long getControlTypeFromDoubleClick( long index );
