//==============================================================================
// gamepadcontrol.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "inputcontrol.h"
#include "config.h"
#include "gamepad.h"
#include "configsinput.h"
#include "xmlreader.h"

// Statics
static BInputEventDetail tempDetail;

//==============================================================================
// BInputControl::BInputControl
//==============================================================================
BInputControl::BInputControl() :
   mControlType(-1),
   mInputCommand(),
   mSoundEvent(),
   mModifierCount(0),
   mRepeatDelay(0),
   mRepeatRate(0),
   mX(0.0f),
   mY(0.0f),
   mAnalog(0.0f),
   mThreshold(0.0f),
   mpInputControlEventHandler(NULL),
   mFlags()
{
   mFlags.setNumber(cFlagCount);
   ZeroMemory(mModifiers, sizeof(mModifiers));
   ZeroMemory(mRepeatTime, sizeof(mRepeatTime));
   ZeroMemory(mDigitalX, sizeof(mDigitalX));
   ZeroMemory(mDigitalY, sizeof(mDigitalY));
   ZeroMemory(mStartTime, sizeof(mStartTime));
}

//==============================================================================
// BInputControl::~BInputControl
//==============================================================================
BInputControl::~BInputControl()
{
}

//==============================================================================
// BInputControl::reset
//==============================================================================
void BInputControl::reset()
{
   DWORD time=timeGetTime();

   for (int i=0; i<BInputSystem::cMaxPorts; i++)
      mRepeatTime[i]=time+mRepeatRate;
}

//==============================================================================
// BInputControl::setup
//==============================================================================
bool BInputControl::setup(BXMLNode node, IInputControlEventHandler* handler)
{
   if(mControlType!=-1)
   {
      BASSERT(0);
      return false;
   }

   mpInputControlEventHandler = handler;

   BSimString name;
   if(!node.getAttribValue("name", &name))
      return false;
   mControlType=gInputSystem.lookupControlType(name.getPtr());
   if(mControlType==-1)
      return false;

   BSimString str;
   if(node.getAttribValue("command", &str))
      mInputCommand=str.getPtr();

   if (node.getAttribValue("sound", &str))
      mSoundEvent=str.getPtr();

   if(node.getAttribValue("repeat", &str))
   {
      setFlag(cFlagRepeat, true);
      float val = 0.0f;
      if(node.getAttribValueAsFloat("rate", val))
         mRepeatRate=(DWORD)(val*1000.0f);
      if(node.getAttribValueAsFloat("delay", val))
         mRepeatDelay=(DWORD)(val*1000.0f);
   }

   if(node.getAttribValueAsFloat("x", mX))
      setFlag(cFlagOverrideX, true);

   if(node.getAttribValueAsFloat("y", mY))
      setFlag(cFlagOverrideY, true);

   if(node.getAttribValueAsFloat("analog", mAnalog))
      setFlag(cFlagOverrideAnalog, true);

   if(node.getAttribValue("swapXY", &str))
      setFlag(cFlagSwapXY, true);

   if(node.getAttribValue("oneway", &str))
      setFlag(cFlagOneWay, true);

   if(node.getAttribValue("digital", &str))
      setFlag(cFlagDigital, true);

   node.getAttribValueAsFloat("threshold", mThreshold);

   BSimString modifierName("modifier");
   for(long i=0; i<cMaxModifiers; i++)
   {
      if(i>0)
         modifierName.format(B("modifier%d"), i+1);
      if(node.getAttribValue(modifierName.getPtr(), &str))
      {
         long modifierControl=gInputSystem.lookupControlType(str.getPtr());
         if(modifierControl==-1)
            break;
         mModifiers[mModifierCount]=modifierControl;
         mModifierCount++;
      }
      else
         break;
   }

   if(node.getAttribValue("event", &str))
   {
      if(str.findLeft(B("stop"))==-1)
         setFlag(cFlagNoStopEvent, true);

      if(str.findLeft(B("start"))==-1)
         setFlag(cFlagNoStartEvent, true);

      if(str.findLeft(B("double"))!=-1)
         setFlag(cFlagDoubleEvent, true);
   }

   return true;
}

