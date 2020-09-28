// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Geometry_h
#define Geometry_h

#include "Array.h"
#include "Pool.h"

class Frame;
class Point;

class Affine {
 public:
    float& operator[](int i) { return _c[i]; }
    const float& operator[](int i) const { return _c[i]; }
    int iszero() const;
    friend ostream& operator<<(ostream& s, const Affine& a);
    POOL_ALLOCATION(Affine);
 protected:
    float _c[3];
    Affine() {};
    Affine(float x, float y, float z);
};

INITIALIZE_POOL(Affine);

class Vector : public Affine {
 public:
    Vector() {}
    Vector(float x, float y, float z);
    friend inline Vector toVector(const Point& p);
    Vector operator-() const;
    friend inline Vector operator+(const Vector& v1, const Vector& v2);
    friend inline Vector operator-(const Vector& v1, const Vector& v2);
    Vector& operator+=(const Vector& v);
    Vector& operator-=(const Vector& v);
    friend inline Vector operator*(const Vector& v, float f);
    friend inline Vector operator*(float f, const Vector& v);
    friend inline Vector operator/(const Vector& v, float f);
    Vector& operator*=(float f);
    Vector& operator/=(float f);
    friend Vector operator*(const Vector& v, const Frame& f);
    Vector& operator*=(const Frame& f);
    friend Vector operator*(const Frame& f, const Vector& normal);
    friend inline Vector cross(const Vector& v1, const Vector& v2);
    friend int normalize(const Vector& vi, Vector& vo);
    int normalize();
    friend Vector normalize(const Vector& v); // or die
    friend Vector oknormalize(const Vector& v);
    friend inline float mag2(const Vector& v);
    friend inline float mag(const Vector& v);
    friend inline bool isunit(const Vector& v);
    friend inline float dot(const Vector& v1, const Vector& v2);
    friend int compare(const Vector& v1, const Vector& v2);
    friend int compare(const Vector& v1, const Vector& v2, float tol);
    friend inline Vector interp(const Vector& v1, const Vector& v2,
                               float f1=.5f);
    friend inline Vector interp(const Vector& v1, const Vector& v2,
                               const Vector& v3, float f1=1.f/3.f,
                               float f2=1.f/3.f);
    friend ostream& operator<<(ostream& s, const Vector& v);
};

class Point : public Affine {
 public:
    Point() {}
    Point(float x, float y, float z);
    friend inline Point toPoint(const Vector& v);
    friend inline Vector operator-(const Point& p1, const Point& p2);
    friend inline Point operator+(const Point& p, const Vector& v);
    friend inline Point operator+(const Vector& v, const Point& p);
    friend inline Point operator-(const Point& p, const Vector& v);
    Point& operator+=(const Vector& v);
    Point& operator-=(const Vector& v);
    friend Point operator*(const Point& p, const Frame& f);
    Point& operator*=(const Frame& f);
    friend inline float pvdot(const Point& p, const Vector& v);
    friend inline float dist2(const Point& p1, const Point& p2);
    friend inline float dist(const Point& p1, const Point& p2);
    friend int compare(const Point& p1, const Point& p2);
    friend int compare(const Point& p1, const Point& p2, float tol);
    friend inline Point interp(const Point& p1, const Point& p2,
                               float f1=.5f);
    friend inline Point interp(const Point& p1, const Point& p2,
                               const Point& p3, float f1=1.f/3.f,
                               float f2=1.f/3.f);
    friend Vector cross(const Point& p1, const Point& p2, const Point& p3);
    friend float area2(const Point& p1, const Point& p2, const Point& p3);
    friend ostream& operator<<(ostream& s, const Point& p);
};

class Frame {
 public:
    Frame() {}
    Frame(const Vector& v0, const Vector& v1,
          const Vector& v2, const Point& pp);
    Vector v[3];
    Point p;                    // comes immediately after v for operator[]!
    //
    void zero();
    void ident();
    Affine& operator[](int i);
    const Affine& operator[](int i) const;
    int isident() const;
    friend Frame operator*(const Frame& f1, const Frame& f2);
    Frame& operator*=(const Frame& f);
    friend int invert(const Frame& fi, Frame& fo);
    int invert();
    friend Frame inverse(const Frame& f);
    Frame operator~() const;
    friend void transpose(const Frame& fi, Frame& fo);
    Frame& transpose();
    friend Frame transpose(const Frame& f);
    void makerighthand();       // flip an axis if necessary
    static Frame translation(const Affine& vv);
    static Frame translation(float x, float y, float z);
    static Frame rotation(int axis, float angle);
    static Frame scaling(float x, float y, float z);
    static Frame scaling(float s);
    static Frame identity();
    friend ostream& operator<<(ostream& s, const Frame& f);
};

