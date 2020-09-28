//==============================================================================
// prepost.h
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

#pragma once
#include "unit.h"

//==============================================================================
//This class is used to store some temp variables
//on the stack before and after a unit has his 
//position/orientation changed.
//==============================================================================

class BPrePost
{
   public:
      explicit BPrePost(BUnit& value);
      void prePositionChanged();
      void postPositionChanged();
   private:
      BUnit&   mV;
      BVector  mHardpointPos;
      int      mHardpointID;
};