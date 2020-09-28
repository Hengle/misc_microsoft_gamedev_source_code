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
   // HeapAlloc
   //=========================================
   public class HeapAlloc
   {
      uint mPtrToHeap = 0;
      int mFlags = 0;
      string mName;

      uint mNumNews = 0;
      uint mNumDeletes = 0;
      uint mNumResizes = 0;
      uint mMaxAllocatedBytes = 0;

      Hashtable mAllocations = new Hashtable(); // KEY is filename, VALUE is FileAlloc
      Hashtable mBlockToFile = new Hashtable(); // KEY is pBlock, VALUE is filename

      public Color ColorVal = GDIStatic.getNextDesiredColor();

      //=========================================
      // HeapAlloc
      //=========================================
      public HeapAlloc(uint PtrToHeap, int flags, string name)
      {
         mPtrToHeap = (PtrToHeap);
         mFlags = (flags);
         mName = (name);
      }

      //=========================================
      // allocMem
      //=========================================
      public void allocMem(uint pHeap, uint pBlock, uint blockSize, HaloWarsMem.BALContext context)
      {
         SymbolInfo.LookupInfo li = new SymbolInfo.LookupInfo();

         for (int i = 0; i < context.mBackTraceSize; i++)
         {
            HaloWarsMem.getSymbolInfo().lookup(context.mBackTrace[i], ref li);
            if (HaloWarsMem.getSymbolInfo().isIgnoreSymbol(Path.GetFileName(li.mFilename)))
               continue;

            if (mBlockToFile.Contains(pBlock))
            {
               GlobalErrors.addError("HeapAlloc : Multiple Allocations of block 0x" + pBlock.ToString("x") + " in heap 0x" + pHeap.ToString("x"));
               return;
            }

            FileAlloc fa = (FileAlloc)mAllocations[li.mFilename];
            if (fa == null)
            {
               fa = new FileAlloc(li.mFilename);
               mAllocations.Add(li.mFilename, fa);
            }
            
            mBlockToFile.Add(pBlock, li.mFilename);
            fa.allocMem(pHeap, pBlock, blockSize, (uint)li.mLine, context);
         

            break;
         }

         mNumNews++;
      }

      //==============================================================================
      // deleteMem
      //==============================================================================
      public void deleteMem(uint pBlock, HaloWarsMem.BALContext context)
      {
         mNumDeletes++;

         FileAlloc fa = getFileContaining(pBlock);
         mBlockToFile.Remove(pBlock);

         
         if (fa != null)
         {
            fa.deleteMem(pBlock);
            if (fa.getTotalAllocatedBytes(false) == 0)
            {
               mAllocations.Remove(fa.getFilename());
            }
            return;
         }

         GlobalErrors.addError("HeapAlloc : Stray delete of block 0x" + pBlock.ToString("x") + ". No file!");
      }

      //==============================================================================
      // resizeMem
      //==============================================================================
      public void resizeMem(uint pOrigBlock, uint NewSize, uint pNewBlock, HaloWarsMem.BALContext context)
      {
         deleteMem(pOrigBlock, context);
         allocMem(this.mPtrToHeap, pNewBlock, NewSize, context);
         mNumNews--;
         mNumDeletes--;
         mNumResizes++;
      }

      //==============================================================================
      // resizeMem
      //==============================================================================
      public uint getTotalAllocatedBytes()
      {
         uint totalSize = 0;

         IDictionaryEnumerator _enumerator = mAllocations.GetEnumerator();
         while (_enumerator.MoveNext())
         {
            totalSize += ((FileAlloc)_enumerator.Value).getTotalAllocatedBytes(false);
         }

         if (totalSize > mMaxAllocatedBytes)
            mMaxAllocatedBytes = totalSize;

         return totalSize;
      }

      //==============================================================================
      // getTotalNumAllocations
      //==============================================================================
      public uint getTotalNumAllocations()
      {
         return (uint)mBlockToFile.Count;
      }

      //==============================================================================
      // getMaxAllocatedBytes
      //==============================================================================
      public uint getMaxAllocatedBytes()
      {
         return (uint)mMaxAllocatedBytes;
      }

      //==============================================================================
      // getMemPtr
      //==============================================================================
      public uint getMemPtr()
      {
         return mPtrToHeap;
      }

      //==============================================================================
      // getNumNews
      //==============================================================================
      public uint getNumNews()
      {
         return mNumNews;
      }

      //==============================================================================
      // getNumDeletes
      //==============================================================================
      public uint getNumDeletes()
      {
         return mNumDeletes;
      }

      //==============================================================================
      // getNumResizes
      //==============================================================================
      public uint getNumResizes()
      {
         return mNumResizes;
      }

      //==============================================================================
      // getFileAllocations
      //==============================================================================
      public Hashtable getFileAllocations()
      {
         return mAllocations;
      }

      //==============================================================================
      // getFileContaining
      //==============================================================================
      public FileAlloc getFileContaining(uint pBlock)
      {
         string fname = (string)mBlockToFile[pBlock];
         if (fname == null)
            return null;

         return (FileAlloc)mAllocations[fname];

      }

      //==============================================================================
      // getBlockSize
      //==============================================================================
      public uint getBlockSize(uint pBlock)
      {
         FileAlloc fa = getFileContaining(pBlock);
         if (fa != null)
            return fa.getBlockSize(pBlock);

         return 0;
      }

      //==============================================================================
      // getName
      //==============================================================================
      public string getName()
      {
         return mName;
      }
   }
}