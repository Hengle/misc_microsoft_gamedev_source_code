// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Stack_h
#define Stack_h

#if 0
{
    Stack<Edge> stack;
    while (!stack.empty()) delete stack.pop();
    ForStack(stack,Point*,p) { consider(p); } EndFor;
}
#endif

#include "Pool.h"

class BStack {
 public:
    BStack();
    ~BStack();
    void clear();
    void push(Univ e);
    Univ pop();                 // ret e, else die
    Univ top() const;           // ret e, else die
    int empty() const;
    int height() const;         // slow O(n)
    int contains(Univ e) const; // slow O(n)
    int remove(Univ e);         // slow O(n), ret: was_there
    POOL_ALLOCATION(BStack);
 public:                        // should be private
friend class BStackIter;
    struct Node {
        POOL_ALLOCATION(Node);
        Univ data;
        Node* next;
    };
    const Node* fastiter_initialize() const;
 private:
    Node* _top;
    DISABLE_COPY(BStack);
};

INITIALIZE_POOL(BStack);
INITIALIZE_POOL_NESTED(BStack::Node,BStackNode);

class BStackIter {
 public:
    BStackIter(const BStack& s);
    ~BStackIter();
    operator void*() const;
    Univ next();
    Univ operator()() const;
    void reinit(const BStack& s);
 private:
    const BStack::Node* _n;
    // operator=() is safe
};

//----------------------------------------------------------------------------

inline BStackIter::BStackIter(const BStack& s) : _n(s._top) { }
inline BStackIter::~BStackIter() { }
inline Univ BStackIter::next() { Univ d=_n->data; _n=_n->next; return d; }
inline BStackIter::operator void*() const { return _n?(void*)1:0; }
inline Univ BStackIter::operator()() const { return _n->data; }
inline void BStackIter::reinit(const BStack& s) { _n=s._top; }


inline BStack::BStack() : _top(0) { }
inline BStack::~BStack() { if (_top) clear(); }
inline int BStack::empty() const { return !_top; }
inline Univ BStack::top() const { return _top->data; }

inline void BStack::push(Univ e)
{
    Node* n=new Node;
    n->data=e;
    n->next=_top;
    _top=n;
}

inline Univ BStack::pop()
{
    // Node* n=assertv(_top);
    Node* n=_top;
    Univ e=n->data;
    _top=n->next;
    delete n;
    return e;
}

inline const BStack::Node* BStack::fastiter_initialize() const
{
    return _top;
}

//----------------------------------------------------------------------------

template<class T> class StackIter;

template<class T>
class Stack : public BStack {
 public:
    inline Stack() { }
    inline ~Stack() { }
    inline void push(T e) { BStack::push(Conv<T>::e(e)); }
    inline T pop() { return Conv<T>::d(BStack::pop()); }
    inline T top() { return Conv<T>::d(BStack::top()); }
    inline int contains(T e) const { return BStack::contains(Conv<T>::e(e)); }
    inline int remove(T e) { return BStack::remove(Conv<T>::e(e)); }
//      typedef StackIter<T> Iter;
};

template<class T>
class StackIter : public BStackIter {
 public:
    inline StackIter(const Stack<T>& s) : BStackIter(s) { }
    inline ~StackIter() { }
    inline T next() { return Conv<T>::d(BStackIter::next()); }
    inline T operator()() const
    { return Conv<T>::d(BStackIter::operator()()); }
    inline void reinit(const Stack<T>& s) { BStackIter::reinit(s); }
};

//#define ForStack(S,T,V) { for (StackIter< T > zz(S);zz;zz.next()) { T V=zz();

#define ForStack(S,T,V) \
{ register const BStack::Node* zz_n=(S).fastiter_initialize(); \
  for (;zz_n;zz_n=zz_n->next) \
  { T V=Conv< T >::d(zz_n->data);
#define DummyEndFor }}

#endif
