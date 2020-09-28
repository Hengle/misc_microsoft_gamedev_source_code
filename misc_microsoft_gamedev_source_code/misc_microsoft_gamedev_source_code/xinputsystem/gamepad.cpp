//==============================================================================
// gamepad.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamepad.h"
#include "XeCR.h"
#include "gamepadmap.h"
#include "inputsystem.h"
#include "configsinput.h"

BSmallDynamicSimArray<BGamepadRumblePattern> BGamepad::mRumblePatternList;
BStringTableLong BGamepad::mRumblePatternTable;

#ifndef BUILD_FINAL
//#define DEBUG_RUMBLE
#endif

//==============================================================================
// BGamepad::BGamepad
//==============================================================================
BGamepad::BGamepad() :
   mPort(-1),
   mMap(NULL),
   mActive(false),
   mRumbleEnabled(true),
#ifndef XBOX
   mDevice(NULL),
#endif
   mStickLX(0.0f),
   mStickLY(0.0f),
   mStickRX(0.0f),
   mStickRY(0.0f),
   mDpadX(0.0f),
   mDpadY(0.0f),
   mNextRumbleID(0),
   mLeftRumbleSpeed(0.0f),
   mRightRumbleSpeed(0.0f)
{
#ifndef XBOX
   ZeroMemory(&mInputState, sizeof(mInputState));
#endif
   ZeroMemory(&mInputState2, sizeof(mInputState2));
   ZeroMemory(mControlActive, sizeof(bool)*cGamepadControlCount);
   ZeroMemory(mLastControlActive, sizeof(bool)*cGamepadControlCount);
   ZeroMemory(mControlValue, sizeof(float)*cGamepadControlCount);
}

//==============================================================================
// BGamepad::~BGamepad
//==============================================================================
BGamepad::~BGamepad()
{
   close();
}

//==============================================================================
// BGamepad::setup
//==============================================================================
bool BGamepad::setup(long port, BGamepadMap* map)
{
   mPort=port;
   mMap=map;

   mStickLX=0.0f;
   mStickLY=0.0f;
   mStickRX=0.0f;
   mStickRY=0.0f;
   mDpadX=0.0f;
   mDpadY=0.0f;

   return true;
}

#ifndef XBOX
//==============================================================================
// enumAxesCallback
//==============================================================================
static BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext)
{
   IDirectInputDevice8* device = (IDirectInputDevice8*)pContext;

   DIPROPRANGE diprg; 
   diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
   diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
   diprg.diph.dwHow        = DIPH_BYID; 
   diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
   diprg.lMin              = -1000; 
   diprg.lMax              = +1000; 

   // Set the range for the axis
   if (FAILED(device->SetProperty(DIPROP_RANGE, &diprg.diph)))
      return DIENUM_STOP;

   return DIENUM_CONTINUE;
}
#endif

