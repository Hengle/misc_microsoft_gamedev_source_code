//==============================================================================
// unitactionplayattachmentanims.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitAttachmentAnim
{
   public:
      int   mAttachmentHandle;
      int   mAnimType;
};

//==============================================================================
//==============================================================================
class BUnitAttachmentOnEventAction
{
   public:
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      int   mActionType;
      int   mAttachmentHandle;
      int   mAnimType;
      DWORD mEventType;
      DWORD mData1;
      DWORD mData2;
      bool  mUseData1;
      bool  mUseData2;
};

//==============================================================================
//==============================================================================
class BUnitActionPlayAttachmentAnims : public BAction
{
   public:
      enum
      {
         cActionPlayAnim,
         cActionResetAnim,
      };

      BUnitActionPlayAttachmentAnims();
      virtual ~BUnitActionPlayAttachmentAnims() { }

      virtual bool               init();

      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      void                       addOnEventAction(int actionType, int attachmentHandle, int animType, int eventType, int data1, int data2, bool useData1, bool useData2);

      DECLARE_FREELIST(BUnitActionPlayAttachmentAnims, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      BSmallDynamicSimArray<BUnitAttachmentOnEventAction> mOnEventActions;
};