//==============================================================================
//  File: particleheap.h
// 
//  Copyright (c) 2000-2006 Ensemble Studios
//==============================================================================
#pragma once

typedef BMemoryHeap BParticleHeapType;

#ifdef USE_SEPERATE_PARTICLE_HEAP
   extern BParticleHeapType gParticleHeap;
#else
   #define gParticleHeap gRenderHeap
#endif

struct BDynamicArrayParticleHeapAllocatorPolicy { BParticleHeapType& getHeap(void) const { return gParticleHeap; } };

template<class ValueType, uint Alignment> struct BDynamicArrayParticleHeapAllocator  : BDynamicArrayFixedHeapAllocator<ValueType, Alignment, BDynamicArrayParticleHeapAllocatorPolicy> { };

template<
   class ValueType, 
      uint Alignment                               = ALIGN_OF(ValueType), 
      template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
      template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
>
class BDynamicParticleArray : public BDynamicArray<ValueType, Alignment, BDynamicArrayParticleHeapAllocator, OptionsPolicy, PointerPolicy>
{
public:
   BDynamicParticleArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};

//==============================================================================

extern BParticleHeapType gParticleBlockHeap;

struct BDynamicArrayParticleBlockHeapAllocatorPolicy { BParticleHeapType& getHeap(void) const { return gParticleBlockHeap; } };

template<class ValueType, uint Alignment> struct BDynamicArrayParticleBlockHeapAllocator  : BDynamicArrayFixedHeapAllocator<ValueType, Alignment, BDynamicArrayParticleBlockHeapAllocatorPolicy> { };

template<
   class ValueType, 
      uint Alignment                               = ALIGN_OF(ValueType), 
      template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
      template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
>
class BDynamicParticleBlockArray : public BDynamicArray<ValueType, Alignment, BDynamicArrayParticleBlockHeapAllocator, OptionsPolicy, PointerPolicy>
{
public:
   BDynamicParticleBlockArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};
