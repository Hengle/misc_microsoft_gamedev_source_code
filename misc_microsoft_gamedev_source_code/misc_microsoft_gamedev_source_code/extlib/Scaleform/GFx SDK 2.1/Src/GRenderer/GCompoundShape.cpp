/**********************************************************************

Filename    :   GCompoundShape.cpp
Content     :   
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Compound Shape container

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GCompoundShape.h"

//-----------------------------------------------------------------------
void GCompoundShape::RemoveAll()
{
    Vertices.removeAll();
    Paths.removeAll();
    CurrPath = 0;
    MinStyle =  0x7FFFFFFF;
    MaxStyle = -0x7FFFFFFF;
    MinX = 1;
    MinY = 1;
    MaxX = 0;
    MaxY = 0;
}

//-----------------------------------------------------------------------
void GCompoundShape::SetCurveTolerance(GCoordType t)
{
    CurveTolerance = t;
    t /= 4;
    ToleranceSquare = t * t;
}

//-----------------------------------------------------------------------
void GCompoundShape::BeginPath(int lStyle, int rStyle, int lineStyle)
{
    Paths.add(SPath(this, Vertices.size(), lStyle, rStyle, lineStyle));
    CurrPath = &Paths.last();
    if(lStyle >= 0)
    {
        if(lStyle < MinStyle) MinStyle = lStyle;
        if(lStyle > MaxStyle) MaxStyle = lStyle;
    }
    if(rStyle >= 0)
    {
        if(rStyle < MinStyle) MinStyle = rStyle;
        if(rStyle > MaxStyle) MaxStyle = rStyle;
    }
}

//-----------------------------------------------------------------------
void GCompoundShape::BeginPath(int lStyle, int rStyle, int lineStyle, 
                               GCoordType x, GCoordType y)
{
    BeginPath(lStyle, rStyle, lineStyle);
    AddVertex(x, y);
}

//-----------------------------------------------------------------------
void GCompoundShape::ClosePath()
{
    if(CurrPath && CurrPath->GetNumVertices() > 1)
    {
        const GPointType& v = CurrPath->GetVertex(0);
        AddVertex(v.x, v.y);
    }
}

//-----------------------------------------------------------------------
void GCompoundShape::AddCurve(GCoordType cx, GCoordType cy, 
                              GCoordType ax, GCoordType ay)
{
    const GPointType& p = Vertices.last();
    if(GMath2D::AbsCrossProduct(p, 
                                GPointType(cx, cy), 
                                GPointType(ax, ay)) < G_CollinearCurveEpsilon)
    {
        AddVertex(ax, ay);
    }
    else
    {
        flattenQuadraticCurve(p.x, p.y, cx, cy, ax, ay);
    }
}

//-----------------------------------------------------------------------
void GCompoundShape::flattenQuadraticCurve(GCoordType x1, GCoordType y1, 
                                           GCoordType x2, GCoordType y2, 
                                           GCoordType x3, GCoordType y3)
{
    const GCoordType halfMultiplier = (GCoordType)0.5f;

    // Calculate all the mid-points of the line segments
    //----------------------
    GCoordType x12   = (x1 + x2) * halfMultiplier;
    GCoordType y12   = (y1 + y2) * halfMultiplier;
    GCoordType x23   = (x2 + x3) * halfMultiplier;
    GCoordType y23   = (y2 + y3) * halfMultiplier;

    GCoordType dx = x3-x1;
    GCoordType dy = y3-y1;
    GCoordType d = (GCoordType)fabs(((x2 - x3) * dy - (y2 - y3) * dx));

    if(d > G_CollinearCurveEpsilon)
    { 
        // Regular case
        //-----------------
        if(d * d <= ToleranceSquare * (dx*dx + dy*dy))
        {
            // If the curvature doesn't exceed the distance_tolerance value
            // we can finish subdivisions.
            //----------------------
            AddVertex(x3, y3);
            return;
        }
    }
    else
    {
        AddVertex(x3, y3);
        return;
    }

    GCoordType x123  = (x12 + x23) * halfMultiplier;
    GCoordType y123  = (y12 + y23) * halfMultiplier;

    // Continue subdivision
    //----------------------
    flattenQuadraticCurve(x1, y1, x12, y12, x123, y123); 
    flattenQuadraticCurve(x123, y123, x23, y23, x3, y3); 
}

//-----------------------------------------------------------------------
bool GCompoundShape::PathsEqual(const SPath& a, const SPath& b)
{
    unsigned ia, ib;
    for(ia = ib = 0; 
        ia < a.GetNumVertices() && ib < a.GetNumVertices(); 
        ++ia, ++ib)
    {
        const GPointType& pa = a.GetVertex(ia);
        const GPointType& pb = b.GetVertex(ib);
        if(pa.x != pb.x || pa.y != pb.y)
        {
            return false;
        }
    }
    return (ia < a.GetNumVertices()) == (ib < b.GetNumVertices());
}

//-----------------------------------------------------------------------
void GCompoundShape::PerceiveBounds(GCoordType* x1, GCoordType* y1, 
                                    GCoordType* x2, GCoordType* y2) const
{
    *x1 = 1;
    *y1 = 1;
    *x2 = 0;
    *y2 = 0;
    if(Vertices.size())
    {
        *x1 = *x2 = Vertices[0].x;
        *y1 = *y2 = Vertices[0].y;
        for(unsigned i = 1; i < Vertices.size(); i++)
        {
            const GPointType& p = Vertices[i];
            if(p.x < *x1) *x1 = p.x;
            if(p.y < *y1) *y1 = p.y;
            if(p.x > *x2) *x2 = p.x;
            if(p.y > *y2) *y2 = p.y;
        }
    }
}

//-----------------------------------------------------------------------
void GCompoundShape::ExpandPathBounds(const SPath& path, 
                                      GCoordType* x1, GCoordType* y1, 
                                      GCoordType* x2, GCoordType* y2)
{
    for (unsigned i = 0; i < path.GetNumVertices(); ++i)
    {
        const GPointType& p = path.GetVertex(i);
        if(p.x < *x1) *x1 = p.x;
        if(p.y < *y1) *y1 = p.y;
        if(p.x > *x2) *x2 = p.x;
        if(p.y > *y2) *y2 = p.y;
    }
}


//-----------------------------------------------------------------------
void GCompoundShape::PerceiveBounds()
{
    PerceiveBounds(&MinX, &MinY, &MaxX, &MaxY);
}

//-----------------------------------------------------------------------
bool GCompoundShape::PointInShape(GCoordType x, GCoordType y, bool nonZero) const
{
    int styleCount = 0;
    unsigned i, j;
    for(i = 0; i < GetNumPaths(); i++)
    {
        const SPath& path = GetPath(i);
        int ls = path.GetLeftStyle();
        int rs = path.GetRightStyle();
        if(ls != rs)
        {
            for(j = 1; j < path.GetNumVertices(); j++)
            {
                GPointType p1 = path.GetVertex(j-1);
                GPointType p2 = path.GetVertex(j);
                if(p1.y != p2.y)
                {
                    int dir = 1;
                    if(p1.y > p2.y)
                    {
                        GAlg::SwapElements(p1, p2);
                        dir = -1;
                    }
                    if(y >= p1.y && y < p2.y &&
                       GMath2D::CrossProduct(p1, p2, GPointType(x, y)) > 0)
                    {
                        if(nonZero)
                        {
                            if(ls >= 0) styleCount += (ls + 1) * dir;
                            if(rs >= 0) styleCount -= (rs + 1) * dir;
                        }
                        else
                        {
                            if(ls >= 0) styleCount ^= 1;
                            if(rs >= 0) styleCount ^= 1;
                        }
                    }
                }
            }
        }
    }
    return styleCount != 0;
}

//-----------------------------------------------------------------------
void GCompoundShape::ScaleAndTranslate(GCoordType kx, GCoordType ky,
                                       GCoordType dx, GCoordType dy)
{
    for(unsigned i = 0; i < Vertices.size(); i++)
    {
        GPointType& p = Vertices[i];
        p.x *= kx;
        p.y *= ky;
        p.x += dx;
        p.y += dy;
    }
}

//-----------------------------------------------------------------------
void GCompoundShape::RemoveShortSegments(GCoordType maxLen)
{
    unsigned i, j;
    for(i = 0; i < GetNumPaths(); ++i)
    {
        SPath& path = GetPath(i);
        if (path.NumVertices > 2)
        {
            unsigned start = path.StartVertex;
            unsigned num   = path.NumVertices - 1;
            GPointType v1  = Vertices[start];
            for(j = 1; j < num; ++j)
            {
                GPointType& v2 = Vertices[start + j];
                if (fabsf(v2.x-v1.x) + fabsf(v2.y-v1.y) >= maxLen)
                    v1 = v2;
                else
                    v2 = v1;
            }
            v1 = Vertices[start + num];
            GPointType& v2 = Vertices[start + num - 1];
            if (fabsf(v2.x-v1.x) + fabsf(v2.y-v1.y) < maxLen)
                v2 = v1;
        }
    }
}

