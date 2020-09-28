//==============================================================================
//
// dynamicArray.h
// 
// Copyright (c) 2003-2007, Ensemble Studios
//
//==============================================================================
#pragma once

// Disable PREfast warnings
#pragma warning( push )
#pragma warning( disable : 25037 25038 25039 25040 25041 25042 )

#ifdef BUILD_FINAL
   #undef DYNAMIC_ARRAY_TRACKING 
   #define DYNAMIC_ARRAY_TRACKING 0
#endif

#if DYNAMIC_ARRAY_TRACKING
#pragma warning(disable:4355) // this used in base memory initialize list
#include "containers\linkedList.h"

class BDynamicArrayTracker
{
   BDynamicArrayTracker();
   BDynamicArrayTracker& operator= (const BDynamicArrayTracker&);
   
public:
   typedef void (*BGetArrayStateCallbackPtr)(void* pArray, uint& typeSize, uint& size, uint& capacity, void*& pAllocation);
   
   typedef BDynamicArrayTracker* BDynamicArrayTrackerPtr;
   typedef BLinkedList<BDynamicArrayTrackerPtr> BDynamicArrayTrackerList;
   
   BDynamicArrayTracker(void* pArray, BGetArrayStateCallbackPtr pGetState) :
      mMagic(0),
      mpArray(pArray),
      mpGetState(pGetState),
      mThreadID(GetCurrentThreadId()),
      mpListEntry(NULL)
   {
   }      
   
   ~BDynamicArrayTracker()
   {
      BDEBUG_ASSERT(!mpListEntry);
   }
   
   void* getArray(void) const { BASSERT(mMagic == cMagic); return mpArray; }
   BGetArrayStateCallbackPtr getGetStateCallback(void) const { BASSERT(mMagic == cMagic); return mpGetState; }
   DWORD getThreadID(void) const { BASSERT(mMagic == cMagic); return mThreadID; }
   
   bool getState(uint& typeSize, uint& size, uint& capacity, void*& pAllocation) const
   {
      MemoryBarrier();
      
      BDEBUG_ASSERT(mMagic == cMagic);

      // This is not thread safe, but it's only for informational purposes!
      __try
      {
         mpGetState(mpArray, typeSize, size, capacity, pAllocation); 
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         return false;
      }

#ifdef BUILD_DEBUG      
      MemoryBarrier();
      
      BDEBUG_ASSERT(mMagic == cMagic);
#endif      
      
      return true;
   }
   
   void add(void)
   {
      BScopedCriticalSection lock(getTrackerListMutex());
      
      BDEBUG_ASSERT(!mpListEntry);
      
      mMagic = cMagic;
      
      mpListEntry = getTrackerList().pushBack(this).getItem();
   }
   
   void remove(void)
   {
      BScopedCriticalSection lock(getTrackerListMutex());
      
      BDEBUG_ASSERT(mpListEntry);
      
      mMagic = 0;
      
      getTrackerList().remove(getTrackerList().getIterator(mpListEntry));
      mpListEntry = NULL;
   }

   struct BStats
   {
      uint mTotalArrays;

      uint mTotalSize;
      uint mTotalSizeInBytes;

      uint mTotalCapacity;
      uint mTotalCapacityInBytes;
   };   
   static void getStatistics(BStats& stats);
   
private:
   enum { cMagic = 0x7743E597 };
   DWORD                      mMagic;
   void*                      mpArray;
   BGetArrayStateCallbackPtr  mpGetState;
   DWORD                      mThreadID;
   BDynamicArrayTrackerList::valueType* mpListEntry;
   
   static BDynamicArrayTrackerList& getTrackerList(void)
   {
      static BDynamicArrayTrackerList trackerList;
      return trackerList;
   }
   
   static BCriticalSection& getTrackerListMutex(void)
   {
      static BCriticalSection mutex;
      return mutex;
   }
};

#endif

// ------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment> struct BDynamicArrayPrimaryHeapAllocator;

#define BDynamicArrayDefaultAllocator BDynamicArrayPrimaryHeapAllocator

// ------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArrayDefaultOptions
{
   // cUseConstructorDestructor   - If true, the object's constructor/destructors are called to initialize/free objects, otherwise
   //                              the object is assumed to contain "plain old data" (i.e. memcpy'able).
   // cClearNewObjects            - If true, memory for new objects will be cleared to 0's before construction. If construction is disabled, new objects 
   //                              are just cleared to 0's.
   // cGrowable                   - If false, the container will BFAIL on any insert/push operation that would cause the container to reallocate.
   //                              Manual reallocations will not fail.
   // cUseCopyConstructor         - If true, new elements will be initialized by calling their copy constructor. Otherwise, new elements are initialized by calling
   //                               their default constructor.
   enum 
   {
      cUseConstructorDestructor  = true, 
      cGrowable                  = true, 
      cClearNewObjects           = false,
      cUseCopyConstructor        = true,
   };

   typedef uint SizeType;
};

// ------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArrayDefaultConstructorOptions : BDynamicArrayDefaultOptions<ValueType, Alignment>
{
   enum 
   {
      cUseCopyConstructor        = false
   };
};

// ------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArrayNoGrowOptions : BDynamicArrayDefaultOptions<ValueType, Alignment>
{
   enum 
   {
      cGrowable                  = false
   };
};

// ------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArrayNoConstructOptions : BDynamicArrayDefaultOptions<ValueType, Alignment>
{
   enum 
   {
      cUseConstructorDestructor  = false,
      cGrowable                  = true, 
      cClearNewObjects           = false, 
   };
};

// ------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArrayNoConstructNoGrowClearNewOptions : BDynamicArrayDefaultOptions<ValueType, Alignment>
{
   enum 
   {
      cUseConstructorDestructor  = false,
      cGrowable                  = false, 
      cClearNewObjects           = true, 
   };
};

// ------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArraySmallOptions : BDynamicArrayDefaultOptions<ValueType, Alignment>
{
   typedef ushort SizeType;
};

//------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArraySeparatePointers
{
   BDynamicArraySeparatePointers() :
      mpRaw(NULL),
      mpAligned(NULL)
   {
   }
   
   enum 
   { 
      cHasSeparateAlignedPtr = true 
   };
   
   void swap(BDynamicArraySeparatePointers<ValueType, Alignment>& other)
   {
      std::swap(mpRaw, other.mpRaw);
      std::swap(mpAligned, other.mpAligned);
   }

   void* mpRaw;
   ValueType* mpAligned;
};

