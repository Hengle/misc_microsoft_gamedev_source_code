//==============================================================================
// xstackTrace.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#ifdef XBOX
#include "hash\hash.h"

//==============================================================================
// class BXStackTrace
//==============================================================================
class BXStackTrace
{
   uint mNumLevels;

   enum { cMaxLevels = 31 };
   DWORD mBackTrace[cMaxLevels];

public:
   BXStackTrace();
   ~BXStackTrace();
      
   uint getNumLevels(void) const { return mNumLevels; }
   DWORD getLevelAddr(uint level) const { BDEBUG_ASSERT(level < mNumLevels); return mBackTrace[level]; }
   
   bool capture(int skipLevels = 0);
   bool capture(_EXCEPTION_POINTERS* pExcept);
   bool capture(DWORD* pInstPtr, DWORD* pStackPtr);
   
   bool operator== (const BXStackTrace& rhs) const;
   bool operator< (const BXStackTrace& rhs) const;
   
   const DWORD* getBackTraceArrayPtr(void) const { return mBackTrace; }
   
   uint getHash(void) const { return hashFast(mBackTrace, mNumLevels, hashFast(&mNumLevels, sizeof(mNumLevels))); }
   operator size_t() const { return getHash(); }
};
#endif // XBOX