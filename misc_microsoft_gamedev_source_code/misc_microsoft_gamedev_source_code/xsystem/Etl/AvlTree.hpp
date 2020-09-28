//==============================================================================
// AvlTree.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================
#ifndef __AvlTree__
#define __AvlTree__

#include <Etl\Iterator.hpp>
#include <Etl\Support.hpp>
#include <Etl\Stack.hpp>
#include <Etl\SmartPtr.hpp>

#define AVL_ITERATOR

namespace Etl
{
template <class Key, class Cmp> class AvlNode;
template <class Key, class Cmp> class AvlIterator;

template <class Key, class Cmp = CompareKeyDefault<Key> >
class AvlTree
{
public:
   typedef ForwardIterator<Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> > Iterator;

   // constructor
   AvlTree() : m_pRoot(NULL), m_lCount(0) {};
   // destructor
   ~AvlTree() { if (m_pRoot) delete m_pRoot; }

   // insert a key into the avltree
   void insert(const Key& k);
   // erase a key from the avltree
   bool erase(const Key& key);
   bool erase(Iterator);
   bool isEmpty() const  { return (m_pRoot == NULL) ? true : false; }

   Iterator search(const Key& key) const;

   inline long count(void) const { return m_lCount; }
   //
   // interfaced is for external iterator
   //
   // Usage:
   // Etl::AvlTree<long> tree;
   //
   // for(AvlTree<long>::Iterator it = tree.begin(); it != tree.end(); it++)
   // {
   // }
   //
   Iterator begin(void);
   Iterator end(void);
   //
   // Alternative iteration interface
   //
   // Usage:
   // Etl::AvlTree<long> tree;
   // Etl::AvlIterator<long> it(tree);
   // or
   // using namespace Etl;
   //
   // AvlTree<long> tree;   // insert some stuff
   // AvlIterator it(tree);
   //
   // for (long* plItem = tree.getFirst(it); plItem != NULL; plItem = tree.getNext(it))
   // {
   // }
   //
   //virtual inline T* getFirst(Etl::AvlIterator<Key, Cmp>&);
   //virtual inline T* getNext(Etl::AvlIterator<Key, Cmp>&);

protected:
   AvlTree(const AvlTree<Key, Cmp>&);
   AvlTree& operator = (const AvlTree<Key, Cmp> &);

   long rotateOnce(AvlNode<Key,Cmp>*& pRoot, long lDir);
   long rotateTwice(AvlNode<Key,Cmp>*& pRoot, long lDir);

   AvlNode<Key,Cmp>* insert(const Key&, AvlNode<Key,Cmp>*&, long&, AvlNode<Key,Cmp>*&);

   long rebalance(AvlNode<Key,Cmp>*&);
   AvlNode<Key,Cmp>* erase(const Key&, AvlNode<Key,Cmp>*&, long&, long);
   bool search(const Key& k, AvlNode<Key,Cmp>* pRoot, long cmp = 0);

   inline bool checkRightBalance(long bal);
   inline bool checkLeftBalance(long bal);
   // Return the opposite direction of the given index
   inline long opposite(long dir);