class Bary {                    // Barycentric coordinates
 public:
    Bary() { }
    Bary(float x, float y, float z);
    float& operator[](int i) { return _p[i]; }
    const float& operator[](int i) const { return _p[i]; }
    inline bool isconvex() const;
    friend inline Bary interp(const Bary& b1, const Bary& b2, float f1=.5f);
    friend ostream& operator<<(ostream& s, const Bary& b);
 private:
    float _p[3];
};

class UV {
 public:
    UV() { }
    UV(float u, float v);
    float& operator[](int i) { return _c[i]; }
    const float& operator[](int i) const { return _c[i]; }
    friend int compare(const UV& uv1, const UV& uv2);
    friend int compare(const UV& uv1, const UV& uv2, float tol);
    friend inline UV interp(const UV& u1, const UV& u2, float f1=.5f);
    friend ostream& operator<<(ostream& s, const UV& uv);
 private:
    float _c[2];
};

extern Point centroid(const Array<Point>& pa);

inline UV toUV(const Vector& v) { ASSERTX(v[2]==0.f); return UV(v[0],v[1]); }
inline Vector toVector(const UV& u) { return Vector(u[0],u[1],0.f); } 

inline UV toUV(const Point& p) { ASSERTX(p[2]==0.f); return UV(p[0],p[1]); }
inline Point toPoint(const UV& u) { return Point(u[0],u[1],0.f); }

// Project v into plane orthogonal to unitdir.
inline Vector projortho(const Vector& v, const Vector& unitdir);

// More robust than acos(dot()) for small angles!
inline float unitvecsangle(const Vector& va, const Vector& vb);

// General affine combination of 4 points.
// Note that interpolation domain is 3-dimensional (non-planar).
// "bary[3]"==1.f-bary[0]-bary[1]-bary[2]
inline Point qinterp(const Point& p1, const Point& p2, const Point& p3,
                     const Point& p4, const Bary& bary);

// Bilinear interpolation within 4 points.
// p3 - p2   v
//  |    |   ^
// p0 - p1   |  ->u
inline Point bilerp(const Point& p0, const Point& p1, const Point& p2,
                    const Point& p3, float u, float v);

// Spherical linear interpolation.  slerp(pa,pb,1.f)==pa.
extern Point slerp(const Point& pa, const Point& pb, float ba);

// Spherical triangle area.
extern float sphtriarea(const Point pa[3]);

// Get barycentric coordinates of point p within triangle pa,
//  or return false if p not in plane of triangle.
extern bool getbary(const Point& p, const Point pa[3], Bary& bary);

// Given vector in plane of triangle, return its barycentric coordinates.
extern Bary vector_bary(const Point pa[3], const Vector& vec);

// Given a triangle and barycentric coordinates, return the vector.
extern Vector bary_vector(const Point pa[3], const Bary& bary);


//----------------------------------------------------------------------------

// *** Affine

inline Affine& Frame::operator[](int i) { return v[i]; }

inline const Affine& Frame::operator[](int i) const { return v[i]; }

inline Affine::Affine(float x, float y, float z) { _c[0]=x; _c[1]=y; _c[2]=z; }

inline int Affine::iszero() const
{
    return !_c[0] && !_c[1] && !_c[2];
}

// *** Vector

inline Vector::Vector(float x, float y, float z) : Affine(x,y,z) { }

inline Vector toVector(const Point& p)
{
    return Vector(p[0],p[1],p[2]);
}

inline Vector Vector::operator-() const
{
    return Vector(-_c[0],-_c[1],-_c[2]);
}

inline Vector operator+(const Vector& v1, const Vector& v2)
{
    return Vector(v1._c[0]+v2._c[0],v1._c[1]+v2._c[1],v1._c[2]+v2._c[2]);
}

