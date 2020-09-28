//=============================================================================
//
//  File: rockallHeapStats.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//=============================================================================
#pragma once

#include "../extlib/rockall/code/rockall/interface/rockallfrontend.hpp"

#pragma warning(disable:4509) //  nonstandard extension used: 'BRockallHeapStats::update' uses SEH and 'pageBitmap' has destructor
//=============================================================================
// class BRockallHeapStats
//=============================================================================
class BRockallHeapStats
{
public:
   BRockallHeapStats()
   {
      clear();
   }
   
   BRockallHeapStats(ROCKALL_FRONT_END* pHeap, bool lockRockallHeap, bool computeUtilization)
   {
      update(pHeap, lockRockallHeap, computeUtilization);
   }

   bool update(ROCKALL_FRONT_END* pHeap, bool lockRockallHeap, bool computeUtilization)
   {
      clear();
      
      if (lockRockallHeap)
         pHeap->LockAll();
         
      uint byteHist[256];
      Utils::ClearObj(byteHist);
      
      BDynamicArray<BYTE> pageBitmap(2U * 65536U);

      __try
      {
         bool activeFlag = false;
         void* pAddress = NULL;
         int space = 0;

         for ( ; ; )
         {
            const bool moreFlag = pHeap->Walk(&activeFlag, &pAddress, &space);
            if (!moreFlag)
            {
               break;
            }
            
            const uint firstPageIndex = (DWORD)pAddress >> 12U;
            const uint lastPageIndex = ((DWORD)pAddress + space - 1) >> 12U;
            for (uint p = firstPageIndex; p <= lastPageIndex; p++)
            {
               const uint pageBitmapOfs = p >> 3U;
               if (pageBitmapOfs < pageBitmap.getSize())
               {
                  const uint pageBitmapMask = 1U << (p & 7U);
                  if ((pageBitmap[pageBitmapOfs] & pageBitmapMask) == 0)
                  {
                     mTotalPages++;
                     pageBitmap[pageBitmapOfs] |= pageBitmapMask;
                  };
               }               
            }               

            if (!activeFlag)
            {
               mNumFreeBlocks++;
               mTotalFreeBytes += space;
               mMaxFreeBlockBytes = max(mMaxFreeBlockBytes, (uint)space);
            }
            else
            {
               mNumUsedBlocks++;
               mTotalUsedBytes += space;
               mMaxUsedBlockBytes = max(mMaxUsedBlockBytes, (uint)space);
               
               if (computeUtilization)
               {
                  __try
                  {
                     for (int i = 0; i < space; i++)
                     {
                        uchar c = ((uchar*)pAddress)[i];
                        byteHist[c]++;
                     }
                  }
                  __except(EXCEPTION_EXECUTE_HANDLER)
                  {
                  }
               }
            }

            mpLowestAddress = min(mpLowestAddress, pAddress);
            mpHighestAddress = max(mpHighestAddress, pAddress);
         }
      }         
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         mFailed = true;

         if (lockRockallHeap)
            pHeap->UnlockAll();

         return false;
      }         

      if (lockRockallHeap)
         pHeap->UnlockAll();
         
      if (computeUtilization)
      {
         mTotalUsedBlockZeroBytes = byteHist[0];
         
         mAveBitsUtilizedPerByte = 0;
         if (mTotalUsedBytes)
         {
            double invLog2 = 1.0f / log(2.0f);
            double invTotalUsedBytes = 1.0f / mTotalUsedBytes;
            for (uint i = 0; i < 256; i++)
            {
               if (byteHist[i])
                  mAveBitsUtilizedPerByte += -log(byteHist[i] * invTotalUsedBytes) * invLog2 * byteHist[i];
            }
            
            mAveBitsUtilizedPerByte *= invTotalUsedBytes;
         }         
      }         

      mFailed = false;

      return true;
   }

   void clear(void)
   {
      mNumFreeBlocks = 0;
      mTotalFreeBytes = 0;

      mNumUsedBlocks = 0;
      mTotalUsedBytes = 0;

      mMaxFreeBlockBytes = 0;
      mMaxUsedBlockBytes = 0;

      mpLowestAddress = (void*)0xFFFFFFFF;
      mpHighestAddress = (void*)0;
      
      mTotalUsedBlockZeroBytes = 0;
      
      mTotalPages = 0;

      mFailed = false;
   }

   uint mNumFreeBlocks;
   uint mTotalFreeBytes;

   uint mNumUsedBlocks;
   uint mTotalUsedBytes;

   uint mMaxFreeBlockBytes;
   uint mMaxUsedBlockBytes;
   
   uint mTotalPages;

   void* mpLowestAddress;
   void* mpHighestAddress;
   
   uint mTotalUsedBlockZeroBytes;
   double mAveBitsUtilizedPerByte;

   bool mFailed;
};

#pragma warning(default:4509)