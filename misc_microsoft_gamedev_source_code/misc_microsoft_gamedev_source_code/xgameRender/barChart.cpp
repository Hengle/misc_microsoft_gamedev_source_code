//============================================================================
// barChart.cpp
// Ensemble Studios (C) 2008
//============================================================================

// xgameRender
#include "xgameRender.h"
#include "barChart.h"
#include "primDraw2D.h"

// xrender
#include "renderDraw.h"

const uint cStayVisibleTime = 30;
//============================================================================
// BBarChart::BBarChart
//============================================================================
BBarChart::BBarChart(void):
   mBarHandleCounter(1),
   mFontDirID(0),
   mVisibleCounter(0)
{

}

//============================================================================
// BBarChart::BBarChart
//============================================================================
BBarChart::~BBarChart(void)
{

}

//============================================================================
// BBarChart::init
//============================================================================
void BBarChart::init(uint fontDirID, BInitParams parms)
{
   ASSERT_THREAD(cThreadIndexSim);

   commandListenerInit();

   mFontDirID = fontDirID;
   CopyMemory(&mParams,&parms,sizeof(BInitParams));
   
   mVisibleCounter = 0;
}

//============================================================================
// BBarChart::deinit
//============================================================================
void BBarChart::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);

   commandListenerDeinit();
}

//============================================================================
// BBarChart::clearBars
//============================================================================
void BBarChart::clearBars(void)
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      mBarValues.clear();
   }
   else
   {
      gRenderThread.submitCommand(mCommandHandle, cBGClearBars);
   }
}

//============================================================================
// BBarChart::render
//============================================================================
void BBarChart::render(void)
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID != cThreadIndexRender)
   {
      gRenderThread.submitCommand(mCommandHandle, cBGRender, mParams.mRenderOption);
      return;
   }

   ASSERT_THREAD(cThreadIndexRender);

   render(mParams.mRenderOption);
}

//============================================================================
// BBarChart::render
//============================================================================
void BBarChart::render(eRenderOptions renderModeOverride)
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID != cThreadIndexRender)
   {
      gRenderThread.submitCommand(mCommandHandle, cBGRender, renderModeOverride);
      return;
   }

   ASSERT_THREAD(cThreadIndexRender);

   if ((!mVisibleCounter) && ((renderModeOverride == eRenderOnlyIfViolators) || (renderModeOverride == eRenderOnlyIfFirstViolator)))
   {
      bool violators = false;
      const uint numBars = mBarValues.getSize();
      for(uint i = 0 ; i < numBars; i++)
      {
         if(mBarValues[i].mValue > mParams.mMaxValue)
            violators = true;

         // [11/5/2008 xemu] only look at the first one if set to that specific mode 
         if (renderModeOverride == eRenderOnlyIfFirstViolator)
            break;
      }
      if(!violators)
         return;
      mVisibleCounter = cStayVisibleTime;
   }
   
   if (mVisibleCounter)
      mVisibleCounter--;

   renderBackground(); 
   renderGridlines();
   renderBars();
}

//============================================================================
// BBarChart::renderBackground
//============================================================================
void BBarChart::renderBackground()
{
   ASSERT_THREAD(cThreadIndexRender);

   const uint x      = mParams.mLocX;
   const uint y      = mParams.mLocY;
   const uint width  = mParams.mWidth;
   const uint height = mParams.mHeight;
   const DWORD color  = mParams.mBGColor;

   gRenderDraw.setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   gRenderDraw.setRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   gRenderDraw.setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
   gRenderDraw.setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
   gRenderDraw.setRenderState(D3DRS_ALPHATESTENABLE, FALSE);

   BPrimDraw2D::drawSolidRect2D(x, y, x+width, y+height, 0, 0, 0, 0, color, 0xFFFFFFFF,  cPosDiffuseVS, cDiffusePS);
}

