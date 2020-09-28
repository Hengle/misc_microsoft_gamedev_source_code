/*****************************************************************

Filename    :   GMatrix2D.cpp
Content     :   2D Matrix class implementation 
Created     :   May 29, 2006
Authors     :   Michael Antonov, Brendan Iribe

History     :   
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GMatrix2D.h"
#include "GStd.h"
#include <stdio.h>

#ifdef GFC_TWIPS_TO_PIXELS //!AB???
#undef GFC_TWIPS_TO_PIXELS
#endif
#define GFC_TWIPS_TO_PIXELS(x)  ((x) / 20.f)

#define GFX_STREAM_BUFFER_SIZE  512

//
// GMatrix2D
//

GMatrix2D   GMatrix2D::Identity;

// GFC_ISFINITE() is provided in case the compiler lacks the standard version
#define GFC_ISFINITE(x)  (sizeof(x) == sizeof(Float) ? GFC_ISFINITE_F(x) : GFC_ISFINITE_D(x))
#define GFC_ISFINITE_F(x) ((x) >= -FLT_MAX && (x) <= FLT_MAX)    // NAN should fail this?
#define GFC_ISFINITE_D(x) ((x) >= -DBL_MAX && (x) <= DBL_MAX)



////////////////XM Matrix Helpers///////
#if defined(_XBOX)


XMGLOBALCONST XMVECTOR  g_XMIdentityR0          = {1.0f, 0.0f, 0.0f, 0.0f};
XMGLOBALCONST XMVECTOR  g_XMIdentityR1          = {0.0f, 1.0f, 0.0f, 0.0f};
XMGLOBALCONST XMVECTOR  g_XMIdentityR2          = {0.0f, 0.0f, 1.0f, 0.0f};
XMGLOBALCONST XMVECTOR  g_XMIdentityR3          = {0.0f, 0.0f, 0.0f, 1.0f};

XMFINLINE XMMATRIX XMLoadFloat2x4A( CONST FLOAT* pSource)
{
	XMMATRIX M;
	M.r[0] = __lvx(pSource, 0);
    M.r[1] = __lvx(pSource, 16);
    M.r[2] = g_XMIdentityR2;
    M.r[3] = g_XMIdentityR3;
	return M;
}

XMFINLINE VOID XMStoreFloat2x4A( FLOAT*   pDestination, XMMATRIX  M )
{
	__stvx(M.r[0], pDestination, 0);
    __stvx(M.r[1], pDestination, 16);
}

inline void Copy2Vector4(__vector4* __restrict dst, const __vector4* __restrict src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

#endif

////////////////////////

void GMatrix2D::SetMatrix(const GMatrix2D &m)
{
#if defined(_XBOX)
	Copy2Vector4( &r[0] , &m.r[0] );
#else
    M_[0][0] = m.M_[0][0];
    M_[0][1] = m.M_[0][1];
    M_[0][2] = m.M_[0][2];
    M_[1][0] = m.M_[1][0];
    M_[1][1] = m.M_[1][1];
    M_[1][2] = m.M_[1][2];
#endif
}
void GMatrix2D::SetMatrix(Float v0, Float v1, Float v2, Float v3, Float v4, Float v5)
{
#if defined(_XBOX)
	r[0] = XMVectorSet(v0,v1,v4,0.0f);
	r[1] = XMVectorSet(v2,v3,v5,0.0f);
#else
	M_[0][0] = v0;
    M_[0][1] = v1;
    M_[0][2] = v4;
    M_[1][0] = v2;
    M_[1][1] = v3;
    M_[1][2] = v5;
#endif

}

void GMatrix2D::AppendScaling (Float sx, Float sy)
{
#if defined(_XBOX)
	r[0] = r[0] * sx;
	r[1] = r[1] * sy;
#else
    M_[0][0] *= sx;
    M_[0][1] *= sx;
    M_[0][2] *= sx;
    M_[1][0] *= sy;
    M_[1][1] *= sy;
    M_[1][2] *= sy;
#endif
}

void GMatrix2D::Swap(GMatrix2D *pm)
{
#if defined(_XBOX)
	GTL::gswap(r[0], pm->r[0]);
    GTL::gswap(r[1], pm->r[1]);
#else
    GTL::gswap(M_[0][0], pm->M_[0][0]);
    GTL::gswap(M_[0][1], pm->M_[0][1]);
    GTL::gswap(M_[0][2], pm->M_[0][2]);
    GTL::gswap(M_[1][0], pm->M_[1][0]);
    GTL::gswap(M_[1][1], pm->M_[1][1]);
    GTL::gswap(M_[1][2], pm->M_[1][2]);
#endif

}


bool    GMatrix2D::IsValid() const
{
    return GFC_ISFINITE(M_[0][0])
        && GFC_ISFINITE(M_[0][1])
        && GFC_ISFINITE(M_[0][2])
        && GFC_ISFINITE(M_[1][0])
        && GFC_ISFINITE(M_[1][1])
        && GFC_ISFINITE(M_[1][2]);
}

// Set the GMatrix2D to identity.
void    GMatrix2D::SetIdentity()
{
#if defined(_XBOX)
	r[0] = g_XMIdentityR0;
	r[1] = g_XMIdentityR1;
#else
    M_[0][0] = 1.0f;
    M_[0][1] = 0.0f;
    M_[0][2] = 0.0f;
    M_[1][0] = 0.0f;
    M_[1][1] = 1.0f;
    M_[1][2] = 0.0f;
#endif
}

// Concatenate m and the current matrix.  When transforming points, 
// m happens first, then the original transformation.
void  GMatrix2D::Prepend(const GMatrix2D& m)
{
#if defined(_XBOX)
	XMMATRIX2X4& t = *this;
	t = XMMatrixMultiply( t , m );
#else
    GMatrix2D t = *this;
    M_[0][0] = t.M_[0][0] * m.M_[0][0] + t.M_[0][1] * m.M_[1][0];
    M_[1][0] = t.M_[1][0] * m.M_[0][0] + t.M_[1][1] * m.M_[1][0];
    M_[0][1] = t.M_[0][0] * m.M_[0][1] + t.M_[0][1] * m.M_[1][1];
    M_[1][1] = t.M_[1][0] * m.M_[0][1] + t.M_[1][1] * m.M_[1][1];
    M_[0][2] = t.M_[0][0] * m.M_[0][2] + t.M_[0][1] * m.M_[1][2] + t.M_[0][2];
    M_[1][2] = t.M_[1][0] * m.M_[0][2] + t.M_[1][1] * m.M_[1][2] + t.M_[1][2];
#endif
}

// Concatenate the current matrix and m.  When transforming points, 
// the original transformation happens first, then m.
void GMatrix2D::Append(const GMatrix2D& m)
{
#if defined(_XBOX)
	XMMATRIX2X4& t = *this;
	t = XMMatrixMultiply( m , t );
#else
    GMatrix2D t = *this;
    M_[0][0] = m.M_[0][0] * t.M_[0][0] + m.M_[0][1] * t.M_[1][0];
    M_[1][0] = m.M_[1][0] * t.M_[0][0] + m.M_[1][1] * t.M_[1][0];
    M_[0][1] = m.M_[0][0] * t.M_[0][1] + m.M_[0][1] * t.M_[1][1];
    M_[1][1] = m.M_[1][0] * t.M_[0][1] + m.M_[1][1] * t.M_[1][1];
    M_[0][2] = m.M_[0][0] * t.M_[0][2] + m.M_[0][1] * t.M_[1][2] + m.M_[0][2];
    M_[1][2] = m.M_[1][0] * t.M_[0][2] + m.M_[1][1] * t.M_[1][2] + m.M_[1][2];
#endif
}

// Set this GMatrix2D to a blend of m1 and m2, parameterized by t.
void    GMatrix2D::SetLerp(const GMatrix2D& m1, const GMatrix2D& m2, Float t)
{
#if defined(_XBOX)
	r[0] = XMVectorLerp( m1.r[0] , m2.r[0] , t );
	r[1] = XMVectorLerp( m1.r[1] , m2.r[1] , t );
#else
    M_[0][0] = gflerp(m1.M_[0][0], m2.M_[0][0], t);
    M_[1][0] = gflerp(m1.M_[1][0], m2.M_[1][0], t);
    M_[0][1] = gflerp(m1.M_[0][1], m2.M_[0][1], t);
    M_[1][1] = gflerp(m1.M_[1][1], m2.M_[1][1], t);
    M_[0][2] = gflerp(m1.M_[0][2], m2.M_[0][2], t);
    M_[1][2] = gflerp(m1.M_[1][2], m2.M_[1][2], t);
#endif

}

// Formats matrix message to a buffer, but 512 bytes at least
void    GMatrix2D::Format(char *pbuffer) const
{

    gfc_sprintf(pbuffer, GFX_STREAM_BUFFER_SIZE, 
            "| %4.4f %4.4f %4.4f |\n"
            "| %4.4f %4.4f %4.4f |\n", 
            M_[0][0], M_[0][1], GFC_TWIPS_TO_PIXELS(M_[0][2]),
            M_[1][0], M_[1][1], GFC_TWIPS_TO_PIXELS(M_[1][2]));
}

// Transform GPointF 'p' by our GMatrix2D.  Put the result in *result.
void    GMatrix2D::Transform(GPointF* result, const GPointF& p) const
{
    GASSERT(result);

    result->x = M_[0][0] * p.x + M_[0][1] * p.y + M_[0][2];
    result->y = M_[1][0] * p.x + M_[1][1] * p.y + M_[1][2];
}

// Transform vector 'v' by our GMatrix2D. Doesn't apply translation.
// Put the result in *result.
void    GMatrix2D::TransformVector(GPointF* result, const GPointF& v) const
{
    GASSERT(result);

    result->x = M_[0][0] * v.x + M_[0][1] * v.y;
    result->y = M_[1][0] * v.x + M_[1][1] * v.y;
}

// Transform GPointF 'p' by the inverse of our GMatrix2D.  Put result in *result.
void    GMatrix2D::TransformByInverse(GPointF* result, const GPointF& p) const
{
    GMatrix2D(*this).Invert().Transform(result, p);
}


// Set this GMatrix2D to the inverse of the given GMatrix2D.
void    GMatrix2D::SetInverse(const GMatrix2D& m)
{
    GASSERT(this != &m);

    // Invert the rotation part.
    Float   det = m.M_[0][0] * m.M_[1][1] - m.M_[0][1] * m.M_[1][0];
    if (det == 0.0f)
    {
        // Not invertible - this happens sometimes (ie. sample6.Swf)

        // Arbitrary fallback
        SetIdentity();
        M_[0][2] = -m.M_[0][2];
        M_[1][2] = -m.M_[1][2];
    }
    else
    {
        Float   invDet = 1.0f / det;
        M_[0][0] = m.M_[1][1] * invDet;
        M_[1][1] = m.M_[0][0] * invDet;
        M_[0][1] = -m.M_[0][1] * invDet;
        M_[1][0] = -m.M_[1][0] * invDet;

        M_[0][2] = -(M_[0][0] * m.M_[0][2] + M_[0][1] * m.M_[1][2]);
        M_[1][2] = -(M_[1][0] * m.M_[0][2] + M_[1][1] * m.M_[1][2]);
    }
}


// Return true if this GMatrix2D reverses handedness.
bool    GMatrix2D::DoesFlip() const
{
    Float   det = M_[0][0] * M_[1][1] - M_[0][1] * M_[1][0];
    return det < 0.F;
}


// Return the determinant of the 2x2 rotation/scale part only.
Float   GMatrix2D::GetDeterminant() const
{
    return M_[0][0] * M_[1][1] - M_[1][0] * M_[0][1];
}

// Return the maximum scale factor that this transform
// applies.  For assessing scale, when determining acceptable
// errors in tessellation.
Float   GMatrix2D::GetMaxScale() const
{
    // @@ take the max length of the two basis vectors.
    Float   basis0Length2 = M_[0][0] * M_[0][0] + M_[1][0] * M_[1][0];
    Float   basis1Length2 = M_[0][1] * M_[0][1] + M_[1][1] * M_[1][1];
    Float   maxLength2 = GTL::gmax<Float>(basis0Length2, basis1Length2);
    return sqrtf(maxLength2);
}

Float   GMatrix2D::GetX() const
{
    return M_[0][2];
}

Float   GMatrix2D::GetY() const
{
    return M_[1][2];
}

Double  GMatrix2D::GetXScale() const
{
    return sqrt(((Double)M_[0][0]) *((Double)M_[0][0]) + ((Double)M_[1][0]) * ((Double)M_[1][0]));
}

Double  GMatrix2D::GetYScale() const
{
    return sqrt(((Double)M_[1][1]) * ((Double)M_[1][1]) + ((Double)M_[0][1]) * ((Double)M_[0][1]));
}

Double  GMatrix2D::GetRotation() const
{
    return atan2((Double)M_[1][0], (Double)M_[0][0]);
}


// This is an axial bound of an oriented (and/or
// sheared, scaled, etc) box.
void    GMatrix2D::EncloseTransform(GRectF *pr, const GRectF& r) const
{
    // Get the transformed bounding box.
    GPointF p0, p1, p2, p3;
    Transform(&p0, r.TopLeft());
    Transform(&p1, r.TopRight());
    Transform(&p2, r.BottomRight());
    Transform(&p3, r.BottomLeft());

    pr->SetRect(p0, p0);
    pr->ExpandToPoint(p1);
    pr->ExpandToPoint(p2);
    pr->ExpandToPoint(p3);
}

