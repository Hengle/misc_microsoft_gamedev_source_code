// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef LinearFunc_h
#define LinearFunc_h

#include "Geometry.h"

class LinearFunc {
 public:
    LinearFunc() { }
    LinearFunc(float a, float b, float c, float d);
    LinearFunc(const Vector& v, const Point& p);
    ~LinearFunc() { }
    const float& operator[](int i) const { return _p[i]; }
    float& operator[](int i) { return _p[i]; }
    float eval(const Point& p) const;
    void add(const LinearFunc& lf);
 private:
    float _p[4];
};

inline LinearFunc::LinearFunc(float a, float b, float c, float d)
{
    _p[0]=a; _p[1]=b; _p[2]=c; _p[3]=d;
}

inline LinearFunc::LinearFunc(const Vector& v, const Point& p)
{
    _p[0]=v[0]; _p[1]=v[1]; _p[2]=v[2];
    _p[3]=-dot(toVector(p),v);
}

inline float LinearFunc::eval(const Point& p) const
{
    return _p[3]+_p[0]*p[0]+_p[1]*p[1]+_p[2]*p[2];
}

inline void LinearFunc::add(const LinearFunc& lf)
{
    _p[0]+=lf._p[0]; _p[1]+=lf._p[1]; _p[2]+=lf._p[2]; _p[3]+=lf._p[3];
}

#endif
