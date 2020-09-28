// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef A3dStream_h
#define A3dStream_h

#include "Geometry.h"
#include "Array.h"

class A3dColor {
 public:
    A3dColor() { }
    A3dColor(float r, float g, float b) { _c[0]=r; _c[1]=g; _c[2]=b; }
    float& operator[](int i) { return _c[i]; }
    const float& operator[](int i) const { return _c[i]; }
    friend int compare(const A3dColor& c1, const A3dColor& c2);
    friend int compare(const A3dColor& c1, const A3dColor& c2, float tol);
    friend ostream& operator<<(ostream& s, const A3dColor& c);
 private:
    float _c[3];                 // r,g,b
};

class A3dVertexColor {
 public:
    A3dVertexColor() { }
    A3dVertexColor(const A3dColor& pd); // specular=white, phong=1
    A3dVertexColor(const A3dColor& pd, const A3dColor& ps,
                   const A3dColor& pg);
    A3dVertexColor(const A3dVertexColor& v) : d(v.d),s(v.s),g(v.g) {}
    A3dColor d;
    A3dColor s;
    A3dColor g;
    friend int compare(const A3dVertexColor& c1, const A3dVertexColor& c2);
    static const A3dVertexColor White,Black,Red,Green,Blue;
};

class A3dVertex {
 public:
    A3dVertex() { }
    A3dVertex(const Point& pp, const Vector& pn, const A3dVertexColor& pc);
    Point p;
    Vector n;
    A3dVertexColor c;
};

// I thought about making this a base class for different types of elements,
// but thought it would be too inefficient to allocate/deallocate every time.
class A3dElem {
 public:
    enum Type { TPolygon='P', TPolyline='L', TPoint='p',
                TComment='#',
                TEndObject='o', TEndFrame='f', TEndFile='q',
                TEditObject='O' };
    // also reserved: 'v','E','n','d','s','g'
    A3dElem();
    A3dElem(Type type, int binary=0, int nv=0); // allocates AND init()
    ~A3dElem();
    // both A3dElem(..nv) and init() allocate and initialize for nv
    void init(Type type, int binary=0, int nv=0);
    void update(Type type, int binary=0); // polygon<>polyline<>point
    void copy(const A3dElem& e);          // deep copy, shallow copy not allowed
    Type type() const;
    void setbinary(int b);
    int binary() const;
    static int statustype(Type type);
    static int commandtype(Type type);
// for TPolygon || TPolyline || TPoint:
    int num() const;
    void addvertices(int i);
    A3dElem& operator+=(const A3dVertex& vertex);
    A3dVertex& operator[](int i);
    const A3dVertex& operator[](int i) const;
    Vector pnormal() const;     // may be degenerate (zero)!
// for TComment:
    void setcomment(const char* str); // str may start with ' '
    const char* comment() const;      // user's responsibility to dup string
// for commandtype():
    float& f(int i);
    float f(int i) const;
 private:
    Type _type;
    int _binary;
    Array<A3dVertex> _v;
    union {
        struct {                // TComment
            const char* s;
        } comment;
        struct {                // commandtype()
            float f[3];
        } other;
    } _u;
    void realloc(int nv);
    DISABLE_COPY(A3dElem);
};

class A3dStream {
 protected:
    A3dStream();
    virtual ~A3dStream();
    A3dVertexColor curcol;
    void setcurcolor(int type, const float f[3]);
    enum { LINELENGTH=1023 };   // max length of ascii line
    enum { BINARYCODE=3 };
 private:
    DISABLE_COPY(A3dStream);
};


class RA3dStream : public A3dStream {
 public:
    RA3dStream();
    ~RA3dStream();
    void read(A3dElem& e);
 protected:
    virtual int readline(int& binary, int& type, float f[3],
                         const char*& comment)=0; // ret success
};

class RSA3dStream : public RA3dStream { // Read from stream
 public:
    RSA3dStream(istream& pis);
    ~RSA3dStream();
    istream& is();
 protected:
    int readline(int& binary, int& type, float f[3], const char*& comment);
 private:
    istream& _is;
};


