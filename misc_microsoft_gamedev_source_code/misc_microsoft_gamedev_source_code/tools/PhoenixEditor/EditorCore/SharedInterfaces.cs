using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;

namespace EditorCore
{


   public interface IMask
   {
      float GetMaskWeight(long index);
      void SetMaskWeight(long index, float value);

      IMask Clone();
      void Set(IMask m);
      void Clear();

      void ResetIterator();
      bool MoveNext(out long index, out float value);

      bool HasData();
   }


   public interface IMaskPickerUI
   {
      void SetLastMask(IMask mask);
      void ClearMaskData();
      void AddMaskToList(IMask mask, string maskName);
      int GetNumMasks();
      List<string> GetMaskNames();
      IMask GetMask(string maskName);
   }

   public class MaskFactory
   {
      public static int mMaxCapacity = 2049 * 2049;
      public static IMask GetNewMask()
      {
         return new ArrayBasedMask(mMaxCapacity);
      }
   }


   public class ArrayBasedMask : JaggedContainer<float>, IMask
   {
      public ArrayBasedMask(int maxCapacity) : base(maxCapacity)
      {
      }
      public float GetMaskWeight(long index)
      {
         return this.GetValue(index);
      }
      public void SetMaskWeight(long index, float value)
      {
         this.SetValue(index,value);
      }
      public IMask Clone()
      {
         ArrayBasedMask newCopy = new ArrayBasedMask(mMaxCapacity);
         for (int i = 0; i < mNumStripes; i++)
         {
            if (mData[i] != null)
            {
               newCopy.mData[i] = new float[cStripeSize];
               Array.Copy(mData[i], newCopy.mData[i], cStripeSize);
               newCopy.mbHasData = true;
               if (newCopy.mMinStripe > i)
                  newCopy.mMinStripe = i;
               if (newCopy.mMaxStripe < i)
                  newCopy.mMaxStripe = i;
            }
         }
         return newCopy;
      }
      public void Set(IMask m)
      {
         ArrayBasedMask sameTypeMask = m as ArrayBasedMask;
         if (sameTypeMask != null)
         {
            base.Set(sameTypeMask);
         }
      }    
   }


   public class JaggedContainer<TDataType> where TDataType : new() 
   {
      public JaggedContainer(int maxCapacity)
      {

         mMaxCapacity = maxCapacity;
         mNumStripes = (maxCapacity >> cShiftAmount) + 1;
         mData = new TDataType[mNumStripes][];
      }
      public int mGuessCount = 0;

      protected int mMaxCapacity = 0;
      protected int mNumStripes = 0;
      protected int cStripeSize = 128;  //WARNING DO NOT CHANGE THIS VALUE. IT WILL WRECK THE WORLD
      protected int cShiftAmount = 7;
      protected int cIOVersion = 1;
      protected long cHeaderStart = long.MaxValue;

      protected bool mbHasData = false;

      protected TDataType[][] mData = null;

      TDataType mEmpty = new TDataType();

      protected int mMinStripe = Int32.MaxValue;
      protected int mMaxStripe = 0;

      public bool HasData()
      {
         return mbHasData;
      }

      public void SetEmptyValue(TDataType empty)
      {
         mEmpty = empty;
      }

      public TDataType GetValue(long index)
      {
         if (index < 0)
            return mEmpty;
         int stripenum = (int)(index >> cShiftAmount);
         if (stripenum >= mData.Length)
            return mEmpty;
         TDataType[] stripe = mData[stripenum];
         if (stripe == null)
         {
            return mEmpty;
         }
         return stripe[(index - (stripenum << cShiftAmount))];
      }

      public bool ContainsValue(long index)
      {
         if (index < 0)
            return false;
         int stripenum = (int)(index >> cShiftAmount);
         TDataType[] stripe = mData[stripenum];
         if (stripe == null)
            return false;
         
         return true;
      }

      public void SetValue(long index, TDataType value)
      {
         if (index < 0)
            return;
         int stripenum = (int)(index >> cShiftAmount);
         TDataType[] stripe = mData[stripenum];
         if (stripe == null)
         {
            mData[stripenum] = new TDataType[cStripeSize];
            stripe = mData[stripenum];
            mbHasData = true;
            mGuessCount += cStripeSize;
            for (int i = 0; i < cStripeSize; i++)
               stripe[i] = mEmpty;

               if (mMinStripe > stripenum)
                  mMinStripe = stripenum;
            if (mMaxStripe < stripenum)
               mMaxStripe = stripenum;
         }
         stripe[(index - (stripenum << cShiftAmount))] = value;
      }

      public JaggedContainer<TDataType> Clone()
      {
         JaggedContainer<TDataType> newCopy = new JaggedContainer<TDataType>(mMaxCapacity);

         for (int i = 0; i < mNumStripes; i++)
         {
            if (mData[i] != null)
            {
               newCopy.mData[i] = new TDataType[cStripeSize];
               Array.Copy(mData[i], newCopy.mData[i], cStripeSize);
               newCopy.mbHasData = true;

               if (newCopy.mMinStripe > i)
                  newCopy.mMinStripe = i;
               if (newCopy.mMaxStripe < i)
                  newCopy.mMaxStripe = i;
            }
         }
         return newCopy;

      }

