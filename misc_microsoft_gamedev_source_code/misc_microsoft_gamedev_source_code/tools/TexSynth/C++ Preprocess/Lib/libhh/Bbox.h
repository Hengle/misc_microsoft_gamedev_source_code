// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Bbox_h
#define Bbox_h

#include "Geometry.h"

class Bbox {
 public:
    Bbox() { }
    Bbox(const Point& pmin, const Point& pmax);
    ~Bbox() { }
    void clear();
    void infinite();
    Point& operator[](int i) { return _p[i]; }
    const Point& operator[](int i) const { return _p[i]; }
    void takeunion(const Bbox& bb);
    void takeunion(const Point& pp);
    void intersect(const Bbox& bb);
    int inside(const Bbox& bb) const;
    int overlap(const Bbox& bb) const;
    // uniform scaling into unit cube, centered on x & y, rest at z=0
    Frame getFrameToCube() const;
    Frame getFrameToSmallCube(float cubesize=.8) const;
    float max_side() const;
    void transform(const Frame& frame);
 private:
    Point _p[2];
    // shallow copy ok
};

#endif