//------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
struct BDynamicArraySinglePointer
{
   BDynamicArraySinglePointer() :
      mpRawPtr(NULL)
   {
   }

   enum 
   { 
      cHasSeparateAlignedPtr = false 
   };

   __declspec(property(get = getRaw, put = putRaw))          void* mpRaw;
   __declspec(property(get = getAligned, put = putNothing))  ValueType* mpAligned;
   
   void swap(BDynamicArraySinglePointer<ValueType, Alignment>& other)
   {
      std::swap(mpRawPtr, other.mpRawPtr);
   }

protected:
   ValueType* mpRawPtr;

   inline void* getRaw(void) const   { return mpRawPtr; }      
   inline ValueType* getAligned(void) const   { return mpRawPtr; }      
   inline void  putRaw(void* p)      { mpRawPtr = static_cast<ValueType*>(p); }      
   inline void  putNothing(void* p)  { p; }
};

template<class ValueType> struct BDynamicArraySeparatePointers<ValueType, 4> : BDynamicArraySinglePointer<ValueType, 4> { };
template<class ValueType> struct BDynamicArraySeparatePointers<ValueType, 2> : BDynamicArraySinglePointer<ValueType, 2> { };
template<class ValueType> struct BDynamicArraySeparatePointers<ValueType, 1> : BDynamicArraySinglePointer<ValueType, 1> { };

// ------------------------------------------------------------------------------------------------------------------------

enum BDynamicArrayAllocOp { cDAAlloc, cDAFree, cDAResize };
typedef void* (BDynamicArrayAllocCallback)(void* pArray, BDynamicArrayAllocOp op, void* p, uint prevNumObjects, uint newNumObjects, int minNumObjects, uint* pNewBufSizeInBytes);

enum BDynamicArrayEntryOp { cDAConstruct, cDADestruct, cDAMove };   
typedef void (BDynamicArrayEntryCallback)(void* pArray, BDynamicArrayEntryOp op, uint first, uint num, void* pDst, void* pSrc);

void DynamicArrayChangeCapacity(
   void* pArrayPtr, uint64 arrayAttributes, BDynamicArrayAllocCallback pAllocFunc, BDynamicArrayEntryCallback pEntryFunc,
   uint newCapacity, bool tryResizing = true, uint minNewCapacity = 0);
                                
void DynamicArrayMakeRoom(
   void* pArrayPtr, uint64 arrayAttributes, BDynamicArrayAllocCallback pAllocFunc, BDynamicArrayEntryCallback pEntryFunc,
   uint numNewEntries, bool growableCheck);

void DynamicArrayResize(
   void* pArrayPtr, uint64 arrayAttributes, BDynamicArrayAllocCallback pAllocFunc, BDynamicArrayEntryCallback pEntryFunc,
   uint newSize, bool growableCheck);
                        
#ifdef XBOX
   #define dynamicArrayMemCpy XMemCpy
   #define dynamicArrayMemSet XMemSet
#else
   #define dynamicArrayMemCpy memcpy
   #define dynamicArrayMemSet memset
#endif

// ------------------------------------------------------------------------------------------------------------------------
// class BDynamicArray
// ValueType                  - Element type
// Alignment                  - Power of 2 alignment. Alignment is always guaranteed, no matter what the behavior of the underlying heap.
//
// Important Notes:         
// Like std::vector, this class destructs objects as soon as they are removed from the container! In other words,
// only the currently valid objects (i.e. objects with indices 0 - (size() - 1)) are actually live and valid. The others
// will ALWAYS be destructed when they are no longer part of the array as soon as possible.
// This container fully supports nesting of other containers, "jagged" arrays, etc.
// DO NOT memset/XMemSet the contents of this structure! 
// ------------------------------------------------------------------------------------------------------------------------
// If you change the template params here, be sure to update stream.h! 
template<
   class ValueType, 
   uint Alignment                               = ALIGN_OF(ValueType), 
   template <class, uint>  class Allocator      = BDynamicArrayDefaultAllocator,
   template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
   template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
   >
class BDynamicArray : public PointerPolicy<ValueType, Alignment>, Allocator<ValueType, Alignment>
{
public:
   typedef OptionsPolicy<ValueType, Alignment>  optionsPolicy;
   typedef Allocator<ValueType, Alignment>      allocator;
   typedef PointerPolicy<ValueType, Alignment>  pointerPolicy;
   
   typedef ValueType                            valueType;
   typedef ValueType*                           iterator;
   typedef const ValueType*                     constIterator;
   
   // For backwards compatibility with std::vector
   typedef ValueType                            value_type;
   typedef const ValueType*                     const_iterator;
         
   enum 
   {
      cUseConstructorDestructor  = OptionsPolicy<ValueType, Alignment>::cUseConstructorDestructor,
      cGrowable                  = OptionsPolicy<ValueType, Alignment>::cGrowable,
      cClearNewObjects           = OptionsPolicy<ValueType, Alignment>::cClearNewObjects,
      cUseCopyConstructor        = OptionsPolicy<ValueType, Alignment>::cUseCopyConstructor
   };
   
   enum { alignment = Alignment };   
   
#define COMPUTE_ATTRIBS  \
   ((uint64)(sizeof(ValueType) | \
   (cUseConstructorDestructor << 20U) |  \
   (cUseCopyConstructor << 21U) | \
   (cClearNewObjects << 22U) | \
   (cCanResizeBuf << 23U) | \
   (cGrowable << 24U) | \
   ((sizeof(OptionsPolicy<ValueType, Alignment>::SizeType) > sizeof(ushort)) << 25U) | \
   ((sizeof(PointerPolicy<ValueType, Alignment> ) > sizeof(ValueType*)) << 26U) | \
   (sizeof(Allocator<ValueType, Alignment> ) << 27U))) | \
   (((uint64)Alignment) << 32U) | \
   (((uint64)sizeof(BDynamicArray)) << 40U)
      
