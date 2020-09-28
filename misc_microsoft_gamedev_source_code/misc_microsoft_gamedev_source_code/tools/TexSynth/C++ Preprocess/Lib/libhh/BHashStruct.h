// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef BHashStruct_h
#define BHashStruct_h

// I split away this extra include file from HashStruct.h so as to unconfuse
// SGI's MIPS PRO C++ compiler, which was pulling many functions out of
// the old HashStruct.cxx and compiling it in all files!

#include "Map.h"

class BHashStruct : private BMap {
 public:
    typedef Univ (*HASHF)(Univ);
    typedef int (*CMPF)(Univ, Univ);
    BHashStruct(HASHF phashf, CMPF pcmpf);
    ~BHashStruct();
    using BMap::clear;
    void enter(Univ s);         // s!=0, s must be new
    int add(Univ s);            // ret: is_new
    Univ retrieve(Univ st) const; // ret sfound or 0
    int remove(Univ st);          // ret wasfound
    using BMap::num;
    using BMap::empty;
    Univ getone() const;          // die if empty
    Univ removeone();             // die if empty
    using BMap::operator new;
    using BMap::operator delete;
 private:
friend class BHashStructIter;
    HASHF _hashf;
    CMPF _cmpf;
};

class BHashStructIter : public BMapIter {
 public:
    inline BHashStructIter(const BHashStruct& s) : BMapIter(s) { }
    inline BHashStructIter(const BHashStruct& s, Random& r)
        : BMapIter(s,r) { }
};

//----------------------------------------------------------------------------

inline Univ BHashStruct::getone() const {
    BMapIter si(*this); return si.value();
}
inline Univ BHashStruct::removeone() {
    Univ e=getone(); remove(e); return e;
}

#endif
