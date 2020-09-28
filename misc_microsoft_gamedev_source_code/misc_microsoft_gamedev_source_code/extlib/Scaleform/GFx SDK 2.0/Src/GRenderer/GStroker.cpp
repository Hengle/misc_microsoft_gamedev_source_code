/**********************************************************************

Filename    :   GStroker.cpp
Content     :   
Created     :   2005
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Path-to-stroke converter

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

----------------------------------------------------------------------
The code of these classes was taken from the Anti-Grain Geometry
Project and modified for the use by Scaleform. 
Permission to use without restrictions is hereby granted to 
Scaleform Corporation by the author of Anti-Grain Geometry Project.
**********************************************************************/

#include "GStroker.h"

//------------------------------------------------------------------------
void GStrokePath::ClosePath(bool forceClosing)
{
    Closed = false;

    if(forceClosing && Path.size())
    {
        AddVertex(Path[0]);
    }

    if(Path.size() > 1)
    {
        Closed = !Path[Path.size()-1].CalcDistance(Path[0]);
    }

    while(Path.size() > 1)
    {
        if(Path[Path.size()-2].CalcDistance(Path[Path.size()-1])) break;
        GStrokeVertexType t = Path[Path.size()-1];
        Path.removeLast();
        Path.removeLast();
        Path.add(t);
    }

    if(Closed)
    {
        while(Path.size() > 2)
        {
            if(Path[Path.size()-1].CalcDistance(Path[0])) break;
            Path.removeLast();
        }
        if(Path.size() < 3) 
        {
            Closed = false;
        }
    }
}

//------------------------------------------------------------------------
void GStroker::calcArc(GCompoundShape& dstShape,
                       GCoordType x,   GCoordType y, 
                       GCoordType dx1, GCoordType dy1, 
                       GCoordType dx2, GCoordType dy2)
{
    GCoordType a1 = (GCoordType)atan2(dy1, dx1);
    GCoordType a2 = (GCoordType)atan2(dy2, dx2);
    GCoordType da = (GCoordType)acos(WidthAbs / 
                                    (WidthAbs + 
                                     G_StrokeCurveToleranceK * 
                                     dstShape.GetCurveTolerance())) * 2;

    dstShape.AddVertex(x + dx1, y + dy1);

    if(a1 > a2) a2 += 2 * GMath2D::Pi;
    int n = int((a2 - a1) / da);
    da = (a2 - a1) / (n + 1);
    a1 += da;

    int i;
    for(i = 0; i < n; i++)
    {
        dstShape.AddVertex(x + (GCoordType)cos(a1) * WidthAbs, 
                           y + (GCoordType)sin(a1) * WidthAbs);
        a1 += da;
    }

    dstShape.AddVertex(x + dx2, y + dy2);
}

