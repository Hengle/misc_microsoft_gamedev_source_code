//==============================================================================
// keyboard.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "keyboard.h"
#include "XeCR.h"
#include "inputsystem.h"
#include "containers\hashMap.h"

//==============================================================================
// BKeyboard::BKeyboard
//==============================================================================
BKeyboard::BKeyboard() :
   mPort(0),
   mChatPadRemapping(false)
{
   ZeroMemory(mControlToVKTable, sizeof(mControlToVKTable));
   
   ZeroMemory(mKeyActive, sizeof(mKeyActive));
}

//==============================================================================
// BKeyboard::~BKeyboard
//==============================================================================
BKeyboard::~BKeyboard()
{
}

//==============================================================================
// BKeyboard::setup
//==============================================================================
bool BKeyboard::setup()
{
   setControlToVK(cKeyBackSpace, VK_BACK);
   setControlToVK(cKeyTab, VK_TAB);
   setControlToVK(cKeyEnter, VK_RETURN);
   setControlToVK(cKeyShift, VK_SHIFT);
   setControlToVK(cKeyCtrl, VK_CONTROL);
   setControlToVK(cKeyAlt, VK_MENU);
   setControlToVK(cKeyPause, VK_PAUSE);
   setControlToVK(cKeyEscape, VK_ESCAPE);
   setControlToVK(cKeySpace, VK_SPACE);
   setControlToVK(cKeyPageUp, VK_PRIOR);
   setControlToVK(cKeyPageDown, VK_NEXT);
   setControlToVK(cKeyEnd, VK_END);
   setControlToVK(cKeyHome, VK_HOME);
   setControlToVK(cKeyLeft, VK_LEFT);
   setControlToVK(cKeyUp, VK_UP);
   setControlToVK(cKeyRight, VK_RIGHT);
   setControlToVK(cKeyDown, VK_DOWN);
   setControlToVK(cKeyPrtSc, VK_SNAPSHOT);
   setControlToVK(cKeyInsert, VK_INSERT);
   setControlToVK(cKeyDelete, VK_DELETE);
   setControlToVK(cKey0, '0');
   setControlToVK(cKey1, '1');
   setControlToVK(cKey2, '2');
   setControlToVK(cKey3, '3');
   setControlToVK(cKey4, '4');
   setControlToVK(cKey5, '5');
   setControlToVK(cKey6, '6');
   setControlToVK(cKey7, '7');
   setControlToVK(cKey8, '8');
   setControlToVK(cKey9, '9');
   setControlToVK(cKeyA, 'A');
   setControlToVK(cKeyB, 'B');
   setControlToVK(cKeyC, 'C');
   setControlToVK(cKeyD, 'D');
   setControlToVK(cKeyE, 'E');
   setControlToVK(cKeyF, 'F');
   setControlToVK(cKeyG, 'G');
   setControlToVK(cKeyH, 'H');
   setControlToVK(cKeyI, 'I');
   setControlToVK(cKeyJ, 'J');
   setControlToVK(cKeyK, 'K');
   setControlToVK(cKeyL, 'L');
   setControlToVK(cKeyM, 'M');
   setControlToVK(cKeyN, 'N');
   setControlToVK(cKeyO, 'O');
   setControlToVK(cKeyP, 'P');
   setControlToVK(cKeyQ, 'Q');
   setControlToVK(cKeyR, 'R');
   setControlToVK(cKeyS, 'S');
   setControlToVK(cKeyT, 'T');
   setControlToVK(cKeyU, 'U');
   setControlToVK(cKeyV, 'V');
   setControlToVK(cKeyW, 'W');
   setControlToVK(cKeyX, 'X');
   setControlToVK(cKeyY, 'Y');
   setControlToVK(cKeyZ, 'Z');
   setControlToVK(cKeyAccent, VK_OEM_3);
   setControlToVK(cKeyMultiply, VK_MULTIPLY);
   setControlToVK(cKeyAdd, VK_ADD);
   setControlToVK(cKeySeparator, VK_SEPARATOR);
   setControlToVK(cKeySubtract, VK_SUBTRACT);
   setControlToVK(cKeyDecimal, VK_DECIMAL);
   setControlToVK(cKeyDivide, VK_DIVIDE);
   setControlToVK(cKeyF1, VK_F1);
   setControlToVK(cKeyF2, VK_F2);
   setControlToVK(cKeyF3, VK_F3);
   setControlToVK(cKeyF4, VK_F4);
   setControlToVK(cKeyF5, VK_F5);
   setControlToVK(cKeyF6, VK_F6);
   setControlToVK(cKeyF7, VK_F7);
   setControlToVK(cKeyF8, VK_F8);
   setControlToVK(cKeyF9, VK_F9);
   setControlToVK(cKeyF10, VK_F10);
   setControlToVK(cKeyF11, VK_F11);
   setControlToVK(cKeyF12, VK_F12);
   setControlToVK(cKeyF16, VK_F16);
   setControlToVK(cKeyShiftLeft, VK_LSHIFT);
   setControlToVK(cKeyShiftRight, VK_RSHIFT);
   setControlToVK(cKeyCtrlLeft, VK_LCONTROL);
   setControlToVK(cKeyCtrlRight, VK_RCONTROL);
   setControlToVK(cKeyAltLeft, VK_LMENU);
   setControlToVK(cKeyAltRight, VK_RMENU);
   setControlToVK(cKeyGreenModifier, VK_GREENMODIFIER);
   setControlToVK(cKeyOrangeModifier, VK_ORANGEMODIFIER);

   for(long i=cFirstKey; i<=cLastKey; i++)
      setVKToControl(mControlToVKTable[i], i);

   return true;
}

