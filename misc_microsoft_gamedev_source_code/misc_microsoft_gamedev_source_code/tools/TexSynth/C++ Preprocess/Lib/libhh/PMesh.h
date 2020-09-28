// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef PMesh_h
#define PMesh_h

#include "Geometry.h"           // for Point, Vector
#include "A3dStream.h"          // for A3dColor
#include "Array.h"
#include "Bbox.h"
#include "Materials.h"

// Currently SGI NCC outputs slow constructor functions for
//  PMWedge and PMVertex (probably bug; they should not exist at all.)
// I traced this to the presence of constructors in
//  the classes Affine and Vector/Point.
// This problem has nothing to do with Pools.

// I define ARRAY_BIT_COPY(class) for most structs so that Array uses
//  memcpy() when resizing Arrays of such structs.

class GMesh;
class Ancestry;
class PMeshIter;

// Vertex attributes.
struct PMVertexAttrib {
    Point point;
};
ARRAY_BIT_COPY(PMVertexAttrib);

// Vertex attribute deltas.
struct PMVertexAttribD {
    Vector dpoint;
};
ARRAY_BIT_COPY(PMVertexAttribD);

void interp(PMVertexAttrib& a, const PMVertexAttrib& a1,
            const PMVertexAttrib& a2, float f1, float f2);
void add(PMVertexAttrib& a,
         const PMVertexAttrib& a1, const PMVertexAttribD& ad);
void sub(PMVertexAttrib& a,
         const PMVertexAttrib& a1, const PMVertexAttribD& ad);
void diff(PMVertexAttribD& ad,
          const PMVertexAttrib& a1, const PMVertexAttrib& a2);
int compare(const PMVertexAttrib& a1, const PMVertexAttrib& a2);
int compare(const PMVertexAttrib& a1, const PMVertexAttrib& a2, float tol);

// Wedge attributes.
struct PMWedgeAttrib {
    Vector normal;
    A3dColor rgb;
    UV uv;
};
ARRAY_BIT_COPY(PMWedgeAttrib);

// Wedge attribute deltas.
struct PMWedgeAttribD {
    Vector dnormal;
    A3dColor drgb;
    UV duv;
};
ARRAY_BIT_COPY(PMWedgeAttribD);

void interp(PMWedgeAttrib& a, const PMWedgeAttrib& a1,
            const PMWedgeAttrib& a2, float f1, float f2);
void add(PMWedgeAttrib& a,
         const PMWedgeAttrib& a1, const PMWedgeAttribD& ad);
void sub_noreflect(PMWedgeAttrib& a, const PMWedgeAttrib& abase,
                   const PMWedgeAttribD& ad);
void sub_reflect(PMWedgeAttrib& a, const PMWedgeAttrib& abase,
                 const PMWedgeAttribD& ad);
void add_zero(PMWedgeAttrib& a, const PMWedgeAttribD& ad);
void diff(PMWedgeAttribD& ad,
          const PMWedgeAttrib& a1, const PMWedgeAttrib& a2);
int compare(const PMWedgeAttrib& a1, const PMWedgeAttrib& a2);
int compare(const PMWedgeAttrib& a1, const PMWedgeAttrib& a2, float tol);

// Face attribute is discrete: an integer material identifier that indexes
//  into the _materials array.
struct PMFaceAttrib {
    int matid;
    // Note: a high bit is used for FACE_VISITED_MASK
    //  in SRMesh::ogl_render_faces_strips().
};
ARRAY_BIT_COPY(PMFaceAttrib);

struct PMVertex {
    PMVertexAttrib attrib;
};
ARRAY_BIT_COPY(PMVertex);

struct PMWedge {
    int vertex;
    PMWedgeAttrib attrib;
};
ARRAY_BIT_COPY(PMWedge);

struct PMFace {
    int wedges[3];
    PMFaceAttrib attrib;
};
ARRAY_BIT_COPY(PMFace);

// optimize: use pointers in PMFace::wedges and W_Edge::vertex
// However, this requires arrays that don't reallocate.
// So either preallocate, or use versatile Win32 memory allocation.

struct PMeshInfo;

