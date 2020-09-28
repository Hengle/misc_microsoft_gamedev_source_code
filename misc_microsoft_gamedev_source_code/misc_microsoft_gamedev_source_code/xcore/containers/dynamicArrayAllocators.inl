// File: dynamicArrayAllocators.h
// Default C RTL, Win-32 Virtual memory API, and Win-32 heap API allocators.

//------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
class BDynamicArrayCAllocator
{
public:
   enum 
   { 
      cCanResizeBuf = true
   };

   void* allocBuf(uint numObjects)
   {
      uint size = numObjects * sizeof(ValueType);
      
      if (!cMemoryAllocatorsSupportAlignment)
         size += (Alignment - 1);
      else if (Alignment > sizeof(DWORD))
      {  
         BCOMPILETIMEASSERT(Alignment <= 16);
         size = Utils::AlignUpValue(size, 16);
      }

      void* p = malloc(size);

      if (cMemoryAllocatorsSupportAlignment)
      {
         BASSERT(Utils::IsAligned(p, Alignment));
      }

      return p;
   }

   void freeBuf(void* p, uint numObjects)
   {
      numObjects;
      free(p);
   }

   void* resizeBuf(void* p, size_t prevNumObjects, uint newNumObjects, int minNumObjects, uint& newBufSizeInBytes)
   {
      prevNumObjects;
      minNumObjects;

      newBufSizeInBytes = 0;

      uint size = newNumObjects * sizeof(ValueType);
      
      if (!cMemoryAllocatorsSupportAlignment)
         size += (Alignment - 1);
      else if (Alignment > sizeof(DWORD))
      {  
         BCOMPILETIMEASSERT(Alignment <= 16);
         size = Utils::AlignUpValue(size, 16);
      }

      void* q = _expand(p, size);
      if (!q)
         return NULL;

      if (cMemoryAllocatorsSupportAlignment)
      {
         BASSERT(Utils::IsAligned(q, Alignment));
      }

      newBufSizeInBytes = static_cast<uint>(_msize(q));

      return q;   
   }

   void swap(BDynamicArrayCAllocator& other)
   {
      other;
   }
};

//------------------------------------------------------------------------------------------------------------------------

// rg [5/15/06] - BDynamicArrayVirtualAllocator is too damn complex.

template<class ValueType, uint Alignment>
class BDynamicArrayVirtualAllocator 
{
   void* mpPages;
   WORD mNumPagesReserved;
   WORD mNumPagesCommited;

public:
   typedef BDynamicArrayVirtualAllocator<ValueType, Alignment> allocator;

   enum 
   { 
      cCanResizeBuf = true
   };

   BDynamicArrayVirtualAllocator() : 
      mpPages(NULL),
      mNumPagesReserved(0),
      mNumPagesCommited(0)
   {
      BCOMPILETIMEASSERT(Alignment <= 4096);
   }

   BDynamicArrayVirtualAllocator(const allocator& other) : 
      mpPages(NULL),
      mNumPagesReserved(0),
      mNumPagesCommited(0)
   {
      BCOMPILETIMEASSERT(Alignment <= 4096);

      const uint numPages = other.getNumPagesReserved();
      if (numPages)
         virtualReserve((numPages << 12) / sizeof(ValueType));
   }

   template<class T, uint A>
   BDynamicArrayVirtualAllocator(const BDynamicArrayVirtualAllocator<T, A>& other) :
      mpPages(NULL),
      mNumPagesReserved(0),
      mNumPagesCommited(0)
   {
      BCOMPILETIMEASSERT(Alignment <= 4096);

      const uint numPages = other.getNumPagesReserved();
      if (numPages)
         virtualReserve((numPages << 12) / sizeof(ValueType));
   }

   allocator& operator= (const allocator& other)
   {
      const uint numPages = other.getNumPagesReserved();
      virtualReserve((numPages << 12) / sizeof(ValueType));

      return *this;
   }

   ~BDynamicArrayVirtualAllocator()
   {
      if (mpPages)
      {
         const BOOL status = VirtualFree(mpPages, 0, MEM_RELEASE);
         status;
         BDEBUG_ASSERT(status);
      }
   }

   uint getNumPagesReserved(void) const { return mNumPagesReserved; }
   uint getNumPagesCommited(void) const { return mNumPagesCommited; }
   void* getPages(void) const { return mpPages; }

