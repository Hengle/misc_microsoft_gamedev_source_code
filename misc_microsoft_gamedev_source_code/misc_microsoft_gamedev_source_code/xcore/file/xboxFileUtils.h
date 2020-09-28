//==============================================================================
// xboxFileUtils.cpp
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

#pragma once

#ifdef XBOX

class BXboxFileUtils
{
   public:
      // Creates the cache partition if one does not exist.
      static bool             isCacheInitialized();
      static void             initXboxCachePartition();

   private:
      static bool             mCacheInitialized;

};

#endif // XBOX