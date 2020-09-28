/**********************************************************************

Filename    :   GCompoundShape.h
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

#ifndef INC_GCOMPOUNDSHAPE_H
#define INC_GCOMPOUNDSHAPE_H

#include "GContainers.h"
#include "GMath2D.h"


// ***** Declared Classes
class GCompoundShape;



//-----------------------------------------------------------------------
const GCoordType G_CollinearCurveEpsilon = (GCoordType)1e-4;

//-----------------------------------------------------------------------
class GCompoundShape
{
public:
    class SPath
    {
    friend class GCompoundShape;
    public:
        typedef GPointType ValueType;

        SPath() {}

        int GetLeftStyle()  const { return LStyle;    }
        int GetRightStyle() const { return RStyle;    }
        int GetLineStyle()  const { return LineStyle; }

        unsigned GetNumVertices() const 
        { 
            return NumVertices; 
        }

        const GPointType& GetVertex(unsigned i) const
        {
            return Shape->GetVertex(StartVertex + i);
        }
   
    private:
        SPath(const GCompoundShape* s, unsigned start, 
              int lStyle, int rStyle, int lineStyle) : 
            Shape(s), 
            NumVertices(0), 
            StartVertex(start),
            LStyle(lStyle), 
            RStyle(rStyle),
            LineStyle(lineStyle)
        {}

        void incNumVertices() { ++NumVertices; }

        const GCompoundShape* Shape;
        unsigned              NumVertices;
        unsigned              StartVertex;
        int                   LStyle;
        int                   RStyle;
        int                   LineStyle;
    };

    GCompoundShape() : 
        CurveTolerance(1),
        ToleranceSquare((GCoordType)0.0625),
        Vertices(),
        Paths(),
        CurrPath(0),
        MinStyle( 0x7FFFFFFF),
        MaxStyle(-0x7FFFFFFF),
        MinX(1),
        MinY(1),
        MaxX(0),
        MaxY(0)
    {}

    void        RemoveAll();
    
    void        SetCurveTolerance(GCoordType t);
    GCoordType  GetCurveTolerance() const { return CurveTolerance; }

    void BeginPath(int lStyle, int rStyle, int lineStyle);

    void BeginPath(int lStyle, int rStyle, int lineStyle, 
                   GCoordType x, GCoordType y);

    void AddVertex(GCoordType x, GCoordType y);

    void AddCurve(GCoordType cx, GCoordType cy, 
                  GCoordType ax, GCoordType ay);

    void ClosePath();

    unsigned            GetNumPaths()       const { return Paths.size(); }
    const SPath&        GetPath(unsigned n) const { return Paths[n]; }

    unsigned            GetNumVertices()      const { return Vertices.size(); }
    const GPointType&   GetVertex(unsigned n) const { return Vertices[n]; }

    int GetMinStyle() const { return MinStyle; }
    int GetMaxStyle() const { return MaxStyle; }

    void PerceiveBounds(GCoordType* x1, GCoordType* y1, 
                        GCoordType* x2, GCoordType* y2) const;

    bool PointInShape(GCoordType x, GCoordType y, bool nonZero=true) const;

    void PerceiveBounds();
    bool HasValidBounds() const { return MinX < MaxX && MinY < MaxY; }

    GCoordType GetMinX()     const { return MinX; }
    GCoordType GetMinY()     const { return MinY; }
    GCoordType GetMaxX()     const { return MaxX; }
    GCoordType GetMaxY()     const { return MaxY; }

    static bool PathsEqual(const SPath& a, const SPath& b);

    void ScaleAndTranslate(GCoordType kx, GCoordType ky,
                           GCoordType dx, GCoordType dy);


#ifdef GCONTAINERS_STANDALONE
    template<class VertexSource>
    void AppendPath(VertexSource& vs, unsigned path_id=0)
    {
        double x; // Must remain double
        double y;

        unsigned cmd;
        vs.rewind(path_id);
        while((cmd = vs.vertex(&x, &y)) != 0)
        {
            if(cmd < 3) 
            {
                AddVertex(GCoordType(x), GCoordType(y));
            }
            else
            if(cmd == 3)
            {
                double x2, y2;
                vs.vertex(&x2, &y2);
                AddCurve(GCoordType(x),  GCoordType(y), 
                         GCoordType(x2), GCoordType(y2));
            }
        }
    }

    template<class VertexSource>
    void AppendMultiPath(VertexSource& vs, 
                         int lStyle, int rStyle, int lineStyle, 
                         unsigned path_id=0)
    {
        double x; // Must remain double
        double y;

        unsigned cmd;
        vs.rewind(path_id);
        while((cmd = vs.vertex(&x, &y)) != 0)
        {
            if(cmd < 3) 
            {
                if(cmd == 1)
                {
                    BeginPath(lStyle, rStyle, lineStyle);
                }
                AddVertex(GCoordType(x), GCoordType(y));
            }
            else
            if(cmd == 3)
            {
                double x2, y2;
                vs.vertex(&x2, &y2);
                AddCurve(GCoordType(x),  GCoordType(y), 
                         GCoordType(x2), GCoordType(y2));
            }
            else
            if((cmd & 0x4F) == 0x4F) // Magic number "close polygon"
            {
                ClosePath();
            }
        }
    }
#endif

private:
    void flattenQuadraticCurve(GCoordType x1, GCoordType y1, 
                               GCoordType x2, GCoordType y2, 
                               GCoordType x3, GCoordType y3);

    GCoordType                 CurveTolerance;
    GCoordType                 ToleranceSquare;
    GPodBVector<GPointType, 8> Vertices;
    GPodBVector<SPath, 6>      Paths;
    SPath*                     CurrPath;
    int                        MinStyle;
    int                        MaxStyle;
    GCoordType                 MinX;
    GCoordType                 MinY;
    GCoordType                 MaxX;
    GCoordType                 MaxY;
};

//-----------------------------------------------------------------------
GINLINE void GCompoundShape::AddVertex(GCoordType x, GCoordType y)
{
    Vertices.add(GPointType(x, y));
    CurrPath->incNumVertices();
}

#endif