// Wedge mesh: faces -> wedges -> vertices
class WMesh {
 public:
    WMesh();
    ~WMesh();
    // Default operator=() and copy_constructor are safe.
    void read(istream& is, const PMeshInfo& pminfo); // must be empty
    void write(ostream& os, const PMeshInfo& pminfo) const;
    void extract_gmesh(GMesh& gmesh, const PMeshInfo& pminfo) const;
    void OK() const;
 public:
    Materials _materials;
    Array<PMVertex> _vertices;
    Array<PMWedge> _wedges;
    Array<PMFace> _faces;
};

// Vertex split record.
// Records the information necessary to split a vertex of the mesh,
//  in order to add to the mesh 1 new vertex and 1/2 new faces.
class Vsplit {
 public:                        // public because of Filterprog construction
friend class AWMesh;
friend class PMesh;
friend class PMeshRStream;
    // Default operator=() and copy_constructor are safe.
    void read(istream& is, const PMeshInfo& pminfo);
    void write(ostream& os, const PMeshInfo& pminfo) const;
    void OK() const;
    int adds_two_faces() const;
    // This format provides these limits:
    // - maximum number of faces: 1<<32
    // - maximum vertex valence:  1<<16
    // - maximum number of materials: 1<<16
// Encoding of vertices vs, vl, vr.
    // Face flclw is the face just CLW of vl from vs.
    //  vs is the vs_index\'th vertex of flclw
    //  (vl is the (vs_index+2)%3\'th vertex of face flclw)
    //  vr is the (vlr_offset1-1)\'th vertex when rotating CLW about vs from vl
    // Special cases:
    // - vlr_offset1==1 : no_vr and no_fr
    // - vlr_offest1==0: no flclw! vspl.flclw is actually flccw.
    unsigned int flclw;         // 0..(mesh.numFaces()-1)
    unsigned short vlr_offset1; // 0..(max_vertex_valence) (prob<valence/2)
    unsigned short code;        // (vs_index(2),ii(2),ws(3),wt(3),wl(2),wr(2),
                                //  fl_matid>=0(1),fr_matid>=0(1))
    enum {
        B_STMASK=0x0007,
        B_LSAME=0x0001,
        B_RSAME=0x0002,
        B_CSAME=0x0004,
        //
        B_LRMASK=0x0003,
        B_ABOVE=0x0000,
        B_BELOW=0x0001,
        B_NEW  =0x0002,         // must be on separate bit.
    };
    enum {
        VSINDEX_SHIFT=0,
        VSINDEX_MASK=(0x0003<<VSINDEX_SHIFT),
        //
        II_SHIFT=2,
        II_MASK=(0x0003<<II_SHIFT),
        //
        S_SHIFT=4,
        S_MASK=(B_STMASK<<S_SHIFT),
        S_LSAME=(B_LSAME<<S_SHIFT),
        S_RSAME=(B_RSAME<<S_SHIFT),
        S_CSAME=(B_CSAME<<S_SHIFT),
        //
        T_SHIFT=7,
        T_MASK=(B_STMASK<<T_SHIFT),
        T_LSAME=(B_LSAME<<T_SHIFT),
        T_RSAME=(B_RSAME<<T_SHIFT),
        T_CSAME=(B_CSAME<<T_SHIFT),
        //
        L_SHIFT=10,
        L_MASK=(B_LRMASK<<L_SHIFT),
        L_ABOVE=(B_ABOVE<<L_SHIFT),
        L_BELOW=(B_BELOW<<L_SHIFT),
        L_NEW  =(B_NEW<<L_SHIFT),
        //
        R_SHIFT=12,
        R_MASK=(B_LRMASK<<R_SHIFT),
        R_ABOVE=(B_ABOVE<<R_SHIFT),
        R_BELOW=(B_BELOW<<R_SHIFT),
        R_NEW  =(B_NEW<<R_SHIFT),
        //
        FLN_SHIFT=14,
        FLN_MASK=(1<<FLN_SHIFT),
        //
        FRN_SHIFT=15,
        FRN_MASK=(1<<FRN_SHIFT),
    };
    //Documentation:
    // vs_index: 0..2: index of vs within flace flclw
    // ii: 0..2: ==alpha(1.0,0.5,0,0)
    //   ii=2: a=0.0 (old_vs=~new_vs)
    //   ii=1: a=0.5
    //   ii=0: a=1.0 (old_vs=~new_vt)
    // Inside wedges
    //  {S,T}{LSAME}: if exists outside left wedge and if same
    //  {S,T}{RSAME}: if exists outside right wedge and if same
    //  {S,T}{CSAME}: if inside left and right wedges are same
    //  (when no_vr, {S,T}RSAME==1, {S,T}CSAME==0)
    // Outside wedges
    //  (when no_vr, RABOVE==1)
    // New face material identifiers
    //  {L,R}NF: if 1, face matids not predicted correctly using ii,
    //     so included in f{l,r}_matid
    //  (when no_vr, RNF==0 obviously)
    //
    //Probabilities:
    //  vs_index: 0..2 (prob. uniform)
    //  ii: ii==2 prob. low/med   (med if 'MeshSimplify -nominii1')
    //      ii==0 prob. low/med
    //      ii==1 prob. high/zero (zero if 'MeshSimplify -monminii1')
    //  {S,T}LSAME: prob. high
    //  {S,T}RSAME: prob. high
    //  {S,T}CSAME: prob. low
    //  {L,R}ABOVE: prob. high
    //  {L,R}BELOW: prob. low
    //  {L,R}NEW:   prob. low
    // Note: wl, wr, ws, wt are correlated since scalar half-edge
    //  discontinuities usually match up at both ends of edges.
    // -> do entropy coding on (ii,wl,wr,ws,wt) symbol as a whole.
// Face attribute values (usually predicted correctly)
    // these are defined only if {L,R}NF respectively
    //  otherwise for now they are set to 0
    unsigned short fl_matid;
    unsigned short fr_matid;
// Vertex attribute deltas
    // for ii==2: vad_large=new_vt-old_vs, vad_small=new_vs-old_vs
    // for ii==0: vad_large=new_vs-old_vs, vad_small=new_vt-old_vs
    // for ii==1: vad_large=new_vt-new_i,  vad_small=new_i-old_vs
    //    where new_i=interp(new_vt,new_vs)
    PMVertexAttribD vad_large;
    PMVertexAttribD vad_small; // is zero if "MeshSimplify -nofitgeom"
// Wedge attribute deltas (size 1--6)
    Array<PMWedgeAttribD> ar_wad;
    // Order: [(wvtfl, wvsfl), [(wvtfr, wvsfr)], wvlfl, [wvrfr]]
// Residual information
    float resid_uni;
    float resid_dir;
 private:
    int expected_wad_num(const PMeshInfo& pminfo) const;
};
// cannot ARRAY_BIT_COPY(Vsplit) since Vsplit contains an Array!

