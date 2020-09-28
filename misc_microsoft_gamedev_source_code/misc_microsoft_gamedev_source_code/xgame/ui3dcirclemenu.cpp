//============================================================================
// ui3dcirclemenu.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#include "common.h"
#include "flashcirclemenuradial1.h"
#include "flashcirclemenuradial2.h"
#include "flashcirclemenuradial3.h"
#include "flashinfopanel.h"
#include "flashbackground.h"
#include "ui3dcirclemenu.h"
#include "visualmanager.h"
#include "visual.h"
#include "uimanager.h"
#include "uigame.h"
#include "scenelightmanager.h"


class B3DCircleMenuBoneLookup
{
public:
   int      mEnum;
   char     mName[64];
};
#define BONEENUM(type, str) {BUI3DCircleMenu::type, str},
B3DCircleMenuBoneLookup g3DCircleMenuBoneLookup[BUI3DCircleMenu::eBoneTotal]=
{
   BONEENUM(eBoneMain,           "bone_main")
   BONEENUM(eBoneRadialPanel01,  "bone_radial_panel01")
   BONEENUM(eBoneRadialPanelPos1,"bone_radial_pos01")
   BONEENUM(eBoneRadialPanelPos2,"bone_radial_pos02")
   BONEENUM(eBoneRadialPanelPos3,"bone_radial_pos03")
   BONEENUM(eBoneRadialPanelPos4,"bone_radial_pos04")
   BONEENUM(eBoneRadialPanelPos5,"bone_radial_pos05")
   BONEENUM(eBoneRadialPanelPos6,"bone_radial_pos06")
   BONEENUM(eBoneRadialPanelPos7,"bone_radial_pos07")
   BONEENUM(eBoneRadialPanelPos8,"bone_radial_pos08")
};

//==============================================================================
// BUICircleMenu::BUICircleMenu
//==============================================================================
BUI3DCircleMenu::BUI3DCircleMenu() : 
mItems(),
mCurrentItem(-1),
mPlayerID(-1),
mPointingAtX(0.0f),
mPointingAtY(0.0f),
mCircleCount(8),
mpRadial1(NULL),
mpRadial2(NULL),
mpRadial3(NULL)
{
}

//==============================================================================
// BUI3DCircleMenu::~BUI3DCircleMenu
//==============================================================================
BUI3DCircleMenu::~BUI3DCircleMenu()
{
}

//==============================================================================
// BUI3DCircleMenu::init()
//==============================================================================
void BUI3DCircleMenu::init()
{
   deinit();

   mModels.resize(3);
   mModels[0] = gUIGame.get3DUIPanel(BUIGame::e3DUIRadialPanel1);
   mModels[1] = gUIGame.get3DUIPanel(BUIGame::e3DUIRadialPanel2);
   mModels[2] = gUIGame.get3DUIPanel(BUIGame::e3DUIRadialPanel3);   

   mBoneHandle.resize(eBoneTotal);
   int handle = -1;
   for (int i = 0; i < eBoneTotal; i++)
   {
      handle = mModels[0]->getBoneHandle(g3DCircleMenuBoneLookup[i].mName);
      mBoneHandle[i] = handle;
   }

   mpRadial1 = new BFlashCircleMenuRadial1();   
   mpRadial1->init(gUIGame.get3DUIFlashMovie(BUIGame::e3DUIRadialPanel1));

   mpRadial2 = new BFlashCircleMenuRadial2();
   mpRadial2->init(gUIGame.get3DUIFlashMovie(BUIGame::e3DUIRadialPanel2));

   mpRadial3 = new BFlashCircleMenuRadial3();
   mpRadial3->init(gUIGame.get3DUIFlashMovie(BUIGame::e3DUIRadialPanel3));   

   mDirLightParams = gSimSceneLightManager.getDirLight(cLCUI);
}

//==============================================================================
// BUI3DCircleMenu::deinit()
//==============================================================================
void BUI3DCircleMenu::deinit()
{
   int count = mModels.getNumber();
   for (int i = 0; i < count; ++i)
   {
      mModels[i] = NULL;
   }
   mBoneHandle.resize(0);
   clearItems();

   if (mpRadial1)
      delete mpRadial1;
   mpRadial1 = NULL;

   if (mpRadial2)
      delete mpRadial2;
   mpRadial2 = NULL;

   if (mpRadial3)
      delete mpRadial3;
   mpRadial3 = NULL;   
}

