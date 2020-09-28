// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef SRMesh_h
#define SRMesh_h

#include "Bbox.h"
#include "EList.h"
#include "LinearFunc.h"
#include "Pool.h"
#include "Map.h"
#include "Materials.h"

class PMeshRStream;
class GMesh;

// True if input *.pm file was not created with '-minii2 -no_fit_geom'
// #define SR_NO_VSGEOM

// True if all normals are Vector(0,0,1), so no need to store them.
// #define SR_NOR001

// True if all matid in Vsplits are predicted correctly from fn[1] and fn[3].
#define SR_PREDICT_MATID

// Do not send render faces if provably outside frustum (see notes in .cxx)
// #define SR_SW_CULLING

// Almost never defined.
// #define ANALYZE_PM_COMPRESSION_WITH_XIA_VARSHNEY

#if defined(SR_NO_VSGEOM)
#undef SR_NOR001
#undef SR_PREDICT_MATID
#endif

struct SRAVertex;               // foward reference
struct SRAFace;                 // foward reference

struct SRVertexGeometry {
    Point point;
#if !defined(SR_NOR001)
    Vector vnormal;
#else
    static Vector vnormal;      // always (0,0,1)
#endif
};

struct SRVertexMorph {
    short coarsening;           // boolean, ==!refining
    short time;                 // time-1 left to achieve goal (always >=0)
    SRVertexGeometry vgrefined; // right child refined geometry
    SRVertexGeometry vginc;     // increment to add during morph
    POOL_ALLOCATION(SRVertexMorph);
};
INITIALIZE_POOL(SRVertexMorph);

struct SRVertex {
    SRAVertex* avertex;         // 0 if not active
    SRVertex* parent;           // 0 if in M^0
    int vspli;                  // -1 if in M^n
};

struct SRAVertex {
    EListNode activev;          // at offset 0 for fast pointer conversion
    SRVertex* vertex;
    SRVertexGeometry vgeom;
    SRVertexMorph* vmorph;      // 0 if no morph
    int visible;                // was vertex visible when last traversed?
    int cached_time;            // for transparent vertex caching
    POOL_ALLOCATION(SRAVertex);
};
INITIALIZE_POOL(SRAVertex);

struct SRFace {
    SRAFace* aface;             // 0 if not active, ==&_isolated_aface if !fr
};

struct SRAFace {
    EListNode activef;          // at offset 0 for fast pointer conversion
    int matid;                  // see notes below.  up here for cache line
    SRAVertex* vertices[3];
    SRAFace* fnei[3];           // &_isolated_aface if no neighbor
    // Notes on matid:
    //  - a high bit is used for FACE_VISITED_MASK in SRMesh::*_render_*
    //  - it is used as temporary 'int' storage in SRMesh::read_pm()
};

struct SRAFacePair {
    // Note that these two faces generally become unconnected thru refinement.
    SRAFace pair[2];
    POOL_ALLOCATION(SRAFacePair);
};
INITIALIZE_POOL(SRAFacePair);

struct SRVsplit {
    SRVertexGeometry vu_vgeom;
#if defined(SR_NO_VSGEOM)
    SRVertexGeometry vs_vgeom, vt_vgeom;
#endif
    // fn[0..3]==flccw,flclw,frclw,frccw
    SRFace* fn[4];              // ==&_inactive_face if no face expected
#if !defined(SR_PREDICT_MATID)
    short fl_matid;
    short fr_matid;
#endif
// Refinement criteria information
#if !defined(SR_NOR001)
    float uni_error_mag2;       // magnitude2 of uniform residual error
#else
    static float uni_error_mag2; // always 0
#endif
    float dir_error_mag2;       // magnitude2 of directional residual error
    float radiusneg;            // max radius of influence (negated)
#if !defined(SR_NOR001)
    float sin2alpha;            // backface region, square(sinf(alpha))
#else
    static float sin2alpha;     // always 0
#endif
};

struct SRRefineParams {
    // Frame _framei;           // to compute screen-space projections.
    int _nplanes;               // number of view frustum planes
    LinearFunc _planes[6];      // view frustum planes
    Point _eye;                 // eyepoint in world coordinates (frame.p)
    float _tz2;                 // screen-space tolerance zoomed and squared
    LinearFunc _eyedir;         // linear func along view direction
};

class SRGeomorphInfo {
 public:
    SRGeomorphInfo();
    ~SRGeomorphInfo();
 private:
friend class SRMesh;
    Map<SRVertex*,SRVertexGeometry*> _ancestors[2];
};