   void virtualFree(void)
   {
      if (mpPages)
      {
         // The container will free the buffer later.
         if (!mNumPagesCommited)
         {
            const BOOL status = VirtualFree(mpPages, 0, MEM_RELEASE);
            status;
            BDEBUG_ASSERT(status);
         }

         mpPages = NULL;
         mNumPagesCommited = 0;
         mNumPagesReserved = 0;
      }
   }

   bool virtualReserve(uint numObjects)
   {
      if (!numObjects)
      {
         virtualFree();
         return true;
      }

      const uint numPages = Utils::AlignUpValue(numObjects * sizeof(ValueType), 4096) >> 12;
      if (numPages == mNumPagesReserved)
         return true;

      virtualFree();

      mpPages = VirtualAlloc(NULL, numPages << 12, 
#ifdef XBOX      
         MEM_RESERVE | MEM_NOZERO, 
#else
         MEM_RESERVE, 
#endif      
         PAGE_READWRITE);

      if (!mpPages)
         return false;

      mNumPagesReserved = static_cast<WORD>(debugRangeCheckIncl(numPages, USHRT_MAX));         

      return true;
   }

   void* allocBuf(uint numObjects)
   {
      BDEBUG_ASSERT(numObjects);

      const uint numPages = Utils::AlignUpValue(numObjects * sizeof(ValueType), 4096) >> 12;

      if (mNumPagesCommited)
      {
         // Must be growing but in-place resize failed.
         // The container will free the current buffer.

         mpPages = VirtualAlloc(NULL, numPages << 12,  
#ifdef XBOX      
            MEM_RESERVE | MEM_COMMIT | MEM_NOZERO, 
#else
            MEM_RESERVE | MEM_COMMIT, 
#endif      
            PAGE_READWRITE);

         mNumPagesCommited = mNumPagesReserved = 0;

         if (!mpPages)
            return NULL;

         mNumPagesCommited = mNumPagesReserved = static_cast<WORD>(debugRangeCheckIncl(numPages, USHRT_MAX));

         return mpPages;
      }
      else if (numPages > mNumPagesReserved)
      {
         // Current allocation is too small, but no pages have been committed yet from the current allocation.
         // Dump it for a bigger block.

         if (!virtualReserve(numObjects))
            return NULL;
      }

      void* p = VirtualAlloc(mpPages, numPages << 12, 
#ifdef XBOX      
         MEM_COMMIT | MEM_NOZERO, 
#else
         MEM_COMMIT, 
#endif      
         PAGE_READWRITE);

      if (!p)
         return NULL;

      mNumPagesCommited = static_cast<WORD>(debugRangeCheckIncl(Math::Max<uint>(mNumPagesCommited, numPages), USHRT_MAX));

      return p;
   }

   void freeBuf(void* p, uint numObjects)
   {
      numObjects;

      if (!p)
         return;

      if (p == mpPages)
      {
         const BOOL status = VirtualFree(mpPages, mNumPagesCommited << 12, MEM_DECOMMIT);
         status;
         BDEBUG_ASSERT(status);

         mNumPagesCommited = 0;
      }
      else
      {
         const BOOL status = VirtualFree(p, 0, MEM_RELEASE);
         status;
         BDEBUG_ASSERT(status);
      }
   }

