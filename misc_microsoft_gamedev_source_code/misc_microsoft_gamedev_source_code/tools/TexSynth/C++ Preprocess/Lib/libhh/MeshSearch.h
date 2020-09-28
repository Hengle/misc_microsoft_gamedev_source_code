// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef MeshSearch_h
#define MeshSearch_h

#include "GMesh.h"
class PolygonSpatial;

#if 0
{
    MeshSearch msearch(mesh);
    msearch.allow_local_project(true);
    NEST { TIMER(_spatial_create); msearch.build_spatial(); }
    Bary bary; Point clp; float d2;
    Face f=msearch(p,bary,clp,d2);
}
#endif

class MeshSearch {
 public:
    MeshSearch(const GMesh& pmesh);
    ~MeshSearch();
    void allow_local_project(bool b);
    void allow_internal_boundaries(bool b);
    void allow_off_surface(bool b);
    void build_spatial();
    Face search(const Point& p, Bary& bary, Point& clp, float& d2);
    const GMesh& mesh() const { return _mesh; }
 private:
    const GMesh& _mesh;
    PolygonSpatial* _ppsp;
    Map<Polygon*,Face> _mpf;
    Frame _ftospatial;
    bool _allow_local_project;
    bool _allow_internal_boundaries;
    bool _allow_off_surface;
    Face _lastf;
    void clear();
};

//----------------------------------------------------------------------------

#endif
