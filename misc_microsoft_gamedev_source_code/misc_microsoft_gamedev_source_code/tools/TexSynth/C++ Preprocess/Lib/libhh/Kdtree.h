// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Kdtree_h
#define Kdtree_h

#include "Array.h"
class BKdtreeNode;
class Stat;

class BKdtree {
 public:
    BKdtree(int nd, int pmaxlevel=8); // # of dimensions, max # subdiv.
    ~BKdtree();
    void clear();
    void allowduplication(float fsize);
    void enter(Univ id, const float bb[][2]); // bb copied
    // search is reentrant (for Hlr).
    // Start searching at loc (default _root) for objects whose bb
    // intersect the one given.
    // For each object found, call cbfunc with id, bb, and current
    // location in the tree.
    // cbfunc ret: 0=nothing, 1=bb_shrunk, 2=stop_search
    // cbfunc may modify the bb by shrinking it
    typedef int (*CBFUNC)(Univ id, float bb[][2], BKdtreeNode* floc);
    int search(float bb[][2], CBFUNC cbfunc, BKdtreeNode* loc=0) const;
                                // ret was_stopped
    void print() const;
 private:
    const int _nd;              // number of dimensions of space
    const int _maxlevel;         // maximum # of subdivision on each axis
    BKdtreeNode* _root;
    float _fsize;                // ok to duplicate if average edge<_fsize
    //
    void recClear(BKdtreeNode* n);
    void recDepth(BKdtreeNode* n, Stat& stat, int depth) const;
    void recEnter(Univ id, const float bb[][2],
                  BKdtreeNode* n, SArray<float>& aval,
                  int level, float inc, int axis, float avgel);
    int recSearch(BKdtreeNode* n, BKdtreeNode* lca,
                  float bb[][2], CBFUNC cbfunc) const;
    void recPrint(BKdtreeNode* n, int l) const;
    DISABLE_COPY(BKdtree);
    DISABLE_ASSIGN_INT(BKdtree);
};

//----------------------------------------------------------------------------

template<class T> class KdtreeNode { public: KdtreeNode() { } }; // dummy

template<class T>
class Kdtree : public BKdtree {
 public:
    inline Kdtree(int nd, int pmaxlevel=8): BKdtree(nd,pmaxlevel) { }
    inline ~Kdtree() { }
    inline void enter(T e, const float bb[][2])
    { BKdtree::enter(Conv<T>::e(e),bb); }
    typedef int (*CBFUNC)(T e, float bb[][2], KdtreeNode<T>* floc);
    inline int search(float bb[][2], CBFUNC cbfunc, KdtreeNode<T>* loc=0) const
    { return BKdtree::search(bb,(BKdtree::CBFUNC)cbfunc, (BKdtreeNode*)loc); }
};

#endif
