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
   // FileAlloc
   //=========================================
   public class FileAlloc
   {
      Hashtable mAllocations = new Hashtable(); // KEY is lineNum, VALUE is LineAlloc
      Hashtable mBlockToLineNum = new Hashtable(); // KEY is pBlock, VALUE is lineNum
      string mFilename = "?";
      uint mTotalBytes = 0;
      //=========================================
      // FileAlloc
      //=========================================
      public FileAlloc(string filename)
      {
         mFilename = filename;
      }
      //=========================================
      // allocMem
      //=========================================
      public void allocMem(uint pHeap, uint pBlock, uint blockSize, uint lineNum, HaloWarsMem.BALContext context)
      {

         if (mBlockToLineNum.Contains(pBlock))
         {
            GlobalErrors.addError("FileAlloc : Multiple Allocations of block 0x" + pBlock.ToString("x") + " in heap 0x" + pHeap.ToString("x"));
            return;
         }

         LineAlloc la = (LineAlloc)mAllocations[lineNum];

         if (la == null)
         {
            la = new LineAlloc(lineNum);
            mAllocations.Add(lineNum, la);
         }

         la.allocMem(pHeap, pBlock, blockSize, context);

         mBlockToLineNum.Add(pBlock, lineNum);

         mTotalBytes += blockSize;
      }

      //==============================================================================
      // deleteMem
      //==============================================================================
      public void deleteMem(uint pBlock)
      {
         LineAlloc la = getLineContaining(pBlock);
         mBlockToLineNum.Remove(pBlock);

         if (la != null)
         {
            mTotalBytes -= la.getBlockSize(pBlock);
            la.deleteMem(pBlock);

            if (la.getTotalAllocatedBytes() == 0)
            {
               mAllocations.Remove(la);
            }
            return;
         }

         GlobalErrors.addError("FileAlloc : Stray Delete of block 0x" + pBlock.ToString("x"));


      }

      //==============================================================================
      // resizeMem
      //==============================================================================
      public void resizeMem(uint pHeap, uint pOrigBlock, uint NewSize, uint pNewBlock, uint lineNum, HaloWarsMem.BALContext context)
      {
         deleteMem(pOrigBlock);
         allocMem(pHeap, pNewBlock, NewSize, lineNum, context);
      }

      //==============================================================================
      // getTotalAllocatedBytes
      //==============================================================================
      public uint getTotalAllocatedBytes(bool inclusive)
      {
         return mTotalBytes;
         //uint totalSize = 0;

         //IDictionaryEnumerator _enumerator = mAllocations.GetEnumerator();
         //while (_enumerator.MoveNext())
         //{
         //   totalSize += ((LineAlloc)_enumerator.Value).getTotalAllocatedBytes();
         //}

         //return totalSize;
      }

      //==============================================================================
      // getTotalNumAllocations
      //==============================================================================
      public uint getTotalNumAllocations()
      {
         uint totalSize = 0;

         IDictionaryEnumerator _enumerator = mAllocations.GetEnumerator();
         while (_enumerator.MoveNext())
         {
            totalSize += ((LineAlloc)_enumerator.Value).getTotalNumAllocations();
         }

         return totalSize;
      }

      //==============================================================================
      // getLineContaining
      //==============================================================================
      public LineAlloc getLineContaining(uint pBlock)
      {
         if (!mBlockToLineNum.Contains(pBlock))
            return null;

         uint lineNum = (uint)mBlockToLineNum[pBlock];
         LineAlloc la = (LineAlloc)mAllocations[lineNum];

         return la;
      }

      //==============================================================================
      // containsBlock
      //==============================================================================
      public bool conainsBlock(uint pBlock)
      {
         return mBlockToLineNum.Contains(pBlock);
      }

      //==============================================================================
      // getBlockSize
      //==============================================================================
      public uint getBlockSize(uint pBlock)
      {
         LineAlloc la = getLineContaining(pBlock);
         if (la == null)
            return 0;

         return la.getBlockSize(pBlock);
      }

      //==============================================================================
      // getFilename
      //==============================================================================
      public string getFilename()
      {
         return mFilename;
      }

      //==============================================================================
      // getLineAllocations
      //==============================================================================
      public Hashtable getLineAllocations()
      {
         return mAllocations;
      }
   }





}
