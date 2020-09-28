// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Spatial_h
#define Spatial_h

#include "Geometry.h"
#include "Map.h"
#include "Stack.h"
#include "Set.h"
#include "Pqueue.h"

class Bbox;

class Spatial {                 // abstract class
 public:
    Spatial(int pgn);
    virtual ~Spatial();
    virtual void clear()=0;
 protected:
friend class BSpatialSearch;
    int _gn;                    // grid size
    float _gni;                 // 1/_gn
    //
    int indexinbounds(int i) const;
    int indicesinbounds(int ci[3]) const;
    int float2index(float fd) const;
    float index2float(int i) const;
    void point2indices(const Point& p, int ci[3]) const;
    void indices2point(int ci[3], Point& p) const;
    void indices2bbox(int ci[3], Bbox& bb) const;
    int encode(int ci[3]) const;
    void decode(int en, int ci[3]) const;
    // for BSpatialSearch:
    // Add elements from cell ci[3] to priority queue with priority equal
    // to distance from pcenter squared.  May use set to avoid duplication.
    virtual void addcell(int ci[3], Pqueue<Univ>& pq, const Point& pcenter,
                         Set<Univ>& set) const=0;
    // Refine distance estimate of first entry in pq (optional)
    virtual void pqrefine(Pqueue<Univ>& pq, const Point& pcenter) const;
    virtual Univ pqid(Univ pqe) const=0; // given pq entry, return id
 private:
    DISABLE_COPY(Spatial);
    DISABLE_ASSIGN_INT(Spatial);
};

class BPointSpatial : public Spatial {
 public:
    BPointSpatial(int pgn);
    ~BPointSpatial();
    void clear();
    // id!=0
    void enter(Univ id, const Point* pp); // note: pp not copied!
    void remove(Univ id, const Point* pp); // must exist, else die
 protected:
    void addcell(int ci[3], Pqueue<Univ>& pq, const Point& pcenter,
                 Set<Univ>& set) const;
    Univ pqid(Univ pqe) const;
 private:
    struct Node {
        Node(Univ pid, const Point* pp) : id(pid), p(pp) { }
        Univ id;
        const Point* p;
    };
    Map<int,Stack<Node*>*> _map; // encoded cube index -> Stack
};

class ObjectSpatial : public Spatial {
 public:
    typedef float (*DISTF)(const Point& p, Univ id);
    ObjectSpatial(int pgn, DISTF papproxf2, DISTF pexactf2);
    ~ObjectSpatial();
    void clear();
    // id!=0
    // Enter an object that comes with a containment function: the
    // function returns true if the object lies within a given bounding
    // box.  A starting point is also given.
    void enter(Univ id, const Point& startp,
               int (*fcontains)(const Bbox& bb));
    // Find the objects that could possibly intersect the segment (p1,p2).
    // The objects are not returned in the exact order of intersection!
    // However, once should_stop is set (ftest's return), the procedure
    // will keep calling ftest with all objects that could be closer.
    void searchsegment(const Point& p1, const Point& p2,
                       int (*ftest)(Univ id)) const;
 protected:
    void addcell(int ci[3], Pqueue<Univ>& pq, const Point& pcenter,
                 Set<Univ>& set) const;
    void pqrefine(Pqueue<Univ>& pq, const Point& pcenter) const;
    Univ pqid(Univ pqe) const;
 private:
    Map<int,Stack<Univ>*> _map; // encoded cube index -> Stack
    DISTF _approxf2;
    DISTF _exactf2;
};

// Search from a point.
class BSpatialSearch {
 public:
    // pmaxdis is only a request, you may get objects that lie farther
    BSpatialSearch(const Spatial& psp, const Point& pp,
                   float pmaxdis=10.f);
    ~BSpatialSearch();
    int done();
    Univ next(float* dis2=0);   // ret id
 private:
friend class Spatial;
    const Spatial& _sp;
    const Point _pcenter;
    float _maxdis;
    Pqueue<Univ> _pq;           // pq of entries by distance
    int _ssi[2][3];             // search space indices (extents)
    float _disbv2;              // distance to search space boundary
    int _axis;                  // axis to expand next
    int _dir;                   // direction in which to expand next (0,1)
    Set<Univ> _setevis;         // may be used by addcell()
    int _ncellsv;
    int _nelemsv;
    //
    void getclosestnextcell();
    void expandsearchspace();
    void consider(int ci[3]);
    DISABLE_COPY(BSpatialSearch);
};

//------------------------------------------------------------------------

template<class T>
class PointSpatial : public BPointSpatial {
 public:
    PointSpatial(int pgn) : BPointSpatial(pgn) { }
    ~PointSpatial() { }
    void enter(T id, const Point* pp)
    { BPointSpatial::enter(Conv<T>::e(id),pp); }
    void remove(T id, const Point* pp)
    { BPointSpatial::remove(Conv<T>::e(id),pp); }
};

template<class T>
class SpatialSearch : public BSpatialSearch {
 public:
    SpatialSearch(const Spatial& psp, const Point& pp,
                  float pmaxdis=10.f) : BSpatialSearch(psp,pp,pmaxdis) { }
    ~SpatialSearch() { }
    T next(float* dis2=0) { return Conv<T>::d(BSpatialSearch::next(dis2)); }
};

#endif
