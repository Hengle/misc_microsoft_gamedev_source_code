//==============================================================================
// htemplate.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef _RESOURCECACHE_H_
#define _RESOURCECACHE_H_

//==============================================================================
// Includes
#include "copylist.h"


//==============================================================================
// Function pointer definitions.
//==============================================================================
// void myFunc(long resourceID)
typedef void (*RESOURCE_CACHE_VOID_ID_PROC)(long resourceID);
// void myFunc(long ownerID, long resourceID)
typedef void (*RESOURCE_CACHE_VOID_BOTH_ID_PROC)(long ownerID, long resourceID);
// long myFunc(void)
typedef long (*RESOURCE_CACHE_LONG_VOID_PROC)(void);

//==============================================================================
// class BResourceCacheEntry
//==============================================================================
class BResourceCacheEntry
{
   public:
                              BResourceCacheEntry(void) : mOwnerID(-1), mResourceID(-1) {}
                              BResourceCacheEntry(long ownerID, long resourceID) : mOwnerID(ownerID), mResourceID(resourceID) {}

      long                    mOwnerID;
      long                    mResourceID;
};



//==============================================================================
// class BResourceCache
//==============================================================================
class BResourceCache
{
   public:
                              BResourceCache(void);
                              ~BResourceCache(void);
      bool                    init(long size);
      void                    empty(void);
      bool                    initOwnerList(long size);

      bool                    grabResource(long ownerID);      // result is true if you got a NEW resource (gainingResourceFunc will also get called if new)
      void                    releaseResource(long ownerID);   
      long                    getResourceID(long ownerID);     // returns -1 if no resource -- i.e. won't grab resources, just snoops current.

      enum
      {
         cReleaseNoChange,    // leave released things in their current spot in the LRU
         cReleaseHead,        // put released things at the beginning of the LRU (used next time a new node is needed)
         cReleaseTail,        // put released things at the end of the LRU (used as far from now as possible)
         cReleaseModeCount
      };
      void                    setReleaseMode(long releaseMode);

      // Functions to set up callbacks.
      void                    setCleanupFunc(RESOURCE_CACHE_VOID_BOTH_ID_PROC cleanupFunc) {mCleanupFunc=cleanupFunc;}
      void                    setCreateResourceFunc(RESOURCE_CACHE_LONG_VOID_PROC createResourceFunc) {mCreateResourceFunc=createResourceFunc;}
      void                    setLosingResourceFunc(RESOURCE_CACHE_VOID_BOTH_ID_PROC losingResourceFunc) {mLosingResourceFunc=losingResourceFunc;}
      void                    setGainingResourceFunc(RESOURCE_CACHE_VOID_BOTH_ID_PROC gainingResourceFunc) {mGainingResourceFunc=gainingResourceFunc;}

   protected:
      long                             mLRUSize;
      BCopyList<BResourceCacheEntry>   mLRU;

      long                             mOwnerSize;
      BHandle                          *mOwnerList;

      long                             mReleaseMode;

      // Callback function pointers.
      RESOURCE_CACHE_VOID_BOTH_ID_PROC mCleanupFunc;
      RESOURCE_CACHE_VOID_BOTH_ID_PROC mGainingResourceFunc;
      RESOURCE_CACHE_VOID_BOTH_ID_PROC mLosingResourceFunc;
      RESOURCE_CACHE_LONG_VOID_PROC    mCreateResourceFunc;
};

#endif 

//==============================================================================
// eof: resourcecache.h
//==============================================================================