class SRViewParams {
 public:
    SRViewParams();             // set defaults
    // frame.p = eyepoint ; frame.v[0]=view_direction; frame.v[1]=left_dir;
    //  frame.v[2]=top_dir   (frame must be orthonormal!)
    void set_frame(const Frame& frame); // no default
    // zoomx is tanf(angle) where angle is between frame.v[0] and left edge
    //  of viewport
    // zoomy is tanf(angle) where angle is between frame.v[0] and top edge
    //  of viewport
    void set_zooms(float zoomx, float zoomy); // no default
    // enable left/right view frustum planes
    void activate_lr_planes(int flag); // default 1
    // enable bottom view frustum plane
    void activate_bottom_plane(int flag); // default 1
    // enable top view frustum plane
    void activate_top_plane(int flag); // default 1
    // hither distance (along frame.v[0] from frame.p).  set <0.f to disable
    void set_hither(float hither); // default -1.f
    // yonder distance (along frame.v[0] from frame.p).  set <0.f to disable
    void set_yonder(float yonder); // default -1.f
    // screen-space tolerance is screen_thresh*min_window_diameter*0.5f
    // in other words, screen-space tolerance in pixels is
    //  screen_thresh*min(nxpixels,nypixels)*0.5f
    void set_screen_thresh(float screen_thresh); // default 0.f
 private:
    Frame _frame;
    float _zoomx;
    float _zoomy;
    int _activate_lr_planes;
    int _activate_bottom_plane;
    int _activate_top_plane;
    float _hither;
    float _yonder;
    float _screen_thresh;
friend class SRMesh;
    int OK() const;
};

#if 0
// Usage:
{
    0 ? read_pm() : read_srm();
    if (0) write_srm();
    set_refine_morph_time();
    set_coarsen_morph_time();
    for (;;) {
        set_view_params();
        if (0) {
            SRGeomorphInfo geoinfo;
            construct_geomorph(geoinfo);
            extract_gmesh(gmesh,geoinfo);
        }
        adapt_refinement();
        ogl_render_*();
        if (0) extract_gmesh(gmesh);
        if (!(is_still_morphing() || is_still_adapting()))
            pause_for_input();
    }
}
#endif

