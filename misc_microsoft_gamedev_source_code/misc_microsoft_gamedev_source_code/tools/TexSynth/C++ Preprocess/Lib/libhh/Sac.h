// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Sac_h
#define Sac_h

#include "Pool.h"
#include "BSac.h"

#if 0
{
    class MVertex {
        ...;
        MAKE_SAC(MVertex);       // must be last entry of class!
    };
    SAC_INITIALIZATION(MVertex);
    
    SAC_ALLOCATE_FUNC(MVertex,Point,vpoint);
    vpoint(v)=Point(1,2,3); SHOW(vindex(v));

    struct A {
        Array<int> ar;
        SACABLE(A);             // Array requires destructor of A to be called
    };
    SAC_ALLOCATE_CD_FUNC(MVertex,A,fvA);
    fvA(v).ar+=1;
    
    static int key_a=SAC_ALLOCATE_CD(MVertex,A);
    sac_access(v,key_a,A).b=3;
}
#endif

template<class T>
class Sac : public BSac {
 public:
// static
    enum { MAX=50 };
    static int allocate(int s) {
        int k=size; size+=s; return k;
    }
    static int allocate(int s, SACFUNC fcons, SACFUNC fdest) {
        if (fcons) {
            assertx(cnum<MAX);
            ckeys[cnum]=size; cfuncs[cnum]=fcons;
            cnum++;
        }
        if (fdest) {
            assertx(dnum<MAX);
            dkeys[dnum]=size; dfuncs[dnum]=fdest;
            dnum++;
        }
        return allocate(s);
    }
    static int getsize() { return size; }
// per-object
    inline Sac() {
        if (cnum) callfuncs(cnum,ckeys,cfuncs);
    }
    inline ~Sac() {
        if (dnum) callfuncs(dnum,dkeys,dfuncs);
    }
 private:
    static int size;
    // Cannot use Arrays here because their constructors might not yet
    // be called at initialization time -> cannot use constructors!
    static int cnum;
    static int ckeys[MAX];
    static SACFUNC cfuncs[MAX];
    static int dnum;
    static int dkeys[MAX];
    static SACFUNC dfuncs[MAX];
};

template<class T>
class sac_template {
 public:
    inline static T& access(BSac& c, int k) { return *(T*)c.access(k); }
};

// Dummy Univ second argument to make it specific to my code.
inline void* operator new(size_t, BSac*, void* p) { return p; }

#define MAKE_SAC(T) \
void* operator new(size_t) \
{ return new char[sizeof(T)-SACDUMMY+Sac<T>::getsize()]; } \
void operator delete(void* p, size_t) \
{ delete p; } \
Sac<T> sac

#define MAKE_POOLED_SAC(T) \
Sac<T> sac; \
void* operator new(size_t) \
{ return pool.allocSize(sizeof(T)-SACDUMMY+Sac<T>::getsize()); } \
void operator delete(void* p, size_t) \
{ pool.freeSize(p,sizeof(T)-SACDUMMY+Sac<T>::getsize()); } \
POOL_ALLOCATION_3(T)

// size, cnum, and dnum intially zero
#define SAC_INITIALIZATION(T) \
int Sac<T>::size; \
int Sac<T>::cnum; \
int Sac<T>::ckeys[MAX]; \
SACFUNC Sac<T>::cfuncs[MAX]; \
int Sac<T>::dnum; \
int Sac<T>::dkeys[MAX]; \
SACFUNC Sac<T>::dfuncs[MAX]

#define POOLED_SAC_INITIALIZATION(T) \
Pool T::pool; \
int T::PoolInit::count; \
SAC_INITIALIZATION(T)

#define SACABLE(T) \
static void sac_construct(void* p) { (void)new((BSac*)0,p)T; } \
static void sac_destruct(void* p) { ((T*)p)->~T(); }

#define sac_access(ob,key,type) \
sac_template<type>::access((ob)->sac,key)

#define SAC_ALLOCATE_CD(sac,type) \
Sac<sac>::allocate(sizeof(type),&type::sac_construct, &type::sac_destruct)

#define SAC_ALLOCATE(sac,type) \
Sac<sac>::allocate(sizeof(type))

#define SAC_ALLOCATE_FUNC(sac,type,accessfunc) \
static int sac_k##accessfunc=SAC_ALLOCATE(sac,type); \
inline type& accessfunc(sac* e) \
{ return sac_access(e,sac_k##accessfunc,type); }

#define SAC_ALLOCATE_CD_FUNC(sac,type,accessfunc) \
static int sac_k##accessfunc=SAC_ALLOCATE_CD(sac,type); \
inline type& accessfunc(sac* e) \
{ return sac_access(e,sac_k##accessfunc,type); }

#define H_SAC_ALLOCATE_FUNC(sac,type,accessfunc) \
extern int sac_k##accessfunc; \
inline type& accessfunc(sac* e) \
{ return sac_access(e,sac_k##accessfunc,type); }

#define C_SAC_ALLOCATE_FUNC(sac,type,accessfunc) \
int sac_k##accessfunc=SAC_ALLOCATE(sac,type);

#endif
