// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef HashStruct_h
#define HashStruct_h

#if 0
{
    HashStruct<segment> hs(hashf,cmpf);
    ForHashStruct(hs,segment,s) { do(s); } EndFor;
}
#endif

#include "BHashStruct.h"

// Maintains a set of structures.
// The structures are hashed according to a user-provided function hashf.
//  Note: hashf can return any value including zero.
// Also, equality of two structures is determined by user function cmpf.
//  Note: cmpf should return: does_not_match (as does strcmp)!
//  Note for hacker: when cmpf is invoked, its first arg is st (below)
//   Thus, st (structure template) can be fake and cmpf can be asymmetric.
// Note: BHashStruct should be empty prior to destruction.

//----------------------------------------------------------------------------

template<class T> class HashStructIter;

template<class T>
class HashStruct : public BHashStruct {
 public:
    typedef Univ (*HASHF)(const T*);
    typedef int (*CMPF)(const T*, const T*);
    inline HashStruct(HASHF phashf, CMPF pcmpf) :
    BHashStruct((BHashStruct::HASHF)phashf, (BHashStruct::CMPF)pcmpf) { }
    inline ~HashStruct() { }
    inline void enter(T* e) { BHashStruct::enter(Univ(e)); }
    inline T* retrieve(const T* e) const
    { return (T*)(void*)(BHashStruct::retrieve(Conv<T*>::e((T*)e))); }
    inline int add(T* e) { return BHashStruct::add(Univ(e)); }
    inline int remove(const T* e)
    { return BHashStruct::remove(Conv<T*>::e((T*)e)); }
    inline T* getone() const { return (T*)(void*)(BHashStruct::getone()); }
    inline T* removeone() { return (T*)(void*)(BHashStruct::removeone()); }
//      typedef HashStructIter<T> Iter;
};

template<class T>
class HashStructIter : public BHashStructIter {
 public:
    inline HashStructIter(const HashStruct<T>& s) : BHashStructIter(s) { }
    inline HashStructIter(const HashStruct<T>& s, Random& r)
        : BHashStructIter(s,r) { }
    inline T* operator()() const
    { return (T*)(void*)(BHashStructIter::value()); }
 private:
    void key() const;           // disable
    void value() const;         // disable
};

// Placed spaces within "< T >" since T may be parametrized itself.
#define ForHashStruct(S,T,V) \
{ for (HashStructIter< T > zz(S);zz;zz.next()) { T* V=zz();
#define DummyEndFor }}

#endif
