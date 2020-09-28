// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Combination_h
#define Combination_h

#if 0
{
    Combination<Vertex> comb;
    ForCombination(comb,Vertex,v,val) { do(v,val); } EndFor;
}
#endif

#include "Map.h"
#include "Pool.h"

class BCombination {
 public:
    BCombination();
    ~BCombination();
    void clear();
    float operator[](Univ e) const;
    float& operator[](Univ e);  // use const version for lookup!
    int num() const;            // should squeeze first
    int empty() const;          // should squeeze first
    float sum() const;          // slow
    void squeeze() const;       // virtual const
    POOL_ALLOCATION(BCombination);
 private:
friend class BCombinationIter;
    Map<Univ,float> _m;
    DISABLE_COPY(BCombination);
};

INITIALIZE_POOL(BCombination);

class BCombinationIter {
 public:
    BCombinationIter(const BCombination& c);
    ~BCombinationIter() { }
    operator void*() const;
    void next();
    Univ elem() const;
    float value() const;
 private:
    MapIter<Univ,float> _mi;
    DISABLE_COPY(BCombinationIter);
};

//----------------------------------------------------------------------------

inline float BCombination::operator[](Univ e) const {
    // if not present float(0) returned, hack
    return _m.retrieve(e);
}
inline float& BCombination::operator[](Univ e) {
    Univ* p=_m.specialretrieveenteraddr(e);
    // if not there, adds 0, which happens to equal float(0), hack
    return *(float *)p;
}
inline int BCombination::num() const { return _m.num(); }
inline int BCombination::empty() const { return _m.empty(); }


inline BCombinationIter::BCombinationIter(const BCombination& c)
: _mi(c._m) { }
inline BCombinationIter::operator void*() const { return _mi; }
inline void BCombinationIter::next() { _mi.next(); }
inline Univ BCombinationIter::elem() const { return _mi.key(); }
inline float BCombinationIter::value() const { return _mi.value(); }

//----------------------------------------------------------------------------

template<class T> class CombinationIter;

template<class T>
class Combination : public BCombination {
 public:
    inline Combination() { }
    inline ~Combination() { }
    inline float operator[](T e) const {
        return BCombination::operator[](Conv<T>::e(e));
    }
    inline float& operator[](T e) {
        return BCombination::operator[](Conv<T>::e(e));
    }
//      typedef CombinationIter<T> Iter;
};

template<class T>
class CombinationIter : public BCombinationIter {
 public:
    inline CombinationIter(const Combination<T>& c)
        : BCombinationIter(c) { }
    inline ~CombinationIter() { }
    inline T elem() const { return Conv<T>::d(BCombinationIter::elem()); }
};

#define ForCombination(S,T,V1,V2) \
{ for (CombinationIter< T > zz(S);zz;zz.next()) \
  { T V1=zz.elem(); float V2=zz.value();
#define DummyEndFor }}

#endif
