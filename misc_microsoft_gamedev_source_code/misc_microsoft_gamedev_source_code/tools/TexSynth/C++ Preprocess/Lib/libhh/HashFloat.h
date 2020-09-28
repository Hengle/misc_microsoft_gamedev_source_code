// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef HashFloat_h
#define HashFloat_h

#include "Map.h"

// Override with vars HASHFLOAT_NIGNOREBITS, HASHFLOAT_SMALL.

class HashFloat {
 public:
    HashFloat(int nignorebits=8, float small=1e-4f);
    ~HashFloat();
    float enter(float f);       // ret: filtered value
    void pre_consider(float f); // more robust pre-pass
 private:
    Map<Univ,float> _m;         // encoded float bucket -> float rep
    int _nignorebits;           // num of bits to ignore in FP representation
    float _small;               // numbers with abs<small are grouped at 0
    float _factor;              // used to access prev and next buckets
    float _ractor;              // 1/_factor
    //
    Univ encode(float v) const;
    DISABLE_COPY(HashFloat);
    DISABLE_ASSIGN_INT(HashFloat);
};

#endif
