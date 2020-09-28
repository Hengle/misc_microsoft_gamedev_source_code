// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef BinarySearch_h
#define BinarySearch_h

// Given xl<xh, feval(xl)<=y_desired<feval(xh)
// Find x such that exists x', x<=x'<x+xtol, feval(x')==y_desired
extern float
ContinuousBinarySearch(float (*feval)(float x),
                       float xl, float xh, float xtol, float y_desired);

// Given xl<xh, feval(xl)<=y_desired<feval(xh)
// Find x such that feval(x)<=y_desired<feval(x+1)
extern int
DiscreteBinarySearch(float (*feval)(int x), int xl, int xh, float y_desired);

// Given xl<xh, ar[xl]<=y_desired<ar[xh]
// Find x such that ar[x]<=y_desired<ar[x+1]
extern int
DiscreteBinarySearch(const float ar[], int xl, int xh, float y_desired);

// Given xl<xh, ar[xl]<=y_desired<ar[xh]
// Find x such that ar[x]<=y_desired<ar[x+1]
extern int
DiscreteBinarySearch(const int ar[], int xl, int xh, int y_desired);

#endif
