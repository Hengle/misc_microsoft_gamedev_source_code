//============================================================================
// graphattribs.h
// Ensemble Studios (C) 2007
//============================================================================

#pragma once
#include "math\generalVector.h"

#include "math\halfFloat.h"
#include "color.h"
#include "d3dtexturemanager.h"
class BGraphManager;

//============================================================================
//============================================================================
class BGraphTimeline
{
   public:
      BGraphTimeline(){};
     ~BGraphTimeline(){ clear(); };

      void clear()
      {
         mData.clear();
         mColor = cDWORDWhite;
         mPlayerID=-1;
         mFadeAlpha=0.0f;
         mFadeStartTime=0.0f;
         mFadeDuration=0.0f;
         mSizeX = 5.0f;
         mSizeY = 5.0f;
         mDotInterval = 1.0f;
         mbEnableHighlight = false;
         mbIsSelected = false;
      }

      BGraphTimeline& operator=(const BGraphTimeline& a)
      {
         mData.resize(a.mData.getNumber());
         for (int i = 0; i < a.mData.getNumber(); ++i)
         {
           mData[i] = a.mData[i];
         }
         mColor = a.mColor;
         mPlayerID = a.mPlayerID;
         mFadeAlpha = a.mFadeAlpha;
         mFadeStartTime = a.mFadeStartTime;
         mFadeDuration = a.mFadeDuration;
         mbEnableHighlight = a.mbEnableHighlight;
         mbIsSelected = a.mbIsSelected;
         mSizeX = a.mSizeX;
         mSizeY = a.mSizeY;
         mDotInterval = a.mDotInterval;
         return *this;
      }

      BDynamicArray<float> mData;
      DWORD                mColor;
      int                  mPlayerID;
      double               mFadeAlpha;
      double               mFadeStartTime;
      double               mFadeDuration;
      float                mSizeX; // line width / dot size
      float                mSizeY;
      float                mDotInterval;
      bool                 mbEnableHighlight : 1;
      bool                 mbIsSelected : 1;
};

//============================================================================
//============================================================================
class BGraphAttribs
{
   friend class BGraphManager;
   friend class BGraphRenderer;
public:
   BGraphAttribs()
   {
      clear();
   }
  ~BGraphAttribs()
   {
      clear();
   }

   void clear()
   {
      mType = cLineGraphNormal;
      mPosition.clear();
      mSizeX = 0.0f;
      mSizeY = 0.0f;
      mbEnabled = true;
      mMaxValue = 0.0f;
      mTimelines.clear();
      mBackgroundColor = cDWORDWhite;
      mbRenderBackground = false;
      mbRenderFilled = false;
      mTexture = cInvalidManagedTextureHandle;
      mTickTexture = cInvalidManagedTextureHandle;    

      mHorizontalSmallTickInterval = 5;
      mHorizontalLargeTickInterval = 10;
      mHorizontalSmallTickSizeX    = 4.0f;
      mHorizontalSmallTickSizeY    = 4.0f;
      mHorizontalLargeTickSizeX    = 4.0f;
      mHorizontalLargeTickSizeY    = 8.0f;
   }

   BGraphAttribs& operator=(const BGraphAttribs& a)
   {
      mType = a.mType;
      mPosition = a.mPosition;
      mSizeX = a.mSizeX;
      mSizeY = a.mSizeY;
      mMaxValue = a.mMaxValue;
      mBackgroundColor = a.mBackgroundColor;
      mbEnabled = a.mbEnabled;
      mbRenderFilled = a.mbRenderFilled;
      mbRenderBackground = a.mbRenderBackground;
      mTexture = a.mTexture;
      mTickTexture = a.mTickTexture;

      mHorizontalSmallTickInterval = a.mHorizontalSmallTickInterval;
      mHorizontalLargeTickInterval = a.mHorizontalLargeTickInterval;
      mHorizontalSmallTickSizeX = a.mHorizontalSmallTickSizeX;
      mHorizontalSmallTickSizeY = a.mHorizontalSmallTickSizeY;
      mHorizontalLargeTickSizeX = a.mHorizontalLargeTickSizeX;
      mHorizontalLargeTickSizeY = a.mHorizontalLargeTickSizeY;

      mTimelines.resize(a.mTimelines.getNumber());      
      for (int i = 0; i < a.mTimelines.getNumber(); ++i)
      {
        mTimelines[i] = a.mTimelines[i];
      }
      return *this;
   }

