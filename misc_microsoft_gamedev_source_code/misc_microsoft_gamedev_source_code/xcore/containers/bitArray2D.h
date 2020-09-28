//============================================================================
//
// File: bitArray2D.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

template<uint NumElementBits>
class BBitArray2D;

template<uint NumElementBits = 1>
class BBitArray2DAccessHelper
{
public:
   typedef BBitArray2D<NumElementBits> BBitArray2DType;

   BBitArray2DAccessHelper(BBitArray2DType& array, uint x, uint y) : mArray(array), mX(x), mY(y)
   {
   }
   
   uint operator= (uint val) 
   {
      mArray.set(mX, mY, val);
      return val;
   }

   operator uint() const
   {  
      return mArray.get(mX, mY);
   }

private:   
   BBitArray2DType& mArray;
   uint mX;
   uint mY;
};

template<uint NumElementBits = 1>
class BBitArray2D
{
public:
   //typedef BBitArray2DConstAccessHelper<NumElementBits> BBitArray2DConstAccessHelperType;
   typedef BBitArray2DAccessHelper<NumElementBits> BBitArray2DAccessHelperType;
   
   BBitArray2D(uint width = 0, uint height = 0)
   {
      BCOMPILETIMEASSERT((NumElementBits >= 1) && (NumElementBits <= 25));
      BDEBUG_ASSERT( ((width == 0) && (height == 0)) || ((width > 0) && (height > 0)) );
            
      mWidth = width;
      mHeight = height;
      mPitch = (width * NumElementBits + 7) >> 3;
      mBytes.resize(mPitch * mHeight + PAD_BYTES);
   }
   
   BBitArray2D(const BBitArray2D& other)
   {  
      BCOMPILETIMEASSERT((NumElementBits >= 1) && (NumElementBits <= 25));
      
      mWidth = other.mWidth;
      mHeight = other.mHeight;
      mPitch = other.mPitch;
      mBytes = other.mBytes;
   }
   
   BBitArray2D& operator= (const BBitArray2D& rhs)
   {  
      if (this == &rhs)
         return *this;
         
      mWidth = rhs.mWidth;
      mHeight = rhs.mHeight;
      mPitch = rhs.mPitch;
      mBytes = rhs.mBytes;
      
      return *this;
   }
   
   const BByteArray& getBytes(void) const   { return mBytes; }
         BByteArray& getBytes(void)         { return mBytes; }
   
   uint getWidth(void) const { return mWidth; }
   uint getHeight(void) const { return mHeight; }
   uint getElements(void) const { return mWidth * mHeight; }
   uint getPitch(void) const { return mPitch; }
   uint getNumElementBits(void) const { return NumElementBits; }
   uint getBitMask(void) const { return BIT_MASK; }
         
   uint get(uint x, uint y) const
   {
      BDEBUG_ASSERT((x < mWidth) && (y < mHeight));
      
      return get(x, y, mPitch, mBytes);
   }
   
   uint get(uint x) const
   {
      BDEBUG_ASSERT(x < mWidth);

      return get(x, mBytes);
   }
         
   void set(uint x, uint y, uint val)
   {
      BDEBUG_ASSERT((x < mWidth) && (y < mHeight));

      set(x, y, val, mPitch, mBytes);
   }   
   
   void set(uint x, uint val)
   {
      BDEBUG_ASSERT(x < mWidth);

      set(x, val, mBytes);
   }   
       
   uint operator() (uint x, uint y) const { return get(x, y); }
   uint operator[] (uint elementIndex) const { BDEBUG_ASSERT(elementIndex < getElements()); return get(elementIndex % mWidth, elementIndex / mWidth); }
   
   BBitArray2DAccessHelperType operator() (uint x, uint y) { return BBitArray2DAccessHelperType(*this, x, y); }
   BBitArray2DAccessHelperType operator[] (uint elementIndex) { BDEBUG_ASSERT(elementIndex < getElements()); return BBitArray2DAccessHelperType(*this, elementIndex % mWidth, elementIndex / mWidth); }
   
   void clear(void)
   {
      mWidth = 0;
      mHeight = 0;
      mPitch = 0;
      mBytes.clear();
   }
   