//==============================================================================
// BKeyboard::setControlToVK
//==============================================================================
void BKeyboard::setControlToVK(long control, long vk)
{
   if(control<0 || control>=cControlCount)
   {
      BASSERT(0);
      return;
   }
   if(vk<0 || vk > UINT16_MAX)
   {
      BASSERT(0);
      return;
   }
   BDEBUG_ASSERT(control <= UCHAR_MAX);
   mControlToVKTable[control] = static_cast<uint16>(vk);
}

//==============================================================================
// BKeyboard::setVKToControl
//==============================================================================
void BKeyboard::setVKToControl(long vk, long control)
{
   if(control<0 || control>=cControlCount)
   {
      BASSERT(0);
      return;
   }
   
   BDEBUG_ASSERT((vk >= 0) && (vk <= UINT16_MAX));
   BDEBUG_ASSERT((control >= 0) && (control <= UINT8_MAX));
   mVKToControlTable.insert(static_cast<uint16>(vk), static_cast<uint8>(control));
}

#ifdef XBOX
struct BChatPadKeyRemapping
{
   uint16 mOrig;
   uint16 mNew;
} gChatPadKeyRemapping[] = 
{
   { VK_GREENMODIFIER, VK_CONTROL },
   { VK_ORANGEMODIFIER, VK_MENU },
   { VK_1, VK_F1 },
   { VK_2, VK_F2 },
   { VK_3, VK_F3 },
   { VK_4, VK_F4 },
   { VK_5, VK_F5 },
   { VK_6, VK_F6 },
   { VK_7, VK_F7 },
   { VK_8, VK_F8 },
   { VK_9, VK_F9 },
   { VK_0, VK_F10 },
   { VK_BACK, VK_DELETE }
};
const uint cNumChatPadKeyRemappings = sizeof(gChatPadKeyRemapping) / sizeof(gChatPadKeyRemapping[0]);
#endif

