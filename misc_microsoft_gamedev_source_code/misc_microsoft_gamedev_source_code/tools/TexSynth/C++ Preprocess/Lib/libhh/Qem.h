// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Qem_h
#define Qem_h

#include "Array.h"
#include "Matrix.h"

#if defined(MAKEDEPEND)
// Let 'make depend' correctly create dependencies on Qem.cxx for clients.
#include "Qem.cxx"
#endif

template<class T, int n>
class Qem {
 public:
    Qem() { }
    ~Qem() { }
    void set_zero();
    void add(const Qem<T,n>& qem);
    void scale(float f);
// Create
    void set_d2_from_plane(const float* dir, float d); // dir*p+d==0 !
    void set_d2_from_point(const float* p0);
    void set_distance_gh98(const float* p0, const float* p1,
                           const float* p2);
    void set_distance_hh99(const float* p0, const float* p1,
                           const float* p2);
// Evaluate
    float evaluate(const float* p) const;
// Find min.  All these return "success"; minp unchanged if unsuccessful
    // regular version
    bool compute_minp(float* minp) const;
    // First nfixed variables are fixed as given; solve for remainder.
    bool compute_minp_constr_first(float* minp, int nfixed) const;
    // Linear functional constraint: lf[0..n-1] * x + lf[n] == 0
    bool compute_minp_constr_lf(float* minp, const float* lf) const;
    // Sparse version w/ independent attribs and lf constr only on geom
    bool fast_minp_constr_lf(float* minp, const float* lf) const;
// Special versions for wedge-based mesh simplification.
    // They take multiple quadrics (*this==ar_q[0]) and assume that the
    //  first 3 rows of matrix variables are shared.
    // regular version
    bool ar_compute_minp(const Array<Qem<T,n>*>& ar_q,
                         Matrix<float>& minp) const;
    // linear functional constraint must be volumetric!
    bool ar_compute_minp_constr_lf(const Array<Qem<T,n>*>& ar_q,
                                   Matrix<float>& minp, const float* lf) const;
// Misc
    void print_cerr() const;
 private:
    // qem(v) = v*a*v + 2*b*v + c
    T _a[(n*(n+1))/2]; // upper triangle of symmetric matrix
    T _b[n];
    T _c;
    template<class TT, int n1, int n2> friend
        void qem_add_submatrix(Qem<TT,n1>& q1, const Qem<TT,n2>& q2);
};

template<class T, int n1, int n2>
extern void qem_add_submatrix(Qem<T,n1>& q1, const Qem<T,n2>& q2);

#endif
