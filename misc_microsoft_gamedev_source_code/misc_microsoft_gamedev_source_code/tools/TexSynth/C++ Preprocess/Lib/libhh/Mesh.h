// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Mesh_h
#define Mesh_h

#include "Map.h"
#include "Stack.h"
#include "Pqueue.h"
#include "Geometry.h"           // because of Point, too bad.
#include "Array.h"
#include "Pool.h"
#include "Sac.h"

class Random;
class MHEdge; typedef MHEdge* HEdge; // private

class MVertex; typedef MVertex* Vertex;
class MFace; typedef MFace* Face;
class MEdge; typedef MEdge* Edge;
typedef MHEdge MCorner; typedef MCorner* Corner;

// Mesh: a set of Vertices, Faces, and Edges and their topological relations.
// Properties:
//   (1a) - vertices appear at most once per face
//   (1b) - an oriented edge (2 consecutive vertices of a face) appears
//           in at most one face; hence the mesh is orientable.
// A Mesh must always satisfy (1a) and (1b); such a mesh is called "legal".
//
//   (2a) - vertices are nice (contain at most 1 (possibly partial) face ring)
//   (2b) - faces are nice: (a,b,c) implies no (a,c,b)
// A Mesh is "nice" if it also satisfies (2a) and (2b);
//  basically, if it is everywhere a 2D manifold.
//
//   (3a) - all faces are triangular
// A Mesh is a "nice triangular mesh" if in addition it satisfies (3a).
//
// MVertex allocates space for Point, which is used later in GMesh
// MVertex, MFace, MEdge allocate space for string, also used in GMesh