   BDynamicArray() 
#if DYNAMIC_ARRAY_TRACKING
      : mTracker(this, getStateCallBack)
#endif      
   {
      mSize = 0;
      mCapacity = 0;
                  
#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif
   }
                              
   // initialSize: initial number of constructed entries
   // initialCapacity: initial size of heap allocation in entries (capacity should always be >= size)
   BDynamicArray(uint initialSize, const allocator& alloc = allocator()) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif      
   {
      mSize = 0;
      mCapacity = 0;
                        
      if (initialSize)
         resize(initialSize);

#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif
   }
   
   BDynamicArray(uint initialSize, uint initialCapacity, const allocator& alloc = allocator()) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif      
   {
      mSize = 0;
      mCapacity = 0;
                  
      if (initialCapacity)
         DynamicArrayChangeCapacity(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, Math::Max(initialSize, initialCapacity));
      
      if (initialSize)
         resize(initialSize);

#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif
   }
   
   template<class T, uint A>
   BDynamicArray(uint initialSize, const Allocator<T, A>& alloc) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif            
   {
      mSize = 0;
      mCapacity = 0;
            
      if (initialSize)
         resize(initialSize);

#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif         
   }
   
   template<class T, uint A>
   BDynamicArray(uint initialSize, uint initialCapacity, const Allocator<T, A>& alloc) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif            
   {
      mSize = 0;
      mCapacity = 0;
            
      if (initialCapacity)
         DynamicArrayChangeCapacity(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, Math::Max(initialSize, initialCapacity));
         
      if (initialSize)
         resize(initialSize);

#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif         
   }
  
   BDynamicArray(const allocator& alloc) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif            
   {
      mSize = 0;
      mCapacity = 0;
            
#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif      
   } 
   
   template<class T, uint A>
   BDynamicArray(const Allocator<T, A>& alloc) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif            
   {
      mSize = 0;
      mCapacity = 0;
            
#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif      
   } 
   
   explicit BDynamicArray(uint numVals, const ValueType* pVals, const allocator& alloc = allocator()) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif            
   {
      mSize = 0;
      mCapacity = 0;
                  
      pushBack(pVals, numVals);

#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif      
   }
   
   explicit BDynamicArray(const ValueType* pVals, const ValueType* pEndVals, const allocator& alloc = allocator()) :
      Allocator<ValueType, Alignment>(alloc),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif      
   {
      mSize = 0;
      mCapacity = 0;
            
      BDEBUG_ASSERT(pEndVals >= pVals);

      pushBack(pVals, pEndVals - pVals);

#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif      
   }
   
   BDynamicArray(const BDynamicArray& other) :
      Allocator<ValueType, Alignment>(other.getAllocator()),
      PointerPolicy<ValueType, Alignment>()
#if DYNAMIC_ARRAY_TRACKING
      , mTracker(this, getStateCallBack)
#endif            
   {
      mSize = 0;
      mCapacity = 0;
      
      *this = other;
      
#if DYNAMIC_ARRAY_TRACKING         
      mTracker.add();
#endif      
   }
   
   ~BDynamicArray()
   {
      BCOMPILETIMEASSERT(sizeof(ValueType) <= 0xFFFFFU);
      BCOMPILETIMEASSERT((Alignment > 0) && ((Alignment & -(int)Alignment) == Alignment));
      BCOMPILETIMEASSERT(Alignment <= 255U);
      BCOMPILETIMEASSERT(sizeof( Allocator<ValueType, Alignment> ) <= 31);
      
#if DYNAMIC_ARRAY_TRACKING         
      mTracker.remove();
#endif      
		
      //if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
         destructEntries(0, mSize);
      
      freeBuf(mpRaw, mCapacity);
      
#ifdef BUILD_DEBUG
      memset(this, 0xFD, sizeof(*this));
#endif      
   }
         
   // DO NOT CALL THIS UNLESS YOU INTEND TO FREE THE MEMORY USED BY THE DESTINATION ARRAY FIRST.
   // Call assignNoDealloc instead.
   BDynamicArray& operator= (const BDynamicArray& rhs)
   {
      if (this == &rhs)
         return *this;
         
      clear();
      
      allocator::operator= (static_cast<const allocator&>(rhs));
                        
      if (!rhs.empty())
         pushBack(rhs.getData(), rhs.size());
      
      return *this;
   }

   // Assigns one array to another without freeing its memory first
   BDynamicArray& assignNoDealloc(const BDynamicArray& rhs)
   {
      if (this == &rhs)
         return *this;
         
      setNumber(0);
      
      allocator::operator= (static_cast<const allocator&>(rhs));
                        
      if (!rhs.empty())
         pushBack(rhs.getData(), rhs.size());
      
      return *this;
   }
   
   template<class OtherValueType, uint OtherAlignment, template <class, uint> class OtherAllocator, template <class, uint> class OtherOptionsPolicy, template <class, uint> class OtherPointerPolicy>
   BDynamicArray& operator= (const BDynamicArray<OtherValueType, OtherAlignment, OtherAllocator, OtherOptionsPolicy, OtherPointerPolicy>& rhs)
   {
      if ((const void*)this == (const void*)&rhs)
         return *this;
         
      clear();
      
      if (!rhs.empty())
      {
         reserve(rhs.getSize());
         for (uint i = 0; i < rhs.getSize(); i++)
         {
            pushBack(rhs[i]);
         }
      }            
      
      return *this;
   }
   
   inline const allocator& getAllocator(void) const { return *this; }
   
   inline allocator& getAllocator(void) { return *this; }
   
   inline void setAllocator(allocator& alloc) 
   { 
      BDEBUG_ASSERT(!getPtr());
      Allocator<ValueType, Alignment>::operator= (alloc);
   }
   
   // true if the container is empty (no constructed objects).
   // Deprecated - use isEmpty() instead!
   inline bool empty(void) const { return 0 == mSize; }          
   
   inline bool isEmpty(void) const { return 0 == mSize; }          
   
   // Number of objects present in container. 
   // This value is the number of objects in a constructed state.
   inline uint size(void) const { return mSize; }
   
   inline uint getSize(void) const { return mSize; }
   
   // Returns the total size in bytes of the objects present in the container.
   inline uint getSizeInBytes(void) const { return mSize * sizeof(ValueType); }
   