//==============================================================================
// BInputControl::handleInput
//==============================================================================
bool BInputControl::handleInput(long port, long event, long controlType, const BInputEventDetail& detail)
{
   if(!checkControl(port, controlType))
      return false;

   if(getFlag(cFlagDigital))
   {
      // This is a special mode to make control sticks act somewhat like 4 separate digital buttons.
      // This can be useful for assigning the control stick to a function that might normally work
      // well on the Dpad or on the A, B, X, and Y buttons.
      switch(event)
      {
         case cInputEventControlStart:
         case cInputEventControlRepeat:
            {
               float x, y;
               if(detail.mX>=mThreshold)
                  x=1.0f;
               else if(detail.mX<=-mThreshold)
                  x=-1.0f;
               else
                  x=0.0f;

               if(detail.mY>=mThreshold)
                  y=1.0f;
               else if(detail.mY<=-mThreshold)
                  y=-1.0f;
               else
                  y=0.0f;

               if(getFlag(cFlagOneWay))
               {
                  if(x!=0.0f && y!=0.0f)
                  {
                     if(x==mDigitalX[port])
                        y=0.0f;
                     else if(y==mDigitalY[port])
                        x=0.0f;
                     else
                        y=0.0f;
                  }
               }

               if(x!=mDigitalX[port] || y!=mDigitalY[port])
               {
                  if(x==0.0f && y==0.0f)
                  {
                     tempDetail.mAnalog=0.0f;
                     tempDetail.mX=mDigitalX[port];
                     tempDetail.mY=mDigitalY[port];
                     eventStop(port, tempDetail);
                  }
                  else if(getFlag(cFlagRepeat))
                  {
                     tempDetail.mAnalog=1.0f;
                     tempDetail.mX=x;
                     tempDetail.mY=y;
                     if(mDigitalX[port]==0.0f && mDigitalY[port]==0.0f)
                        eventStart(port, tempDetail);
                     else
                        eventRepeat(port, tempDetail);
                  }
                  else
                  {
                     if(mDigitalX[port]!=0.0f || mDigitalY[port]!=0.0f)
                     {
                        tempDetail.mAnalog=1.0f;
                        tempDetail.mX=0.0f;
                        tempDetail.mY=0.0f;
                        eventStop(port, tempDetail);
                     }

                     tempDetail.mAnalog=1.0f;
                     tempDetail.mX=x;
                     tempDetail.mY=y;
                     eventStart(port, tempDetail);
                  }
               }
               else if(getFlag(cFlagRepeat) && (x!=0.0f || y!=0.0f))
               {
                  tempDetail.mAnalog=1.0f;
                  tempDetail.mX=x;
                  tempDetail.mY=y;
                  eventRepeat(port, tempDetail);
               }

               mDigitalX[port]=x;
               mDigitalY[port]=y;
            }
            return true;

         case cInputEventControlStop:
            if(mDigitalX[port]!=0.0f || mDigitalY[port]!=0.0f)
            {
               tempDetail.mAnalog=1.0f;
               tempDetail.mX=0.0f;
               tempDetail.mY=0.0f;
               eventStop(port, tempDetail);
               mDigitalX[port]=0.0f;
               mDigitalY[port]=0.0f;
            }
            return true;
      }
   }
   else
   {
      switch(event)
      {
         case cInputEventControlStart:
            eventStart(port, detail);
            return true;

         case cInputEventControlRepeat:
            if(getFlag(cFlagRepeat))
               eventRepeat(port, detail);
            return true;

         case cInputEventControlStop:
            eventStop(port, detail);
            return true;
      }
   }

   return false;
}

