// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef UnionFind_h
#define UnionFind_h

#include "Map.h"

class BUnionFind {
 public:
    BUnionFind();
    ~BUnionFind();
    void clear();
    // elements cannot be 0
    int unify(Univ e1, Univ e2); // ret: were_different
    int equal(Univ e1, Univ e2);
    Univ get_label(Univ e);     // (gets modified by unify() )
 private:
    Map<Univ,Univ> _m;
    //
    Univ irep(Univ e, int& present);
    DISABLE_COPY(BUnionFind);
};

//----------------------------------------------------------------------------

template<class T>
class UnionFind : public BUnionFind {
 public:
    inline UnionFind() { }
    inline ~UnionFind() { }
    inline int unify(T e1, T e2)
    { return BUnionFind::unify(Conv<T>::e(e1),Conv<T>::e(e2)); }
    inline int equal(T e1, T e2)
    { return BUnionFind::equal(Conv<T>::e(e1),Conv<T>::e(e2)); }
    inline T get_label(T e)
    { return Conv<T>::d(BUnionFind::get_label(Conv<T>::e(e))); }
};

#endif