inline Vector operator-(const Vector& v1, const Vector& v2)
{
    return Vector(v1._c[0]-v2._c[0],v1._c[1]-v2._c[1],v1._c[2]-v2._c[2]);
}

inline Vector& Vector::operator+=(const Vector& v)
{
    _c[0]+=v._c[0]; _c[1]+=v._c[1]; _c[2]+=v._c[2]; return *this;
}

inline Vector& Vector::operator-=(const Vector& v)
{
    _c[0]-=v._c[0]; _c[1]-=v._c[1]; _c[2]-=v._c[2]; return *this;
}

inline Vector operator*(const Vector& v, float f)
{
    return Vector(v._c[0]*f,v._c[1]*f,v._c[2]*f);
}

inline Vector operator*(float f, const Vector& v)
{
    return Vector(v._c[0]*f,v._c[1]*f,v._c[2]*f);
}

inline Vector operator/(const Vector& v, float f)
{
    return Vector(v._c[0]/f,v._c[1]/f,v._c[2]/f);
}

inline Vector& Vector::operator*=(float f)
{
    _c[0]*=f; _c[1]*=f; _c[2]*=f; return *this;
}

inline Vector& Vector::operator/=(float f)
{
    _c[0]/=f; _c[1]/=f; _c[2]/=f; return *this;
}

inline Vector cross(const Vector& v1, const Vector& v2)
{
    float v1x=v1[0], v1y=v1[1], v1z=v1[2];
    float v2x=v2[0], v2y=v2[1], v2z=v2[2];
    return Vector(v1y*v2z-v1z*v2y, v1z*v2x-v1x*v2z, v1x*v2y-v1y*v2x);
}

inline float mag2(const Vector& v)
{
    return v._c[0]*v._c[0]+v._c[1]*v._c[1]+v._c[2]*v._c[2];
}

inline float mag(const Vector& v)
{
    return sqrt(mag2(v));
}

inline bool isunit(const Vector& v)
{
    return abs(mag2(v)-1.f)<1e-4f;
}

inline float dot(const Vector& v1, const Vector& v2)
{
    return v1._c[0]*v2._c[0]+v1._c[1]*v2._c[1]+v1._c[2]*v2._c[2];
}

inline Vector interp(const Vector& v1, const Vector& v2, float f1)
{
    const float f2=1.f-f1;
    return Vector(f1*v1._c[0]+f2*v2._c[0],
                  f1*v1._c[1]+f2*v2._c[1],
                  f1*v1._c[2]+f2*v2._c[2]);
}

inline Vector interp(const Vector& v1, const Vector& v2, const Vector& v3,
                    float f1, float f2) {
    const float f3=1.f-f1-f2;
    return Vector(f1*v1._c[0]+f2*v2._c[0]+f3*v3._c[0],
                  f1*v1._c[1]+f2*v2._c[1]+f3*v3._c[1],
                  f1*v1._c[2]+f2*v2._c[2]+f3*v3._c[2]);
}

// *** Point

inline Point::Point(float x, float y, float z) : Affine(x,y,z) { }

inline Point toPoint(const Vector& v) { return Point(v[0],v[1],v[2]); }

inline Vector operator-(const Point& p1, const Point& p2)
{
    return Vector(p1._c[0]-p2._c[0],p1._c[1]-p2._c[1],p1._c[2]-p2._c[2]);
}

inline Point operator+(const Point& p, const Vector& v)
{
    return Point(p._c[0]+v[0],p._c[1]+v[1],p._c[2]+v[2]);
}

inline Point operator+(const Vector& v, const Point& p)
{
    return Point(v[0]+p._c[0],v[1]+p._c[1],v[2]+p._c[2]);
}

inline Point operator-(const Point& p, const Vector& v)
{
    return Point(p._c[0]-v[0],p._c[1]-v[1],p._c[2]-v[2]);
}

inline Point& Point::operator+=(const Vector& v)
{
    _c[0]+=v[0]; _c[1]+=v[1]; _c[2]+=v[2]; return *this;
}

inline Point& Point::operator-=(const Vector& v)
{
    _c[0]-=v[0]; _c[1]-=v[1]; _c[2]-=v[2]; return *this;
}

inline float pvdot(const Point& p, const Vector& v)
{
    return p._c[0]*v[0]+p._c[1]*v[1]+p._c[2]*v[2];
}

