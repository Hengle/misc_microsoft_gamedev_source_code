//==============================================================================
// unitactiondome.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "interptable.h"
#include "containers\dynamicArray.h"

class BVisualRenderAttributes;
class BObject;
class BGrannyModel;
class BVisual;

//==============================================================================
//==============================================================================
class BMeshOpacityAnim
{
   public:
      BMeshOpacityAnim() : mMeshIndex(-1) { }
      virtual ~BMeshOpacityAnim() { }
         
      int mMeshIndex;
      BFloatLerpTable mOpacityTable;
};
typedef BSmallDynamicSimArray<BMeshOpacityAnim> BModelOpacityAnim;

//==============================================================================
//==============================================================================
class BUnitActionDome : public BAction
{
   public:
      BUnitActionDome() { }
      virtual ~BUnitActionDome();

      virtual bool   connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void   disconnect();
      virtual bool   init();
      virtual bool   setState(BActionState state);
      virtual bool   update(float elapsed);

      void           render(BVisualRenderAttributes* pRenderAttributes, DWORD subUpdate, DWORD subUpdatesPerUpdate, float elapsedSubUpdateTime);
      static void    renderStatic(BObject* pObject, BVisualRenderAttributes* pRenderAttributes, DWORD subUpdate, DWORD subUpdatesPerUpdate, float elapsedSubUpdateTime);

      DECLARE_FREELIST(BUnitActionDome, 1);

      virtual bool   save(BStream* pStream, int saveType) const;
      virtual bool   load(BStream* pStream, int saveType);

   protected:

      void           updateAnimTime(float elapsed);
      static bool    initMeshAnimData(BObject* pObject);
      static bool    loadMeshAnim(const char* filename, BGrannyModel* pGrannyModel);

      BDynamicSimArray<BVisual*> mMeshVisuals;
      float          mAnimTime;

      enum
      {
         cStateHole1Forward,
         cStateHole1Reverse,
         cStateHole2Forward,
         cStateHole2Reverse,
         cStateHole3Forward,
         cStateHole3Reverse,
         cStateFastHoleForward,
         cStateFastHoleReverse,
         cStateHoleIdle
      };
      BYTE           mHoleState;

      // Static animation data
      static BSmallDynamicSimArray<BModelOpacityAnim> mModelOpacityAnimArray;
      static float                              mHole1MaxAnimTime;
      static float                              mHole2MaxAnimTime;
      static float                              mHole3MaxAnimTime;
      static float                              mFastHoleMaxAnimTime;
      static int                                mMeshAnimDataRefCount;
      static int                                mNumMeshes;
};