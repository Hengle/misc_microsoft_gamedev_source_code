//==============================================================================
// clip2d.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _CLIP2D_H_
#define _CLIP2D_H_


//==============================================================================
// These clip a 2D point against an axis-aligned rectangle.
//==============================================================================
bool clipLine(long &x1, long &z1, long &x2, long &z2, long xmin, long zmin, long xmax, long zmax);
bool clipLine(float &x1, float &z1, float &x2, float &z2, float xmin, float zmin, float xmax, float zmax);


#endif