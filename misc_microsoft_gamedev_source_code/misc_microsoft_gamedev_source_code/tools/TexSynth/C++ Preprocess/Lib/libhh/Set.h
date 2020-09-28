// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Set_h
#define Set_h

#include "Map.h"
#include "Pool.h"

#if 0
{
    Set<Edge> sete;
    ForSet(set,Edge,e) { consider(e); } EndFor;
}
#endif

class BSet {
 public:
    BSet() { }
    ~BSet() { }
    void clear();
    // e may be zero
    void enter(Univ e); // e must be new
    int add(Univ e);    // ret is_new
    int remove(Univ e); // ret was_present
    int contains(Univ e) const; // ret does_contain
    int num() const;
    int empty() const;
    Univ getone() const;        // die if empty
    Univ removeone();           // die if empty
    void OK() const;
    POOL_ALLOCATION(BSet);
 private:
friend class BSetIter;
    BMap _m;
    DISABLE_COPY(BSet);
};

INITIALIZE_POOL(BSet);

class Random;

class BSetIter {
 public:
    BSetIter(const BSet& s);
    BSetIter(const BSet& s, Random& r);
    ~BSetIter() { }
    Univ operator()() const;
    operator void*() const;
    void next();
 private:
    BMapIter _mi;
    DISABLE_COPY(BSetIter);
};

//----------------------------------------------------------------------------

inline BSetIter::BSetIter(const BSet& s) : _mi(s._m) { }
inline BSetIter::BSetIter(const BSet& s, Random& r) : _mi(s._m,r) { }
inline Univ BSetIter::operator()() const { return _mi.key(); }
inline BSetIter::operator void*() const { return _mi; }
inline void BSetIter::next() { _mi.next(); }

inline void BSet::clear() { _m.clear(); }
inline void BSet::enter(Univ e) { _m.enter(e,Conv<int>::e(1)); }
inline int BSet::contains(Univ e) const { return _m.contains(e); }
inline int BSet::remove(Univ e) { return _m.remove(e)?1:0; }

inline int BSet::add(Univ e) { return _m.specialadd(e,Conv<int>::e(1)); }

inline int BSet::num() const { return _m.num(); }
inline int BSet::empty() const { return _m.empty(); }
inline Univ BSet::getone() const { BSetIter si(*this); return si(); }
inline Univ BSet::removeone() { Univ e=getone(); remove(e); return e; }
inline void BSet::OK() const { _m.OK(); }

//----------------------------------------------------------------------------

template<class T> class SetIter;

template<class T>
class Set : public BSet {
 public:
    inline Set() { }
    inline ~Set() { }
    inline void enter(T e) { BSet::enter(Conv<T>::e(e)); }
    inline int add(T e) { return BSet::add(Conv<T>::e(e)); }
    inline int remove(T e) { return BSet::remove(Conv<T>::e(e)); }
    inline int contains(T e) const { return BSet::contains(Conv<T>::e(e)); }
    inline T getone() const { return Conv<T>::d(BSet::getone()); }
    inline T removeone() { return Conv<T>::d(BSet::removeone()); }
//      typedef SetIter<T> Iter;
};

template<class T>
class SetIter : public BSetIter {
 public:
    inline SetIter(const Set<T>& s) : BSetIter(s) { }
    inline SetIter(const Set<T>& s, Random& r) : BSetIter(s,r) { }
    inline ~SetIter() { }
    inline T operator()() const { return Conv<T>::d(BSetIter::operator()()); }
};

#define ForSet(S,T,V) { for (SetIter< T > zz(S);zz;zz.next()) { T V=zz();
#define DummyEndFor }}

#endif
