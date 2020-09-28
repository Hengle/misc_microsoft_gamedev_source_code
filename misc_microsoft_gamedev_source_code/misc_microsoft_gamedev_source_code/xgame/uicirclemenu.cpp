//==============================================================================
// uicirclemenu.cpp
//
// Copyright (c) 2004-2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "uicirclemenu.h"
#include "civ.h"
#include "config.h"
#include "database.h"
#include "FontSystem2.h"
#include "player.h"
#include "prototech.h"
#include "techtree.h"
#include "ui.h"
#include "uigame.h"
#include "usermanager.h"
#include "user.h"
#include "world.h"
#include "configsgame.h"
#include "game.h"

//==============================================================================
// BUICircleMenu::BUICircleMenu
//==============================================================================
BUICircleMenu::BUICircleMenu() : 
   BUIElement(),
   mCircleWidth(256),
   mCircleCount(8),
   mItemRadius(120.0f),
   mItemWidth(64),
   mUnlitColor(cDWORDCyan),
   mLitColor(cDWORDWhite),
   mUnavailableColor(cDWORDRed),
   mUnavailableLitColor(cDWORDOrange),
   mItems(),
   mCurrentItem(-1),
   mBaseText(),
   mBaseTextDetail(),
   mPointingAtX(0.0f),
   mPointingAtY(0.0f),
   mPlayerID(-1)
{
}

//==============================================================================
// BUICircleMenu::~BUICircleMenu
//==============================================================================
BUICircleMenu::~BUICircleMenu()
{
}

//==============================================================================
// BUICircleMenu::autoPosition
//==============================================================================
void BUICircleMenu::autoPosition(BUser* pUser)
{
   long x, y;
   gUIGame.getViewCenter(pUser, x, y);
   if (gGame.isSplitScreen() && !gGame.isVerticalSplit() && pUser == gUserManager.getPrimaryUser())
      y+=15;
   setPosition(x-(mCircleWidth/2), y-(mCircleWidth/2));
}

//==============================================================================
// BUICircleMenu::cleanup
//==============================================================================
void BUICircleMenu::cleanup()
{
   clearItems();
   for(long i=0; i<cMaxCostItems; i++)
      mCostIcons[i].cleanup();
   for(long i=0; i<cMaxCostItems; i++)
      mCostAmounts[i].cleanup();
   for(long i=0; i<cMaxInfoLines; i++)
      mInfoLines[i].cleanup();
}

//==============================================================================
// BUICircleMenu::clearItems
//==============================================================================
void BUICircleMenu::clearItems()
{
   for(uint i=0; i<mItems.getSize(); i++)
      mItems[i].mButton.cleanup();
   mItems.clear();
   mCurrentItem=-1;
}

//==============================================================================
// BUICircleMenu::addItem
//==============================================================================
long BUICircleMenu::addItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, bool unavailable, const BUString& label, const BUString& infoText, const BUString& infoDetail, const char* pTextureName)
{
   BUICircleMenuItem item;
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
   item.mButton.setText(label);
   item.mButton.setFont(mFontHandle);
   item.mButton.setColor(cDWORDWhite);
   item.mButton.setTexture(pTextureName, true);
   item.mInfoText=infoText;
   item.mInfoDetail=infoDetail;
   item.mUnavailable=unavailable;
   long index=mItems.add(item);
   return index;
}

//==============================================================================
// BUICircleMenu::addItem
//==============================================================================
long BUICircleMenu::addItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, long techPrereqID, bool unavailable, const BUString& label, const BUString& infoText, const BUString& infoDetail, BManagedTextureHandle textureHandle)
{
   BUICircleMenuItem item;
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
   item.mButton.setText(label);
   item.mButton.setFont(mFontHandle);
   item.mButton.setColor(cDWORDWhite);
   item.mButton.setTexture(textureHandle, true);
   item.mInfoText=infoText;
   item.mInfoDetail=infoDetail;
   item.mUnavailable=unavailable;
   long index=mItems.add(item);
   return index;
}