   // Current capacity (allocated size) of container.
   // This value is the maximum number of objects that can be placed into the container without growing.
   inline uint capacity(void) const { return mCapacity; }
   
   inline uint getCapacity(void) const { return mCapacity; }
   
   // Destructs all entries then deallocates the memory block.
   void clear(void) { DynamicArrayChangeCapacity(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, 0); }
                              
   // Changes the number of live objects in container.
   // If growableCheck is true, BFAIL will be called if cGrowable is false and the container tries to reallocate.
   // resize(0) does not free the allocated memory block! Call clear() instead.
   inline void resize(uint newSize, bool growableCheck = false) 
   {
      DynamicArrayResize(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, newSize, growableCheck);
   }
   
   // Change's the container capacity.
   // Memory backing current objects is always preserved.
   // If 0 is specified, the container's memory block is shrunk to the smallest possible size.
   inline void reserve(uint newCapacity)
   {
      DynamicArrayChangeCapacity(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, Math::Max(mSize, newCapacity));
   }
   
   ValueType* enlarge(uint num)
   {
      const uint curSize = size();
      resize(size() + num);
      if (!getPtr())
         return NULL;
      return getPtr() + curSize;
   }
   
   // Adds object to container
   inline void pushBack(const ValueType& val)
   {
      // Not safe to push an object from within the container.
      BDEBUG_ASSERT( (!mpAligned) || (&val < mpAligned) || (&val >= (mpAligned + mSize)));
         
      //if (roomLeft() < 1)
      if (mSize == mCapacity)
         DynamicArrayMakeRoom(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, 1, true);

      if (BIsBuiltInType<ValueType>::Flag)
      {
         // This may be a dangerous optimization.
         mpAligned[mSize] = val;
      }
      else if (cUseConstructorDestructor)
      {
         if (cClearNewObjects)
            dynamicArrayMemSet(&mpAligned[mSize], 0, sizeof(ValueType));
            
         Utils::ConstructInPlace(&mpAligned[mSize], val);
      }
      else 
      {
         if (sizeof(ValueType) <= 16)
            memcpy(&mpAligned[mSize], &val, sizeof(ValueType));
         else
            dynamicArrayMemCpy(&mpAligned[mSize], &val, sizeof(ValueType));
      }            
      
      mSize++;
   }
      
   // Helps port code that uses std::vector.
   inline void push_back(const ValueType& val) { pushBack(val); }
               
   // Places num objects at end of container
   inline void pushBack(const ValueType* pVals, uint num)
   {
      if (!num)
         return;
      
      BDEBUG_ASSERT(pVals);
      
      if (roomLeft() < num)
         DynamicArrayMakeRoom(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, num, true);
      
      copyElements(num, mpAligned + mSize, pVals);
            
      mSize += num;
      BDEBUG_ASSERT(mSize <= mCapacity);
   }
   
   inline void append(const BDynamicArray& other)
   {
      if (other.size())
         pushBack(&other[0], other.size());
   }
         
   // Removes last object from container
   inline void popBack(void)
   {
      BDEBUG_ASSERT(mSize > 0);
      if (mSize)
      {  
         destructEntries(mSize - 1, 1);
         mSize--;
      }
   }
         
   // Returns pointer to first object - may return NULL if no memory allocated to container
   inline const ValueType* getData(void) const   { return mpAligned; }
   inline       ValueType* getData(void)         { return mpAligned; }
      
   // Returns pointer to raw memory block - may be unaligned!
   inline const void* getRawData(void) const   { return mpRaw; }
   inline       void* getRawData(void)         { return mpRaw; }
   
   // Object accessor
   inline const ValueType& operator[] (uint i) const { BDEBUG_ASSERT(i < mSize); return mpAligned[i]; }
   
   // Object accessor
   inline ValueType& operator[] (uint i) { BDEBUG_ASSERT(i < mSize); return mpAligned[i]; }
   
   // Object accessor
   inline const ValueType& at (uint i) const { BDEBUG_ASSERT(i < mSize); return mpAligned[i]; }

   // Object accessor
   inline ValueType& at (uint i) { BDEBUG_ASSERT(i < mSize); return mpAligned[i]; }
   
   // Very unsafe! Don't use unless you know what you're doing, because the created object will not be constructed!
   // The returned pointer can be used to initialize the object. This method of inserting new objects into the container avoids 
   // calling the copy constructor on each insertion.
   inline ValueType* pushBackNoConstruction(uint num)
   {
      if (roomLeft() < num)
         DynamicArrayMakeRoom(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, num, true);

      // rg [1/14/05] - The caller wants no construction, so let them clear if they want to.
      //if (cClearNewObjects)
      //   memset(&mpAligned[mSize], 0, num * sizeof(ValueType));

      ValueType* pRet = &mpAligned[mSize];

      mSize += num;
      BDEBUG_ASSERT(mSize <= mCapacity);
      
      return pRet;
   }

   // Returns reference to first element in container.
   inline const ValueType& front(void) const  { BDEBUG_ASSERT(!empty()); return (*this)[0]; }
   inline       ValueType& front(void)        { BDEBUG_ASSERT(!empty()); return (*this)[0]; }
   
   // Returns reference to the last element in container.
   inline const ValueType& back(void) const   { BDEBUG_ASSERT(!empty()); return (*this)[mSize - 1]; }
   inline       ValueType& back(void)         { BDEBUG_ASSERT(!empty()); return (*this)[mSize - 1]; }

   // Returns a pointer to the first element in the container.
   inline const ValueType* begin(void) const  { return mpAligned; }            
   inline       ValueType* begin(void)        { return mpAligned; }                  
   
   // Returns a pointer to the element one after the last element in the container.     
   inline const ValueType* end(void) const  { return mpAligned + mSize; }            
   inline       ValueType* end(void)        { return mpAligned + mSize; }                  

   // Returns a pointer to the first element in the container, or NULL if the container has no memory backing it.
   // Be forewarned that if you insert new elements into the container the returned pointers may change!
   inline const ValueType* getPtr(void) const  { return mpAligned; }            
   inline       ValueType* getPtr(void)        { return mpAligned; }                           
        