   enum BGraphType
   {
      cLineGraphNormal,
      cLineGraphDots,      
      cGrapTypeTotal,
   };

   BGraphType getType() const { return mType; };
   void setType(BGraphType type) { mType = type; };

   int  getTimelineCount() { return mTimelines.getNumber(); }
   BGraphTimeline* getTimeline(int index);
   void addTimeline(const BDynamicArray<float>& mData, DWORD color, int playerID, float fadeDuration);
   void normalize();
   void update(double time);

   const BVec3& getPosition() const { return mPosition; }
   void setPosition(const BVec3& position) { mPosition = position; }

   float getSizeX() const { return mSizeX; }
   void  setSizeX(float size) { mSizeX = size; }

   float getSizeY() const { return mSizeY; }
   void  setSizeY(float size) { mSizeY = size; }

   bool  getEnabled() const { return mbEnabled; }
   void  setEnabled(bool enabled) { mbEnabled = enabled; }

   bool  getRenderFilled() const { return mbRenderFilled; }
   void  setRenderFilled(bool enabled) { mbRenderFilled = enabled; }

   bool  getRenderBackground() const { return mbRenderBackground; }
   void  setRenderBackground(bool enabled) { mbRenderBackground = enabled; }

   DWORD getBackgroundColor() const { return mBackgroundColor; }
   void setBackgroundColor(DWORD c) { mBackgroundColor = c; }

   BManagedTextureHandle getTextureHandle() const { return mTexture; }
   void setTextureHandle(BManagedTextureHandle v) { mTexture = v; }

   BManagedTextureHandle getTickTextureHandle() const { return mTickTexture; }
   void setTickTextureHandle(BManagedTextureHandle v) { mTickTexture = v; }

   int getHorizontalSmallTickInterval() const { return mHorizontalSmallTickInterval; };
   void setHorizontalSmallTickInterval(int value) { mHorizontalSmallTickInterval = value; };
   int getHorizontalLargeTickInterval() const { return mHorizontalLargeTickInterval; };
   void setHorizontalLargeTickInterval(int value) { mHorizontalLargeTickInterval = value; };

   float getHorizontalSmallTickSizeX() const { return mHorizontalSmallTickSizeX; };
   void setHorizontalSmallTickSizeX(float value) { mHorizontalSmallTickSizeX = value; };
   float getHorizontalSmallTickSizeY() const { return mHorizontalSmallTickSizeY; };
   void setHorizontalSmallTickSizeY(float value) { mHorizontalSmallTickSizeY = value; };

   float getHorizontalLargeTickSizeX() const { return mHorizontalLargeTickSizeX; };
   void setHorizontalLargeTickSizeX(float value) { mHorizontalLargeTickSizeX = value; };
   float getHorizontalLargeTickSizeY() const { return mHorizontalLargeTickSizeY; };
   void setHorizontalLargeTickSizeY(float value) { mHorizontalLargeTickSizeY = value; };

private:            

   BDynamicArray<BGraphTimeline> mTimelines;
   BGraphType     mType;
   BVec3          mPosition;
   BHalfFloat     mSizeX;
   BHalfFloat     mSizeY;
   BHalfFloat     mMaxValue;
   DWORD          mBackgroundColor;
   BManagedTextureHandle mTexture;
   BManagedTextureHandle mTickTexture;

   int            mHorizontalSmallTickInterval;
   int            mHorizontalLargeTickInterval;
   float          mHorizontalSmallTickSizeX;
   float          mHorizontalSmallTickSizeY;
   float          mHorizontalLargeTickSizeX;
   float          mHorizontalLargeTickSizeY;
   
   bool           mbEnabled:1;
   bool           mbRenderFilled : 1;
   bool           mbRenderBackground : 1;      
};