   long m_lCount;
   AvlNode<Key,Cmp>* m_pRoot;   // The root of the tree
   AvlNode<Key,Cmp>* m_pFirst;

private:
   friend class ForwardIterator< Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> >;
   static void forward(AvlIterator<Key,Cmp>& it);
   static Key& itget(const AvlIterator<Key,Cmp>& it) { return it.m_pCurrent->m_key; }
};

//template <class Key, class Cmp>
//inline T* AvlTree<Key,Cmp>::getFirst(Etl::AvlIterator<T,Cmp>& it)
//{
//   return (it.getCurrent()) ? &it.getCurrent()->m_key : 0;
//}

//template <class Key, class Cmp>
//inline T* AvlTree<Key,Cmp>::getNext(Etl::AvlIterator<T,Cmp>& it)
//{
//   it.getNext();
//   return (it.getCurrent()) ? &it.getCurrent()->m_key : 0;
//}

template <class Key, class Cmp>
inline void AvlTree<Key,Cmp>::forward(AvlIterator<Key,Cmp>& it)
{
   it.getNext();
}

template <class Key, class Cmp>
inline ForwardIterator< Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> >
   AvlTree<Key,Cmp>::begin(void)
{
   return ForwardIterator< Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> >(AvlIterator<Key,Cmp>(m_pRoot));
}

template <class Key, class Cmp>
inline ForwardIterator< Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> >
   AvlTree<Key,Cmp>::end(void)
{
   return ForwardIterator< Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> > (AvlIterator<Key,Cmp>(NULL));
}

template <class Key, class Cmp>
inline bool AvlTree<Key, Cmp>::checkRightBalance(long bal)
{
   return (bal > AvlNode<Key,Cmp>::RightHeavy) ? true : false;
}

template <class Key, class Cmp>
inline bool AvlTree<Key, Cmp>::checkLeftBalance(long bal)
{
   return (bal < AvlNode<Key,Cmp>::LeftHeavy)  ? true : false;
}

template <class Key, class Cmp>
inline long AvlTree<Key,Cmp>::opposite(long dir)
{
   return (long)(1 - (long)(dir));
}

template <class Key, class Cmp>
inline void AvlTree<Key, Cmp>::insert(const Key& k)
{
   long  sChange = 0; // ignore needed for insert() call
   AvlNode<Key,Cmp> *pNewNode = 0;
   m_lCount++;
   insert(k, m_pRoot, sChange, pNewNode);
}

template <class Key, class Cmp>
inline bool AvlTree<Key, Cmp>::erase(Iterator it)
{
   long sChange = 0;  // ignore needed for erase() call
   AvlNode<Key,Cmp>* pNodeToDelete = erase(*it, m_pRoot, sChange, 0);

   if (pNodeToDelete)
   {
      // zero out so we don't get recursive deletes
      pNodeToDelete->m_pSubtree[0] = 0;
      pNodeToDelete->m_pSubtree[1] = 0;
      delete pNodeToDelete;
      return true;
   }
   return false;
}

template <class Key, class Cmp>
inline bool AvlTree<Key, Cmp>::erase(const Key& key)
{
   long sChange = 0;  // ignore needed for erase() call
   AvlNode<Key,Cmp>* pNodeToDelete = erase(key, m_pRoot, sChange, 0);

   if (pNodeToDelete)
   {
      // zero out so we don't get recursive deletes
      pNodeToDelete->m_pSubtree[0] = 0;
      pNodeToDelete->m_pSubtree[1] = 0;
      delete pNodeToDelete;
      return true;
   }
   return false;
}

template <class Key, class Cmp>
AvlNode<Key,Cmp>* AvlTree<Key,Cmp>::insert(
   const Key& k,
   AvlNode<Key,Cmp>*& pRoot,
   long& sChange,
   AvlNode<Key,Cmp>*& pNewNode
   )
{
   // See if the tree is empty
   if (!pRoot)
   {
      // insert new node here
      pNewNode = pRoot = new AvlNode<Key,Cmp>(k);
      sChange = 1;
      return  NULL;
   }
   // Initialize
   long increase = 0;
   AvlNode<Key,Cmp>* pFound = 0;
   // compare items and determine which direction to search
   long  result = pRoot->compare(k);
   long  lDir = (result == -1) ? (long)0 /*left*/ : (long)1 /* right */;

   if (result != 0)
   {
      // insert into "lDir" subtree
      pFound = insert(k, pRoot->m_pSubtree[lDir], sChange, pNewNode);
      if (pFound)
         return  pFound;   // already here - dont insert
      increase = result * sChange;  // set balance factor increment
   }
   else
   {   // key already in tree at this node
      increase = 0;
      return  pRoot;
   }
   //
   // update balance factor
   //
   pRoot->m_sBalanceFactor += increase;
   //
   // re-balance if needed -- height of current tree increases only if its
   // subtree height increases and the current tree needs no rotation.
   //
   sChange =  (increase && pRoot->m_sBalanceFactor) ? (1 - rebalance(pRoot)) : 0;

   return  NULL;
}

template <class Key, class Cmp>
long AvlTree<Key,Cmp>::rebalance(AvlNode<Key,Cmp>*& pRoot)
{
   long lHeightChange = 0;

   if (checkLeftBalance(pRoot->m_sBalanceFactor))         // Need a right rotation?
   {
      if (pRoot->m_pSubtree[AvlNode<Key,Cmp>::Left]->m_sBalanceFactor == AvlNode<Key,Cmp>::RightHeavy)
         lHeightChange = rotateTwice(pRoot, AvlNode<Key,Cmp>::Right);       // RL rotation needed
      else
         lHeightChange = rotateOnce(pRoot, AvlNode<Key,Cmp>::Right);        // RR rotation needed
   }
   else if (checkRightBalance(pRoot->m_sBalanceFactor))   // Need a left rotation?
   {
      if (pRoot->m_pSubtree[AvlNode<Key,Cmp>::Right]->m_sBalanceFactor == AvlNode<Key,Cmp>::LeftHeavy)
         lHeightChange = rotateTwice(pRoot, AvlNode<Key,Cmp>::Left);        // LR rotation needed
      else
         lHeightChange = rotateOnce(pRoot, AvlNode<Key,Cmp>::Left);         // LL rotation needed
   }

   return  lHeightChange;
}

template <class Key, class Cmp>
long AvlTree<Key,Cmp>::rotateOnce(AvlNode<Key,Cmp>*& pRoot, long lDir)
{
   long lHeightChange;
   long  sOtherDir = opposite(lDir);
   AvlNode<Key,Cmp>* pParent = 0;
   AvlNode<Key,Cmp>* pOldRoot = pRoot;

   // See if otherDir subtree is balanced. If it is, then this
   // rotation will *not* change the overall tree height.
   // Otherwise, this rotation will shorten the tree height.
   lHeightChange = (pRoot->m_pSubtree[sOtherDir]->m_sBalanceFactor == AvlNode<Key,Cmp>::Balanced) ? 0 : 1;

   pParent = pRoot;
   // assign new root
   pRoot = pOldRoot->m_pSubtree[sOtherDir];
   // new-root exchanges it's "lDir" m_pSubtree for it's parent
   pOldRoot->m_pSubtree[sOtherDir] = pRoot->m_pSubtree[lDir];
   pRoot->m_pSubtree[lDir] = pOldRoot;

   // update balances
   pOldRoot->m_sBalanceFactor = -((lDir == AvlNode<Key,Cmp>::Left) ? --(pRoot->m_sBalanceFactor) : ++(pRoot->m_sBalanceFactor));

   return  lHeightChange;
}

template <class Key, class Cmp>
long AvlTree<Key,Cmp>::rotateTwice(AvlNode<Key,Cmp>*& pRoot, long lDir)
{
   long  sOtherDir = opposite(lDir);
   AvlNode<Key,Cmp> * pOldRoot = pRoot;
   AvlNode<Key,Cmp> * pOldSubtree = pRoot->m_pSubtree[sOtherDir];

   // assign new root
   pRoot = pOldRoot->m_pSubtree[sOtherDir]->m_pSubtree[lDir];

   // new-root exchanges it's "lDir" m_pSubtree for it's grandparent
   pOldRoot->m_pSubtree[sOtherDir] = pRoot->m_pSubtree[lDir];

   pRoot->m_pSubtree[lDir] = pOldRoot;
   // new-root exchanges it's "other-dir" m_pSubtree for it's parent
   pOldSubtree->m_pSubtree[lDir] = pRoot->m_pSubtree[sOtherDir];
   pRoot->m_pSubtree[sOtherDir] = pOldSubtree;
   // update balances
   pRoot->m_pSubtree[AvlNode<Key,Cmp>::Left]->m_sBalanceFactor  = -Max<long>(pRoot->m_sBalanceFactor, 0);
   pRoot->m_pSubtree[AvlNode<Key,Cmp>::Right]->m_sBalanceFactor = -Min<long>(pRoot->m_sBalanceFactor, 0);
   pRoot->m_sBalanceFactor = 0;

   // A double rotation always shortens the overall height of the tree
   return 1;
}

template <class Key, class Cmp>
AvlNode<Key,Cmp>* AvlTree<Key,Cmp>::erase(
   const Key& key,
   AvlNode<Key,Cmp>*& pRoot,
   long& sChange,
   long cmp)
{
   // See if the tree is empty
   if (!pRoot)
   {
      sChange = false;
      return  NULL;
   }

   // Initialize
   AvlNode<Key,Cmp>* pFound = 0;
   long decrease = 0;

   // compare items and determine which direction to search
   long  result = pRoot->compare(key, cmp);
   long  lDir = (result == -1) ? AvlNode<Key,Cmp>::Left : AvlNode<Key,Cmp>::Right;

   if (result != 0)
   {
      pFound = erase(key, pRoot->m_pSubtree[lDir], sChange, cmp);
      if (!pFound)
         return NULL;   // not found - can't delete

      decrease = result * sChange;    // set balance factor decrement
   }
   else
   {
      pFound = pRoot;
      //
      // We found a key so:
      //
      // We no that result == 0, and pRoot points to the node that we
      // need to delete. So there are three cases:
      //
      // 1. The node doesnt have siblings, so remove it an return.
      // 2. The node is a sibling, and has 1 sibling. Make pRoot
      //    point to the child sibiling.
      // 3. The node has two siblings. We need to swap items with the
      //    successor of pRoot and delete the sucessor.
      //
      if ((pRoot->m_pSubtree[AvlNode<Key,Cmp>::Left] == NULL) &&
          (pRoot->m_pSubtree[AvlNode<Key,Cmp>::Right] == NULL))
      {
         m_lCount--;
         // We have a leaf -- remove it
         //delete pRoot; -- (bgoodman) removed, needs to be deleted above, keep uncommented

         pRoot = NULL;
         sChange = true;    // height changed from 1 to 0
         return  pFound;
      }
      else if ((pRoot->m_pSubtree[AvlNode<Key,Cmp>::Left] == NULL) ||
               (pRoot->m_pSubtree[AvlNode<Key,Cmp>::Right] == NULL))
      {
         // We have one child -- only child becomes new root
         AvlNode<Key,Cmp>* pNodeToDelete = pRoot;

         pRoot = pRoot->m_pSubtree[(pRoot->m_pSubtree[AvlNode<Key,Cmp>::Right]) ?
            AvlNode<Key,Cmp>::Right :
            AvlNode<Key,Cmp>::Left];

         sChange = true;    // We just shortened the subtree
         // Null-out the subtree pointers so we dont recursively delete
         pNodeToDelete->m_pSubtree[AvlNode<Key,Cmp>::Left] = pNodeToDelete->m_pSubtree[AvlNode<Key,Cmp>::Right] = NULL;
         // delete pNodeToDelete -- (bgoodman) removed, needs to be deleted above, keep uncommented
         m_lCount--;
         return  pFound;
      }
      else
      {
         // We have two children, find the successor and replace our current
         // item with that of the successor
         pRoot = erase(key, pRoot->m_pSubtree[AvlNode<Key,Cmp>::Right], decrease, -1);
         pRoot->m_pSubtree[AvlNode<Key,Cmp>::Right] = pFound->m_pSubtree[AvlNode<Key,Cmp>::Right];
         pRoot->m_pSubtree[AvlNode<Key,Cmp>::Left] = pFound->m_pSubtree[AvlNode<Key,Cmp>::Left];
      }
   }

   pRoot->m_sBalanceFactor -= decrease;       // update balance factor
   //
   // Rebalance if necessary -- the height of current tree changes if one
   // of two things happens: (1) a rotation was performed which changed
   // the height of the subtree (2) the subtree height decreased and now
   // matches the height of its other subtree (so the current tree now
   // has a zero balance when it previously did not).
   //
   if (decrease)
   {
      if (pRoot->m_sBalanceFactor)
         sChange = rebalance(pRoot);  // rebalance and see if height changed
      else
         sChange = true;   // balanced because subtree decreased
   }
   else
      sChange = false;

   return  pFound;
}

template <class Key, class Cmp>
inline ForwardIterator<Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> >
   AvlTree<Key,Cmp>::search(const Key& key) const
{
   AvlIterator<Key,Cmp> it;
   it.search(m_pRoot, key);
   return ForwardIterator<Key, AvlIterator<Key,Cmp>, AvlTree<Key,Cmp> >(it);
}

#if 0
template <class Key, class Cmp>
inline Key* AvlTree<Key,Cmp>::search(const Key& k) const
{
   AvlNode<Key,Cmp>* m_pCurrent = pRoot;
   long lResult = 0;

   while (m_pCurrent != NULL && (lResult = m_pCurrent->compare(key)) != 0)
   {
      m_pCurrent = (lResult < 0) ?
         m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Left] :
         m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Right];
   }
   return &m_pCurrent->m_key;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// class AvlNode