   void resize(uint newWidth, uint newHeight, bool preserveContents = true)
   {
      BDEBUG_ASSERT( ((newWidth == 0) && (newHeight == 0)) || ((newWidth > 0) && (newHeight > 0)) );
      
      if ((mWidth == newWidth) && (mHeight == newHeight))
         return;
         
      const uint newPitch = (newWidth * NumElementBits + 7) >> 3;
      BByteArray newBytes(newPitch * newHeight + PAD_BYTES);
      
      if (preserveContents)
      {
         const uint yh = Math::Min(newHeight, mHeight);
         const uint xh = Math::Min(newWidth, mWidth);
         for (uint y = 0; y < yh; y++)
         {
            for (uint x = 0; x < xh; x++)
            {
               const uint val = get(x, y, mPitch, mBytes);
               set(x, y, val, newPitch, newBytes);
            }
         }
      }   
      
      mWidth = newWidth;
      mHeight = newHeight;
      mPitch = newPitch;
      mBytes.swap(newBytes);         
   }
   
   void setAll(uint val)
   {
      val &= BIT_MASK;
      
      if (val == 0)
         mBytes.setAll(0);
      else if (val == BIT_MASK)
         mBytes.setAll(255);
      else
      {
         for (uint y = 0; y < mHeight; y++)
            for (uint x = 0; x < mWidth; x++)
               set(x, y, val);
      }               
   }
   
   uint getSerializeSize(void) const
   {
      return sizeof(uint) * 3 + BDynamicArraySerializer::getSize(mBytes);
   }
   
   void* serialize(void* pDst) const
   {
      pDst = Utils::writeObj(pDst, mWidth);
      pDst = Utils::writeObj(pDst, mHeight);
      pDst = Utils::writeObj(pDst, mPitch);
      
      pDst = BDynamicArraySerializer::serialize(mBytes, pDst);
         
      return pDst;
   }
   
   const void* deserialize(const void* pSrc)
   {
      pSrc = Utils::readObj(pSrc, mWidth);
      pSrc = Utils::readObj(pSrc, mHeight);
      pSrc = Utils::readObj(pSrc, mPitch);
      
      pSrc = BDynamicArraySerializer::deserialize(mBytes, pSrc);
      
      return pSrc;
   }
      
private:
   enum { PAD_BYTES = (NumElementBits < 2U) ? 0U : ( (NumElementBits < 10U) ? 1U : ( (NumElementBits < 18U) ? 2U : 3U) ) };
   enum { BIT_MASK = (NumElementBits == 32) ? 0xFFFFFFFFU : ((1U << NumElementBits) - 1U) };
   
   BByteArray mBytes;
   uint mWidth;
   uint mHeight;
   uint mPitch;
         
   uint get(uint x, uint y, uint pitch, const BByteArray& bytes) const
   {
      const uint bitOfs = x * NumElementBits;
      const uint firstByteOfs = (bitOfs >> 3) + (y * pitch);
      const uint shift = bitOfs & 7;

      uint v = bytes[firstByteOfs];
      if (NumElementBits >= 2)
      {
         v |= (bytes[firstByteOfs + 1] << 8);
         if (NumElementBits >= 10)
         {
            v |= (bytes[firstByteOfs + 2] << 16);
            if (NumElementBits >= 18)
               v |= (bytes[firstByteOfs + 3] << 24);
         }
      }         

      return (v >> shift) & BIT_MASK;
   }   
   
   uint get(uint x, const BByteArray& bytes) const
   {
      const uint bitOfs = x * NumElementBits;
      const uint firstByteOfs = (bitOfs >> 3);
      const uint shift = bitOfs & 7;

      uint v = bytes[firstByteOfs];
      if (NumElementBits >= 2)
      {
         v |= (bytes[firstByteOfs + 1] << 8);
         if (NumElementBits >= 10)
         {
            v |= (bytes[firstByteOfs + 2] << 16);
            if (NumElementBits >= 18)
               v |= (bytes[firstByteOfs + 3] << 24);
         }
      }         

      return (v >> shift) & BIT_MASK;
   }   
            
   void set(uint x, uint y, uint val, uint pitch, BByteArray& bytes)
   {
      const uint bitOfs = x * NumElementBits;
      const uint firstByteOfs = (bitOfs >> 3) + (y * pitch);
      const uint shift = bitOfs & 7;

      uint v = bytes[firstByteOfs];
      if (NumElementBits >= 2)
      {
         v |= (bytes[firstByteOfs + 1] << 8);
         if (NumElementBits >= 10)
         {
            v |= (bytes[firstByteOfs + 2] << 16);
            if (NumElementBits >= 18)
               v |= (bytes[firstByteOfs + 3] << 24);
         }
      }         

      v &= ~(BIT_MASK << shift);
      v |= (val & BIT_MASK) << shift;

      bytes[firstByteOfs] = static_cast<uchar>(v);
      if (NumElementBits >= 2)
      {
         bytes[firstByteOfs + 1] = static_cast<uchar>(v >> 8);
         if (NumElementBits >= 10)
         {
            bytes[firstByteOfs + 2] = static_cast<uchar>(v >> 16);
            if (NumElementBits >= 18)
               bytes[firstByteOfs + 3] = static_cast<uchar>(v >> 24);
         }
      }
   }
   
