//============================================================================
//
//  linkedList.h
//  
//  Copyright (c) 1999-2007, Ensemble Studios
// 
//  rg [2/17/07] - This is an updated version of Age3's copyList.h/.inl
//
//============================================================================
#pragma once

template <class T, class Allocator> class BLinkedList;
template <class T, class Allocator> class BLinkedListIterator;
template <class T, class Allocator> class BConstLinkedListIterator;

//----------------------------------------------------------------------------
//  Class BLinkedListIterator
//----------------------------------------------------------------------------
template<class T, class Allocator>
class BLinkedListIterator
{
   typedef BLinkedList<T, Allocator> BLinkedList;
   typedef typename BLinkedList::BNode BNode;
   
   friend class BConstLinkedListIterator<T, Allocator>;
   friend class BLinkedList;
   
   T* mpItem;
   BLinkedList* mpLinkedList;
      
   static T* getLLEndItem(void) { return (T*)0xFFFFFFFF; }
   
   typename BLinkedList::BNode* getNode(void) const { return reinterpret_cast<typename BLinkedList::BNode*>(mpItem); }
      
public:
   BLinkedListIterator() :
      mpLinkedList(NULL),
      mpItem(NULL)
   {
   }
   
   BLinkedListIterator(const BLinkedListIterator& other) : 
      mpLinkedList(other.mpLinkedList), 
      mpItem(other.mpItem) 
   { 
   }
               
   BLinkedListIterator(BLinkedList* pLinkedList, T* pItem) : 
      mpLinkedList(pLinkedList), 
      mpItem(pItem) 
   {
   }
   
   BLinkedListIterator& operator= (const BLinkedListIterator& other)
   {
      mpLinkedList = other.mpLinkedList;
      mpItem = other.mpItem;
      return *this;
   }
   
   BLinkedList* getLinkedList(void) const { return mpLinkedList; }
   T* getItem(void) const { return mpItem; }
      
   bool isNull(void) const { return mpItem == NULL; }
   bool isEnd(void) const { return mpItem == getLLEndItem(); }
         
   BLinkedListIterator operator++(int)  //postincrement
   {
      BLinkedListIterator result(*this);
      ++*this;
      return result;
   }
   
   BLinkedListIterator operator--(int)  //postdecrement
   {
      BLinkedListIterator result(*this);
      --*this;
      return result;
   }

   BLinkedListIterator& operator++()  //preincrement
   {
      BDEBUG_ASSERT(mpLinkedList && mpItem && (mpItem != getLLEndItem()));
      BDEBUG_ASSERT(!mpLinkedList->isEmpty());
      
      mpItem = (T*)((BNode*)mpItem)->mpNext;
      if (!mpItem)
         mpItem = getLLEndItem();
         
      return *this;
   }
   
   BLinkedListIterator& operator--()  //predecrement
   {
      BDEBUG_ASSERT(mpLinkedList && mpItem);
      BDEBUG_ASSERT(!mpLinkedList->isEmpty());
      
      if (mpItem == getLLEndItem())
      {
         mpItem = (T*)mpLinkedList->mpUsedTail;
         BDEBUG_ASSERT(mpItem);
      }
      else
      {
         mpItem = (T*)((BNode*)mpItem)->mpPrev;
         BDEBUG_ASSERT(mpItem);
      }
      
      return *this;
   }
         
   T& operator*() const
   {  
      BDEBUG_ASSERT(mpLinkedList && mpItem && (mpItem != getLLEndItem()) && !mpLinkedList->isEmpty());
      return *mpItem;
   }
   
   T* operator->() const
   { 
      BDEBUG_ASSERT(mpLinkedList && mpItem && (mpItem != getLLEndItem()) && !mpLinkedList->isEmpty());
      return mpItem;
   }
   
   operator bool() const { return mpItem != NULL; }
   
   bool operator == (const BLinkedListIterator& b) const { return (mpLinkedList == b.mpLinkedList) && (mpItem == b.mpItem); }
   bool operator != (const BLinkedListIterator& b) const { return !(*this == b); }
   
   bool operator == (const BConstLinkedListIterator<T, Allocator>& b) const { return (mpLinkedList == b.mpLinkedList) && (mpItem == b.mpItem); }
   bool operator != (const BConstLinkedListIterator<T, Allocator>& b) const { return !(*this == b); }
};

//----------------------------------------------------------------------------
//  Class BConstLinkedListIterator
//----------------------------------------------------------------------------
template<class T, class Allocator>
class BConstLinkedListIterator
{
   typedef BLinkedList<T, Allocator> BLinkedList;
   typedef typename BLinkedList::BNode BNode;
   
