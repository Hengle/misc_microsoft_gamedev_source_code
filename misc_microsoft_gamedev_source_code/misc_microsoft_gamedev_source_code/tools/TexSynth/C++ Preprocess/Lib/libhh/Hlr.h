// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Hlr_h
#define Hlr_h

#include "Geometry.h"
#include "Polygon.h"
#include "Kdtree.h"
#include "Array.h"
#include "Bbox.h"

struct HlrSegment;

struct HlrPolygon {
    const Polygon* p;
    Vector n;
    float d;
    float tol;
    Bbox bb;
};

// Assumes orthographic projection of objects lying in unit cube along -x axis.
// Proper tolerancing requires depth in unit cube.
// Kdtree requires y and z to be in unit cube.
// To do general perspective projection, transform all elements to lie in unit
// cube prior to calling hlr.
class Hlr {
 public:
    Hlr();
    ~Hlr();
    // enter polygons.
    // polygons are not copied but may be deleted with deleteclear().
    // remember to delete poly if enter() returns 0.
    int enter(const Polygon* poly); // ret: was_non_degen
    // draw objects (reentrant I think)
    int drawpoint(const Point& p) const; // ret: is_visible
    void setdrawsegcb(void (*func)(const Point& p1, const Point& p2));
    void drawseg(const Point& p1, const Point& p2) const;
    // clear structure (leave it to user to delete polygons)
    void clear();
    // delete polygons and clear structure
    void deleteclear();
 private:
    void (*_fdrawsegcb)(const Point& p1, const Point& p2);
    Array<HlrPolygon> _polygons;
    Kdtree<int> _kd;
    //
    friend int HlrPointConsiderpoly(int pn, float bb[][2],
                                    KdtreeNode<int>* kdloc);
    void rendersegkd(HlrSegment& s, KdtreeNode<int>* kdloc) const;
    friend int HlrSegConsiderpoly(int pn, float bb[][2],
                                  KdtreeNode<int>* kdloc);
    int handlepolygon(HlrSegment& s, int pn, KdtreeNode<int>* kdloc) const;
    DISABLE_COPY(Hlr);
};

#endif
