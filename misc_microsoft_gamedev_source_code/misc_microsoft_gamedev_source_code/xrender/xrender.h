//============================================================================
//
//  xrender.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "xcore.h"
#include "xsystem.h"

#include <fxl.h>
#include <d3dx9.h>
#include <xgraphics.h>

#ifndef MAKENOSRGBFMT
   #define MAKENOSRGBFMT(D3dFmt) ((D3dFmt) & ~(D3DFORMAT_SIGNX_MASK | D3DFORMAT_SIGNY_MASK | D3DFORMAT_SIGNZ_MASK))
#endif

typedef BDynamicArray<char,   4, BDynamicArrayRenderHeapAllocator> BRenderCharArray;  
typedef BDynamicArray<uchar,  4, BDynamicArrayRenderHeapAllocator> BRenderUCharArray;  
typedef BDynamicArray<WORD,   4, BDynamicArrayRenderHeapAllocator> BRenderWORDArray;  
typedef BDynamicArray<int,    4, BDynamicArrayRenderHeapAllocator> BRenderIntArray;
typedef BDynamicArray<DWORD,  4, BDynamicArrayRenderHeapAllocator> BRenderDWORDArray;

template<
   class ValueType, 
   uint Alignment                               = ALIGN_OF(ValueType), 
   template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
   template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
>
class BDynamicRenderArray : public BDynamicArray<ValueType, Alignment, BDynamicArrayRenderHeapAllocator, OptionsPolicy, PointerPolicy>
{
public:
   BDynamicRenderArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};

// BSmallDynamicRenderArray's max size/capacity is 65535 objects, but it's 4 bytes smaller than a regular BDynamicArray. (8 for alignment <= 4, 12 otherwise).
template<class ValueType, uint Alignment = ALIGN_OF(ValueType)>
class BSmallDynamicRenderArray : public BDynamicArray<ValueType, Alignment, BDynamicArrayRenderHeapAllocator, BDynamicArraySmallOptions, BDynamicArraySeparatePointers>
{
public:
   BSmallDynamicRenderArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};
