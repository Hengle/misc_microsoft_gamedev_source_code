//============================================================================
//
// File: queue.h
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once

//-----------------------------------------------------------------------------
// class BByteQueue
//-----------------------------------------------------------------------------
class BByteQueue
{
public:
   BByteQueue(uint maxEntries = 0) : 
      mHead(0), 
      mTail(0), 
      mNum(0), 
      mMaxEntries(maxEntries),
      mpBuf(NULL)
   { 
      BDEBUG_ASSERT((mMaxEntries == 0) || Math::IsPow2(mMaxEntries)); 

      if (mMaxEntries)
         mpBuf = new BYTE[mMaxEntries];
   }

   ~BByteQueue()
   {
      delete[] mpBuf;
   }


   uint getMaxSize(void) const { return mMaxEntries; }

   // The number of bytes in the buffer.
   uint getSize(void) const 
   { 
      return mNum;
   }

   // The amount of unused space remaining in the buffer.
   uint getAvail(void) const 
   { 
      return avail();
   }

   bool getEmpty(void) const 
   { 
      return empty();
   }

   bool getFull(void) const 
   {  
      return full();
   }

   void clear() 
   {
      mHead = mTail = mNum = 0; 
   }

   bool resize(uint newMaxEntries)
   {
      if (newMaxEntries == mMaxEntries)
         return true;

      if (mNum > newMaxEntries)         
         return false;
         
      const uint origNum = mNum;
      
      BYTE* pBuf = NULL;
      if (newMaxEntries)
      {
         pBuf = new BYTE[newMaxEntries];

         if (mNum)
            popFront(mNum, pBuf);
      }

      delete[] mpBuf;
               
      mpBuf = pBuf;
      
      mNum = origNum;
      mMaxEntries = newMaxEntries;
      mHead = mNum;
      mTail = 0;
      
      return true;
   }
   
   const BYTE& peekFront(uint index) const  { BDEBUG_ASSERT(index < getSize()); return mpBuf[wrap(mTail + index)]; }
         BYTE  peekFront(uint index)        { BDEBUG_ASSERT(index < getSize()); return mpBuf[wrap(mTail + index)]; }
    
   // false on failure
   bool popFront(void)
   {
      bool success = false;

      if (!empty())
      {
         mTail = nextWrap(mTail);
         mNum--;

         success = true;
      }

      return success;
   }

   // false on failure
   bool popFront(BYTE& obj)
   {
      bool success = false;

      if (!empty())
      {
         obj = mpBuf[mTail];
         mTail = nextWrap(mTail);
         mNum--;

         success = true;
      }

      return success;
   }

   // Returns number of objects popped.
   uint popFront(uint maxObjects, BYTE* pDstObjects)
   {
      const uint numObjectsToCopy = Math::Min<uint>(mNum, maxObjects);

      uint bytesRemaining = numObjectsToCopy;
      while (bytesRemaining)
      {
         uint bytesToCopy = Math::Min(bytesRemaining, mMaxEntries - mTail);
         memcpy(pDstObjects, mpBuf + mTail, bytesToCopy);
         
         pDstObjects    += bytesToCopy;
         bytesRemaining -= bytesToCopy;
         mTail          = wrap(mTail + bytesToCopy);
      }

      mNum -= numObjectsToCopy;

      return numObjectsToCopy;
   }

   // false on failure
   bool pushBack(const BYTE& a)
   {
      bool success = false;

      if (!full())
      {
         at(mHead) = a;
         mHead = nextWrap(mHead);
         mNum++;

         success = true;
      }

      return success;
   }

   // false on failure
   bool pushBack(uint numObjects, const BYTE* pSrcObjects)
   {
      if (!numObjects)
         return true;

      bool success = false;

      if (numObjects <= avail())
      {
         uint bytesRemaining = numObjects;
         while (bytesRemaining)
         {
            uint bytesToCopy = Math::Min(bytesRemaining, mMaxEntries - mHead);
            memcpy(mpBuf + mHead, pSrcObjects, bytesToCopy);

            pSrcObjects    += bytesToCopy;
            bytesRemaining -= bytesToCopy;
            mHead          = wrap(mHead + bytesToCopy);
         }         

         mNum += numObjects;

         success = true;
      }

      return success;
   }

protected:
   uint mHead;
   uint mTail; 
   uint mNum;
   uint mMaxEntries;
   BYTE* mpBuf;

   const BYTE& at (uint i) const  { return mpBuf[debugRangeCheck<uint>(i, mMaxEntries)]; }
         BYTE& at (uint i)        { return mpBuf[debugRangeCheck<uint>(i, mMaxEntries)]; }

   uint wrap(uint i) const       { return i & (mMaxEntries - 1); }
   uint nextWrap(uint i) const   { return (i + 1) & (mMaxEntries - 1); }
   uint prevWrap(uint i) const   { return (i - 1) & (mMaxEntries - 1); }

   bool empty(void) const { return mNum == 0; }
   bool full(void) const { return mNum == mMaxEntries; }
   uint avail(void) const { const uint res = mMaxEntries - mNum; return res; }   
};
