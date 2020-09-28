using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;

namespace EditorCore
{
   /// <summary>
   /// This class holds 2D data in a sparse fashion
   /// A block can be MxN in size
   /// This class will allocate to the next multiple of blockSize
   /// When a block becomes empty it is removed from memory
   /// When a block is requested, it is created.
   /// 
   /// </summary>
   /// <remarks>
   /// This certain version uses 16x16 blocks and uses X>>4 for fast division
   /// If you need a version that changes this, it's better to duplicate this class with
   /// different naming conventions. Otherwise, persistant (disk) data may be written with this 
   /// specific spacing as a relivant factor.
   /// </remarks>
   public class Sparse2DBlockArray<T>
   {
      public int Width{get{return mWidth;}}
      public int Height { get { return mHeight; } }
      protected int mWidth = 128;
      protected int mHeight = 128;
      protected byte mBlockWidth = 16;
      protected byte mBlockHeight = 16;

      protected int mNumXBlocks = 0;
      protected int mNumYBlocks = 0;
      protected T mEmptyValue;
      protected bool mEraseEmptyBlocksDynamically = true;

      protected Sparse2DBlock<T>[,] mSparseBlocks = null;

      protected sealed class Sparse2DBlock<U>
      {
         private U[,] mData;

         public Sparse2DBlock(int width, int height, U emptyVal)
         {
            mData = new U[width, height];
            setAllTo(emptyVal);

         }

         private void setAllTo(U emptyVal)
         {
            for (int x = 0; x < mData.GetLength(0); x++)
               for (int y = 0; y < mData.GetLength(1); y++)
                  mData[x, y] = emptyVal;
         }
         ~Sparse2DBlock()
         {
            mData = null;
         }

         public bool isAllEqualTo(U emptyVal)
         {
            for (int x = 0; x < mData.GetLength(0); x++)
               for (int y = 0; y < mData.GetLength(1); y++)
                  if (!mData[x, y].Equals(emptyVal))
                     return false;

            return true;
         }

         public U this[int xIndex, int yIndex]
         {
            get
            {
               return mData[xIndex, yIndex];
            }
            set
            {
               mData[xIndex, yIndex] = value;
            }
         }
      }



      public Sparse2DBlockArray(int width, int height, T emptyValue, bool eraseEmptyBlocksDynamically)
      {
         mBlockWidth = 16;
         mBlockHeight = 16;
         mWidth = width;
         mHeight = height;
         mEmptyValue = emptyValue;
         mEraseEmptyBlocksDynamically = eraseEmptyBlocksDynamically;

         mNumXBlocks = mWidth / mBlockWidth;
         if (mNumXBlocks * mBlockWidth < mWidth) mNumXBlocks++;
         mNumYBlocks = mHeight / mBlockHeight;
         if (mNumYBlocks * mBlockHeight < mHeight) mNumYBlocks++;
         mSparseBlocks = new Sparse2DBlock<T>[mNumXBlocks, mNumYBlocks];
      }

      ~Sparse2DBlockArray()
      {
         destroy();
      }
      public virtual void destroy()
      {
         if(mSparseBlocks!=null)
         {
            for (int x = 0; x < mNumXBlocks; x++)
               for (int y = 0; y < mNumYBlocks; y++)
                  mSparseBlocks[x, y] = null;
               mSparseBlocks = null;
         }

      }

      public virtual T this[int xIndex, int yIndex]
      {
         get
         {
            if (xIndex < 0 || xIndex >= mWidth || yIndex < 0 || yIndex >= mWidth)
               return mEmptyValue;

            int xBlockIndex = xIndex >> 4;// xIndex / mBlockWidth;
            int yBlockIndex = yIndex >> 4;// yIndex / mBlockHeight;

            
            if(mSparseBlocks[xBlockIndex,yBlockIndex]==null)
               return mEmptyValue;


            int localX = xIndex % mBlockWidth;
            int localY = yIndex % mBlockHeight;

            return mSparseBlocks[xBlockIndex, yBlockIndex][localX, localY];
         }
         set
         {
            if (xIndex < 0 || xIndex >= mWidth || yIndex < 0 || yIndex >= mWidth)
               return;


            int xBlockIndex = xIndex >> 4;// xIndex / mBlockWidth;
            int yBlockIndex = yIndex >> 4;// yIndex / mBlockHeight;


            if (mSparseBlocks[xBlockIndex, yBlockIndex] == null)
               mSparseBlocks[xBlockIndex, yBlockIndex] = new Sparse2DBlock<T>(mBlockWidth, mBlockHeight, mEmptyValue);


            int localX = xIndex % mBlockWidth;
            int localY = yIndex % mBlockHeight;

            mSparseBlocks[xBlockIndex, yBlockIndex][localX, localY] = value;

            if (mEraseEmptyBlocksDynamically)
               if (value.Equals(mEmptyValue))
                  if (mSparseBlocks[xBlockIndex, yBlockIndex].isAllEqualTo(mEmptyValue))
                     mSparseBlocks[xBlockIndex, yBlockIndex] = null;
         }
      }

