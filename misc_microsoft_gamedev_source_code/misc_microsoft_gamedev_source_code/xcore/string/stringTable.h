//==============================================================================
// stringtable.h
//
// Copyright (c) 1999-2001, Ensemble Studios
//==============================================================================

#ifndef _STRINGTABLE_H_
#define _STRINGTABLE_H_

//==============================================================================
// 
//==============================================================================
#include "hash\hash.h"

//==============================================================================

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
// really just a struct
class BStringTableIter
{
   public:
      // state for iterating
      int         mIndex;
};

//==============================================================================
// A BStringTable is a simple mapping between strings and an arbitrary passed
// in data type.  Tag overloading is not allowed.  The string table manages all
// of its own memory, so all data is passed in by reference, with the exception
// of iterator state, which is managed by the client.
template <
      class Type, 
      bool caseSensitive=false, 
      DWORD hashSize=256, 
      HASH_FUNCTION=hash, 
      class STRING_TYPE=BSimString, 
      template<class, uint> class ARRAY_ALLOCATOR=BDynamicArraySimHeapAllocator
   >
class BStringTable
{
   public:
      // Types
      typedef BDynamicArray<STRING_TYPE, 4, ARRAY_ALLOCATOR> BTagsArrayType;
      typedef BDynamicArray<Type, 4, ARRAY_ALLOCATOR> BValueArrayType;
   
      // Return codes
      enum
      {
         cErrOK,
         cErrCollision,
         cErrNoMatch,
      };

      // Constructors
      BStringTable( ) :
         mTags(),
         mValues(),
         mHashFunction(hash),
         mHashMask(0x7FFFFFFF)
      {
         // Find left-most one bit.
         // Also, generate the appropriate hash mask (this will zero out bits that would
         // could make a value higher than the hash size).  For example, the mask for 
         // hash size 256 would be 0xFF.
         DWORD bitMask=0x80000000;
         for(long i=0; i<32; i++, bitMask>>=1, mHashMask>>=1)
         {
            if(hashSize&bitMask)
               break;
         }

         // Check that we had a valid power-of-two passed in for hash size.
         if(bitMask==0 || hashSize&(~bitMask))
            BASSERT(0);    // hashSize must be power-of-two!  (hash table entries will be wasted)

         // Must have a valid hash function to avoid crash.
         BASSERT(mHashFunction);
      }

      // Destructors
      ~BStringTable( void ) 
      { 
      };

      // Functions
      void                 copy(const BStringTable &orig)
      {
         clearAll();
         long i;
         for (i=0; i < orig.mTags.getNumber(); i++)
         {
            add(orig.mTags[i].getPtr(), orig.mValues[i]);
         }
      }

      // add a new tag-value pair to the table
      long                 add(const char *tag, const Type &val)
      {
         // Check for bad param
         if(!tag)
            return(-1);

         int loc, loc2;
       
         // first make sure we don't have this tag already sitting around
         DWORD hash=hashTag(tag);
         bool found=find(tag, hash, NULL);
         if(found)
            return(cErrCollision);

         // put it in the array
         loc=mTags.getNumber();
         mTags.setNumber(loc+1);
         mTags[loc].set(tag);

         // also track the value
         loc2 = mValues.add(val);
         BASSERT(loc == loc2);
         
         // add hash entry pointing to this string.
         mHashTable[hash].add(loc);
         
         
         // TEST TEST
         /*
         if(mHashTable[hash].getNumber() > 1)
         {
         trace("colliding strings:");
         for(long i=0; i<mHashTable[hash].getNumber(); i++)
         {
         trace("    '%s'", (const char *)mTags[mHashTable[hash][i]]);
         }
         }
         */
         // END TEST
         
         return(cErrOK);
      }
      