   void* resizeBuf(void* p, size_t prevNumObjects, uint newNumObjects, int minNumObjects, uint& newBufSizeInBytes)
   {
      prevNumObjects;

      if (p != mpPages)
         return NULL;

      //if (!minNumObjects)
      //   minNumObjects = newNumObjects;

      uint numPages = Math::Max<uint>(1, Utils::AlignUpValue(newNumObjects * sizeof(ValueType), 4096) >> 12);   
      uint numPagesMin = Math::Max<uint>(1, Utils::AlignUpValue(minNumObjects * sizeof(ValueType), 4096) >> 12);   

      if (numPagesMin > mNumPagesReserved)
      {
         // Container is trying to grow.
         // Reserve a new buffer, but only commit a few more pages.

         mpPages = VirtualAlloc(NULL, numPages << 12, 
#ifdef XBOX      
            MEM_RESERVE | MEM_NOZERO, 
#else
            MEM_RESERVE, 
#endif      
            PAGE_READWRITE);

         mNumPagesCommited = mNumPagesReserved = 0;

         if (!mpPages)
            return NULL;

         mNumPagesReserved = static_cast<WORD>(numPages);

         uint numPagesToCommit = numPagesMin;
         numPagesToCommit = Math::Min<uint>(numPagesToCommit, numPages);

         void* q = VirtualAlloc(mpPages, numPagesToCommit << 12, 
#ifdef XBOX      
            MEM_COMMIT | MEM_NOZERO, 
#else
            MEM_COMMIT, 
#endif      
            PAGE_READWRITE);         

         if (!q)  
         {
            VirtualFree(mpPages, 0, MEM_RELEASE);

            mpPages = NULL;
            mNumPagesCommited = mNumPagesReserved = 0;

            return NULL;
         }

         mNumPagesCommited = static_cast<WORD>(numPagesToCommit);            

         newBufSizeInBytes = numPagesToCommit << 12;

         return mpPages;
      }

      // Change the current allocation's commit size.

      if (numPages < mNumPagesCommited)
      {
         uint numPagesToFree = mNumPagesCommited - numPages;

         const BOOL status = VirtualFree(reinterpret_cast<uchar*>(p) + (numPages << 12), numPagesToFree << 12, MEM_DECOMMIT);
         status;
         BDEBUG_ASSERT(status);

         mNumPagesCommited = static_cast<WORD>(debugRangeCheckIncl(numPages, USHRT_MAX));               
      }
      else if (numPagesMin > mNumPagesCommited)
      {
         void* q = VirtualAlloc(mpPages, numPagesMin << 12, 
#ifdef XBOX      
            MEM_COMMIT | MEM_NOZERO, 
#else
            MEM_COMMIT, 
#endif      
            PAGE_READWRITE);

         if (!q)
            return NULL;

         mNumPagesCommited = static_cast<WORD>(debugRangeCheckIncl(numPagesMin, USHRT_MAX));               
      }            

      newBufSizeInBytes = mNumPagesCommited << 12;

      return p;   
   }

   void swap(BDynamicArrayVirtualAllocator& other)
   {
      std::swap(mpPages, other.mpPages);
      std::swap(mNumPagesReserved, other.mNumPagesReserved);
      std::swap(mNumPagesCommited, other.mNumPagesCommited);
   }
};

//------------------------------------------------------------------------------------------------------------------------

class BDynamicArrayWin32Heap
{
public:
   BDynamicArrayWin32Heap(HANDLE heap) :
      mHeap(heap),
      mOwnsHeap(false),
      mRefCount(1)
   {
   }
   
   BDynamicArrayWin32Heap(DWORD flOptions = 0, SIZE_T dwInitialSize = 4096, SIZE_T dwMaximumSize = 0) :
      mOwnsHeap(true),
      mRefCount(1)
   {
      mHeap = HeapCreate(flOptions, dwInitialSize, dwMaximumSize);
   }
   
   ~BDynamicArrayWin32Heap()
   {
      if (mOwnsHeap)
         HeapDestroy(mHeap);
   }

   // Not thread safe!
   void addRef(void) { mRefCount++; }

   uint getRefCount(void) { return mRefCount; }

   // Assumes BDynamicArrayWin32Heap was created with operator new.
   void release(void)
   {
      BDEBUG_ASSERT(mRefCount > 0);
      mRefCount--;
      if (mRefCount == 0) 
         delete this;
   }

   HANDLE getHandle(void) const { return mHeap; }

private:
   HANDLE mHeap;
   short mRefCount;
   bool mOwnsHeap;
   
   BDynamicArrayWin32Heap(const BDynamicArrayWin32Heap&);
   BDynamicArrayWin32Heap& operator= (const BDynamicArrayWin32Heap&);
};

extern __declspec(selectany) BDynamicArrayWin32Heap gDefaultDynamicArrayWin32Heap(GetProcessHeap());

//------------------------------------------------------------------------------------------------------------------------

template<class ValueType=uchar, uint Alignment = ALIGN_OF(ValueType) >
class BDynamicArrayWin32HeapAllocator
{
public:
   typedef BDynamicArrayWin32HeapAllocator<ValueType, Alignment> allocator;

   enum 
   { 
      cCanResizeBuf = true
   };

   BDynamicArrayWin32HeapAllocator(BDynamicArrayWin32Heap* pHeap = &gDefaultDynamicArrayWin32Heap) :
      mpHeap(pHeap)
   {
      mpHeap->addRef();
   }