// For each face, what are its 3 neighbors?
//  faces[i] is across edge opposite of wedges[i].
//  <0 if no neighbor.
struct PMFaceNeighbors {
    int faces[3];
};
ARRAY_BIT_COPY(PMFaceNeighbors);

// Wedge mesh augmented with adjacency information.
// Specifically, dual graph encoding face-face adjacency
//  using PMFaceNeighbors.
class AWMesh : public WMesh {
 public:
    AWMesh();
    // AWMesh(const WMesh& wmesh); // expensive
    ~AWMesh();
    // Default operator=() and copy_constructor are safe.
    void read(istream& is, const PMeshInfo& pminfo); // must be empty
    void write(ostream& os, const PMeshInfo& pminfo) const;
    void OK() const;
    void rotate_ccw(int v, int& w, int& f) const;
    void rotate_clw(int v, int& w, int& f) const;
// Rendering: common code and data
    enum { FACE_VISITED_MASK=1<<30 }; // high bit of matid
    int _cur_frame_mask;              // 0 or FACE_VISITED_MASK
// Rendering using OpenGL
    void ogl_render_faces_individually(const PMeshInfo& pminfo,
                                       int usetexture);
    void ogl_render_faces_strips(const PMeshInfo& pminfo,
                                 int usetexture);
    void ogl_render_edges();
// Rendering using Direct3D
    // to be defined
 public:
    Array<PMFaceNeighbors> _fnei; // must be same size as _faces!!
 public:
    int get_jvf(int v, int f) const; // get index of vertex v in face f
    int get_wvf(int v, int f) const;
    int mostClwFace(int v, int f); // negative if v is interior vertex
    int mostCcwFace(int v, int f); // negative if v is interior vertex
    bool isBoundary(int v, int f);
    bool gatherFaces(int v, int f, Array<int>& faces); // ret: is_boundary
 private:
    void construct_adjacency();
    void apply_vsplit_ancestry(Ancestry* ancestry, int vs, int isr,
                               int onumwedges, int code,
                               int wvlfl, int wvrfr,
                               int wvsfl, int wvsfr,
                               int wvtfl, int wvtfr);
 private:
// Rendering using OpenGL
    Array<uchar> _ogl_mat_byte_rgba; // size is _materials.num()*4 !
    void ogl_process_materials();
// Rendering using Direct3D
    // to be defined
 protected:
    friend void global_reorder_vspl(int first_ivspl, int last_ivspl);
    void apply_vsplit(const Vsplit& vspl, const PMeshInfo& pminfo,
                      Ancestry* ancestry=0);
    void undo_vsplit(const Vsplit& vspl, const PMeshInfo& pminfo);
};