///////////////////////////////////////////////////////////////////////////////
template <class Key, class Cmp>
class AvlNode
{
public:
   enum  { LeftHeavy = -1, Balanced = 0, RightHeavy = 1 };
   enum  { Left = 0, Right = 1 };

   AvlNode(const Key&);
   virtual ~AvlNode(void);

   inline long balanceFactor(void) const   { return  m_slBalanceFactor; }
   inline AvlNode* subtree(long dir) const { return  m_pSubtree[dir];   }

   long compare(Key key, long cmp = 0) const;
   Key getKey(void) const { return m_key; }

   friend class AvlTree<Key,Cmp>;
   friend class AvlIterator<Key,Cmp>;
   AvlNode(const AvlNode<Key,Cmp> &);
   AvlNode & operator = (const AvlNode<Key,Cmp> &);

   AvlNode<Key,Cmp>*   m_pSubtree[2];    // Pointers to subtrees [Left = 0, Right = 1]
   long                m_sBalanceFactor; // Balance factor
   Key                 m_key;            // data member
};

template <class Key, class Cmp>
AvlNode<Key,Cmp>::AvlNode(const Key& k) : m_key(k), m_sBalanceFactor(0)
{
   m_pSubtree[Left]  = NULL;
   m_pSubtree[Right] = NULL;
}

