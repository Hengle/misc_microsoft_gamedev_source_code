//============================================================================
//
//  File: alignedAlloc.inl
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================

//============================================================================
// BAlignedAlloc::New
//============================================================================
template<class T> 
inline T* BAlignedAlloc::New(uint alignment, BMemoryHeap& heap, bool zero)
{
   T* p = reinterpret_cast<T*>(Malloc(sizeof(T), alignment, heap, zero));
   if (!BIsBuiltInType<T>::Flag) Utils::ConstructInPlace(p);
   return p;
}

//============================================================================
// BAlignedAlloc::New
//============================================================================
template<class T> 
inline T* BAlignedAlloc::New(const T& obj, uint alignment, BMemoryHeap& heap, bool zero)
{
   T* p = reinterpret_cast<T*>(Malloc(sizeof(T), alignment, heap, zero));
   Utils::ConstructInPlace(p, obj);
   return p;
}

//============================================================================
// BAlignedAlloc::New
//============================================================================
// I didn't use a macro to repeat New to avoid issues with commas in types.
template<class T, class A> 
inline T* BAlignedAlloc::New(uint alignment, BMemoryHeap& heap, const A& a) 
{ 
   T* p = reinterpret_cast<T*>(Malloc(sizeof(T), alignment, heap, false)); 
   new ((void*)p) T (a); 
   return p;
}

//============================================================================
// BAlignedAlloc::New
//============================================================================
template<class T, class A, class B> 
inline T* BAlignedAlloc::New(uint alignment, BMemoryHeap& heap, const A& a, const B& b) 
{ 
   T* p = reinterpret_cast<T*>(Malloc(sizeof(T), alignment, heap, false)); 
   new ((void*)p) T (a, b); 
   return p; 
}

//============================================================================
// BAlignedAlloc::New
//============================================================================
template<class T, class A, class B, class C> 
inline T* BAlignedAlloc::New(uint alignment, BMemoryHeap& heap, const A& a, const B& b, const C& c) 
{ 
   T* p = reinterpret_cast<T*>(Malloc(sizeof(T), alignment, heap, false)); 
   new ((void*)p) T (a, b, c); 
   return p; 
}

//============================================================================
// BAlignedAlloc::New
//============================================================================
template<class T, class A, class B, class C, class D> 
inline T* BAlignedAlloc::New(uint alignment, BMemoryHeap& heap, const A& a, const B& b, const C& c, const D& d) 
{ 
   T* p = reinterpret_cast<T*>(Malloc(sizeof(T), alignment, heap, false)); 
   new ((void*)p) T (a, b, c, d); 
   return p; 
}

//============================================================================
// BAlignedAlloc::New
//============================================================================
template<class T, class A, class B, class C, class D, class E> 
inline T* BAlignedAlloc::New(uint alignment, BMemoryHeap& heap, const A& a, const B& b, const C& c, const D& d, const E& e) 
{ 
   T* p = reinterpret_cast<T*>(Malloc(sizeof(T), alignment, heap, false)); 
   new ((void*)p) T (a, b, c, d, e); 
   return p; 
}

//============================================================================
// BAlignedAlloc::Delete
//============================================================================
template<class T> inline void BAlignedAlloc::Delete(T* p, BMemoryHeap& heap) 
{
   if (p)
   {
      if (!BIsBuiltInType<T>::Flag)
         Utils::DestructInPlace(p);
      Free(p, heap);
   }
}

//============================================================================
// BAlignedAlloc::NewArray
//============================================================================
template<class T> 
inline T* BAlignedAlloc::NewArray(uint num, uint alignment, BMemoryHeap& heap, bool zero)
{
   BDEBUG_ASSERT((num > 0) && (num < 1024*1024*1024) && (alignment >= 1) && (alignment <= 4096) && Math::IsPow2(alignment));

   const uint padding = (alignment > sizeof(DWORD)) ? (alignment - 1) : 0;
   const uint rawSize = sizeof(T) * num + sizeof(DWORD) * 2 + padding;
   
   uchar* pRaw = reinterpret_cast<uchar*>(heap.New(rawSize));
   if (!pRaw)
   {
      BFATAL_FAIL("BAlignedAlloc: New failed");
   }

   T* pAligned = reinterpret_cast<T*>(Utils::AlignUp(pRaw + sizeof(DWORD) * 2, alignment));
   
   *reinterpret_cast<uchar**>(reinterpret_cast<uchar*>(pAligned) - sizeof(DWORD) * 2) = pRaw;
   *reinterpret_cast<DWORD*>(reinterpret_cast<uchar*>(pAligned) - sizeof(DWORD)) = num;

   if (zero)
      Utils::FastMemSet(pAligned, 0, num*sizeof(T));
      
   if (!BIsBuiltInType<T>::Flag)
      Utils::ConstructArrayInPlace(pAligned, num);
   
   BDEBUG_ASSERT((uint)((reinterpret_cast<uchar*>(pAligned) + (sizeof(T) * num)) - pRaw) <= rawSize);

   return pAligned;
}

//============================================================================
// BAlignedAlloc::DeleteArray
//============================================================================
template<class T> 
inline void BAlignedAlloc::DeleteArray(T* p, BMemoryHeap& heap)
{
   if (!p)
      return;
   
   BDEBUG_ASSERT((reinterpret_cast<DWORD>(p) & 3) == 0);
   
   DWORD num = *reinterpret_cast<DWORD*>(reinterpret_cast<uchar*>(p) - sizeof(DWORD));
   BDEBUG_ASSERT((num > 0) && (num <= 1024*1024*1024));
      
   T* pRaw = *reinterpret_cast<T**>(reinterpret_cast<uchar*>(p) - sizeof(DWORD) * 2);
   BDEBUG_ASSERT(pRaw && (reinterpret_cast<DWORD>(pRaw) >= 65536) && (pRaw < p) && ((reinterpret_cast<DWORD>(pRaw) & 3) == 0) );
   
   if (!BIsBuiltInType<T>::Flag)
      Utils::DestructArrayInPlace(p, num);
   
   const bool success = heap.Delete(pRaw);
   success;
   BDEBUG_ASSERT(success);
}



