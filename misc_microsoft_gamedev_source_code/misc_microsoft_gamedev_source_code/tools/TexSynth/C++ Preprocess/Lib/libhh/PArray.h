// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef PArray_h
#define PArray_h

// for PArray<T>, like Array<T>, T must have a public operator=()
template<class T, int psize>
class PArray {
 public:
    PArray(int ne=0) {
        ASSERTX(ne>=0);
        if (ne<=psize) { _size=psize; _a=_pa; } else { _size=ne; _a=new T[_size]; }
        _n=ne;
    }
    PArray<T,psize>& operator=(const PArray<T,psize>& ar);
    PArray(const PArray<T,psize>& ar)   { _size=psize; _a=_pa; _n=0; *this=ar; }
    ~PArray()                           { if (_size!=psize) delete[] _a; }
    int num() const                     { return _n; }
    operator const T*() const           { return _a; }
    operator T*()                       { return _a; }
    int ok(int i) const                 { return i>=0 && i<_n; }
    int ok(const T* e) const            { return ok(safe_trunc_int(e-_a)); }
    const T& operator[](int i) const    { ASSERTX(i>=0 && i<_n); return _a[i]; }
    T& operator[](int i)                { ASSERTX(i>=0 && i<_n); return _a[i]; }
    const T& last() const               { return operator[](_n-1); }
    T& last()                           { return operator[](_n-1); }
    void clear() {
        if (_size!=psize) { delete[] _a; _size=psize; _a=_pa; }
        _n=0;
    }
    void init(int ne) {         // allocate ne, CLEAR if too small
        ASSERTX(ne>=0);
        if (unsigned(ne)>unsigned(_size)) { // unsigned comparison for fast init(0)
            if (_size!=psize) delete[] _a;
            _size=ne;
            _a=new T[_size];
        }
        _n=ne;
    }
    void fill(T e)                      { ForIndex(i,num()) { _a[i]=e; } EndFor; }
    void access(int e) {        // allocate at least e+1, COPY if too small
        ASSERTX(e>=0);
        int ne=e+1;
        if (ne>_size) resize_aux(ne);
        if (ne>_n) _n=ne;
    }
    void need(int ne) {         // allocate ne, COPY if too small
        ASSERTX(ne>=0);
        if (ne>_size) resize_aux(ne);
        _n=ne;
    }
    int add(int ne) {           // ret: previous num()
        ASSERTX(ne>=0);
        int cn=_n; need(_n+ne); return cn;
    }
    void sub(int ne) {
        _n-=ne;
        ASSERTX(ne>=0 && _n>=0);
    }
    PArray<T,psize>& operator+=(const T& e) { // do not use a+=a[..]!!
        if (_n+1<=_size) {                    // replicate code to optimize common case
            _a[_n++]=e; return *this;
        } else {
            resize_aux(_n+1);
            _a[_n++]=e; return *this;
        }
    }
    void squeeze()                      { if (_n<_size) resize(_n); } 
    void reverse()                      { ForIndex(i,_n/2) { swap(&_a[i],&_a[_n-1-i]); } EndFor; } 
    int contains(const T& e) const;
    int index(const T& e) const; // ret -1 if not contained
 private:
    T* _a;
    int _size;
    int _n;
    T _pa[psize];
 private:
    void resize(int nsize);
    void resize_aux(int ne);
    void operator=(int);           // disable
};

// was 'const T&', but with 'Cut*', expanded to 'const Cut*&' (bad)
#define ForPArray(A,T,V) ForIndex(zzz,(A).num()) T const& V=(A)[zzz];

//----------------------------------------------------------------------

template<class T, int psize>
PArray<T,psize>& PArray<T,psize>::operator=(const PArray<T,psize>& ar)
{
    if (&ar==this) return *this;
    init(ar.num());
    Array<T>::copy_array(_a,ar,_n);
    return *this;
}

template<class T, int psize>
void PArray<T,psize>::resize(int nsize)
{
    ASSERTX(_n<=nsize);
    if (nsize<=psize) {
        if (_size==psize) return;
        Array<T>::copy_array(_pa,_a,_n);
        delete[] _a;
        _a=_pa; _size=psize;
    } else {
        T* na=new T[nsize];
        Array<T>::copy_array(na,_a,_n);
        if (_size!=psize) delete[] _a;
        _a=na; _size=nsize;
    }
}

template<class T, int psize>
__declspec(noinline)
void PArray<T,psize>::resize_aux(int ne)
{
    resize(max(_n+int(_n/2)+3,ne));
}

template<class T, int psize>
int PArray<T,psize>::contains(const T& e) const
{
    ForIndex(i,_n) { if (_a[i]==e) return 1; } EndFor;
    return 0;
}

template<class T, int psize>
int PArray<T,psize>::index(const T& e) const
{
    ForIndex(i,_n) { if (_a[i]==e) return i; } EndFor;
    return -1;
}

#endif
