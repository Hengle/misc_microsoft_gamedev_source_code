//============================================================================
// ui3dcirclemenu.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#pragma once 

//#include "flashcirclemenuradial1.h"
#include "cost.h"
#include "pop.h"
#include "visual.h"
#include "sceneLightManager.h"

class BFlashCircleMenuRadial1;
class BFlashCircleMenuRadial2;
class BFlashCircleMenuRadial3;

//==============================================================================
// BUICircleMenuItem
//==============================================================================
class BUI3DCircleMenuItem
{
public:
   BUI3DCircleMenuItem() : mOrder(-1), mID(-1), mCost(), mPops(), mpVisual(NULL), mVisualScale(1.0f), mTrainCount(-1), mTrainLimit(-1), mTechPrereqID(-1), mInfoText(), mInfoDetail() {}
   BUI3DCircleMenuItem(const BUI3DCircleMenuItem& source) { *this=source; }
   BUI3DCircleMenuItem& operator=(const BUI3DCircleMenuItem& source)
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
      mInfoText=source.mInfoText;
      mInfoDetail=source.mInfoDetail;
      mpVisual=source.mpVisual;
      mVisualScale=source.mVisualScale;
      return *this;
   }

   long                    mOrder;
   long                    mID;
   BCost                   mCost;
   BPopArray               mPops;
   long                    mTrainCount;
   long                    mTrainLimit;
   long                    mTechPrereqID;
   BUString                mInfoText;
   BUString                mInfoDetail;
   BVisual*                mpVisual;
   float                   mVisualScale;
};

class BUI3DCircleMenu
{
   public:
      BUI3DCircleMenu();
     ~BUI3DCircleMenu();
     
     enum BUICircleMenuBoneType
     {
        eBoneMain,
        eBoneRadialPanel01,
        eBoneRadialPanelPosStart,
        eBoneRadialPanelPos1 = eBoneRadialPanelPosStart,
        eBoneRadialPanelPos2,
        eBoneRadialPanelPos3,
        eBoneRadialPanelPos4,
        eBoneRadialPanelPos5,
        eBoneRadialPanelPos6,
        eBoneRadialPanelPos7,
        eBoneRadialPanelPos8,
        eBoneTotal
     };

      void init();
      void deinit();

      void update(float elapsedTime);
      void refresh();
      void renderFlash();
      void render();

      void clearItems();
      long getItemIndex(long id) const;
      long addItem(long order, long id, const BCost* pCost, BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, const BUString& label, const BUString& infoText, const BUString& infoDetail, const char* visualName);
      long addItem(long order, long id, const BCost* pCost, BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, const BUString& label, const BUString& infoText, const BUString& infoDetail, BVisual* pVisual, float visualScale = 1.0f);
      void removeItem(long index);
      void setItemLabel(long index, const BUString& label);
      long getNumberItems() const { return mItems.getNumber(); }
      virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
      

   private:
      void updatePointingAt(float x, float y, bool playSound);
      long calcCircleIndex(float x, float y, long indexCount, float offset);

      BDynamicArray<int>                    mBoneHandle;
      BDynamicArray<BVisual*>               mModels;
      BDynamicSimArray<BUI3DCircleMenuItem> mItems;
      BDirLightParams                       mDirLightParams;
      BFlashCircleMenuRadial1*              mpRadial1;
      BFlashCircleMenuRadial2*              mpRadial2;
      BFlashCircleMenuRadial3*              mpRadial3;      
      int                                   mCurrentItem;
      int                                   mCircleCount;
      float                                 mPointingAtX;
      float                                 mPointingAtY;
      int                                   mPlayerID;
};