//==============================================================================
// BUI3DCircleMenu::clearItems
//==============================================================================
void BUI3DCircleMenu::clearItems()
{
   mItems.clear();
   mCurrentItem=-1;
}

//==============================================================================
// BUI3DCircleMenu::addItem
//==============================================================================
long BUI3DCircleMenu::addItem(long order, long id, const BCost* pCost, BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, const BUString& label, const BUString& infoText, const BUString& infoDetail, const char* visualName)
{
   BUI3DCircleMenuItem item;
   item.mOrder=order;
   item.mID=id;
   if(pCost)
      item.mCost=*pCost;
   if(pPops)
   {
      long count=pPops->getNumber();
      for(long i=0; i<count; i++)
      {
         BPop pop=pPops->get(i);
         item.mPops.add(pop);
      }
   }
   item.mTrainCount=trainCount;
   item.mTrainLimit=trainLimit;
   item.mTechPrereqID=techPrereqID;
   item.mInfoText=infoText;
   item.mInfoDetail=infoDetail;
   long index=mItems.add(item);
   return index;
}

//==============================================================================
// BUI3DCircleMenu::addItem
//==============================================================================
long BUI3DCircleMenu::addItem(long order, long id, const BCost* pCost, BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, const BUString& label, const BUString& infoText, const BUString& infoDetail, BVisual* pVisual, float visualScale)
{
   BUI3DCircleMenuItem item;
   item.mOrder=order;
   item.mID=id;
   if(pCost)
      item.mCost=*pCost;
   if(pPops)
   {
      long count=pPops->getNumber();
      for(long i=0; i<count; i++)
      {
         BPop pop=pPops->get(i);
         item.mPops.add(pop);
      }
   }
   item.mTrainCount=trainCount;
   item.mTrainLimit=trainLimit;
   item.mTechPrereqID=techPrereqID;
   item.mInfoText=infoText;
   item.mInfoDetail=infoDetail;
   item.mpVisual = pVisual;
   item.mVisualScale = visualScale;
   long index=mItems.add(item);
   return index;
}

//==============================================================================
// BUI3DCircleMenu::removeItem
//==============================================================================
void BUI3DCircleMenu::removeItem(long index)
{
   mItems.erase(index);
}

//==============================================================================
// BUI3DCircleMenu::getItemIndex
//==============================================================================
long BUI3DCircleMenu::getItemIndex(long id) const
{
   uint count=mItems.getSize();
   for(uint i=0; i<count; i++)
   {
      if(mItems[i].mID==id)
         return i;
   }
   return -1;
}

//==============================================================================
// BUI3DCircleMenu::refresh
//==============================================================================
void BUI3DCircleMenu::refresh()
{
   mCurrentItem=-1;
   updatePointingAt(mPointingAtX, mPointingAtY, false);
}

//==============================================================================
// BUI3DCircleMenu::update
//==============================================================================
void BUI3DCircleMenu::update(float elapsedTime)
{
   uint count=mItems.getSize();
   for(uint i=0; i<count; i++)
   {
      if (mItems[i].mpVisual!=NULL)
         mItems[i].mpVisual->update(elapsedTime);
   }
}

//==============================================================================
// BUI3DCircleMenu::renderFlash
//==============================================================================
void BUI3DCircleMenu::renderFlash()
{
   mpRadial1->render();
   mpRadial2->render();
   mpRadial3->render();
}