inline float dist2(const Point& p1, const Point& p2)
{
    float x=p1._c[0]-p2._c[0],y=p1._c[1]-p2._c[1],z=p1._c[2]-p2._c[2];
    return x*x+y*y+z*z;
}

inline float dist(const Point& p1, const Point& p2)
{
    return sqrt(dist2(p1,p2));
}

inline Point interp(const Point& p1, const Point& p2, float f1)
{
    const float f2=1.f-f1;
    return Point(f1*p1._c[0]+f2*p2._c[0],
                 f1*p1._c[1]+f2*p2._c[1],
                 f1*p1._c[2]+f2*p2._c[2]);
// I do not know if this is any better  
//      float x=p2._c[0], y=p2._c[1], z=p2._c[2];
//      return Point((p1._c[0]-x)*f1+x,
//                   (p1._c[1]-y)*f1+y,
//                   (p1._c[2]-z)*f1+z);
}

inline Point interp(const Point& p1, const Point& p2, const Point& p3,
                    float f1, float f2) {
    const float f3=1.f-f1-f2;
    return Point(f1*p1._c[0]+f2*p2._c[0]+f3*p3._c[0],
                 f1*p1._c[1]+f2*p2._c[1]+f3*p3._c[1],
                 f1*p1._c[2]+f2*p2._c[2]+f3*p3._c[2]);
//      float x=p3._c[0], y=p3._c[1], z=p3._c[2];
//      return Point((p1._c[0]-x)*f1+(p2._c[0]-x)*f2+x,
//                   (p1._c[1]-y)*f1+(p2._c[1]-y)*f2+y,
//                   (p1._c[2]-z)*f1+(p2._c[2]-z)*f2+z);
}

// *** Bary

inline Bary::Bary(float x, float y, float z) { _p[0]=x; _p[1]=y; _p[2]=z; }

inline bool Bary::isconvex() const
{
    return (_p[0]>=0.f && _p[0]<=1.f &&
            _p[1]>=0.f && _p[1]<=1.f &&
            _p[2]>=0.f && _p[2]<=1.f);
}

inline Bary interp(const Bary& b1, const Bary& b2, float f1)
{
    Bary bn;
    const float f2=1.f-f1;
    bn[0]=b1[0]*f1+b2[0]*f2;
    bn[1]=b1[1]*f1+b2[1]*f2;
    bn[2]=b1[2]*f1+b2[2]*f2;
    return bn;
}

// *** UV

inline UV::UV(float u, float v) { _c[0]=u; _c[1]=v; }

inline UV interp(const UV& u1, const UV& u2, float f1)
{
    UV un;
    const float f2=1.f-f1;
    un[0]=u1[0]*f1+u2[0]*f2;
    un[1]=u1[1]*f1+u2[1]*f2;
    return un;
}

// *** Misc

inline Vector projortho(const Vector& v, const Vector& unitdir)
{
    ASSERTX(isunit(unitdir));
    return v-unitdir*dot(v,unitdir);
}

inline float unitvecsangle(const Vector& va, const Vector& vb)
{
    ASSERTX(isunit(va) && isunit(vb));
    float vdot=dot(va,vb);
    if (vdot>+.95f) {
        return asinf(mag(cross(va,vb)));
    } else if (vdot<-.95f) {
        return PI-asinf(mag(cross(va,vb)));
    } else {
        return acosf(vdot);
    }
}

inline Point qinterp(const Point& p1, const Point& p2, const Point& p3,
                     const Point& p4, const Bary& bary)
{
    float f1=bary[0],f2=bary[1],f3=bary[2];
    const float f4=1.f-f1-f2-f3;
    return Point(f1*p1[0]+f2*p2[0]+f3*p3[0]+f4*p4[0],
                 f1*p1[1]+f2*p2[1]+f3*p3[1]+f4*p4[1],
                 f1*p1[2]+f2*p2[2]+f3*p3[2]+f4*p4[2]);
}

inline Point bilerp(const Point& p0, const Point& p1, const Point& p2,
                    const Point& p3, float u, float v)
{
    // return qinterp(p[0],p[1],p[2],p[3],Bary((1.f-u)*(1.f-v),u*(1.f-v),u*v));
    return interp(interp(p0,p1,1.f-u),
                  interp(p3,p2,1.f-u),
                  1.f-v);
}

#endif