class Mesh {
 public:
    Mesh();
    virtual ~Mesh();
    void clear();
    // copy carries flags (but not sac fields)
    void copy(const Mesh& m);   // must be empty; (does not take a GMesh!)
// Raw manipulation functions, may lead to non-nice Meshes.
    // always legal
    Vertex createVertex();
    // die if degree(v)>0
    virtual void destroyVertex(Vertex v);
    // ret 0 if duplicate vertices or if existing edge
    int legalCreateFace(const Array<Vertex>& va) const;
    // die if !legalCreateFace()
    Face createFace(const Array<Vertex>& va);
    Face createFace(Vertex v1, Vertex v2, Vertex v3); // convenience function
    // always legal
    virtual void destroyFace(Face f);
// Vertex
    int isNice(Vertex v) const;
    int degree(Vertex v) const; // == number of adjacent vertices/edges
    int numBoundaries(Vertex v) const; // 0/1 for a nice vertex
    int isBoundary(Vertex v) const;    // isNice(v), degree(v)>0
    Edge oppEdge(Vertex v, Face f) const; // isTriangle(f); slow
    Vertex oppVertex(Vertex v, Edge e) const;
    // all mostClw and mostCcw assert isNice(v)
    // move about vertices adjacent to a vertex
    Vertex mostClwVertex(Vertex v) const; // if !bnd,ret any; may ret 0
    Vertex mostCcwVertex(Vertex v) const; // if !bnd,ret any; may ret 0
    Vertex clwVertex(Vertex v, Vertex vext) const; // slow; may ret 0
    Vertex ccwVertex(Vertex v, Vertex vext) const; // slow; may ret 0
    // move about faces adjacent to a vertex
    Face mostClwFace(Vertex v) const; // if !bnd,ret any; may ret 0
    Face mostCcwFace(Vertex v) const; // if !bnd,ret any; may ret 0
    Face clwFace(Vertex v, Face f) const; // slow; may ret 0
    Face ccwFace(Vertex v, Face f) const; // slow; may ret 0
    // move about edges adjacent to a vertex
    Edge mostClwEdge(Vertex v) const; // if !bnd,ret any; may ret 0
    Edge mostCcwEdge(Vertex v) const; // if !bnd,ret any; may ret 0
    Edge clwEdge(Vertex v, Edge e) const; // may ret 0
    Edge ccwEdge(Vertex v, Edge e) const; // may ret 0
    // get face relative to vertex
    Face ccwFace(Vertex v, Edge e) const; // may ret 0
    Face clwFace(Vertex v, Edge e) const; // may ret 0
// Face
    int isNice(Face f) const;
    int numVertices(Face f) const;
    int isTriangle(Face f) const;
    int isBoundary(Face f) const; // == has a boundary vertex
    Face oppFace(Face f, Edge e) const; // ret 0 if isBoundary(e)
    Face oppFace(Vertex v, Face f) const; // isTriangle(f); ret 0 if none
    // ccw order
    void vertices(Face f, Array<Vertex>& va) const;
    void corners(Face f, Array<Corner>& ca) const;
    // move about a face
    Edge clwEdge(Face f, Edge e) const;
    Edge ccwEdge(Face f, Edge e) const;
    Vertex clwVertex(Face f, Vertex v) const; // slow
    Vertex ccwVertex(Face f, Vertex v) const; // slow
    Edge clwEdge(Face f, Vertex v) const;     // slow
    Edge ccwEdge(Face f, Vertex v) const;     // slow
// Edge
    int isBoundary(Edge e) const;
    Vertex vertex1(Edge e) const;
    Vertex vertex2(Edge e) const;
    Face face1(Edge e) const;
    Face face2(Edge e) const;   // ret 0 if isBoundary(e)
    Vertex sideVertex1(Edge e) const; // isTriangle(face1())
    Vertex sideVertex2(Edge e) const; // isTriangle(face2()); 0 if isBoundary
    Vertex oppVertex(Edge e, Face f) const; // isTriangle(f)
    Edge oppBoundary(Edge e, Vertex v) const; // isBoundary(e)
    Edge clwBoundary(Edge e) const;           // isBoundary(e)
    Edge ccwBoundary(Edge e) const;           // isBoundary(e)
// Corner
    Corner corner(Vertex v, Face f) const;
    Vertex corner_vertex(Corner c) const;
    Face corner_face(Corner c) const;
    Corner ccwCorner(Corner c) const; // around vertex
    Corner clwCorner(Corner c) const; // around vertex
    Corner ccwFaceCorner(Corner c) const; // around face
    Corner clwFaceCorner(Corner c) const; // around face
    Corner ccwCorner(Vertex v, Edge e) const;
    Corner clwCorner(Vertex v, Edge e) const;
// Other associations
    // obtain edge from vertices
    Edge queryEdge(Vertex v, Vertex w) const;
    Edge edge(Vertex v, Vertex w) const; // asserts exists
    Edge orderedEdge(Vertex v1, Vertex v2) const; // asserts exists, oriented
    // get face from 2 consecutive vertices (ccw order)
    Face face(Vertex v, Vertex vccw) const; // may ret 0
// Counting routines (fast)
    int numVertices() const;
    int numFaces() const;
    int numEdges() const;
// Random access (fast), assert there exist at least one
    Vertex randomVertex(Random& r) const;
    Face randomFace(Random& r) const;
    Edge randomEdge(Random& r) const; // unbiased for a closed triangle mesh
// Flags
    enum { ALL=~0 };            // all flags
    static int allocateFlag() { assertx(_s_flag<31); return 1<<(_s_flag++); }
    // get selected flag(s)
    int gflag(int flagmask) const; // flag for whole mesh
    int flag(Vertex v, int flagmask) const;
    int flag(Face f, int flagmask) const;
    int flag(Edge e, int flagmask) const;
    // set or reset selected flag(s), return previous state of flag(s)
    int gmodFlag(int flagmask, int pbool);
    int modFlag(Vertex v, int flagmask, int pbool);
    int modFlag(Face f, int flagmask, int pbool);
    int modFlag(Edge e, int flagmask, int pbool);
// Triangular mesh operations (die if not triangular!)
    // would collapse be legal?
    int legalEdgeCollapse(Edge e) const;
    // would collapse preserve a nice mesh?
    int niceEdgeCollapse(Edge e) const;
    // would edge swap be legal?  (legal implies nice here)
    int legalEdgeSwap(Edge e) const;
    //
    virtual void collapseEdgeVertex(Edge e, Vertex vs);
    // die if !legalEdgeCollapse(e)
    // remove f1,[f2], v2, (v2,{*})
    // add (v1,{**})  where {**}={*}-{v1,vo1,vo2}
    virtual void collapseEdge(Edge e);
    // splitEdge(e) always legal
    // remove f1,[f2], (v1,v2)
    // add 2/4 faces, vnew, (vnew,v1), (vnew,v2), (vnew,vo1), [(vnew,vo2)]
    virtual Vertex splitEdge(Edge e, int vid=0);
    // die if !legalEdgeSwap(e)
    // remove f1,f2, (v1,v2)
    // add 2 faces, (vo1,vo2)
    virtual Edge swapEdge(Edge e);
// More mesh operations
    // vs2 can be 0, returns v2, leaves hole
    virtual Vertex splitVertex(Vertex v1, Vertex vs1, Vertex vs2, int v2i);
    // introduce one center vertex and triangulate face
    virtual Vertex center_split_face(Face f); // always legal
    // introduce an edge to split face on (v1,v2)
    virtual Edge split_face(Face f, Vertex v1, Vertex v2);
    // remove the consecutive set of edges separating two faces
    //  (may destroy some vertices if >1 edge shared by two faces)
    int legal_coalesce_faces(Edge e);
    virtual Face coalesce_faces(Edge e); // die if !legal
    virtual Vertex insert_vertex_on_edge(Edge e);
    virtual Edge remove_vertex_between_edges(Vertex vr);
// Mesh
    Vertex idvertex(int i) const;
    int vertexid(Vertex v) const;
    Face idface(int i) const;
    int faceid(Face f) const;
    Vertex id_retrieve_vertex(int i) const;
    Face id_retrieve_face(int i) const;
    int isNice() const;
    void renumber();            // renumber vertices and faces
// Misc
    void OK() const;            // die if problem
    bool valid(Vertex v) const; // die if invalid
    bool valid(Face f) const;   // die if invalid
    bool valid(Edge e) const;   // die if invalid
    bool valid(Corner c) const; // die if invalid
 public:                        // Discouraged:
    // next 2 die if index is already used
    virtual Vertex createVertexI(int id);
    virtual Face createFaceI(int id, const Array<Vertex>& va);
 public:                        // Should be private
    const Map<int,Vertex>& fastiter_vertices() const { return _id2vertex; }
    const Map<int,Face>& fastiter_faces() const { return _id2face; }
 protected:
    static int sdebug;          // 0=no, 1=min, 2=max
 public:                        // should be private
    HEdge mostClwHEdge(Vertex v) const; // isNice(v), may ret 0
    HEdge mostCcwHEdge(Vertex v) const; // isNice(v), may ret 0
    HEdge clwHEdge(HEdge e) const;       // may ret 0
    HEdge ccwHEdge(HEdge e) const;       // may ret 0
 public:
    void vertex_renumber_id_private(Vertex v, int newid); // for Pedro
    void face_renumber_id_private(Face v, int newid); // for Pedro
 private:
    int _flags;
    Map<int,Vertex> _id2vertex; // also act as set of vertices and faces
    Map<int,Face> _id2face;
    int _vertexnum;             // id to assign to next new vertex
    int _facenum;               // id to assign to next new face
    int _nedges;
    //
    static int _s_flag;
    //
    HEdge herep(Vertex v) const;
    HEdge herep(Face v) const;
    HEdge herep(Edge e) const;
    int isBoundary(HEdge he) const;
    HEdge hefromev1(Edge e, Vertex v) const; // may ret 0
    HEdge hefromev2(Edge e, Vertex v) const; // may ret 0
    HEdge hefromef(Edge e, Face f) const;    // may ret 0
    HEdge getHEdge(Vertex v, Face f) const; // slow; on f pointing to v
    HEdge queryHEdge(Vertex v1, Vertex v2) const;
    void enterHEdge(HEdge he, Vertex v1);
    void removeHEdge(HEdge he, Vertex v1);
    void create_bogus_hedges(Array<HEdge>& ar_he);
    void remove_bogus_hedges(const Array<HEdge>& ar_he);
    void gatheredgecoalescevertices(Edge e, Array<Vertex>& va) const;
friend class MeshHEdgeIter;
friend class FaceHEdgeIter;
friend class EdgeVertexIter;
friend class EdgeFaceIter;
friend class VertexCcwHEdgeIter;
    DISABLE_COPY(Mesh);
};

