//==============================================================================
// uilist.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "uilist.h"
#include "fontSystem2.h"
#include "render.h"
#include "ui.h"

// Constants
const float cFirstNextSelectionTime=0.5f;
const float cFastNextSelectionTime=0.1f;

//==============================================================================
// BUIList::BUIList
//==============================================================================
BUIList::BUIList() :
   BUIElement(),
   mCurrentItem(-1),
   mItems(),
   mItemIDs(),
   mControlling(false),
   mControlDirection(0),
   mControlTime(0.0f),
   mControlFast(false),
   mLastTime(0),
   mFontHandle(NULL),
   mColumnWidth(300.0f),
   mItemsPerColumn(10),
   mMultiColumn(false),
   mMaxColumns(1),
   mLeftColumn(0),
   mCurrentColumn(0),
   mJustifyCenter(false),
   mRowSpacing(30.0f),
   mTextColor(cColorWhite),
   mHighlightColor(cColorCyan)
{
}

//==============================================================================
// BUIList::~BUIList
//==============================================================================
BUIList::~BUIList()
{
}

//==============================================================================
// BUIList::update
//==============================================================================
void BUIList::update()
{
   controlUpdate();
}

//==============================================================================
// BUIList::setCurrentItem
//==============================================================================
void BUIList::setCurrentItem(long index)
{
   if(mItems.getNumber()==0)
   {
      mCurrentItem=-1;
      mCurrentColumn=0;
      mLeftColumn=0;
      return;
   }
   if(index<0)
   {
      mCurrentItem=0;
      mCurrentColumn=0;
      mLeftColumn=0;
      return;
   }
   if(index>=mItems.getNumber())
      mCurrentItem=mItems.getNumber()-1;
   else
      mCurrentItem=index;

   mCurrentColumn=mCurrentItem/mItemsPerColumn;
   if(mCurrentColumn<mLeftColumn)
      mLeftColumn=mCurrentColumn;
   else if(mCurrentColumn>mLeftColumn+mMaxColumns-1)
      mLeftColumn=mCurrentColumn-mMaxColumns+1;
}

//==============================================================================
// BUIList::setCurrentItemByID
//==============================================================================
void BUIList::setCurrentItemByID(long index)
{
   for (int i=0; i < mItemIDs.getNumber(); i++ )
   {
      if( mItemIDs[i] == index )
      {
         setCurrentItem(i);
         return;
      }
   }

   setCurrentItem(0);  //couldn't find it.  Revert to the top.
}

//==============================================================================
// BUIList::refresh
//==============================================================================
void BUIList::refresh()
{
   BUIElement::refresh();

   mWidth=(long)((mColumnWidth*mMaxColumns)+0.5f);
   mHeight=(long)((mRowSpacing*mItemsPerColumn)+0.5f);

   mLeftColumn=0;
   mCurrentColumn=0;

   if(mItems.getNumber()>0)
      setCurrentItem(0);
   else
      setCurrentItem(-1);
}

//==============================================================================
// BUIList::render
//==============================================================================
void BUIList::render(long parentX, long parentY)
{
   float x1=(float)(parentX+mX);
   float y1=(float)(parentY+mY);
   float x2=(float)(x1+mWidth-1);

   float y=y1;

   DWORD hilightColorDWORD = mHighlightColor.asDWORD();
   DWORD textColorDWORD = mTextColor.asDWORD();

   if(mMultiColumn)
   {
      long start=mLeftColumn*mItemsPerColumn;
      long end=min(mItems.getNumber(), start+mItemsPerColumn*mMaxColumns);
      for(long i=start; i<end; i++)
      {
         long col = (i - start) / mItemsPerColumn;
         if ((i % mItemsPerColumn) == 0)
            y = y1;

         gFontManager.drawText(mFontHandle, x1 + (col * mColumnWidth), y, mItems[i].getPtr(), (i==mCurrentItem?hilightColorDWORD:textColorDWORD));
         y+=mRowSpacing; 
      }
      if(mLeftColumn>0)
      {
         gFontManager.setFont(mFontHandle);
         float offset=gFontManager.getLineLength(L"<", 1)+2;
         gFontManager.drawText(mFontHandle, x1-offset, y1, L"<", textColorDWORD);
      }
      if(end<mItems.getNumber())
         gFontManager.drawText(mFontHandle, x2+2, y1, L">", textColorDWORD);
   }
   else if(mJustifyCenter)
   {
      for(long i=0; i<mItems.getNumber(); i++)
      {
         gFontManager.drawText(mFontHandle, x1 + (0.5f*mWidth), y, mItems[i].getPtr(), (i==mCurrentItem?hilightColorDWORD:textColorDWORD), BFontManager2::cJustifyCenter);
         y+=mRowSpacing; 
      }
   }
   else
   {
      for(long i=0; i<mItems.getNumber(); i++)
      {
         gFontManager.drawText(mFontHandle, x1 , y, mItems[i].getPtr(), (i==mCurrentItem?hilightColorDWORD:textColorDWORD));
         y+=mRowSpacing; 
      }
   }
}

