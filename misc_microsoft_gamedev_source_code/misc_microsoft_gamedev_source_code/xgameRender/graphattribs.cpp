//============================================================================
// graphattribs.h
// Ensemble Studios (C) 2007
//============================================================================

#include "xgameRender.h"
#include "graphattribs.h"

//============================================================================
//============================================================================
void BGraphAttribs::addTimeline(const BDynamicArray<float>& mData, DWORD color, int playerID, float fadeDuration)
{
   BGraphTimeline timeLine;
   timeLine.mColor = color;
   timeLine.mPlayerID = playerID;
   timeLine.mData = mData;
   timeLine.mFadeDuration = fadeDuration;
   timeLine.mFadeAlpha = 1.0f;
   timeLine.mFadeStartTime = -1.0f;

   mTimelines.add(timeLine);
}

//============================================================================
//============================================================================
void BGraphAttribs::normalize()
{
   //-- normalize the data of all of our timelines

   int    maxSampleCount = 0;
   float  maxValue = 0.0f;
   for (int i =0; i < mTimelines.getNumber(); ++i)
   {
      //-- determine the largest Sample Count (X - AXIS)
      int curCount = mTimelines[i].mData.getNumber();
      if (curCount > maxSampleCount)
         maxSampleCount = curCount;

      //-- largest
      float curValue = 0.0f;
      for (int j = 0; j < mTimelines[i].mData.getNumber(); ++j)
      {
         curValue = mTimelines[i].mData[j];
         if (curValue > maxValue)
            maxValue = curValue;
      }     
   }

   //-- burn trough the time lines 
   for (int z = 0; z < mTimelines.getNumber(); ++z)
   {
      //-- pad with zeros if necessary
      int padCount = maxSampleCount - mTimelines[z].mData.getNumber();
      if (padCount > 0)
      {
         for (int w = 0; w < padCount; ++w)
            mTimelines[z].mData.add(0.0f);
      }

      //-- normalize           
      for (int y=0; y < mTimelines[z].mData.getNumber(); ++y)
      {
         mTimelines[z].mData[y] = mTimelines[z].mData[y] / max(maxValue, 1.0f);
      }      
   }

   mMaxValue = maxValue;
}

//============================================================================
//============================================================================
void BGraphAttribs::update(double time)
{
   double dTime = 0.0f;   
   for (int i = 0; i < mTimelines.getNumber(); ++i)
   {
      if (mTimelines[i].mFadeDuration > 0.0f)
      {
         //-- start the fade
         if (mTimelines[i].mFadeStartTime < 0.0f)
         {
            mTimelines[i].mFadeAlpha = 0.0f;
            mTimelines[i].mFadeStartTime = time;
         }
         else
         {
            dTime = time - mTimelines[i].mFadeStartTime;
            mTimelines[i].mFadeAlpha = dTime / mTimelines[i].mFadeDuration;
            mTimelines[i].mFadeAlpha = Math::Clamp(mTimelines[i].mFadeAlpha,(double) 0.0f, (double) 1.0f);
         }
      }   
   }
}

//============================================================================
//============================================================================
BGraphTimeline* BGraphAttribs::getTimeline(int index)
{
   if ( (index<0) || (index>= mTimelines.getNumber()) )
      return NULL;

   return &(mTimelines[index]);
}