      //-- remove a mapping
      long remove(const char *tag)
      {
         // Check for bad param
         if(!tag)
            return(-1);

         // do we have this?
         DWORD hash=hashTag(tag);
         bool found=find(tag, hash, NULL);
         if(!found)
            return (-1);
         
         // find the index
         STRING_TYPE explicitCastString(tag);

         long index = mTags.find(explicitCastString);
         if (index == -1)
            return (-1);
         
         //--swap this one with the last one in the array
         //-- if this is the last one then don't swap
         long swap = mTags.getNumber() -1;
         if (swap < 0)  // this case can never be true.. but ok, we'll check
            return (-1);

         //-- store off the hash for the swapped field
         DWORD oldhash = hashTag(mTags[swap].getPtr());

              
         //-- remove the entry from the hash table
         long i;
         long count = mHashTable[hash].getNumber();
         for (i=0; i < count; i++)
         {
            const STRING_TYPE &str = mTags[ mHashTable[hash][i] ];
            if (str.compare(STRING_TYPE(tag)) == 0)
            {
               mHashTable[hash].removeIndex(i);
               break;
            }
         }

         //-- copy the last one to the index and kill the last one
         if (index != swap)
         {
            mTags[index] = mTags[swap];
            mValues[index] = mValues[swap];
         }

         mTags.removeIndex(swap);
         mValues.removeIndex(swap);

         //-- now fix up the hash table
         count = mHashTable[oldhash].getNumber();
         for (i = 0; i < count; i++)
         {
            long val = mHashTable[oldhash][i];
            if (val == swap)
            {
               mHashTable[oldhash][i] = index;
               break;
            }
         }


         return index;
         
      }

      // clear all entries
      void                 clearAll(void)
      {
         // clear out the arrays proper
         mTags.setNumber(0);
         mValues.setNumber(0);
         for(long i=0; i<hashSize; i++)
            mHashTable[i].setNumber(0);
      }

      // is the table empty?
      bool                 isEmpty(void) const
      {
         if (mTags.getNumber() > 0)
            return(false);
         else
            return(true);
      }

      // find the value matching a particular tag
      // returns whether it found it or not
      bool                 find(const char *tag, Type *pVal) const
      {
         // TEST TEST
         /*
         long maxEntries=0;
         long occupiedEntries=0;
         for(long i=0; i<hashSize; i++)
         {
            long num=mHashTable[i].getNumber();
            if(num>0)
               occupiedEntries++;
            if(num>maxEntries)
               maxEntries=num;
         }
         trace("********hash info -- occupied: %d    max: %d", occupiedEntries, maxEntries);
         */
         // END TEST

         // Check for bad param
         if(!tag)
            return(false);

         // Get hash value for string.
         DWORD hash=hashTag(tag);
         
         // first make sure we don't have this tag already sitting around
         bool found=find(tag, hash, pVal);
         return(found);
      }

      // adds the tag if it doesn't exist.  updates if it does
      long update(const char *tag, const Type &val)
      {
         long dwret = add(tag, val);

         // if ok, it was added
         if (dwret == cErrOK)
            return dwret;

         // must already exist, find and update
         DWORD hash=hashTag(tag);

         update(tag, hash, val);

         return cErrOK;
      }

      // To use the iterator, you allocate a BStringTableIter, then
      // call iterStart on it.  Then you can use iterNext to get all
      // your elements.  Here's a sample.
      // 
      // BStringTable<int> table;
      // BStringTableIter  iter;
      // long              val;
      // 
      // table.iterStart(&iter); 
      // while (table.iterNext(&iter,&val))
      // {
      //    // code that uses val goes here
      // }

      // begin iteration
      void                 iterStart(BStringTableIter *pIter) const
      {
         if(!pIter)
         {
            BASSERT(0);
            return;
         }
         pIter->mIndex = 0;
      }

      // get next iterator value
      // returns whether or not it actually got anything
      bool                 iterNext(BStringTableIter *pIter, Type *pVal, STRING_TYPE *pTag = NULL) const
      {
         if(!pIter || pIter->mIndex<0)
         {
            BASSERT(0);
            return(false);
         }

         // if we are off the edge of the array, then that's all there is to do
         if (pIter->mIndex >= mTags.getNumber())
            return(false);

         // poke in the value
         if (pVal != NULL)
            *pVal = mValues[pIter->mIndex];
         if (pTag != NULL)
            *pTag = mTags[pIter->mIndex];

         // advance the iterator
         pIter->mIndex += 1;
         return(true);
      }

      // Do we want an iterated find, instead of just disallowing tag conflicts?

      // how many unique tags do we have registered?
      long                 numTags(void) const
      {
         return(mTags.getNumber());
      }

      // some debugging tools
      void                 dumpContents(void) const
      {
         BStringTableIter iter;
         Type val;
         STRING_TYPE tag;
         int count = 0;

         iterStart(&iter);
         while (iterNext(&iter,&val,&tag))
         {
            count++;
            trace("Entry %d: %s -> %d",count,(const char *)tag,val);
         }
      }

