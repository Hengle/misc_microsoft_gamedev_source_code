// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef VertexCache_h
#define VertexCache_h

#include "Pool.h"
#include "Array.h"
#include "EList.h"

class VertexCacheIter;

class VertexCache {
 public:
    enum Type { NOTYPE, FIFO, LRU };
    static const char* type_string(Type type);
    static VertexCache* create_new(Type type);
 public:
    VertexCache();
    virtual ~VertexCache();
    virtual Type type() const=0;
    virtual void init(int nverts1, int cs)=0; // vertex ids begin at 1
    virtual void copy(const VertexCache& vc)=0; // must be init'ed !
    // vi_replaced undefined if hits, and may be 0 if !hits and cache not full
    virtual bool access_hits(int vi)=0;
    virtual bool contains(int vi) const=0;
    virtual int location(int vi) const=0; // -1..cs-1 (0=recent) (may be slow)
    virtual int location_alt(int vi) const=0; // 0..cs (0=recent) (may be slow)
    virtual VertexCacheIter* iterator() const=0;
    void print_cerr() const;
 private:
    DISABLE_COPY(VertexCache);
};

class VertexCacheIter {         // ordering of vertices undefined!
 public:
    virtual ~VertexCacheIter();
    virtual int next()=0;       // return 0 if no more vertices
 private:
friend class FifoVertexCacheIter;
friend class LruVertexCacheIter;
    VertexCacheIter();
    DISABLE_COPY(VertexCacheIter);
};

//------------------------------------------------------------------------

class FifoVertexCache : public VertexCache {
 public:
    FifoVertexCache();
    ~FifoVertexCache();
    Type type() const;
    void init(int nverts1, int cs);
    void copy(const VertexCache& vc); // vc must be a FifoVertexCache!
    bool access_hits(int vi);
    bool contains(int vi) const;
    int location(int vi) const;
    int location_alt(int vi) const;
    VertexCacheIter* iterator() const;
 private:
friend class FifoVertexCacheIter;
    enum { NOENTRY=0 };         // value of empty entry in _queuev
    Array<int> _queuev;         // dec. circ. FIFO queue, may contain NOENTRY's
    int _iprev;                 // 0..cs-1: entry_last_added
    // _vinqueue[NOENTRY=0] is trash
    Array<int> _vinqueue;       // vid -> [0..cs-1, -1]
    DISABLE_COPY(FifoVertexCache);
};

class LruVertexCache : public VertexCache {
 public:
    LruVertexCache();
    ~LruVertexCache();
    Type type() const;
    void init(int nverts1, int cs);
    void copy(const VertexCache& vc); // vc must be a LruVertexCache!
    bool access_hits(int vi);
    bool contains(int vi) const;
    int location(int vi) const;
    int location_alt(int vi) const;
    VertexCacheIter* iterator() const;
 public:                        // should be private but uses Pool
    enum { NOENTRY=0 };         // value of empty entry in _list Node
    struct Node {
        POOL_ALLOCATION(Node);
        EListNode elist;
        int vert;               // NOENTRY if cache entry is empty
    };
 private:
friend class LruVertexCacheIter;
    int _cs;
    EList _list;
    // _vinlist[NOENTRY=0] is trash
    Array<Node*> _vinlist;
    DISABLE_COPY(LruVertexCache);
};

#endif
