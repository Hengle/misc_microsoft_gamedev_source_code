// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef BoundingSphere_h
#define BoundingSphere_h

#include "Geometry.h"

class BoundingSphere {
 public:
    BoundingSphere() { }
    // default operator=() and copy_constructor valid
    void join(const BoundingSphere& bs1, const BoundingSphere& bs2);
 public:
    Point point;
    float radius;
};

inline void
BoundingSphere::join(const BoundingSphere& bs1, const BoundingSphere& bs2)
{
    float d=dist(bs1.point,bs2.point);
    if (bs1.radius>=d+bs2.radius) {
        *this=bs1;
    } else if (bs2.radius>=d+bs1.radius) {
        *this=bs2;
    } else {
        // (from above, d obviously cannot be zero)
        float newradius=(bs1.radius+bs2.radius+d)*0.5f;
        // point==interp(bs1.point,bs2.point,b1)
        //  where r1+(1-b1)*d==r2+(b1)*d
        // therefore b1=((r1-r2)/d+1)/2
        float b1=((bs1.radius-bs2.radius)/d+1.f)*0.5f;
        ASSERTX(b1>=0.f && b1<=1.f);
        point=interp(bs1.point,bs2.point,b1);
        radius=newradius;
    }
}

#endif