//==============================================================================
// BUICircleMenu::setItemLabel
//==============================================================================
void BUICircleMenu::setItemLabel(long index, const BUString& label)
{
   BUICircleMenuItem& item=mItems[index];
   item.mButton.setText(label);
}

//==============================================================================
// BUICircleMenu::removeItem
//==============================================================================
void BUICircleMenu::removeItem(long index)
{
   mItems.erase(index);
}

//==============================================================================
// BUICircleMenu::getItemIndex
//==============================================================================
long BUICircleMenu::getItemIndex(long id) const
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
// BUICircleMenu::getItemIndexByOrder
//==============================================================================
long BUICircleMenu::getItemIndexByOrder(long order) const
{
   uint count=mItems.getSize();
   for(uint i=0; i<count; i++)
   {
      if(mItems[i].mOrder==order)
         return i;
   }
   return -1;
}

//==============================================================================
// BUICircleMenu::refresh
//==============================================================================
void BUICircleMenu::refresh()
{
   mWidth  = mCircleWidth;
   mHeight = mCircleWidth;

   long menuCenterX = (mWidth / 2);
   long menuCenterY = (mHeight / 2);

   BUIElement::refresh();

   mCurrentItem=-1;

   // Info text lines
   for(long i=0; i<cMaxInfoLines; i++)
   {
      BUIElement& line=mInfoLines[i];
      line.setColor(cDWORDWhite);
      line.setFont(mFontHandle);
      line.setHidden(true);
   }

   // Cost items
   for(long i=0; i<cMaxCostItems; i++)
   {
      BUIElement& amount=mCostAmounts[i];
      amount.setColor(cDWORDWhite);
      amount.setFont(mFontHandle);
      amount.setHidden(true);
      mCostIcons[i].setHidden(true);
      mCostType[i]=-1;
      mCostID[i]=-1;
      mCostValue[i]=0.0f;
   }

   // Item buttons
   for(uint i=0; i<mItems.getSize(); i++)
   {
      BUICircleMenuItem& item=mItems[i];
      positionButton(&item.mButton, menuCenterX, menuCenterY, mItemWidth, mItemWidth, (float)item.mOrder, mCircleCount, mItemRadius, false, 0.0f);
      item.mButton.refresh();
   }

   updateInfoText();

   updatePointingAt(mPointingAtX, mPointingAtY, false, false);
}

//==============================================================================
// BUICircleMenu::positionButton
//==============================================================================
float BUICircleMenu::positionButton(BUIElement* pButton, long x, long y, long xs, long ys, float pos, long count, float radius, bool useStartVal, float startVal)
{
   float val=0.0f;

   if(radius>0.0f && count>0)
   {
      float inc = cTwoPi / count;

      if(useStartVal)
         val = startVal - (inc * pos);
      else
         val = cPiOver2 - (inc * pos);

      if(val > cTwoPi)
         val -= cTwoPi;
      else if(val < 0.0f)
         val += cTwoPi;

      x += (long)(cos(val) * radius);
      y -= (long)(sin(val) * radius);
   }

   pButton->setPosition((x - (xs / 2)), (y - (ys / 2)));
   pButton->setSize(xs, ys);

   return val;
}

//==============================================================================
// BUICircleMenu::handleInput
//==============================================================================
bool  BUICircleMenu::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);

   if(start || repeat || stop)
   {
      switch(controlType)
      {
         case cStickLeft:
            {
               bool playSound = true;

               //-- check to not play the clicking sound twice if we have the new UI
               //-- turned on
               if (gConfig.isDefined(cConfigFlashGameUI))
                  playSound = false;
               updatePointingAt(detail.mX, detail.mY, playSound, true);
            }            
            return true;
      }
   }

   return BUIElement::handleInput(port, event, controlType, detail);
}