struct PMeshInfo {
    int _read_version;
    int _has_rgb;
    int _has_uv;
    int _has_resid;
    int _has_wad2;
    int _tot_nvsplits;
    int _full_nvertices;
    int _full_nwedges;
    int _full_nfaces;
    Bbox _full_bbox;
};

// Progressive mesh:
//  contains a base mesh and a sequence of vertex split records.
class PMesh {
 public:
    PMesh();
    ~PMesh();
    // Default operator=() and copy_constructor are safe.
    // non-progressive read
    void read(istream& is);     // die unless empty
    void write(ostream& os) const;
    void truncate_beyond(PMeshIter& pmi); // remove all vsplits beyond iterator
    void truncate_prior(PMeshIter& pmi); // advance base mesh
 public:
friend class PMeshRStream;
    AWMesh _base_mesh;
    Array<Vsplit> _vsplits;
    PMeshInfo _info;
 private:
    static void read_header(istream& is, PMeshInfo& pm);
    static int at_trailer(istream& is);
    // const AWMesh& base_mesh const { return _base_mesh; }
};

// Progressive mesh stream
// Can be either:
//  - read from an existing PMesh, or
//  - read from an input streamm, or
//  - read from an input stream and archived to a PMesh
class PMeshRStream {
 public:
    PMeshRStream(const PMesh& pm);
    PMeshRStream(istream& is, PMesh* ppm_construct=0);
    // Default operator=() and copy_constructor are safe.
    ~PMeshRStream();
    void read_base_mesh(AWMesh* bmesh=0); // always call this first!
    const AWMesh& base_mesh();
    int is_reversible() const { return _pm?1:0; }
    const Vsplit* next_vsplit();
    const Vsplit* prev_vsplit(); // die if !is_reversible()
    const Vsplit* peek_next_vsplit(); // peek without using it
    PMeshInfo _info;
 private:
friend class PMeshIter;
friend class PMesh;             // for PMesh::truncate_*()
    istream* _is;               // may be 0
    PMesh* _pm;                 // may be 0
    // _vspliti==-1 before base_mesh is read
    int _vspliti;               // def if _pm, next to read from _pm->_vsplits
    Vsplit _vspl;               // def if !_pm, temp
    int _vspl_ready;            // def if !_pm, true if _vspl is only peeked
    AWMesh _lbase_mesh;        // used to store basemesh if !_pm
};

// Progressive mesh iterator (is a AWMesh!)
class PMeshIter : public AWMesh {
 public:
    PMeshIter(PMeshRStream& pmrs);
    // Cloning is provided by copy_constructor.
    ~PMeshIter();
    // Default operator=() and copy_constructor are safe.
    int next();                 // ret: success
    int prev();                 // same; die if !_pmrs.is_reversible()
    int goto_nvertices(int nvertices); // ret: success
    int goto_nfaces(int nfaces);       // ret: success (within +-1, favor 0/-1)
    const PMeshRStream& rstream() const;
    PMeshRStream& rstream();
 private:
friend class Geomorph;
friend class PMesh;             // for PMesh::truncate_*()
    PMeshRStream& _pmrs;
    int nextA(Ancestry* ancestry);
    int goto_nverticesA(int nvertices, Ancestry* ancestry);
    int goto_nfacesA(int nfaces, Ancestry* ancestry);
};