#ifndef XBOX
//==============================================================================
// BGamepad::createDevice
//==============================================================================
bool BGamepad::createDevice(REFGUID guid, BGamepadMap* map)
{
   HRESULT hr = gInputSystem.getDirectInput()->CreateDevice(guid, &mDevice, NULL);
   if (FAILED(hr)) 
      return false;

   DWORD errorCode = mDevice->SetDataFormat(&c_dfDIJoystick);
   errorCode = mDevice->SetCooperativeLevel(gInputSystem.getWindowHandle(), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
   errorCode = mDevice->EnumObjects(enumAxesCallback, (VOID*)mDevice, DIDFT_AXIS);

   hr=mDevice->Acquire();

   mMap=map;

   mActive=true;

   return true;
}
#endif

//==============================================================================
// BGamepad::resetDevice
//==============================================================================
void BGamepad::resetDevice()
{
#ifndef XBOX
   DWORD errorCode=0;
   if (mDevice)
      errorCode=mDevice->Acquire();
#endif
}


//==============================================================================
// BGamepad::close
//==============================================================================
void BGamepad::close()
{
#ifndef XBOX
   if(mDevice)
   {
      mDevice->Release();
      mDevice=NULL;
   }
#endif
   mActive=false;
   mPort=-1;
   mMap=NULL;
}

//==============================================================================
// BGamepad::update
//==============================================================================
void BGamepad::update(BInputData* pInputData, IInputEventHandler* pInputEventHandler, bool doUpdateRumble)
{
   int64 inputTime = 0;
   uint inputIndex = 0;
   uint inputCount = 0;

   if (!pInputEventHandler)
      return;

   #ifndef XBOX
   if (!mDevice)
   #endif
   {
      if (mPort==-1)
         return;
      if (pInputData)
      {
         if (pInputData->mCount[mPort] == BInputData::cMaxInputRecords)
            inputIndex = pInputData->mIndex[mPort];
      }
   }

   for (;;)
   {
      #ifndef XBOX
      if(!mDevice)
      #endif
      {
         if (pInputData)
         {
            if (inputCount >= pInputData->mCount[mPort])
               break;
            memcpy(&mInputState2, &(pInputData->mInputStates[mPort][inputIndex]), sizeof(XINPUT_STATE));
            inputTime = pInputData->mTimeStamps[mPort][inputIndex];
            inputCount++;
            inputIndex++;
            if (inputIndex == BInputData::cMaxInputRecords)
               inputIndex = 0;
         }
         else
         {
            if(XInputGetState(mPort, &mInputState2) != ERROR_SUCCESS)
               return;
            LARGE_INTEGER time;
            QueryPerformanceCounter(&time);
            inputTime=time.QuadPart;
         }
      }

      #ifndef XBOX
      if(mDevice)
      {
         DWORD errorCode = mDevice->Poll();
         if(errorCode!=DI_OK && errorCode!=DI_NOEFFECT)
         {
            resetDevice();
            return;
         }

         errorCode = mDevice->GetDeviceState(sizeof(mInputState), &mInputState);
         if(errorCode!=DI_OK)
         {
            resetDevice();
            return;
         }

         LARGE_INTEGER time;
         QueryPerformanceCounter(&time);
         inputTime=time.QuadPart;
      }
      #endif

      // Save off the last control active states
      for(long i=0; i<cGamepadControlCount; i++)
      {
         mLastControlActive[i]=mControlActive[i];
      }

      // Get the current single control states and values
      for(long i=cFirstSingleControl; i<=cLastSingleControl; i++)
      {
         float val = 0.0f;
         if (mMap)
         {
            #ifndef XBOX
            if(mDevice)
               val=mMap->translate(i, mInputState);
            else
            #endif         
               val=mMap->translate(i, mInputState2);
         }
         
         mControlValue[i]=val;
         mControlActive[i]=(val!=0.0f);
      }

      // Calculate the combo control states and values
      for(long i=cFirstComboControl; i<=cLastComboControl; i++)
      {
         float val=0.0f;
         switch(i)
         {
            case cStickLeft:
               mStickLX=mControlValue[cStickLeftRight]-mControlValue[cStickLeftLeft];
               mStickLY=mControlValue[cStickLeftDown]-mControlValue[cStickLeftUp];
               val=(float)sqrt((mStickLX*mStickLX)+(mStickLY*mStickLY));
               break;

            case cStickRight:
               mStickRX=mControlValue[cStickRightRight]-mControlValue[cStickRightLeft];
               mStickRY=mControlValue[cStickRightDown]-mControlValue[cStickRightUp];
               val=(float)sqrt((mStickRX*mStickRX)+(mStickRY*mStickRY));
               break;

            case cDpad:
               mDpadX=mControlValue[cDpadRight]-mControlValue[cDpadLeft];
               mDpadY=mControlValue[cDpadDown]-mControlValue[cDpadUp];
               val=(float)sqrt((mDpadX*mDpadX)+(mDpadY*mDpadY));
               break;
         }
         mControlValue[i]=val;
         mControlActive[i]=(val!=0.0f);
      }

      // Send out control events
      BInputEventDetail detail;
      detail.mTime = inputTime;

      for(long i=0; i<cGamepadControlCount; i++)
      {
         if(mControlActive[i])
         {
            detail.mAnalog=mControlValue[i];

            switch(i)
            {
               case cStickLeftUp:
               case cStickLeftDown:
               case cStickLeftRight:
               case cStickLeftLeft:
               case cStickLeft:
                  detail.mX=mStickLX;
                  detail.mY=mStickLY;
                  break;

               case cStickRightUp:
               case cStickRightDown:
               case cStickRightRight:
               case cStickRightLeft:
               case cStickRight:
                  detail.mX=mStickRX;
                  detail.mY=mStickRY;
                  break;

               case cDpadUp:
               case cDpadDown:
               case cDpadRight:
               case cDpadLeft:
               case cDpad:
                  detail.mX=mDpadX;
                  detail.mY=mDpadY;
                  break;

               case cButtonThumbLeft:
                  detail.mX=mStickLX;
                  detail.mY=mStickLY;
                  break;

               case cButtonThumbRight:
                  detail.mX=mStickRX;
                  detail.mY=mStickRY;
                  break;

               default:
                  detail.mX=0.0f;
                  detail.mY=0.0f;
                  break;
            }

            bool handled;
            if(!mLastControlActive[i])
               handled=pInputEventHandler->handleInput(mPort, cInputEventControlStart, i, detail);
            else
               handled=pInputEventHandler->handleInput(mPort, cInputEventControlRepeat, i, detail);

            if(handled)
            {
               // If the specific control handled the input event, then the combo control
               // should not be allowed to process the method. Otherwise the message would
               // get processed twice.
               switch(i)
               {
                  case cStickLeftUp:
                  case cStickLeftDown:
                  case cStickLeftLeft:
                  case cStickLeftRight:
                     mControlActive[cStickLeft]=false;
                     break;

                  case cStickRightUp:
                  case cStickRightDown:
                  case cStickRightLeft:
                  case cStickRightRight:
                     mControlActive[cStickRight]=false;
                     break;

                  case cDpadUp:
                  case cDpadDown:
                  case cDpadLeft:
                  case cDpadRight:
                     mControlActive[cDpad]=false;
                     break;

                  case cTriggerLeft:
                  case cTriggerRight:
                     mControlActive[cTriggers]=false;
                     break;
               }
            }
         }
         else
         {
            detail.mAnalog=0.0f;
            if(mLastControlActive[i])
               pInputEventHandler->handleInput(mPort, cInputEventControlStop, i, detail);
         }
      }

      #ifndef XBOX
      if(!mDevice)
      #endif
      {
         if (pInputData)
            continue;
      }

      break;
   }

   // Rumble
   if (doUpdateRumble)
      updateRumble();
}

//==============================================================================
// BGamepad::playRumblePattern
//==============================================================================
int BGamepad::playRumblePattern(int patternIndex, bool loop)
{
#ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigUserGamepadRumble))
      return -1;
#endif

   if (!mRumbleEnabled)
      return -1;

   if (patternIndex < 0 || patternIndex >= mRumblePatternList.getNumber())
      return -1;

   int rumbleID = mNextRumbleID;
   mNextRumbleID++;

   BGamepadRumblePattern& pattern = mRumblePatternList[patternIndex];

   uint numSeq = pattern.getSize();
   for (uint i=0; i<numSeq; i++)
   {
      BGamepadRumbleSequence& sequence = pattern[i];
      if (sequence.getNumber() > 0)
      {
//-- FIXING PREFIX BUG ID 7795
         const BGamepadRumble& action = sequence[0];
//--

         BGamepadRumble rumble;

         rumble.mID = rumbleID;
         rumble.mLeftRumbleType = action.mLeftRumbleType;
         rumble.mLeftStrength = action.mLeftStrength;
         rumble.mRightRumbleType = action.mRightRumbleType;   
         rumble.mRightStrength = action.mRightStrength;
         rumble.mDuration = action.mDuration;
         rumble.mPatternIndex = (int16)patternIndex;
         rumble.mPatternSequence = (uint8)i;
         rumble.mPatternAction = 0;
         rumble.mStartTime = timeGetTime();
         rumble.mLoop = loop;
         rumble.mPlayed = false;

         mRumbleList.add(rumble);
      }
   }

   return rumbleID;
}

//==============================================================================
// BGamepad::playRumble
//==============================================================================
int BGamepad::playRumble(int leftRumbleType, float leftStrength, int rightRumbleType, float rightStrength, float duration, bool loop)
{
#ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigUserGamepadRumble))
      return -1;
#endif

   if (!mRumbleEnabled)
      return -1;

   BGamepadRumble rumble;

   rumble.mID = mNextRumbleID;
   mNextRumbleID++;

   rumble.mLeftRumbleType = leftRumbleType;   
   rumble.mLeftStrength = leftStrength;
   rumble.mRightRumbleType = rightRumbleType;   
   rumble.mRightStrength = rightStrength;
   rumble.mStartTime = timeGetTime();
   rumble.mDuration = duration;
   rumble.mLoop = loop;
   rumble.mPlayed = false;

   mRumbleList.add(rumble);

   return rumble.mID;
}

//==============================================================================
// BGamepad::stopRumble
//==============================================================================
void BGamepad::stopRumble(int rumbleID)
{
   for (int i=mRumbleList.getNumber()-1; i>=0; i--)
   {
//-- FIXING PREFIX BUG ID 7796
      const BGamepadRumble& rumble = mRumbleList[i];
//--
      if (rumble.mID == rumbleID)
      {
         bool isPattern = (rumble.mPatternIndex != -1);
         mRumbleList.removeIndex(i);
         if (!isPattern)
            break;
      }
   }
}

//==============================================================================
// BGamepad::resetRumble
//==============================================================================
void BGamepad::resetRumble()
{
   mRumbleList.clear();
   mNextRumbleID=0;
   XINPUT_VIBRATION vibration;
   vibration.wLeftMotorSpeed = 0;
   vibration.wRightMotorSpeed = 0;
   XInputSetState(mPort, &vibration);
   mLeftRumbleSpeed=0.0f;
   mRightRumbleSpeed=0.0f;
}

//==============================================================================
// BGamepad::updateRumble
//==============================================================================
void BGamepad::updateRumble()
{
#ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigUserGamepadRumble))
   {
      if (mLeftRumbleSpeed != 0.0f || mRightRumbleSpeed != 0.0f)
         resetRumble();
      return;
   }
#endif

   if( !mRumbleEnabled )
   {
      if (mLeftRumbleSpeed != 0.0f || mRightRumbleSpeed != 0.0f)
         resetRumble();
      return;
   }

   #ifdef DEBUG_RUMBLE
   bool logRumble = false;
   #endif

   float rightSpeed = 0.0f;
   float leftSpeed = 0.0f;

   DWORD time = timeGetTime();

   for (int i=mRumbleList.getNumber()-1; i>=0; i--)
   {
      #ifdef DEBUG_RUMBLE
      logRumble = true;
      #endif

      BGamepadRumble& rumble = mRumbleList[i];

      float elapsed = (time - rumble.mStartTime) * 0.001f;

      if (elapsed >= rumble.mDuration)
      {
         if (rumble.mPatternIndex != -1)
         {
            if (rumble.mPlayed || rumble.mLoop)
            {
               BGamepadRumblePattern& pattern = mRumblePatternList[rumble.mPatternIndex];
               BGamepadRumbleSequence& sequence = pattern[rumble.mPatternSequence];
               uint8 nextAction = rumble.mPatternAction + 1;
               if (nextAction >= sequence.getSize())
               {
                  if (rumble.mLoop)
                  {
                     if (sequence.getSize() == 1)
                     {
                        elapsed = fmod(elapsed, rumble.mDuration);
                        rumble.mStartTime = time - (DWORD)(elapsed*1000.0f);
                     }
                     else
                     {
//-- FIXING PREFIX BUG ID 7797
                        const BGamepadRumble& action = sequence[0];
//--
                        rumble.mLeftRumbleType = action.mLeftRumbleType;
                        rumble.mLeftStrength = action.mLeftStrength;
                        rumble.mRightRumbleType = action.mRightRumbleType;   
                        rumble.mRightStrength = action.mRightStrength;
                        rumble.mPatternAction = 0;
                        elapsed -= rumble.mDuration;
                        rumble.mStartTime = time - (DWORD)(elapsed*1000.0f);
                        rumble.mDuration = action.mDuration;
                     }
                  }
                  else
                  {
                     if (rumble.mPlayed)
                     {
                        mRumbleList.removeIndex(i);
                        continue;
                     }
                  }
               }
               else
               {
//-- FIXING PREFIX BUG ID 7798
                  const BGamepadRumble& action = sequence[nextAction];
//--
                  rumble.mLeftRumbleType = action.mLeftRumbleType;
                  rumble.mLeftStrength = action.mLeftStrength;
                  rumble.mRightRumbleType = action.mRightRumbleType;   
                  rumble.mRightStrength = action.mRightStrength;
                  rumble.mPatternAction = nextAction;
                  elapsed -= rumble.mDuration;
                  rumble.mStartTime = time - (DWORD)(elapsed*1000.0f);
                  rumble.mDuration = action.mDuration;
               }
            }
         }
         else
         {
            if (rumble.mLoop)
            {
               elapsed = fmod(elapsed, rumble.mDuration);
               rumble.mStartTime = time - (DWORD)(elapsed*1000.0f);
            }
            else
            {
               if (rumble.mPlayed)
               {
                  mRumbleList.removeIndex(i);
                  continue;
               }
            }
         }
      }

      rumble.mPlayed = true;

      float scale;
      if (elapsed >= rumble.mDuration)
         scale = 1.0f;
      else
         scale = elapsed / rumble.mDuration;

      if (rumble.mRightRumbleType != cRumbleTypeNone)
      {
         float speed = getRumblePatternValue(rumble.mRightRumbleType, rumble.mRightStrength, scale, cRumbleMotorRight);
         if (speed > rightSpeed)
            rightSpeed = speed;
      }

      if (rumble.mLeftRumbleType != cRumbleTypeNone)
      {
         float speed = getRumblePatternValue(rumble.mLeftRumbleType, rumble.mLeftStrength, scale, cRumbleMotorLeft);
         if (speed > leftSpeed)
            leftSpeed = speed;
      }
   }

   if (rightSpeed > 1.0f)
      rightSpeed = 1.0f;

   if (leftSpeed > 1.0f)
      leftSpeed = 1.0f;

   if (leftSpeed != mLeftRumbleSpeed || rightSpeed != mRightRumbleSpeed)
   {
      #ifdef DEBUG_RUMBLE
      logRumble=true;
      #endif

      XINPUT_VIBRATION vibration;
      vibration.wLeftMotorSpeed = (WORD)(65535.0f*leftSpeed);
      vibration.wRightMotorSpeed = (WORD)(65535.0f*rightSpeed);
      XInputSetState(mPort, &vibration);

      mLeftRumbleSpeed = leftSpeed;
      mRightRumbleSpeed = rightSpeed;
   }

   #ifdef DEBUG_RUMBLE
   if (logRumble)
      gConsole.output(cChannelUI, "rumble id=%d t=%-10u l=%-.2f r=%-.2f", mPort, time, leftSpeed, rightSpeed);
   #endif
}

//==============================================================================
// BGamepad::getRumblePatternValue
//==============================================================================
float BGamepad::getRumblePatternValue(long type, float strength, float scale, int rumbleMotor)
{
   float value = 0.0f;

   switch (type)
   {
      case cRumbleTypeSineWave:
      {
         float radians = scale * cPi;
         value = sinf(radians);
         break;
      }
      case cRumbleTypeFixed:
      {
         value = 1.0f;
         break;
      }
      case cRumbleTypeIntervalBurst:
      {
         float radians = scale * cTwoPi;
         int sineCurve = (INT)(50*(sinf(0.7f*radians)+1));
         if (sineCurve < 30)
            value = 0.0f;
         else 
            value = 1.0f;
         break;
      }
      case cRumbleTypeRandomNoise:
      {
         value = getRandRange(cUIRand, 0, 100) * 0.01f;
         break;
      }
      case cRumbleTypeIncline:
      {
         value = scale;
         break;
      }
      case cRumbleTypeDecline:
      {
         value = 1.0f - scale;
         break;
      }
      case cRumbleTypeBumpLRL:
      {
         if (rumbleMotor == cRumbleMotorLeft)
         {
            if (scale < 0.33f || scale >= 0.66f)
               value = 1.0f;
         }
         else if (rumbleMotor == cRumbleMotorRight)
         {
            if (scale >= 0.33f && scale < 0.66f)
               value = 1.0f;
         }
         break;
      }
   }

   value *= strength;

   return value; 
}

//==============================================================================
// BGamepad::getRumbleType
//==============================================================================
int BGamepad::getRumbleType(const char* pName)
{
   if (stricmp(pName, "Fixed")==0)
      return cRumbleTypeFixed;
   else if (stricmp(pName, "SineWave")==0)
      return cRumbleTypeSineWave;
   else if (stricmp(pName, "IntervalBurst")==0)
      return cRumbleTypeIntervalBurst;
   else if (stricmp(pName, "RandomNoise")==0)
      return cRumbleTypeRandomNoise;
   else if (stricmp(pName, "Incline")==0)
      return cRumbleTypeIncline;
   else if (stricmp(pName, "Decline")==0)
      return cRumbleTypeDecline;
   else if (stricmp(pName, "BumpLRL")==0)
      return cRumbleTypeBumpLRL;
   return cRumbleTypeNone;
}

//==============================================================================
// BGamepad::getRumbleMotor
//==============================================================================
int BGamepad::getRumbleMotor(const char* pName)
{
   if (stricmp(pName, "Both")==0)
      return cRumbleMotorBoth;
   else if (stricmp(pName, "Left")==0)
      return cRumbleMotorLeft;
   else if (stricmp(pName, "Right")==0)
      return cRumbleMotorRight;
   return -1;
}

//==============================================================================
// BGamepad::getRumbleTypeName
//==============================================================================
const char* BGamepad::getRumbleTypeName(int rumbleType)
{
   switch (rumbleType)
   {
      case cRumbleTypeFixed: return "Fixed";
      case cRumbleTypeSineWave: return "SineWave";
      case cRumbleTypeIntervalBurst: return "IntervalBurst";
      case cRumbleTypeRandomNoise: return "RandomNoise";
      case cRumbleTypeIncline: return "Incline";
      case cRumbleTypeDecline: return "Decline";
      case cRumbleTypeBumpLRL: return "BumpLRL";
   }
   return "None";
}

//==============================================================================
// BGamepad::getRumbleMotorName
//==============================================================================
const char* BGamepad::getRumbleMotorName(int rumbleMotor)
{
   switch (rumbleMotor)
   {
      case cRumbleMotorBoth: return "Both";
      case cRumbleMotorLeft: return "Left";
      case cRumbleMotorRight: return "Right";
   }
   return "None";
}

//==============================================================================
// BGamepad::getRumblePatternName
//==============================================================================
const char* BGamepad::getRumblePatternName(int patternType)
{
   if(patternType<0 || patternType>=mRumblePatternTable.getTags().getNumber())
      return "";
   return mRumblePatternTable.getTags().get(patternType).getPtr();
}

//==============================================================================
// BGamepad::isControlActive
//==============================================================================
bool BGamepad::isControlActive(long control) const
{
   if(control<cGamepadControlCount)
      return mControlActive[control];
   else
      return false;
}

//==============================================================================
// BGamepad::getControlValue
//==============================================================================
float BGamepad::getControlValue(long control) const
{
   if(control<cGamepadControlCount)
      return mControlValue[control];
   else
      return 0.0f;
}

//==============================================================================
// BGamepad::wasActivatedThisUpdate
//==============================================================================
bool BGamepad::wasActivatedThisUpdate(long control)
{
   if(control<cGamepadControlCount)
      return mControlValue[control] && !mLastControlActive[control];
   else
      return 0.0f;
}

//==============================================================================
// BGamepad::loadRumblePattern
//==============================================================================
void BGamepad::loadRumblePattern(BXMLNode& rootNode)
{
   BSimString patternName;
   if (!rootNode.getAttribValueAsString("Name", patternName))
      return;

   BGamepadRumblePattern newPattern;
   BGamepadRumblePattern* pPattern = &newPattern;

   int patternIndex = getRumblePattern(patternName);
   if (patternIndex != -1)
   {
      pPattern = mRumblePatternList.getPtr()+patternIndex;
      pPattern->clear();
   }

   int nodeCount=rootNode.getNumberChildren();
   for (int i=0; i<nodeCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      const BPackedString name(node.getName());
      if (name == "Sequence")
      {
         BGamepadRumbleSequence sequence;
         int childCount=node.getNumberChildren();
         for (int j=0; j<childCount; j++)
         {
            BXMLNode childNode(node.getChild(j));
            const BPackedString childName(childNode.getName());
            if (childName == "Action")
            {
               BGamepadRumble action;
               BSimString str;
               if (childNode.getAttribValueAsString("LeftRumbleType", str))
                  action.mLeftRumbleType=getRumbleType(str);
               if (childNode.getAttribValueAsString("RightRumbleType", str))
                  action.mRightRumbleType=getRumbleType(str);
               childNode.getAttribValueAsFloat("LeftStrength", action.mLeftStrength);
               childNode.getAttribValueAsFloat("RightStrength", action.mRightStrength);
               childNode.getAttribValueAsFloat("Duration", action.mDuration);
               sequence.add(action);
            }
         }
         pPattern->add(sequence);
      }
   }

   if (patternIndex == -1)
   {
      patternIndex = mRumblePatternList.add(newPattern);
      mRumblePatternTable.add(patternName.getPtr(), patternIndex);
   }
}

//==============================================================================
// BGamepadRumble::BGamepadRumble
//==============================================================================
BGamepadRumble::BGamepadRumble() :
   mID(-1),
   mLeftRumbleType(BGamepad::cRumbleTypeNone),
   mLeftStrength(0.0f),
   mRightRumbleType(BGamepad::cRumbleTypeNone),
   mRightStrength(0.0f),
   mStartTime(0),
   mDuration(0.0f),
   mPatternIndex(-1),
   mPatternSequence(0),
   mPatternAction(0),
   mLoop(false),
   mPlayed(false)
{
}