      public void cleanEmpty()
      {
         {
            for (int x = 0; x < mNumXBlocks; x++)
               for (int y = 0; y < mNumYBlocks; y++)
                  if (mSparseBlocks[x, y] != null && mSparseBlocks[x, y].isAllEqualTo(mEmptyValue))
                     mSparseBlocks[x, y] = null;
         }
      }

      //these cannot be overridden
      public void setAllToEmpty()
      {
         for (int x = 0; x < mNumXBlocks; x++)
            for (int y = 0; y < mNumYBlocks; y++)
               mSparseBlocks[x, y] = null;
      }
      public void setAllToValue(T val)
      {
         for (int x = 0; x < mWidth; x++)
            for (int y = 0; y < mHeight; y++)
               this[x, y] = val;
      }


      /// <returns>
      /// This will return an axis aligned rectangle that contains the 'block' boundries 
      ///  of the allocated data in the set that does not equal the empty value
      ///  </returns>
      public Rectangle giveConservativeNonEmptyBounds()
      {
         int minx = mWidth;
         int miny = mHeight;
         int maxx = 0;
         int maxy = 0;

         for (int y = 0; y < mNumYBlocks; y++)
         {
            for (int x = 0; x < mNumXBlocks; x++)
            {
               if (mSparseBlocks[x, y] != null && !mSparseBlocks[x, y].isAllEqualTo(mEmptyValue))
               {
                  if (x * mBlockWidth < minx) minx = x * mBlockWidth;
                  if (y * mBlockHeight < miny) miny = y * mBlockHeight;

                  if ((x + 1) * mBlockWidth > maxx) maxx = (x + 1) * mBlockWidth;
                  if ((y + 1) * mBlockHeight > maxy) maxy = (y + 1) * mBlockHeight;
               }
            }
         }

         return new Rectangle(minx, miny, maxx - minx, maxy - miny);
      }

      /// <returns>
      /// This will return an axis aligned rectangle that contains the 'block' boundries 
      ///  of the allocated data in the set</returns>
      public Rectangle giveConservativeAllocatedBounds()
      {
         int minx = mWidth;
         int miny = mHeight;
         int maxx = 0;
         int maxy = 0;

         for (int y = 0; y < mNumYBlocks; y++)
         {
            for (int x = 0; x < mNumXBlocks; x++)
            {
               if (mSparseBlocks[x, y] != null)
               {
                  if (x * mBlockWidth < minx) minx = x * mBlockWidth;
                  if (y * mBlockHeight < miny) miny = y * mBlockHeight;

                  if ((x + 1) * mBlockWidth > maxx) maxx = (x + 1) * mBlockWidth;
                  if ((y + 1) * mBlockHeight > maxy) maxy = (y + 1) * mBlockHeight;
               }
            }
         }

         return new Rectangle(minx, miny, maxx - minx, maxy - miny);
      }


      public virtual Sparse2DBlockArray<T> Clone()
      {
         Sparse2DBlockArray<T> cpy = new Sparse2DBlockArray<T>(mWidth, mHeight, mEmptyValue, mEraseEmptyBlocksDynamically);

         for (int x = 0; x < mWidth; x++)
         {
            for (int y = 0; y < mHeight; y++)
            {
               cpy[x, y] = this[x, y];
            }
         }
         return cpy;
      }

      public virtual void CopyTo(Sparse2DBlockArray<T> dst)
      {
         if (dst.mHeight != mHeight || dst.mWidth != mWidth)
            throw new Exception("Error, cannot copy Sparse2DBlockArray to dest: Dest is not the same size");

         for (int x = 0; x < mWidth; x++)
         {
            for (int y = 0; y < mHeight; y++)
            {
               dst[x, y] = this[x, y];
            }
         }
      }

   }

   /// <remarks>
   /// This certain version keeps a boolean array marking if an entire block 
   /// is the same value or not. If it is, the memory is freed, and we keep the constant value
   /// in another array
   /// </remarks>
   public class Sparse2DBlockDeflateArray<T> : Sparse2DBlockArray<T>
   {
      BitArray64 mIsBlockConstant = null;
      T[] mBlockConstantValue = null;
      bool mDeflateBlocksDynamically = true;

