// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef NLOpt_h
#define NLOpt_h

#include "Hh.h"
#include "Array.h"

// Minimize a function f(X) : R^n -> R
// Supply:
//  - an initial guess X_0
//  - a function to evaluate f(X)
//  - a function to evaluate grad(f(X))==(df/dx_1, df/dx_2, ..., df/dx_n)

class NLOpt {
 public:
    NLOpt(int n, float (*feval)(), void (*fevalg)(Array<float>& ret_grad));
    ~NLOpt();
    bool iterate(Array<float>* px); // modify current guess, return: done
 private:
// approach-independent:
    int _n;
    float (*_feval)();
    void (*_fevalg)(Array<float>& ret_grad);
    int _niter;
    bool _done;
// approach-dependent:
    int _m;
    Array<double> _fx;
    Array<double> _fg;
    Array<float> _gval;
    Array<double> _diag;
    Array<double> _w;
};

#endif