//============================================================================
// BBarChart::renderGridlines
//============================================================================
void BBarChart::renderGridlines()
{
   ASSERT_THREAD(cThreadIndexRender);

   const uint cNumGridLines = 3;
   const uint cDistBetweenLines = mParams.mWidth / cNumGridLines;
   const uint cPrintInc = (uint)(mParams.mMaxValue / cNumGridLines);

   BUString str;
   
   const uint y0 = mParams.mLocY;
   const uint y1 = mParams.mLocY + mParams.mHeight;

   uint x0 = mParams.mLocX;
   for(uint i = 0 ; i < cNumGridLines + 1; i++, x0+=cDistBetweenLines)
   {
      BPrimDraw2D::drawLine2D(x0,y0,x0,y1,mParams.mGridLineColor,mParams.mGridLineColor);

      const uint barVal = cPrintInc * i;
      str.format(L"%i", barVal);
      mRenderFont.DrawText((float)x0,(float)(y1 + 10),mParams.mFontColor,str.getPtr());
   }
}

//============================================================================
// BBarChart::renderBars
//============================================================================
void BBarChart::renderBars()
{
   ASSERT_THREAD(cThreadIndexRender);

   const uint numBars = mBarValues.getSize();
   const uint cBarSpacing = (uint)(mParams.mHeight * 0.1f);
   const uint cBarHeight = (mParams.mHeight / numBars) - cBarSpacing;
   const uint cBarHeightHalf = cBarHeight >>1;
   const uint cAvgTabWidth = 2;
   const DWORD cAvgTabColor = 0xFFFFFF00;

   const uint x      = mParams.mLocX;
   uint y      = mParams.mLocY;
  
   BUString str;
   for(uint i = 0 ; i < numBars; i++)
   {
      const float value = mBarValues[i].mValue;
      
      const float ratio = value / mParams.mMaxValue;
      uint width  = (uint)(mParams.mWidth * ratio);
      
      const float avgValue =Math::Max<float>((float)(mBarValues[i].mAvgValue.getAverage()),value);
      const float avgratio = avgValue / mParams.mMaxValue;
      uint avgwidth  = (uint)(mParams.mWidth * avgratio);

      DWORD color  = mBarValues[i].mColor;

      gRenderDraw.setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      gRenderDraw.setRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      gRenderDraw.setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      gRenderDraw.setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
      gRenderDraw.setRenderState(D3DRS_ALPHATESTENABLE, FALSE);

      BPrimDraw2D::drawLine2D(x, y + cBarHeightHalf, x + avgwidth, y + cBarHeightHalf, 0xFFFFFFFF);

      BPrimDraw2D::drawSolidRect2D(x + avgwidth, y, x + avgwidth + cAvgTabWidth, y+cBarHeight, 0, 0, 0, 0, cAvgTabColor, 0xFFFFFFFF,  cPosDiffuseVS, cDiffusePS);
      BPrimDraw2D::drawSolidRect2D(x, y, x+width, y+cBarHeight, 0, 0, 0, 0, color, 0xFFFFFFFF,  cPosDiffuseVS, cDiffusePS);

      str.format(L"%s - %02.1f", mBarValues[i].mText.getPtr(), value);
      mRenderFont.DrawText((float)x,(float)y,mParams.mFontColor,str.getPtr());

      y += cBarHeight + cBarSpacing;
   }

}

//============================================================================
// BBarChart::addBar
//============================================================================
BBarChartBarHandle BBarChart::addBar(BUString text,DWORD color)
{
   const uint handle = mBarHandleCounter++;

   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      BBarData dat;
      dat.mColor = color;
      dat.mValue = 0;
      dat.mHandle = handle;
      dat.mText.copy(text);
      addBarInternal(&dat);
   }
   else
   {
      BBarData* lp = new (gRenderHeap) BBarData;
            
      lp->mColor = color;
      lp->mValue = 0;
      lp->mHandle = handle;
      lp->mText.copy(text);
      
      gRenderThread.submitCommand(mCommandHandle, cBGAddBar, sizeof(BBarData**), &lp);
   }

   return handle;
}

