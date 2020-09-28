// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef GeomOp_h
#define GeomOp_h

#if 0
{
    float ang[3];
    FrameToEuclideanAngles(f,ang);
    EuclideanAnglesToFrame(ang,f);
}
#endif

#include "Geometry.h"

// *** Radii

// Compute the circumscribed radius of the 3 points p0, p1, p2.
extern float CircumRadius(const Point& p0, const Point& p1, const Point& p2);

// Compute the inscribed radius of the 3 points p0, p1, p2.
extern float InRadius(const Point& p0, const Point& p1, const Point& p2);

extern float AspectRatio(const Point& p0, const Point& p1, const Point& p2);

// *** Misc

// ret: cosf of signed angle away from "flattness" (==exterior angle)
// range -1..1  (or -2 if a triangle is degenerate)
extern float DihedralAngleCos(const Point& p1, const Point& p2,
                              const Point& po1, const Point& po2);

// return angle from -PI to PI (negative is concave), or -10 if degeneracy
//  (==exterior angle)  (cosf(SignedDihedralAngle())==DihedralAngleCos())
extern float SignedDihedralAngle(const Point& p1, const Point& p2,
                                 const Point& po1, const Point& po2);

// Compute solid angle at p surrounded by ordered loop pa[]
// range 0..4*PI
extern float SolidAngle(const Point& p, const Point pa[], int np);

// ret: cosf of signed angle away from "flattness" (==exterior angle)
// range -1..1  (or -2 if an edge is degenerate)
extern float AngleCos(const Point& p1, const Point& p2, const Point& p3);

// *** Frames and Euclidean angles

// Compute Euclidean angles of f.
extern void FrameToEuclideanAngles(const Frame& f, float ang[3]);

// Modify f by setting v[0],v[1],v[2] according to Euclidean angles.
// f.p is unchanged.
extern void EuclideanAnglesToFrame(const float ang[3], Frame& f);

// Modify f so that its x axis points towards p and its y axis is vertical;
// f.p is ignored and unchanged.
extern void FrameAimAt(Frame& f, const Vector& v);

// Modify f so that its y axis lies in the xy plane
extern void FrameMakeLevel(Frame& f);

// Modify f so that its x and y axes lie in the xy plane.
extern void FrameMakeHoriz(Frame& f);

#endif
