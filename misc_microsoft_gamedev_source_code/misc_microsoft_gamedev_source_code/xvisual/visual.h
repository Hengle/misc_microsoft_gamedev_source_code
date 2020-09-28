//==============================================================================
// visual.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "visualrenderattributes.h"
#include "visualitem.h"
#include "visualattachment.h"

// Forward declarations
class BBoundingBox;
class BProtoVisual;
class IVisualInstance;

//============================================================================
// Point handles
//============================================================================
#define POINTFROMPOINTHANDLE(n)  ((n)&0x00FFFFFF)
#define ITEMFROMPOINTHANDLE(n)   (((n)&0xFF000000)>>24) 
#define POINTHANDLE(item, point) (((item)<<24)|point)

//==============================================================================
// BVisual
//==============================================================================
class BVisual : public BVisualItem
{
   public:
                                 BVisual();
      virtual                    ~BVisual();

      virtual bool               init(BProtoVisual* pProtoVisual, int64 userData, bool synced, DWORD tintColor, const BMatrix& worldMatrix, int displayPriority);
      virtual bool               clone(const BVisual* pSource, bool synced, int64 userData, bool bDisregardAttachments, DWORD tintColor, const BMatrix& worldMatrix);
      virtual bool               clone(const BVisualItem* pSource, bool synced, int64 userData, bool bDisregardAttachments, DWORD tintColor, const BMatrix& worldMatrix);
      virtual void               deinit();

      virtual void               update(float elapsedTime, DWORD tintColor, const BMatrix& matrix, DWORD subUpdate);
      void                       updatePreAsync(float elapsedTime, DWORD tintColor, const BMatrix& matrix, DWORD subUpdate, bool animationEnabled = true);
      void                       updateAsync(float elapsedTime, DWORD subUpdate);

      virtual void               setAnimation(long animationTrack, long animType, bool applyInstantly, DWORD tintColor, const BMatrix& worldMatrix, float timeIntoAnimation = 0.0f, long forceAnimID = -1, bool reset = false, BVisualItem* startOnThisAttachment = NULL, const BProtoVisualAnimExitAction* pOverrideExitAction = NULL);
      virtual void               copyAnimationTrack(long fromTrack, long toTrack, bool applyInstantly, DWORD tintColor, const BMatrix& worldMatrix, BVisualItem* startOnThisAttachment = NULL);
      BVisualAnimationData       getAnimationData(long animationTrack, long animType);

      void                       setUserData(int64 val) { mUserData=val; }
      int64                      getUserData() const { return mUserData; }

      BProtoVisual*              getProtoVisual() const { return mpProtoVisual; }
      const BProtoVisual*        getProtoVisualConst() const { return mpProtoVisual; }
      long                       getProtoVisualID() const;

      virtual void               onAcquire();
      virtual void               onRelease();

      bool                       hasAnimation(long animType) const;

      DECLARE_FREELIST(BVisual, 10);

      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType, BProtoVisual* pProtoVisual, const BMatrix& worldMatrix, DWORD tintColor);
      virtual bool postLoad(int saveType, const BMatrix& worldMatrix, DWORD tintColor);

   protected:
      int64                      mUserData;
      BProtoVisual*              mpProtoVisual;
      DWORD                      mPrevProtoVisualGeneration;
};
