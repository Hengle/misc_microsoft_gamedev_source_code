// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef STree_h
#define STree_h

struct BSTreeNode;

class BSTree {
 public:
    typedef int (*CMPF)(Univ, Univ);
    typedef void (*ACCESS)(Univ);
    BSTree(CMPF pcmpf=0);       // cmpf defaults to pointer comparison
    // Note that default pointer comparison is not consistent with
    //  comparisons of *signed* number types.
    ~BSTree();
    void clear();
    int empty() const;
    // data may not be zero!
    Univ enter(Univ data);      // ret=data if not equal to any element
    Univ retrieve(Univ data) const; // ret=0 if no element equal
    Univ remove(Univ data);         // ret=data removed, or 0
    void traverse(ACCESS func);
    void print() const;
    Univ pred(Univ data) const;
    Univ succ(Univ data) const;
    Univ pred_eq(Univ data) const;
    Univ succ_eq(Univ data) const;
    Univ min() const;
    Univ max() const;
 private:
    BSTreeNode* _root;
    CMPF _cmpf;
    // temp variables
    ACCESS _func;               // to avoid passing in recursion
 private:
    int compare(Univ data1, Univ data2) const;
    void rec_clear(BSTreeNode* t);
    BSTreeNode* splay(Univ data, BSTreeNode* t);
    void rec_traverse(const BSTreeNode* t);
    void rec_print(const BSTreeNode* t, int indent) const;
    DISABLE_COPY(BSTree);
};

//----------------------------------------------------------------------------

template<class T>
class STree : public BSTree {
 public:
    typedef int (*CMPF)(T,T);
    typedef void (*ACCESS)(T);
    inline STree(CMPF pcmpf=0) : BSTree((BSTree::CMPF)pcmpf) { }
    inline ~STree() { }
    inline T enter(T e) { return Conv<T>::d(BSTree::enter(Conv<T>::e(e))); }
    inline T retrieve(T e) const
    { return Conv<T>::d(BSTree::retrieve(Conv<T>::e(e))); }
    inline T remove(T e) { return Conv<T>::d(BSTree::remove(Conv<T>::e(e))); }
    inline void traverse(ACCESS func)
    { BSTree::traverse((BSTree::ACCESS)func); }
    inline T pred(T e) const
    { return Conv<T>::d(BSTree::pred(Conv<T>::e(e))); }
    inline T succ(T e) const
    { return Conv<T>::d(BSTree::succ(Conv<T>::e(e))); }
    inline T pred_eq(T e) const
    { return Conv<T>::d(BSTree::pred_eq(Conv<T>::e(e))); }
    inline T succ_eq(T e) const
    { return Conv<T>::d(BSTree::succ_eq(Conv<T>::e(e))); }
    inline T min() const
    { return Conv<T>::d(BSTree::min()); }
    inline T max() const
    { return Conv<T>::d(BSTree::max()); }
};

#endif
