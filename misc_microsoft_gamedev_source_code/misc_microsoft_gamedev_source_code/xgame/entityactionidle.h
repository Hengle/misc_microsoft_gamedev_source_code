//==============================================================================
// entityactionidle.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BEntityActionIdle : public BAction
{
   public:
      BEntityActionIdle() { }
      virtual ~BEntityActionIdle() { }

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);
      virtual bool               setState(BActionState state);

      virtual bool               isAllowedWhenGameOver() { return true; }

      // How long has this idle action been "idling"
      DWORD                      getIdleDuration() const { return(mIdleDuration); }

      DECLARE_FREELIST(BEntityActionIdle, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      DWORD                      mIdleDuration;
};
