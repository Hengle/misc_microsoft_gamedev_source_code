//============================================================================
//
// File: dynamicArray2D.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

template<class ValueType, int Alignment = (__alignof(ValueType) > 0) ? __alignof(ValueType) : 1>
class BDynamicArray2D
{
public:
   typedef BDynamicArray<ValueType, Alignment> BArrayType;
   
   BDynamicArray2D(uint width = 0, uint height = 0)
   {
      mElements.resize(width * height);
      mWidth = width;
      mHeight = height;
   }

   BDynamicArray2D(const BDynamicArray2D& other)
   {
      mElements = other.mElements;
      mWidth = other.mWidth;
      mHeight = other.mHeight;
   }
   
   BDynamicArray2D& operator= (const BDynamicArray2D& other)
   {
      if (this == &other)
         return *this;
         
      mElements = other.mElements;
      mWidth = other.mWidth;
      mHeight = other.mHeight;
      
      return *this;
   }
              
   const BArrayType& getArray(void) const { return mElements; }
         BArrayType& getArray(void)       { return mElements; }

   void resize(uint newWidth, uint newHeight, bool preserveContents = true)
   {
      if ((mWidth == newWidth) && (mHeight == newHeight))
         return;

      BArrayType newElements(newWidth * newHeight);
      
      if (preserveContents)
      {
         const uint yh = Math::Min(newHeight, mHeight);
         const uint xh = Math::Min(newWidth, mWidth);
         for (uint y = 0; y < yh; y++)
            for (uint x = 0; x < xh; x++)
               newElements[x + y * newWidth] = mElements[x + y * mWidth];
      }
      
      mElements.swap(newElements);
      mWidth = newWidth;
      mHeight = newHeight;
   }

   void clear(void)
   {
      mElements.clear();
      mWidth = 0;
      mHeight = 0;
   }
   
   uint getWidth(void) const { return mWidth; }
   uint getHeight(void) const { return mHeight; }
   uint getElements(void) const { return mElements.size(); }
   
   // Object accessor
   const ValueType& operator[] (uint elementIndex) const 
   { 
      return mElements[elementIndex];
   }

   // Object accessor
   ValueType& operator[] (uint elementIndex) 
   { 
      return mElements[elementIndex];
   }
   
   const ValueType& operator() (uint x, uint y) const
   {
      BASSERT((x < mWidth) && (y < mHeight));
      return mElements[x + y * mWidth];
   }
   
   ValueType& operator() (uint x, uint y) 
   {
      BASSERT((x < mWidth) && (y < mHeight));
      return mElements[x + y * mWidth];
   }
   
   BDynamicArray2D& swap(BDynamicArray2D& rhs)
   {
      mElements.swap(rhs.mElements);
      std::swap(mWidth, rhs.mWidth);
      std::swap(mHeight, rhs.mHeight);
      return *this;
   }
   
   void setAll(const ValueType& t)   
   {
      mElements.setAll(t);
   }

private:
   BArrayType mElements;
   uint mWidth;
   uint mHeight;
};

#ifdef TEST_ARRAY2D
#include "bytepacker.h"

template<typename T>
inline bool testArray2D(void)
{
   const uint valMask = (sizeof(T)==4) ? 0xFFFFFFFF : ((1 << (sizeof(T) * 8)) - 1);
   
   for (uint i = 0; i < 16; i++)
   {
      srand(sizeof(T)*257+i*22);
      const uint width = (rand() & 511) + 1;
      const uint height = (rand() & 511) + 1;

      BDynamicArray2D<T> array(width, height);

      srand(width*height+99*i);
      for (uint y = 0; y < array.getHeight(); y++)
         for (uint x = 0; x < array.getWidth(); x++)
         {
            T expectedVal = static_cast<T>( (rand()|(rand()<<16)) & valMask );
            array(x, y) = expectedVal;
         }

      srand(width*height+99*i);
      for (uint y = 0; y < array.getHeight(); y++)
         for (uint x = 0; x < array.getWidth(); x++)
         {
            T expectedVal = static_cast<T>( (rand()|(rand()<<16)) & valMask );
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
            T expectedVal = static_cast<T>( (rand()|(rand()<<16)) & valMask );
            if ((x < newWidth) && (y < newHeight))
            {
               uint val = array(x, y);
               if (val != expectedVal)
                  return false;
            }                  
         }
   }         

   char buf[256];
   sprintf(buf, "BDynamicArray2D %i Tested OK\n", sizeof(T));
   OutputDebugStringA(buf);

   return true;     
}

inline bool testArray2D(void) 
{
   if (!testArray2D<BPacked32>()) return false;
   if (!testArray2D<BPacked24>()) return false;
   if (!testArray2D<BPacked16>()) return false;
   
   if (!testArray2D<unsigned char>()) return false;
   if (!testArray2D<unsigned short>()) return false;
   if (!testArray2D<unsigned int>()) return false;
         
   return true;
}
#endif
