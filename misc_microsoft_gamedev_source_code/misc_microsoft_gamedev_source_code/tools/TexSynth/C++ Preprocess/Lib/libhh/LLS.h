// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef LLS_h
#define LLS_h

#include "Array.h"
#include "Matrix.h"
#include "Stack.h"

class LLS {                     // For A*x=b, find argmin_x |A*x-b|
 public:
    // virtual constructor
    static LLS* create_new(int m, int n, int nd, float nonzerofrac);
    virtual ~LLS();
    virtual void clear();
    // All entries will be zero unless entered as below.
    void enter_a(const Matrix<float>& mat);
    void enter_a_r(int r, const Array<float>& ar);
    void enter_a_c(int c, const Array<float>& ar);
    virtual void enter_a_rc(int r, int c, float val)=0; // r<_m, c<_n
    virtual void enter_a_r(int r, const float* va)=0;   // r<_m, va[_n]
    virtual void enter_a_c(int c, const float* va)=0;   // c<_n, va[_m]
    void enter_b(const Matrix<float>& mat);
    void enter_b_r(int r, const Array<float>& ar);
    void enter_b_c(int c, const Array<float>& ar);
    void enter_b_rc(int r, int c, float val);           // r<_m, c<_nd
    void enter_b_r(int r, const float* va);             // r<_m, va[_nd]
    void enter_b_c(int c, const float* va);             // c<_nd, va[_m]
    void enter_xest(const Matrix<float>& mat);
    void enter_xest_r(int r, const Array<float>& ar);
    void enter_xest_c(int c, const Array<float>& ar);
    void enter_xest_rc(int r, int c, float val);        // r<_n, c<_nd
    void enter_xest_r(int r, const float* va);          // r<_n, va[_nd]
    void enter_xest_c(int c, const float* va);          // c<_nd, va[_n]
    virtual bool solve(double* rssb=0, double* rssa=0)=0; // ret: success
    void get_x(Matrix<float>& mat);
    void get_x_r(int r, Array<float>& ar);
    void get_x_c(int c, Array<float>& ar);
    float get_x_rc(int r, int c);                         // r<_n, c<_nd
    void get_x_r(int r, float* va);                       // r<_n, va[_nd]
    void get_x_c(int c, float* va);                       // c<_nd, va[_n]
    int num_rows() const { return _m; }
 protected:
    int _m, _n, _nd;
    Matrix<float> _b;           // [_nd][_m]
    Matrix<float> _x;           // [_nd][_n];
    bool _solved;               // solve() can destroy A, so check
    LLS(int m, int n, int nd);
 private:
    DISABLE_COPY(LLS);
};

// Sparse conjugate gradient approach.
class SparseLLS : public LLS {
 public:
    SparseLLS(int m, int n, int nd);
    ~SparseLLS();
    void clear();
    void enter_a_rc(int r, int c, float val);
    void enter_a_r(int r, const float* va);
    void enter_a_c(int c, const float* va);
    bool solve(double* rssb=0, double* rssa=0);
    void set_tolerance(float tolerance); // default 1e-10f
    void set_maxiter(int maxiter);       // default BIGINT
    void set_verbose(int verb);          // default 0
 public:                                 // should be private
    struct Ival {
        Ival() { }
        Ival(int i, float v) : _i(i), _v(v) { }
        int _i;
        float _v;
    };
 private:
    SArray<Array<Ival> > _rows;
    SArray<Array<Ival> > _cols;
    float _tolerance;
    int _maxiter;
    int _verb;
    int _nentries;
    void multmv(const SArray<float>& vi, SArray<float>& vo) const;
    void multmtv(const SArray<float>& vi, SArray<float>& vo) const;
    bool docg(SArray<float>& x, SArray<float>& h,
              double* prssb, double* prssa) const;
    DISABLE_COPY(SparseLLS);
};

class FullLLS : public LLS {
 public:
    FullLLS(int m, int n, int nd);
    ~FullLLS();
    void clear();
    void enter_a_rc(int r, int c, float val);
    void enter_a_r(int r, const float* va);
    void enter_a_c(int c, const float* va);
    bool solve(double* rssb=0, double* rssa=0);
 protected:
    Matrix<float> _a;           // [_m][_n]
    virtual bool solveaux()=0;  // abstract class
 private:
    double getrss();
};

// LU decomposition on A^t*A=A^t*b (slow).
class LudLLS : public FullLLS {
 public:
    LudLLS(int m, int n, int nd);
    ~LudLLS();
 protected:
    bool solveaux();
};

// Givens substitution approach.
class GivensLLS : public FullLLS {
 public:
    GivensLLS(int m, int n, int nd);
    ~GivensLLS();
 protected:
    bool solveaux();
};

// Singular value decomposition.
class SvdLLS : public FullLLS {
 public:
    SvdLLS(int m, int n, int nd);
    ~SvdLLS();
 protected:
    bool solveaux();
    Array<float> _fa, _fb, _s, _work;
};

// Double-precision version of above.
class SvdDoubleLLS : public FullLLS {
 public:
    SvdDoubleLLS(int m, int n, int nd);
    ~SvdDoubleLLS();
 protected:
    bool solveaux();
    Array<double> _fa, _fb, _s, _work;
};

// QR decomposition.
class QrdLLS : public FullLLS {
 public:
    QrdLLS(int m, int n, int nd);
    ~QrdLLS();
 protected:
    bool solveaux();
    Array<float> _fa, _fb, _work;
    Array<int> _jpvt;
};


//------------------------------------------------------------------------

inline void LLS::enter_b_rc(int r, int c, float val)
{
    ASSERTX(c>=0 && c<_nd && r>=0 && r<_m);
    _b[c][r]=val;
}

inline void LLS::enter_xest_rc(int r, int c, float val)
{
    ASSERTX(c>=0 && c<_nd && r>=0 && r<_n);
    _x[c][r]=val;
}

inline float LLS::get_x_rc(int r, int c)
{
    ASSERTX(c>=0 && c<_nd && r>=0 && r<_n);
    return _x[c][r];
}

#endif
