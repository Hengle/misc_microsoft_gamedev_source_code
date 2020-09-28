//------------------------------------------------------------------------------
// File: palette.h
//------------------------------------------------------------------------------
#pragma once
//------------------------------------------------------------------------------
#include "colorUtils.h"
//------------------------------------------------------------------------------
class BPalette
{
public:
   enum { cMaxPalSize = 256 };

protected:
   BRGBAColor mEntries[cMaxPalSize];
   
   int mSigEntries;
   bool mHasAlpha;
      
public:
   BPalette(int sigEntries = cMaxPalSize, bool hasAlpha = false) :
      mSigEntries(debugRangeCheckIncl(sigEntries, 1, (int)cMaxPalSize)),
      mHasAlpha(hasAlpha)
   { 
      std::fill(mEntries, mEntries + cMaxPalSize, gBlackColor);
   }

   BPalette(const BRGBAColor* Pentries, int sigEntries = cMaxPalSize, bool hasAlpha = false) :
      mSigEntries(debugRangeCheckIncl(sigEntries, 1, (int)cMaxPalSize)),
      mHasAlpha(hasAlpha)
   { 
	   BDEBUG_ASSERT(Pentries);
	   debugRangeCheckIncl(sigEntries, (int)cMaxPalSize);
      
      Utils::Copy(Pentries, Pentries + sigEntries, mEntries);
	   std::fill(mEntries + sigEntries, mEntries + cMaxPalSize, gBlackColor);
   }

   BPalette& operator= (const BPalette& p)
   {
      mSigEntries = p.mSigEntries;
      mHasAlpha = p.mHasAlpha;

      Utils::Copy(p.mEntries, p.mEntries + cMaxPalSize, mEntries);

      return *this;
   }

   // any of the cMaxPalSize entries may be modified
   void set(const BRGBAColor* Pentries, int first = 0, int num = cMaxPalSize)
   {
      BDEBUG_ASSERT(Pentries);
	   debugRangeCheck(first, (int)cMaxPalSize);
      BDEBUG_ASSERT(num >= 0);
      BDEBUG_ASSERT((first + num - 1) < cMaxPalSize);

      Utils::Copy(Pentries, Pentries + num, mEntries + first);
   }

   void set(int entry, const BRGBAColor& c) 
   { 
	   set(&c, entry, 1); 
   }

   void setSigEntries(int sigEntries) 
   {
      mSigEntries = debugRangeCheckIncl(sigEntries, 1, (int)cMaxPalSize);
   }

   void setAlpha(bool hasAlpha)
   {
      mHasAlpha = hasAlpha;
   }

   int getSigEntries(void) const 
   { 
	   return mSigEntries; 
   }

   bool getHasAlpha(void) const 
   { 
	   return mHasAlpha; 
   }

   const BRGBAColor* getPal(void) const 
   { 
	   return mEntries; 
   }

   const BRGBAColor& entry(uint i) const 
   {
      return mEntries[debugRangeCheck(i, (int)cMaxPalSize)]; 
   }

   BRGBAColor& entry(uint i) 
   {
      return mEntries[debugRangeCheck(i, (int)cMaxPalSize)]; 
   }

   const BRGBAColor& operator[] (uint i) const 
   { 
	   return entry(i); 
   }

   BRGBAColor& operator[] (uint i) 
   { 
	   return entry(i); 
   }

   uint bestMatch(const BRGBAColor& c) const
   {
      uint bestDist = UINT_MAX, bestIndex = 0;

      for (int i = 0; i < mSigEntries; i++)
      {
         uint dist = entry(i).distSquared(c, mHasAlpha);
         
         if (dist < bestDist)
         { 
            bestDist = dist;
            bestIndex = i;
         } 
      }

      return bestIndex;
   }

   bool operator== (const BPalette& b) const
   {
      if ((mHasAlpha != b.mHasAlpha) || (mSigEntries != b.mSigEntries)) 
		   return false;
      
      //return std::equal(mEntries, mEntries + mSigEntries, b.mEntries);
      for (int i = 0; i < mSigEntries; i++)
         if (mEntries[i] != b.mEntries[i])
            return false;
            
      return true;
   }

   bool operator!= (const BPalette& b) const
   { 
	   return !(*this == b); 
   }
};
//------------------------------------------------------------------------------
