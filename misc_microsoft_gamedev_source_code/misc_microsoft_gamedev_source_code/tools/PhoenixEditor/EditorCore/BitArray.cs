using System;
using System.Collections.Generic;
using System.Text;

namespace EditorCore
{
   /// <summary>
   /// This creates an array of N intX values, whos individual bits can be quried and modified
   /// </summary>
   /// <remarks>
   /// If you want a different block size, duplicate this class and handle the specifics internally
   /// This is a good idea if you know your internal blocksize mimmics some specific POW2
   ///
   /// </remarks>
   /// 
   /// 
   
   public sealed class BitArray64
   {
      private int mNumElements = 0;
      private int mNumBlocks = 0;
      private ulong[] mBitArrayBlocks = null;

      //constants used for shifting
      private const int cPrimShift     = 6;
      private const int cNumBits       = 64;
      private const int cSecAnd        = cNumBits - 1;
      private const ulong cTopMask = unchecked((ulong)0x8000000000000000);
      private const ulong cMaxPos = unchecked((ulong)0x7FFFFFFFFFFFFFFF);
      private const ulong cMinPos = unchecked((ulong)0xFFFFFFFFFFFFFFFE);
      private const ulong cFull = unchecked((ulong)0xFFFFFFFFFFFFFFFF);
      private const ulong cEmpty = unchecked((ulong)0x0000000000000000);

      public BitArray64(int numElements)
      {
         mNumElements = numElements;
         mNumBlocks = mNumElements >> cPrimShift; // X / cNumBits
         if (mNumBlocks * cNumBits < mNumElements)
            mNumBlocks++;

         mBitArrayBlocks = new ulong[mNumBlocks];
         setAll(false);
      }
      ~BitArray64()
      {
         destroy();
      }
      public void destroy()
      {
         mBitArrayBlocks = null;
      }

      public void setValue(uint index, bool state)
      {
         if (index < 0 || index >= mNumElements)
            return;

         uint sIndex = (index & cSecAnd);
         uint pIndex = index >> cPrimShift;
         setContainerState(ref mBitArrayBlocks[pIndex],state,(cTopMask>>(int)sIndex));
      }
      //--------------------------------
      public void  setSpanState(int index, int stride, bool state)
      {
         if (index + stride >= mNumElements)
            stride = (mNumElements - 1) - index;

         int pIndexStart = index >> cPrimShift;
         int pIndexEnd = (index + stride) >> cPrimShift;

         int sIndexStart = (index & cSecAnd);
         int sIndexEnd = ((index+stride) & cSecAnd);
         
         if(pIndexEnd == pIndexStart)
         {
            ulong msk = makeSpanBitMask((byte)sIndexStart, (byte)sIndexEnd);
            setContainerState(ref mBitArrayBlocks[pIndexStart], state, msk);
         }
         else
         {

            //handle the edge cases first
            ulong msk = makeSpanBitMask((byte)sIndexStart, (byte)cNumBits);
            setContainerState(ref mBitArrayBlocks[pIndexStart], state, msk);

            ulong istate = state ? cFull : cEmpty;
            for(int i=pIndexStart+1;i<pIndexEnd;i++)
            {
               mBitArrayBlocks[i] = istate;
            }


            // end case
            msk = makeSpanBitMask(0, (byte)sIndexEnd);
            setContainerState(ref mBitArrayBlocks[pIndexEnd], state, msk);
         }
      }
      //--------------------------------
      public bool isSet(int index)
      {
         if (index < 0 || index >= mNumElements)
            return false;

         int pIndex = index >> cPrimShift;
         int sIndex = (index & cSecAnd);

         return (mBitArrayBlocks[pIndex] & (cTopMask >> sIndex)) != 0;
      }
      //--------------------------------
      public void setAll(bool onOff)
      {
         ulong iOnOff = onOff ? cFull : cEmpty;
         for (int i = 0; i < mNumBlocks; i++)
            mBitArrayBlocks[i] = iOnOff;
      }

      //--------------------------------
      private void setContainerState(ref ulong val, bool state, ulong mask)
      {
         ulong istate = state ? cFull : cEmpty;
         val ^= (istate ^ val) & mask;
      }
      //--------------------------------
      private ulong makeSpanBitMask(byte startIndex, byte endIndex)
      {

         byte l = (byte)(endIndex - startIndex);
         byte es = (byte)(cNumBits - endIndex);

         return (cFull >> (cNumBits - l)) << es;
      }


      //--------------------------------
      public void copyTo(BitArray64 arr)
      {
         if (arr.mNumElements != mNumElements || mNumBlocks != arr.mNumBlocks)
            throw new Exception("Error, cannot copy BitArray64 to target: Target does not have the same number of elements");

         for (int i = 0; i < mNumBlocks; i++)
            arr.mBitArrayBlocks[i] = mBitArrayBlocks[i];
      }

   }
}
