//==============================================================================
// inputevent.cpp
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "inputevent.h"

//==============================================================================
// BInputEvent::BInputEvent
//==============================================================================
BInputEvent::BInputEvent()
{
}

//==============================================================================
// BInputEvent::~BInputEvent
//==============================================================================
BInputEvent::~BInputEvent()
{
}

//==============================================================================
// BInputEvent::clear
//==============================================================================
void BInputEvent::clear(void)
{
   mFlags.zero();
   mContext.setNumber(0, true);
}

//==============================================================================
// BInputEventKey::BInputEventKey
//==============================================================================
BInputEventKey::BInputEventKey() 
{
   mType = cEventKey;
}

//==============================================================================
// BInputEventKey::clear
//==============================================================================
void BInputEventKey::clear(void)
{
   mKey = 0;
   mSubtype = 0;
   mModifiers.zero();
   BInputEvent::clear();
}

//==============================================================================
// BInputEventGamepad::BInputEventGamepad
//==============================================================================
BInputEventGamepad::BInputEventGamepad() 
{
   mType = cEventGamepad;
}

//==============================================================================
// BInputEventGamepad::clear
//==============================================================================
void BInputEventGamepad::clear(void)
{
   mPort = -1;
   mEvent = -1;
   mControlType = -1;
   mX = 0.0f;
   mY = 0.0f;
   mAnalog = 0.0f;
   BInputEvent::clear();
}