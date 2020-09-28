// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef MeshOp_h
#define MeshOp_h

#include "GMesh.h"
#include "Stat.h"
#include "Set.h"
#include "Queue.h"
#include "Map.h"

// *** Misc

extern void GatherBoundary(const Mesh& mesh, Edge e, Queue<Edge>& queuee);

// Face-Face connected
extern void GatherComponent(const Mesh& mesh, Face f, Set<Face>& setf);

// Connected through vertices (at least as large as GatherComponent)
extern void GatherComponentV(const Mesh& mesh, Face f, Set<Face>& setf);

extern void MeshStatComponents(const Mesh& mesh, Stat& Scompf);
extern void MeshStatBoundaries(const Mesh& mesh, Stat& Sbound);

// ret: genus value (accounting for number of components and boundaries)
extern float MeshGenus(const Mesh& mesh);

// ret: string terminated with "\n"
extern const char* MeshGenusString(const Mesh& mesh);

// For faces with >3 sides, find a good triangulation of the vertices.
// Return: success (may fail if some edges already exist).
extern int TriangulateFace(GMesh& mesh, Face f);

// ret: cosf of signed angle away from "flattness" (==exterior angle)
// range -1..1  (or -2 if a triangle is degenerate)
// For non-triangles, looks at average of immediate neighbors on either side.
extern float EdgeDihedralAngleCos(const GMesh& mesh, Edge e);

// must be a nice interior vertex
extern float VertexSolidAngle(const GMesh& mesh, Vertex v);

// Return a criterion given an edge e that is low if the edge should be
// collapsed.  Use the product of the edge length with the smallest inscribed
// radius of the two adjacent faces (its dimension is area).
extern float CollapseEdgeHackCriterion(const GMesh& mesh, Edge e);

// Return change in volume, but penalize bad dihedral angles.
extern float CollapseEdgeVolumeCriterion(const GMesh& mesh, Edge e);

// Return memoryless QEM and penalize bad dihedral angles.
extern float CollapseEdgeQemCriterion(const GMesh& mesh, Edge e);

extern void MeshRemoveBoundary(Mesh& mesh, Edge erep, Set<Face>& ret_setf);
             
// *** Retriangulate

typedef int (*EDGEF)(const GMesh& m, Edge e);

// For all Mesh Edge e,
//  if dihedral angle cosf of faces both before and after is >mincos,
//   and if (fdoswap(e)) then
//    call fdel(e), swap the edge, and call fadd(newedge).
// Consider all affect edges again.  Return number of edges swapped.
extern int RetriangulateAll(GMesh& mesh, float mincos,
                            EDGEF fdoswap, EDGEF fdel=0, EDGEF fadd=0);

// Consider only Edge e and recursively, all affected edges.
// e cannot be boundary edge!
extern int RetriangulateFromEdge(GMesh& mesh, Edge e, float mincos,
                                 EDGEF fdoswap, EDGEF fdel=0, EDGEF fadd=0);

// Consider swapping Edge e.  Return new_edge or 0 if not swapped.
// e cannot be boundary edge!
extern int RetriangulateOneEdge(GMesh& mesh, Edge e, float mincos,
                                EDGEF fdoswap, EDGEF fdel=0, EDGEF fadd=0);

extern int CircumRadiusSwapCrit(const GMesh& mesh, Edge e);
extern int DiagonalDistanceSwapCrit(const GMesh& mesh, Edge e);

// *** Normal estimation

class Vnors {
 public:
    Vnors();
    ~Vnors();
    int is_unique() const;
    const Vector& unique_nor() const; //  if is_unique()
    const Vector& face_nor(Face f) const; // if !is_unique()
    const Vector& get_nor(Face f) const;  // in any case
    void clear();
    enum NorType { DEFAULT_NOR, ANGLE_NOR, SUM_NOR, AREA_NOR,
                   SLOAN_NOR, SUBDIV_NOR };
 private:
    friend int ComputeVnors(const GMesh& mesh, Vertex v,
                            Vnors& vnors,
                            Vnors::NorType nortype);
    Map<Face,Vector*>* _mfnor;  // if zero, common normal stored in _nor
    Vector _nor;
    DISABLE_COPY(Vnors);
};

// Compute the normal(s) of the faces around vertex v.
// Overwrite given vnors with the information.
// Normals are obtained as follows:
//  DEFAULT_NOR: look at environment variables to pick scheme
//  ANGLE_NOR: sum normals of adjacent faces, weighted by subtended angle
//  SUM_NOR: sum normals of adjacent faces
//  AREA_NOR: sum normals of adjacent faces, weighted by area of faces
//  SLOAN_NOR: sum normals of adjacent faces, weighted by inverse squared area
//  SUBDIV_NOR: from the limit surface of the SIG94 piecewise smooth scheme
// If string(v) contains normal information, use that instead.
// Returns the number of unique normals computed.
extern int ComputeVnors(const GMesh& mesh, Vertex v, Vnors& vnors,
                        Vnors::NorType nortype=Vnors::DEFAULT_NOR);

// *** Projection onto mesh

// If fast!=0 and point p projects within interior of face and edges of face
// are not sharp, do not consider neighboring faces.
extern float
ProjectPNeighb(const GMesh& mesh, const Point& p, Face& pf,
               Bary& ret_bary, Point& ret_clp, int fast);

//----------------------------------------------------------------------

inline int Vnors::is_unique() const { return !_mfnor; }
inline const Vector& Vnors::unique_nor() const { return _nor; }
inline const Vector& Vnors::face_nor(Face f) const { return *_mfnor->get(f); }
inline const Vector& Vnors::get_nor(Face f) const {
    if (_mfnor) return *_mfnor->get(f); else return _nor;
}

#endif
