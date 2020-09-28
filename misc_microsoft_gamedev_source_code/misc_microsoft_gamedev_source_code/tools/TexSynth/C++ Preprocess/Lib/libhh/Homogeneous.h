// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Homogeneous_h
#define Homogeneous_h

#include "Geometry.h"
#include "Matrix4.h"

class Homogeneous {
 public:
    Homogeneous();
    Homogeneous(float x, float y, float z, float w);
    Homogeneous(const Vector4& v);
    Homogeneous(const Point& p);
    Homogeneous(const Vector& v);
    float& operator[](int i);
    const float& operator[](int i) const;
    int iszero() const;
    friend inline Homogeneous normalize(const Homogeneous& h);
    Homogeneous& normalize();
    friend inline Homogeneous operator*(const Point& p, float f);
    friend inline Homogeneous operator*(float f, const Point& p);
    friend inline Homogeneous operator*(const Homogeneous& h, float f);
    friend inline Homogeneous operator*(float f, const Homogeneous& h);
    Homogeneous& operator*=(float f);
    friend inline Homogeneous operator/(const Homogeneous& h, float f);
    Homogeneous& operator/=(float f);
    friend inline Homogeneous operator*(const Homogeneous& h,
                                        const Matrix4& f);
    Homogeneous& operator*=(const Matrix4& f);
    friend inline Homogeneous operator+(const Homogeneous& h1,
                                        const Homogeneous& h2);
    friend inline Homogeneous operator-(const Homogeneous& h1,
                                        const Homogeneous& h2);
    Homogeneous& operator+=(const Homogeneous& h);
    Homogeneous& operator-=(const Homogeneous& h);
    friend ostream& operator<<(ostream& s, const Homogeneous& h);
 private:
    Vector4 _v;
};

inline Point toPoint(const Homogeneous& h);
inline Vector toVector(const Homogeneous& h);

extern Matrix4 toMatrix4(const Frame& f);
extern Frame toFrame(const Matrix4& m);

//----------------------------------------------------------------------------

// *** Homogeneous

inline Homogeneous::Homogeneous() : _v(0,0,0,0) { }

inline Homogeneous::Homogeneous(float x, float y, float z, float w)
: _v(x,y,z,w) { }

inline Homogeneous::Homogeneous(const Vector4& pv) : _v(pv) { }

inline Homogeneous::Homogeneous(const Point& p) : _v(p[0],p[1],p[2],1) { }

inline Homogeneous::Homogeneous(const Vector& pv): _v(pv[0],pv[1],pv[2],0) { }

inline float& Homogeneous::operator[](int i) { return _v[i]; }
inline const float& Homogeneous::operator[](int i) const { return _v[i]; }

inline int Homogeneous::iszero() const
{
    return !_v[0] && !_v[1] && !_v[2] && !_v[3];
}

inline Homogeneous normalize(const Homogeneous& h)
{
    float a=1.f/assertv(h[3]);
    return Homogeneous(h[0]*a,h[1]*a,h[2]*a,h[3]*a);
}

inline Homogeneous& Homogeneous::normalize()
{
    return *this=::normalize(*this);
}

inline Homogeneous operator*(const Point& p, float f)
{ return Homogeneous(p[0]*f,p[1]*f,p[2]*f,f); }

inline Homogeneous operator*(float f, const Point& p) { return p*f; }

inline Homogeneous operator*(const Homogeneous& h, float f) { return h._v*f; }

inline Homogeneous operator*(float f, const Homogeneous& h) { return h*f; }

inline Homogeneous& Homogeneous::operator*=(float f)
{ return *this=*this*f; }

inline Homogeneous operator/(const Homogeneous& h, float f) { return h._v/f; }

inline Homogeneous& Homogeneous::operator/=(float f)
{ return *this=*this/f; }

inline Homogeneous operator*(const Homogeneous& h, const Matrix4& f)
{ return h._v*f; }

inline Homogeneous& Homogeneous::operator*=(const Matrix4& f)
{ return *this=*this*f; }

inline Homogeneous operator+(const Homogeneous& h1, const Homogeneous& h2)
{ return h1._v+h2._v; }

inline Homogeneous operator-(const Homogeneous& h1, const Homogeneous& h2)
{ return h1._v-h2._v; }

inline Homogeneous& Homogeneous::operator+=(const Homogeneous& h)
{
    return *this=*this+h;
}

inline Homogeneous& Homogeneous::operator-=(const Homogeneous& h)
{
    return *this=*this-h;
}

inline Point toPoint(const Homogeneous& h)
{
    const float TOLERANCE=1e-5f;
    assertx(abs(h[3]-1.f)<TOLERANCE);
    return Point(h[0],h[1],h[2]);
}

inline Vector toVector(const Homogeneous& h)
{
    const float TOLERANCE=1e-5f;
    assertx(abs(h[3])<TOLERANCE);
    return Vector(h[0],h[1],h[2]);
}

#endif
