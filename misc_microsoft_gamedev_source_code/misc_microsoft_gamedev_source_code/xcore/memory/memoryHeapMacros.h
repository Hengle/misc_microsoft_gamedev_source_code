//=============================================================================
//
//  memoryHeapMacros.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//=============================================================================
#pragma once

//=============================================================================
// heapNew
//=============================================================================
template<class Type> inline Type* heapNew(BMemoryHeap& heap)
{
   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);
   uint size = Utils::AlignUpValue(sizeof(Type), align);
   
   Type* p = static_cast<Type*>(heap.New(size));
   BVERIFY(p);
   
   BDEBUG_ASSERT(Utils::IsAligned(p, align));  
   
   if (!BIsBuiltInType<Type>::Flag) Utils::ConstructInPlace(p);

   return p;
}

//=============================================================================
// heapNewInit
//=============================================================================
template<class Type> inline Type* heapNewInit(BMemoryHeap& heap, const Type& obj)
{
   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);
   uint size = Utils::AlignUpValue(sizeof(Type), align);
   
   Type* p = static_cast<Type*>(heap.New(size));
   BVERIFY(p);
   
   BDEBUG_ASSERT(Utils::IsAligned(p, align));

   if (!BIsBuiltInType<Type>::Flag) Utils::ConstructInPlace(p, obj);

   return p;
}

//=============================================================================
// heapDelete
//=============================================================================
template<class Type> inline void heapDelete(Type* p, BMemoryHeap& heap)
{
   if (!p)
      return;
      
   if (!BIsBuiltInType<Type>::Flag) Utils::DestructInPlace(p);
   
   bool success = heap.Delete(p);
   success;
   BDEBUG_ASSERT(success);
}

//=============================================================================
// heapNewArray
//=============================================================================
template<class Type> inline Type* heapNewArray(unsigned int num, BMemoryHeap& heap)
{
   BDEBUG_ASSERT((num > 0) && ((num * sizeof(Type)) < 2147483648));
   
   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);
   uint size = align + num * sizeof(Type);

   size = Utils::AlignUpValue(size, align);

   unsigned int* p = static_cast<unsigned int*>(heap.New(size));
   BVERIFY(p);
   *p = num;

   Type* q = reinterpret_cast<Type*>(reinterpret_cast<uchar*>(p) + align);
   
   BDEBUG_ASSERT(Utils::IsAligned(q, align));

   if (!BIsBuiltInType<Type>::Flag) Utils::ConstructArrayInPlace(q, num);
   
   return q;
}

//=============================================================================
// heapDeleteArray
//=============================================================================
template<class Type> inline void heapDeleteArray(Type* p, BMemoryHeap& heap)
{
   if (!p)
      return;
      
   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);

   uint* q = reinterpret_cast<uint*>(reinterpret_cast<uchar*>(p) - align);
   const unsigned int num = *q;
   BDEBUG_ASSERT((num > 0) && ((num * sizeof(Type)) < 2147483648));

   if (!BIsBuiltInType<Type>::Flag) Utils::DestructArrayInPlace(p, num);

   bool success = heap.Delete(q);
   success;
   BDEBUG_ASSERT(success);
}

// All new macros BFATAL_FAIL on failure
#define HEAP_NEW(type, heap)                 heapNew<type>(heap)
#define HEAP_NEW_INIT(type, heap, init)      heapNewInit<type>(heap, init)
#define HEAP_DELETE(p, heap)                 heapDelete(p, heap)

#define HEAP_NEW_ARRAY(type, num, heap)      heapNewArray<type>(num, heap)
#define HEAP_DELETE_ARRAY(p, heap)           heapDeleteArray(p, heap)

// Use heapDelete or HEAP_DELETE to delete objects created by this overload of operator new.
// Example usage:
// uint* p = new(gSimHeap) uint;
// heapDelete(p, gSimHeap);
inline void* operator new  (size_t s, BMemoryHeap& heap) 
{ 
   void* p = heap.New(static_cast<int>(s));
   BVERIFY(p);
   return p;
}
