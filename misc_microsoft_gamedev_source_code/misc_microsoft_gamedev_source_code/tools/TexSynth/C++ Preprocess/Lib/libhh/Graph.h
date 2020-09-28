// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Graph_h
#define Graph_h

#include "Map.h"
#include "Stack.h"

#if 0
{
    Graph<int> g;
    ForGraphVertices(g,int,v1) {
        ForGraphEdges(g,int,v1,v2) {
            doedge(v1,v2);
        } EndFor;
    } EndFor;
}
#endif

// A BGraph represents a set of tuples.
// Each tuple consists of (Univ, Univ).
// The BGraph allows quick iteration over outgoing edges.
// Duplicate edges are not allowed.
// Graph should be used to encode a relation from a set of elements to itself.
class BGraph {
 public:
    BGraph();
    ~BGraph();
    void clear();
    // enter and remove domain vertices
    void enter(Univ v); // must be new
    int contains(Univ v) const;
    int remove(Univ v);         // must have 0 outdegree, ret: was_there
    // enter an edge
    void enter(Univ v1, Univ v2); // must be new (v1 must be present)
    // enter undirected edge
    void enteru(Univ v1, Univ v2); // must be new (v1 & v2 must be present)
    int contains(Univ v1, Univ v2) const; // O(n) slow
    int remove(Univ v1, Univ v2);         // O(n) , ret: was_there
    int removeu(Univ v1, Univ v2);        // O(n) , ret: was_there
    int outdegree(Univ v) const;
    void add(const BGraph& g);
 private:
friend class BGraphVertices;
friend class BGraphEdges;
    Map<Univ,Stack<Univ>*> _m;
    DISABLE_COPY(BGraph);
};

// Iterate over the first vertices of the edges
class BGraphVertices {
 public:
    BGraphVertices(const BGraph& g);
    ~BGraphVertices() { }
    operator void*() const;
    void next();
    Univ operator()() const;
 private:
    MapIter<Univ,Stack<Univ>*> _mi;
    DISABLE_COPY(BGraphVertices);
};

// Given a vertex, iterate over all outgoing edges
class BGraphEdges : public StackIter<Univ> {
 public:
    BGraphEdges(const BGraph& pg, Univ v);
    ~BGraphEdges() { }
};

//----------------------------------------------------------------------------

inline BGraphVertices::BGraphVertices(const BGraph& pg) : _mi(pg._m) { }
inline BGraphVertices::operator void*() const { return _mi; }
inline void BGraphVertices::next() { _mi.next(); }
inline Univ BGraphVertices::operator()() const { return _mi.key(); }

inline BGraphEdges::BGraphEdges(const BGraph& pg, Univ v)
: StackIter<Univ>(*pg._m.get(v)) { }

//----------------------------------------------------------------------------

template<class T>
class Graph : public BGraph {
 public:
    inline Graph() { }
    inline ~Graph() { }
    inline void enter(T v) { BGraph::enter(Conv<T>::e(v)); }
    inline int contains(T v) const { return BGraph::contains(Conv<T>::e(v)); }
    inline int remove(T v) { return BGraph::remove(Conv<T>::e(v)); }
    inline void enter(T v1, T v2)
    { BGraph::enter(Conv<T>::e(v1),Conv<T>::e(v2)); }
    inline void enteru(T v1, T v2)
    { BGraph::enteru(Conv<T>::e(v1),Conv<T>::e(v2)); }
    inline int contains(T v1, T v2) const
    { return BGraph::contains(Conv<T>::e(v1),Conv<T>::e(v2)); }
    inline int remove(T v1, T v2)
    { return BGraph::remove(Conv<T>::e(v1),Conv<T>::e(v2)); }
    inline int removeu(T v1, T v2)
    { return BGraph::removeu(Conv<T>::e(v1),Conv<T>::e(v2)); }
    inline int outdegree(T v) const
    { return BGraph::outdegree(Conv<T>::e(v)); }
    inline void add(const Graph<T>& g) { BGraph::add(g); }
};

template<class T>
class GraphVertices : public BGraphVertices {
 public:
    inline GraphVertices(const Graph<T>& g) : BGraphVertices(g) { }
    inline ~GraphVertices() { }
    inline T operator()() const
    { return Conv<T>::d(BGraphVertices::operator()()); }
};

template<class T>
class GraphEdges : public BGraphEdges {
 public:
    inline GraphEdges(const Graph<T>& g, T v)
        : BGraphEdges(g,Conv<T>::e(v)) { }
    inline ~GraphEdges() { }
    inline T next() { return Conv<T>::d(BGraphEdges::next()); }
    inline T operator()() const
    { return Conv<T>::d(BGraphEdges::operator()()); }
};

#define ForBGraphVertices(G,V) \
{ for (BGraphVertices zz(G);zz;zz.next()) { Univ V=zz();
#define DummyEndFor }}
#define ForBGraphEdges(G,V1,V2) \
{ for (BGraphEdges zz(G,V1);zz;zz.next()) { Univ V2=zz();
#define DummyEndFor }}

#define ForGraphVertices(G,T,V) \
{ for (GraphVertices< T > zz(G);zz;zz.next()) { T V=zz();
#define DummyEndFor }}
#define ForGraphEdges(G,T,V1,V2) \
{ for (GraphEdges< T > zz(G,V1);zz;zz.next()) { T V2=zz();
#define DummyEndFor }}

#endif
