// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Facedistance_h
#define Facedistance_h

#include "Geometry.h"
#include "Bbox.h"

// Find a lower bound on the distance between p and triangle (p1,p2,p3)
inline float LBDistPTri(const Point& p, const Point& p1,
                        const Point& p2, const Point& p3);

inline float LBDistPBbox(const Point& p, const Bbox& bb);

extern float DistPointTriangle2(const Point& p, const Point& p1,
                                const Point& p2, const Point& p3);

// Given point p and triangular face with vertices p1,p2,p3,
//  return distance squared dis2, and compute
//  convex barycentric coordinates of the closest point cba within
//  the triangle, and clp (the closest point in the triangle).
extern float ProjectPTri(const Point& p, const Point& p1,
                         const Point& p2, const Point& p3,
                         Bary& ret_cba, Point& ret_clp);

// Return squared distance from p to segment (p1,p2).
// Optionally return barycentric coordinate with respect to p1
//  p=~=interp(p1,p2,cba)   (cba lies within [0,1])
extern float ProjectPSeg(const Point& p, const Point& p1,
                         const Point& p2, float* ret_cba);

//----------------------------------------------------------------------------

inline float LBDistPTri(const Point& p, const Point& p1,
                        const Point& p2, const Point& p3)
{
    float d=0;
    ForIndex(c,3) {
        float v1=p1[c], mi=v1, ma=v1, v2=p2[c], v3=p3[c];
        if (v2<mi) mi=v2; else if (v2>ma) ma=v2;
        if (v3<mi) mi=v3; else if (v3>ma) ma=v3;
        float v=p[c], a;
        if ((a=v-ma)>0) { if (a>d) d=a; }
        else if ((a=mi-v)>0) { if (a>d) d=a; }
    } EndFor;
    return d;
}

inline float LBDistPBbox(const Point& p, const Bbox& bb)
{
//     float d=0;
//     ForIndex(c,3) {
//         float v=p[c], a;
//         if ((a=v-bb[1][c])>0) { if (a>d) d=a; }
//         else if ((a=bb[0][c]-v)>0) { if (a>d) d=a; }
//     } EndFor;
    float d=0, v, a;
    v=p[0];
    if ((a=v-bb[1][0])>0) { d=a; }
    else if ((a=bb[0][0]-v)>0) { d=a; }
    v=p[1];
    if ((a=v-bb[1][1])>0) { if (a>d) d=a; }
    else if ((a=bb[0][1]-v)>0) { if (a>d) d=a; }
    v=p[2];
    if ((a=v-bb[1][2])>0) { if (a>d) d=a; }
    else if ((a=bb[0][2]-v)>0) { if (a>d) d=a; }
    return d;
}

#endif
