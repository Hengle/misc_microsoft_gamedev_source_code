//============================================================================
//
// File: staticArray2D.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

template<typename ValType, uint Width, uint Height>
class BStaticArray2D
{
public:
   BStaticArray2D() { }
      
   BStaticArray2D(const ValType& v) 
   {
      for (uint y = 0; y < Height; y++)
         for (uint x = 0; x < Width; x++)
            mV[y][x] = v;
   }
   
   void setAll(const ValType& v)
   {
      for (uint y = 0; y < Height; y++)
         for (uint x = 0; x < Width; x++)
            mV[y][x] = v;
   }
   
   void setAll(const ValType* pV)
   {
      for (uint y = 0; y < Height; y++)
         for (uint x = 0; x < Width; x++)
            mV[y][x] = pV[x + y * Width];
   }            
   
   uint getWidth(void) const { return Width; }
   uint getHeight(void) const { return Height; }
   uint getElements(void) const { return Width * Height; }

   const ValType& operator() (uint x, uint y) const   { BDEBUG_ASSERT((x < Width) && (y < Height)); return mV[y][x]; }
         ValType& operator() (uint x, uint y)         { BDEBUG_ASSERT((x < Width) && (y < Height)); return mV[y][x]; }
         
   const ValType& operator[] (uint elementIndex) const   { BDEBUG_ASSERT(elementIndex < getElements()); return (&mV[0][0])[elementIndex]; }
         ValType& operator[] (uint elementIndex)         { BDEBUG_ASSERT(elementIndex < getElements()); return (&mV[0][0])[elementIndex]; }            

private:
   ValType mV[Height][Width];         
};

