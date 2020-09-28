using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.Drawing;
using System.Windows.Forms;
using System.IO;

namespace RemoteMemory
{


   //=========================================
   // LineAlloc
   //=========================================
   public class LineAlloc
   {
      uint mLinenum = 0;
      uint mTotalBytes = 0;
      Hashtable mAllocations = new Hashtable(); // KEY is blockPointer, VALUE is BlockAlloc

      public LineAlloc(uint lineNum)
      {
         mLinenum = lineNum;
      }
      //=========================================
      // allocMem
      //=========================================
      public void allocMem(uint pHeap, uint pBlock, uint blockSize, HaloWarsMem.BALContext context)
      {
         if (mAllocations.Contains(pBlock))
         {
            GlobalErrors.addError(" LineAlloc : Multiple Allocations of block 0x" + pBlock.ToString("x") + " in heap 0x" + pHeap.ToString("x"));
            return;
         }


         BlockAlloc ba = new BlockAlloc();
         ba.mpHeap = pHeap;
         ba.mpBlock = pBlock;
         ba.mBlockSize = blockSize;
         context.copyTo(ref ba.mContext);

         mAllocations.Add(pBlock, ba);

         mTotalBytes += blockSize;
      }

      //==============================================================================
      // deleteMem
      //==============================================================================
      public void deleteMem(uint pBlock)
      {
         if (!mAllocations.Contains(pBlock))
         {
            GlobalErrors.addError(" LineAlloc : Stray Delete of block 0x" + pBlock.ToString("x"));
            return;
         }

         mTotalBytes -= ((BlockAlloc)mAllocations[pBlock]).mBlockSize;

         mAllocations.Remove(pBlock);

      }

      //==============================================================================
      // resizeMem
      //==============================================================================
      public void resizeMem(uint pHeap, uint pOrigBlock, uint NewSize, uint pNewBlock, HaloWarsMem.BALContext context)
      {
         deleteMem(pOrigBlock);
         allocMem(pHeap, pNewBlock, NewSize, context);
      }

      //==============================================================================
      // containsBlock
      //==============================================================================
      public bool containsBlock(uint pBlock)
      {
         return mAllocations.Contains(pBlock);
      }

      //==============================================================================
      // getBlockSize
      //==============================================================================
      public uint getBlockSize(uint pBlock)
      {
         if (!containsBlock(pBlock))
            return 0;

         return ((BlockAlloc)mAllocations[pBlock]).mBlockSize;

      }

      //==============================================================================
      // getBlockContaining
      //==============================================================================
      public BlockAlloc getBlockContaining(uint pBlock)
      {
         BlockAlloc ba = (BlockAlloc)mAllocations[pBlock];

         return ba;
      }

      //==============================================================================
      // getTotalAllocatedBytes
      //==============================================================================
      public uint getTotalAllocatedBytes()
      {
         return mTotalBytes;
      }

      //==============================================================================
      // getTotalNumAllocations
      //==============================================================================
      public uint getTotalNumAllocations()
      {
         return (uint)mAllocations.Count;
      }

      //==============================================================================
      // getBlockAllocations
      //==============================================================================
      public Hashtable getBlockAllocations()
      {
         return mAllocations;
      }

      //==============================================================================
      // getLinenum
      //==============================================================================
      public uint getLinenum()
      {
         return mLinenum;
      }
   }
}