//==============================================================================
// burningEffectLimits.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "simtypes.h"

typedef uint BBurningEffectLimitID;
typedef BUInt8<BBurningEffectLimitID, UINT8_MIN, UINT8_MAX> BBurningEffectLimitSmall;
__declspec(selectany) extern const BBurningEffectLimitID cInvalidBurningEffectLimitID = 0;

//==============================================================================
// class BBurningEffectLimit
//==============================================================================
class BBurningEffectLimit
{
public:

   BBurningEffectLimitSmall mID;    // 1 byte
   int            mLimit;           // 2 bytes
   BSimString     mObjectType;      // 2 bytes

   BBurningEffectLimit& operator= (const BBurningEffectLimit& v)
   {
      this->mID            = v.mID;
      this->mLimit         = v.mLimit;
      this->mObjectType    = v.mObjectType;
      return (*this);
   }
};