//==============================================================================
// BUICircleMenu::updatePointingAt
//==============================================================================
void BUICircleMenu::updatePointingAt(float x, float y, bool playSound, bool doRumble)
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
         if(dist < 2 && (closestIndex==-1 || dist<closestDist))
         {
            closestIndex=i;
            closestDist=dist;
         }
      }
   }

   if(closestIndex!=mCurrentItem)
   {
      mCurrentItem=closestIndex;
      updateInfoText();
      if(playSound)
         gUI.playRolloverSound();
      if(doRumble)
         gUI.playRumbleEvent(BRumbleEvent::cTypeRadialMenuItem);
   }
}

//==============================================================================
// BUICircleMenu::calcCircleIndex
//==============================================================================
long BUICircleMenu::calcCircleIndex(float x, float y, long indexCount, float offset)
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

//==============================================================================
// BUICircleMenu::update
//==============================================================================
void BUICircleMenu::update()
{
   for(long i=0; i<cMaxInfoLines; i++)
      mInfoLines[i].update();

//-- FIXING PREFIX BUG ID 2312
   const BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
//--
   for(long i=0; i<cMaxCostItems; i++)
   {
      bool afford=true;
      if(pPlayer && mCostType[i]!=-1)
      {
         if(mCostType[i]==BUIGamePlayerStat::cTypeResource)
         {
            if(pPlayer->getResource(mCostID[i])<mCostValue[i])
               afford=false;
         }
         if(mCostType[i]==BUIGamePlayerStat::cTypePop)
         {
            if(!pPlayer->checkPop(mCostID[i], mCostValue[i]))
               afford=false;
         }
      }
      mCostAmounts[i].setColor((afford ? cDWORDWhite : cDWORDRed));
      mCostAmounts[i].update();
   }

   for(long i=0; i<cMaxCostItems; i++)
      mCostIcons[i].update();

   for(uint i=0; i<mItems.getSize(); i++)
      mItems[i].mButton.update();
}

//==============================================================================
// BUICircleMenu::render
//==============================================================================
void BUICircleMenu::render(long parentX, long parentY)
{
   BUIElement::render(parentX, parentY);
   long x=parentX+mX;
   long y=parentY+mY;
   for(long i=0; i<cMaxInfoLines; i++)
      mInfoLines[i].render(x, y);
   for(long i=0; i<cMaxCostItems; i++)
      mCostAmounts[i].render(x, y);
   for(long i=0; i<cMaxCostItems; i++)
      mCostIcons[i].render(x, y);
   for(uint i=0; i<mItems.getSize(); i++)
   {
      if(i==(uint)mCurrentItem)
      {        
         if(mItems[i].mUnavailable)
            mItems[i].mButton.setColor(mUnavailableLitColor);
         else
            mItems[i].mButton.setColor(mLitColor);
         mItems[i].mButton.setFlag(BUIElement::cFlagOverbright, true);
      }
      else
      {
         if(mItems[i].mUnavailable)
            mItems[i].mButton.setColor(mUnavailableColor);
         else
            mItems[i].mButton.setColor(mUnlitColor);
         mItems[i].mButton.setFlag(BUIElement::cFlagOverbright, false);
      }
      mItems[i].mButton.render(x, y);
   }
}