//------------------------------------------------------------------------
void GStroker::calcMiter(GCompoundShape& dstShape,
                         const GStrokeVertexType& v0, 
                         const GStrokeVertexType& v1, 
                         const GStrokeVertexType& v2,
                         GCoordType dx1, GCoordType dy1, 
                         GCoordType dx2, GCoordType dy2,
                         LineJoinType lineJoin,
                         GCoordType miterLimit,
                         GCoordType epsilon,
                         GCoordType dbevel)
{
    GCoordType xi  = v1.x;
    GCoordType yi  = v1.y;
    GCoordType di  = 1;
    GCoordType lim = WidthAbs * miterLimit;
    bool miterLimitExceeded = true; // Assume the worst
    bool intersectionFailed = true; // Assume the worst

    if(GMath2D::CalcIntersection(v0.x + dx1, v0.y + dy1,
                                 v1.x + dx1, v1.y + dy1,
                                 v1.x + dx2, v1.y + dy2,
                                 v2.x + dx2, v2.y + dy2,
                                 &xi, &yi,
                                 epsilon))
    {
        // Calculation of the intersection succeeded
        //---------------------
        di = GMath2D::CalcDistance(v1.x, v1.y, xi, yi);
        if(di <= lim)
        {
            // Inside the miter limit
            //---------------------
            dstShape.AddVertex(xi, yi);
            miterLimitExceeded = false;
        }
        intersectionFailed = false;
    }
    else
    {
        // Calculation of the intersection failed, most probably
        // the three points lie one straight line. 
        // First check if v0 and v2 lie on the opposite sides of vector: 
        // (v1.x, v1.y) -> (v1.x+dx1, v1.y-dy1), that is, the perpendicular
        // to the line determined by vertices v0 and v1.
        // This condition determines whether the next line segments continues
        // the previous one or goes back.
        //----------------
        GCoordType x2 = v1.x + dx1;
        GCoordType y2 = v1.y + dy1;
        if(((v0.x - x2)*dy1 - (v0.y - y2)*dx1 < 0.0) !=
           ((v2.x - x2)*dy1 - (v2.y - y2)*dx1 < 0.0))
        {
            // This case means that the next segment continues 
            // the previous one (straight line)
            //-----------------
            dstShape.AddVertex(v1.x + dx1, v1.y + dy1);
            miterLimitExceeded = false;
        }
    }

    if(miterLimitExceeded)
    {
        // Miter limit exceeded
        //------------------------
        switch(lineJoin)
        {
        case MiterBevelJoin:
            // For the compatibility with SVG, PDF, etc, 
            // we use a simple bevel join instead of
            // "smart" bevel
            //-------------------
            dstShape.AddVertex(v1.x + dx1, v1.y + dy1);
            dstShape.AddVertex(v1.x + dx2, v1.y + dy2);
            break;

        case MiterRoundJoin:
            calcArc(dstShape, v1.x, v1.y, dx1, dy1, dx2, dy2);
            break;

        default:
            // If no miter-revert, calculate new dx1, dy1, dx2, dy2
            //----------------
            if(intersectionFailed)
            {
                dstShape.AddVertex(v1.x + dx1 - dy1 * miterLimit, 
                                   v1.y + dy1 + dx1 * miterLimit);
                dstShape.AddVertex(v1.x + dx2 + dy2 * miterLimit, 
                                   v1.y + dy2 - dx2 * miterLimit);
            }
            else
            {
                GCoordType x1 = v1.x + dx1;
                GCoordType y1 = v1.y + dy1;
                GCoordType x2 = v1.x + dx2;
                GCoordType y2 = v1.y + dy2;
                di = (lim - dbevel) / (di - dbevel);
                dstShape.AddVertex(x1 + (xi - x1) * di, y1 + (yi - y1) * di);
                dstShape.AddVertex(x2 + (xi - x2) * di, y2 + (yi - y2) * di);
            }
            break;
        }
    }
}

//------------------------------------------------------------------------
void GStroker::calcCap(GCompoundShape& dstShape,
                       const GStrokeVertexType& v0, 
                       const GStrokeVertexType& v1, 
                       GCoordType len,
                       LineCapType cap)
{
    GCoordType dx1 = (v1.y - v0.y) / len;
    GCoordType dy1 = (v0.x - v1.x) / len;
    GCoordType dx2 = 0;
    GCoordType dy2 = 0;

    dx1 *= WidthAbs;
    dy1 *= WidthAbs;

    if(cap != RoundCap)
    {
        if(cap == SquareCap)
        {
            dx2 = dy1;
            dy2 = dx1;
        }
        dstShape.AddVertex(v0.x - dx1 + dx2, v0.y - dy1 - dy2);
        dstShape.AddVertex(v0.x + dx1 + dx2, v0.y + dy1 - dy2);
    }
    else
    {
        GCoordType a1 = (GCoordType)atan2(-dy1, -dx1);
        GCoordType a2 = a1 + GMath2D::Pi;
        GCoordType da = 
            (GCoordType)acos(WidthAbs / 
                            (WidthAbs + 
                             G_StrokeCurveToleranceK * 
                             dstShape.GetCurveTolerance())) * 2;
        int i;
        int n = int((a2 - a1) / da);
        da = (a2 - a1) / (n + 1);
        a1 += da;

        dstShape.AddVertex(v0.x - dx1, v0.y - dy1);
        for(i = 0; i < n; i++)
        {
            dstShape.AddVertex(v0.x + (GCoordType)cos(a1) * WidthAbs, 
                               v0.y + (GCoordType)sin(a1) * WidthAbs);
            a1 += da;
        }
        dstShape.AddVertex(v0.x + dx1, v0.y + dy1);
    }
}

