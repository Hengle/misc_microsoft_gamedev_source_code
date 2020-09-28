// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Gmesh_h
#define Gmesh_h

#include "Mesh.h"
#include "Geometry.h"

// *** See DOCUMENTATION on MESH FILE FORMAT at the end of this file.

class Polygon; class WA3dStream; class A3dVertexColor;

// Corner data is currently not handled

class GMesh : public Mesh {
 public:
    GMesh();
    ~GMesh();
// Extend functionality
    // copy carries flags, strings, and geometry (but not sac fields)
    void copy(const GMesh& mo); // must be empty
    void merge(const GMesh& mo, Map<Vertex,Vertex>* mvvn=0);
    void destroyVertex(Vertex v);
    void destroyFace(Face f);
    // do appropriate actions with geometry, SHARPE, and face strings
    void collapseEdgeVertex(Edge e, Vertex vs);
    void collapseEdge(Edge e);
    Vertex splitEdge(Edge e, int id=0);
    Edge swapEdge(Edge e);
    //
    Vertex splitVertex(Vertex v1, Vertex vs1, Vertex vs2, int v2i);
    Vertex center_split_face(Face f);
    Edge split_face(Face f, Vertex v1, Vertex v2);
    Face coalesce_faces(Edge e);
    Vertex insert_vertex_on_edge(Edge e);
    Edge remove_vertex_between_edges(Vertex vr);
// Geometry
    const Point& point(Vertex v) const;
    void setPoint(Vertex v, const Point& p);
    void polygon(Face f, Polygon& polygon) const;
    float length2(Edge e) const;
    float length(Edge e) const;
    float area(Face f) const;
    void transform(const Frame& frame);
// Strings
    const char* string(Vertex v) const;
    const char* string(Face f) const;
    const char* string(Edge e) const;
    const char* string(Corner c) const;
    static const char* string_key(const char* str, const char* key);
    const char* corner_key(Corner c, const char* key) const; // Corner|Vertex
    bool parse_corner_key_vec(Corner c, const char* key,
                              float ar[], int num) const; // Corner|Vertex
    // copies string
    void setString(Vertex v, const char* s);
    void setString(Face f, const char* s);
    void setString(Edge e, const char* s);
    void setString(Corner c, const char* s);
    static const char* string_update(const char* str, const char* key,
                                     const char* val);
    void updateString(Vertex v, const char* key, const char* val);
    void updateString(Face f, const char* key, const char* val);
    void updateString(Edge e, const char* key, const char* val);
    void updateString(Corner c, const char* key, const char* val);
// standard I/O for my meshes (see format below)
    void read(istream& is);     // read a whole mesh, discard comments
    void readline(char* s);     // no '\n' required
    static int recognizeLine(const char* s);
    void write(ostream& os) const;
    void write(WA3dStream& oa3d, const A3dVertexColor& col) const;
    void write_face(WA3dStream& oa3d, const A3dVertexColor& col, Face f) const;
    ostream* recordChanges(ostream* pos); // pos may be 0, ret old
// Flag bits
    // Predefined Vertex,Face,Edge flag bits
    // first 2 are parsed when reading
    static const int& CUSPV;    // "cusp" on Vertex
    static const int& SHARPE;   // "sharp" on Edge
// Discouraged:
    Vertex createVertexI(int id);
    Face createFaceI(int id, const Array<Vertex>& va); // or die
 private:
    ostream* _os;               // for recordChanges
};

// Format a vector string "(%g ... %g)" with num=1..4
extern const char* hform_vec(const float ar[], int num);

// Parse a vector in a {key=value}+ string
extern bool parse_key_vec(const char* str, const char* key,
                          float ar[], int num);

// I/O Mesh Format (Vertices and Faces must fit on one line)
//   (vertex numbers begin with 1)
//   Vertex vi  x y z [{other_info}]
//   Face fi  vi1 vi2 ... vin [{other_info}]
//   MVertex vi newx newy newz
//   Ecol v1 v2
//   Eswa v1 v2
//   Espl v1 v2 vnew
// Example:
//   Vertex 1  1.5e2 0 1.5 {normal=(0,1,0)}
//   Vertex 2  0 1.5 0
//   Face 1  1 2 3
//   Face 2  2 3 4 5 {color=red, phong=2}
//  fi may be zero, in which case a number is assigned