//==============================================================================
// BInputControl::checkControl
//==============================================================================
bool BInputControl::checkControl(long port, long controlType)
{
   if(controlType!=mControlType)
      return false;

   if(mModifierCount>0)
   {
      for(long i=0; i<mModifierCount; i++)
      {
         if(!gInputSystem.getGamepad(port).isControlActive(mModifiers[i]))
            return false;
      }
   }

   if(getFlag(cFlagOneWay))
   {
//-- FIXING PREFIX BUG ID 7791
      const BGamepad& gamepad=gInputSystem.getGamepad(port);
//--
      switch(controlType)
      {
         case cDpadUp:
         case cDpadDown:
            if(gamepad.isControlActive(cDpadRight) || gamepad.isControlActive(cDpadLeft))
               return false;
            break;

         case cDpadLeft:
         case cDpadRight:
            if(gamepad.isControlActive(cDpadUp) || gamepad.isControlActive(cDpadDown))
               return false;
            break;

         case cStickLeftUp:
         case cStickLeftDown:
            if(gamepad.isControlActive(cStickLeftRight) || gamepad.isControlActive(cStickLeftLeft))
               return false;
            break;

         case cStickLeftLeft:
         case cStickLeftRight:
            if(gamepad.isControlActive(cStickLeftUp) || gamepad.isControlActive(cStickLeftDown))
               return false;
            break;

         case cStickRightUp:
         case cStickRightDown:
            if(gamepad.isControlActive(cStickRightRight) || gamepad.isControlActive(cStickRightLeft))
               return false;
            break;

         case cStickRightLeft:
         case cStickRightRight:
            if(gamepad.isControlActive(cStickRightUp) || gamepad.isControlActive(cStickRightDown))
               return false;
            break;
      }
   }

   return true;
}

//==============================================================================
// BInputControl::eventStart
//==============================================================================
void BInputControl::eventStart(long port, const BInputEventDetail& detail)
{
   DWORD time=timeGetTime();
   mRepeatTime[port]=time+(mRepeatDelay>0 ? mRepeatDelay : mRepeatRate);

   fixupDetail(detail, tempDetail);

   if(getFlag(cFlagDoubleEvent))
   {
      if(mStartTime[port]!=0)
      {
         DWORD elapsed=time-mStartTime[port];

         float val=0.25f;
         gConfig.get(cConfigGamepadDoubleClickTime, &val);
         DWORD doubleClickTime=(DWORD)(val*1000.0f);

         if(elapsed<=doubleClickTime)
         {
            if (mpInputControlEventHandler)
               mpInputControlEventHandler->executeInputEvent(port, cInputEventCommandDouble, mControlType, tempDetail, mInputCommand, this);

            mStartTime[port]=0;
            return;
         }
      }
      mStartTime[port]=time;
   }

   if(!getFlag(cFlagNoStartEvent))
   {
      if (mpInputControlEventHandler)
         mpInputControlEventHandler->executeInputEvent(port, cInputEventCommandStart, mControlType, tempDetail, mInputCommand, this);
   }
}

//==============================================================================
// BInputControl::eventRepeat
//==============================================================================
void BInputControl::eventRepeat(long port, const BInputEventDetail& detail)
{
   DWORD time=timeGetTime();
   if(time>=mRepeatTime[port])
   {
      mRepeatTime[port]=time+mRepeatRate;
      fixupDetail(detail, tempDetail);
      if (mpInputControlEventHandler)
         mpInputControlEventHandler->executeInputEvent(port, cInputEventCommandRepeat, mControlType, tempDetail, mInputCommand, this);
   }
}

//==============================================================================
// BInputControl::eventStop
//==============================================================================
void BInputControl::eventStop(long port, const BInputEventDetail& detail)
{
   if(!getFlag(cFlagNoStopEvent))
   {
      fixupDetail(detail, tempDetail);
      if (mpInputControlEventHandler)
         mpInputControlEventHandler->executeInputEvent(port, cInputEventCommandStop, mControlType, tempDetail, mInputCommand, this);
   }
}

//==============================================================================
// BInputControl::fixupDetail
//==============================================================================
void BInputControl::fixupDetail(const BInputEventDetail& in, BInputEventDetail& out)
{
   out.mAnalog=in.mAnalog;
   out.mX=in.mX;
   out.mY=in.mY;

   if(getFlag(cFlagSwapXY))
   {
      out.mX=in.mY;
      out.mY=in.mX;
   }

   if(getFlag(cFlagOverrideX))
      out.mX=mX;

   if(getFlag(cFlagOverrideY))
      out.mY=mY;

   if(getFlag(cFlagOverrideAnalog))
      out.mAnalog=mAnalog;
}
