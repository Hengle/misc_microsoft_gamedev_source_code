//==============================================================================
// unitactionhonk.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionHonk : public BAction
{
   public:
      BUnitActionHonk() { }
      virtual ~BUnitActionHonk() { }

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);
      void                       honk();

      virtual void               setTarget(BSimTarget target) { mTarget=target; }

      DECLARE_FREELIST(BUnitActionHonk, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      BSimTarget                 mTarget;
};