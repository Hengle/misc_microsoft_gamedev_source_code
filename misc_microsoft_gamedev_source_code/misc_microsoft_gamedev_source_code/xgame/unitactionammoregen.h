//==============================================================================
// unitactionammoregen.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionAmmoRegen : public BAction
{
   public:
      BUnitActionAmmoRegen() { }
      virtual ~BUnitActionAmmoRegen() { }

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      DECLARE_FREELIST(BUnitActionAmmoRegen, 5);

   protected:
};