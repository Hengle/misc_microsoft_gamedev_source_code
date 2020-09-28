//------------------------------------------------------------------------------------------------------------------------
//  File: allocators.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

//------------------------------------------------------------------------------------------------------------------------
// class BCAllocator
// This class uses the C run time library.
//------------------------------------------------------------------------------------------------------------------------
class BCAllocator
{
public:
   enum 
   { 
      cCanResizeBuf = true
   };

   // Returns NULL on failure      
   void* alloc(uint size, uint* pActualSize = NULL, bool zero = false)
   {
      void* p = alignedMalloc(size);
      if (!p)
         return NULL;
            
      if (pActualSize)
         *pActualSize = static_cast<int>(alignedMSize(p));
      
      if (zero)
      {
#ifdef XBOX      
         XMemSet(p, 0, size);         
#else
         memset(p, 0, size);
#endif                  
      }
      
      return p;
   }

   // Not named "free" because this could be a macro!
   void dealloc(void* p)
   {
      alignedFree(p);
   }

   // Returns NULL on failure      
   void* resize(void* p, uint newSize, bool move = true, uint* pActualSize = NULL)
   {
      void* q = alignedRealloc(p, newSize, 0, move);
                  
      if (!q)
         return NULL;

      if (pActualSize)
         *pActualSize = static_cast<uint>(alignedMSize(q));

      return q;   
   }
   
   uint getSize(void* p)
   {
      return alignedMSize(p);
   }

   void swap(BCAllocator& other)
   {
      other;
   }
   
   bool operator== (const BCAllocator& other) const
   {
      other;
      return true;
   }
};

extern __declspec(selectany) BCAllocator gCAllocator;

//------------------------------------------------------------------------------------------------------------------------
// class BHeapAllocator
// This class permits each container to use any heap, but it bloats the container by 4-bytes.
//------------------------------------------------------------------------------------------------------------------------
class BHeapAllocator
{
   BMemoryHeap* mpHeap;

public:
   enum 
   { 
      cCanResizeBuf = true
   };
      
   BHeapAllocator(BMemoryHeap& heap = gPrimaryHeap) :
      mpHeap(&heap)
   {
   }
   
   explicit BHeapAllocator(BMemoryHeap* pHeap) :
      mpHeap(pHeap)
   {
   }

   BHeapAllocator(const BHeapAllocator& other) :
      mpHeap(other.getHeap())
   {  
   }

   BHeapAllocator& operator= (const BHeapAllocator& other)
   {
      mpHeap = other.getHeap();
      return *this;
   }

   BMemoryHeap* getHeap(void) const { return mpHeap; }

   void setHeap(BMemoryHeap* pHeap) { mpHeap = pHeap;  }      

   // Returns NULL on failure      
   void* alloc(uint size, uint* pActualSize = NULL, bool zero = false)
   {
      int space;
      void* p = mpHeap->New(size, &space, zero);
      
      if (!p)
         return NULL;
         
      if (pActualSize)
         *pActualSize = static_cast<uint>(space);
         
      return p;
   }

   void dealloc(void* p)
   {
      const bool success = mpHeap->Delete(p);
      success;
      BDEBUG_ASSERT(success);
   }

   // Returns NULL on failure      
   void* resize(void* p, uint newSize, bool move = true, uint* pActualSize = NULL)
   {
      void* q;
      
      int actualSize;
      
      if (!p)
         q = mpHeap->New(newSize, &actualSize);
      else
         q = mpHeap->Resize(p, newSize, move, &actualSize);
      
      if (!q)
         return NULL;

      if (pActualSize)
         *pActualSize = static_cast<uint>(actualSize);

      return q;   
   }
   
   uint getSize(void* p)
   {
      int size;
      const bool success = mpHeap->Details(p, &size);
      success;
      BDEBUG_ASSERT(success);
      return size;
   }

   void swap(BHeapAllocator& other)
   {
      std::swap(mpHeap, other.mpHeap);
   }
   
   bool operator== (const BHeapAllocator& other) const
   {
      return mpHeap == other.mpHeap;
   }
};

//------------------------------------------------------------------------------------------------------------------------
// class BFixedHeapAllocator
// This class uses a policy class to retrieve the heap object. Intended to be used to create allocators that use
// specific global heaps. The base type must provide a getHeap() method, and should be empty.
//------------------------------------------------------------------------------------------------------------------------
template<typename GetHeapPolicy>
class BFixedHeapAllocator : public GetHeapPolicy
{
public:
   enum 
   { 
      cCanResizeBuf = true
   };
   
   // Returns NULL on failure         
   void* alloc(uint size, uint* pActualSize = NULL, bool zero = false)
   {
      int space;
      void* p = getHeap().New(size, &space, zero);

      if (!p)
      {
         BFATAL_FAIL("New failed");
      }

      if (pActualSize)
         *pActualSize = static_cast<uint>(space);

      return p;
   }

   void dealloc(void* p)
   {
      const bool success = getHeap().Delete(p);
      success;
      BDEBUG_ASSERT(success);
   }

   // Returns NULL on failure      
   void* resize(void* p, uint newSize, bool move = true, uint* pActualSize = NULL)
   {
      void* q;

      int actualSize;

      if (!p)
         q = getHeap().New(newSize, &actualSize);
      else
         q = getHeap().Resize(p, newSize, move, &actualSize);

      if (!q)
         return NULL;

      if (pActualSize)
         *pActualSize = static_cast<uint>(actualSize);

      return q;   
   }
   
   uint getSize(void* p)
   {
      int size;
      const bool success = getHeap().Details(p, &size);
      BDEBUG_ASSERT(success);
      return size;
   }

   void swap(BFixedHeapAllocator& other)
   {
   }
   
