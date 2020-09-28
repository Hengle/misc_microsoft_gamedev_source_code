// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Pool_h
#define Pool_h

// See notes in Pool.cxx

// See also Sac.h which defined MAKE_POOLED_SAC()!

#define POOL_ALLOCATION(T) POOL_ALLOCATION_ACTIVE(T)

#define POOL_ALLOCATION_ACTIVE(T) \
POOL_ALLOCATION_1(T) \
POOL_ALLOCATION_2(T)

#define POOL_ALLOCATION_1(T) \
void* operator new(size_t s) \
{ return s==sizeof(T)?pool.alloc():pool.specialalloc(s); } \
void* operator new(size_t, void* p) \
{ return p; } \
void operator delete(void* p, size_t s) \
{ if (s==sizeof(T)) pool.free(p); else pool.specialfree(p,s); }

#define POOL_ALLOCATION_2(T) \
static Pool pool; \
struct PoolInit { \
    inline PoolInit() { if (!count++) pool.construct(sizeof(T),#T); } \
    inline ~PoolInit() { if (!--count) pool.destroy(); } \
    static int count; \
}

#define POOL_ALLOCATION_3(T) \
static Pool pool; \
struct PoolInit { \
    inline PoolInit() { if (!count++) pool.construct(0,#T); } \
    inline ~PoolInit() { if (!--count) pool.destroy(); } \
    static int count; \
}

#define INITIALIZE_POOL(T) INITIALIZE_POOL_ACTIVE(T)

#define INITIALIZE_POOL_ACTIVE(T) INITIALIZE_POOL_NESTED(T,T)

#define INITIALIZE_POOL_NESTED(T,name) \
static T::PoolInit poolInit_##name;

#define ALLOCATE_POOL(T) \
Pool T::pool; \
int T::PoolInit::count

class Pool {
 public:
    Pool();
    ~Pool();
    void construct(unsigned esize, const char* name);
    void destroy();
    // allocate based on static size of class
    void* alloc();
    void free(void* p);
    // allocate based on size of first allocSize() call
    void* allocSize(size_t s);
    void freeSize(void* p, size_t s);
    // hook for malloc()
    void* specialalloc(size_t s);
    void specialfree(void* p, size_t s);
 private:
    struct Link { Link* next; };
    struct Chunk { Chunk* next; };
    int _esize;
    const char* _name;
    Link* _h;
    Chunk* _chunkh;
    int _nalloc;
    int _offset;
    //
    void init();
    void grow();
    void growSize(int size);
    DISABLE_COPY(Pool);
};

//----------------------------------------------------------------------

inline void* Pool::alloc()
{
    if (!_h) grow();
    Link* p=_h; _h=p->next; return p;
}

inline void Pool::free(void* pp)
{
    Link* p=(Link*)pp;
    p->next=_h; _h=p;
}

inline void* Pool::allocSize(size_t s64)
{
    int s=safe_trunc_int(s64);
    if (!_h) growSize(s);
    Link* p=_h; _h=p->next; return p;
}

inline void Pool::freeSize(void* pp, size_t s64)
{
    int s=safe_trunc_int(s64);
    if ((int)s<=_esize) {
        // Pool::free(pp);
        Link* p=(Link*)pp; p->next=_h; _h=p;
    } else { specialfree(pp,s); }
}

#endif