   BDynamicArrayWin32HeapAllocator(const BDynamicArrayWin32HeapAllocator& other) :
      mpHeap(other.getHeap())
   {  
      mpHeap->addRef();
   }

   template<class T, uint A>
   BDynamicArrayWin32HeapAllocator(const BDynamicArrayWin32HeapAllocator<T, A>& other) :
      mpHeap(other.getHeap())
   {  
      mpHeap->addRef();
   }

   BDynamicArrayWin32HeapAllocator& operator= (const BDynamicArrayWin32HeapAllocator& other)
   {
      mpHeap->release();

      mpHeap = other.getHeap();

      mpHeap->addRef();
   }

   ~BDynamicArrayWin32HeapAllocator()
   {
      mpHeap->release();
   }

   BDynamicArrayWin32Heap* getHeap(void) const { return mpHeap; }

   void setHeap(BDynamicArrayWin32Heap* pHeap)
   {
      mpHeap->release();

      mpHeap = pHeap;  

      mpHeap->addRef();
   }      

   void* allocBuf(uint numObjects)
   {
      const uint size = numObjects * sizeof(ValueType) + (Alignment - 1);

      return HeapAlloc(mpHeap->getHandle(), 0, size);
   }

   void freeBuf(void* p, uint numObjects)
   {
      numObjects;

      if (!p)
         return;

      BDEBUG_ASSERT(mpHeap);

      HeapFree(mpHeap->getHandle(), 0, p);
   }

   void* resizeBuf(void* p, size_t prevNumObjects, uint newNumObjects, int minNumObjects, uint& newBufSizeInBytes)
   {
      prevNumObjects;
      minNumObjects;

      BDEBUG_ASSERT(p && mpHeap);

      newBufSizeInBytes = 0;

      const uint size = newNumObjects * sizeof(ValueType) + (Alignment - 1);

      void* q = HeapReAlloc(mpHeap->getHandle(), HEAP_REALLOC_IN_PLACE_ONLY, p, size);

      if (!q)
         return NULL;

      newBufSizeInBytes = size;

      return q;   
   }

   void swap(allocator& other)
   {  
      std::swap(mpHeap, other.mpHeap);
   }

private:
   BDynamicArrayWin32Heap* mpHeap;      
};

//------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment>
class BDynamicArrayHeapAllocator
{
   BMemoryHeap* mpHeap;
   
public:
   enum 
   { 
      cCanResizeBuf = true
   };
   
   typedef BDynamicArrayHeapAllocator<ValueType, Alignment> allocator;

   inline BDynamicArrayHeapAllocator(BMemoryHeap* pHeap = &gPrimaryHeap) :
      mpHeap(pHeap)
   {
   }

   inline BDynamicArrayHeapAllocator(const BDynamicArrayHeapAllocator& other) :
      mpHeap(other.getHeap())
   {  
   }

   template<class T, uint A>
   inline BDynamicArrayHeapAllocator(const BDynamicArrayHeapAllocator<T, A>& other) :
      mpHeap(other.getHeap())
   {  
   }

   inline BDynamicArrayHeapAllocator& operator= (const BDynamicArrayHeapAllocator& other)
   {
      mpHeap = other.getHeap();
      return *this;
   }

   inline BMemoryHeap* getHeap(void) const { return mpHeap; }

   inline void setHeap(BMemoryHeap* pHeap) { mpHeap = pHeap;  }      

   inline void* allocBuf(uint numObjects)
   {
      uint size = numObjects * sizeof(ValueType);

      void* p;
      if (Alignment <= mpHeap->getMaxSupportedAlignment())
      {
         size = Utils::AlignUpValue(size, Alignment);
         
         p = mpHeap->New(size);

         // The pointer MUST be aligned here because the array aligns up the pointer itself!
         BASSERT( Utils::IsAligned(p, Alignment) );
      }
      else
      {
         if (Alignment >= sizeof(DWORD))
            size += Alignment - 1;
         
         p = mpHeap->New(size);
      }         

      return p;
   }

   inline void freeBuf(void* p, uint numObjects)
   {
      numObjects;
      mpHeap->Delete(p);
   }

