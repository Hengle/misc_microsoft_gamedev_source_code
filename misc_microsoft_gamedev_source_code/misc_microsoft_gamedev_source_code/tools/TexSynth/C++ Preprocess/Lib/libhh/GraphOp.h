// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef GraphOp_h
#define GraphOp_h

#include "Graph.h"
#include "Pqueue.h"
#include "Set.h"

#include "Geometry.h"           // Point pa[] requires size

template<class T> class PointSpatial;
class Stat;

extern void GraphSymmetricClosure(BGraph& g);

// Return vertices in order of increasing graph distance from vs.
// Vertex vs itself is returned on first invocation of next().
// Graph may be directed.
class Dijkstra {
 public:
    Dijkstra(const BGraph& pg, Univ vs,
             float (*pfdist)(Univ v1, Univ v2));
    ~Dijkstra();
    int done();
    Univ next(float& dis);      // ret vertex, or die
 private:
    const BGraph& _g;
    float (*_fdist)(Univ v1, Univ v2);
    HPqueue<Univ> _pq;
    Set<Univ> _set;
    DISABLE_COPY(Dijkstra);
};

// Given a graph gnew consisting solely of vertices,
// computes the MST of undirectedg over the vertices in gnew under the cost
// metric fdist.
// Returns is_connected.
// Implementation: Kruskal's algorithm, O(e log(e))
//  Prim's algorithm is recommended when e=~n^2
extern int GraphMst(const BGraph& undirectedg,
                    float (*fdist)(Univ v1, Univ v2),
                    BGraph& gnew);

// Returns a newly allocated undirected graph that is the MST of undirectedg.
// Returns 0 if g is not connected.
extern BGraph* newGraphMst(const BGraph& undirectedg,
                           float (*fdist)(Univ v1, Univ v2));

// Returns a newly allocated undirected graph that is the minimum spanning
// tree of the full graph between the num points, where the cost metric
// between two points v1 and v2 is fdist(v1,v2).
// Implementation: Prim's algorithm, O(n^2)!!
extern Graph<int>* newGraphMst(int num, float (*fdist)(int v1, int v2));

// Same as GraphMst() but works specifically on an array of points and tries
// to do it more quickly by making use of a spatial data structure.
// Implementation: Prim's MST on series of subgraphs.
extern Graph<int>* newGraphQuickEmst(const Point pa[], int num,
                                     const PointSpatial<int>& sp);

// Return statistics about graph edge lengths.
// If undirected, edges stats are duplicated.
extern Stat* newGraphEdgeStats(const BGraph& g,
                               float (*fdist)(Univ v1, Univ v2));

// Returns a newly allocated directed graph that connects each vertex to its
// kcl closest neighbors (based on Euclidean distance).
// Consider applying GraphSymmetricClosure() !
extern Graph<int>* newGraphEKClosest(const Point pa[], int num, int kcl,
                                     const PointSpatial<int>& sp);

// Access each connected component of a graph.
// next() returns a representative vertex of each component.
class GraphComponent {
 public:
    GraphComponent(const BGraph& g);
    ~GraphComponent();
    operator void*() const;
    void next();
    Univ operator()() const;
 private:
    const BGraph& _g;
    BGraphVertices _gv;
    Set<Univ> _set;
    DISABLE_COPY(GraphComponent);
};

extern int GraphNumComponents(const BGraph& g);

// Color a graph (heuristically since optimal is NP-hard).
// Assign colors (starting with 1).
// Returns number colors assigned.
extern int GraphColor(const BGraph& graph, Map<Univ,int>& ret_colors);

//----------------------------------------------------------------------------

inline GraphComponent::GraphComponent(const BGraph& g) : _g(g), _gv(_g) { }
inline GraphComponent::~GraphComponent() { }
inline GraphComponent::operator void*() const { return _gv; }
inline Univ GraphComponent::operator()() const { return _gv(); }

#endif