//------------------------------------------------------------------------
void GStroker::calcJoin(GCompoundShape& dstShape,
                        const GStrokeVertexType& v0, 
                        const GStrokeVertexType& v1, 
                        const GStrokeVertexType& v2,
                        GCoordType len1, 
                        GCoordType len2)
{
    GCoordType dx1, dy1, dx2, dy2;
    GCoordType epsilon = (len1 + len2) * G_IntersectionEpsilonStroker;

    dx1 = WidthAbs * (v1.y - v0.y) / len1;
    dy1 = WidthAbs * (v0.x - v1.x) / len1;
    dx2 = WidthAbs * (v2.y - v1.y) / len2;
    dy2 = WidthAbs * (v1.x - v2.x) / len2;

    if(GMath2D::CrossProduct(v0, v1, v2) > 0)
    {
        // Inner join
        //---------------
        GCoordType minLen = (len1 < len2) ? len1 : len2;
        calcMiter(dstShape,
                  v0, v1, v2, dx1, dy1, dx2, dy2, 
                  MiterBevelJoin, 
                  minLen / WidthAbs,
                  epsilon, 0);
    }
    else
    {
        // Outer join
        //---------------
        GCoordType dx = (dx1 + dx2) / 2;
        GCoordType dy = (dy1 + dy2) / 2;
        GCoordType dbevel = (GCoordType)sqrt(dx * dx + dy * dy);
        LineJoinType lineJoin = LineJoin;
        if(lineJoin == RoundJoin || lineJoin == BevelJoin)
        {
            // This is an optimization that reduces the number of points 
            // in cases of almost collinear segments. If there's no
            // visible difference between bevel and miter joins we'd rather
            // use miter join because it adds only one point instead of two. 
            //
            // Here we calculate the middle point between the bevel points 
            // and then, the distance between v1 and this middle point. 
            // At outer joins this distance always less than stroke width, 
            // because it's actually the height of an isosceles triangle of
            // v1 and its two bevel points. If the difference between this
            // width and this value is small (no visible bevel) we can switch
            // to the miter join. 
            //
            // The constant in the expression makes the result approximately 
            // the same as in round joins and caps. 
            //-------------------
            if((WidthAbs - dbevel) < 
                dstShape.GetCurveTolerance() * G_StrokeCurveToleranceK / 2)
            {
                if(GMath2D::CalcIntersection(v0.x + dx1, v0.y + dy1,
                                             v1.x + dx1, v1.y + dy1,
                                             v1.x + dx2, v1.y + dy2,
                                             v2.x + dx2, v2.y + dy2,
                                             &dx, &dy,
                                             epsilon))
                {
                    dstShape.AddVertex(dx, dy);
                }
                else
                {
                    dstShape.AddVertex(v1.x + dx1, v1.y + dy1);
                }
                return;
            }
        }

        switch(lineJoin)
        {
        case MiterJoin:
        case MiterBevelJoin:
        case MiterRoundJoin:
            calcMiter(dstShape, 
                      v0, v1, v2, dx1, dy1, dx2, dy2, 
                      lineJoin, 
                      MiterLimit,
                      epsilon,
                      dbevel);
            break;

        case RoundJoin:
            calcArc(dstShape, v1.x, v1.y, dx1, dy1, dx2, dy2);
            break;

        default: // Bevel join
            dstShape.AddVertex(v1.x + dx1, v1.y + dy1);
            dstShape.AddVertex(v1.x + dx2, v1.y + dy2);
            break;
        }
    }
}

