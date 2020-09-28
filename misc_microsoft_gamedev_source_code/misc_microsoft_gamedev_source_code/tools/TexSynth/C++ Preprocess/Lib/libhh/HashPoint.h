// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef HashPoint_h
#define HashPoint_h

#include "HashFloat.h"
#include "HashStruct.h"
#include "Geometry.h"
#include "Pool.h"

struct HashPointEntry {         // moved out for __SGICC
    Point p;
    int index;
    POOL_ALLOCATION(HashPointEntry);
};

INITIALIZE_POOL(HashPointEntry);

// Override with vars HASHFLOAT_NIGNOREBITS, HASHFLOAT_SMALL.

class HashPoint {
 public:
    HashPoint(int nignorebits=8, float small=1e-4);
    ~HashPoint();
    int enter(const Point& p);  // p is copied, ret: index (first is 0)
    void pre_consider(const Point& p);  // more robust pre-pass
 private:
    typedef HashPointEntry Entry;
    //
    HashFloat* _hf[3];
    HashStruct<HashPointEntry> _hs;
    int _index;                 // current index, assigned to next new point
    //
    static Univ hashEntry(const Entry* e);
    static int cmpEntry(const Entry* e1, const Entry* e2);
    DISABLE_COPY(HashPoint);
    DISABLE_ASSIGN_INT(HashPoint);
};

#endif
