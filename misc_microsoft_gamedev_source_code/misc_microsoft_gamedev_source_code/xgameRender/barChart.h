//============================================================================
// barChart.h
// Ensemble Studios (C) 2008
//============================================================================

#pragma once

// xcore
#include "containers\dynamicArray.h"
#include "threading\eventDispatcher.h"
#include "math\runningAverage.h"

// xrender
#include "renderThread.h"

//xgamerender
#include "AtgFont.h"

typedef uint BBarChartBarHandle;
const BBarChartBarHandle cInvalidBarChartBarHandle = UINT_MAX;

//============================================================================
// BBarChart
//============================================================================
class BBarChart : public BRenderCommandListener
{
public:

   BBarChart(void);
   ~BBarChart(void);
   enum eRenderOptions
   {
      eRenderAlways =0,
      eRenderOnlyIfViolators,
      eRenderOnlyIfFirstViolator,

      cForceDWORD = 0xFFFFFFFF
   };

   class BInitParams
   {
   public:
      BInitParams():mLocX(85),
         mLocY(200),
         mWidth(320),
         mHeight(240),
         mMaxValue(100),
         mBGColor(0xCC888888),
         mGridLineColor(0xFF000000),
         mFontColor(0xFFFFFFFF),
         mRenderOption(eRenderAlways)
      {
        
      };

      uint     mWidth;
      uint     mHeight;
      uint     mLocX;
      uint     mLocY;
      float    mMaxValue;
      DWORD    mBGColor;
      DWORD    mGridLineColor;
      DWORD    mFontColor;
      eRenderOptions mRenderOption;
   };

   void init(uint fontDirID, BInitParams parms);
   void deinit(void);

   void clearBars(void);

   void render(void);
   void render(eRenderOptions renderModeOverride);

   BBarChartBarHandle addBar(BUString text, DWORD color);
   void setBarValue(uint handle,float value);
   void addBarValue(uint handle,float valueToAdd);
   void setBarMaxValue(float maxVal);

   //only call from the render thread:
   bool isAnyValueOverMax(void);
   
protected:
   class BBarData
   {
   public:
      BBarData() : mValue(0), mHandle(0), mColor(0) { }
      
      float             mValue;
      uint              mHandle;
      DWORD             mColor;
      BUString          mText;
      BRunningAverage<float>   mAvgValue;
   };
   BDynamicArray<BBarData> mBarValues;
   uint mBarHandleCounter;

   void addBarInternal(const BBarData* dat);
   void renderBackground(void);
   void renderGridlines(void);
   void renderBars(void);

   //Visual Attribs
   BInitParams    mParams;
   

   ATG::Font      mRenderFont;
   uint           mFontDirID;
   uint           mVisibleCounter;

   enum eCommandEnums
   {
      cBGDestroy=0,
      cBGClearBars,
      cBGAddBar,
      cBGSetBarValue,
      cBGAddBarValue,
      cBGRender,
      cBGSetBarMaxValue,
   };
   void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   void initDeviceData(void);
   void deinitDeviceData(void);

};