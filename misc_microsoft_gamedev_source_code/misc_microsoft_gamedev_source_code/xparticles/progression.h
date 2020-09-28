//============================================================================
// progression.h
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#pragma once
#include "particleheap.h"
#include "xmlreader.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BGradientPoint
{
   public:
      BGradientPoint() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);
      
      void clear(void)
      {
         mAlpha = 0.0f;
         mColor = 0;
      }

      float mAlpha;
      DWORD mColor;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BColorProgression
{
   public:
      BColorProgression() { clear(); }
           
      bool load(BXMLNode node, BXMLReader* pReader);
      DWORD getValue(float alpha) const;
      void  getValue(float alpha, XMVECTOR* RESTRICT pValue);

      int getMemoryCost();
      
      void clear(void)
      {
         mCycles = 0.0f;
         mLoop = false;
      }

      BDynamicParticleArray<BGradientPoint> mStages;      
      float mCycles;
      bool  mLoop;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BPalletteColor
{
   public:
      BPalletteColor() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);

      int getMemoryCost();
      
      void clear(void)
      {
         mWeight = 0.0f;
         mColor = 0;
      }

      float mWeight;
      DWORD mColor;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BFloatProgressionStage
{
   public:
      BFloatProgressionStage() { clear(); }
     
      bool load(BXMLNode node, BXMLReader* pReader);

      int getMemoryCost();
      
      void clear(void)
      {
         mAlpha = 0;
         mValue = 0;
         mValueVar = 0;
      }

      float mValue;
      float mAlpha;
      float mValueVar;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BFloatProgression
{
   public:
      BFloatProgression() { clear(); }
     
      bool  load(BXMLNode node, BXMLReader* pReader);
      void  getValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const;
      void  getValueWithCycles(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const;

      int getMemoryCost();
      
      void clear(void)
      {
         mCycles = 0;
         mLoop = 0;
      }

      BDynamicParticleArray<BFloatProgressionStage> mStages;
      float mCycles;
      bool  mLoop;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BVectorProgression
{
   public: 
      BVectorProgression(){};
     ~BVectorProgression(){};
      bool load(BXMLNode node, BXMLReader* pReader);
      void initLookupTable(int entryCount);

      int getMemoryCost();
      
      void getXValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const;
      void getYValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const;
      void getZValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const;
                  
      BFloatProgression mXProgression;
      BFloatProgression mYProgression;
      BFloatProgression mZProgression;      
};