   friend class BLinkedListIterator<T, Allocator>;
   friend class BLinkedList;
   
   const T* mpItem;
   const BLinkedList* mpLinkedList;
      
   static T* getLLEndItem(void) { return (T*)0xFFFFFFFF; }
   
   const typename BLinkedList::BNode* getNode(void) const { return reinterpret_cast<const typename BLinkedList::BNode*>(mpItem); }
      
public:
   BConstLinkedListIterator() :
      mpLinkedList(NULL),
      mpItem(NULL)
   {
   }
   
   BConstLinkedListIterator(const BConstLinkedListIterator& other) : 
      mpLinkedList(other.mpLinkedList), 
      mpItem(other.mpItem) 
   { 
   }
   
   BConstLinkedListIterator(const BLinkedListIterator<T, Allocator>& other) : 
      mpLinkedList(other.mpLinkedList), 
      mpItem(other.mpItem) 
   { 
   }
               
   BConstLinkedListIterator(const BLinkedList* pLinkedList, const T* pItem) : 
      mpLinkedList(pLinkedList), 
      mpItem(pItem) 
   {
   }
   
   BConstLinkedListIterator& operator= (const BConstLinkedListIterator& other)
   {
      mpLinkedList = other.mpLinkedList;
      mpItem = other.mpItem;
      return *this;
   }
   
   BConstLinkedListIterator& operator= (const BLinkedListIterator<T, Allocator>& other)
   {
      mpLinkedList = other.mpLinkedList;
      mpItem = other.mpItem;
      return *this;
   }
   
   const BLinkedList* getLinkedList(void) const { return mpLinkedList; }
   const T* getItem(void) const { return mpItem; }
   
   bool isNull(void) const { return mpItem == NULL; }
   bool isEnd(void) const { return mpItem == getLLEndItem(); }
         
   BConstLinkedListIterator operator++(int)  //postincrement
   {
      BConstLinkedListIterator result(*this);
      ++*this;
      return result;
   }
   
   BConstLinkedListIterator operator--(int)  //postdecrement
   {
      BConstLinkedListIterator result(*this);
      --*this;
      return result;
   }

   BConstLinkedListIterator& operator++()  //preincrement
   {
      BDEBUG_ASSERT(mpLinkedList && mpItem && (mpItem != getLLEndItem()));
      BDEBUG_ASSERT(!mpLinkedList->isEmpty());
            
      mpItem = (T*)((BNode*)mpItem)->mpNext;
      if (!mpItem)
         mpItem = getLLEndItem();
         
      return *this;
   }
   
   BConstLinkedListIterator& operator--()  //predecrement
   {
      BDEBUG_ASSERT(mpLinkedList && mpItem);
      BDEBUG_ASSERT(!mpLinkedList->isEmpty());

      if (mpItem == getLLEndItem())
      {
         mpItem = (T*)mpLinkedList->mpUsedTail;
         BDEBUG_ASSERT(mpItem);
      }
      else
      {
         mpItem = (T*)((BNode*)mpItem)->mpPrev;
         BDEBUG_ASSERT(mpItem);
      }
      
      return *this;
   }
     
   const T& operator*() const 
   {  
      BDEBUG_ASSERT(mpLinkedList && mpItem && (mpItem != getLLEndItem()) && !mpLinkedList->isEmpty());
      return *mpItem;
   }
   
   const T* operator->() const 
   { 
      BDEBUG_ASSERT(mpLinkedList && mpItem && (mpItem != getLLEndItem()) && !mpLinkedList->isEmpty());
      return mpItem;
   }
   
   operator bool() const { return mpItem != NULL; }
   
   bool operator == (const BLinkedListIterator<T, Allocator>& b) const { return (mpLinkedList == b.mpLinkedList) && (mpItem == b.mpItem); }
   bool operator != (const BLinkedListIterator<T, Allocator>& b) const { return !(*this == b); }
   
   bool operator == (const BConstLinkedListIterator& b) const { return (mpLinkedList == b.mpLinkedList) && (mpItem == b.mpItem); }
   bool operator != (const BConstLinkedListIterator& b) const { return !(*this == b); }
};

//----------------------------------------------------------------------------
//  Class BLinkedList
//----------------------------------------------------------------------------
template <class T, class Allocator = BPrimaryFixedHeapAllocator> 
class BLinkedList
{
public:
   typedef T                                          valueType;
   typedef Allocator                                  allocator;
   typedef BConstLinkedListIterator <T, Allocator>    constIterator;
   typedef BLinkedListIterator      <T, Allocator>    iterator;
      
   friend class constIterator;
   friend class iterator;
   