   // begin is the first element to erase.
   // end points one beyond the last element to erase.
   // end must be greater or equal to begin, if they are equal nothing is erased.
   void erase(uint begin, uint end)
   {
      if (begin == end)
         return;
         
      BDEBUG_ASSERT(end > begin);
      debugRangeCheck(begin, mSize);
      debugRangeCheckIncl(end, mSize);
      
      const uint numEntriesDeleted = end - begin;
      const uint numEntriesToCopy = mSize - end;
      
      if (numEntriesToCopy)
      {
         if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
         {
            for (uint i = 0; i < numEntriesToCopy; i++)
               (*this)[begin + i] = (*this)[end + i];
         }
         else
         {
            memmove(&mpAligned[begin], &mpAligned[end], numEntriesToCopy * sizeof(ValueType));
         }
      }
                  
      resize(mSize - numEntriesDeleted);
   }
   
   inline void erase(uint index)
   {
      erase(index, index + 1);
   }
   
   // This method erases the specified element without moving all subsequent entries. 
   // It replaces the erased element with the last.
   void eraseUnordered(uint index)
   {
      BDEBUG_ASSERT(index < size());

      if ((index + 1) != size())
         (*this)[index] = (*this)[size() - 1];

      popBack();
   }
         
   // Insert num entries starting at position pos. Current entries will be moved to make room.
   // If pos == size(), this is equivalent to calling pushBack(), or resizing the container.
   // If pObjects is NULL, entries are inserted and constructed using the value type's default constructor.
   void insert(uint pos, uint num, const ValueType* pObjects)
   {
      if (!num)
         return;
         
      if (pos > size())
      {
         BDEBUG_ASSERT(0);
         return;
      }
      else if (pos == size())
      {
         // Special case if inserting at the end.
         if (pObjects)
            pushBack(pObjects, num);
         else
            resize(size() + num, true);
         return;
      }
      
      // Make room for new elements.
      const uint origSize = mSize;
      const uint newSize = mSize + num;
      if (roomLeft() < num)
         DynamicArrayMakeRoom(this, COMPUTE_ATTRIBS, allocCallback, entryCallback, num, true);
      
      mSize = newSize;
      
      uint srcIndex = pos;
      uint dstIndex = pos + num;
      uint srcNum = origSize - pos;
      
      BDEBUG_ASSERT((srcNum <= origSize) && (srcIndex < origSize) && (dstIndex < newSize) && ((dstIndex + srcNum) == newSize));
      
      // Relocate old elements being displaced by insertion.
      if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         if (cClearNewObjects)
            dynamicArrayMemSet(mpAligned + origSize, 0, num * sizeof(ValueType));
            
         for (int i = srcNum - 1; i >= 0; i--)
         {
            Utils::ConstructInPlace(mpAligned + dstIndex + i, mpAligned[srcIndex + i]);
            Utils::DestructInPlace(mpAligned + srcIndex + i);
         }
      }
      else
      {
         memmove(mpAligned + dstIndex, mpAligned + srcIndex, srcNum * sizeof(ValueType));
      }
                     