//------------------------------------------------------------------------
void GStroker::generateEquidistant(GCompoundShape& dstShape, bool bothSides)
{
    if(Path.GetNumVertices() < 2) return;

    unsigned i;
    if(Path.IsClosed())
    {
        if(bothSides || Width > 0)
        {
            dstShape.BeginPath(0, -1, -1);
            for(i = 0; i < Path.GetNumVertices(); i++)
            {
                calcJoin(dstShape, 
                         Path.GetVertexPrev(i), 
                         Path.GetVertex    (i), 
                         Path.GetVertexNext(i), 
                         Path.GetVertexPrev(i).dist,
                         Path.GetVertex    (i).dist);
            }
            dstShape.ClosePath();
        }

        if(bothSides || Width < 0)
        {
            dstShape.BeginPath(0, -1, -1);
            for(int j = Path.GetNumVertices() - 1; j >= 0; j--)
            {
                calcJoin(dstShape,
                         Path.GetVertexNext(j), 
                         Path.GetVertex    (j), 
                         Path.GetVertexPrev(j), 
                         Path.GetVertex    (j).dist, 
                         Path.GetVertexPrev(j).dist);
            }
            dstShape.ClosePath();
        }
    }
    else
    {
        dstShape.BeginPath(0, -1, -1);

        calcCap(dstShape,
                Path.GetVertex(0),
                Path.GetVertex(1),
                Path.GetVertex(0).dist,
                StartLineCap);

        for(i = 1; i < Path.GetNumVertices() - 1; i++)
        {
            calcJoin(dstShape, 
                     Path.GetVertexPrev(i), 
                     Path.GetVertex    (i), 
                     Path.GetVertexNext(i), 
                     Path.GetVertexPrev(i).dist,
                     Path.GetVertex    (i).dist);
        }

        calcCap(dstShape,
                Path.GetVertex(Path.GetNumVertices() - 1), 
                Path.GetVertex(Path.GetNumVertices() - 2), 
                Path.GetVertex(Path.GetNumVertices() - 2).dist,
                EndLineCap);

        for(i = Path.GetNumVertices() - 2; i > 0; i--)
        {
            calcJoin(dstShape,
                     Path.GetVertexNext(i), 
                     Path.GetVertex    (i), 
                     Path.GetVertexPrev(i), 
                     Path.GetVertex    (i).dist, 
                     Path.GetVertexPrev(i).dist);
        }

        dstShape.ClosePath();
    }
}

//------------------------------------------------------------------------
void GStroker::generateEquidistant(const GCompoundShape::SPath& srcPath, 
                                   GCompoundShape& dstShape,
                                   bool bothSides)
{
    Path.RemoveAll();
    unsigned i;
    for(i = 0; i < srcPath.GetNumVertices(); i++)
    {
        const GPointType& v = srcPath.GetVertex(i);
        Path.AddVertex(GStrokeVertexType(v.x, v.y));
    }
    Path.ClosePath(!bothSides);
    generateEquidistant(dstShape, bothSides);
}

//------------------------------------------------------------------------
void GStroker::generateEquidistant(const GCompoundShape& srcShape, 
                                   int srcStyle,
                                   GCompoundShape& dstShape,
                                   bool bothSides)
{
    Path.RemoveAll();
    unsigned i, j;
    for(i = 0; i < srcShape.GetNumPaths(); i++)
    {
        const GCompoundShape::SPath& srcPath = srcShape.GetPath(i);
        if((srcStyle < 0 || srcPath.GetLineStyle() == srcStyle) &&
            srcPath.GetNumVertices())
        {
            bool merge = false;
            if(Path.GetNumVertices())
            {
                const GStrokeVertexType& v0 = Path.GetVertex(Path.GetNumVertices() - 1);
                const GPointType         v1 = srcPath.GetVertex(0);
                if(GMath2D::CalcDistance(v0, v1) <= G_StrokeVertexDistEpsilon)
                {
                    merge = true;
                }
            }
            if(merge)
            {
                for(j = 1; j < srcPath.GetNumVertices(); j++)
                {
                    const GPointType& v = srcPath.GetVertex(j);
                    Path.AddVertex(GStrokeVertexType(v.x, v.y));
                }
            }
            else
            {
                if(Path.GetNumVertices())
                {
                    Path.ClosePath(!bothSides);
                    generateEquidistant(dstShape, bothSides);
                    Path.RemoveAll();
                }
                for(j = 0; j < srcPath.GetNumVertices(); j++)
                {
                    const GPointType& v = srcPath.GetVertex(j);
                    Path.AddVertex(GStrokeVertexType(v.x, v.y));
                }
            }
        }
    }
    if(Path.GetNumVertices() > 1)
    {
        Path.ClosePath(!bothSides);
        generateEquidistant(dstShape, bothSides);
    }
}