   //-- Callback Prototypes
   typedef int (CALLBACK BCompareFunc)(const T& item1, const T& item2, void* pParam);

   //-- Construction/Destruction
   enum { cGrowExponential = 0xFFFFFFFF };
   BLinkedList(uint initialSize = 0, uint growSize = cGrowExponential, Allocator& alloc = Allocator());
   BLinkedList(const BLinkedList<T, Allocator>& list);
   BLinkedList(Allocator& alloc);
   ~BLinkedList();

   //-- List Control
   void   reset      (uint initialSize = 0, uint growSize = cGrowExponential);
   void   empty      ();
   void   optimize   ();
   bool   validate(void) const;
   
   Allocator&  getAllocator(void) { return mAllocator; }
   void        setAllocator(const Allocator& alloc) { reset(0, mGrowSize); mAllocator = alloc; }

   //-- List Info
   bool     isEmpty          () const;
   bool     isGrowExponential() const;
   bool     isUsingBlockAllocator() const;
   uint     getSize          () const;
   uint     getGrowSize      () const;
   uint     getAllocatedSize () const;
   
   //-- Adding Items
   iterator addToHead(const T& item);
   iterator addToTail(const T& item);
         
   iterator addBefore(const T& item, iterator hItem);
   iterator addAfter (const T& item, iterator hItem);
   
   iterator addSorted(const T& item, bool ascending = true);
   iterator addSorted(const T& item, BCompareFunc* pFunc, void* pParam, bool ascending = true);
   
   iterator pushFront(const T& item) { return addToHead(item); }
   iterator pushBack(const T& item) { return addToTail(item); }

   //-- Removing Items
   void     removeHead      ();
   void     removeTail      ();
   void     remove          (iterator hItem);
   iterator removeAndGetPrev(iterator hItem);
   iterator removeAndGetNext(iterator hItem);

   //-- Relocation
   void     moveToHead(iterator hItem);
   void     moveToTail(iterator hItem);
   
   void     swap(iterator a, iterator b);

   //-- Navigation   
   constIterator  getIterator(const T* p) const { return constIterator(this, p); }
        iterator  getIterator(      T* p)       { return      iterator(this, p); }
   
   constIterator  getItem   (uint index) const;
        iterator  getItem   (uint index);
   
   constIterator  getHead   (void) const;
        iterator  getHead   (void);
   
   constIterator  getTail   (void) const;
         iterator getTail   (void);   
         
   constIterator  getPrev    (constIterator hItem) const;
         iterator getPrev    (iterator hItem);
   
   constIterator  getNext    (constIterator hItem) const;
         iterator getNext    (iterator hItem);
   
   constIterator  getPrevWithWrap   (constIterator hItem) const;
         iterator getPrevWithWrap   (iterator hItem);
   
   constIterator  getNextWithWrap   (constIterator hItem) const;
         iterator getNextWithWrap   (iterator hItem);
      
   iterator          begin(void)       { return isEmpty() ? end() : getHead(); }
   constIterator     begin(void) const { return isEmpty() ? end() : getHead(); }
   iterator          end(void)         { return iterator(this, getLLEndItem()); }
   constIterator     end(void) const   { return constIterator(this, getLLEndItem()); }
   
   const T& front(void) const { BDEBUG_ASSERT(!isEmpty()); return mpUsedHead->mItem; }
         T& front(void)       { BDEBUG_ASSERT(!isEmpty()); return mpUsedHead->mItem; }
         
   const T& back(void) const { BDEBUG_ASSERT(!isEmpty()); return mpUsedTail->mItem; }
         T& back(void)       { BDEBUG_ASSERT(!isEmpty()); return mpUsedTail->mItem; }
      
   //-- Searching and Sorting
   constIterator findItemForward (const T& item, constIterator hStart = constIterator()) const;
   constIterator findItemBackward(const T& item, constIterator hStart = constIterator()) const;
   constIterator findItemForward (BCompareFunc* pFunc, void* pParam, const T& item, constIterator hStart = constIterator()) const;
   constIterator findItemBackward(BCompareFunc* pFunc, void* pParam, const T& item, constIterator hStart = constIterator()) const;
   
   //iterator findItemForward (const T& item, iterator hStart = iterator());
   //iterator findItemBackward(const T& item, iterator hStart = iterator());
   //iterator findItemForward (BCompareFunc* pFunc, void* pParam, const T& item, iterator hStart = iterator());
   //iterator findItemBackward(BCompareFunc* pFunc, void* pParam, const T& item, iterator hStart = iterator());
   
   void   quickSort       (bool ascending = true);
   void   insertionSort   (bool ascending = true);
   void   quickSort       (BCompareFunc* pFunc, void* pParam, bool ascending = true);
   void   insertionSort   (BCompareFunc* pFunc, void* pParam, bool ascending = true);

