//==============================================================================
// unitactionchangeowner.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionChangeOwner : public BAction
{
   public:
      BUnitActionChangeOwner() { }
      virtual ~BUnitActionChangeOwner() { }

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      DECLARE_FREELIST(BUnitActionChangeOwner, 5);

   protected:

};