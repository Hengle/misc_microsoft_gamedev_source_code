// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Array_h
#define Array_h

#if 0
{
    static Array<Point> ar; ar.init(np);
    Array<Point> ar; ar+=Point(1,2,3);
}
#endif

// SArray<T> supports classes that have no public operator=()
template<class T>
class SArray {
 public:
    SArray(int size=0)                  { ASSERTX(size>=0); _size=size; _a=_size?new T[_size]:0; }
    ~SArray()                           { delete[] _a; }
    int num() const                     { return _size; }
    operator const T*() const           { return _a; }
    operator T*()                       { return _a; }
    int ok(int i) const                 { return i>=0 && i<_size; }
    int ok(const T* e) const            { return ok(safe_trunc_int(e-_a)); }
    const T& operator[](int i) const    { ASSERTX(i>=0 && i<_size); return _a[i]; }
    T& operator[](int i)                { ASSERTX(i>=0 && i<_size); return _a[i]; }
    const T& last() const               { return operator[](_size-1); }
    T& last()                           { return operator[](_size-1); }
    void init(int ne) {
        ASSERTX(ne>=0);
        if (ne!=_size) { _size=ne; delete[] _a,_a=_size?new T[_size]:0; }
    }
    void fill(T e)                      { ForIndex(i,num()) { _a[i]=e; } EndFor; }
    void clear()                        { init(0); }
 protected:
    T* _a;
    int _size;
 private:
    void operator=(SArray<T>&) { } // disable
    SArray(SArray<T>&) { }         // disable
    void operator=(int);           // disable
};

// for Array<T>, T must have a public operator=()
template<class T>
class Array {
 public:
    Array(int ne=0)                     { ASSERTX(ne>=0); _size=ne; _a=_size?new T[_size]:0; _n=ne; }
    Array<T>& operator=(const Array<T>& ar);
    Array(const Array<T>& ar)           { _size=0; _a=0; _n=0; *this=ar; }
    ~Array()                            { delete[] _a; }
    int num() const                     { return _n; }
    operator const T*() const           { return _a; }
    operator T*()                       { return _a; }
    int ok(int i) const                 { return i>=0 && i<_n; }
    int ok(const T* e) const            { return ok(safe_trunc_int(e-_a)); }
    const T& operator[](int i) const    { ASSERTX(i>=0 && i<_n); return _a[i]; }
    T& operator[](int i)                { ASSERTX(i>=0 && i<_n); return _a[i]; }
    const T& last() const               { return operator[](_n-1); }
    T& last()                           { return operator[](_n-1); }
    void clear()                        { delete[] _a,_a=0; _n=_size=0; }
    void init(int ne) {         // allocate ne, CLEAR if too small
        ASSERTX(ne>=0);
        if (unsigned(ne)>unsigned(_size)) { // unsigned comparison for fast init(0)
            delete[] _a,_a=new T[ne],_size=ne;
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
    Array<T>& operator+=(const T& e) { // do not use a+=a[..]!!
        if (_n+1<=_size) {             // replicate code to optimize common case
            _a[_n++]=e; return *this;
        } else {
            resize_aux(_n+1);
            _a[_n++]=e; return *this;
        }
    }
    void squeeze()                      { if (_n<_size) resize(_n); }
    void preallocate(int s)             { ASSERTX(s>=0); if (_n<s) { resize(s); } else { assertx(_n==s); } }
    void reverse()                      { ForIndex(i,_n/2) { swap(&_a[i],&_a[_n-1-i]); } EndFor; }
    int contains(const T& e) const;
    int index(const T& e) const; // ret -1 if not contained
    void append(const Array<T>& ar);
    // unnecessary: friend ostream& operator<<(ostream& s, const Array<T>& ar);
 private:
    T* _a;
    int _size;
    int _n;
 public:                        // to allow ARRAY_BIT_COPY(T)
    static void copy_raw_bits(T* na, const T* oa, int n);
    static void copy_array(T* na, const T* oa, int n);
 private:
    void resize(int nsize);
    void resize_aux(int ne);
    void operator=(int);           // disable
};

// was 'const T&', but with 'Cut*', expanded to 'const Cut*&' (bad)
#define ForArray(A,T,V) ForIndex(zzz,(A).num()) T const& V=(A)[zzz];

//----------------------------------------------------------------------

template<class T>
void Array<T>::copy_raw_bits(T* na, const T* oa, int n)
{
    // Note: does not call T::operator=(), so only use on non-pointer types.
    if (n) memcpy((void*)na,(void*)oa,n*sizeof(T));
}

template<class T>
void Array<T>::copy_array(T* na, const T* oa, int n)
{
    ForIndex(i,n) { na[i]=oa[i]; } EndFor;
}

#define ARRAY_BIT_COPY(T) \
inline void Array< T >::copy_array(T* na, const T* oa, int n) \
{ copy_raw_bits(na,oa,n); }

ARRAY_BIT_COPY(int);
ARRAY_BIT_COPY(unsigned);
ARRAY_BIT_COPY(short);
ARRAY_BIT_COPY(ushort);
ARRAY_BIT_COPY(char);
ARRAY_BIT_COPY(uchar);
ARRAY_BIT_COPY(float);
ARRAY_BIT_COPY(double);
ARRAY_BIT_COPY(bool);

// It would be nice to specialize copy_array for all T of type pointer:
//  but I cannot find a way to do this.
// template<class T*>
// void Array<T*>::copy_array(T** na, const T** oa, int n) \
// { copy_raw_bits(na,oa,n); }

//------------------------------------------------------------------------

template<class T>
Array<T>& Array<T>::operator=(const Array<T>& ar)
{
    if (&ar==this) return *this;
    init(ar.num());
    copy_array(_a,ar._a,_n);
    return *this;
}

template<class T>
void Array<T>::resize(int nsize)
{
    _size=nsize;
    ASSERTX(_n<=_size);
    T* na=_size?new T[_size]:0;
    copy_array(na,_a,_n);
    delete[] _a,_a=na;
}

template<class T>
__declspec(noinline)
void Array<T>::resize_aux(int ne)
{
    resize(max(_n+int(_n/2)+3,ne));
}

template<class T>
int Array<T>::contains(const T& e) const
{
    ForIndex(i,_n) { if (_a[i]==e) return 1; } EndFor;
    return 0;
}

template<class T>
int Array<T>::index(const T& e) const
{
    ForIndex(i,_n) { if (_a[i]==e) return i; } EndFor;
    return -1;
}


template<class T>
void Array<T>::append(const Array<T>& ar)
{
    int base=add(ar.num());
    ForIndex(i,ar.num()) { _a[base+i]=ar[i]; } EndFor;
}

template<class T>
inline ostream& operator<<(ostream& s, const Array<T>& ar)
{
    s << hform("Array (%d) {\n",ar.num());
    ForIndex(i,ar.num()) {
        s << "  " << ar[i] << "\n";
    } EndFor;
    s << "}\n";
    return s;
}

#endif
