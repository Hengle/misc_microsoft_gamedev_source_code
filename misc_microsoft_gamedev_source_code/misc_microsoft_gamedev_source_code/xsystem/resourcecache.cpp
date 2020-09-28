//==============================================================================
// resourcecache.cpp
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "resourcecache.h"


//==============================================================================
// BResourceCache::BResourceCache
//==============================================================================
BResourceCache::BResourceCache(void) :
   mLRUSize(-1),
   mOwnerSize(-1),
   mOwnerList(NULL),
   mReleaseMode(cReleaseNoChange),

   mCleanupFunc(NULL),
   mGainingResourceFunc(NULL),
   mLosingResourceFunc(NULL),
   mCreateResourceFunc(NULL)
{
}


//==============================================================================
// BResourceCache::~BResourceCache
//==============================================================================
BResourceCache::~BResourceCache(void)
{
   empty();
}


//==============================================================================
// BResourceCache::init
//==============================================================================
bool BResourceCache::init(long size)
{
   // Empty any existing stuff.
   empty();

   // Need a creation function.
   if(!mCreateResourceFunc)
   {
      BASSERT(0);
      return(false);
   }

   // Need a real size.
   if(size<=0)
   {
      BASSERT(0);
      return(false);
   }

   // Remember size.
   mLRUSize=size;

   // Add the requested number of entries.
   for(long i=0; i<size; i++)
   {
      // Create a resource.
      long resourceID=mCreateResourceFunc();

      // Add it to list with no owner.
      mLRU.addToHead(BResourceCacheEntry(-1, resourceID));
   }

   // Success.
   return(true);
}


//==============================================================================
// BResourceCache::empty
//==============================================================================
void BResourceCache::empty(void)
{
   // If we have a cleanup function registered, call it for each item.
   if(mCleanupFunc)
   {
      // Iterate the list.
      BHandle handle=NULL;
      BResourceCacheEntry *entry=NULL;
      entry=mLRU.getHead(handle);
      while(handle)
      {
         // If we got something real, call cleanup.
         if(entry)
            mCleanupFunc(entry->mOwnerID, entry->mResourceID);

         // Next one.
         entry=mLRU.getNext(handle);
      }
   }

   // Nuke cache.
   mLRU.empty();
   mLRUSize=-1;

   // Nuke owner list.
   delete []mOwnerList;
   mOwnerList=NULL;
   mOwnerSize=-1;
}


//==============================================================================
// BResourceCache::initOwnerList
//==============================================================================
bool BResourceCache::initOwnerList(long size)
{
   // Get rid of old crap.
   delete []mOwnerList;
   mOwnerList=NULL;
   mOwnerSize=-1;

   // Check size
   if(size<=0)
   {
      BASSERT(0);
      return(false);
   }

   // Allocate list.
   mOwnerList = new BHandle[size];
   if(!mOwnerList)
   {
      BASSERT(0);
      return(false);
   }
   mOwnerSize=size;

   // Init.
   memset(mOwnerList, 0, mOwnerSize*sizeof(BHandle));

   // Iterate over the LRU and clear out owners.
   BHandle handle=NULL;
   BResourceCacheEntry *entry=NULL;
   entry=mLRU.getHead(handle);
   while(handle)
   {
      // clear owner
      if(entry)
         entry->mOwnerID=-1;

      // Next one.
      entry=mLRU.getNext(handle);
   }

   // Success.
   return(true);
}


//==============================================================================
// BResourceCache::getResourceID
//==============================================================================
long BResourceCache::getResourceID(long ownerID)
{
   // Check stuff.
   if(ownerID<0 || ownerID>=mOwnerSize || !mOwnerList)
   {
      // Don't give us bad stuff.
      BASSERT(0);
      return(-1);
   }

   // If no handle, owner doesn't have any resource right now.
   if(!mOwnerList[ownerID])
      return(-1);

   // Look up entry for owner.
   BResourceCacheEntry *entry=mLRU.getItem(mOwnerList[ownerID]);

   // If this is null, something really bad happened with the list.
   if(!entry)
   {
      BASSERT(0);
      return(-1);
   }

   // Give back the ID.
   return(entry->mResourceID);
}


//==============================================================================
// BResourceCache::grabResource
//==============================================================================
bool BResourceCache::grabResource(long ownerID)
{
   // Check stuff.
   if(ownerID<0 || ownerID>=mOwnerSize || !mOwnerList)
   {
      // Don't give us bad stuff.
      BASSERT(0);
      return(false);
   }

   // If we don't already have a resource, we have to pull one from the LRU.
   bool gotSomethingNew=false;
   if(!mOwnerList[ownerID])
   {
      // Get the head of the list.
      BHandle hItem=NULL;
      BResourceCacheEntry *entry=mLRU.getHead(hItem);
      if(!hItem || !entry || entry->mResourceID<0)
      {
         // Something seriously crazy happened to the list.  Just close our eyes and bail.
         BASSERT(0);
         return(false);
      }

      // If this belonged to someone, we need to take care of some bookkeeping.
      if(entry->mOwnerID>=0)
      {
         // Unlink old owner.
         mOwnerList[entry->mOwnerID]=NULL;

         // Tell old owner the bad news.
         if(mLosingResourceFunc)
            mLosingResourceFunc(entry->mOwnerID, entry->mResourceID);
      }

      // Set up the new owner
      mOwnerList[ownerID]=hItem;
      entry->mOwnerID=ownerID;

      // Tell the new owner that it has acquired something new.
      if(mGainingResourceFunc)
         mGainingResourceFunc(ownerID, entry->mResourceID);

      // Remember that this was a new resource so we can pass that info back.
      gotSomethingNew=true;
   }

   // Push this node to the end of the LRU list since we just used it.
   mLRU.moveToTail(mOwnerList[ownerID]);

   // Return back whether we got something new or not.
   return(gotSomethingNew);
}


//==============================================================================
// BResourceCache::releaseResource
//==============================================================================
void BResourceCache::releaseResource(long ownerID)
{
   // Check stuff.
   if(ownerID<0 || ownerID>=mOwnerSize || !mOwnerList)
   {
      // Don't give us bad stuff.
      //DCP/JCE: Taking this out temporarily as it's foobaring returns from screen-savered machines.
      //BASSERT(0);
      return;
   }

   // If this owner really didn't have anything to release, we're done.
   if(!mOwnerList[ownerID])
      return;

   // Grab the entry.
   BHandle hItem=mOwnerList[ownerID];
   BResourceCacheEntry *entry=mLRU.getItem(hItem);

   // Nuke the link to this owner.
   entry->mOwnerID=-1;
   mOwnerList[ownerID]=NULL;

   // Ok, now we've freed the node from it's owner we do something (or nothing) with it based
   // on the release mode.
   if(mReleaseMode==cReleaseHead)
      mLRU.moveToHead(hItem);
   else if(mReleaseMode==cReleaseTail)
      mLRU.moveToTail(hItem);
   // else leave it where it currently is.
}


//==============================================================================
// BResourceCache::setReleaseMode
//==============================================================================
void BResourceCache::setReleaseMode(long releaseMode)
{
   // Make sure this mode is valid.
   if(releaseMode<0 || releaseMode>=cReleaseModeCount)
   {
      BASSERT(0);
      releaseMode=cReleaseNoChange;
   }

   mReleaseMode=releaseMode;
}


//==============================================================================
// eof: resourcecache.cpp
//==============================================================================
