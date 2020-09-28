/**********************************************************************

Filename    :   GStrokerAA.h
Content     :   
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Anti-Aliased stroke generator

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

#ifndef INC_GSTROKERAA_H
#define INC_GSTROKERAA_H

#include "GStroker.h"


// ***** Declared Classes
class GStrokerAA;




class GStrokerAA : public GStrokerTypes
{
public:    

    struct VertexType
    {
        GCoordType x, y; // Coordinates
        int id;          // Some abstract ID, such as style, index, etc

        VertexType() {}
        VertexType(GCoordType x_, GCoordType y_, int id_=-1) : 
            x(x_), y(y_), id(id_) 
        {}
    };

    struct TriangleType
    {
        unsigned v1, v2, v3;
    };

    GStrokerAA() : 
        LineJoin(RoundJoin),
        StartLineCap(RoundCap),
        EndLineCap(RoundCap),
        MiterLimit(4),
        StyleLeft(0),
        StyleRight(0),
        WidthLeft(0), 
        WidthRight(0),
        AaWidthLeft(0.5f),
        AaWidthRight(0.5f),
        Tolerance(1)
    {}

    void        SetStyleLeft(int v)             { StyleLeft = v; }
    void        SetStyleRight(int v)            { StyleRight = v; }
    int         GetStyleLeft() const            { return StyleLeft; }
    int         GetStyleRight() const           { return StyleRight; }

    void        SetSolidWidth(GCoordType v)      { WidthLeft = WidthRight = v / 2; }
    void        SetSolidWidthLeft(GCoordType v)  { WidthLeft = v; }
    void        SetSolidWidthRight(GCoordType v) { WidthRight = v; }

    GCoordType  GetSolidWidth()      const      { return WidthLeft + WidthRight; }
    GCoordType  GetSolidWidthLeft()  const      { return WidthLeft; }
    GCoordType  GetSolidWidthRight() const      { return WidthRight; }

    void        SetAntiAliasWidth(GCoordType v)      { AaWidthLeft = AaWidthRight = v; }
    void        SetAntiAliasWidthLeft(GCoordType v)  { AaWidthLeft = v; }
    void        SetAntiAliasWidthRight(GCoordType v) { AaWidthRight = v; }

    GCoordType  GetAntiAliasWidth()      const  { return (AaWidthLeft + AaWidthRight) / 2; }
    GCoordType  GetAntiAliasWidthLeft()  const  { return AaWidthLeft; }
    GCoordType  GetAntiAliasWidthRight() const  { return AaWidthRight; }

    void        SetLineJoin(LineJoinType v)     { LineJoin = v; }
    LineJoinType GetLineJoin() const            { return LineJoin; }

    void        SetLineCap(LineCapType v)       { StartLineCap = EndLineCap = v; }
    void        SetStartLineCap(LineCapType v)  { StartLineCap = v; }
    void        SetEndLineCap(LineCapType v)    { EndLineCap = v; }
    LineCapType GetStartLineCap() const         { return StartLineCap; }
    LineCapType GetEndLineCap()   const         { return EndLineCap; }

    void        SetMiterLimit(GCoordType v)     { MiterLimit = v; }
    GCoordType  GetMiterLimit() const           { return MiterLimit; }

    void        SetCurveTolerance(GCoordType v)     { Tolerance = v; }
    GCoordType  GetCurveTolerance() const           { return Tolerance; }

    unsigned    GetNumVertices() const              { return Vertices.size(); }
    const VertexType& GetVertex(unsigned i) const   { return Vertices[i]; }

    unsigned    GetNumTriangles() const                 { return Triangles.size(); }
    const TriangleType& GetTriangle(unsigned i) const   { return Triangles[i]; }

    void RemoveAll()
    {
        Path.RemoveAll();
        Vertices.removeAll();
        Triangles.removeAll();
    }

    void Tessellate(const GCompoundShape::SPath& srcPath);
    void Tessellate(const GCompoundShape& srcShape, int srcStyle);

private:
    enum VertexMarkers
    {
        MarkerSolidL = 0xFFFFFFFC,
        MarkerSolidR = 0xFFFFFFFD,
        MarkerTotalL = 0xFFFFFFFE,
        MarkerTotalR = 0xFFFFFFFF
    };

    struct WidthsType
    {
        GCoordType solidWidthL;
        GCoordType solidWidthR;
        GCoordType solidWidth;  // Average:  (L + R) / 2
        GCoordType totalWidthL;
        GCoordType totalWidthR;
        GCoordType totalWidth;  // Average:  (L + R) / 2
        GCoordType widthCoeff;  // Unsigned: (R > L) ? L/R : R/L
        GCoordType solidCoeffL; // Unsigned: SolidL/TotalL
        GCoordType solidCoeffR; // Unsigned: SolidR/TotalR
        GCoordType solidLimitL;
        GCoordType solidLimitR;
        GCoordType totalLimitL;
        GCoordType totalLimitR;
        bool       solidFlagL;
        bool       solidFlagR;
        bool       aaFlagL;
        bool       aaFlagR;
        bool       solidFlag;
        bool       rightSideCalc;

        WidthsType() : 
            solidWidthL(0), solidWidthR(0), solidWidth(0),
            totalWidthL(0), totalWidthR(0), totalWidth(0),
            widthCoeff(0), 
            solidCoeffL(0), solidCoeffR(0),
            solidLimitL(0), solidLimitR(0),
            totalLimitL(0), totalLimitR(0),
            solidFlagL(0),  solidFlagR(0),
            aaFlagL(0),     aaFlagR(0),
            solidFlag(0),   rightSideCalc(0)
        {}
    };

    struct JoinParamType
    {
        GCoordType dx1SolidL, dy1SolidL, dx1TotalL, dy1TotalL;
        GCoordType dx2SolidL, dy2SolidL, dx2TotalL, dy2TotalL;
        GCoordType dx3SolidL, dy3SolidL, dx3TotalL, dy3TotalL;
        GCoordType dx1SolidR, dy1SolidR, dx1TotalR, dy1TotalR;
        GCoordType dx2SolidR, dy2SolidR, dx2TotalR, dy2TotalR;
        GCoordType dx3SolidR, dy3SolidR, dx3TotalR, dy3TotalR;

        GCoordType dbSolidL, dbTotalL, dbSolidR, dbTotalR;

        GCoordType xMiterPrevL, yMiterPrevL;
        GCoordType xMiterPrevR, yMiterPrevR; 
        GCoordType dMiterPrevL, dMiterPrevR; 

        GCoordType xMiterThisL, yMiterThisL;
        GCoordType xMiterThisR, yMiterThisR;
        GCoordType dMiterThisL, dMiterThisR; 

        GCoordType xMiterNextL, yMiterNextL;
        GCoordType xMiterNextR, yMiterNextR;
        GCoordType dMiterNextL, dMiterNextR;

        GCoordType xSolidMiterL, ySolidMiterL;
        GCoordType xSolidMiterR, ySolidMiterR;
        GCoordType dSolidMiterL, dSolidMiterR;

        bool badMiterPrevL, badMiterPrevR;
        bool badMiterThisL, badMiterThisR;
        bool badMiterNextL, badMiterNextR;
        bool rightTurnPrev, rightTurnThis, rightTurnNext;
        bool overlapPrev, overlapThis;

        JoinParamType() : 
            dx1SolidL(0), dy1SolidL(0), dx1TotalL(0), dy1TotalL(0),
            dx2SolidL(0), dy2SolidL(0), dx2TotalL(0), dy2TotalL(0),
            dx3SolidL(0), dy3SolidL(0), dx3TotalL(0), dy3TotalL(0),
            dx1SolidR(0), dy1SolidR(0), dx1TotalR(0), dy1TotalR(0),
            dx2SolidR(0), dy2SolidR(0), dx2TotalR(0), dy2TotalR(0),
            dx3SolidR(0), dy3SolidR(0), dx3TotalR(0), dy3TotalR(0),
            dbSolidL(0), dbTotalL(0), dbSolidR(0), dbTotalR(0),
            xMiterPrevL(0), yMiterPrevL(0),
            xMiterPrevR(0), yMiterPrevR(0), 
            dMiterPrevL(0), dMiterPrevR(0), 
            xMiterThisL(0), yMiterThisL(0),
            xMiterThisR(0), yMiterThisR(0),
            dMiterThisL(0), dMiterThisR(0), 
            xMiterNextL(0), yMiterNextL(0),
            xMiterNextR(0), yMiterNextR(0),
            dMiterNextL(0), dMiterNextR(0),
            xSolidMiterL(0), ySolidMiterL(0),
            xSolidMiterR(0), ySolidMiterR(0),
            dSolidMiterL(0), dSolidMiterR(0),
            badMiterPrevL(0), badMiterPrevR(0),
            badMiterThisL(0), badMiterThisR(0),
            badMiterNextL(0), badMiterNextR(0),
            rightTurnPrev(0), rightTurnThis(0), rightTurnNext(0),
            overlapPrev(0), overlapThis(0) {}
    };

    unsigned addVertex(GCoordType x, GCoordType y, int id)
    {
        Vertices.add(VertexType(x, y, id));
        return Vertices.size() - 1;
    }

    void addTriangle(unsigned v1, unsigned v2, unsigned v3)
    {
        TriangleType tri;
        tri.v1 = v1; tri.v2 = v2; tri.v3 = v3;
        Triangles.add(tri);
    }

    void calcWidths(WidthsType& w) const;

    void calcButtCap(const GStrokeVertexType& v1, 
                     const GStrokeVertexType& v2, 
                     GCoordType len,
                     const WidthsType& w,
                     bool endFlag);

    void calcRoundCap(const GStrokeVertexType& v1, 
                      const GStrokeVertexType& v2, 
                      GCoordType len,
                      const WidthsType& w,
                      bool endFlag);

    void calcCap(const GStrokeVertexType& v1, 
                 const GStrokeVertexType& v2, 
                 GCoordType len,
                 const WidthsType& w,
                 bool endFlag);

    void calcRoundJoin(const GStrokeVertexType& v1, 
                       const WidthsType& w,
                       const JoinParamType& p);

    void calcBevelJoin(const GStrokeVertexType& v1, 
                       const WidthsType& w,
                       const JoinParamType& p,
                       LineJoinType lineJoin);

    void calcMiterJoin(const GStrokeVertexType& v1, 
                       const WidthsType& w,
                       JoinParamType& p,
                       LineJoinType lineJoin);

    static
    bool MitersIntersect(GCoordType ax, GCoordType ay, 
                         GCoordType bx, GCoordType by,
                         GCoordType cx, GCoordType cy, 
                         GCoordType dx, GCoordType dy,
                         GCoordType epsilon);

    void calcJoinParam(const GStrokeVertexType& v1, 
                       const GStrokeVertexType& v2,
                       const GStrokeVertexType& v3,
                       const WidthsType& w,
                       JoinParamType& p);

    void calcInitialJoinParam(const GStrokeVertexType& v1, 
                              const GStrokeVertexType& v2,
                              const WidthsType& w,
                              JoinParamType& p);

    void calcJoin(const GStrokeVertexType& v1, 
                  const GStrokeVertexType& v2, 
                  const GStrokeVertexType& v3,
                  const WidthsType& w,
                  JoinParamType& p);

    void calcButtJoin(const GStrokeVertexType& v1, 
                      const GStrokeVertexType& v2, 
                      GCoordType len, 
                      const WidthsType& w);

    void tessellate();

    LineJoinType              LineJoin;
    LineCapType               StartLineCap;
    LineCapType               EndLineCap;
    GCoordType                MiterLimit;
    int                       StyleLeft;
    int                       StyleRight;
    GCoordType                WidthLeft; 
    GCoordType                WidthRight;
    GCoordType                AaWidthLeft;
    GCoordType                AaWidthRight;
    GCoordType                Tolerance;
    GStrokePath               Path;
    GPodBVector<VertexType>   Vertices;
    GPodBVector<TriangleType> Triangles;
    unsigned                  SolidL;
    unsigned                  SolidR;
    unsigned                  TotalL;
    unsigned                  TotalR;
};


#endif