//============================================================================
// BBarChart::addBarInternal
//============================================================================
void BBarChart::addBarInternal(const BBarData* dat)
{
   ASSERT_THREAD(cThreadIndexRender);

   BBarData localDat;
   localDat.mColor = (*dat).mColor;
   localDat.mHandle = (*dat).mHandle;
   localDat.mValue = (*dat).mValue;
   localDat.mText.copy((*dat).mText);
   localDat.mAvgValue.set(cStayVisibleTime);
   mBarValues.add(localDat);
}

//============================================================================
// BBarChart::setBarValue
//============================================================================
void BBarChart::setBarValue(uint handle,float value)
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      for(uint i =0 ; i < mBarValues.getSize(); i++)
      {
         if(mBarValues[i].mHandle == handle)
         {
            mBarValues[i].mValue = value;
            mBarValues[i].mAvgValue.addSample(value);
            return;
         }
      }
   }
   else
   {
      BBarData* lp = new(gRenderHeap) BBarData;
            
      lp->mValue = value;
      lp->mHandle = handle;
      
      gRenderThread.submitCommand(mCommandHandle, cBGSetBarValue, sizeof(BBarData**), &lp);
   }
}

//============================================================================
// BBarChart::setBarMaxValue
//============================================================================
void BBarChart::setBarMaxValue(float maxValue)
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      mParams.mMaxValue = maxValue;
   }
   else
   {
      BBarData* lp = new(gRenderHeap) BBarData;

      lp->mValue = maxValue;
      lp->mHandle = 0;

      gRenderThread.submitCommand(mCommandHandle, cBGSetBarMaxValue, sizeof(BBarData**), &lp);
   }
}

//============================================================================
// BBarChart::addBarValue
//============================================================================
void BBarChart::addBarValue(uint handle,float valueToAdd)
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      for(uint i =0 ; i < mBarValues.getSize(); i++)
      {
         if(mBarValues[i].mHandle == handle)
         {
            mBarValues[i].mValue += valueToAdd;
            return;
         }
      }
   }
   else
   {
      BBarData* lp = new (gRenderHeap) BBarData;
      
      lp->mValue = valueToAdd;
      lp->mHandle = handle;

      gRenderThread.submitCommand(mCommandHandle, cBGAddBarValue, sizeof(BBarData**), &lp);
   }
}

//============================================================================
// BBarChart::isAnyValueOverMax
//============================================================================
bool BBarChart::isAnyValueOverMax(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   for(uint i =0 ; i < mBarValues.getSize(); i++)
   {
      if(mBarValues[i].mValue >= mParams.mMaxValue)
      {
         return true;
      }
   }
   return false;
}

//============================================================================
// BBarChart::initDeviceData
//============================================================================
void BBarChart::initDeviceData(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   mRenderFont.Create(mFontDirID, "courier_10.xpr");
}

//============================================================================
// BBarChart::deinitDeviceData
//============================================================================
void BBarChart::deinitDeviceData(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   mRenderFont.Destroy();
   mBarValues.clear();
}

//============================================================================
// BBarChart::processCommand
//============================================================================
void BBarChart::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
   case cBGDestroy:
   case cBGClearBars:
      {
         clearBars();
         break;
      }
   case cBGAddBar:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BBarData*));
         BBarData* p = *(BBarData**)(pData);
         addBarInternal(p);
         HEAP_DELETE(p, gRenderHeap);
         break;
      }
   case cBGSetBarValue:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BBarData*));
         BBarData* p = *(BBarData**)(pData);
         setBarValue(p->mHandle, p->mValue);
         HEAP_DELETE(p, gRenderHeap);
         break;
      }
   case cBGAddBarValue:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BBarData*));
         BBarData* p = *(BBarData**)(pData);
         addBarValue(p->mHandle, p->mValue);
         HEAP_DELETE(p, gRenderHeap);
         break;
      }
   case cBGRender:
      {
         render(*(reinterpret_cast<const eRenderOptions*>(pData)));
         break;
      }

   case cBGSetBarMaxValue:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BBarData*));
         BBarData* p = *(BBarData**)(pData);
         setBarMaxValue(p->mValue);
         HEAP_DELETE(p, gRenderHeap);
         break;
      }

   }
}