//==============================================================================
// BUIList::handleInput
//==============================================================================
bool BUIList::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   port; detail;

   if(event==cInputEventControlStart)
   {
      switch(controlType)
      {
         case cDpadLeft:
         case cStickLeftLeft:
            if(mMultiColumn)
            {
               if(!mControlling)
                  controlStart(-2);
               return true;
            }
            break;

         case cDpadRight:
         case cStickLeftRight:
            if(mMultiColumn)
            {
               if(!mControlling)
                  controlStart(2);
               return true;
            }
            break;

         case cDpadDown:
         case cStickLeftDown:
            if(!mControlling)
               controlStart(1);
            return true;

         case cDpadUp:
         case cStickLeftUp:
            if(!mControlling)
               controlStart(-1);
            return true;
      }
   }
   else if(event==cInputEventControlStop)
   {
      switch(controlType)
      {
         case cDpadDown:
         case cDpadUp:
         case cStickLeftDown:
         case cStickLeftUp:
            if(mControlling)
               controlStop();
            return true;

         case cDpadLeft:
         case cDpadRight:
         case cStickLeftLeft:
         case cStickLeftRight:
            if(mMultiColumn)
            {
               if(mControlling)
                  controlStop();
               return true;
            }
            break;
      }
   }
   else if(event==cInputEventControlRepeat)
   {
      switch(controlType)
      {
         case cDpadDown:
         case cDpadUp:
         case cStickLeftDown:
         case cStickLeftUp:
            return true;

         case cDpadLeft:
         case cDpadRight:
         case cStickLeftLeft:
         case cStickLeftRight:
            if(mMultiColumn)
               return true;
            break;
      }
   }

   return false;
}


//==============================================================================
// BUIList::getCurrentItemID
//==============================================================================
long BUIList::getCurrentItemID()
{
   long id = -1;

   if ( (mCurrentItem>=0) && (mCurrentItem<mItemIDs.getNumber()) )
      id = mItemIDs[mCurrentItem];

   return id;
}


//==============================================================================
// BUIList::addItem
//==============================================================================
void BUIList::addItem(const char* item, long id)
{ 
   BSimString szItem(item);
   long index=mItems.add(szItem);
   if(index==-1)
      return;
   if(mItemIDs.add(id)==-1)
   {
      mItems.removeIndex(index);
      return;
   }
   if(mCurrentItem==-1) 
      mCurrentItem=0; 
}

//==============================================================================
// BUIList::setItem
//==============================================================================
void BUIList::setItem( const char* item, long id )
{
   long numItems = mItems.getNumber();
   for( long i = 0; i < numItems; i++ )
   {
      if( mItemIDs[i] == id )
      {
         mItems[i].format( "%s", item );
         return;
      }
   }
}

//==============================================================================
// BUIList::controlStart
//==============================================================================
void BUIList::controlStart(long direction)
{
   mControlling=true;
   mControlDirection=direction;
   mControlTime=0.0f;
   mControlFast=false;
   controlChangeSelection();
}

//==============================================================================
// BUIList::controlStop
//==============================================================================
void BUIList::controlStop()
{
   mControlling=false;
   mControlDirection=0;
}

//==============================================================================
// BUIList::controlUpdate
//==============================================================================
void BUIList::controlUpdate()
{
   DWORD time=timeGetTime();
   float elapsedTime=(time-mLastTime)*0.001f;
   if(elapsedTime>0.1f)
      elapsedTime=0.1f;
   mLastTime=time;

   if(mControlDirection==0)
      return;

   mControlTime+=elapsedTime;
   if(mControlFast)
   {
      if(mControlTime>=cFastNextSelectionTime)
      {
         controlChangeSelection();
         mControlTime=0.0f;
      }
   }
   else
   {
      if(mControlTime>=cFirstNextSelectionTime)
      {
         controlChangeSelection();
         mControlTime=0.0f;
         mControlFast=true;
      }
   }
}

//==============================================================================
// BUIList::controlChangeSelection
//==============================================================================
void BUIList::controlChangeSelection()
{
   long lastItem=mCurrentItem;

   if(mControlDirection==1)
   {
      if(mCurrentItem<mItems.getNumber()-1)
         setCurrentItem(mCurrentItem+1);
      else
         setCurrentItem(0);
   }
   else if(mControlDirection==-1)
   {
      if(mCurrentItem>0)
         setCurrentItem(mCurrentItem-1);
      else
         setCurrentItem(mItems.getNumber()-1);
   }
   else if (mControlDirection==2)
   {
      long target = mCurrentItem + mItemsPerColumn;
      if(target<mItems.getNumber())
         setCurrentItem(target);
   }
   else if (mControlDirection==-2)
   {
      long target = mCurrentItem - mItemsPerColumn;
      if(target>=0)
         setCurrentItem(target);
   }

   if(mCurrentItem!=lastItem)
      gUI.playRolloverSound();
}
