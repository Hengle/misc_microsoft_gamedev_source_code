// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef QArray_h
#define QArray_h

// T must have a public operator=()
template<class T, int size>
class QArray {
 private:
    T _a[size];
    // conceptually, _if<_ir
    int _if;                    // index to front (next element to dequeue)
    int _ir;                    // index to rear (location for next enqueue)
 public:
    QArray() {
        ASSERTX(size>0);
        clear();
    }
    ~QArray() {
        // nothing to be done
    }
    void clear() {
        _if=0;
        _ir=0;
    }
    bool empty() const {
        return _if==_ir;
    }
    void copy(const QArray<T,size>& q);
    void enqueue(T e) {
        _a[_ir]=e;
        _ir++;
        _ir-= (_ir==size)*size;
        ASSERTX(_if!=_ir);
    }
    T dequeue() {
        ASSERTX(_if!=_ir);
        T e=_a[_if];
        _if++;
        _if-= (_if==size)*size;
        return e;
    }
    T front() {
        ASSERTX(_if!=_ir);
        return _a[_if];
    }
    void insertfirst(T e) {
        _ir--;
        _ir+= (_ir==0)*size;
        _a[_ir]=e;
        ASSERTX(_if!=_ir);
    }
};

template<class T, int size>
void QArray<T,size>::copy(const QArray<T,size>& q) {
    if (q._if<=q._ir) {
        For (int i=q._if;i<q._ir;i++) { _a[i]=q._a[i]; } EndFor;
    } else {
        For (int i=q._if;i<size;i++) { _a[i]=q._a[i]; } EndFor;
        For (int i=0;i<q._ir;i++) { _a[i]=q._a[i]; } EndFor;
    }
    _if=q._if;
    _ir=q._ir;
}

#endif
