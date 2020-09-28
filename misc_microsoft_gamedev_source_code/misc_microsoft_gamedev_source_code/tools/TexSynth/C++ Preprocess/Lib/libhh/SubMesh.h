// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef SubMesh_h
#define SubMesh_h

#include "GMesh.h"
#include "Combination.h"
#include "Homogeneous.h"
#include "Array.h"

struct Combvh {
    Combination<Vertex> c;
    Homogeneous h;
    int iscombination() const;
    Point evaluate(const GMesh& mesh) const;
    POOL_ALLOCATION(Combvh);
};

INITIALIZE_POOL(Combvh);

class Mvcvh : public Map<Vertex,Combvh*> {
 public:
    Mvcvh();
    ~Mvcvh();                   // automatically deletes combinations
    void shallowclear();        // no delete of combinations
    void clear();               // NONVIRTUAL!
    int isconvolution() const;  // check combination is affine
    void compose(const Combvh& ci, Combvh& co) const; // co=ci*this
    // compose two maps to produce one, die unless mconv.isconvolution()
    void compose(const Mvcvh& mconv); // this=mconv*this
};

class SubMesh {
 public:
    SubMesh(GMesh& mesh);
    ~SubMesh();
    void clear();
    static const int& VARIABLEV; // MVertex flag bit
    // mesh() may be modified if no more SubMesh operations will be done.
    GMesh& mesh() { return _m; }
    const GMesh& mesh() const { return _m; }
    GMesh& origmesh() { return _omesh; }
    const GMesh& origmesh() const { return _omesh; }
// subdivide (makes use of refine(),createconv(),convolveself(),...)
    void subdivide(float cosang=1.f);
    void subdividen(int nsubdiv, int limit, float cosang=1.f,
                    bool triang=true);
// Combinations
    // get a combination (expressing v of mesh() in terms of origmesh())
    const Combvh& combination(Vertex v) const;
    // Compose c1 with _cmvcvh to get combination in terms of orig. verts.
    void composecmvcvh(const Combvh& ci, Combvh& co) const;
// update vertex positions on mesh() according to its mask
    void updatevertexposition(Vertex v);
    void updatevertexpositions();
// misc
    void maskparameters(int ps222, float pweighta)
    { _s222=ps222; _weighta=pweighta; }
    void init() { _s222=0; _weighta=0; }
// omesh to and from mesh       
    Face origface(Face f) const;
    void origfaceindex(Face fi, Face& fo, int& pindex) const;
    Face getface(Face of, int index) const;
// split and compute splitting masks
    void refine(Mvcvh& mconv);  // 1to4 split at edge medians
    // refine near creases, and refine edges with cosdihedral <cosang
    void selrefine(Mvcvh& mconv, float cosang);
// compute averaging masks
    typedef void (SubMesh::*FVMASK)(Vertex v, Combvh& comb) const;
    void createconv(Mvcvh& mconv, FVMASK f); // use a subdivision mask
// the masks
    void averagingmask(Vertex v, Combvh& comb) const;
    void limitmask(Vertex v, Combvh& comb) const;
// triangulate
    void triangulatequads(Mvcvh& mconv);  // 1to4 split at centroids
// apply a convolution
    void convolveself(const Mvcvh& mconv); // _cmvcvh=mconv*_cmvcvh
// debug
    void showmvcvh(const Mvcvh& mvcvh) const;
    void showcmvcvh() const;
 private:
    GMesh& _omesh;
    GMesh _m;
    Mvcvh _cmvcvh;              // maps vertices of _m to Combvh of _omesh
    Map<Face,Face> _mforigf;    // face of _m -> face of _omesh
    Map<Face,int> _mfindex;     // face of _m -> index within origf
    Map<Face,Array<Face>*> _mofif; // face of _omesh -> (int -> face of _m)
    bool _allvvar;                 // if all vertices are VARIABLEV
    bool _isquad;
    //
    int _s222;
    float _weighta;
    //
    int sharp(Edge e) const;
    int nume(Vertex v) const;
    int numsharpe(Vertex v) const;
    Edge oppsharpe(Vertex v, Edge e) const;
    Vertex oppsharpv(Vertex v, Vertex v2) const;
    void subdivideaux(float cosang, Mvcvh* mconv);
    void creaseaveragingmask(Vertex v, Combvh& comb) const;
    int extraordinarycreasev(Vertex v) const;
    DISABLE_COPY(SubMesh);
};

#endif
