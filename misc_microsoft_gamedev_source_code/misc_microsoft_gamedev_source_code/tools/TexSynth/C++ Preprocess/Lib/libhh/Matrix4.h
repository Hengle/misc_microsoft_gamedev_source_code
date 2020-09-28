// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Matrix4_h
#define Matrix4_h

class Matrix4;

class Vector4 {
 public:
    Vector4() { }
    Vector4(float x, float y, float z, float w);
    float& operator[](int i) { return _c[i]; }
    const float& operator[](int i) const { return _c[i]; }
    friend inline Vector4 operator+(const Vector4& v1, const Vector4& v2);
    friend inline Vector4 operator-(const Vector4& v1, const Vector4& v2);
    Vector4& operator+=(const Vector4& v);
    Vector4& operator-=(const Vector4& v);
    friend inline Vector4 operator*(const Vector4& v, float f);
    friend inline Vector4 operator*(float f, const Vector4& v);
    Vector4& operator*=(float f);
    friend inline Vector4 operator/(const Vector4& v, float f);
    Vector4& operator/=(float f);
    friend Vector4 operator*(const Vector4& v, const Matrix4& m);
    friend Vector4 operator*(const Matrix4& m, const Vector4& v);
    Vector4& operator*=(const Matrix4& m);
    friend ostream& operator<<(ostream& s, const Vector4& v);
 protected:
    float _c[4];
};

class Matrix4 {
 public:
    Matrix4() { }
    Matrix4(const Vector4& v0, const Vector4& v1,
            const Vector4& v2, const Vector4& v3);
    Vector4& operator[](int i) { return _v[i]; }
    const Vector4& operator[](int i) const { return _v[i]; }
    void zero();
    void ident();
    friend Matrix4 operator*(const Matrix4& m1, const Matrix4& m2);
    Matrix4& operator*=(const Matrix4& m);
    friend int invert(const Matrix4& mi, Matrix4& mo);
    friend Matrix4 inverse(const Matrix4& m);
    Matrix4 operator~() const;
    Matrix4& transpose();
    friend Matrix4 transpose(const Matrix4& m);
    friend ostream& operator<<(ostream& s, const Matrix4& m);
    Matrix4& fix();             // make affine if close to affine
 protected:
    Vector4 _v[4];
};

//----------------------------------------------------------------------------

// *** Vector4

inline Vector4::Vector4(float x, float y, float z, float w)
{
    _c[0]=x; _c[1]=y; _c[2]=z; _c[3]=w;
}

inline Vector4 operator+(const Vector4& v1, const Vector4& v2)
{
    return Vector4(v1._c[0]+v2._c[0],v1._c[1]+v2._c[1],
                   v1._c[2]+v2._c[2],v1._c[3]+v2._c[3]);
}

inline Vector4 operator-(const Vector4& v1, const Vector4& v2)
{
    return Vector4(v1._c[0]-v2._c[0],v1._c[1]-v2._c[1],
                   v1._c[2]-v2._c[2],v1._c[3]-v2._c[3]);
}

inline Vector4& Vector4::operator+=(const Vector4& v)
{
    _c[0]+=v._c[0]; _c[1]+=v._c[1]; _c[2]+=v._c[2]; _c[3]+=v._c[3];
    return *this;
}

inline Vector4& Vector4::operator-=(const Vector4& v)
{
    _c[0]-=v._c[0]; _c[1]-=v._c[1]; _c[2]-=v._c[2]; _c[3]-=v._c[3];
    return *this;
}

inline Vector4 operator*(const Vector4& v, float f)
{
    return Vector4(v._c[0]*f,v._c[1]*f,v._c[2]*f,v._c[3]*f);
}

inline Vector4 operator*(float f, const Vector4& v)
{
    return Vector4(v._c[0]*f,v._c[1]*f,v._c[2]*f,v._c[3]*f);
}

inline Vector4& Vector4::operator*=(float f)
{
    _c[0]*=f; _c[1]*=f; _c[2]*=f; _c[3]*=f; return *this;
}

inline Vector4 operator/(const Vector4& v, float f)
{
    return v*(1.f/f);
}

inline Vector4& Vector4::operator/=(float f)
{
    return *this*=(1.f/f);
}

// *** Matrix4

inline Matrix4::Matrix4(const Vector4& v0, const Vector4& v1,
                        const Vector4& v2, const Vector4& v3)
{
    _v[0]=v0; _v[1]=v1; _v[2]=v2; _v[3]=v3;
}

#endif