class StringKeyIter {
 public:
    StringKeyIter(const char* pstr);
    ~StringKeyIter();
    int next(const char*& kb, int& kl, const char*& vb, int& vl);
 private:
    const char* _str;
    const char* _s;
    // shallow copy is safe
};

#define ForStringKeyValue(S,KS,KL,VS,VL) \
{ StringKeyIter zz(S); \
  const char* KS; const char* VS; \
  int KL; int VL; \
  while (zz.next(KS,KL,VS,VL)) {
#define DummyEndFor }}

//----------------------------------------------------------------------

inline const Point& GMesh::point(Vertex v) const { return v->point; }

inline const char* GMesh::string(Vertex v) const { return v->string; }
inline const char* GMesh::string(Face f) const { return f->string; }
inline const char* GMesh::string(Edge e) const { return e->string; }
inline const char* GMesh::string(Corner c) const { return c->string; }
inline void GMesh::setString(Vertex v, const char* s)
{ delete[] (char*)v->string; v->string=newString(s); }
inline void GMesh::setString(Face f, const char* s)
{ delete[] (char*)f->string; f->string=newString(s); }
inline void GMesh::setString(Edge e, const char* s)
{ delete[] (char*)e->string; e->string=newString(s); }
inline void GMesh::setString(Corner c, const char* s)
{ delete[] (char*)c->string; c->string=newString(s); }


// MESH FILE FORMAT
// 
// Set of one-line records.  Each record contains either:
// 
// # a comment with '#' in first column
// Vertex vi x y z [{other_info}]
// Face fi vi1 vi2 vi3 [{other_info}]
// Corner vi fi {other_info}
// Edge vi1 vi2 {other_info}
// 
// The values vi and fi are vertex and face indices respectively,
//  starting at 1.
// 
// other_info: a string containing a list of attributes separated by exactly
//  one space; each attribute has the form:
//  - attrib=value
//  - attrib="some string value"
//  - attrib=(some string value)
//  - attrib                (its value is "")
// 
// As mentioned in the PM paper, scalar attributes are associated with the mesh
//  vertices, or with the corners adjacent to a vertex.
// (If all corner attributes about a vertex are the same, the attributes can be
//  moved to the vertex.)
// Also, discrete attributes are associated with the faces of the mesh.
// 
// My simplification procedure recognizes the following attributes on meshes:
//  - normal=(x y z)    normals (on vertices or corners)
//  - uv=(u v)          texture coordinates (on vertices or corners)
//  - rgb=(r g b)       color values (on vertices, corners, or on faces)
//  - wid=id            wedge_identifier (on vertices or corners)
//  - any set of attributes on faces (matid=d mat="material name", etc.)
//  - no attributes on edges (actually, it supports "sharp" edges on input).
// 
// The wedge identifiers need not be present in the input mesh.
// They are created by the simplification procedure, and identify those corners
// about a vertex which share common attributes.  These wedge identifiers could
// be used for more efficient storage.
// 
// In addition, meshes which are in fact geomorphs contain "ancestor"
// attributes (as they are called in the paper) to enable smooth visual
// interpolation.  These attributes are:
// 
//  - Opos=(x y z)              the old position of the vertex
//  - Onormal=(x y z)   the old normal (on vertex or corner)
//  - Ouv=(u v)         the old texture coordinates (on vertex or corner)
//  - Orgb=(r g b)              the old color values (on vertex or corner)
// 
// Example:
// 
// # Some random mesh.
// Vertex 1  1.1e0 0 1.5 {normal=(0 1 0) uv=(0 0)}
// Vertex 2  0 1.5 0 {normal=(1 0 0) uv=(0.5 0.5)}
// Vertex 3  1 1.5 0
// Vertex 4  1 1.5 1 {normal=(1 0 0) uv=(0.5 0.5)}
// Face 1  1 2 3 {mat="red_brick17" rgb=(1 0 0)}
// Face 2  3 2 4 {mat="grey_cement" rgb=(.5 .5 .5)}
// Corner 3 1 {normal=(1 0 0) uv=(0.5 0.5)}
// Corner 3 2 {normal=(0 1 0) uv=(0 0.5)}
//
// (For exact specifications, refer to GMesh.cxx)

#endif
