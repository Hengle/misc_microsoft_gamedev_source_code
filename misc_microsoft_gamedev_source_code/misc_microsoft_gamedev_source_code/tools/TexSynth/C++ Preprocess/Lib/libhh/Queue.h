// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Queue_h
#define Queue_h

#if 0
{
    Queue<Vertex> queuev;
    while (!queue.empty()) delete queue.dequeue();
    ForStack(stack,Point*,p) { consider(p); } EndFor;
}
#endif

#include "Pool.h"

class BQueue {
 public:
    BQueue();
    ~BQueue();
    void clear();
    void enqueue(Univ e);       // "insertlast"
    Univ dequeue();             // ret e, else die
    Univ front() const;         // ret e, else die (only look)
    void insertfirst(Univ e);
    int empty() const;
    int length() const;         // slow O(n)
    int contains(Univ e) const; // slow O(n)
    void addtoend(BQueue& q);   // quick merge, clears q
    void addtofront(BQueue& q); // quick merge, clears q
    void copy(const BQueue& q);
 public:                        // should be protected
friend class BQueueIter;
    struct Node {
        POOL_ALLOCATION(Node);
        Univ data;
        Node* next;
    };
 private:
    Node* _front;
    Node* _rear;
    DISABLE_COPY(BQueue);
};

INITIALIZE_POOL_NESTED(BQueue::Node,BQueueNode);

class BQueueIter {
 public:
    BQueueIter(const BQueue& s);
    ~BQueueIter();
    operator void*() const;
    void next();
    Univ operator()() const;
 private:
    const BQueue::Node* _n;
    // operator=() is safe
};

//----------------------------------------------------------------------------

inline BQueueIter::BQueueIter(const BQueue& s) : _n(s._front) { }
inline BQueueIter::~BQueueIter() { }
inline void BQueueIter::next() { _n=_n->next; }
inline BQueueIter::operator void*() const { return _n?(void*)1:0; }
inline Univ BQueueIter::operator()() const { return _n->data; }


inline BQueue::BQueue() : _front(0), _rear(0) { }
inline BQueue::~BQueue() { if (_front) clear(); }
inline int BQueue::empty() const { return !_front; }
inline Univ BQueue::front() const { return _front->data; }

inline void BQueue::enqueue(Univ e)
{
    Node* n=new Node;
    n->data=e;
    n->next=0;
    if (_rear) _rear->next=n;
    else _front=n;
    _rear=n;
}

inline void BQueue::insertfirst(Univ e)
{
    Node* n=new Node;
    n->data=e;
    n->next=_front;
    _front=n;
    if (!_rear) _rear=n;
}

inline Univ BQueue::dequeue()
{
    // Node* n=assertv(_front);
    Node* n=_front;
    Univ e=n->data;
    _front=n->next;
    delete n;
    if (!_front) _rear=0;
    return e;
}

//----------------------------------------------------------------------------

template<class T> class QueueIter;

template<class T>
class Queue : public BQueue {
 public:
    inline Queue() { }
    inline ~Queue() { }
    inline void enqueue(T e) { BQueue::enqueue(Conv<T>::e(e)); }
    inline T dequeue() { return Conv<T>::d(BQueue::dequeue()); }
    inline T front() const { return Conv<T>::d(BQueue::front()); }
    inline void insertfirst(T e) { BQueue::insertfirst(Conv<T>::e(e)); }
    inline int contains(T e) const { return BQueue::contains(Conv<T>::e(e)); }
    inline void addtoend(Queue<T>& q) { BQueue::addtoend(q); }
    inline void addtofront(Queue<T>& q) { BQueue::addtofront(q); }
    inline void copy(const Queue<T>& q) { BQueue::copy(q); }
//      typedef QueueIter<T> Iter;
};

template<class T>
class QueueIter : public BQueueIter {
 public:
    inline QueueIter(const Queue<T>& q) : BQueueIter(q) { }
    inline ~QueueIter() { }
    inline T operator()() const
    { return Conv<T>::d(BQueueIter::operator()()); }
};

#define ForQueue(S,T,V) { for (QueueIter< T > zz(S);zz;zz.next()) { T V=zz();
#define DummyEndFor }}

#endif
