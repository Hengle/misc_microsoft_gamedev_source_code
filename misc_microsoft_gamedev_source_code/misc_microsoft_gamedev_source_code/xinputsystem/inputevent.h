//==============================================================================
// inputevent.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#ifndef _INPUTEVENT_H_
#define _INPUTEVENT_H_

// Includes
#include "bitarray.h"
#include "bitvector.h"
#include "point.h"

//==============================================================================
// BInputEvent
//==============================================================================
class BInputEvent
{
   public:
      enum 
      {
         cEventGamepad,
         cEventKey,
         cEventNumTypes
      };

                     BInputEvent();
      virtual        ~BInputEvent();

      virtual void   clear();

      // Variables
      long           mType;
      BBitVector     mFlags;
      BBitArray      mContext;

};

typedef BDynamicSimArray<BInputEvent *> BInputEventArray;

//==============================================================================
// BInputEventGamepad
//==============================================================================
class BInputEventGamepad : public BInputEvent
{
   public:
                     BInputEventGamepad();

      virtual void   clear();

      long           mPort;
      long           mEvent;
      long           mControlType;
      float          mX;
      float          mY;
      float          mAnalog;
};

//==============================================================================
// BInputEventKey
//==============================================================================
class BInputEventKey : public BInputEvent
{
   public:
                     BInputEventKey();

      virtual void   clear(void);

      long           mKey;
      long           mSubtype;
      BBitVector     mModifiers;
};

#endif
