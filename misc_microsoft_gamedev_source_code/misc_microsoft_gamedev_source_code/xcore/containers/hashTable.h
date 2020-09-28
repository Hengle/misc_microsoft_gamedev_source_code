//==============================================================================
// hashtable.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#pragma once

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include "hash\hash.h"

//typedef void* bhandle;

// This class wraps a void * to prevent c++ from "helpfully" casting things to bhandles
// unintentionally (like char *, etc.).  This makes for more awkward looking casts within
// the template code, but doesn't affect outside usage.
class bhandle
{
   public:
      // Force the cast from void * to bhandle to be explicit.
      explicit                bhandle(void *val) {mValue=val;}

      // Freely let bhandles be converted to void*
                              operator void*() {return(mValue);}

   protected:
      void                    *mValue;
};


//==============================================================================
// Used for storing iteration state
//==============================================================================
class BHashTableIter
{
   public:
                              BHashTableIter() : mBucketIndex(0), mEntry(NULL) {}
      
      long                    mBucketIndex;
      bhandle                 mEntry;
};



template <class ValueType, class KeyType, DWORD HashSize=256, HASH_FUNCTION HashFn=hash, bool useMalloc=false>
class BHashTable
{
   public:
      BHashTable() :
         mHashMask(0x7FFFFFFF),
         mCount(0)
      {
         // Find left-most one bit.
         // Also, generate the appropriate hash mask (this will zero out bits that would
         // could make a value higher than the hash size).  For example, the mask for 
         // hash size 256 would be 0xFF.
         DWORD bitMask=0x80000000;
         for(long i=0; i<32; i++, bitMask>>=1, mHashMask>>=1)
         {
            if(HashSize&bitMask)
               break;   
         }

         // Check that we had a valid power-of-two passed in for hash size.
         if(bitMask==0 || HashSize&(~bitMask))
            BFAIL("HashSize must be power-of-two!  (hash table entries will be wasted)");

         // Must have a valid hash function to avoid crash.
         BASSERTM(HashFn, "A hash function must be specified.");

         // Clear hash list.
         memset(mHashList, 0, sizeof(mHashList));
      }
      
      ~BHashTable()
      {
         removeAll();
      }


      bhandle add(const KeyType &key, const ValueType &value)
      {
         // Allocate a new node.
         BHashEntry *newNode=allocateHashEntry();
         if(!newNode)
         {
            BFAIL("BHashTable::add -- failed to allocate new node.");
            return(bhandle(NULL));
         }

         // Get hash for this key.
         DWORD hash=hashKey(key);

         // Get first entry in the list.
         BHashEntry *root=&mHashList[hash];
        
         // Fill it in.
         newNode->mKey=key;
         newNode->mValue=value;
         newNode->mNext=root->mNext;
         newNode->mPrev=root;
         if(root->mNext)
            root->mNext->mPrev=newNode;

         // Insert this node at the front of the list.
         mHashList[hash].mNext=newNode;

         // Hand back a handle to the node we added.
         return(bhandle(newNode));
      }

      bhandle remove(bhandle handle)
      {
         // Grab the node.
         BHashEntry *entry=(BHashEntry*)(void*)(handle);

         // Grab next pointer.
         BHashEntry *next=entry->mNext;

         // Unhook node from the list.
         entry->mPrev->mNext=entry->mNext;
         if(next)
            next->mPrev=entry->mPrev;

         // Free the entry.
         releaseHashEntry(entry);

         // Return the next node in the list to simplify removing while walking.
         return(bhandle(next));
      }

      // Removes all entries with a given key AND value.
      void removeByKeyAndValue(const KeyType &key, const ValueType &value)
      {
         bhandle handle=find(key);
         while(handle)
         {
            // Get the value of this entry.
            const ValueType &currValue=get(handle);

            // If it matches the value we're looking for, remove it.
            // Otherwise, just search forward for other key matches.
            if(currValue==value)
            {
               // Remove from the list.
               handle=remove(handle);
            }
            else
            {
               // Grab the next key match.
               // TODO: unroll this to avoid re-hashing the key.
               handle=find(key, handle);
            }
         }
      }

      // Removes all entries matching a specific key.
      void removeByKey(const KeyType &key)
      {
         // Keep calling find with this key and remove each.
         // TODO: unroll this to avoid rehashing the key
         bhandle handle=find(key);
         while(handle)
            handle=remove(handle);
      }

