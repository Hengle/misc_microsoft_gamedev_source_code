//==============================================================================
// UnitActionScaleLOS.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BObject;
//==============================================================================
//==============================================================================
class BUnitActionScaleLOS : public BAction
{
   public:
      BUnitActionScaleLOS() { }
      virtual ~BUnitActionScaleLOS() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      void                       setStartValue(float v) { mStartValue = v; }
      void                       setFinishValue(float v) { mFinishValue = v; }
      void                       setDuration(DWORD d);


      DECLARE_FREELIST(BUnitActionScaleLOS, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      float                      mStartValue;
      float                      mFinishValue;
      DWORD                      mFinishTime;
      DWORD                      mDuration;
};