      // Construct new elements.
      if (pObjects)
      {
         copyElements(num, mpAligned + pos, pObjects);
      }
      else 
      {
         constructEntries(pos, num);
      }
   }
   
   inline void insert(uint pos, const ValueType& object)
   {
      insert(pos, 1, &object);
   }
   
   // Quickly swaps container's contents with another.
   inline BDynamicArray& swap(BDynamicArray& rhs)
   {
      pointerPolicy::swap(rhs);

      std::swap(mSize_,     rhs.mSize_);
      std::swap(mCapacity_, rhs.mCapacity_);
      
      allocator::swap(rhs);
      
      return *this;
   }
   
   friend bool operator== (const BDynamicArray& lhs, const BDynamicArray& rhs) 
   {
      if (lhs.size() != rhs.size())
         return false;
      for (uint objIter = 0; objIter < lhs.size(); objIter++)
         if (!(lhs[objIter] == rhs[objIter]))
            return false;
      return true;
   }
   
   friend bool operator< (const BDynamicArray& lhs, const BDynamicArray& rhs) 
   {
      if (lhs.size() < rhs.size())
         return true;
      else if (lhs.size() == rhs.size())
      {
         for (uint objIter = 0; objIter < lhs.size(); objIter++)
         {
            if (lhs[objIter] < rhs[objIter])
               return true;
            else if (!(lhs[objIter] == rhs[objIter]))
               return false;
         }
      }
                     
      return false;
   }
   
   friend bool operator >  (const BDynamicArray& a, const BDynamicArray& b) { return b < a; }
   friend bool operator <= (const BDynamicArray& a, const BDynamicArray& b) { return !(b < a); }
   friend bool operator >= (const BDynamicArray& a, const BDynamicArray& b) { return !(a < b); }
   friend bool operator != (const BDynamicArray& a, const BDynamicArray& b) { return !(a == b); }
   
   inline void sort(void)
   {
      if (size())
         std::sort(begin(), end());
   }
   
   inline void shellSort(void)
   {
      Utils::ShellSort(begin(), end());
   }
   
   // Removes duplicate elements that are adjacent to each other using std::unique(). 
   // Container must be sorted!
   inline void removeDuplicates(void)
   {
      if (size())
      {
         ValueType* pNewEnd = std::unique(begin(), end());
         resize(pNewEnd - begin());
      }
   }
  
     // Binary search - returns cInvalidIndex if not found.
   inline int binarySearch(const ValueType& searchObj) const
   {
      int l = 0;
      int r = size() - 1;

      while (r >= l)
      {
         const int m = debugRangeCheck((l + r) >> 1, size());
         const ValueType& obj = (*this)[m];

         if (searchObj == obj)
            return m;
         else if (searchObj < obj)
            r = m - 1;
         else
            l = m + 1;
      }

      return cInvalidIndex;
   }
   
   inline bool binarySearch(const ValueType& searchObj, uint& index) const
   {
      int l = 0;
      int r = size() - 1;

      while (r >= l)
      {
         const int m = debugRangeCheck((l + r) >> 1, size());
         const ValueType& obj = (*this)[m];

         if (searchObj == obj)
         {
            index = m;
            return true;
         }
         else if (searchObj < obj)
            r = m - 1;
         else
            l = m + 1;
      }

      index = 0;
      return false;
   }
   
   // Sequential search, starting at beginning. Returns cInvalidIndex if not found.
   inline int find(const ValueType& searchObj) const
   {
      for (uint i = 0; i < mSize; i++)
         if (mpAligned[i] == searchObj)
            return i;
      return cInvalidIndex;
   }
   
   // Sequential search, starting at beginning. Returns false if not found.
   inline bool find(const ValueType& searchObj, uint& index) const
   {
      for (uint i = 0; i < mSize; i++)
      {
         if (mpAligned[i] == searchObj)
         {
            index = i;
            return true;
         }
      }
      index = 0;
      return false;
   }
   
   // Sequential search, starting at end. Returns cInvalidIndex if not found.
   inline int findLast(const ValueType& searchObj) const
   {
      for (int i = mSize - 1; i >= 0; i--)
         if (mpAligned[i] == searchObj)
            return i;
      return cInvalidIndex;
   }
   
   // Sequential search, starting at end. Returns false if not found.
   inline bool findLast(const ValueType& searchObj, uint& index) const
   {
      for (int i = mSize - 1; i >= 0; i--)
      {
         if (mpAligned[i] == searchObj)
         {
            index = i;
            return true;
         }
      }
      index = 0;
      return false;
   }
     
   inline void reverse(void)
   {
      const uint n = mSize >> 1;
      for (uint i = 0; i < n; i++)
         std::swap(mpAligned[i], mpAligned[mSize - 1 - i]);
   }
   
   inline void setAll(const ValueType& t)
   {
      if ((BIsBuiltInType<ValueType>::Flag) && (sizeof(ValueType) == 1))
      {
         dynamicArrayMemSet(mpAligned, *reinterpret_cast<const uchar*>(&t), mSize);
      }
      else
      {
         for (uint i = 0; i < mSize; i++)
            mpAligned[i] = t;
      }            
   }
   
   // Be forewarned that if you resize, insert new elements, etc. the container the returned pointers may change!
   // Don't persist the returned pointer unless you are sure you're not going to modify the container's contents.
   inline BDataBuffer getDataBuffer(void) { return BDataBuffer(reinterpret_cast<BYTE*>(getPtr()), getSizeInBytes()); }
   inline BConstDataBuffer getConstDataBuffer(void) const { return BConstDataBuffer(reinterpret_cast<const BYTE*>(getPtr()), getSizeInBytes()); }

   inline BTypedConstDataBuffer<ValueType> getTypedConstDataBuffer(void) const { return BTypedConstDataBuffer<ValueType>(getPtr(), getSize()); }
   inline operator BTypedConstDataBuffer<ValueType> () const { return getTypedConstDataBuffer(); }
   
   inline BDynamicArray& operator+= (const BTypedConstDataBuffer<ValueType>& buf) { if (buf.getLen()) pushBack(buf.getPtr(), buf.getLen()); return *this; }
   inline BDynamicArray& assign(const BTypedConstDataBuffer<ValueType>& buf) { BDEBUG_ASSERT(buf.getPtr() != getPtr()); clear(); if (buf.getLen()) pushBack(buf.getPtr(), buf.getLen()); return *this; }
   
   // The following methods are present for more compatibility with utsimplearray.
            
   // Do not change getNumber() back to returning a uint! That would break code that assumes a signed return value.
   // Call size() or getSize() if you want an unsigned return value.
   inline long getNumber(void) const { return static_cast<long>(size()); }

   // jce [10/7/2008] -- removed the "force" parameter here because it was being ignored by the new underlying implementation
   // (this function was from age3's BSimpleArray).  Any old code expecting this force parameter to work isn't going to, so I'm taking
   // it out so that will be more obvious to the user.
   inline bool setNumber(uint newSize) 
   { 
      resize(newSize); 
      return true; 
   }
   
   inline bool validIndex(uint val) const { return val < mSize; }
         
   // add() can never fail. If it does, the app has ran out of memory, and add will not return.
   // So the returned index will always be valid.
   inline uint add(const ValueType& t) { pushBack(t); return getSize() - 1; }
   inline bool add(const ValueType* pSrc, uint num) { pushBack(pSrc, num); return true; }
   inline const ValueType& get(uint i) const { return (*this)[static_cast<uint>(i)]; }
   inline ValueType& get(uint i) { return (*this)[static_cast<uint>(i)]; }
   
   inline ValueType& grow(void) { return *enlarge(1); }
   
   inline void swapIndices(uint index1, uint index2)
   {
      if (validIndex(index1) && validIndex(index2) && (index1 != index2))
         std::swap(at(index1), at(index2));
   }
   
   inline uint insertAtIndex(const ValueType& t, uint index)
   {
      insert(index, t);
      return index;
   }
   
   inline uint uniqueAdd(const ValueType& t)
   {
      int pos = find(t);
      if (cInvalidIndex == pos)
         pos = add(t);
      return pos;
   }
      
   inline bool removeIndex(uint index, bool preserveOrder = true)
   {
      if (preserveOrder)
         erase(index);
      else
         eraseUnordered(index);
      return true;
   }
   
   inline bool removeValueAllInstances(const ValueType& t, bool preserveOrder = true)
   {
      uint index = 0;
      bool valueRemoved = false;

      while (index < getSize())
      {
         if (mpAligned[index] == t)
         {
            removeIndex(index, preserveOrder);
            valueRemoved = true;
         }
         else
            index++;
      }
      
      return valueRemoved;
   }
            
   inline bool remove(const ValueType& t, bool preserveOrder = true) { return removeValue(t, preserveOrder); }
   
   inline bool removeValue(const ValueType& t, bool preserveOrder = true)
   {
      int index = find(t);
      if (index == cInvalidIndex)
         return false;
      removeIndex(index, preserveOrder);
      return true;
   }
   
   inline bool removeValueReverse(const ValueType& t, bool preserveOrder = true)
   {
      int index = findLast(t);
      if (index == cInvalidIndex)
         return false;
      removeIndex(index, preserveOrder);
      return true;
   }
   
   inline void setAt(uint pos, const ValueType& t)
   {
      resize(Math::Max(pos + 1, size()));
      at(pos) = t;
   }
   
   inline void sort(int (__cdecl *pCompareFunc)(const void *, const void *))
   {
      if (mSize)
         qsort(mpAligned, mSize, sizeof(ValueType), pCompareFunc);
   }
   
   inline bool contains(const ValueType& t) const { return find(t) != cInvalidIndex; }
   
   inline void copyPointerInto(const ValueType* pSrc, uint pos, uint num)
   {
      resize(Math::Max(size(), pos + num));
      
      BDEBUG_ASSERT((pos + num) <= mSize);
      
      copyElements(num, &mpAligned[pos], pSrc);
   }
   
   inline void copyPointerInto(const ValueType* pSrc, uint num) { copyPointerInfo(pSrc, 0, num); }
   
   inline void copyPointerFrom(ValueType* pDst, uint pos, uint num)
   {
      resize(Math::Max(size(), pos + num));
      
      BDEBUG_ASSERT((num + pos) <= mSize);
      
      copyElements(num, pDst, &mpAligned[pos]);
   }
   
   inline void copyPointerFrom(ValueType* pDst, uint num) { copyPointerFrom(pDst, 0, num); }
                           
