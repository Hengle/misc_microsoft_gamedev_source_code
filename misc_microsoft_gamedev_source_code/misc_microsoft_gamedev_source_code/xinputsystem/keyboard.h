//==============================================================================
// keyboard.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#pragma once

// Includes
#include "inputcontrolenum.h"
#include "containers\bitArray2D.h"

//==============================================================================
// BKeyboard
//==============================================================================
class BKeyboard
{
   public:
                           BKeyboard();
                           ~BKeyboard();

      bool                 setup();
      void                 update();
      
      void                 setChatPadRemapping(bool enabled) { mChatPadRemapping = enabled; }
      bool                 getChatPadRemapping() const { return mChatPadRemapping;}
      
      bool                 isKeyActive(long controlType) const;
      bool                 isKeyPrevActive(long controlType) const;
      bool                 wasKeyPressed(long controlType) const;

#ifndef BUILD_FINAL
      bool                 simulateKeyActive(long controlType, bool active);
#endif

      void                 setPort(long port) { mPort = port; }

   protected:
            
      void                 setControlToVK(long control, long vk);
      void                 setVKToControl(long vk, long control);

      uint16               mControlToVKTable[cControlCount];
      
      typedef BHashMap<uint16, uint8> BVKToControlHashMap;
      BVKToControlHashMap  mVKToControlTable;
            
      enum 
      { 
         cKeyActiveFlag = 1,
         cKeyPrevActiveFlag = 2
      };
      
      uchar                mKeyActive[cKeyCount];

      long                 mPort;
      
      bool                 mChatPadRemapping;
};