template <class Key, class Cmp>
AvlNode<Key,Cmp>::~AvlNode(void)
{
   if (m_pSubtree[Left])  delete m_pSubtree[Left];
   if (m_pSubtree[Right]) delete m_pSubtree[Right];
}

template <class Key, class Cmp>
inline long AvlNode<Key,Cmp>::compare(Key key, long cmpType) const
{
   Cmp cmp;

   if (cmpType == 0)
      return cmp(m_key, key);
   else if (cmpType == -1)
      return  (m_pSubtree[Left] == NULL) ? 0 : -1;

   return  (m_pSubtree[Right] == NULL) ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////
// AvlIterator
///////////////////////////////////////////////////////////////////////////////
template <class Key, class Cmp>
class AvlIterator
{
public:
   // this constructor sets the iterator to the first entry
   // in the tree
   AvlIterator();
   AvlIterator(AvlNode<Key,Cmp>* pTreeRoot);
   AvlIterator(const AvlIterator<Key,Cmp>&);
   ~AvlIterator() { /*m_pStack->unRef();*/ }

   inline bool operator == (const AvlIterator&) const;
   inline bool operator != (const AvlIterator&) const;
   inline Key* operator -> (void) const { return &m_pCurrent->m_key; }
   inline operator Key* () const  { return &m_pCurrent->m_key; }
   inline operator * () const { return m_pCurrent->m_key; }

   inline void search(AvlNode<Key,Cmp>* pRoot, const Key& k);
   inline void getNext(void);
   inline AvlNode<Key,Cmp>* getCurrent(void) { return m_pCurrent; }

protected:
   friend class AvlTree<Key,Cmp>;
   void getFirst(AvlNode<Key,Cmp>* pTreeRoot);

   SmartPtr< Stack< AvlNode<Key,Cmp>* > > m_pStack;
   AvlNode<Key,Cmp>* m_pCurrent;

};

template <class Key, class Cmp>
inline AvlIterator<Key,Cmp>::AvlIterator() : m_pCurrent(0)
{
   m_pStack = new Stack< AvlNode<Key,Cmp>* >;
   //m_pStack->ref();
}

template <class Key, class Cmp>
inline AvlIterator<Key,Cmp>::AvlIterator(const AvlIterator<Key,Cmp>& it)
{
   m_pStack = it.m_pStack;
   //m_pStack->ref();
   m_pCurrent = it.m_pCurrent;
}

template <class Key, class Cmp>
inline bool AvlIterator<Key,Cmp>::operator == (const AvlIterator<Key,Cmp>& it) const
{
   if (m_pCurrent == it.m_pCurrent)
         return true;
   return false;

}

template <class Key, class Cmp>
inline bool AvlIterator<Key,Cmp>::operator != (const AvlIterator<Key,Cmp>& it) const
{
   if (m_pCurrent != it.m_pCurrent)
         return true;
   return false;
}

template <class Key, class Cmp>
inline AvlIterator<Key,Cmp>::AvlIterator(AvlNode<Key,Cmp>* pTreeRoot)
{
   m_pStack = new Stack< AvlNode<Key,Cmp>* >;
   //m_pStack->ref();
   getFirst(pTreeRoot);
}

template <class Key, class Cmp>
inline void AvlIterator<Key, Cmp>::search( AvlNode<Key,Cmp>* pRoot, const Key& key )
{
   m_pCurrent = pRoot;
   long lResult = 0;

   while (m_pCurrent != NULL && (lResult = m_pCurrent->compare(key)) != 0)
   {
      m_pStack->push(m_pCurrent);
      m_pCurrent = (lResult < 0) ?
         m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Left] :
         m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Right];
   }
}