// Records vertex and wedge ancestry during PM traversal in order to
//  construct geomorphs.
class Ancestry {
 public:
    Array<PMVertexAttrib> _vancestry;
    Array<PMWedgeAttrib> _wancestry;
};

// Geomorph endstates: pair of attributes for a changing vertex.
struct PMVertexAttribG {
    int vertex;
    PMVertexAttrib attribs[2];
};
ARRAY_BIT_COPY(PMVertexAttribG);

// Geomorph endstates: pair of attributes for a changing wedge.
struct PMWedgeAttribG {
    int wedge;
    PMWedgeAttrib attribs[2];
};
ARRAY_BIT_COPY(PMWedgeAttribG);

// A geomorph is a mesh which is able to smoothly transition between two
//  endstates.
// It is a mesh together with a list of vertices and wedges that change and
//  their endstates.
// It can be evaluated over a continuous blend parameter 0<=alpha<=1.
// It is created by applying a sequence of vsplits to a PM iterator.
class Geomorph : public WMesh {
 public:
    Geomorph();
    ~Geomorph();
    // Default operator=() and copy_constructor are safe.
// Construction
    // Create a geomorph from pmi's current mesh to the mesh obtained
    //  after applying n vsplits to pmi.
    // Note side-effect on pmi!
    // Ret: was_able_to_go_all_the_way; die if !empty
    int construct_next(PMeshIter& pmi, int nvsplits);
    // Same up to nvertices
    // Ret: success
    int construct_goto_nvertices(PMeshIter& pmi, int nvertices);
    // Same up to nfaces (or nfaces-1)
    // Ret: success
    int construct_goto_nfaces(PMeshIter& pmi, int nfaces);
// Evaluation
    // Modify each vertex and wedge attributes of this mesh by linearly
    //  interpolating between attribs[0] and attribs[1].
    // Could optimize this by:
    //  (1) creating a list of only the changing attributes  (done)
    //  (2) precomputing (attribs[1]-attribs[0])
    //  (3) renormalizing only normals that vary significantly
    void evaluate(float alpha); // 0<=alpha<=1 (0==coarse)
 private:
friend class SGeomorph;
    Array<PMVertexAttribG> _vgattribs;
    Array<PMWedgeAttribG> _wgattribs;
    enum Type { WANT_VSPLITS, WANT_NVERTICES, WANT_NFACES };
    int construct(PMeshIter& pmi, Type type, int num);
};

// A simple mesh is one in which wedges are converted to vertices:
//  faces -> vertices
// This means that geometry information (vertex positions) may be duplicated.
// The PM representation should not operate on such meshes directly because
//  vsplits would tear the surface (unless complicated constraints were
//  introduced to keep duplicated vertices together which is essentially what
//  WMesh does.)

// Simple vertex attributes.
struct PMSVertexAttrib {
    PMVertexAttrib v;
    PMWedgeAttrib w;
};
ARRAY_BIT_COPY(PMSVertexAttrib);

void interp(PMSVertexAttrib& a, const PMSVertexAttrib& a1,
            const PMSVertexAttrib& a2, float f1, float f2);

struct PMSVertex {
    PMSVertexAttrib attrib;
};
ARRAY_BIT_COPY(PMSVertex);

struct PMSFace {
    int vertices[3];
    PMFaceAttrib attrib;
};
ARRAY_BIT_COPY(PMSFace);

struct PMSVertexAttribG {
    int vertex;
    PMSVertexAttrib attribs[2];
};
ARRAY_BIT_COPY(PMSVertexAttribG);

// Simple mesh: faces -> vertices.
// Split wedges into independent vertices.
class SMesh {
 public:
    SMesh(const WMesh& wmesh);
    ~SMesh();
    // Default operator=() and copy_constructor are safe.
    void extract_gmesh(GMesh& gmesh, int has_rgb, int has_uv) const;
 public:
    Materials _materials;
    Array<PMSVertex> _vertices;
    Array<PMSFace> _faces;
};

