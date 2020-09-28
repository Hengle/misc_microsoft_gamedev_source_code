//============================================================================
//
//  File: alignedAlloc.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#if defined(new) || defined(delete)
   #error Operators new and delete can't be defined as macros in alignedAlloc.h!
#endif

//============================================================================
// class BAlignedAlloc
//============================================================================
class BAlignedAlloc
{
public:
   // Allocates aligned memory. 
   // alignment must be [1,16] and a power of 2.
   // Free or Delete MUST be used to delete the block.
   // Malloc/New BFATAL_FAIL's on failure.
   // These methods are capitalized in case they are redefined as macros.
   // Note: These methods only impose extra overhead when the specified heap
   // doesn't support aligned allocations.
   static void*                                                               Malloc(uint size, uint alignment = 16, BMemoryHeap& heap = gPrimaryHeap, bool zero = false);   
   static void                                                                Free(void* pData, BMemoryHeap& heap = gPrimaryHeap);

   template<class T> static T*                                                New(uint alignment = ALIGN_OF(T), BMemoryHeap& heap = gPrimaryHeap, bool zero = false);
   template<class T> static T*                                                New(const T& obj, uint alignment = ALIGN_OF(T), BMemoryHeap& heap = gPrimaryHeap, bool zero = false);
   
   template<class T, class A> static T*                                       New(uint alignment, BMemoryHeap& heap, const A& a);
   template<class T, class A, class B> static T*                              New(uint alignment, BMemoryHeap& heap, const A& a, const B& b);
   template<class T, class A, class B, class C> static T*                     New(uint alignment, BMemoryHeap& heap, const A& a, const B& b, const C& c);
   template<class T, class A, class B, class C, class D> static T*            New(uint alignment, BMemoryHeap& heap, const A& a, const B& b, const C& c, const D& d);
   template<class T, class A, class B, class C, class D, class E> static T*   New(uint alignment, BMemoryHeap& heap, const A& a, const B& b, const C& c, const D& d, const E& e);
      
   template<class T> static void                                              Delete(T* p, BMemoryHeap& heap = gPrimaryHeap);
   
   // DeleteArray() MUST be used to delete objects allocated with NewArray.
   template<class T> static T*                                                NewArray(uint num, uint alignment = ALIGN_OF(T), BMemoryHeap& heap = gPrimaryHeap, bool zero = false);
   template<class T> static void                                              DeleteArray(T* p, BMemoryHeap& heap = gPrimaryHeap);
};

//============================================================================
// Macros
//============================================================================
#define ALIGNED_NEW(type, heap)              BAlignedAlloc::New<type>(ALIGN_OF(type), heap)
#define ALIGNED_NEW_INIT(type, heap, init)   BAlignedAlloc::New<type>(ALIGN_OF(type), heap, init)
#define ALIGNED_DELETE(p, heap)              BAlignedAlloc::Delete(p, heap)

#define ALIGNED_NEW_ARRAY(type, num, heap)   BAlignedAlloc::NewArray<type>(num, ALIGN_OF(type), heap)
#define ALIGNED_DELETE_ARRAY(p, heap)        BAlignedAlloc::DeleteArray(p, heap)     

//============================================================================
// Inlines
//============================================================================
#include "alignedAlloc.inl"