template <class Key, class Cmp>
inline void AvlIterator<Key, Cmp>::getFirst(AvlNode<Key,Cmp>* pTreeRoot)
{
   AvlNode<Key,Cmp>* pFound = pTreeRoot;

   while (pFound && pFound->m_pSubtree[AvlNode<Key,Cmp>::Right])
   {
      m_pStack->push(pFound);
      pFound = pFound->m_pSubtree[AvlNode<Key,Cmp>::Right];
   }

   m_pCurrent = pFound;
}

template <class Key, class Cmp>
inline void AvlIterator<Key, Cmp>::getNext(void)
{
   AvlNode<Key,Cmp>* pNext = NULL;

   if (m_pStack->count() > 0)
   {
      if (m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Left])
      {
         getFirst(m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Left]);
         return;
      }
      else
      {
         pNext = m_pStack->pop();
         while (pNext->compare(m_pCurrent->getKey()) < 0)
         {
            pNext = m_pStack->pop();
         }
      }
   }
   else if (m_pCurrent && m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Right])
   {
      if (m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Left])
      {
         getFirst(m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Left]);
         return;
      }
      else
         m_pCurrent = m_pCurrent->m_pSubtree[AvlNode<Key,Cmp>::Right];
   }
   m_pCurrent = pNext;
}

}

#endif
