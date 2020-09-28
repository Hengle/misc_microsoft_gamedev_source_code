// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef EStack_h
#define EStack_h

#include "stddef.h"             // for offsetof()

#if 0
{
    ForEStackOuter(queue,SRVertex,_active,v) {
        assertx(v->_active.linked());
    } EndFor;
}
#endif

// Implements a stack node to embed within other classes (see Stack.h)
class EStackNode {
 public:
    EStackNode();
    ~EStackNode();
    int linked() const;
    EStackNode* next() const;
 private:
friend class EStack;
    EStackNode* _next;
    DISABLE_COPY(EStackNode);
};

// The object which heads a stack pointing into EStackNode's.
// Note that a single EStackNode member can be assigned to one of
//  several mutually exclusive stacks (but then the EStackNode by itself
//  does not provide an efficient way to detect to which stack
//  the node belongs).
class EStack {
 public:
    EStack();
    ~EStack();
    void push(EStackNode* n);
    const EStackNode* pop();
    const EStackNode* front() const;
    int empty() const;
    int length() const;         // slow O(n)
    void clear();
    const EStackNode* delim() const;
    EStackNode* delim();
 private:
    EStackNode _delim;
    DISABLE_COPY(EStack);
};

//----------------------------------------------------------------------

inline EStackNode::EStackNode() : _next(0) { }
inline EStackNode::~EStackNode() { }
inline int EStackNode::linked() const { return _next!=0; }
inline EStackNode* EStackNode::next() const { return _next; }

inline const EStackNode* EStack::delim() const { return &_delim; }
inline EStackNode* EStack::delim() { return &_delim; }
inline int EStack::empty() const { return delim()->next()==delim(); }
inline const EStackNode* EStack::front() const { return delim()->next(); }

inline void EStack::push(EStackNode* n)
{
    ASSERTX(!n->_next);
    n->_next=delim()->_next;
    delim()->_next=n;
}

inline const EStackNode* EStack::pop()
{
    EStackNode* n=delim()->_next;
    ASSERTX(n!=delim());
    delim()->_next=n->_next;
    n->_next=0;
    return n;
}

#define EStackOuter(T,F,p) (T*)((char*)p-offsetof(T,F))

#define ForEStack(L,N) \
{ const EStackNode* const zzdelim=(L).delim(); \
  for (const EStackNode* N=zzdelim->next();N!=zzdelim;N=N->next()) {
#define DummyEndFor }}

#define ForEStackOuter(L,T,F,N) \
ForEStack(L,zz) T* N=EStackOuter(T,F,zz);

#endif
