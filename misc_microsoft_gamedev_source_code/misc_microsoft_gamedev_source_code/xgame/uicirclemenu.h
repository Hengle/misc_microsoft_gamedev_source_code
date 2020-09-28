//==============================================================================
// uicirclemenu.h
//
// Copyright (c) 2004-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "cost.h"
#include "pop.h"
#include "uielement.h"

class BUser;

//==============================================================================
// BUICircleMenuItem
//==============================================================================
class BUICircleMenuItem
{
   public:
      BUICircleMenuItem() : mOrder(-1), mID(-1), mCost(), mPops(), mTrainCount(-1), mTrainLimit(-1), mTechPrereqID(-1), mButton(), mInfoText(), mInfoDetail(), mUnavailable(false) {}
      BUICircleMenuItem(const BUICircleMenuItem& source) { *this=source; }
      BUICircleMenuItem& operator=(const BUICircleMenuItem& source)
      {
         if(this==&source)
            return *this;
         mOrder=source.mOrder;
         mID=source.mID;
         mCost=source.mCost;
         mPops=source.mPops;
         mTrainCount=source.mTrainCount;
         mTrainLimit=source.mTrainLimit;
         mTechPrereqID=source.mTechPrereqID;
         mButton=source.mButton;
         mInfoText=source.mInfoText;
         mInfoDetail=source.mInfoDetail;
         mUnavailable=source.mUnavailable;
         return *this;
      }

      long                    mOrder;
      long                    mID;
      BCost                   mCost;
      BPopArray               mPops;
      long                    mTrainCount;
      long                    mTrainLimit;
      long                    mTechPrereqID;
      BUIElement              mButton;
      BUString                mInfoText;
      BUString                mInfoDetail;
      bool                    mUnavailable;

};

//==============================================================================
// BUICircleMenu
//==============================================================================
class BUICircleMenu : public BUIElement
{
   public:
                              BUICircleMenu();
      virtual                 ~BUICircleMenu();

      void                    setCircleWidth(long width) { mCircleWidth=width; }
      void                    setCircleCount(long count) { mCircleCount=count; }
      void                    setItemRadius(float radius) { mItemRadius=radius; }
      void                    setItemWidth(long width) { mItemWidth=width; }
      void                    setBaseText(const WCHAR* pText) { mBaseText=pText; }
      void                    setBaseTextDetail(const WCHAR* pText) { mBaseTextDetail=pText; }
      void                    setColors(DWORD unlitColor, DWORD litColor, DWORD unavailColor, DWORD unavailLitColor) { mUnlitColor=unlitColor; mLitColor=litColor; mUnavailableColor=unavailColor; mUnavailableLitColor=unavailLitColor; }
      void                    setPlayer(long id) { mPlayerID=id; }

      void                    autoPosition(BUser* pUser);

      virtual void            refresh();
      virtual void            update();
      virtual void            render(long parentX, long parentY);
      virtual bool            handleInput(long port, long event, long controlType, BInputEventDetail& detail);
      virtual void            cleanup();

      void                    resetPointingAt() { mCurrentItem=-1; mPointingAtX=0.0f; mPointingAtY=0.0f; }

      void                    clearItems();
      long                    addItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, bool unavailable, const BUString& label, const BUString& infoText, const BUString& infoDetail, const char* pTextureName);
      long                    addItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, bool unavailable, const BUString& label, const BUString& infoText, const BUString& infoDetail, BManagedTextureHandle textureHandle);
      void                    removeItem(long index);
      void                    setItemLabel(long index, const BUString& label);
      long                    getNumberItems() const { return mItems.getNumber(); }
      long                    getItemID(long index) const { return mItems[index].mID; }
      bool                    getItemUnavailable(long index) const { return mItems[index].mUnavailable; }
      long                    getItemIndex(long id) const;
      long                    getItemOrder(long index) const { return mItems[index].mOrder; }
      long                    getItemIndexByOrder(long order) const;
      long                    getCurrentItemIndex() const { return mCurrentItem; }
      long                    getCurrentItemID() const { return ((mCurrentItem == -1) ? -1 : mItems[mCurrentItem].mID); } 
      long                    getCurrentItemOrder() const { return ((mCurrentItem == -1) ? -1 : mItems[mCurrentItem].mOrder); } 
      BCost*                  getCurrentItemCost() { return ((mCurrentItem == -1) ? NULL : &(mItems[mCurrentItem].mCost)); }
      BPopArray*              getCurrentItemPops() { return ((mCurrentItem == -1) ? NULL : &(mItems[mCurrentItem].mPops)); }

   protected:
      void                    updatePointingAt(float x, float y, bool playSound, bool doRumble);
      float                   positionButton(BUIElement* pButton, long x, long y, long xs, long ys, float pos, long count, float radius, bool useStartVal, float startVal);
      long                    calcCircleIndex(float x, float y, long indexCount, float offset);
      void                    updateInfoText();

      long                    mCircleWidth;
      long                    mCircleCount;
      float                   mItemRadius;
      long                    mItemWidth;
      DWORD                   mUnlitColor;
      DWORD                   mLitColor;
      DWORD                   mUnavailableColor;
      DWORD                   mUnavailableLitColor;

      enum
      {
         cMaxInfoLines=10,
         cMaxCostItems=4,
      };

      long                             mCurrentItem;
      BDynamicSimArray<BUICircleMenuItem> mItems;

      BUString                mBaseText;
      BUString                mBaseTextDetail;

      float                   mPointingAtX;
      float                   mPointingAtY;

      BUIElement              mInfoLines[cMaxInfoLines];
      BUIElement              mCostAmounts[cMaxCostItems];
      BUIElement              mCostIcons[cMaxCostItems];
      long                    mCostType[cMaxCostItems];
      long                    mCostID[cMaxCostItems];
      float                   mCostValue[cMaxCostItems];

      long                    mPlayerID;

};