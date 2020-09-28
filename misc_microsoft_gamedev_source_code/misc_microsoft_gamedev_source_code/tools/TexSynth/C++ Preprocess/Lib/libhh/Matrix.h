// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Matrix_h
#define Matrix_h

#include "Array.h"

template<class T>
class Matrix : public SArray< SArray<T> > {
 public:
    Matrix() : SArray< SArray<T> >(0) { }
    Matrix(int pm, int pn) : SArray< SArray<T> >(pm) {
        ForIndex(i,pm) { _a[i].init(pn); } EndFor;
    }
    ~Matrix() {}
    void init(int pm, int pn) {
        SArray< SArray<T> >::init(pm);
        ForIndex(i,pm) { _a[i].init(pn); } EndFor;
    };
    void fill(T e) { ForIndex(y,ysize()) { _a[y].fill(e); } EndFor; }
    void copy(const Matrix<T>& matrix);
    int ysize() const { return num(); }
    int xsize() const { ASSERTX(num()); return (*this)[0].num(); }
    // unnecessary: friend ostream& operator<<(ostream& s, const Matrix<T>& mat);
};

template<class T>
void Matrix<T>::copy(const Matrix<T>& matrix)
{
    init(matrix.ysize(),matrix.xsize());
    ForIndex(y,matrix.ysize()) {
        ForIndex(x,matrix.xsize()) {
            _a[y][x]=matrix[y][x];
        } EndFor;
    } EndFor;
}

template<class T1, class T2>
static void
subtract_matrices(Matrix<T1>& dmatrix,
                  const Matrix<T2>& omatrix, const Matrix<T2>& pmatrix)
{
    assertx(omatrix.ysize()==pmatrix.ysize());
    assertx(omatrix.xsize()==pmatrix.xsize());
    dmatrix.init(omatrix.ysize(),omatrix.xsize());
    ForIndex(y,dmatrix.ysize()) {
        ForIndex(x,dmatrix.xsize()) {
            T1 d=(T1)(omatrix[y][x]-pmatrix[y][x]);
#if !defined(NDEBUG)
            assertw1(pmatrix[y][x]+d==omatrix[y][x]);
#endif
            dmatrix[y][x]=d;
        } EndFor;
    } EndFor;
}

template<class T>
inline ostream& operator<<(ostream& s, const Matrix<T>& mat)
{
    s << hform("Matrix (%d,%d) {\n",mat.ysize(),mat.xsize());
    ForIndex(y,mat.ysize()) {
        s << " ";
        ForIndex(x,mat.xsize()) { s << " " << mat[y][x]; } EndFor;
        s << "\n";
    } EndFor;
    s << "}\n";
    return s;
}

#endif
