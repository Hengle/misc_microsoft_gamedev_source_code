//==============================================================================
// pop.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

#include "math\halfFloat.h"

//==============================================================================
// BPop
//==============================================================================
class BPop
{
   public:
      BPop() :
         mID(-1),
         mCount(0.0f)
      {
      }

      BPop(const BPop& source)
      { 
         *this=source; 
      }

      BPop& operator=(const BPop& source)
      {
         if(this==&source)
            return *this;
         mID=source.mID;
         mCount=source.mCount;
         return *this;
      }

      short                   mID;
      BHalfFloat              mCount;
};

typedef BSmallDynamicSimArray<BPop> BPopArray;