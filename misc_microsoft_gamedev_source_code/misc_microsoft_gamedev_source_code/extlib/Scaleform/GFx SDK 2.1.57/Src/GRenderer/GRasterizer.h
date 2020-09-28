/**********************************************************************

Filename    :   GRasterizer.h
Content     :   
Created     :   2005-2006
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Scanline rasterizer with anti-aliasing

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

----------------------------------------------------------------------
The code of these classes was taken from the Anti-Grain Geometry
Project and modified for the use by Scaleform. 
Permission to use without restrictions is hereby granted to 
Scaleform Corporation by the author of Anti-Grain Geometry Project.
See http://antigtain.com for details.

The author of Anti-Grain Geometry gratefully acknowleges the support 
of David Turner, Robert Wilhelm, and Werner Lemberg - the authors of 
the FreeType libray - in producing this work. 
See http://www.freetype.org for details.
**********************************************************************/

#ifndef INC_GRASTERIZER_H
#define INC_GRASTERIZER_H

#include "GTypes.h"
#include "GCompoundShape.h"

#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif
#include <string.h>


// ***** Declared Classes
class GRasterizer;



class GRasterizer
{
    enum
    {
        SubpixelShift = 8,
        SubpixelScale = 1 << SubpixelShift,
        SubpixelMask  = SubpixelScale - 1,

        AntiAliasShift  = 8,
        AntiAliasScale  = 1 << AntiAliasShift,
        AntiAliasMask   = AntiAliasScale - 1,
        AntiAliasScale2 = AntiAliasScale * 2,
        AntiAliasMask2  = AntiAliasScale2 - 1
    };

public:
    enum FillRuleType
    {
        FillNonZero,
        FillEvenOdd
    };

    GRasterizer();

    // Setup (optional)
    //-----------------------------------
    // FillRule = FillNonZero by default
    void            SetFillRule(FillRuleType f) { FillRule = f; }
    FillRuleType    GetFillRule() const         { return FillRule; }

    void            SetGamma(GCoordType g);
    GCoordType      GetGamma() const            { return Gamma; }

    // "Manual" rasterization interface
    //-----------------------------------
    void Reset();
    void MoveTo(GCoordType x, GCoordType y);
    void LineTo(GCoordType x, GCoordType y);
    void ClosePolygon();

    // "Automatic" rasterization. Internally it calls 
    // MoveTo()/LineTo(). The function accumulates
    // the shapes, so that you can add more than one
    // shape and rasterize it as a whole. Call Reset()
    // if you want to start a new rasterization process.
    //-----------------------------------
    void AddShape(const GCompoundShape& shape, 
                  GCoordType ratio = 1, 
                  int activeStyle = -1);

    void AddShapeScaled(const GCompoundShape& shape, 
                        GCoordType scaleX, GCoordType scaleY, 
                        GCoordType transX, GCoordType transY, 
                        int activeStyle = -1);

    // After AddShape() or MoveTo()/LineTo() the 
    // bounding box is valid, as well as the number of 
    // resulting scan lines.
    //-----------------------------------
    int         GetMinX() const { return MinX; }
    int         GetMinY() const { return MinY; }
    int         GetMaxX() const { return MaxX; }
    int         GetMaxY() const { return MaxY; }
    unsigned    GetNumScanlines() const { return SortedYs.size(); }

    // Call SortCells() after the shape is added. It
    // returns true if there's something to sweep and 
    // false if the shape is empty.
    //-----------------------------------
    bool SortCells();

    // Sweep one scan line, that is, rasterize it. 
    // "pRaster" should point to the beginning of the row
    // of gray-scale pixels. The pRaster[0] corresponds to 
    // GetMinX() value. The typical loop is:
    //
    // if(rasterizer.SortCells())
    // {
    //     for(unsigned i = 0; i < rasterizer.NumScanlines(); i++)
    //     {
    //         rasterizer.SweepScanline(i, pRaster + i * rasterWidth);
    //     }
    // }
    //-----------------------------------
    void SweepScanline(unsigned scanline, unsigned char* praster,
                       unsigned numChannels = 1) const;

    void SweepScanlineThreshold(unsigned scanline, unsigned char* praster,
                                unsigned numChannels = 1, 
                                unsigned threshold = AntiAliasMask/2) const;
private:
    struct Cell 
    { 
        int x, y, Cover, Area; 
    };

    struct SortedY 
    { 
        unsigned Start, Count; 
    };

    void line(int x1, int y1, int x2, int y2);
    void horLine(int ey, int x1, int y1, int x2, int y2);

    GINLINE void setCurrCell(int x, int y)
    {
        if((CurrCell.x - x) | (CurrCell.y - y))
        {
            if(CurrCell.Area | CurrCell.Cover)
            {
                Cells.add(CurrCell);
            }
            CurrCell.x     = x;
            CurrCell.y     = y;
            CurrCell.Cover = 0;
            CurrCell.Area  = 0;
        }
    }

    GINLINE static bool cellXLess(const Cell* a, const Cell* b)
    {
        return a->x < b->x;
    }

    GINLINE unsigned calcAlpha(int area) const
    {
        int alpha = area >> (SubpixelShift * 2 + 1 - AntiAliasShift);

        if (alpha < 0) 
            alpha = -alpha;
        if (FillRule == FillEvenOdd)
        {
            alpha &= AntiAliasMask2;
            if (alpha > AntiAliasScale)
                alpha = AntiAliasScale2 - alpha;
        }
        if (alpha > AntiAliasMask) 
            alpha = AntiAliasMask;
        return GammaLut.size() ? GammaLut[alpha] : alpha;
    }
    
    GINLINE unsigned calcAlpha(int area, unsigned threshold) const
    {
        int alpha = area >> (SubpixelShift * 2 + 1 - AntiAliasShift);

        if (alpha < 0) 
            alpha = -alpha;
        if (FillRule == FillEvenOdd)
        {
            alpha &= AntiAliasMask2;
            if (alpha > AntiAliasScale)
                alpha = AntiAliasScale2 - alpha;
        }
        return (alpha <= int(threshold)) ? 0 : AntiAliasMask;
    }
    

    FillRuleType         FillRule;
    GCoordType           Gamma;
    GPodVector<unsigned> GammaLut;
    GPodBVector<Cell,10> Cells;
    GPodVector<Cell*>    SortedCells;
    GPodVector<SortedY>  SortedYs;
    Cell                 CurrCell;
    int                  MinX;
    int                  MinY;
    int                  MaxX;
    int                  MaxY;
    int                  StartX;
    int                  StartY;
    int                  LastX;
    int                  LastY;
};

#endif