      void removeAll(void)
      {
         // TODO: could speed this up by customizing to not bother fixing list's link pointers 
         // since we're removing everything.

         // Go through each hash value.
         for(long i=0; i<HashSize; i++)
         {
            // Walk the list removing everything.
            BHashEntry *entry=mHashList[i].mNext;
            while(entry)
               entry=(BHashEntry*)(void*)remove(bhandle(entry));
         }
      }

      const ValueType &get(bhandle handle) const
      {
         BHashEntry *entry=(BHashEntry*)(void*)handle;
         return(entry->mValue);
      }

      bhandle find(const KeyType &key, bhandle previousHandle=bhandle(NULL)) const
      {
         // Find a place to start looking.  If we have a previous handle passed in,
         // we simple walk the list from there.  Otherwise we grab the beginning of the list.
         BHashEntry *entry;
         if(previousHandle)
            entry=((BHashEntry*)(void*)previousHandle)->mNext;
         else
         {
            // Get hash for this key.
            DWORD hash=hashKey(key);

            // Get root entry for this list.
            entry=mHashList[hash].mNext;
         }

         // Iterate through the list looking for the key requested.
         while(entry)
         {
            // Did we match?  If so, hand it back.
            if(entry->mKey == key)
               return(bhandle(entry));

            // Next one.
            entry=entry->mNext;
         }

         // Sorry, not found.
         return(bhandle(NULL));
      }

      long getCount(void) const
      {
         return(mCount);
      }

      long getCount(const KeyType &key) const
      {
         // Nothing so far...
         long count=0;

         bhandle handle=find(key);
         while(handle)
         {
            // Inc count.
            count++;

            // Look for the next one.
            handle=find(key, handle);
         };

         return(count);
      }

      bool isEmpty(void) const
      {
         return(getCount() == 0);
      }

      bhandle iterStart(BHashTableIter &iter)
      {
         iter.mBucketIndex = 0;
         iter.mEntry = bhandle(NULL);
         return(iterNext(iter));
      }
      
      bhandle iterNext(BHashTableIter &iter)
      {
         BHashEntry *entry = ((BHashEntry*)(void*)iter.mEntry);
         if(iter.mEntry)
         {
            // Get next.
            entry = entry->mNext;
            if(entry)
            {
               // Update iterator.
               bhandle result(entry);
               iter.mEntry = result;
               
               // Hand back result.
               return(result);
            }
            else
            {
               // We hit the end of the list for this bucket, so time for a new one.
               iter.mBucketIndex++;
            }
         }
         
         // Find the next bucket with something interesting
         for(; iter.mBucketIndex<HashSize; iter.mBucketIndex++)
         {
            // Is there something in this list.
            if(mHashList[iter.mBucketIndex].mNext)
            {
               // Update iterator.
               bhandle result(mHashList[iter.mBucketIndex].mNext);
               iter.mEntry = result;
               
               // Hand back result.
               return(result);
            }
         }
         
         // If we got here, nothing left.
         return(bhandle(NULL));
      }


      void update(bhandle handle, const ValueType &value)
      {
         // Grab the node.
         BHashEntry *entry=(BHashEntry*)(void*)(handle);

         // Set the value
         entry->mValue = value;
      }

      void updateByKey(const KeyType &key, const ValueType &value)
      {
         bhandle handle=find(key);
         if(handle)
            update(handle, value);
      }

   protected:
      class BHashEntry
      {
         public:
            KeyType           mKey;
            ValueType         mValue;

            BHashEntry        *mNext;
            BHashEntry        *mPrev;
      };

      DWORD                   hashKey(const KeyType &key) const
      {
         DWORD hash=HashFn((const unsigned char *)&key, sizeof(KeyType), 0);
         hash&=mHashMask;
         return(hash);
      }
      BHashEntry              *allocateHashEntry(void)
      {
         // Update active node count.
         mCount++;
         
         // Stub for now, good old-fashioned new.
         if(useMalloc)
            return((BHashEntry*)malloc(sizeof(BHashEntry)));
         else
            return(new BHashEntry);
      }
      void                    releaseHashEntry(BHashEntry *hashEntry)
      {
         // Update active node count.
         mCount--;

         // Stub for now, good old-fashioned delete.
         if(useMalloc)
            free(hashEntry);
         else
            delete hashEntry;
      }

      BHashEntry              mHashList[HashSize];
      DWORD                   mHashMask;
      long                    mCount;
};

#endif 

//==============================================================================
// eof: hashtable.h
//==============================================================================