//==============================================================================
// BUICircleMenu::updateInfoText
//==============================================================================
void BUICircleMenu::updateInfoText()
{
   for(long i=0; i<cMaxInfoLines; i++)
   {
      mInfoLines[i].setHidden(true);
      mInfoLines[i].setColor(cDWORDWhite);
   }
   for(long i=0; i<cMaxCostItems; i++)
   {
      mCostAmounts[i].setHidden(true);
      mCostIcons[i].setHidden(true);
      mCostID[i]=-1;
      mCostType[i]=-1;
      mCostValue[i]=0.0f;
   }

   BCost cost;
   BPopArray* pPops=NULL;
   long costLine=-1;

   long count=0;

   if(mCurrentItem==-1)
   {
      mInfoLines[count].setText(mBaseText);
      count++;

      if(!mBaseTextDetail.isEmpty())
      {
         BSmallDynamicSimArray<BUString> lines;
         uint lineCount=gUI.wordWrapText(mBaseTextDetail, 6, lines);
         for(uint i=0; i<lineCount; i++)
         {
            mInfoLines[count].setText(lines[i]);
            count++;
         }
      }
   }
   else
   {
      BUICircleMenuItem& item=mItems[mCurrentItem];

      if(!item.mUnavailable)
      {
         BUString s;

         if(item.mTechPrereqID!=-1)
         {
//-- FIXING PREFIX BUG ID 2314
            const BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
//--
            if(pPlayer)
            {
               if(pPlayer->getTechTree()->getTechStatus(item.mTechPrereqID, -1)!=BTechTree::cStatusActive)
               {
//-- FIXING PREFIX BUG ID 2313
                  const BProtoTech* pProtoTech=gDatabase.getProtoTech(item.mTechPrereqID);
//--
                  if(pProtoTech)                         
                  {
                     s.format(L"%S", pProtoTech->getName().getPtr());
                     mInfoLines[count].setText(s);
                     mInfoLines[count].setColor(cDWORDRed);
                     count++;
                  }
               }
            }
         }

         if(s.isEmpty() && item.mTrainLimit!=-1)
         {
            s.format(L"%d / %d", item.mTrainCount, item.mTrainLimit);
            mInfoLines[count].setText(s);
            if(item.mTrainCount>=item.mTrainLimit)
               mInfoLines[count].setColor(cDWORDRed);
            count++;
         }

         if(item.mCost.getTotal()!=0.0f || item.mPops.getNumber()>0)
         {
            mInfoLines[count].setText(L"");
            cost=item.mCost;
            pPops=&(item.mPops);
            costLine=count;
            count++;
         }
      }

      if(!item.mInfoText.isEmpty())
      {
         mInfoLines[count].setText(item.mInfoText);
         count++;
      }

      if(!item.mInfoDetail.isEmpty())
      {
         BSmallDynamicSimArray<BUString> lines;
         uint lineCount=gUI.wordWrapText(item.mInfoDetail, 6, lines);
         for(uint i=0; i<lineCount; i++)
         {
            mInfoLines[count].setText(lines[i]);
            count++;
         }
      }
   }

   if(count>0)
   {
      gFontManager.setFont(mFontHandle);
      long height=(long)gFontManager.getLineHeight();
      long width=(long)(mWidth*0.7f);
      long x=(mWidth/2)-(width/2);
      long y=(mHeight/2)-(height*count/2);

      for(long i=0; i<count; i++)
      {
         mInfoLines[i].setSize(width, height);
         mInfoLines[i].setPosition(x, y);
         y+=height;
         mInfoLines[i].setHidden(false);
         mInfoLines[i].refresh();
      }
   }

   if(costLine!=-1)
   {
      const long cResourceSpacing=4;
      gFontManager.setFont(mCostAmounts[0].getFont());
      long lineHeight=(long)(gFontManager.getLineHeight());
      long iconSize=lineHeight;
      BUString s;
      long wid=0;

      const long cMaxStats=8;
      bool resourceProcessed[cMaxStats];
      bool popProcessed[cMaxStats];
      long statIDs[cMaxStats];
      long statTypes[cMaxStats];
      float statAmounts[cMaxStats];
      for(long i=0; i<cMaxStats; i++)
      {
         resourceProcessed[i]=false;
         statIDs[i]=-1;
         statTypes[i]=-1;
         statAmounts[i]=0.0f;
      }

      // Add all the stats (cost and pop) in the same order they are displayed on the main UI
      long statCount=0;
      BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
      if(!pPlayer)
         return;
      long civID=pPlayer->getCivID();
      long playerStatCount=gUIGame.getNumberPlayerStats(civID);
      for(long i=0; i<playerStatCount; i++)
      {
         const BUIGamePlayerStat*  pStat=gUIGame.getPlayerStat(civID, i);
         if(pStat->mID<cMaxStats && (pStat->mType==BUIGamePlayerStat::cTypeResource || pStat->mType==BUIGamePlayerStat::cTypePop))
         {
            if(pStat->mType==BUIGamePlayerStat::cTypeResource)
            {
               resourceProcessed[pStat->mID]=true;
               statAmounts[statCount]=cost.get(pStat->mID);
            }
            else
            {
               popProcessed[pStat->mID]=true;
               if(pPops)
               {
                  for(long j=0; j<pPops->getNumber(); j++)
                  {
                     BPop pop=pPops->get(j);
                     if(pop.mID==pStat->mID)
                     {
                        statAmounts[statCount]=(float)pop.mCount;
                        break;
                     }
                  }
               }
            }
            statIDs[statCount]=pStat->mID;
            statTypes[statCount]=pStat->mType;
            statCount++;
            if(statCount==cMaxStats)
               break;
         }
      }

      /*
      // Add in other stats that aren't displayed on the main UI
      if(statCount<cMaxStats)
      {
         for(long i=0; i<cost.getNumberResources(); i++)
         {
            if(resourceProcessed[i])
               continue;
            statIDs[statCount]=i;
            statTypes[statCount]=BUIGamePlayerStat::cTypeResource;
            statAmounts[statCount]=cost.get(i);
            statCount++;
            if(statCount==cMaxStats)
               break;
         }

         for(long i=0; i<gDatabase.getNumberPops(); i++)
         {
            if(popProcessed[i])
               continue;
            statIDs[statCount]=i;
            statTypes[statCount]=BUIGamePlayerStat::cTypePop;
            if(pPops)
            {
               for(long j=0; j<pPops->getNumber(); j++)
               {
                  BPop pop=pPops->get(j);
                  if(pop.mID==i)
                  {
                     statAmounts[statCount]=(float)pop.mCount;
                     break;
                  }
               }
            }
            statCount++;
            if(statCount==cMaxStats)
               break;
         }
      }
      */

      float costAmounts[cMaxCostItems];
      long costIndex=0;
      for(long i=0; i<statCount && costIndex<cMaxCostItems; i++)
      {
         float v=statAmounts[i];
         if(v!=0.0f)
         {
            wid+=iconSize;
            s.format(L"%.0f", v);
            wid+=((long)(gFontManager.getLineLength(s, s.length())+0.5f))+cResourceSpacing;
            costAmounts[costIndex]=v;
            costIndex++;
         }
      }

      long x=mInfoLines[costLine].getX()+(mInfoLines[costLine].getWidth()/2)-(wid/2);
      long y=mInfoLines[costLine].getY();
      costIndex=0;
      for(long i=0; i<statCount && costIndex<cMaxCostItems; i++)
      {
         float v=statAmounts[i];
         if(v!=0.0f)
         {
            BUIElement& icon=mCostIcons[costIndex];
            icon.setHidden(false);
            if(statTypes[i]==BUIGamePlayerStat::cTypeResource)
               icon.setTexture(gUIGame.getResourceTextIcon(statIDs[i]), true);
            else if(statTypes[i]==BUIGamePlayerStat::cTypePop)
               icon.setTexture(gUIGame.getPopTextIcon(statIDs[i]), true);
            icon.setPosition(x, y);
            icon.setSize(iconSize, iconSize);
            x+=iconSize;

            BUIElement& amount=mCostAmounts[costIndex];
            amount.setHidden(false);
            s.format(L"%.0f", v);
            amount.setText(s);
            long amountWid=((long)(gFontManager.getLineLength(s, s.length())+0.5f))+cResourceSpacing;
            amount.setPosition(x, y);
            amount.setSize(amountWid, lineHeight);
            x+=amountWid;

            mCostType[costIndex]=statTypes[i];
            mCostID[costIndex]=statIDs[i];
            mCostValue[costIndex]=v;

            costIndex++;
         }
      }
   }
}