      public Sparse2DBlockDeflateArray(int width, int height, T emptyValue, bool eraseEmptyBlocksDynamically, bool deflateBlocksDynamically)
         : base(width, height, emptyValue, eraseEmptyBlocksDynamically)
      {
         mIsBlockConstant = new BitArray64(mNumXBlocks * mNumYBlocks);
         mIsBlockConstant.setAll(false);
         mBlockConstantValue = new T[mNumXBlocks * mNumYBlocks];
         mDeflateBlocksDynamically = deflateBlocksDynamically;
      }
      ~Sparse2DBlockDeflateArray()
      {
         destroy();
      }
      public override void destroy()
      {
         mIsBlockConstant = null;
         mBlockConstantValue = null;
         base.destroy();
      }

      public override T this[int xIndex, int yIndex]
      {
         get
         {
            if (xIndex < 0 || xIndex >= mWidth || yIndex < 0 || yIndex >= mWidth)
               return mEmptyValue;

            int xBlockIndex = xIndex >> 4;// xIndex / mBlockWidth;
            int yBlockIndex = yIndex >> 4;// yIndex / mBlockHeight;
            

            int blockIndex = xBlockIndex + yBlockIndex * mNumXBlocks;
            if (mIsBlockConstant.isSet(blockIndex))
               return mBlockConstantValue[blockIndex];

            return base[xIndex, yIndex];
         }
         set
         {
            if (xIndex < 0 || xIndex >= mWidth || yIndex < 0 || yIndex >= mWidth)
               return;

            int xBlockIndex = xIndex >> 4;// xIndex / mBlockWidth;
            int yBlockIndex = yIndex >> 4;// yIndex / mBlockHeight;
            int blockIndex = xBlockIndex + yBlockIndex * mNumXBlocks;

            if (mIsBlockConstant.isSet(blockIndex))
            {
               if (mBlockConstantValue[blockIndex].Equals(value))
                  return;

               //we had a deflated block, that we're now adding noise to. 
               //inflate and add the data
               mSparseBlocks[xBlockIndex, yBlockIndex] = new Sparse2DBlock<T>(mBlockWidth, mBlockHeight, mBlockConstantValue[blockIndex]);
               mIsBlockConstant.setValue((uint)blockIndex, false);
               mBlockConstantValue[blockIndex] = mEmptyValue; 
            }

            base[xIndex, yIndex] = value;


            //if the entire block is equal to the same value.. deflate!
            if (mDeflateBlocksDynamically && !value.Equals(mEmptyValue))
            {
               if (mSparseBlocks[xBlockIndex, yBlockIndex].isAllEqualTo(value))
               {
                  mSparseBlocks[xBlockIndex, yBlockIndex] = null;

                  mIsBlockConstant.setValue((uint)blockIndex, true);
                  mBlockConstantValue[blockIndex] = value;

               }
            }
         }
      }


      public void deflateBlocks()
      {
         for (int x = 0; x < mNumXBlocks; x++)
         {
            for (int y = 0; y < mNumYBlocks; y++)
            {
               if (mSparseBlocks[x, y] == null)
                  continue;

               int blockIndex = x + y * mNumXBlocks;
               T val = mSparseBlocks[x, y][0, 0];
               if (mSparseBlocks[x, y].isAllEqualTo(val))
               {
                  mSparseBlocks[x, y] = null;

                  mIsBlockConstant.setValue((uint)blockIndex, true);
                  mBlockConstantValue[blockIndex] = val;

               }
            }
         }
      }

      public virtual Sparse2DBlockDeflateArray<T> Clone()
      {
         Sparse2DBlockDeflateArray<T> cpy = new Sparse2DBlockDeflateArray<T>(mWidth, mHeight, mEmptyValue, mEraseEmptyBlocksDynamically, mDeflateBlocksDynamically);

         //mIsBlockConstant.copyTo(cpy.mIsBlockConstant);
         //mBlockConstantValue.CopyTo(cpy.mBlockConstantValue);

         for (int x = 0; x < mWidth; x++)
         {
            for (int y = 0; y < mHeight; y++)
            {
               cpy[x, y] = this[x, y];
            }
         }
         return cpy;
      }

      public virtual void CopyTo(Sparse2DBlockDeflateArray<T> dst)
      {
         if (dst.mHeight != mHeight || dst.mWidth != mWidth)
            throw new Exception("Error, cannot copy Sparse2DBlockDeflateArray to dest: Dest is not the same size");

         for (int x = 0; x < mWidth; x++)
         {
            for (int y = 0; y < mHeight; y++)
            {
               dst[x, y] = this[x, y];
            }
         }
      }

   }
}
