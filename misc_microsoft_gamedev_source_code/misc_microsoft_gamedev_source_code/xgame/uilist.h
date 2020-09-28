//==============================================================================
// uilist.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "uielement.h"

//==============================================================================
// BUIList
//==============================================================================
class BUIList : public BUIElement
{
   public:
                                    BUIList();
                                    ~BUIList();

      virtual void                  refresh();
      virtual void                  update();
      virtual void                  render(long parentX, long parentY);
      virtual bool                  handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      void                          setFont(BHandle hFont) { mFontHandle=hFont; }
      void                          setRowSpacing(float spacing) { mRowSpacing=spacing; }
      void                          setColumnWidth(float width) { mColumnWidth=width; }
      void                          setNumberItemsPerColumn(long num) { mItemsPerColumn=num; }
      void                          setMaxColumns(long num) { mMaxColumns=num; }
      void                          setMultiColumn(bool enable) { mMultiColumn=enable; }
      void                          setJustifyCenter(bool justify) { mJustifyCenter=justify; }
      void                          setColors(BColor textColor, BColor highlightColor)  { mTextColor=textColor; mHighlightColor=highlightColor; }

      void                          clearItems() { mItems.clear(); mItemIDs.clear(); mCurrentItem=-1; stop(); }
      void                          addItem(const char* item, long id=-1);
      long                          getCurrentItem() const { return mCurrentItem; }
      long                          getCurrentItemID();

      long                          getNumberItems() const { return mItems.getNumber(); }
      const BSimString&             getItem(long index) const { return mItems[index]; }
      long                          getItemID(long index) const { return mItemIDs[index]; }
      void                          setCurrentItem(long index);
      void                          setCurrentItemByID(long index);
      
      void                          setItem( const char* item, long id );

      void                          stop() { if(mControlling) controlStop(); }

   protected:
      void                          controlStart(long direction);
      void                          controlUpdate();
      void                          controlStop();
      void                          controlChangeSelection();

      long                          mCurrentItem;
      BDynamicSimArray<BSimString>  mItems;
      BDynamicSimArray<long>        mItemIDs;
      bool                          mControlling;
      long                          mControlDirection;
      float                         mControlTime;
      bool                          mControlFast;
      DWORD                         mLastTime;
      BHandle                       mFontHandle;
      float                         mRowSpacing;
      BColor                        mTextColor;
      BColor                        mHighlightColor;
      float                         mColumnWidth;
      long                          mItemsPerColumn;
      long                          mMaxColumns;
      long                          mLeftColumn;
      long                          mCurrentColumn;
      bool                          mMultiColumn;
      bool                          mJustifyCenter;

};