protected:

   typename OptionsPolicy<ValueType, Alignment>::SizeType mSize_;
   typename OptionsPolicy<ValueType, Alignment>::SizeType mCapacity_;

   inline uint getSizeMaxValue(void) const 
   { 
      return (((uint64)1U) << (sizeof(typename OptionsPolicy<ValueType, Alignment>::SizeType) * 8U)) - 1U; 
   }  
   
   __declspec(property(get = getSizeProp,     put = putSizeProp))      uint mSize;
   __declspec(property(get = getCapacityProp, put = putCapacityProp))  uint mCapacity;
   
   inline uint getSizeProp(void) const 
   { 
      return mSize_; 
   }
         
   inline void putSizeProp(uint s) 
   { 
      BDEBUG_ASSERT(s <= getSizeMaxValue());
      mSize_ = static_cast<typename OptionsPolicy<ValueType, Alignment>::SizeType>(s);
   }
   
   inline uint getCapacityProp(void) const 
   { 
      return mCapacity_; 
   }
   
   inline void putCapacityProp(uint s) 
   { 
      BDEBUG_ASSERT(s <= getSizeMaxValue());
      mCapacity_ = static_cast<typename OptionsPolicy<ValueType, Alignment>::SizeType>(s);
   }
                                   
   inline void destructEntries(uint first, uint num)
   {
      if (!num)
         return;
         
      BDEBUG_ASSERT(mSize <= mCapacity);
      debugRangeCheck(first, mSize);
      BDEBUG_ASSERT(first + num <= mSize);
      
      if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         for (uint i = 0; i < num; i++)
            Utils::DestructInPlace(&mpAligned[first + i]);
      }
      
#ifdef BUILD_DEBUG
      memset(mpAligned + first, 0xFD, sizeof(ValueType) * num);
#endif
   }
   
   inline void constructEntries(uint first, uint num)
   {
      if (!num)
         return;
      
      BDEBUG_ASSERT(mSize <= mCapacity);            
      debugRangeCheck(first, mCapacity);
      BDEBUG_ASSERT(first + num <= mCapacity);
      
      constructElements(num, &mpAligned[first]);
   }
   
   inline uint roomLeft(void) const
   {
      BDEBUG_ASSERT( ((mpRaw) || ((NULL == mpAligned) && (0 == mCapacity) && (0 == mSize))) && (mSize <= mCapacity) );
                  
      const uint roomLeft = mCapacity - mSize;
      
      return roomLeft;
   }
   
   inline void constructElements(uint num, ValueType* pDst)
   {
      if (BIsBuiltInType<ValueType>::Flag)
      {
         // Always memset for built-in types, instead of messing around with copy constructing using 
         // the value defined by the type's default constructor. This should almost always be faster, 
         // particularly in debug.
         if ((cClearNewObjects) || (cUseConstructorDestructor))
         {
            dynamicArrayMemSet(pDst, 0, num * sizeof(ValueType));
         }
      }
      else
      {
         if (cClearNewObjects)
            dynamicArrayMemSet(pDst, 0, num * sizeof(ValueType));
         
         if (cUseConstructorDestructor)
         {
            if (cUseCopyConstructor)
               constructInPlaceCopy(num, pDst, ValueType());
            else
            {
               const ValueType* pEndDst = pDst + num;
               while (pDst != pEndDst)
               {
                  Utils::ConstructInPlace(pDst);
                  pDst++;
               }
            }
         }
      }         
   }
         
   inline void constructInPlaceCopy(uint num, ValueType* pDst, const ValueType& v)
   {
      const ValueType* pEndDst = pDst + num;
      while (pDst != pEndDst)
      {
         Utils::ConstructInPlace(pDst, v);
         pDst++;
      }
   }
               
   // src/dst should not overlap!
   inline void copyElements(uint num, ValueType* pDst, const ValueType* pSrc, bool destructOldElements = false)
   {
      BDEBUG_ASSERT( ((pSrc + num) <= pDst) || (pSrc >= (pDst + num)) );
      
      if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         if (cClearNewObjects)
            dynamicArrayMemSet(pDst, 0, num * sizeof(ValueType));
                  
         const ValueType* pEndDst = pDst + num;
         while (pDst != pEndDst)
         {
            Utils::ConstructInPlace(pDst, *pSrc);
            if (destructOldElements)
               Utils::DestructInPlace(pSrc);
            pSrc++;
            pDst++;
         }
      }
      else
      {
         dynamicArrayMemCpy(pDst, pSrc, num * sizeof(ValueType));
      }
   }         
      
#if DYNAMIC_ARRAY_TRACKING
   BDynamicArrayTracker mTracker;
   
   static void getStateCallBack(void* pArray, uint& typeSize, uint& size, uint& capacity, void*& pAllocation)
   {
      const BDynamicArray* p = static_cast<const BDynamicArray*>(pArray);
      
      typeSize    = sizeof(ValueType);
      size        = p->mSize;
      capacity    = p->mCapacity;
      pAllocation = p->mpRaw;
   }