class SRMesh {
 public:
    SRMesh();
    ~SRMesh();
    // SRMesh must be empty prior to read_*().  SRMesh is left coarsened.
    void read_pm(PMeshRStream& pmrs);
    void read_srm(istream& is);
    void write_srm(ostream& os) const; // must be fully_coarsened.
    void fully_refine();
    void fully_coarsen();
    void set_refine_morph_time(int refine_morph_time); // 0 = disable
    void set_coarsen_morph_time(int coarsen_morph_time); // 0 = disable
    void set_view_params(const SRViewParams& vp);
    void adapt_refinement(int nvtraverse=BIGINT);
    int is_still_morphing() const;
    int is_still_adapting() const;
    int num_vertices_refine_morphing() const;
    int num_vertices_coarsen_morphing() const;
    void construct_geomorph(SRGeomorphInfo& geoinfo);
    void extract_gmesh(GMesh& gmesh) const;
    void extract_gmesh(GMesh& gmesh, const SRGeomorphInfo& geoinfo) const;
    int num_active_vertices() const;
    int num_active_faces() const;
    void OK() const;
    const Bbox& get_bbox() const { return _bbox; }
// Interface: Rendering using OpenGL
    void ogl_render_faces_individually(int unlit_texture);
    void ogl_render_faces_strips(int unlit_texture);
    int ogl_render_striplines(); // return number of strips
    void ogl_render_edges();
    void ogl_show_radii();
    void ogl_show_residuals(int uniform_too);
    void ogl_render_faces_tvc(int unlit_texture);
    int ogl_render_tvclines(); // return number of cache misses
// Interface: Rendering using Direct3D
    void d3d_render_faces_individually(int unlit_texture);
    void d3d_render_faces_strips(int unlit_texture);
    int d3d_render_striplines(); // return number of strips
    void d3d_render_edges();
 private:
    Bbox _bbox;
    Materials _materials;
    SArray<SRVertex> _vertices;
    SArray<SRFace> _faces;
    SArray<SRVsplit> _vsplits;
    SArray<SRAVertex> _base_vertices;
    SArray<SRAFace> _base_faces;
    EList _active_vertices;
    EList _active_faces;
    int _nactivevertices;
    int _nactivefaces;
    SRVertex* _quick_first_vt;
    SRFace* _quick_first_fl;
    SRVertexGeometry* _quick_first_vg;
    int _refine_morph_time;
    int _coarsen_morph_time;
    int _num_vertices_refine_morphing;
    int _num_vertices_coarsen_morphing;
    int _was_modified;
    int _cache_time;
// Static structs
    // Properties: aface==&_isolated_aface
    static SRFace _isolated_face;   // fn[*] when no expected neighbor
    // Properties: matid==illegal, fnei[*]==&_isolated_aface
    static SRAFace _isolated_aface; // fr->activef when !creates_2faces
// Refinement parameters
    SRViewParams _view_params;
    SRRefineParams _refp;
// Rendering: common code and data
    enum { FACE_VISITED_MASK=1<<30 }; // high bit of matid
    enum { ILLEGAL_MATID=-1 };
    int _cur_frame_mask;        // 0 or FACE_VISITED_MASK
// Rendering: accessor functions
    // main ones
    const Point& get_point(const SRAVertex* va) const;
    const Vector& get_normal(const SRAVertex* va) const;
    // auxiliary ones
    int splitable(const SRAVertex* va) const;
    float get_uni_error_mag2(const SRAVertex* va) const;
    float get_dir_error_mag2(const SRAVertex* va) const;
    float get_radiusneg(const SRAVertex* va) const;
    float get_sin2alpha(const SRAVertex* va) const;
// Rendering using OpenGL
    Array<uchar> _ogl_mat_byte_rgba; // size is _materials.num()*4 !
    void ogl_process_materials();
    void ogl_render_faces_strips_texture0();
    void ogl_render_faces_strips_texture1();
// Rendering using Direct3D
    // to be defined
 private:
    int get_vf_j0(const SRAVertex* v, const SRAFace* f) const;
    int get_vf_j1(const SRAVertex* v, const SRAFace* f) const;
    int get_vf_j2(const SRAVertex* v, const SRAFace* f) const;
    SRAFace*& get_fnei(SRAFace* f, SRAFace* fn) const;
    SRAFace* rotate_clw(SRAFace* f, SRAVertex* v) const;
    SRAFace* rotate_ccw(SRAFace* f, SRAVertex* v) const;
    SRVertex* get_vt(int vspli) const;
    SRFace* get_fl(int vspli) const;
    SRVertexGeometry* get_vg(int vspli) const;
    int get_vspli(const SRFace* fl) const; // slow, requires integer divide
    int is_splitable(const SRVertex* v) const;
    int has_been_created(const SRVertex* v) const;
    int has_been_split(const SRVertex* v) const;
    int is_active_f(const SRFace* f) const;
    int is_active_v(const SRVertex* v) const;
    int creates_2faces(const SRVsplit* vspl) const;
    const SRVertexGeometry* refined_vg(const SRAVertex* va) const;
    int vspl_legal(const SRVertex* vs) const;
    int ecol_legal(const SRVertex* vt) const; // vt left child of its parent!
    void compute_bspheres(const Array<SRVertexGeometry>& vgeoms);
    void compute_nspheres(const Array<SRVertexGeometry>& vgeoms);
    int is_visible(const SRVertexGeometry* vg, const SRVsplit* vspl) const;
    int big_error(const SRVertexGeometry* vg, const SRVsplit* vspl) const;
    int qrefine(const SRVertex* vs) const;
    int qcoarsen(const SRVertex* vt) const;
    void apply_vspl(SRVertex* vs, EListNode*& pn);
    void apply_ecol(SRVertex* vs, EListNode*& pn);
    void set_initial_view_params();
    void force_vsplit(SRVertex* vsf, EListNode*& n);
    void finish_vmorph(SRAVertex* va);
    void start_coarsen_morphing(SRVertex* vt);
    void abort_coarsen_morphing(SRVertex* vc);
    void perhaps_abort_coarsen_morphing(SRVertex* vc);
    void verify_optimality() const;
    void display_hierarchy_height() const;
    void update_vmorphs();
    int verify_all_faces_visited() const;
    int verify_all_vertices_uncached() const;
#if defined(ANALYZE_PM_COMPRESSION_WITH_XIA_VARSHNEY)
    int SRMesh::get_iflclw(SRVertex* vs);
    void SRMesh::refine_in_best_dflclw_order();
#endif
};

//----------------------------------------------------------------------------

inline int SRMesh::is_splitable(const SRVertex* v) const {
    return v->vspli>=0;
}

inline int SRMesh::num_active_vertices() const { return _nactivevertices; }
inline int SRMesh::num_active_faces() const { return _nactivefaces; }

inline const Point& SRMesh::get_point(const SRAVertex* va) const
{ return va->vgeom.point; }
inline const Vector& SRMesh::get_normal(const SRAVertex* va) const
{ return va->vgeom.vnormal; }
inline int SRMesh::splitable(const SRAVertex* va) const
{ return is_splitable(va->vertex); }
inline float SRMesh::get_uni_error_mag2(const SRAVertex* va) const
{ return _vsplits[va->vertex->vspli].uni_error_mag2; }
inline float SRMesh::get_dir_error_mag2(const SRAVertex* va) const
{ return _vsplits[va->vertex->vspli].dir_error_mag2; }
inline float SRMesh::get_radiusneg(const SRAVertex* va) const
{ return _vsplits[va->vertex->vspli].radiusneg; }
inline float SRMesh::get_sin2alpha(const SRAVertex* va) const
{ return _vsplits[va->vertex->vspli].sin2alpha; }

#endif