      public void Set(JaggedContainer<TDataType> m)
      {
         mMinStripe = Int32.MaxValue;
         mMaxStripe = 0;
         JaggedContainer<TDataType> sameTypeMask = m;

         if (sameTypeMask != null)
         {
            for (int i = 0; i < mNumStripes; i++)
            {
               TDataType[] copy = null;
               if (sameTypeMask.mData[i] != null)
               {
                  copy = new TDataType[cStripeSize];
                  Array.Copy(sameTypeMask.mData[i], copy, cStripeSize);
                  mbHasData = true;
                  if (mMinStripe > i)
                     mMinStripe = i;
                  if (mMaxStripe < i)
                     mMaxStripe = i;
               }
               mData[i] = copy;



            }
         }
      }

      public void Clear()
      {
         mData = new TDataType[mNumStripes][];
         mbHasData = false;
         mGuessCount = 0;

         mMinStripe = Int32.MaxValue;
         mMaxStripe = 0;
      }

      int mItStripenum = 0;
      int mItLocalIndex = 0;
      public void ResetIterator()
      {

         if (mMinStripe == Int32.MaxValue)
            mMinStripe = 0;
         mItStripenum = mMinStripe;
         mItLocalIndex = 0;
      }

      public bool MoveNext(out long index, out TDataType value)
      {
         //CLM IS THIS OK??
         if (mItStripenum >= mNumStripes || mItStripenum > mMaxStripe)
         {
            index = -1;
            value = mEmpty;
            return false;
         }

         while (mData[mItStripenum] == null)
         {
            mItStripenum++;

            if (mItStripenum >= mNumStripes || mItStripenum > mMaxStripe)
            {
               index = -1;
               value = mEmpty;
               return false;
            }
         }

         value = mData[mItStripenum][mItLocalIndex];
         index = (mItStripenum << cShiftAmount) + mItLocalIndex;

         mItLocalIndex++;

         if (mItLocalIndex == cStripeSize)
         {
            mItLocalIndex = 0;
            mItStripenum++;
         }
         return true;
      }

      public bool MoveNextStripe(out long index, out TDataType[] value)
      {
         if (mData.GetLength(0) <= mItStripenum)
         {
            index = -1;
            value = new TDataType[0];
            return false;
         }

         while (mData[mItStripenum] == null )
         {
            mItStripenum++;

            if (mItStripenum >= mNumStripes || mItStripenum > mMaxStripe)
            {
               index = -1;
               value = new TDataType[0];
               return false;
            }
         }

         value = mData[mItStripenum];
         index = (mItStripenum << cShiftAmount) + mItLocalIndex;

         mItLocalIndex = 0;
         mItStripenum++;
        
         return true;
      }

      protected void SaveHeader(BinaryWriter w)
      {
         w.Write(cHeaderStart);
         w.Write(cStripeSize);
         w.Write(cIOVersion);
      }
      protected void LoadHeader(BinaryReader r)
      {
         long start = r.ReadInt64();
         int stripesize = r.ReadInt32();
         int ioversion = r.ReadInt32();
         
      }

      //header / version
      //public bool Save(BinaryWriter w, SaveDelegate d)
      //{
      //   ResetIterator();
      //   long index;
      //   TDataType value;

      //   SaveHeader(w);

      //   while (MoveNext(out index, out value))
      //   {
      //      if (mItLocalIndex == 0)
      //      {
      //         w.Write(index);
      //      }
      //      d.Invoke(w, value);
      //   }
      //   return true;
      //}
      public bool SaveByStripe(BinaryWriter w, SaveStripeDelegate d)
      {
         ResetIterator();
         long index;
         TDataType[] values;

         SaveHeader(w);
         //int origin = w.BaseStream.p
         int numstripes = 0;
         while (MoveNextStripe(out index, out values))
         {
            //if (mItLocalIndex == 0)  ... always true
            w.Write(index);
            d.Invoke(w, values);
            numstripes++;
         }

         w.Write(cIOEnd);

         return true;
      }
      //public bool Load(BinaryReader r, LoadDelegate d)
      //{
      //   Clear();
      //   return true;
      //}
      public bool LoadByStripe(BinaryReader r, LoadStripeDelegate d)
      {
         Clear();
         LoadHeader(r);

         long index;
         while ((index = r.ReadInt64()) != cIOEnd)
         {
            int stripenum = (int)(index >> cShiftAmount);
            TDataType[] stripe = new TDataType[cStripeSize];
            mData[stripenum] = stripe;
            mbHasData = true;            
            d.Invoke(r, stripe);


            if (mMinStripe > stripenum)
               mMinStripe = stripenum;
            if (mMaxStripe < stripenum)
               mMaxStripe = stripenum;
         }

         return true;
      }

      long cIOEnd = long.MinValue;

      //public delegate void SaveDelegate(BinaryWriter w, TDataType val);
      public delegate void SaveStripeDelegate(BinaryWriter w, TDataType[] values);
      //public delegate void LoadDelegate(BinaryReader r, TDataType val);
      public delegate void LoadStripeDelegate(BinaryReader r, TDataType[] values);
   }


}