#endif 
   
   static void* allocCallback(void* pArray, BDynamicArrayAllocOp op, void* p, uint prevNumObjects, uint newNumObjects, int minNumObjects, uint* pNewBufSizeInBytes)
   {
      BDynamicArray* pDynArray = (BDynamicArray*)pArray;
      switch (op)
      {
         case cDAAlloc:   
            return pDynArray->allocBuf(newNumObjects);
         case cDAFree:    
            pDynArray->freeBuf(p, prevNumObjects); 
            break;
         case cDAResize:  
            return pDynArray->resizeBuf(p, prevNumObjects, newNumObjects, minNumObjects, *pNewBufSizeInBytes);
      }
      return NULL;
   }
   
   static void entryCallback(void* pArray, BDynamicArrayEntryOp op, uint first, uint num, void* pDst, void* pSrc)
   {
      BDynamicArray* pDynArray = (BDynamicArray*)pArray;
      switch (op)
      {
         case cDAConstruct:
            pDynArray->constructElements(num, (ValueType*)pDst);
            break;
         case cDADestruct:
            pDynArray->destructEntries(first, num);
            break;
         case cDAMove:
            pDynArray->copyElements(num, (ValueType*)pDst, (ValueType*)pSrc, true);
            break;
      }
   }
};

//------------------------------------------------------------------------------------------------------------------------

#include "dynamicArrayAllocators.inl"

//------------------------------------------------------------------------------------------------------------------------

// These are suffixed with "Vec" for compatibility with code ported from Wrench.
typedef BDynamicArray<uchar>  UCharVec;
typedef BDynamicArray<ushort> UShortVec;
typedef BDynamicArray<short>  ShortVec;
typedef BDynamicArray<int>    IntVec;
typedef BDynamicArray<uint>   UIntVec;

typedef BDynamicArray<uchar>  UCharArray;
typedef BDynamicArray<ushort> UShortArray;
typedef BDynamicArray<short>  ShortArray;
typedef BDynamicArray<int>    IntArray;
typedef BDynamicArray<uint>   UIntArray;

typedef BDynamicArray<int64>  Int64Array;
typedef BDynamicArray<uint64> UInt64Array;

typedef UCharArray            BByteArray;

typedef BDynamicArray<BYTE>   BDynamicBYTEArray;
typedef BDynamicArray<BYTE,   4, BDynamicArraySimHeapAllocator> BDynamicSimBYTEArray;

typedef BDynamicArray<long>   BDynamicLongArray;
typedef BDynamicArray<long,   4, BDynamicArraySimHeapAllocator> BDynamicSimLongArray;

typedef BDynamicArray<float>   BDynamicFloatArray;
typedef BDynamicArray<float,   4, BDynamicArraySimHeapAllocator> BDynamicSimFloatArray;

typedef BDynamicArray<uint,   4, BDynamicArraySimHeapAllocator> BDynamicSimUIntArray;

typedef BDynamicArray<char,   4, BDynamicArraySimHeapAllocator> BSimCharArray;  
typedef BDynamicArray<uchar,  4, BDynamicArraySimHeapAllocator> BSimUCharArray;  
typedef BDynamicArray<WORD,   4, BDynamicArraySimHeapAllocator> BSimWORDArray;  
typedef BDynamicArray<int,    4, BDynamicArraySimHeapAllocator> BSimIntArray;
typedef BDynamicArray<DWORD,  4, BDynamicArraySimHeapAllocator> BSimDWORDArray;

// This dynamic array type uses the C run time library allocator (malloc). 
// It's only useful when you absolutely want to avoid anything to do with Rockall.
template<
   class ValueType, 
      uint Alignment                               = ALIGN_OF(ValueType), 
      template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
      template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
>
class BDynamicCHeapArray : public BDynamicArray<ValueType, Alignment, BDynamicArrayCAllocator, OptionsPolicy, PointerPolicy>
{
public:
   BDynamicCHeapArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};


// BSmallDynamicSimArray's max size/capacity is 65535 objects, but it's 4 bytes smaller than a regular BDynamicArray. (8 for alignment <= 4, 12 otherwise).
template<class ValueType, uint Alignment = ALIGN_OF(ValueType)>
class BSmallDynamicArray : public BDynamicArray<ValueType, Alignment, BDynamicArrayDefaultAllocator, BDynamicArraySmallOptions, BDynamicArraySeparatePointers>
{
public:
   BSmallDynamicArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};

template<
   class ValueType, 
   uint Alignment                               = ALIGN_OF(ValueType), 
   template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
   template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
>
class BDynamicSimArray : public BDynamicArray<ValueType, Alignment, BDynamicArraySimHeapAllocator, OptionsPolicy, PointerPolicy>
{
public:
   explicit BDynamicSimArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};

// BSmallDynamicSimArray's max size/capacity is 65535 objects, but it's 4 bytes smaller than a regular BDynamicArray. (8 for alignment <= 4, 12 otherwise).
template<class ValueType, uint Alignment = ALIGN_OF(ValueType)>
class BSmallDynamicSimArray : public BDynamicArray<ValueType, Alignment, BDynamicArraySimHeapAllocator, BDynamicArraySmallOptions, BDynamicArraySeparatePointers>
{
public:
   BSmallDynamicSimArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};

// This class uses bitwise copying.
class BDynamicArraySerializer
{
public:
   template<class T> static int getSize(const T& a)
   {
      return sizeof(uint) + (a.getSize() * sizeof(T::valueType));
   }
   
   template<class T>
   static void* serialize(const T& a, void* pDst)
   {
      pDst = Utils::writeObj(pDst, a.getSize());
      
      if (a.getSize())
      {
         const uint sizeInBytes = a.getSizeInBytes();
         Utils::FastMemCpy(pDst, a.getPtr(), sizeInBytes);
         pDst = static_cast<uchar*>(pDst) + sizeInBytes;
      }
      
      return pDst;
   }
   
   template<class T>
   static const void* deserialize(T& a, const void* pSrc)
   {
      uint size;
      pSrc = Utils::readObj(pSrc, size);
      
      a.resize(size);
      if (size)
      {
         const uint sizeInBytes = size * sizeof(T::valueType);
         Utils::FastMemCpy(a.getPtr(), pSrc, sizeInBytes);
         pSrc = static_cast<const uchar*>(pSrc) + sizeInBytes;
      }      
      
      return pSrc;
   }
};

#undef COMPUTE_ATTRIBS

#pragma warning( pop )
#if DYNAMIC_ARRAY_TRACKING
#pragma warning(default:4355) // this used in base memory initialize list
#endif

