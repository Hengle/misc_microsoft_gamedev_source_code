//==============================================================================
// unitactionstasis.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"


//==============================================================================
//==============================================================================
class BUnitActionStasis : public BAction
{
   public:

      BUnitActionStasis();
      virtual ~BUnitActionStasis() { }
      virtual void               disconnect();

      virtual bool               init();

      virtual bool               update(float elapsed);


      DECLARE_FREELIST(BUnitActionStasis, 5);

   protected:

      bool                       grabControllers();
      void                       releaseControllers();

};