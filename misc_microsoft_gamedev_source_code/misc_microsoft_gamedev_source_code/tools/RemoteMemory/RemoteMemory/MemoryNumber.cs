using System;
using System.Collections.Generic;
using System.Text;

namespace RemoteMemory
{
   class MemoryNumber
   {
      public static string convert( uint mNumBytes)
      {
         uint kb = 1024;
         uint mb = kb*kb;

        
         //megabytes
         if (mNumBytes > mb)
         {
            float mbBytes = ((uint) (mNumBytes / (float)mb*1000) / 1000.0f);
            return mbBytes.ToString() + "mb";
         }

         //kilobytes
         if(mNumBytes > kb)
         {
            float kbBytes = (((uint)(mNumBytes / (float)kb) * 1000) / 1000.0f);
            return kbBytes.ToString() + "kb";
         }

         return mNumBytes.ToString();
      }
   }
}
