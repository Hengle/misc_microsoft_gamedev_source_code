//==============================================================================
// AvlMap.hpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef __AvlMap__
#define __AvlMap__

#include <Etl\AvlTree.hpp>
#include <Etl\Pair.hpp>

#define AVL_MAP_BASE_CLASS  AvlTree< Pair<Key, T>, AvlMapCompare<Key, T, Cmp> >
#define AVL_MAP_NODE        AvlNode< Pair<Key, T>, AvlMapCompare<Key, T, Cmp> >
#define AVL_MAP_ITERATOR    ForwardIterator<                                      \
                            Pair<Key,T>,                                          \
                            AvlIterator<Pair<Key,T>, AvlMapCompare<Key,T, Cmp> >, \
                            AvlTree<Pair<Key,T> ,                                 \
                            AvlMapCompare<Key,T, Cmp> > >

namespace Etl
{

template <class Key, class T, class Cmp>
struct AvlMapCompare
{
   inline long operator() (const Pair<Key, T>& b1, const Pair<Key, T>& b2) const
   {
      Cmp cmp;
      return cmp(b1.first, b2.first);
   }
};

template <class Key,
          class T,
          class Cmp = CompareKeyDefault<Key>,
          class NodeObj = AVL_MAP_NODE >

class AvlMap : public AVL_MAP_BASE_CLASS
{
public:
   AvlMap();

   AVL_MAP_ITERATOR insert(const Key&, const T&);
   bool erase(Iterator);
   bool erase(const Key&);
   AVL_MAP_ITERATOR search(const Key&) const;
   T& operator [] (const Key&);

private:

};


template <class Key, class T, class Cmp, class NodeObj>
AvlMap<Key, T, Cmp, NodeObj>::AvlMap() : AVL_MAP_BASE_CLASS()
{
}

template <class Key, class T, class Cmp, class NodeObj>
bool AvlMap<Key, T, Cmp, NodeObj>::erase(Iterator it)
{
   return AVL_MAP_BASE_CLASS::erase(*it);
}

template <class Key, class T, class Cmp, class NodeObj>
bool AvlMap<Key, T, Cmp, NodeObj>::erase(const Key& k)
{
   return AVL_MAP_BASE_CLASS::erase(k);
}

template <class Key, class T, class Cmp, class NodeObj>
T& AvlMap<Key, T, Cmp, NodeObj>::operator [] (const Key& k)
{
   long lChange = 0;
   AVL_MAP_NODE* pNewNode = 0;

   AVL_MAP_BASE_CLASS::insert(Pair<Key, T>(k), m_pRoot, lChange, pNewNode);

   return pNewNode->m_key.second;
}

template <class Key, class T, class Cmp, class NodeObj>
inline AVL_MAP_ITERATOR AvlMap<Key, T, Cmp, NodeObj>::search(const Key& k) const
{
   return AVL_MAP_BASE_CLASS::search(Pair<Key, T>(k));
}

template <class Key, class T, class Cmp, class NodeObj>
inline AVL_MAP_ITERATOR AvlMap<Key, T, Cmp, NodeObj>::insert(const Key& k, const T& t)
{
   return AVL_MAP_BASE_CLASS::insert(Pair<Key, T>(k, t));
}

}
#endif

