//============================================================================
// progression.cpp
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#include "xparticlescommon.h"
#include "xmlreader.h"
#include "progression.h"
#include "color.h"
#include "ParticleSystemManager.h"
#include "math\VMXUtils.h"
#include "consoleOutput.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BColorProgression::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   int numChildren = node.getNumberChildren();
   BXMLNode child;
   for (int i = 0; i < numChildren; i++)
   {
      child = node.getChild(i);
      const BPackedString szTag(child.getName());
      if (szTag.compare(("Stages")) == 0)
      {
         int numGrandChildren = child.getNumberChildren();
         mStages.resize(numGrandChildren);
         BXMLNode grandChild;
         for (int j = 0; j < numGrandChildren; ++j)
         {
            grandChild = child.getChild(j);
            if (!mStages[j].load(grandChild, pReader))
               return false;
         }
      }
      else if (szTag.compare(("Loop")) == 0)
      {
         child.getTextAsBool(mLoop);
      }      
      else if (szTag.compare(("Cycles")) == 0)
      {
         child.getTextAsFloat(mCycles);
      }
      else
      {
         gConsoleOutput.warning("BColorProgression::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BColorProgression::getMemoryCost()
{
   int bytes = 0;
   bytes += mStages.getSizeInBytes();
   bytes += sizeof(mCycles);
   bytes += sizeof(mLoop);
   
   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD BColorProgression::getValue(float alpha) const
{
   debugRangeCheckIncl<float>(alpha, 1.0f);   
   int numStages = mStages.getSize();
   BDEBUG_ASSERT(numStages >= 2);

   for (int i = 1; i < numStages; ++i)
   {
      if (mStages[i].mAlpha >= alpha)
      {
         float a = mStages[i-1].mAlpha;
         float b = mStages[i].mAlpha;
         float oneOverLength = 1.0f / (b-a);
         float x = alpha - a;
         float localAlpha = x * oneOverLength;

         DWORD colorA = mStages[i-1].mColor;
         DWORD colorB = mStages[i].mColor;

         return BParticleSystemManager::lerpColor(colorA, colorB, localAlpha);                     
      }
   }
   return cDWORDWhite;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BColorProgression::getValue(float alpha, XMVECTOR* RESTRICT pOut)
{
   debugRangeCheckIncl<float>(alpha, 1.0f);   
   BDEBUG_ASSERT(mStages.getSize() >= 2);

   float tempAlpha = *(float*)&alpha;
   //float a,b,x, length,oneOverLength; 

   XMVECTOR a, b, x, length, colorA, colorB, localAlpha;
   x;
   XMVECTOR mag, mask, ooMag;

   for (uint i = 1; i < mStages.getSize(); ++i)
   {
      if (mStages[i].mAlpha >= alpha)
      {
         a = XMVectorReplicate(mStages[i-1].mAlpha);
         b = XMVectorReplicate(mStages[i].mAlpha);
         length = __vrlimi(XMVectorSubtract(b, a), XMVectorZero(), VRLIMI_CONST(0,1,1,1), 0);
         mag  = XMVector3Length(length);
         mask = XMVectorGreater(mag, gVectorFloatCompEps);
         ooMag = XMVectorReciprocal(mag);

         XMVECTOR inAlpha = XMVectorReplicate(tempAlpha);
         localAlpha = __vrlimi(XMVectorAndInt(XMVectorSubtract(inAlpha,a) * ooMag, mask), XMVectorZero(), VRLIMI_CONST(0,0,0,0), 0);

         colorA = XMLoadColor((const XMCOLOR*) &mStages[i-1].mColor);
         colorB = XMLoadColor((const XMCOLOR*) &mStages[i].mColor);

         *pOut = XMVectorLerpV(colorA, colorB, localAlpha);
         return;
      }
   }

   *pOut = gVectorOne;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BGradientPoint::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   node.getChildValue("Color", mColor);
   node.getChildValue("Alpha", mAlpha); 
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BPalletteColor::getMemoryCost()
{
   int bytes = 0;
   bytes += sizeof(mWeight);
   bytes += sizeof(mColor);

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BPalletteColor::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   node.getChildValue("Color", mColor);
   node.getChildValue("Weight", mWeight);

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFloatProgressionStage::getMemoryCost()
{
   return sizeof(BFloatProgressionStage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFloatProgressionStage::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   if (!node.getChildValue("Value", mValue))
      return false;
   if (!node.getChildValue("Alpha", mAlpha))
      return false;
   if (!node.getChildValue("ValueVariance", mValueVar))
      return false;

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFloatProgression::getMemoryCost()
{
   int bytes = 0;

   bytes += mStages.getSizeInBytes();
   bytes += sizeof(mCycles);
   bytes += sizeof(mLoop);

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFloatProgression::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();
   BXMLNode child;
   for (int i = 0; i < numChildren; i++)
   {
      child = node.getChild(i);
      const BPackedString szTag(child.getName());
      if (szTag.compare(("Stages")) == 0)
      {
         int numGrandChildren = child.getNumberChildren();
         mStages.resize(numGrandChildren);
         BXMLNode grandChild;
         for (int j = 0; j < numGrandChildren; ++j)
         {
            grandChild = child.getChild(j);
            if (!mStages[j].load(grandChild, pReader))
               return false;
         }
      }
      else if (szTag.compare(("Loop")) == 0)
      {
         if (!child.getTextAsBool(mLoop))
            return false;
      }
      else if (szTag.compare(("Cycles")) == 0)
      {
         if (!child.getTextAsFloat(mCycles))
            return false;
      }
      else
      {
         gConsoleOutput.warning("BFloatProgression::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFloatProgression::getValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const
{
   debugRangeCheckIncl<float>(alpha, 1.0f);   
   BDEBUG_ASSERT(mStages.getSize() >= 2);

   float tempAlpha = *(float*)&alpha;
   //float a,b,x, length,oneOverLength; 
   
   XMVECTOR a, b, x, length, pointAValue, pointBValue, localAlpha;
   x;
   XMVECTOR mag, mask, ooMag;
         
   for (uint i = 1; i < mStages.getSize(); ++i)
   {
      if (mStages[i].mAlpha >= alpha)
      {
         //-- Pseudo Code of the local alpha calculation before it was vmxed and LHS cleaned
#if 0
         float  tempA = mStages[i-1].mAlpha;
         float  tempB = mStages[i].mAlpha;
         float  tempLength = tempB-tempA;
         float  tempOneOverLength = 1.0f / tempLength;
         float tempLocalAlpha = 1.0f;
         if (tempLength > 0.0f)
         {
            float x = alpha - tempA;
            tempLocalAlpha = x * tempOneOverLength;
         }
         float tempAValue  = BParticleSystemManager::getIndexedVarience(varianceIndex, mStages[i-1].mValue, mStages[i-1].mValueVar);
         float tempBValue  = BParticleSystemManager::getIndexedVarience(varianceIndex, mStages[i].mValue, mStages[i].mValueVar);
         float output = BParticleSystemManager::lerpScalar(tempAValue, tempBValue, tempLocalAlpha);
#endif
         
         
         a = XMLoadScalar(&mStages[i-1].mAlpha);
         b = XMLoadScalar(&mStages[i].mAlpha);
         length = __vrlimi(XMVectorSubtract(b, a), XMVectorZero(), VRLIMI_CONST(0,1,1,1), 0);
         mag  = XMVector3Length(length);
         mask = XMVectorGreater(mag, gVectorFloatCompEps);
         ooMag = XMVectorReciprocal(mag);

         XMVECTOR inAlpha = XMLoadScalar(&tempAlpha);
         localAlpha = __vrlimi(XMVectorAndInt(XMVectorSubtract(inAlpha,a) * ooMag, mask), XMVectorZero(), VRLIMI_CONST(0,1,1,1), 0);
         
         pointAValue = PS_GET_INDEXED_VARIANCE_X(varianceIndex,  mStages[i-1].mValue, mStages[i-1].mValueVar);
         pointBValue = PS_GET_INDEXED_VARIANCE_X(varianceIndex,  mStages[i].mValue, mStages[i].mValueVar);

         
         //trace("Life Alpha (%f) ==> Opacity (%f)", alpha, output);
         *pOut = XMVectorLerpV(pointAValue, pointBValue, localAlpha);
         return;
      }
   }

   *pOut = gVectorOne;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFloatProgression::getValueWithCycles(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const
{
   if(mLoop)
   {
      alpha *= mCycles;
      alpha = fmod(alpha, 1.0f);
   }

   getValue(alpha, varianceIndex, pOut);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BVectorProgression::getMemoryCost()
{
   int bytes = 0;

   bytes += mXProgression.getMemoryCost();
   bytes += mYProgression.getMemoryCost();
   bytes += mZProgression.getMemoryCost();

   return bytes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BVectorProgression::load(BXMLNode node, BXMLReader* pReader)
{
   if (!node || !pReader)
      return false;

   int numChildren = node.getNumberChildren();
   BXMLNode child;
   for (int i = 0; i < numChildren; i++)
   {
      child = node.getChild(i);
      const BPackedString szTag(child.getName());
      if (szTag.compare(("XProgression")) == 0)
      {
         if (!mXProgression.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("YProgression")) == 0)
      {
         if (!mYProgression.load(child, pReader))
            return false;
      }
      else if (szTag.compare(("ZProgression")) == 0)
      {
         if (!mZProgression.load(child, pReader))
            return false;
      }
      else
      {
         gConsoleOutput.warning("BVectorProgression::load: Unrecognized element name: %s\n", szTag.getPtr());
      }
   }
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BVectorProgression::getXValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const
{
   mXProgression.getValue(alpha, varianceIndex, pOut);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BVectorProgression::getYValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const
{
   mYProgression.getValue(alpha, varianceIndex, pOut);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BVectorProgression::getZValue(float alpha, int varianceIndex, XMVECTOR* RESTRICT pOut) const
{
   mZProgression.getValue(alpha, varianceIndex, pOut);
}