//==============================================================================
// BUICircleMenu::render
//==============================================================================
void BUI3DCircleMenu::render()
{
   BVector pos;
   BMatrix matrix;
   matrix.makeIdentity();
   static bool r1 = true;
   static bool r2 = true;
   static bool r3 = true;

   //-- render the info panels   
   BDirLightParams dirLightParams = mDirLightParams;
      
   //dirLightParams.mDir = XMVectorSet(0,0.25f,0.75f,1);   
   dirLightParams.mColor = XMLoadColor((XMCOLOR*)&cColorRed);
   dirLightParams.mShadows = false;
   dirLightParams.mEnabled = true;
   
   DWORD tintColor = cDWORDWhite;
   if (r3)
      gUIManager->renderModel(mModels[2], matrix, mpRadial3->getRenderTargetTexture(), tintColor);
   if (r2)
      gUIManager->renderModel(mModels[1], matrix, mpRadial2->getRenderTargetTexture(), tintColor);
   if (r1)
      gUIManager->renderModel(mModels[0], matrix, mpRadial1->getRenderTargetTexture(), tintColor);
   
   BMatrix objMatrix;
   int positionIndex = -1;   
   for (int i = 0; i < mItems.getNumber(); ++i)
   {
      //-- tint the selected item
      if (i == mCurrentItem)
         tintColor = cDWORDOrange;
      else
         tintColor = cDWORDWhite;

      positionIndex = eBoneRadialPanelPosStart+mItems[i].mOrder;
      debugRangeCheck(positionIndex, eBoneTotal);

      mModels[0]->getBone(mBoneHandle[positionIndex], &pos, &objMatrix);

      matrix.makeScale(mItems[i].mVisualScale);
      matrix = XMMatrixMultiply(matrix, objMatrix);
      gUIManager->renderModel(mItems[i].mpVisual, matrix, cInvalidTextureHandle, tintColor);
   }
}

//==============================================================================
// BUICircleMenu::handleInput
//==============================================================================
bool  BUI3DCircleMenu::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);

   if(start || repeat || stop)
   {
      switch(controlType)
      {
      case cStickLeft:
      case cDpad:
         updatePointingAt(detail.mX, detail.mY, true);
         return true;
      }
   }

   return false;
}

//==============================================================================
// BUICircleMenu::updatePointingAt
//==============================================================================
void BUI3DCircleMenu::updatePointingAt(float x, float y, bool playSound)
{ 
   mPointingAtX=x;
   mPointingAtY=y;

   long item=calcCircleIndex(x, y, mCircleCount, 0.0f);

   long closestIndex=-1;

   if(item!=-1)
   {
      long closestDist=0;
      for(uint i=0; i<mItems.getSize(); i++)
      {
         long calcOrder=mItems[i].mOrder;
         long dist1=0;
         long dist2=0;
         if(item>calcOrder)
         {
            dist1=item-calcOrder;
            dist2=calcOrder+(mCircleCount-item);

         }
         else if(item<calcOrder)
         {
            dist1=calcOrder-item;
            dist2=(mCircleCount-calcOrder)+item;
         }
         long dist=min(dist1, dist2);
         if(closestIndex==-1 || dist<closestDist)
         {
            closestIndex=i;
            closestDist=dist;
         }
      }
   }

   if(closestIndex!=mCurrentItem)
   {
      mCurrentItem=closestIndex;
      //updateInfoText();
      if(playSound)
         gUI.playRolloverSound();
   }
}

//==============================================================================
// BUI3DCircleMenu::calcCircleIndex
//==============================================================================
long BUI3DCircleMenu::calcCircleIndex(float x, float y, long indexCount, float offset)
{ 
   long index=-1;

   if(indexCount!=0 && (x!=0.0f || y!=0.0f))
   {
      float r=0.0f;
      if(x!=0.0f)
      {
         if(y!=0.0f)
         {
            if(x>0.0f)
            {
               if(y<0.0f)
                  r=cTwoPi+(float)atan(y/x);
               else
                  r=(float)atan(y/x);
            }
            else
               r=(float)atan(y/x)+cPi;
         }
         else
         {
            if(x>0.0f)
               r=0.0f;
            else
               r=cPi;
         }
      }
      else
      {
         if(y>0.0f)
            r=cPiOver2;
         else
            r=cThreePiOver2;
      }

      r = r + (cPiOver2 + (cPi / indexCount)) + offset;
      while(r<0.0f)
         r+=cTwoPi;
      while(r>=cTwoPi)
         r-=cTwoPi;
      index = (long)(r * (indexCount / cTwoPi));   
   }

   return index;
}