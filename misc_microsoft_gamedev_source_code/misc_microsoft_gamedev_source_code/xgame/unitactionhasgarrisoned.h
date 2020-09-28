//==============================================================================
// unitactionhasgarrisoned.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionHasGarrisoned : public BAction
{
   public:
      BUnitActionHasGarrisoned() { }
      virtual ~BUnitActionHasGarrisoned() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      //virtual void               disconnect();
      //Init.
      virtual bool               init();

      //virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      DECLARE_FREELIST(BUnitActionHasGarrisoned, 5);

   protected:

};