// *** Iterators.
// Iterators can crash if continued after any change in the Mesh.
// HEdge iterators should not be used by the general public.

// for private use:
#define ForVertexHEdge(m,vv,he) \
ForStack((vv)->fastiter_hedges(),HEdge,zzz) HEdge he=zzz->fastiter_prev();
#define ForFaceHEdge(m,ff,he) \
{ HEdge zz_hef=(ff)->fastiter_herep(), he=zz_hef; \
  for (;he;he=he->fastiter_next(),he=he==zz_hef?0:he) {
#define DummyEndFor }}
#define ForVertexCcwHEdge(m,vv,he) \
{ HEdge zz_hef=(m).mostClwHEdge(vv), he=zz_hef; \
  for (;he;he=(m).ccwHEdge(he),he=he==zz_hef?0:he) {
#define DummyEndFor }}

// These mesh iterators do not define an order.
#define ForMeshVertex(m,v) ForMapValue((m).fastiter_vertices(),int,Vertex,v)
#define ForMeshFace(m,f) ForMapValue((m).fastiter_faces(),int,Face,f)
#define ForMeshEdge(m,e) \
{ Edge e; for (MeshEdgeIter zz(m);e=zz.next(),e;) {
#define DummyEndFor }}

// These sort by id numbers.
#define ForMeshOrderedVertex(m,v) \
{ Vertex v; for (MeshOrderedVertexIter zz(m);v=zz.next(),v;) {
#define DummyEndFor }}
#define ForMeshOrderedFace(m,f) \
{ Face f; for (MeshOrderedFaceIter zz(m);f=zz.next(),f;) {
#define DummyEndFor }}

// Vertex iterators do not specify order! Work correctly on non-nice vertices.
#define ForVertexVertex(m,vv,v) \
{ Vertex v; for (VertexVertexIter zz(m,vv);v=zz.next(),v;) {
#define DummyEndFor }}
#define ForVertexFace(m,vv,f) \
{ Face f; for (VertexFaceIter zz(m,vv);f=zz.next(),f;) {
#define DummyEndFor }}
#define ForVertexEdge(m,vv,e) \
{ Edge e; for (VertexEdgeIter zz(m,vv);e=zz.next(),e;) {
#define DummyEndFor }}
#define ForVertexCorner(m,vv,c) ForVertexHEdge(m,vv,c)

// Face iterators all go CCW
#define ForFaceVertex(m,ff,v) \
ForFaceHEdge(m,ff,zz_he) Vertex v=zz_he->fastiter_vert();
#define ForFaceFace(m,ff,f) \
{ Face f; for (FaceFaceIter zz(m,ff);f=zz.next(),f;) {
#define DummyEndFor }}
#define ForFaceEdge(m,ff,e) \
ForFaceHEdge(m,ff,zz_he) Edge e=zz_he->fastiter_edge();
#define ForFaceCorner(m,ff,c) ForFaceHEdge(m,ff,c)

// Edge iterators do not define an order.
#define ForEdgeVertex(m,ee,v) \
{ Vertex v; for (EdgeVertexIter zz(m,ee);v=zz.next(),v;) {
#define DummyEndFor }}
#define ForEdgeFace(m,ee,f) \
{ Face f; for (EdgeFaceIter zz(m,ee);f=zz.next(),f;) {
#define DummyEndFor }}

// These vertex iterators go CCW, but require nice vertices.
#define ForVertexCcwVertex(m,vv,v) \
{ Vertex v; for (VertexCcwVertexIter zz(m,vv);v=zz.next(),v;) {
#define DummyEndFor }}
#define ForVertexCcwFace(m,vv,f) \
ForVertexCcwHEdge(m,vv,zz_he) Face f=zz_he->fastiter_face();
#define ForVertexCcwCorner(m,vv,c) ForVertexCcwHEdge(m,vv,c)

//--------------------------------------------------------------------------

class MVertex {
 public:
    static int allocateFlag() { assertx(_s_flag<31); return 1<<(_s_flag++); }
 private:
    Stack<HEdge> sthe;           // hedges he such that he->prev->vert==this
    int id;
    int flags;
    const char* string;
    Point point;
    MVertex() { string=0; }
    ~MVertex() { delete[] (char*)string; }
    static int _s_flag;
friend class Mesh;
friend class GMesh;
friend class MeshHEdgeIter;
friend class VertexHEdgeIter;
    DISABLE_COPY(MVertex);
 public:                        // should be private
    const Stack<HEdge>& fastiter_hedges() const { return sthe; }
 public:
    MAKE_POOLED_SAC(MVertex);   // must be last entry of class!
};

INITIALIZE_POOL(MVertex);

class MFace {
 public:
    static int allocateFlag() { assertx(_s_flag<31); return 1<<(_s_flag++); }
 private:
    HEdge herep;
    int id;
    int flags;
    const char* string;
    MFace() { string=0; }
    ~MFace() { delete[] (char*)string; }
    static int _s_flag;
friend class Mesh;
friend class GMesh;
    DISABLE_COPY(MFace);
 public:                        // should be private
    HEdge fastiter_herep() const { return herep; }
 public:
    MAKE_POOLED_SAC(MFace);     // must be last entry of class!
};

INITIALIZE_POOL(MFace);

class MEdge {
 public:
    static int allocateFlag() { assertx(_s_flag<31); return 1<<(_s_flag++); }
 private:
    HEdge herep;
    int flags;
    const char* string;
    MEdge() { string=0; }
    ~MEdge() { delete[] (char*)string; }
    static int _s_flag;
friend class Mesh;
friend class GMesh;
friend class MeshEdgeIter; // for efficient access to herep
    DISABLE_COPY(MEdge);
 public:
    HEdge fastiter_herep() { return herep; }
    MAKE_POOLED_SAC(MEdge);     // must be last entry of class!
};

INITIALIZE_POOL(MEdge);

class MHEdge {
 private:
    HEdge prev;                 // previous HEdge in ring around face
    HEdge next;                 // next HEdge in ring around face
    HEdge sym;                  // pointer to symmetric HEdge (or 0)
    Vertex vert;                // Vertex to which this HEdge is pointing
    Face face;                  // Face on which this HEdge belongs
    Edge edge;                  // Edge to which this HEdge belongs
    const char* string;
    MHEdge() { string=0; }
    ~MHEdge() { delete[] (char*)string; }
friend class Mesh;
friend class GMesh;
friend class MeshEdgeIter;
friend class VertexHEdgeIter;
friend class VertexVertexIter;
friend class VertexFaceIter;
friend class VertexEdgeIter;
friend class FaceHEdgeIter;
friend class FaceVertexIter;
friend class FaceFaceIter;
friend class FaceEdgeIter;
friend class EdgeVertexIter;
friend class EdgeFaceIter;
friend class VertexCcwVertexIter;
friend class VertexCcwFaceIter;
    DISABLE_COPY(MHEdge);
 public:                        // should be private
    HEdge fastiter_next() { return next; }
    HEdge fastiter_prev() { return prev; }
    Vertex fastiter_vert() { return vert; }
    Face fastiter_face() { return face; }
    Edge fastiter_edge() { return edge; }
    HEdge fastiter_sym() { return sym; }
 public:
    MAKE_POOLED_SAC(MHEdge);    // must be last entry of class!
};

INITIALIZE_POOL(MHEdge);

// *** inlines

inline HEdge Mesh::herep(Vertex v) const
{
    return v->sthe.empty() ? 0 : v->sthe.top()->prev;
}

inline HEdge Mesh::herep(Face f) const { return f->herep; }
inline HEdge Mesh::herep(Edge e) const { return e->herep; }

inline int Mesh::isBoundary(Edge e) const { return herep(e)->sym ? 0 : 1; }

inline Vertex Mesh::vertex1(Edge e) const { return herep(e)->prev->vert; }
inline Vertex Mesh::vertex2(Edge e) const { return herep(e)->vert; }

inline Face Mesh::face1(Edge e) const { return herep(e)->face; }
inline Face Mesh::face2(Edge e) const {
    HEdge he=herep(e);
    return he->sym ? he->sym->face : 0;
}

inline Vertex Mesh::corner_vertex(Corner c) const { return c->vert; }
inline Face Mesh::corner_face(Corner c) const { return c->face; }

inline Corner Mesh::ccwCorner(Corner c) const { return c->sym?c->sym->prev:0; }
inline Corner Mesh::clwCorner(Corner c) const { return c->next->sym; }
inline Corner Mesh::ccwFaceCorner(Corner c) const { return c->next; }
inline Corner Mesh::clwFaceCorner(Corner c) const { return c->prev; }

inline Vertex Mesh::idvertex(int i) const { return _id2vertex.get(i); }
inline int Mesh::vertexid(Vertex v) const { return v->id; }
inline Face Mesh::idface(int i) const { return _id2face.get(i); }
inline int Mesh::faceid(Face f) const { return f->id; }

inline Vertex Mesh::id_retrieve_vertex(int i) const
{ return _id2vertex.retrieve(i); }
inline Face Mesh::id_retrieve_face(int i) const
{ return _id2face.retrieve(i); }

inline int Mesh::gflag(int flm) const { return _flags&flm; }
inline int Mesh::flag(Vertex v, int flm) const { return v->flags&flm; }
inline int Mesh::flag(Face f, int flm) const { return f->flags&flm; }
inline int Mesh::flag(Edge e, int flm) const { return e->flags&flm; }

inline int Mesh::gmodFlag(int flm, int pbool) {
    int st=gflag(flm);
    if (pbool) { if (st!=flm) _flags|=flm; }
    else { if (st) _flags&=~flm; }
    return st;
}
inline int Mesh::modFlag(Vertex v, int flm, int pbool) {
    int st=flag(v,flm);
    if (pbool) { if (st!=flm) v->flags|=flm; }
    else { if (st) v->flags&=~flm; }
    return st;
}
inline int Mesh::modFlag(Face f, int flm, int pbool) {
    int st=flag(f,flm);
    if (pbool) { if (st!=flm) f->flags|=flm; }
    else { if (st) f->flags&=~flm; }
    return st;
}
inline int Mesh::modFlag(Edge e, int flm, int pbool) {
    int st=flag(e,flm);
    if (pbool) { if (st!=flm) e->flags|=flm; }
    else { if (st) e->flags&=~flm; }
    return st;
}

inline int Mesh::isTriangle(Face f) const
{
    HEdge he=herep(f);
    return he->next->next->next==he;
}

// *** ITERATORS
// * Mesh

class MeshHEdgeIter {
 public:
    MeshHEdgeIter(const Mesh& m);
    HEdge next();
 private:
    MapIter<int,Vertex> mi;
    StackIter<HEdge> si;
};

class MeshEdgeIter {
 public:
    MeshEdgeIter(const Mesh& m);
    Edge next();
 private:
    MeshHEdgeIter it;
};

class MeshOrderedVertexIter {
 public:
    MeshOrderedVertexIter(const Mesh& m);
    Vertex next();
 private:
    Pqueue<Vertex> pq;
};

class MeshOrderedFaceIter {
 public:
    MeshOrderedFaceIter(const Mesh& m);
    Face next();
 private:
    Pqueue<Face> pq;
};

// * Vertex

class VertexHEdgeIter {         // return HEdges pointing to v
 public:
    VertexHEdgeIter(const Mesh& m, Vertex v);
    HEdge next();
 private:
    StackIter<HEdge> si;
};

class VertexVertexIter {
 public:
    VertexVertexIter(const Mesh& m, Vertex v);
    Vertex next();
 private:
    VertexHEdgeIter it;
    Vertex extrav;
};

class VertexFaceIter {
 public:
    VertexFaceIter(const Mesh& m, Vertex v);
    Face next();
 private:
    VertexHEdgeIter it;
};

class VertexEdgeIter {
 public:
    VertexEdgeIter(const Mesh& m, Vertex v);
    Edge next();
 private:
    VertexHEdgeIter it;
    Edge extrae;
};

// * Face

class FaceHEdgeIter {
 public:
    FaceHEdgeIter(const Mesh& m, Face f);
    HEdge next();
 private:
    HEdge hef;
    HEdge he;
};

class FaceVertexIter {
 public:
    FaceVertexIter(const Mesh& m, Face f);
    Vertex next();
 private:
    FaceHEdgeIter it;
};

class FaceFaceIter {
 public:
    FaceFaceIter(const Mesh& m, Face f);
    Face next();
 private:
    FaceHEdgeIter it;
};

class FaceEdgeIter {
 public:
    FaceEdgeIter(const Mesh& m, Face f);
    Edge next();
 private:
    FaceHEdgeIter it;
};

// * Edge

class EdgeVertexIter {
 public:
    EdgeVertexIter(const Mesh& m, Edge e);
    Vertex next();
 private:
    HEdge he;
    int i;
};

class EdgeFaceIter {
 public:
    EdgeFaceIter(const Mesh& m, Edge e);
    Face next();
 private:
    HEdge he;
    int i;
};

// * VertexCcw

class VertexCcwHEdgeIter {      // return HEdges pointing to v
 public:
    VertexCcwHEdgeIter(const Mesh& m, Vertex vp);
    HEdge next();
 private:
    const Mesh& mesh;
    Vertex v;
    HEdge hec, hef;
};

class VertexCcwVertexIter {
 public:
    VertexCcwVertexIter(const Mesh& m, Vertex vp);
    Vertex next();
 private:
    VertexCcwHEdgeIter it;
    Vertex extrav;
};

class VertexCcwFaceIter {
 public:
    VertexCcwFaceIter(const Mesh& m, Vertex v);
    Face next();
 private:
    VertexCcwHEdgeIter it;
};

// ** Mesh iter

inline HEdge MeshHEdgeIter::next()
{
    if (si) return si.next();
    if (!mi) return 0;
    for (;;) {
        mi.next();
        if (!mi) return 0;
        si.reinit(mi.value()->sthe);
        if (si) return si.next();
    }
}

inline Edge MeshEdgeIter::next()
{
    for (;;) {
        HEdge he=it.next();
        if (!he) return 0;
        Edge e=he->edge;
        if (e->herep==he) return e;
    }
}

inline Vertex MeshOrderedVertexIter::next()
{
    return pq.empty() ? 0 : pq.removemin();
}

inline Face MeshOrderedFaceIter::next()
{
    return pq.empty() ? 0 : pq.removemin();
}

// ** Vertex iter

inline VertexHEdgeIter::VertexHEdgeIter(const Mesh&, Vertex v)
: si(v->sthe) { }

inline HEdge VertexHEdgeIter::next() { return si ? si.next()->prev : 0; }

// ** Face iter

inline FaceHEdgeIter::FaceHEdgeIter(const Mesh& m, Face f)
{ he=hef=m.herep(f); }

inline HEdge FaceHEdgeIter::next()
{
    if (!he) return 0;
    HEdge her=he;
    he=he->next;
    if (he==hef) he=0;
    return her;
}

inline FaceVertexIter::FaceVertexIter(const Mesh& m, Face f)
: it(m,f) { }

inline Vertex FaceVertexIter::next() {
    HEdge he=it.next();
    return he ? he->vert : 0;
}

#endif
