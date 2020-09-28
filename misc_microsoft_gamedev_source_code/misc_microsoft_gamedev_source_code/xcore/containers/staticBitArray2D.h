//============================================================================
//
// File: staticBitArray2D.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

template<uint Width, uint Height>
class BStaticBitArray2D;

template<uint Width, uint Height>
class BStaticBitArray2DAccessHelper
{
public:
   typedef BStaticBitArray2D<Width, Height> BStaticBitArray2DType;

   BStaticBitArray2DAccessHelper(BStaticBitArray2DType& array, uint x, uint y) : mArray(array), mX(x), mY(y)
   {
   }

   operator uint () const
   {
      return mArray.get(mX, mY);
   }

   uint operator= (uint val)
   {
      mArray.set(mX, mY, val);
      return val;
   }

private:
   BStaticBitArray2DType& mArray;
   uint mX;
   uint mY;
};

template<uint Width, uint Height>
class BStaticBitArray2D
{
public:
   typedef BStaticBitArray2DAccessHelper<Width, Height> BStaticBitArray2DAccessHelperType;

   BStaticBitArray2D()
   {
      clear();
   }         
   
   void clear(void)
   {
      memset(mBits, 0, cNumBytes);
   }
   
   uint getWidth(void) const { return Width; }
   uint getHeight(void) const { return Height; }
   uint getElements(void) const { return Width * Height; }

   uint operator() (uint x, uint y) const
   {
      BDEBUG_ASSERT((x < Width) && (y < Height));
      
      const uint bitNum = x + y * Width;
      const uint byteOfs = bitNum >> 3U;
      const uint shift = bitNum & 7U;
      
      return (mBits[byteOfs] >> shift) & 1U;
   }

   BStaticBitArray2DAccessHelperType operator() (uint x, uint y) 
   {  
      return BStaticBitArray2DAccessHelperType(*this, x, y);
   }

   uint get(uint x, uint y) const 
   {
      BDEBUG_ASSERT((x < Width) && (y < Height));
      return (*this)(x, y);
   }

   void set(uint x, uint y, uint val) 
   {
      BDEBUG_ASSERT((x < Width) && (y < Height));
      
      const uint bitNum = x + y * Width;
      const uint byteOfs = bitNum >> 3U;
      const uint shift = bitNum & 7U;
      
      mBits[byteOfs] &= ~(1 << shift);
      mBits[byteOfs] |= ((val & 1) << shift);
   }

   void setAll(uint val)
   {
      for (uint i = 0; i < cNumBytes; i++)
         mBits[i] = static_cast<uchar>(-static_cast<int>(val));
   }

private:
   enum { cNumBytes = (Width * Height + 7) >> 3 };
   uchar mBits[cNumBytes];
};

