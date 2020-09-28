// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef HMath_h
#define HMath_h

class Trig {
 public:
// Remember that these functions cannot be called in
// static initialization routines (since the tables are not yet ready)!
    // compute cosf(i/j*2pi)
    static float cosf(int i, int j);
    // compute sinf(i/j*2pi)
    static float sinf(int i, int j);
// internal
    static void init();
 private:
    enum { SIZE=12 };
    static float _costable[SIZE+1][SIZE];
    static float _sintable[SIZE+1][SIZE];
};

//----------------------------------------------------------------------

inline float Trig::cosf(int i, int j)
{
    ASSERTX(_costable[1][0]);
    if (i<0) i=-i;
    ASSERTX(i<j);
    return j<=SIZE?_costable[j][i] : ::cosf(i*2*PI/j);
}

inline float Trig::sinf(int i, int j)
{
    ASSERTX(_costable[1][0]);
    if (i<0) {
        i=-i;
        ASSERTX(i<j);
        return -(j<=SIZE?_sintable[j][i] : ::sinf(i*2*PI/j));
    } else {
        ASSERTX(i<j);
        return j<=SIZE?_sintable[j][i] : ::sinf(i*2*PI/j);
    }
}

#endif
