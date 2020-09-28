//==============================================================================
// unitactioncoreslide.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "convexhull.h"

//==============================================================================
//==============================================================================
class BSlideArea
{
   public:
      BSlideArea() {}
      virtual ~BSlideArea() {}

      //Save/load.
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BConvexHull mHull;
      BVector mStartPt1;
      BVector mStartPt2;
};

//==============================================================================
//==============================================================================
class BUnitActionCoreSlide : public BAction
{
   public:
      BUnitActionCoreSlide() : mInArea(-1), mSlideAreasInitialized(false) { }
      virtual ~BUnitActionCoreSlide() { }

      virtual bool   connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void   disconnect();
      virtual bool   init();
      virtual bool   setState(BActionState state);
      virtual bool   update(float elapsed);

      DECLARE_FREELIST(BUnitActionCoreSlide, 1);

      virtual bool   save(BStream* pStream, int saveType) const;
      virtual bool   load(BStream* pStream, int saveType);

   protected:

      void           initSlideAreas();
      void           updateAreaLocation();
      void           calcDesiredRestPos();
      void           updateSlide(float elapsed);

      BSmallDynamicSimArray<BSlideArea> mSlideAreas;
      BVector        mDesiredRestPos;
      int            mInArea;
      bool           mSlideAreasInitialized:1;
};