   inline void* resizeBuf(void* p, size_t prevNumObjects, uint newNumObjects, int minNumObjects, uint& newBufSizeInBytes)
   {
      prevNumObjects;
      minNumObjects;

      newBufSizeInBytes = 0;

      uint size = newNumObjects * sizeof(ValueType);
            
      int actualSize = 0;
      void* q;
      
      if (Alignment <= mpHeap->getMaxSupportedAlignment())
      {
         size = Utils::AlignUpValue(size, 16);
         
         q = mpHeap->Resize(p, size, 0, &actualSize);
         
         // The pointer MUST be aligned here because the array aligns up the pointer itself!
         BASSERT( Utils::IsAligned(q, Alignment) );
      }
      else
      {
         if (Alignment > sizeof(DWORD))
            size += Alignment - 1;
            
         q = mpHeap->Resize(p, size, 0, &actualSize);
      }  
      
      if (!q)
         return NULL;       
      
      BDEBUG_ASSERT(q == p);

      newBufSizeInBytes = static_cast<uint>(actualSize);

      return q;   
   }

   inline void swap(BDynamicArrayHeapAllocator& other)
   {
      std::swap(mpHeap, other.mpHeap);
   }
};

//------------------------------------------------------------------------------------------------------------------------

template<class ValueType, uint Alignment, class AllocatorPolicy>
class BDynamicArrayFixedHeapAllocator : public AllocatorPolicy
{
public:
   enum 
   { 
      cCanResizeBuf = true
   };

   inline void* allocBuf(uint numObjects)
   {
      uint size = numObjects * sizeof(ValueType);

      void* p;
      if (Alignment <= getHeap().getMaxSupportedAlignment())
      {
         size = Utils::AlignUpValue(size, 16);
               
         p = getHeap().New(size);

         // The pointer MUST be aligned here because the array aligns up the pointer itself!
         BASSERT( Utils::IsAligned(p, Alignment) );
      }
      else
      {
         if (Alignment > sizeof(DWORD))
            size += Alignment - 1;
         
         p = getHeap().New(size);
      }
            
      return p;
   }

   inline void freeBuf(void* p, uint numObjects)
   {
      numObjects;
      getHeap().Delete(p);
   }

   inline void* resizeBuf(void* p, size_t prevNumObjects, uint newNumObjects, int minNumObjects, uint& newBufSizeInBytes)
   {
      prevNumObjects;
      minNumObjects;

      newBufSizeInBytes = 0;

      uint size = newNumObjects * sizeof(ValueType);
      
      int actualSize = 0;
      void* q;

      if (Alignment <= getHeap().getMaxSupportedAlignment())
      {
         size = Utils::AlignUpValue(size, 16);
         
         q = getHeap().Resize(p, size, 0, &actualSize);

         // The pointer MUST be aligned here because the array aligns up the pointer itself!            
         BASSERT( Utils::IsAligned(q, Alignment) );
      }
      else
      {
         if (Alignment > sizeof(DWORD))
            size += Alignment - 1;
          
         q = getHeap().Resize(p, size, 0, &actualSize);
      }                  
      
      if (!q)
         return NULL;

      BDEBUG_ASSERT(q == p);
            
      newBufSizeInBytes = static_cast<uint>(actualSize);

      return q;   
   }

   inline void swap(BDynamicArrayFixedHeapAllocator& other)
   {
      other;
   }
};

struct BDynamicArrayPrimaryHeapAllocatorPolicy { BMemoryHeap&     getHeap(void) const { return gPrimaryHeap; } };
struct BDynamicArraySimHeapAllocatorPolicy     { BMemoryHeap&     getHeap(void) const { return gSimHeap; } };
struct BDynamicArrayRenderHeapAllocatorPolicy  { BMemoryHeap&     getHeap(void) const { return gRenderHeap; } };

template<class ValueType, uint Alignment>
struct BDynamicArrayPrimaryHeapAllocator  : BDynamicArrayFixedHeapAllocator<ValueType, Alignment, BDynamicArrayPrimaryHeapAllocatorPolicy> { };

template<class ValueType, uint Alignment>
struct BDynamicArraySimHeapAllocator      : BDynamicArrayFixedHeapAllocator<ValueType, Alignment, BDynamicArraySimHeapAllocatorPolicy> { };

template<class ValueType, uint Alignment>
struct BDynamicArrayRenderHeapAllocator   : BDynamicArrayFixedHeapAllocator<ValueType, Alignment, BDynamicArrayRenderHeapAllocatorPolicy> { };

//------------------------------------------------------------------------------------------------------------------------
