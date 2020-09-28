// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Principal_h
#define Principal_h

#include "Geometry.h"
#include "Matrix.h"

// Given nv points pa[], compute the principal component frame f,
// and the (redundant) eigenvalues eimag[3].
// The frame f is guaranteed to be invertible and orthogonal, as the axis
// will always have non-zero (albeit very small) lengths.
// The values eimag[] will be zero for the axes that should be zero.
// The frame f is also guaranteed to be right-handed.
extern void PrincipalComponents(const Point pa[], int nv, Frame& f, float eimag[3]);

// Same but for nv vectors va[].
// Note that the original of frame f, f.p, will therefore be the origin.
extern void PrincipalComponents(const Vector va[], int nv, Frame& f, float eimag[3]);

// input: mi[m][n]: m data points of dimension n
// output: mo[n][n]: n orthonormal eigenvectors (rows, by decreasing eigenv.)
//         eimag[n]: eigenvalues
// note: the mean must be subtracted out of mi[][] if desired.
extern void PrincipalComponents(const Matrix<float>& mi, Matrix<float>& mo, Array<float>& eimag);

// Same using incremental approximate computation.
// mo.ysize() specifies the desired number of largest eigenvectors.
// note: the mean must be subtracted out of mi[][] if desired.
extern void IncPrincipalComponents(const Matrix<float>& mi, Matrix<float>& mo, Array<float>& eimag, int niter);

// Using expectation maximization.
// mo.ysize() specifies the desired number of largest eigenvectors.  niter~10-50.
// note: the mean must be subtracted out of mi[][] if desired.
extern bool EMPrincipalComponents(const Matrix<float>& mi, Matrix<float>& mo, Array<float>& eimag, int niter);

// subtract out the mean of the rows.
extern void SubtractMean(Matrix<float>& mi);

#endif
