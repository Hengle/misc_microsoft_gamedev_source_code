// File: skiplist.inl

template <class Key, class T, uint N, class LessThan, class Allocator>
inline void BSkipList<Key, T, N, LessThan, Allocator>::allocHeadNode()
{
   if (!mpHead)
   {
      mpHead = NodeType::allocNode(mMaxListLevel + 1, mAlloc);
      mpNil = mpHead;

      mpHead->setHeadFlag();

      for (uint i = 0; i <= mMaxListLevel; i++)
         mpHead->setNext(i, mpNil);   

      mpHead->setPrev(mpHead);
   }
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline void BSkipList<Key, T, N, LessThan, Allocator>::freeHeadNode()
{  
   if (mpHead)
   {
      NodeType::freeNode(mpHead, mAlloc);
      mpHead = NULL;
      mpNil = NULL;
   }
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline void BSkipList<Key, T, N, LessThan, Allocator>::init(uint maxListLevel)
{
   maxListLevel = Math::Min<uint>(maxListLevel, cMaxEverListLevel);

   mMaxListLevel = maxListLevel;
   mListLevel = 0;
   mNumItems = 0;
   mNumNodes = 0;
   mRandSeed = 0x1234ABCD;

   mpHead = NULL;
   mpNil = NULL;
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline void BSkipList<Key, T, N, LessThan, Allocator>::deinit()
{
   clear();
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline void BSkipList<Key, T, N, LessThan, Allocator>::clone(const BSkipList& other)
{
   clear();
   
   mMaxListLevel = other.mMaxListLevel;
   mRandSeed = other.mRandSeed;
      
   if (other.getEmpty())
      return;

   allocHeadNode();

   NodeType* pUpdate[cMaxEverListLevel + 1];
   for (uint i = 0; i <= cMaxEverListLevel; i++)
      pUpdate[i] = mpHead;

   NodeType* pNewNode = NULL;
   uint newNodeLevel = 0;
   uint numNewNodeItems = 0;
   for (const_iterator it = other.begin(); it != other.end(); ++it)
   {
      if (!pNewNode)
      {
         newNodeLevel = getRandLevel();
         mListLevel = Math::Max(mListLevel, newNodeLevel);
         
         pNewNode = NodeType::allocNode(newNodeLevel + 1, mAlloc);
         mNumNodes++;
         numNewNodeItems = 0;

         for (uint i = 0; i <= newNodeLevel; i++)
         {
            pNewNode->setNext(i, mpNil);
            if (i == 0)
            {
               pNewNode->setPrev(pUpdate[i]);

               mpNil->setPrev(pNewNode);
            }

            pUpdate[i]->setNext(i, pNewNode);

            pUpdate[i] = pNewNode;
         }
      }

      pNewNode->constructItem(numNewNodeItems, *it);

      numNewNodeItems++;

      if (numNewNodeItems == N)
      {
         pNewNode->setNumItems(numNewNodeItems);
         pNewNode = NULL;   
      }
   }

   if (pNewNode)
      pNewNode->setNumItems(numNewNodeItems);

   mNumItems = other.mNumItems;
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline uint BSkipList<Key, T, N, LessThan, Allocator>::getRandLevel()
{
   if (!mMaxListLevel)
      return 0;
      
   DWORD jsr = mRandSeed;
   
   // Shouldn't happen
   BDEBUG_ASSERT(jsr);
            
   jsr ^= (jsr << 17);
   jsr ^= (jsr >> 13);
   jsr ^= (jsr << 5);
   mRandSeed = jsr;
      
   const uint level = Utils::CountLeadingZeros32(jsr);

   return Math::Min(level, mMaxListLevel);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline BSkipList<Key, T, N, LessThan, Allocator>::BSkipList(int maxListLevel, const LessThan& lessThanFunctor, const Allocator& alloc) :
   mLessThan(lessThanFunctor),
   mAlloc(alloc)
{
   init(maxListLevel);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline BSkipList<Key, T, N, LessThan, Allocator>::BSkipList(const BSkipList& other) :
   mLessThan(other.mLessThan),
   mAlloc(other.mAlloc),
   mpHead(NULL),
   mpNil(NULL),
   mNumItems(0),
   mNumNodes(0),
   mListLevel(0),
   mMaxListLevel(0),
   mRandSeed(0x1234ABCD)
{
   clone(other);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline BSkipList<Key, T, N, LessThan, Allocator>& BSkipList<Key, T, N, LessThan, Allocator>::operator= (const BSkipList& rhs)
{
   if (this == &rhs)
      return *this;

   deinit();

   mLessThan = rhs.mLessThan;
   mAlloc = rhs.mAlloc;

   clone(rhs);

   return *this;
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline BSkipList<Key, T, N, LessThan, Allocator>::~BSkipList()
{
   deinit();
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline void BSkipList<Key, T, N, LessThan, Allocator>::setMaxListLevel(uint maxListLevel)
{
   clear();

   mMaxListLevel = maxListLevel;
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline void BSkipList<Key, T, N, LessThan, Allocator>::clear()
{
   if (mpHead)
   {
      NodeType* pNode = mpHead->getNext(0);
      while (pNode != mpHead)
      {
         NodeType* pNext = pNode->getNext(0);
         NodeType::freeNode(pNode, mAlloc);
         pNode = pNext;
      }

      freeHeadNode();
   }

   mNumItems = 0;
   mNumNodes = 0;
   mListLevel = 0;
}
   
// Returned bool will be true if the key is inserted, or false if the key already exists.
// The returned iterator will point to the existing, or newly inserted entry.
// If the key already exists, and allowDuplicates is false, the returned iterator will point to the first instance of the key.
template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::InsertResult BSkipList<Key, T, N, LessThan, Allocator>::insert(const Key& key, const T& data, bool allowDuplicates)
{
   NodeType* pUpdate[cMaxEverListLevel + 1];

   allocHeadNode();

   NodeType* pNode = mpHead;

   for (int i = mListLevel; i >= 0; i--) 
   {
      for ( ; ; )
      {
         NodeType* pNext = pNode->getNext(i);

         if ((pNext != mpNil) && (keyLess(pNext->getHighestKey(), key)))
            pNode = pNext;
         else  
            break;
      }

      pUpdate[i] = pNode;
   }

   BDEBUG_ASSERT((pNode == mpHead) || keyLess(pNode->getHighestKey(), key));

   NodeType* pPrevNode = pNode;
   pNode = pNode->getNext(0);

   // pPrevNode - all elements must be < key
   // pNode - some elements may be <= key, one or more must be >= key
   // pPrevNode and/or pNode may be mpHead

   uint nodeIndex = 0;
   bool nodeFoundKey = false;
   if (pNode != mpHead)
   {
      nodeFoundKey = pNode->findLowerKey(key, nodeIndex, mLessThan);
      if ((!allowDuplicates) && (nodeFoundKey))
         return InsertResult(iterator(this, pNode, nodeIndex), false);
   }

   mNumItems++;

   if (N > 1)
   {
      if ((pPrevNode != mpHead) && (nodeIndex == 0))
      {
         if (!pPrevNode->isFull())
         {
            uint itemIndex = pPrevNode->getNumItems();
            pPrevNode->insertItem(itemIndex, key, data);
            return InsertResult(iterator(this, pPrevNode, itemIndex), true);
         }
      }

      if (pNode != mpHead) 
      {
         if (!pNode->isFull())
         {
            pNode->insertItem(nodeIndex, key, data);
            return InsertResult(iterator(this, pNode, nodeIndex), true);
         }
         else if ((nodeIndex < N) && (pNode->tryRotateNext()))
         {
            pNode->insertItem(nodeIndex, key, data);
            return InsertResult(iterator(this, pNode, nodeIndex), true);
         }
      }
   }      

   InsertResult results;
   results.second = true;

   const uint newLevel = getRandLevel();      

   NodeType* pNewNode = NodeType::allocNode(newLevel + 1, mAlloc);
   mNumNodes++;

   BDEBUG_ASSERT(nodeIndex < N);

   if ((pNode == mpHead) || (N == 1))
   {
      pNewNode->insertItem(0, key, data);

      results.first = iterator(this, pNewNode, 0);
   }
   else
   {
      BDEBUG_ASSERT(pNode->getNumItems() == N);

      for (uint i = 0; i < nodeIndex; i++)
      {
         pNewNode->constructItem(i, pNode->getItem(i));
         pNode->destroyItem(i);
      }

      pNewNode->constructItem(nodeIndex, key, data);
      pNewNode->setNumItems(nodeIndex + 1);

      if (nodeIndex > 0)
      {
         for (uint i = nodeIndex; i < N; i++)
         {
            pNode->constructItem(i - nodeIndex, pNode->getItem(i));
            pNode->destroyItem(i);  
         }
         pNode->setNumItems(N - nodeIndex);
      }            

      results.first = iterator(this, pNewNode, nodeIndex);               
   }

   if (newLevel > mListLevel) 
   {
      for (uint i = mListLevel + 1; i <= newLevel; i++)
         pUpdate[i] = mpNil;

      mListLevel = newLevel;
   }

   BDEBUG_ASSERT(pNode->getPrev() == pPrevNode);
   BDEBUG_ASSERT(pNode->getNext(0)->getPrev() == pNode);

   pNewNode->setNext(0, pNode);
   pNewNode->setPrev(pPrevNode);

   pNode->setPrev(pNewNode);
   pPrevNode->setNext(0, pNewNode);

   for (uint i = 1; i <= newLevel; i++) 
   {
      pNewNode->setNext(i, pUpdate[i]->getNext(i));

      pUpdate[i]->setNext(i, pNewNode);
   }

   return results;
}

// If the key exists, the returned iterator will point to the first instance of the key. Otherwise end() is returned.
template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::const_iterator BSkipList<Key, T, N, LessThan, Allocator>::find(const Key& key) const
{
   if (!mpHead)
      return end();

   NodeType* pNode = mpHead;

   for (int i = mListLevel; i >= 0; i--) 
   {
      for ( ; ; )
      {
         NodeType* pNext = pNode->getNext(i);

         if ((pNext != mpNil) && (keyLess(pNext->getHighestKey(), key)))
            pNode = pNext;
         else  
            break;
      }
   }

   pNode = pNode->getNext(0);
   if (pNode != mpNil)
   {
      uint index;
      if (pNode->findLowerKey(key, index, mLessThan))
         return const_iterator(this, pNode, index);
   }            

   return end();
}

// If the key exists, the returned iterator will point to the first instance of the key. Otherwise, end() is returned.
template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::iterator BSkipList<Key, T, N, LessThan, Allocator>::find(const Key& key)
{
   const BSkipList& skipList = *this;
   const_iterator it(skipList.find(key));
   return iterator(this, const_cast<NodeType*>(it.mpNode), it.mItemIndex);
}

// Returns an iterator to the first element with a key value that is equal to or greater than that of a specified key.
template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::const_iterator BSkipList<Key, T, N, LessThan, Allocator>::lowerBound(const Key& key) const
{
   if (!mpHead)
      return end();

   NodeType* pNode = mpHead;

   for (int i = mListLevel; i >= 0; i--) 
   {
      for ( ; ; )
      {
         NodeType* pNext = pNode->getNext(i);

         if ((pNext != mpNil) && (keyLess(pNext->getHighestKey(), key)))
            pNode = pNext;
         else  
            break;
      }
   }

   //NodeType* pPrevNode = pNode;
   pNode = pNode->getNext(0);

   // pPrevNode - all elements must be < key
   // pNode - some elements may be <= to key, at least one must be >= to key
   // pPrevNode and/or pNode may be mpHead

   if (pNode == mpNil)
      return end();

   uint index;
   pNode->findLowerKey(key, index, mLessThan);
   return const_iterator(this, pNode, index);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::iterator BSkipList<Key, T, N, LessThan, Allocator>::lowerBound(const Key& key) 
{
   const BSkipList& skipList = *this;
   const_iterator it(skipList.lowerBound(key));
   return iterator(this, const_cast<NodeType*>(it.mpNode), it.mItemIndex);
}

// Returns an iterator to the first element with a key having a value that is greater than that of a specified key.
template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::const_iterator BSkipList<Key, T, N, LessThan, Allocator>::upperBound(const Key& key) const
{
   if (!mpHead)
      return end();

   NodeType* pNode = mpHead;

   for (int i = mListLevel; i >= 0; i--) 
   {
      for ( ; ; )
      {
         NodeType* pNext = pNode->getNext(i);

         if ((pNext != mpNil) && (keyLessEqual(pNext->getHighestKey(), key)))
            pNode = pNext;
         else  
            break;
      }
   }

   //NodeType* pPrevNode = pNode;
   pNode = pNode->getNext(0);

   // pPrevNode - all elements must be <= key
   // pNode - some elements may be <= to key, at least one must be > to key
   // pPrevNode and/or pNode may be mpHead

   if (pNode == mpNil)
      return end();

   uint index = pNode->findUpperKeyBinary(key, mLessThan);
   return const_iterator(this, pNode, index);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::iterator BSkipList<Key, T, N, LessThan, Allocator>::upperBound(const Key& key) 
{
   const BSkipList& skipList = *this;
   const_iterator it(skipList.upperBound(key));
   return iterator(this, const_cast<NodeType*>(it.mpNode), it.mItemIndex);
}

// This method scans the nodes starting from the head. This may be slow!
template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::const_iterator BSkipList<Key, T, N, LessThan, Allocator>::findByIndex(uint itemIndex) const
{
   if (itemIndex >= mNumItems)
      return end();

   if (itemIndex < (mNumItems >> 1))
   {
      // Scan forwards.
      const NodeType* pCurNode = mpHead->getNext(0);
      uint curItemIndex = 0;
      while (pCurNode != mpHead)
      {
         const uint nextItemIndex = curItemIndex + pCurNode->getNumItems();

         if ((itemIndex >= curItemIndex) && (itemIndex < nextItemIndex))
            return const_iterator(this, pCurNode, itemIndex - curItemIndex);

         curItemIndex = nextItemIndex;
         pCurNode = pCurNode->getNext(0);            
      }
   }
   else
   {
      // Scan backwards.
      const NodeType* pCurNode = mpHead->getPrev();
      int curItemIndex = mNumItems - 1;
      while (pCurNode != mpHead)
      {
         const int nextItemIndex = curItemIndex - pCurNode->getNumItems();

         if (((int)itemIndex > nextItemIndex) && ((int)itemIndex <= curItemIndex))
            return const_iterator(this, pCurNode, itemIndex - nextItemIndex - 1);

         curItemIndex = nextItemIndex;
         pCurNode = pCurNode->getPrev();
      }
   }         

   return end();
}

// This method scans the nodes starting from the head. This may be slow!
template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::iterator BSkipList<Key, T, N, LessThan, Allocator>::findByIndex(uint itemIndex) 
{
   const BSkipList& skipList = *this;
   const_iterator it(skipList.findByIndex(itemIndex));
   return iterator(this, const_cast<NodeType*>(it.mpNode), it.mItemIndex);
}

// Deletes the first instance of key.
// Returns true if found.
template <class Key, class T, uint N, class LessThan, class Allocator>
inline bool BSkipList<Key, T, N, LessThan, Allocator>::erase(const Key& key)
{
   if (!mpHead)
      return false;

   NodeType* pUpdate[cMaxEverListLevel + 1];

   NodeType* pNode = mpHead;

   for (int i = mListLevel; i >= 0; i--) 
   {
      for ( ; ; )
      {
         NodeType* pNext = pNode->getNext(i);

         if ((pNext != mpNil) && (keyLess(pNext->getHighestKey(), key)))
            pNode = pNext;
         else  
            break;
      }

      pUpdate[i] = pNode;
   }

   pNode = pNode->getNext(0);

   if (pNode == mpNil) 
      return false;

   uint index;
   bool keyFound = pNode->findLowerKey(key, index, mLessThan);
   if (!keyFound)
      return false;

   pNode->removeItem(index);
   mNumItems--;

   if (!pNode->isEmpty())      
      return true;

   pUpdate[0]->setNext(0, pNode->getNext(0));
   pNode->getNext(0)->setPrev(pUpdate[0]);

   for (uint i = 1; i <= mListLevel; i++)
   {
      if (pUpdate[i]->getNext(i) != pNode)
         break;

      pUpdate[i]->setNext(i, pNode->getNext(i));
   }

   BDEBUG_ASSERT(mNumNodes > 0);
   mNumNodes--;
   NodeType::freeNode(pNode, mAlloc);

   while ((mListLevel > 0) && (mpHead->getNext(mListLevel) == mpNil))
      mListLevel--;

   return true;
}

// Deletes the exact item pointed to by the iterator
// Returns true if found.
template <class Key, class T, uint N, class LessThan, class Allocator>
inline bool BSkipList<Key, T, N, LessThan, Allocator>::erase(const iterator& it)
{
   if ((!mpHead) || (it.mpNode == mpHead) || (!it.mpNode))
      return false;

   NodeType* pUpdate[cMaxEverListLevel + 1];

   NodeType* pNode = mpHead;

   for (int i = mListLevel; i >= 0; i--) 
   {
      for ( ; ; )
      {
         NodeType* pNext = pNode->getNext(i);

         if ((pNext != mpNil) && (keyLess(pNext->getHighestKey(), it->first)))
            pNode = pNext;
         else  
            break;
      }

      pUpdate[i] = pNode;
   }

   NodeType* pPrevDelNode = pNode;
   pNode = pNode->getNext(0);
   if (pNode == mpNil)
      return false;

   NodeType* pDelNode = pNode;
   while (pDelNode != it.mpNode)
   {
      pPrevDelNode = pDelNode;
      pDelNode = pDelNode->getNext(0);
      if (pDelNode == mpNil)
         return false;
   }      

   if (it.mItemIndex >= pDelNode->getNumItems())
      return false;

   mNumItems--;

   if (pDelNode->getNumItems() > 1)
   {
      pDelNode->removeItem(it.mItemIndex);
      return true;               
   }            

   BDEBUG_ASSERT(pPrevDelNode->getNext(0) == pDelNode);
   pPrevDelNode->setNext(0, pDelNode->getNext(0));
   pDelNode->getNext(0)->setPrev(pPrevDelNode);

   const Key& delKey = pDelNode->getItemKey(it.mItemIndex);

   const uint delNodeLevel = pDelNode->getNumNextPointers() - 1;
   for (uint i = 1; i <= delNodeLevel; i++)
   {
      NodeType* pPrevNode = pUpdate[i];
      NodeType* pCurNode = pUpdate[i]->getNext(i);

      for ( ; ; )
      {
         if (pCurNode == pDelNode)
         {
            pPrevNode->setNext(i, pDelNode->getNext(i));  
            break;
         }

         NodeType* pNextNode = pCurNode->getNext(i);
         if (pNextNode == mpNil)
            break;

         if (keyGreater(pNextNode->getLowestKey(), delKey))
            break;

         pPrevNode = pCurNode;
         pCurNode = pNextNode;
      }
   }

   BDEBUG_ASSERT(mNumNodes > 0);
   mNumNodes--;
   NodeType::freeNode(pDelNode, mAlloc);

   while ((mListLevel > 0) && (mpHead->getNext(mListLevel) == mpNil))
      mListLevel--;

   return true;
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::iterator BSkipList<Key, T, N, LessThan, Allocator>::begin() 
{
   if (!mpHead) 
      return end();
   return iterator(this, mpHead->getNext(0), 0);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::const_iterator BSkipList<Key, T, N, LessThan, Allocator>::begin() const
{
   if (!mpHead) 
      return end();
   return const_iterator(this, mpHead->getNext(0), 0);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::iterator BSkipList<Key, T, N, LessThan, Allocator>::end() 
{
   return iterator(this, mpHead, 0);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
inline typename BSkipList<Key, T, N, LessThan, Allocator>::const_iterator BSkipList<Key, T, N, LessThan, Allocator>::end() const
{
   return const_iterator(this, mpHead, 0);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
void BSkipList<Key, T, N, LessThan, Allocator>::swap(SkipListType& other)
{
   std::swap(mpHead,          other.mpHead);
   std::swap(mpNil,           other.mpNil);
   std::swap(mListLevel,      other.mListLevel);
   std::swap(mMaxListLevel,   other.mMaxListLevel);
   std::swap(mNumItems,       other.mNumItems);
   std::swap(mNumNodes,       other.mNumNodes);
   std::swap(mLessThan,       other.mLessThan);
   std::swap(mAlloc,          other.mAlloc);
   std::swap(mRandSeed,       other.mRandSeed);
}

template <class Key, class T, uint N, class LessThan, class Allocator>
bool BSkipList<Key, T, N, LessThan, Allocator>::operator== (const BSkipList& other) const
{
   if (mNumItems != other.mNumItems)
      return false;

   const_iterator itL = begin();
   const_iterator itR = other.begin();

   while (itL != end())
   {
      if (*itL != *itR)
         return false;

      ++itL;
      ++itR;
   }

   return true;
}

template <class Key, class T, uint N, class LessThan, class Allocator>
bool BSkipList<Key, T, N, LessThan, Allocator>::operator< (const BSkipList& other) const
{
   if (mNumItems < other.mNumItems)
      return true;
   else if (mNumItems == other.mNumItems)
   {
      const_iterator itL = begin();
      const_iterator itR = other.begin();

      while (itL != end())
      {
         if (*itL < *itR)
            return true;
         else if (!(*itL == *itR))
            return false;

         ++itL;
         ++itR;
      }
   }         

   return false;
}

template <class Key, class T, uint N, class LessThan, class Allocator>
bool BSkipList<Key, T, N, LessThan, Allocator>::check() const
{
   if (!mpHead)
   {
      if (mNumItems)
         return false;
      return true;
   }

   if (mpHead->getNumItems())
      return false;
   if (mpHead != mpNil)
      return false;

   if (!mNumItems)
   {
      if (mpHead->getNext(0) != mpHead)
         return false;
      return true;
   }

   if (mListLevel > mMaxListLevel)
      return false;

   for (uint level = 0; level <= mListLevel; level++)
   {
      NodeType* pCur = mpHead->getNext(level);
      if (level == 0)
      {
         if (pCur->getPrev() != mpHead)
            return false;
      }

      uint totalItemsFound = 0;
      uint totalNodesFound = 0;

      while (pCur != mpHead)
      {
         if ((!pCur->getNumItems()) || (pCur->getNumItems() > N))
            return false;

         totalItemsFound += pCur->getNumItems();

         if ((totalItemsFound > mNumItems) || (totalNodesFound > mNumNodes))
            return false;

         totalNodesFound++;

         for (int i = 0; i < (int)pCur->getNumItems() - 1; i++)
            if (!keyLessEqual(pCur->getItemKey(i), pCur->getItemKey(i + 1)))
               return false;

         NodeType* pNext = pCur->getNext(level);
         if ((!pNext) || (pNext == pCur))
            return false;

         if ((level == 0) && (pNext->getPrev() != pCur))
            return false;

         if (pNext != mpHead)
         {
            if (!keyLessEqual(pCur->getHighestKey(), pNext->getLowestKey()))
               return false;
         }                  

         pCur = pNext;
      }      

      if (totalItemsFound > mNumItems)
         return false;
      if ((level == 0) && (totalItemsFound != mNumItems))
         return false;
      if ((level == 0) && (totalNodesFound != mNumNodes))
         return false;
      //printf("Check: Level %u Nodes: %u\n", level, totalNodesFound);
   }        

   for (uint i = 0; i <= mListLevel; i++)
   {
      if (mpHead->getNext(i) == mpNil)
         return false;  
   }

   for (uint i = mListLevel + 1; i <= mMaxListLevel; i++)
   {
      if (mpHead->getNext(i) != mpNil)
         return false;  
   }

   return true;
}