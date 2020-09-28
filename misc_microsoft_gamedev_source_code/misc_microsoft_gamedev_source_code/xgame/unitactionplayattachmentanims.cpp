//==============================================================================
// unitactionplayattachmentanims.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionplayattachmentanims.h"
#include "unit.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionPlayAttachmentAnims, 5, &gSimHeap);

//==============================================================================
//==============================================================================
BUnitActionPlayAttachmentAnims::BUnitActionPlayAttachmentAnims()
{
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayAttachmentAnims::init()
{
   if (!BAction::init())
      return(false);
   mFlagPersistent=true;
   mOnEventActions.clear();
   return(true);
}


//==============================================================================
//==============================================================================
bool BUnitActionPlayAttachmentAnims::update(float elapsed)
{
   return BAction::update(elapsed);
}

//==============================================================================
//==============================================================================
void BUnitActionPlayAttachmentAnims::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   for (int i=0; i<mOnEventActions.getNumber(); i++)
   {
//-- FIXING PREFIX BUG ID 1823
      const BUnitAttachmentOnEventAction& action = mOnEventActions[i];
//--
      if (eventType == action.mEventType)
      {
         if (action.mUseData1 && data1 != action.mData1)
            continue;
         if (action.mUseData2 && data2 != action.mData2)
            continue;
         switch (action.mActionType)
         {
            case cActionPlayAnim:
               pUnit->playAttachmentAnim(action.mAttachmentHandle, action.mAnimType);
               break;
            case cActionResetAnim:
               pUnit->resetAttachmentAnim(action.mAttachmentHandle);
               break;
         }
         mOnEventActions.removeIndex(i);
         i--;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPlayAttachmentAnims::addOnEventAction(int actionType, int attachmentHandle, int animType, int eventType, int data1, int data2, bool useData1, bool useData2)
{
   BUnitAttachmentOnEventAction action;
   action.mActionType=actionType;
   action.mAttachmentHandle=attachmentHandle;
   action.mAnimType=animType;
   action.mEventType=eventType;
   action.mData1=data1;
   action.mData2=data2;
   action.mUseData1=useData1;
   action.mUseData2=useData2;
   mOnEventActions.add(action);
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayAttachmentAnims::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASSARRAY(pStream, saveType, mOnEventActions, uint8, 200);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPlayAttachmentAnims::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASSARRAY(pStream, saveType, mOnEventActions, uint8, 200);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitAttachmentOnEventAction::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, int, mActionType);
   GFWRITEVAR(pStream, int, mAttachmentHandle);
   GFWRITEVAR(pStream, int, mAnimType);
   GFWRITEVAR(pStream, DWORD, mEventType);
   GFWRITEVAR(pStream, DWORD, mData1);
   GFWRITEVAR(pStream, DWORD, mData2);
   GFWRITEVAR(pStream, bool, mUseData1);
   GFWRITEVAR(pStream, bool, mUseData2);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitAttachmentOnEventAction::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, int, mActionType);
   GFREADVAR(pStream, int, mAttachmentHandle);
   GFREADVAR(pStream, int, mAnimType);
   GFREADVAR(pStream, DWORD, mEventType);
   GFREADVAR(pStream, DWORD, mData1);
   GFREADVAR(pStream, DWORD, mData2);
   GFREADVAR(pStream, bool, mUseData1);
   GFREADVAR(pStream, bool, mUseData2);
   return true;
}
