using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   class AllocStats
   {


      static List<HeapAlloc> mRegisteredHeaps = new List<HeapAlloc>();

      #region HEAP REGION

      //============================================================================
      // registerHeap
      //============================================================================
      static public void registerHeap(HaloWarsMem.BALPacketRegisterHeap pkt)
      {
         HeapAlloc pHeap = getHeapFromBasePtr(pkt.mPtr);
         if (pHeap != null)
         {
            //ALREADY HAVE THIS HEAP!!
            GlobalErrors.addError("Multiple allocations of heap 0x" + pkt.mPtr.ToString("x"));
            return;
         }

         HeapAlloc hm = new HeapAlloc(pkt.mPtr, pkt.mFlags, pkt.mName);
         mRegisteredHeaps.Add(hm);
      }

      //============================================================================
      // getHeapFromBasePtr
      //============================================================================
      static public HeapAlloc getHeapFromBasePtr(uint basePtr)
      {
         for (int i = 0; i < mRegisteredHeaps.Count; i++)
         {
            if (mRegisteredHeaps[i].getMemPtr() == basePtr)
               return mRegisteredHeaps[i];
         }
         return null;
      }

      //============================================================================
      // getHeapFromIndex
      //============================================================================
      static public HeapAlloc getHeapFromIndex(int index)
      {
         if (index < 0 || index >= mRegisteredHeaps.Count)
            return null;

         return mRegisteredHeaps[index];
      }

      //============================================================================
      // getNumHeaps
      //============================================================================
      static public int getNumHeaps()
      {
         return mRegisteredHeaps.Count;
      }

      //============================================================================
      // getNumHeaps
      //============================================================================
      static public void clearHeaps()
      {
         mRegisteredHeaps.Clear();
      }
      #endregion
   }
}