   //-- Operators
   BLinkedList<T, Allocator>& operator = (const BLinkedList<T, Allocator>& list);

private:
   //-- Private structs
   
   template<uint i> struct BNodeAligner { };
   // This assumes 4-byte pointers!
   template<> struct BNodeAligner<16> { uint mPadding[2]; };
   
   struct BNode
   {
      // mItemData MUST be first
      uchar   mItemData[sizeof(T)];
      
      BNode* mpPrev;
      BNode* mpNext;

      BNodeAligner<ALIGN_OF(T)> mAligner;
      
      T* getItemPtr(void) 
      { 
         return reinterpret_cast<T*>(mItemData); 
      }
      
      const T* getItemPtr(void) const 
      { 
         return reinterpret_cast<const T*>(mItemData); 
      }
      
      void construct(void) 
      { 
         if (!BIsBuiltInType<T>::Flag)
            Utils::ConstructInPlace(getItemPtr()); 
      }   
      
      void construct(const T& item) 
      { 
         Utils::ConstructInPlace(getItemPtr(), item); 
      }   
      
      void destruct(void)
      {
         if (!BIsBuiltInType<T>::Flag) Utils::DestructInPlace(getItemPtr());
      }
      
      void putItem(const T& item) 
      { 
         *getItemPtr() = item; 
      }
      
      const T& getItem(void) const 
      { 
         return *getItemPtr(); 
      }
      
      T& getItem(void) 
      { 
         return *getItemPtr(); 
      }
      
      __declspec(property(get = getItem, put = putItem)) T mItem;
   };

   struct BBlock
   {
      uint   mNumNodes;
      BBlock* mpNextBlock;
      BNode*  mpNodes;
   };
         
   void    destructAllNodes(void);
   bool    isValidIterator(constIterator hItem) const; 
   void    addToFreeHead(BNode* pNode, bool destruct);
   void    addBlock     (uint numNodes);
   BNode*  getFreeNode  ();
   void    swapNodes    (BNode*& pNode1, BNode*& pNode2);
   void    linkToHead   (BNode* pNode);
   void    linkToTail   (BNode* pNode);
   void    unlink       (BNode* pNode);

   //-- Sorting Functions
   BNode*  getPivot     (bool ascending, uint numNodes, BNode*& pFirst, BNode*& pLast);
   void    quickSort    (bool ascending, uint numNodes, BNode*  pFirst, BNode*  pLast);
   void    insertionSort(bool ascending, uint numNodes, BNode*  pFirst, BNode*  pLast);
   BNode*  getPivot     (bool ascending, uint numNodes, BNode*& pFirst, BNode*& pLast, BCompareFunc* pFunc, void* pParam);
   void    quickSort    (bool ascending, uint numNodes, BNode*  pFirst, BNode*  pLast, BCompareFunc* pFunc, void* pParam);
   void    insertionSort(bool ascending, uint numNodes, BNode*  pFirst, BNode*  pLast, BCompareFunc* pFunc, void* pParam);
   
   void    updateValidObjects(int i);

   //-- Private Data
   uint              mSize;
   uint              mGrowSize;
   uint              mAllocatedSize;
      
   BBlock*           mpBlocks;
   
   BNode*            mpFreeHead;
   
   BNode*            mpUsedHead;
   BNode*            mpUsedTail;

#ifdef BUILD_DEBUG
   int               mNumValidObjects;
#endif
   
   Allocator         mAllocator;
   
   bool              mUseBlockAllocator : 1;
   bool              mGrowExponential : 1;
         
   struct BPointerRange
   {
      BPointerRange() { }
      BPointerRange(void* pBegin, void* pEnd) : mpBegin(pBegin), mpEnd(pEnd) { }

      void* mpBegin;
      void* mpEnd;

      bool operator< (const BPointerRange& other) const { return mpEnd < other.mpBegin; }  

      bool operator== (const BPointerRange& other) const 
      { 
         if (mpBegin > other.mpEnd)
            return false;
         if (mpEnd < other.mpBegin)
            return false;
         return true;
      } 
   };
   
   static T*      getLLEndItem(void) { return (T*)0xFFFFFFFF; }
   static BNode*  getLLEndNode(void) { return (BNode*)0xFFFFFFFF; }
   
   static bool isEnd(const T* p) { return p == getLLEndItem(); }
   static bool isEnd(const BNode* p) { return p == getLLEndNode(); }
};

//----------------------------------------------------------------------------
//  Template Implementation
//----------------------------------------------------------------------------
#include "linkedList.inl"