	  const BValueArrayType&         getValues(void) const {return mValues;}
     const BTagsArrayType&          getTags(void)   const {return mTags;  }

      const char *getTagAtIndex(long index) const
      {
         if(index<0 || index>=mTags.getNumber())
            return(NULL);
         return(mTags[index].getPtr());
      }

      // Variables

   protected:

      #pragma warning(disable:4702)
      // Functions
      DWORD                hashTag(const char *tag) const
      {
         // Check for bad param
         if(!tag)
            return(0);

         const unsigned char *key;
         long keyLen;
         char temp[256];

         // 27apr01 - ham - moved temp var out of local if scope to fix dangling memory reference

         #pragma warning(disable:4127)
         if(caseSensitive)
         {
            key=(const unsigned char *)tag;
            keyLen=strlen(tag);
         }
         else
         {
            // convert to uppercase.
            keyLen=0;
            while(tag[keyLen]!='\0' && keyLen<256)
            {
               //temp[keyLen]=(char)toupper(tag[keyLen]);
               const char cUpperDelta='a'-'A';
               if(tag[keyLen]>='a' && tag[keyLen]<='z')
                  temp[keyLen]=char(tag[keyLen]-cUpperDelta);
               else
                  temp[keyLen]=tag[keyLen];

               keyLen++;
            }
            key=(const unsigned char *)temp;
         }
         #pragma warning(default:4127)

         // Get hash value for string.
         DWORD hash=mHashFunction(key, keyLen, 0);
         hash&=mHashMask;
         return(hash);
      }
      #pragma warning(default:4702)

      #pragma warning(disable:4702)
      bool find(const char *tag, DWORD hashValue, Type *pVal) const
      {
         // Check for bad param
         if(!tag)
            return(false);

         // Get row for this hash entry.
         const BHashTableArrayType& hashRow=mHashTable[hashValue];

         // Loop through entries in case-sensitive or case-insensitive way.
         #pragma warning(disable:4127)
         if(caseSensitive)
         {
            for(long i=0; i<hashRow.getNumber(); i++)
            {
               if(strcmp(mTags[hashRow[i]].getPtr(), tag) == 0)
               {
                  if(pVal)
                     *pVal=mValues[hashRow[i]];
                  return(true);
               }
            }
         }
         else
         {
            for(long i=0; i<hashRow.getNumber(); i++)
            {
               if(stricmp(mTags[hashRow[i]].getPtr(), tag) == 0)
               {
                  if(pVal)
                     *pVal=mValues[hashRow[i]];
                  return(true);
               }
            }
         }
         #pragma warning(default:4127)

         return(false);
      }
      #pragma warning(default:4702)


      #pragma warning(disable:4702)
      bool update(const char *tag, DWORD hashValue, const Type val)
      {
         // Check for bad param
         if(!tag)
            return(false);

         // Get row for this hash entry.
         const BHashTableArrayType&hashRow=mHashTable[hashValue];

         // Loop through entries in case-sensitive or case-insensitive way.
         #pragma warning(disable:4127)
         if(caseSensitive)
         {
            for(long i=0; i<hashRow.getNumber(); i++)
            {
               if(strcmp(mTags[hashRow[i]].getPtr(), tag) == 0)
               {
                  mValues[hashRow[i]] = val;
                  return(true);
               }
            }
         }
         else
         {
            for(long i=0; i<hashRow.getNumber(); i++)
            {
               if(stricmp(mTags[hashRow[i]].getPtr(), tag) == 0)
               {
                  mValues[hashRow[i]] = val;
                  return(true);
               }
            }
         }
         #pragma warning(default:4127)

         return(false);
      }
      #pragma warning(default:4702)

      // Variables
                  
      BTagsArrayType          mTags;
      BValueArrayType         mValues;

      typedef BDynamicArray<long, 4, ARRAY_ALLOCATOR, BDynamicArraySmallOptions, BDynamicArraySinglePointer> BHashTableArrayType;
      BHashTableArrayType     mHashTable[hashSize];
      
      DWORD                   mHashMask;
      HASH_FUNCTION           mHashFunction;

}; // BStringTable


typedef BStringTable<long> BStringTableLong;

//==============================================================================
#endif // _STRINGTABLE_H_

//==============================================================================
// eof: stringtable.h
//==============================================================================


