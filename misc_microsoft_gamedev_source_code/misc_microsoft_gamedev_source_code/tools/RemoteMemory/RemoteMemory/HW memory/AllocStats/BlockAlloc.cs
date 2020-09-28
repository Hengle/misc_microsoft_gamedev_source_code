using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   //=========================================
   // BlockAlloc
   //=========================================
   public class BlockAlloc
   {
      public uint mpHeap;
      public uint mpBlock;
      public uint mBlockSize;

      public HaloWarsMem.BALContext mContext = new HaloWarsMem.BALContext();
   }

}
