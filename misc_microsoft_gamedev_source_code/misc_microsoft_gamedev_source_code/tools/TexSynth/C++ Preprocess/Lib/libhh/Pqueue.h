// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Pqueue_h
#define Pqueue_h

#include "Map.h"
#include "Pool.h"

// Self-resizing priority queue
class BPqueue {
 public:
    BPqueue(int size=0);
    ~BPqueue();
    void clear();
    void enter(Univ e, float pri); // pri>=0 always
    int empty() const;
    int num() const;
    Univ min() const;           // ret e, else die
    float minpriority() const;  // ret pri, else die
    Univ removemin();           // ret e, else die
    void enterUnsorted(Univ e, float pri);
    void sort();
 protected:
    struct Node { Univ e; float pri; };
    int _size;
    int _num;
    Node* _ar;
    // {_ar,_size,_num} could be made Array<Node> but for efficiency
    void resize(int newsize);
    void nswitch(int n1, int n2);
    void adjust(int n, int up, int down);
 private:
    DISABLE_COPY(BPqueue);
    DISABLE_ASSIGN_INT(BPqueue);
};

// Hashed priority queue
class BHPqueue : private BPqueue {
 public:
    BHPqueue(int size=0);
    ~BHPqueue();
    void clear();
    void enter(Univ e, float pri); // pri>=0 always
    using BPqueue::empty;
    using BPqueue::num;
    using BPqueue::min;
    using BPqueue::minpriority;
    Univ removemin();
    int contains(Univ e) const;
    float retrieve(Univ e) const; // ret pri or <0
    float remove(Univ e);         // ret pri or <0
    float update(Univ e, float pri); // ret prevpri or <0
    float enterupdate(Univ e, float pri); // ret prevpri or <0
    bool enterupdateifsmaller(Univ e, float pri); // ret: was updated
    bool enterupdateifgreater(Univ e, float pri); // ret: was updated
    void enterUnsorted(Univ e, float pri);
    void sort();
 private:
    Map<Univ,int> _m;            // element -> index in array
    void nswitch(int n1, int n2);
    void adjust(int n, int up, int down);
    int find(Univ e) const;     // ret -1 if not there
};

//----------------------------------------------------------------------------

inline int BPqueue::empty() const { return !_num; }
inline int BPqueue::num() const { return _num; }
inline Univ BPqueue::min() const { ASSERTX(_num); return _ar[0].e; }
inline float BPqueue::minpriority() const { ASSERTX(_num); return _ar[0].pri; }

inline void BPqueue::enterUnsorted(Univ e, float pri)
{
    ASSERTX(pri>=0);
    if (_num==_size) resize(_size*2);
    _ar[_num].e=e;
    _ar[_num].pri=pri;
    _num++;
}

inline void BHPqueue::enterUnsorted(Univ e, float pri)
{
    _m.enter(e,_num);
    BPqueue::enterUnsorted(e,pri);
}

//----------------------------------------------------------------------------

template<class T>
class Pqueue : public BPqueue {
 public:
    inline Pqueue(int size=0) : BPqueue(size) { }
    inline ~Pqueue() { }
    inline void enter(T e, float pri) { BPqueue::enter(Conv<T>::e(e),pri); }
    inline T min() const { return Conv<T>::d(BPqueue::min()); }
    inline T removemin() { return Conv<T>::d(BPqueue::removemin()); }
    inline void enterUnsorted(T e, float pri)
    { BPqueue::enterUnsorted(Conv<T>::e(e),pri); }
};

template<class T>
class HPqueue : public BHPqueue {
 public:
    inline HPqueue(int size=0) : BHPqueue(size) { }
    inline ~HPqueue() { }
    inline void enter(T e, float pri) { BHPqueue::enter(Conv<T>::e(e),pri); }
    inline T min() const { return Conv<T>::d(BHPqueue::min()); }
    inline T removemin() { return Conv<T>::d(BHPqueue::removemin()); }
    inline void enterUnsorted(T e, float pri)
    { BHPqueue::enterUnsorted(Conv<T>::e(e),pri); }
    inline int contains(T e) const
    { return BHPqueue::contains(Conv<T>::e(e)); }
    inline float retrieve(T e) const
    { return BHPqueue::retrieve(Conv<T>::e(e)); }
    inline float remove(T e)
    { return BHPqueue::remove(Conv<T>::e(e)); }
    inline float update(T e, float pri)
    { return BHPqueue::update(Conv<T>::e(e),pri); }
    inline float enterupdate(T e, float pri)
    { return BHPqueue::enterupdate(Conv<T>::e(e),pri); }
    inline bool enterupdateifsmaller(T e, float pri)
    { return BHPqueue::enterupdateifsmaller(Conv<T>::e(e),pri); }
    inline bool enterupdateifgreater(T e, float pri)
    { return BHPqueue::enterupdateifgreater(Conv<T>::e(e),pri); }
};

#endif
