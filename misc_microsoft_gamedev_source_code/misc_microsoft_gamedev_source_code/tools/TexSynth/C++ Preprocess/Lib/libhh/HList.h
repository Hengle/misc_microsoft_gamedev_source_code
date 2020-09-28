// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef HList_h
#define HList_h

#include "Pool.h"
#include "Map.h"

// Implements an ordered set
class BHList {
 public:
    BHList();
    ~BHList();
    void clear();
    void unshift(Univ e);       // insert at beg
    Univ shift();               // remove from beg (else die)
    void push(Univ e);          // insert at end
    Univ pop();                 // remove from end (else die)
    Univ first() const;         // peek (else die)
    Univ last() const;          // peek (else die)
    void insert_before(Univ eo, Univ en); // else die
    void insert_after(Univ eo, Univ en);  // else die
    int remove(Univ e);         // ret: was_found
    Univ before(Univ e) const;  // else 0
    Univ after(Univ e) const;   // else 0
    int empty() const;
    int num() const;
    int contains(Univ e) const;
    int add(Univ e);            // add to end if not in list (ret: is_new)
 public:                        // should be protected
friend class BHListIter;
    struct Node {
        POOL_ALLOCATION(Node);
        Univ data;
        Node* prev;
        Node* next;
    };
 private:
    Node* _first;
    Node* _last;
    Map<Univ,Node*> _map;
    DISABLE_COPY(BHList);
};

INITIALIZE_POOL_NESTED(BHList::Node,BHListNode);

class BHListIter {
 public:
    BHListIter(const BHList& s);
    ~BHListIter();
    operator void*() const;
    void next();
    Univ operator()() const;
 private:
    const BHList::Node* _n;
    // operator=() is safe
};

//----------------------------------------------------------------------------

inline BHListIter::BHListIter(const BHList& s) : _n(s._first) { }
inline BHListIter::~BHListIter() { }
inline void BHListIter::next() { _n=_n->next; }
inline BHListIter::operator void*() const { return _n?(void*)1:0; }
inline Univ BHListIter::operator()() const { return _n->data; }

//----------------------------------------------------------------------------

template<class T> class HListIter;

template<class T>
class HList : public BHList {
 public:
    inline HList() { }
    inline ~HList() { }
    inline void unshift(T e) { BHList::unshift(Conv<T>::e(e)); }
    inline T shift() { return Conv<T>::d(BHList::shift()); }
    inline void push(T e) { BHList::push(Conv<T>::e(e)); }
    inline T pop() { return Conv<T>::d(BHList::pop()); }
    inline T first() const { return Conv<T>::d(BHList::first()); }
    inline T last() const { return Conv<T>::d(BHList::last()); }
    inline void insert_before(T eo, T en)
    { BHList::insert_before(Conv<T>::e(eo), Conv<T>::e(en)); }
    inline void insert_after(T eo, T en)
    { BHList::insert_after(Conv<T>::e(eo), Conv<T>::e(en)); }
    inline int remove(T e) { return BHList::remove(Conv<T>::e(e)); }
    inline T after(T e) const
    { return Conv<T>::d(BHList::after(Conv<T>::e(e))); }
    inline T before(T e) const
    { return Conv<T>::d(BHList::before(Conv<T>::e(e))); }
    inline int contains(T e) const { return BHList::contains(Conv<T>::e(e)); }
    inline int add(T e) { return BHList::add(Conv<T>::e(e)); }
//      typedef HListIter<T> Iter;
};

template<class T>
class HListIter : public BHListIter {
 public:
    inline HListIter(const HList<T>& q) : BHListIter(q) { }
    inline ~HListIter() { }
    inline T operator()() const
    { return Conv<T>::d(BHListIter::operator()()); }
};

#define ForHList(L,T,V) { for (HListIter< T > zz(L);zz;zz.next()) { T V=zz();
#define DummyEndFor }}

#endif