//==============================================================================
// BKeyboard::update
//==============================================================================
void BKeyboard::update()
{
   for (long i = 0; i < cKeyCount; i++)
      mKeyActive[i] = (uchar)((mKeyActive[i] & ~cKeyPrevActiveFlag) | ((mKeyActive[i] & cKeyActiveFlag) ? cKeyPrevActiveFlag : 0));
   
   // Get the current key states
#ifdef XBOX
   for(;;)
   {
      XINPUT_KEYSTROKE keystroke;
      DWORD res = XInputGetKeystroke(XUSER_INDEX_ANY, XINPUT_FLAG_KEYBOARD, &keystroke);
      if (res != ERROR_SUCCESS)
         break;

#if 0            
      trace("VirtualKey: 0x%04X, Unicode: 0x%04X, HIDCode: 0x%02X, Down: %u, Up: %u",
         keystroke.VirtualKey,
         keystroke.Unicode,
         keystroke.HidCode,
         (keystroke.Flags & XINPUT_KEYSTROKE_KEYDOWN) != 0,
         (keystroke.Flags & XINPUT_KEYSTROKE_KEYUP) != 0
         );
#endif         
            
      if (mChatPadRemapping)         
      {
         for (uint i = 0; i < cNumChatPadKeyRemappings; i++)
         {
            if (keystroke.VirtualKey == gChatPadKeyRemapping[i].mOrig)
            {
               keystroke.VirtualKey = gChatPadKeyRemapping[i].mNew;
               break;
            }
         }
      }
                     
      BVKToControlHashMap::const_iterator it(mVKToControlTable.find(static_cast<uint16>(keystroke.VirtualKey)));
      if (it == mVKToControlTable.end())
         continue;
         
      long controlType = it->second;
      if (!controlType)
         continue;
         
      long index = controlType - cFirstKey;
      if (keystroke.Flags & XINPUT_KEYSTROKE_KEYDOWN)
         mKeyActive[index] |= cKeyActiveFlag;
      else if (keystroke.Flags & XINPUT_KEYSTROKE_KEYUP)
         mKeyActive[index] &= ~cKeyActiveFlag;
   }
#else
   for(long i=cFirstKey; i<=cLastKey; i++)
   {
      long index=i-cFirstKey;
      SHORT keyState=GetAsyncKeyState(mControlToVKTable[i]);
      if(keyState&0x8000)
         mKeyActive[index] |= cKeyActiveFlag;
      else
         mKeyActive[index] &= ~cKeyActiveFlag;
   }
#endif

   // Send out control events
   BInputEventDetail detail;
   for(long i=0; i<cKeyCount; i++)
   {
      long controlType=cFirstKey+i;
      if(mKeyActive[i] & cKeyActiveFlag)
      {
         detail.mAnalog=1.0f;
         if (0 == (mKeyActive[i] & cKeyPrevActiveFlag))
            gInputSystem.handleInput(mPort, cInputEventControlStart, controlType, detail);
         else
            gInputSystem.handleInput(mPort, cInputEventControlRepeat, controlType, detail);
      }
      else
      {
         detail.mAnalog=0.0f;
         if (mKeyActive[i] & cKeyPrevActiveFlag)
            gInputSystem.handleInput(mPort, cInputEventControlStop, controlType, detail);
      }
   }
}

//==============================================================================
// BKeyboard::isKeyActive
//==============================================================================
bool BKeyboard::isKeyActive(long controlType) const
{
   long index=controlType-cFirstKey;
   if(index<0 || index>=cKeyCount)
      return false;
   return (mKeyActive[index] & cKeyActiveFlag) != 0;
}

//==============================================================================
// BKeyboard::isKeyPrevActive
//==============================================================================
bool BKeyboard::isKeyPrevActive(long controlType) const
{
   long index=controlType-cFirstKey;
   if(index<0 || index>=cKeyCount)
      return false;
   return (mKeyActive[index] & cKeyPrevActiveFlag) != 0;
}

//==============================================================================
// BKeyboard::wasKeyPressed
//==============================================================================
bool BKeyboard::wasKeyPressed(long controlType) const
{
   long index=controlType-cFirstKey;
   if(index<0 || index>=cKeyCount)
      return false;   

   return mKeyActive[index] == cKeyActiveFlag;
}

#ifndef BUILD_FINAL
   //==============================================================================
   // BKeyboard::simulateKeyActive
   //==============================================================================
   bool BKeyboard::simulateKeyActive(long controlType, bool active)
   {
      long index=controlType-cFirstKey;
      if(index<0 || index>=cKeyCount)
         return false;
      uchar oldValue = mKeyActive[index];
      // Simulate this key as "active" or "not active" as requested with active flag
      if (active)
         mKeyActive[index] |= cKeyActiveFlag;
      else
         mKeyActive[index] &= ~cKeyActiveFlag;
      return (oldValue != 0);
   }
#endif