class WA3dStream : public A3dStream {
 public:
    WA3dStream();
    ~WA3dStream();
    void write(const A3dElem& e);
    void writeComment(const char* string); // can contain newlines
    void writeEndObject(int binary=0, float f0=1, float f1=1);
    void writeClearObject(int binary=0, float f0=1, float f1=0);
    void writeEndFrame(int binary=0);
    virtual void flush()=0;
 protected:
    int oldformat;              // old a3d format
    virtual void output(int binary, int type,
                        float f1, float f2, float f3)=0;
    virtual void outputcomment(const char* s)=0;
    virtual void blankline()=0;
 private:
    int _first;                 // first write?
    int _forcechoicebinary;     // force choice one way or the other
    int _choicebinary;          // which way is forced
    int _pblank;                // previous element left a blank line
    //
    void writeoldformat(const A3dElem& e);
};

class WSA3dStream : public WA3dStream { // Write to stream
 public:
    WSA3dStream(ostream& pos);
    ~WSA3dStream();
    void flush();
    ostream& os();
 protected:
    void output(int binary, int type, float f1, float f2, float f3);
    void outputcomment(const char* s);
    void blankline();
 private:
    ostream& _os;
};

//------------------------------------------------------------------------
//
// NAME
//   A3d - new ascii 3D file format
//
// DESCRIPTION
//   The format is a list of records.  Each record contains a character code
//   and three floating point numbers.
//   Here is a description:
//
//    # Comments begin with a '#' in the first column.
//    
//    # Geometric primitives
//    #  polygon
//    P 0 0 0
//    v x1 y1 z1
//    v x2 y2 z2
//    ...
//    E 0 0 0
//    #  polyline
//    L 0 0 0
//    v x1 y1 z1
//    v x2 y2 z2
//    ...
//    E 0 0 0
//    #  point
//    p x y z
//
//    # State information
//    #  diffuse color  (r,g,b between 0. and 1.)
//    d r g b
//    #  specular color
//    s r g b
//    #  phong factor
//    g phong 0 0
//    #  normal  (should be normalized)
//    n vx vy vz
//    # The color state is permanent
//    #   (except the colors are reset to zero at End{Object,Frame,File})
//    # The normal should precede a (v)ertex or a (p)oint and applies only to
//    #  that vertex/point.  (lack of normal is equivalent to zero normal)
//
//    # Special commands (for advanced use)
//    #  EndObject: ends current object and selects a new object number
//    #  (works like seek(2): int_code=0 -> beg, int_code=1 -> relative, ...)
//    #  (ex.: 'o 1 1 0' select next object, 'o 0 3 0' select object 3)
//    o int_code int_disp 0
//    #  EndFrame (used by viewer to determine when to update screen)
//    f 0 0 0
//    #  EndFile (the remainder of the stream is no longer a3d input)
//    q 0 0 0
//    #  EditObject: clears an object (same format as EndObject)
//    O int_code int disp 0
//    # Geometric primitives are always appended to the current object.
//    # Use EndObject to select a different object, and EditObject to clear an
//    # object before replacing its definition.


//----------------------------------------------------------------------------

// *** A3dVertexColor

inline A3dVertexColor::A3dVertexColor(const A3dColor& pd)
: d(pd), s(1,1,1), g(1,0,0) { }

inline A3dVertexColor::A3dVertexColor(const A3dColor& pd, const A3dColor& ps,
                                      const A3dColor& pg)
: d(pd), s(ps), g(pg) { }

// *** A3dVertex

inline A3dVertex::A3dVertex(const Point& pp, const Vector& pn,
                            const A3dVertexColor& pc)
: p(pp), n(pn), c(pc) { }

// *** A3dElem

inline A3dElem::A3dElem() : _type(TPolygon), _binary(0) { }

inline int A3dElem::statustype(Type type)
{
    return type=='d' || type=='s' || type=='g';
}

inline int A3dElem::commandtype(Type type)
{
    return type==A3dElem::TEndObject || type==A3dElem::TEndFrame ||
        type==A3dElem::TEndFile || type==A3dElem::TEditObject;
}

inline A3dElem::Type A3dElem::type() const { return _type; }

inline int A3dElem::num() const { return _v.num(); }

inline A3dElem& A3dElem::operator+=(const A3dVertex& vertex)
{ _v+=vertex; return *this; }

inline A3dVertex& A3dElem::operator[](int i) { return _v[i]; }

inline const A3dVertex& A3dElem::operator[](int i) const { return _v[i]; }

#endif