// Simple geomorph.
// Efficiency trade-off:
// - a simple mesh is quicker to render since there is one less level of
//    indirection (no wedges).
// - but, it may be more expensive to geomorph since duplicated vertex
//    positions must be interpolated separately.
class SGeomorph : public SMesh {
 public:
    SGeomorph(const Geomorph& geomorph);
    ~SGeomorph();
    // Default operator=() and copy_constructor are safe.
    void evaluate(float alpha); // 0<=alpha<=1 (0==coarse)
    void extract_gmesh(GMesh& gmesh, int has_rgb, int has_uv) const;
 private:
    Array<PMSVertexAttribG> _vgattribs;
};

//----------------------------------------------------------------------------

inline int Vsplit::adds_two_faces() const
{
    return vlr_offset1>1;
}

#if !defined(NDEBUG)

inline int AWMesh::get_jvf(int v, int f) const
{
    ASSERTX(_vertices.ok(v));
    ASSERTX(_faces.ok(f));
    ForIndex(j,3) {
        if (_wedges[_faces[f].wedges[j]].vertex==v) return j;
    } EndFor;
    assertnever("");
    return 0;
}

inline int AWMesh::get_wvf(int v, int f) const
{
    return _faces[f].wedges[get_jvf(v,f)];
}

#else

inline int AWMesh::get_jvf(int v, int f) const
{
//     return (_wedges[_faces[f].wedges[0]].vertex==v ? 0 :
//             _wedges[_faces[f].wedges[1]].vertex==v ? 1 :
//             2);
    return ((_wedges[_faces[f].wedges[1]].vertex==v)+
            (_wedges[_faces[f].wedges[2]].vertex==v)*2);
}

inline int AWMesh::get_wvf(int v, int f) const
{
    int w0=_faces[f].wedges[0];
    if (_wedges[w0].vertex==v) return w0;
    int w1=_faces[f].wedges[1];
    if (_wedges[w1].vertex==v) return w1;
    int w2=_faces[f].wedges[2];
    return w2;
}

#endif

#define ForAWVertexFace(mesh,v,f,ff) \
{int ff=f, __zz_lastf, __zz_stopf; \
do { __zz_lastf=ff; ff=(mesh)._fnei[ff].faces[MOD3((mesh).get_jvf(v,ff)+2)];} while( ff>=0 && ff != f); \
if(ff<0){__zz_stopf = -1; ff = __zz_lastf;} else ff=__zz_stopf = f; \
for(;ff>=0; __zz_lastf=(mesh)._fnei[ff].faces[MOD3((mesh).get_jvf(v,ff)+1)], ff = (__zz_lastf<0 || __zz_lastf==__zz_stopf)?-1:__zz_lastf) { 

#define ForAWVertexVertex(mesh,v,f,vv,ff)  \
{int vv, ff=f,     __zz_lastf, __zz_nextv, __zz_stopv, __zz_j; \
do { __zz_lastf=ff; ff=(mesh)._fnei[ff].faces[MOD3((mesh).get_jvf(v,ff)+2)];} while( ff>=0 && ff != f); \
if(ff<0) ff=__zz_lastf; __zz_j = (mesh).get_jvf(v,ff); \
__zz_stopv = vv = (mesh)._wedges[(mesh)._faces[ff].wedges[MOD3(__zz_j+1)]].vertex; \
__zz_nextv = (mesh)._wedges[(mesh)._faces[ff].wedges[MOD3(__zz_j+2)]].vertex; \
for(;vv >= 0 ;\
vv = __zz_nextv, __zz_lastf = ff, ff=(mesh)._fnei[ff].faces[MOD3(__zz_j+1)], \
ff<0? (__zz_nextv =-1, ff=__zz_lastf) : ( \
__zz_nextv = (mesh)._wedges[(mesh)._faces[ff].wedges[MOD3((__zz_j=(mesh).get_jvf(v,ff))+2)]].vertex, \
(__zz_nextv==__zz_stopv?__zz_nextv=-1:0))){

#endif
