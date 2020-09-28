//==============================================================================
// linearprobehashtable.h
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================
#pragma once

#include "containers\dynamicArray.h"
#include "hash\hash.h"

template<typename Key, typename Type>
struct BLinearProbeHashTableTraits
{
   // Hashes key.
   size_t operator()(const Key& key) const
   {
      return hashFast(reinterpret_cast<const uchar*>(&key), sizeof(key));
   }

   // True if the value is valid.
   // By default the memory block holding the nodes is initialized to all 0's.
   bool isValidValue(const std::pair<const Key, Type>& value) const
   {
      return (0 != value.first) || (0 != value.second);
   }
};

// Algorithm from Knuth's Sorting and Searching, section 6.4, Searching Algorithm L.
// This container can be useful when your keys and associated values are small. The memory for all keys/values
// is allocated up front in one block, and key collision searches are resolved with simple linear probing to minimize 
// cache misses during probing.
// Don't use this class to hold large types.
template<typename Key, typename Type, typename Traits = BLinearProbeHashTableTraits<Key, Type> >
class BLinearProbeHashTable
{
public:
   typedef Key key_type;
   typedef Type mapped_type;
   typedef Traits key_traits;
   typedef std::pair<const Key, Type> value_type;
      
   BLinearProbeHashTable(uint reserveSize = 0) :
      mpNodes(NULL),
      mpEndOfNodes(NULL),
      mNumNodes(0),
      mNumValid(0),
      mNumValidGrowThresh(0),
      mTraits()
   {
      if (reserveSize)
      {
         if (!Math::IsPow2(reserveSize))
            reserveSize = Math::NextPowerOf2(reserveSize);
         
         setNodeBufSize(reserveSize);
      }            
   }

   // Second is true if insertion occured.
   // Container will be automatically grown if it becomes too full.
   std::pair<value_type*, bool> insert(const value_type& value, bool allowGrowing = true)
   {
      if (!mpNodes)
         setNodeBufSize(1);
         
      const uint h = mTraits(value.first) & (mNumNodes - 1);

      BNode* pFirstNode = mpNodes + h;   
      BNode* pNode = pFirstNode;

      for ( ; ; )
      {
         if (!mTraits.isValidValue(pNode->mValue))
            break;            

         // Found value?
         if (value.first == pNode->mValue.first)
            return std::make_pair(&pNode->mValue, false);

         pNode++;
         if (pNode == mpEndOfNodes)
            pNode = mpNodes;

         if (pNode == pFirstNode)
         {
            if (!allowGrowing)
            {
               BFAIL("BLinearProbeHash::insert: Can't grow container");
               // It makes no sense to continue here, but we've got to do something
               break;
            }

            return growAndInsert(value);
         }
      }

      if ((mNumValid >= mNumValidGrowThresh) && (allowGrowing))
         return growAndInsert(value);

      mNumValid++;

      Utils::ConstructInPlace(pNode, BNode(value));
      return std::make_pair(&pNode->mValue, true);
   }
   
   // NULL if not found.
   value_type* find(const value_type& value)
   {
      if (!mpNodes)
         return NULL;

      const uint h = mTraits(value.first) & (mNumNodes - 1);

      BNode* pFirstNode = mpNodes + h;   
      BNode* pNode = pFirstNode;

      for ( ; ; )
      {
         if (!mTraits.isValidValue(pNode->mValue))
            break;            

         // Found value?
         if (value.first == pNode->mValue.first)
            return &pNode->mValue;

         pNode++;
         if (pNode == mpEndOfNodes)
            pNode = mpNodes;

         if (pNode == pFirstNode)
            return NULL;
      }

      return NULL;
   }

   // Deletes all objects in container but doesn't free the allocated memory block.
   void deleteAll(void)
   {
      if (mNumValid)
      {
         for (uint i = 0; i < mNumNodes; i++)
         {
            if (mTraits.isValidValue(mpNodes[i].mValue))
            {
               Utils::DestructInPlace(&mpNodes[i]);

               memset(&mpNodes[i], 0, sizeof(BNode));

               mNumValid--;
               if (0 == mNumValid)
                  break;
            }
         }
      }
   }