   bool operator==(const BFixedHeapAllocator& other) const
   {
      return true;
   }
};

//------------------------------------------------------------------------------------------------------------------------
struct BGetCRunTimeHeapPolicy { BMemoryHeap&       getHeap(void) const { return gCRunTimeHeap; } };
struct BGetPrimaryHeapPolicy  { BMemoryHeap&       getHeap(void) const { return gPrimaryHeap; } };
struct BGetSimHeapPolicy      { BMemoryHeap&       getHeap(void) const { return gSimHeap; } };
struct BGetRenderHeapPolicy   { BMemoryHeap&       getHeap(void) const { return gRenderHeap; } };

struct BCRunTimeFixedHeapAllocator  : BFixedHeapAllocator<BGetCRunTimeHeapPolicy>  { };
struct BPrimaryFixedHeapAllocator   : BFixedHeapAllocator<BGetPrimaryHeapPolicy>  { };
struct BSimFixedHeapAllocator       : BFixedHeapAllocator<BGetSimHeapPolicy>      { };
struct BRenderFixedHeapAllocator    : BFixedHeapAllocator<BGetRenderHeapPolicy>   { };

extern __declspec(selectany) BCRunTimeFixedHeapAllocator gCRunTimeFixedHeapAllocator;
extern __declspec(selectany) BPrimaryFixedHeapAllocator  gPrimaryFixedHeapAllocator;
extern __declspec(selectany) BSimFixedHeapAllocator      gSimHeapFixedHeapAllocator;
extern __declspec(selectany) BRenderFixedHeapAllocator   gRenderFixedHeapAllocator;
//------------------------------------------------------------------------------------------------------------------------
// allocatorNew
//------------------------------------------------------------------------------------------------------------------------
template<class Type, class Allocator> inline Type* allocatorNew(Allocator& allocator)
{
   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);
   uint size = Utils::AlignUpValue(sizeof(Type), align);
   
   Type* p = static_cast<Type*>(allocator.alloc(size));
   if (!p)
   {
      BFATAL_FAIL("allocatorNew: alloc failed");
   }

   if (!BIsBuiltInType<Type>::Flag) Utils::ConstructInPlace(p);
   
   BDEBUG_ASSERT(Utils::IsAligned(p, align));

   return p;
}

//------------------------------------------------------------------------------------------------------------------------
// allocatorNewInit
//------------------------------------------------------------------------------------------------------------------------
template<class Type, class Allocator> inline Type* allocatorNewInit(Allocator& allocator, const Type& obj)
{
   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);
   uint size = Utils::AlignUpValue(sizeof(Type), align);
   
   Type* p = static_cast<Type*>(allocator.alloc(size));
   if (!p)
   {
      BFATAL_FAIL("allocatorNewInit: alloc failed");
   }

   if (!BIsBuiltInType<Type>::Flag) Utils::ConstructInPlace(p, obj);
   
   BDEBUG_ASSERT(Utils::IsAligned(p, align));

   return p;
}

//------------------------------------------------------------------------------------------------------------------------
// allocatorDelete
//------------------------------------------------------------------------------------------------------------------------
template<class Type, class Allocator> inline void allocatorDelete(Type* p, Allocator& allocator)
{
   if (!p)
      return;
      
   if (!BIsBuiltInType<Type>::Flag) Utils::DestructInPlace(p);
   
   allocator.dealloc(p);
}

//------------------------------------------------------------------------------------------------------------------------
// allocatorNewArray
//------------------------------------------------------------------------------------------------------------------------
template<class Type, class Allocator> inline Type* allocatorNewArray(uint num, Allocator& allocator)
{
   BDEBUG_ASSERT((num > 0) && ((num * sizeof(Type)) < 2147483648));
   
   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);
   uint size = align + num * sizeof(Type);
   
   size = Utils::AlignUpValue(size, align);
   
   uint* p = static_cast<uint*>(allocator.alloc(size));
   if (!p)
   {
      BFATAL_FAIL("allocatorNewArray: alloc failed");
   }
         
   *p = num;
   
   Type* q = reinterpret_cast<Type*>(reinterpret_cast<uchar*>(p) + align);
   
   BDEBUG_ASSERT(Utils::IsAligned(q, align));
   
   if (!BIsBuiltInType<Type>::Flag) Utils::ConstructArrayInPlace(q, num);        

   return q;
}

//------------------------------------------------------------------------------------------------------------------------
// allocatorDeleteArray
//------------------------------------------------------------------------------------------------------------------------
template<class Type, class Allocator> inline void allocatorDeleteArray(Type* p, Allocator& allocator)
{
   if (!p)
      return;

   const uint align = (__alignof(Type) >= sizeof(DWORD)) ? __alignof(Type) : sizeof(DWORD);
   
   uint* q = reinterpret_cast<uint*>(reinterpret_cast<uchar*>(p) - align);
   const uint num = *q;
   BDEBUG_ASSERT((num > 0) && ((num * sizeof(Type)) < 2147483648));
   
   if (!BIsBuiltInType<Type>::Flag) Utils::DestructArrayInPlace(p, num);        
   
   allocator.dealloc(q);
}

// All new macros BFATAL_FAIL on failure

#define ALLOCATOR_NEW(type, allocator)                   allocatorNew<type>(allocator)
#define ALLOCATOR_NEW_INIT(type, allocator, init)        allocatorNewInit<type>(allocator, init)
#define ALLOCATOR_DELETE(p, allocator)                   allocatorDelete(p, allocator)

#define ALLOCATOR_NEW_ARRAY(type, num, allocator)        allocatorNewArray<type>(num, allocator)
#define ALLOCATOR_DELETE_ARRAY(p, allocator)             allocatorDeleteArray(p, allocator)
