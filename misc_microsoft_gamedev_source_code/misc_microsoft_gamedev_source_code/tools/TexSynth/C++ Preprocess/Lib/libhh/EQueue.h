// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef EQueue_h
#define EQueue_h

#include "stddef.h"             // for offsetof()

#if 0
{
    ForEQueueOuter(queue,SRVertex,_active,v) {
        assertx(v->_active.linked());
    } EndFor;
}
#endif

// Implements a queue node to embed within other classes (see Queue.h)
class EQueueNode {
 public:
    EQueueNode();
    ~EQueueNode();
    int linked() const;
    EQueueNode* next() const;
 private:
friend class EQueue;
    EQueueNode* _next;
    DISABLE_COPY(EQueueNode);
};

// The object which heads a queue pointing into EQueueNode's.
// Note that a single EQueueNode member can be assigned to one of
//  several mutually exclusive queues (but then the EQueueNode by itself
//  does not provide an efficient way to detect to which queue
//  the node belongs).
class EQueue {
 public:
    EQueue();
    ~EQueue();
    void enqueue(EQueueNode* n);
    const EQueueNode* dequeue();
    const EQueueNode* front() const;
    void insertfirst(EQueueNode* n);
    int empty() const;
    int length() const;         // slow O(n)
    const EQueueNode* delim() const;
    EQueueNode* delim();
 private:
    EQueueNode _delim;
    EQueueNode* _rear;          // ==_delim if empty()
    DISABLE_COPY(EQueue);
};

//----------------------------------------------------------------------

inline EQueueNode::EQueueNode() : _next(0) { }
inline EQueueNode::~EQueueNode() { }
inline int EQueueNode::linked() const { return _next!=0; }
inline EQueueNode* EQueueNode::next() const { return _next; }

inline const EQueueNode* EQueue::delim() const { return &_delim; }
inline EQueueNode* EQueue::delim() { return &_delim; }
inline int EQueue::empty() const { return delim()->next()==delim(); }
inline const EQueueNode* EQueue::front() const { return delim()->next(); }

inline void EQueue::enqueue(EQueueNode* n)
{
    ASSERTX(!n->_next);
    n->_next=delim();
    _rear->_next=n;
    _rear=n;
}

inline void EQueue::insertfirst(EQueueNode* n)
{
    ASSERTX(!n->_next);
    n->_next=delim()->_next;
    if (n->_next==delim()) _rear=n;
    delim()->_next=n;
}

inline const EQueueNode* EQueue::dequeue()
{
    EQueueNode* n=delim()->_next;
    ASSERTX(n!=delim());
    delim()->_next=n->_next;
    if (n->_next==delim()) _rear=delim();
    n->_next=0;
    return n;
}

#define EQueueOuter(T,F,p) (T*)((char*)p-offsetof(T,F))

#define ForEQueue(L,N) \
{ const EQueueNode* const zzdelim=(L).delim(); \
  for (const EQueueNode* N=zzdelim->next();N!=zzdelim;N=N->next()) {
#define DummyEndFor }}

#define ForEQueueOuter(L,T,F,N) \
ForEQueue(L,zz) T* N=EQueueOuter(T,F,zz);

#endif