   // Deletes all objects in container and frees the memory block.
   void clear(void)
   {
      deleteAll();
         
      mNodeBuf.clear();
      
      mpNodes = NULL;
      mpEndOfNodes = NULL;
      mNumNodes = 0;
      mNumValid = 0;
      mNumValidGrowThresh = 0;
   }
   
   // Grows the container's memory block so it can hold at least numNodes nodes before it must be grown again.
   void reserve(uint numNodes)
   {
      if (numNodes)
      {
         if (!Math::IsPow2(numNodes))
            numNodes = Math::NextPowerOf2(numNodes);
            
         if (numNodes > mNumNodes)
            grow(numNodes);
      }            
   }

   // Returns number of valid nodes in container.
   uint numValid(void) const
   {
      return mNumValid;
   }

   // Returns number of nodes in memory block, for iteration purposes.
   uint numNodes(void) const
   {
      return mNumNodes;
   }
   
   // Returns pointer to node's value or NULL, for iteration purposes.
   value_type* getNode(uint nodeIndex) const
   {
      BDEBUG_ASSERT(nodeIndex < mNumNodes);
      
      if (nodeIndex >= mNumNodes)
         return NULL;
      
      if (!mTraits.isValidValue(mpNodes[nodeIndex].mValue))
         return NULL;
         
      return &mpNodes[nodeIndex].mValue;
   }

private:
   struct BNode
   {
      value_type mValue;

      BNode(const value_type& value) : mValue(value) { }
   };
   
   typedef BDynamicArray<uchar, 16, false, true, true> BNodeBufArray;
   BNodeBufArray mNodeBuf;
   BNode* mpNodes;
   BNode* mpEndOfNodes;
   uint mNumNodes;
   uint mNumValid;
   uint mNumValidGrowThresh;
   Traits mTraits;

   // Be careful calling this method because the container's memory could be moved by the dynamicarray's resize()!
   void setNodeBufSize(uint numNodes)
   {
      BDEBUG_ASSERT(Math::IsPow2(numNodes));

      mNodeBuf.resize(numNodes * sizeof(BNode));

      mNumNodes = numNodes;
      mNumValid = 0;
      // Set auto grow threshold to 65% full which is around 4.58 probes per unsuccessful test on average
      mNumValidGrowThresh = (mNumNodes * 96 + 64) / 128;
      
      mpNodes = reinterpret_cast<BNode*>(&mNodeBuf[0]);
      mpEndOfNodes = &mpNodes[mNumNodes];
   }
   
   void grow(int newNumNodes = -1)
   {
      BNodeBufArray tempNodeBuf;
      tempNodeBuf.swap(mNodeBuf);

      const uint oldNumNodes = mNumNodes;
      const BNode* pOldNodes = mpNodes;
      uint numLeft = mNumValid;

      setNodeBufSize((newNumNodes < 0) ? (oldNumNodes * 2) : newNumNodes);

      if (numLeft)
      {
         // Migrate old objects into new memory block.
         for (uint i = 0; i < oldNumNodes; i++)
         {
            if (mTraits.isValidValue(pOldNodes[i].mValue))
            {
               std::pair<value_type*, bool> insertRes = insert(pOldNodes[i].mValue, false);
               BDEBUG_ASSERT(insertRes.second);

               Utils::DestructInPlace(&pOldNodes[i]);
               // No need to clear the node at pOldNodes[i] as this block will go out of scope in this function.

               numLeft--;
               if (0 == numLeft)
                  break;
            }
         }
      }         
   }

   std::pair<value_type*, bool> growAndInsert(const value_type& value, int newNumNodes = -1)
   {
      grow(newNumNodes);
            
      std::pair<value_type*, bool> insertRes = insert(value, false);            
      BDEBUG_ASSERT(insertRes.second);
            
      return insertRes;
   }
   
   // copy construction and equals operator disabled for now
   BLinearProbeHashTable(BLinearProbeHashTable& other);
   BLinearProbeHashTable& operator= (BLinearProbeHashTable& rhs);
};
