//============================================================================
//
// flashmoviedefinition.h
//
//============================================================================
#pragma once
#include "scaleformIncludes.h"

//============================================================================
// class BFlashMovieDefinition
//============================================================================
class BFlashMovieDefinition
{
   public:
      BFlashMovieDefinition();
     ~BFlashMovieDefinition();

      enum eStatus
      {
         cStatusInvalid,
         cStatusInitialized,
         cStatusLoaded,
         cStatusLoadFailed,         
         cStatusTotal
      };

      bool init(const char* filename, uint category);
      bool load();
      void unload();

      void addRef(void) { mRefCount++; }
      void decRef(void) { mRefCount--; mRefCount = Math::Max((uint) 0, mRefCount); }
      uint getRefCount(void) { return mRefCount; };

      GFxMovieDef*        mpMovieDef;
      GFxMovieInfo        mMovieInfo;      
      BString             mFilename;
      uint                mRefCount;
      uint                mAssetCategoryIndex;
      uint                mStatus;
};