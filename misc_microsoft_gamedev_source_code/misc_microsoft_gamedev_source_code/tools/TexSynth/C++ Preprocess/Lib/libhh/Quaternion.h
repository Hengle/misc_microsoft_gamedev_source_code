// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Quaternion_h
#define Quaternion_h

#include "Geometry.h"

class Quaternion {
 public:
    Quaternion() { }
    Quaternion(const Quaternion& q);
    Quaternion(const Frame& f); // Frame origin f.p is ignored!
    Quaternion(const Vector& axis, float angle);
    Quaternion(const Vector& vf, const Vector& vt);
                                // resulting quaternion is 2*rotation vf->vt!
    void zero();
    // extraction
    void angle_axis(float& angle, Vector& axis) const;
    float angle() const;
    Vector axis() const;        // axis will be zero vector if angle==0
    // multiplication
    friend Quaternion operator*(const Quaternion& q1,
                                const Quaternion& q2);
    Quaternion& operator*=(const Quaternion& q);
    // multiplicative inverse
    friend inline void invert(const Quaternion& qi, Quaternion& qo);
    Quaternion& invert();
    friend inline Quaternion inverse(const Quaternion& q);
    // Frame origin is set to zero!
    friend Frame toFrame(const Quaternion& q);
    // Power of Quaternion
    friend void pow(const Quaternion& qi, float f, Quaternion& qo);
    friend Quaternion pow(const Quaternion& qi, float f);
    // Power of Frame
    friend void pow(const Frame& fi, float f, Frame& fo);
    friend Frame pow(const Frame& fi, float f);
    // ? log(Quaternion) == ?what type.   a Vector? vo[3]==0!
    friend void log(const Quaternion& qi, Vector& vo);
    friend Vector log(const Quaternion& q);
    // ? meaning of exponentiation
    friend void exp(const Vector& v, Quaternion& qo);
    friend Quaternion exp(const Vector& v);
    // NOTE:    pow(qi,f) ==
    //          slerp(Quaternion(Vector(0,0,0),0),qi,f) ==
    //          exp(log(qi)*f)
    // spherical linear interpolation of unit quaternion.
    // t:[0..1] qo(0)=q0, qo(1)=q1
    friend Quaternion slerp(const Quaternion& q0,
                            const Quaternion& q1, float t);
    // spherical cubic interpolation of unit quaternion.
    // t:[0..1] qo(0)=q0, qo(1)=q1
    // a and b are intermediate quaternions to form Bezier-like curve.
    friend Quaternion squad(const Quaternion& q0, const Quaternion& a,
                            const Quaternion& b, const Quaternion& q1,
                            float t);
    // hermite-like segment.  again: qo(0)=q0, qo(1)=q1
    // but simulate qo(-1)=qb, qo(2)=qa
    friend Quaternion squadseg(const Quaternion* qb, const Quaternion& q0,
                               const Quaternion& q1, const Quaternion* qa,
                               float t);
    void OK() const;
    friend ostream& operator<<(ostream& s, const Quaternion& q);
 private:
    float _c[4];
    //
    float& operator[](int i) { return _c[i]; }
    const float& operator[](int i) const { return _c[i]; }
    Quaternion(float x, float y, float z, float w) {
        _c[0]=x; _c[1]=y; _c[2]=z; _c[3]=w;
    }
    friend inline float mag2(const Quaternion& q) {
        return q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3];
    }
    friend inline float mag(const Quaternion& q) { return sqrt(mag2(q)); }
    friend inline void normalize(const Quaternion& qi, Quaternion& qo);
    inline void normalize();
    // ? meaning of dot
    friend inline float dot(const Quaternion& q1, const Quaternion& q2)
    { return q1[0]*q2[0]+q1[1]*q2[1]+q1[2]*q2[2]+q1[3]*q2[3]; }
};

//----------------------------------------------------------------------------

inline Quaternion::Quaternion(const Quaternion& q)
{
    // *this=q doesn't work always
    _c[0]=q._c[0]; _c[1]=q._c[1]; _c[2]=q._c[2]; _c[3]=q._c[3];
}

inline Quaternion::Quaternion(const Vector& axis, float angle)
{
    float a=mag(axis);
    a=a?sinf(angle*.5f)/a:1.f;
    _c[0]=axis[0]*a; _c[1]=axis[1]*a; _c[2]=axis[2]*a;
    _c[3]=cosf(angle*.5f);
}

inline Quaternion::Quaternion(const Vector& vf, const Vector& vt)
{
    _c[3]=dot(vf,vt);
    Vector vc=cross(vf,vt);
    _c[0]=vc[0],_c[1]=vc[1],_c[2]=vc[2];
}

inline void Quaternion::OK() const
{
    assertx(abs(mag2(*this)-1)<=1e-6);
}

inline void Quaternion::zero()
{
    _c[0]=_c[1]=_c[2]=0; _c[3]=1;
}


inline void normalize(const Quaternion& qi, Quaternion& qo)
{
    float a=mag(qi);
    assertw(a);
    ForIndex(i,4) { qo[i]=qi[i]/a; } EndFor;
}

inline void Quaternion::normalize()
{
    ::normalize(*this,*this); 
}

inline void Quaternion::angle_axis(float& angle, Vector& axis) const
{
    OK();
    angle=myacos(_c[3])*2;
    float a=sinf(angle*.5f);
    a=a?1.f/a:1.f;
    axis=Vector(_c[0]*a,_c[1]*a,_c[2]*a);
}

inline float Quaternion::angle() const
{
    OK();
    return myacos(_c[3])*2;
}

inline Vector Quaternion::axis() const
{
    OK();
    float a=mysqrt(1.f-_c[3]*_c[3]); // ==sinf(angle()*.5)
    a=a?1.f/a:1.f;
    return Vector(_c[0]*a,_c[1]*a,_c[2]*a);
}

inline void invert(const Quaternion& qi, Quaternion& qo)
{
    qi.OK();
    qo[0]=-qi[0]; qo[1]=-qi[1]; qo[2]=-qi[2]; qo[3]=qi[3];
}

inline Quaternion& Quaternion::invert()
{
    ::invert(*this,*this); return *this;
}

inline Quaternion inverse(const Quaternion& q)
{
    Quaternion qr; invert(q,qr); return qr;
}

#endif
