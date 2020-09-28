/**********************************************************************

Filename    :   GStroker.h
Content     :   Path-to-stroke converter
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

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

#ifndef INC_GSTROKER_H
#define INC_GSTROKER_H

#include "GCompoundShape.h"


// ***** Declared Classes
struct GStrokeVertexType;
class  GStrokePath;
class  GStroker;
class  GStrokerTypes;



//-----------------------------------------------------------------------
const GCoordType G_StrokeVertexDistEpsilon    = (GCoordType)1e-4;
const GCoordType G_StrokeCurveToleranceK      = (GCoordType)0.25;
const GCoordType G_IntersectionEpsilonStroker = (GCoordType)1e-2;

//-----------------------------------------------------------------------
struct GStrokeVertexType
{
    GCoordType x;
    GCoordType y;
    GCoordType dist;

    GStrokeVertexType() {}
    GStrokeVertexType(GCoordType x_, GCoordType y_) :
        x(x_), y(y_), dist(0) {}

    bool CalcDistance(const GStrokeVertexType& val)
    {
        dist = GMath2D::CalcDistance(*this, val);
        return dist > G_StrokeVertexDistEpsilon;
    }
};

//-----------------------------------------------------------------------
// This is an internal class, a stroke path storage. It calculates
// the distance between the neighbor vertices and filters coinciding
// vertices. It's important for stroke calculations. Function ClosePath()
// checks if the first and last vertices coincide and closes the path,
// i.e., removes the extra vertex and sets Close=true
//-----------------------------------------------------------------------
class GStrokePath
{
public:
    typedef GPodBVector<GStrokeVertexType> ContainerType;

    GStrokePath() : Closed(false) {}

    void RemoveAll()
        { Path.removeAll(); Closed = false; }

    void AddVertex(const GStrokeVertexType& v);

    void ClosePath(bool forceClosing);

    unsigned GetNumVertices() const { return Path.size(); }

    const GStrokeVertexType& GetVertexPrev(unsigned i) const
    {
        if(i == 0) i += Path.size();
        return Path[i - 1];
    }

    const GStrokeVertexType& GetVertex(unsigned i) const
    {
        return Path[i];
    }

    GStrokeVertexType& GetVertex(unsigned i)
    {
        return Path[i];
    }

    const GStrokeVertexType& GetVertexNext(unsigned i) const
    {
        if(++i >= Path.size()) i -= Path.size();
        return Path[i];
    }

    const GStrokeVertexType& GetVertexForw(unsigned i) const
    {
        i += 2;
        if(i >= Path.size()) i -= Path.size();
        return Path[i];
    }

    bool IsClosed() const { return Closed; }

private:
    ContainerType Path;
    bool          Closed;

    GStrokePath(const GStrokePath&)
        { Closed=false; }
};

//------------------------------------------------------------------------
GINLINE void GStrokePath::AddVertex(const GStrokeVertexType& v)
{
    if(Path.size() > 1)
    {
        if(!Path[Path.size()-2].CalcDistance(Path[Path.size()-1]))
        {
            Path.removeLast();
        }
    }
    Path.add(v);
}


//-----------------------------------------------------------------------

// Stroker types, separated so that they can also be used in GStrokerAA.
class GStrokerTypes
{
public:
    enum LineCapType
    {
        ButtCap,
        SquareCap,
        RoundCap
    };

    enum LineJoinType
    {
        MiterJoin,      // When exceeding miter limit cut at the limit
        MiterBevelJoin, // When exceeding miter limit use bevel join
        MiterRoundJoin, // When exceeding miter limit use round join
        RoundJoin,
        BevelJoin
    };
};


//-----------------------------------------------------------------------
class GStroker : public GStrokerTypes
{
public:

    GStroker() :
        Width(GCoordType(1.0)),
        WidthAbs(GCoordType(1.0)),
        LineJoin(RoundJoin),
        StartLineCap(RoundCap),
        EndLineCap(RoundCap),
        MiterLimit(GCoordType(4.0))
    {}

    void SetWidth(GCoordType v)
    {
        Width = v / 2;
        WidthAbs = (Width < 0) ? -Width : Width;
    }
    GCoordType      GetWidth() const { return Width * 2; }

    void            SetLineJoin(LineJoinType v)     { LineJoin = v; }
    LineJoinType    GetLineJoin() const             { return LineJoin; }

    void            SetLineCap(LineCapType v)       { StartLineCap = EndLineCap = v; }
    void            SetStartLineCap(LineCapType v)  { StartLineCap = v; }
    void            SetEndLineCap(LineCapType v)    { EndLineCap = v; }

    LineCapType     GetStartLineCap() const         { return StartLineCap; }
    LineCapType     GetEndLineCap()   const         { return EndLineCap; }

    void            SetMiterLimit(GCoordType v)     { MiterLimit = v; }
    GCoordType      GetMiterLimit() const           { return MiterLimit; }

    void GenerateStroke(const GCompoundShape::SPath& srcPath,
                        GCompoundShape& dstShape)
    {
        generateEquidistant(srcPath, dstShape, true);
    }

    void GenerateStroke(const GCompoundShape& srcShape,
                        int srcStyle,
                        GCompoundShape& dstShape)
    {
        generateEquidistant(srcShape, srcStyle, dstShape, true);
    }

    void GenerateContour(const GCompoundShape::SPath& srcPath,
                         GCompoundShape& dstShape)
    {
        generateEquidistant(srcPath, dstShape, false);
    }

    void GenerateContour(const GCompoundShape& srcShape,
                         int srcStyle,
                         GCompoundShape& dstShape)
    {
        generateEquidistant(srcShape, srcStyle, dstShape, false);
    }

private:

    GStroker(const GStroker&) :
        Width(GCoordType(1.0)),
        WidthAbs(GCoordType(1.0)),
        LineJoin(RoundJoin),
        StartLineCap(RoundCap),
        EndLineCap(RoundCap),
        MiterLimit(GCoordType(4.0))
    { }

    const GStroker& operator = (const GStroker&)
    { return *this; }

    void calcArc(GCompoundShape& dstShape,
                 GCoordType x,   GCoordType y,
                 GCoordType dx1, GCoordType dy1,
                 GCoordType dx2, GCoordType dy2);

    void calcMiter(GCompoundShape& dstShape,
                   const GStrokeVertexType& v0,
                   const GStrokeVertexType& v1,
                   const GStrokeVertexType& v2,
                   GCoordType dx1, GCoordType dy1,
                   GCoordType dx2, GCoordType dy2,
                   LineJoinType lineJoin,
                   GCoordType miterLimit,
                   GCoordType epsilon,
                   GCoordType dbevel);

    void calcCap(GCompoundShape& dstShape,
                 const GStrokeVertexType& v0,
                 const GStrokeVertexType& v1,
                 GCoordType len,
                 LineCapType cap);

    void calcJoin(GCompoundShape& dstShape,
                  const GStrokeVertexType& v0,
                  const GStrokeVertexType& v1,
                  const GStrokeVertexType& v2,
                  GCoordType len1,
                  GCoordType len2);

    void generateEquidistant(GCompoundShape& dstShape, bool bothSides);

    void generateEquidistant(const GCompoundShape::SPath& srcPath,
                             GCompoundShape& dstShape,
                             bool bothSides);

    void generateEquidistant(const GCompoundShape& srcShape,
                             int srcStyle,
                             GCompoundShape& dstShape,
                             bool bothSides);

    GCoordType      Width;
    GCoordType      WidthAbs;
    LineJoinType    LineJoin;
    LineCapType     StartLineCap;
    LineCapType     EndLineCap;
    GCoordType      MiterLimit;

    GStrokePath     Path;
};

#endif