   void set(uint x, uint val, BByteArray& bytes)
   {
      const uint bitOfs = x * NumElementBits;
      const uint firstByteOfs = (bitOfs >> 3);
      const uint shift = bitOfs & 7;

      uint v = bytes[firstByteOfs];
      if (NumElementBits >= 2)
      {
         v |= (bytes[firstByteOfs + 1] << 8);
         if (NumElementBits >= 10)
         {
            v |= (bytes[firstByteOfs + 2] << 16);
            if (NumElementBits >= 18)
               v |= (bytes[firstByteOfs + 3] << 24);
         }
      }         

      v &= ~(BIT_MASK << shift);
      v |= (val & BIT_MASK) << shift;

      bytes[firstByteOfs] = static_cast<uchar>(v);
      if (NumElementBits >= 2)
      {
         bytes[firstByteOfs + 1] = static_cast<uchar>(v >> 8);
         if (NumElementBits >= 10)
         {
            bytes[firstByteOfs + 2] = static_cast<uchar>(v >> 16);
            if (NumElementBits >= 18)
               bytes[firstByteOfs + 3] = static_cast<uchar>(v >> 24);
         }
      }
   }
};

#ifdef TEST_BITARRAY_2D
template<uint numBits>
inline bool testBitArray2D(void)
{
   for (uint i = 0; i < 16; i++)
   {
      srand(numBits*257+i*22);
      const uint width = (rand() & 511) + 1;
      const uint height = (rand() & 511) + 1;
      
      BBitArray2D<numBits> array(width, height);

      srand(width*height+99*i);
      for (uint y = 0; y < array.getHeight(); y++)
         for (uint x = 0; x < array.getWidth(); x++)
         {
            uint expectedVal = (rand()|(rand()<<16)) & array.getBitMask();
            array(x, y) = expectedVal;
         }

      srand(width*height+99*i);
      for (uint y = 0; y < array.getHeight(); y++)
         for (uint x = 0; x < array.getWidth(); x++)
         {
            uint expectedVal = (rand()|(rand()<<16)) & array.getBitMask();
            uint val = array(x, y);
            if (val != expectedVal)
               return false;
         }
         
      const uint newWidth = (rand() & 511) + 1;
      const uint newHeight = (rand() & 511) + 1;
      
      array.resize(newWidth, newHeight);
      
      srand(width*height+99*i);
      for (uint y = 0; y < height; y++)
         for (uint x = 0; x < width; x++)
         {
            uint expectedVal = (rand()|(rand()<<16)) & array.getBitMask();
            if ((x < newWidth) && (y < newHeight))
            {
               uint val = array(x, y);
               if (val != expectedVal)
                  return false;
            }                  
         }
   }         

   char buf[256];
   sprintf(buf, "BBitArray2D<%i> Tested OK\n", numBits);
   OutputDebugStringA(buf);

   return true;     
}

inline bool testBitArray2D(void)
{
   if (!testBitArray2D<1>()) return false;
   if (!testBitArray2D<2>()) return false;
   if (!testBitArray2D<3>()) return false;
   if (!testBitArray2D<4>()) return false;
   if (!testBitArray2D<5>()) return false;
   if (!testBitArray2D<6>()) return false;
   if (!testBitArray2D<7>()) return false;
   if (!testBitArray2D<8>()) return false;
   if (!testBitArray2D<9>()) return false;
   if (!testBitArray2D<10>()) return false;
   if (!testBitArray2D<11>()) return false;
   if (!testBitArray2D<12>()) return false;
   if (!testBitArray2D<13>()) return false;
   if (!testBitArray2D<14>()) return false;
   if (!testBitArray2D<15>()) return false;
   if (!testBitArray2D<16>()) return false;      
   if (!testBitArray2D<17>()) return false;
   if (!testBitArray2D<18>()) return false;
   if (!testBitArray2D<19>()) return false;
   if (!testBitArray2D<20>()) return false;
   if (!testBitArray2D<21>()) return false;
   if (!testBitArray2D<22>()) return false;
   if (!testBitArray2D<23>()) return false;      
   if (!testBitArray2D<24>()) return false;
   if (!testBitArray2D<25>()) return false;
   
   return true;
}
#endif