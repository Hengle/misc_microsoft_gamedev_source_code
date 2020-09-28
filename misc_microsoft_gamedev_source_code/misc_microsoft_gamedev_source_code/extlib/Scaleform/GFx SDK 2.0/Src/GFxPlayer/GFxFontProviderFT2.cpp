/**********************************************************************

Filename    :   GFxFontProviderFT2.cpp
Content     :   FreeType2 font provider
Created     :   6/21/2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

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
**********************************************************************/

#include "GFxFontProviderFT2.h"
#include "GFxShapeCache.h"
#include "GFxString.h"
#include "GMath2D.h"


static const GCoordType GFxExternalFontFT2_CubicTolerance = 2.0f;

namespace GMath2D
{
    // The code of Cubic to Qadratic approximation was taken from the 
    // Anti-Grain Geometry research works and modified for the use by Scaleform. 
    // Permission to use without restrictions is hereby granted to 
    // Scaleform Corporation by the author of Anti-Grain Geometry Project.
    // See http://antigtain.com for details.
    //-----------------------------------------------------------------------




    //-----------------------------------------------------------------------
    template<class Path> 
    void SubdivideCubicToQuadratic(GCoordType x1, GCoordType y1,
                                   GCoordType x2, GCoordType y2,
                                   GCoordType x3, GCoordType y3,
                                   GCoordType x4, GCoordType y4, 
                                   Path& path, GCoordType tolerance)
    {
        GCoordType xc;
        GCoordType yc;
        if(!CalcIntersection(x1, y1, x2, y2, x3, y3, x4, y4, &xc, &yc, tolerance))
        {
            xc = (x2 + x3) / 2;
            yc = (y2 + y3) / 2;
        }

        GCoordType x12   = (x1 + xc ) / 2;
        GCoordType y12   = (y1 + yc ) / 2;
        GCoordType x23   = (xc + x4 ) / 2;
        GCoordType y23   = (yc + y4 ) / 2;
        GCoordType xq    = (x12+ x23) / 2;
        GCoordType yq    = (y12+ y23) / 2;

                   x12   = (x1  + x2  ) / 2;
                   y12   = (y1  + y2  ) / 2;
                   x23   = (x2  + x3  ) / 2;
                   y23   = (y2  + y3  ) / 2;
        GCoordType x34   = (x3  + x4  ) / 2;
        GCoordType y34   = (y3  + y4  ) / 2;
        GCoordType x123  = (x12 + x23 ) / 2;
        GCoordType y123  = (y12 + y23 ) / 2;
        GCoordType x234  = (x23 + x34 ) / 2;
        GCoordType y234  = (y23 + y34 ) / 2;
        GCoordType x1234 = (x123+ x234) / 2;
        GCoordType y1234 = (y123+ y234) / 2;

        GCoordType d;
        d = fabsf(fabsf(CalcLinePointDistance(x1, y1, x4, y4, xq, yq)) - 
                  fabsf(CalcLinePointDistance(x1, y1, x4, y4, x1234, y1234))) +
                  fabsf(CalcLinePointDistance(x123, y123, x234, y234, xq, yq));


        if(d < tolerance)
        {
            path.CurveToAbs(SInt(xc), SInt(yc), SInt(x4), SInt(y4));
            return;
        }
        SubdivideCubicToQuadratic(x1, y1, x12, y12, x123, y123, x1234, y1234, path, tolerance);
        SubdivideCubicToQuadratic(x1234, y1234, x234, y234, x34, y34, x4, y4, path, tolerance);
    }



    //-----------------------------------------------------------------------
    struct CubicCoordType
    {
        GCoordType x1, y1, x2, y2, x3, y3, x4, y4;
    };


    //-----------------------------------------------------------------------
    template<class CurveType>
    void SubdivideCubicCurve(const CurveType& c, GCoordType t, 
                             CurveType* c1, CurveType* c2)
    {
        // Local variables are necessary in case of reuse curve c,
        // that is, when c1 or c2 points to the same physical c.
        //---------------------
        GCoordType x1    = c.x1;
        GCoordType y1    = c.y1;
        GCoordType x12   = c.x1 + t*(c.x2 - c.x1);
        GCoordType y12   = c.y1 + t*(c.y2 - c.y1);
        GCoordType x23   = c.x2 + t*(c.x3 - c.x2);
        GCoordType y23   = c.y2 + t*(c.y3 - c.y2);
        GCoordType x34   = c.x3 + t*(c.x4 - c.x3);
        GCoordType y34   = c.y3 + t*(c.y4 - c.y3);
        GCoordType x123  = x12  + t*(x23  - x12 );
        GCoordType y123  = y12  + t*(y23  - y12 );
        GCoordType x234  = x23  + t*(x34  - x23 );
        GCoordType y234  = y23  + t*(y34  - y23 );
        GCoordType x1234 = x123 + t*(x234 - x123);
        GCoordType y1234 = y123 + t*(y234 - y123);
        GCoordType x4    = c.x4;
        GCoordType y4    = c.y4;
        c1->x1 = x1; 
        c1->y1 = y1;
        c1->x2 = x12; 
        c1->y2 = y12; 
        c1->x3 = x123;
        c1->y3 = y123;
        c1->x4 = x1234;
        c1->y4 = y1234;
        c2->x1 = x1234;
        c2->y1 = y1234; 
        c2->x2 = x234;
        c2->y2 = y234; 
        c2->x3 = x34; 
        c2->y3 = y34;
        c2->x4 = x4;
        c2->y4 = y4;
    }

    //-----------------------------------------------------------------------
    template<class Path> 
    void CubicToQuadratic(GCoordType x1, GCoordType y1,
                          GCoordType x2, GCoordType y2,
                          GCoordType x3, GCoordType y3,
                          GCoordType x4, GCoordType y4, 
                          Path& path, GCoordType tolerance)
    {
        GCoordType ax  =   -x1 + 3*x2 - 3*x3 + x4;
        GCoordType ay  =   -y1 + 3*y2 - 3*y3 + y4;
        GCoordType bx  =  3*x1 - 6*x2 + 3*x3;
        GCoordType by  =  3*y1 - 6*y2 + 3*y3;
        GCoordType cx  = -3*x1 + 3*x2;
        GCoordType cy  = -3*y1 + 3*y2;
        GCoordType den = ay*bx - ax*by;

        GCoordType t1 = -1;
        GCoordType t2 = -1;

        if (den != 0)
        {
            GCoordType tc = -0.5f * (ay*cx - ax*cy) / den;
            GCoordType d  = sqrtf(tc*tc - (by*cx - bx*cy) / (3*den));
            t1 = tc - d;
            t2 = tc + d;
        }

        unsigned numSubcurves = 1;
        CubicCoordType cc;
        CubicCoordType sc[3];
        cc.x1 = x1; cc.y1 = y1;
        cc.x2 = x2; cc.y2 = y2;
        cc.x3 = x3; cc.y3 = y3;
        cc.x4 = x4; cc.y4 = y4;

        switch(int(t2 > 0 && t2 < 1) * 2 + int(t1 > 0 && t1 < 1))
        {
        case 0:
            sc[0] = cc;
            numSubcurves = 1;
            break;

        case 1:
            SubdivideCubicCurve(cc, t1, &sc[0], &sc[1]);
            numSubcurves = 2;
            break;

        case 2:
            SubdivideCubicCurve(cc, t2, &sc[0], &sc[1]);
            numSubcurves = 2;
            break;

        case 3:
            if(t2 < t1) 
            {
                GCoordType t = t1; t1 = t2; t2 = t;
            }
            SubdivideCubicCurve(cc, t1, &sc[0], &sc[1]);
            SubdivideCubicCurve(sc[1], (t2 - t1) / (1 - t1), &sc[1], &sc[2]);
            numSubcurves = 3;
            break;
        }

        unsigned i;
        for(i = 0; i < numSubcurves; ++i)
        {
            const CubicCoordType& c = sc[i];
            SubdivideCubicToQuadratic(c.x1, c.y1, c.x2, c.y2, c.x3, c.y3, c.x4, c.y4, path, tolerance);
        }
    }



} // namespace GMath2D


//------------------------------------------------------------------------
bool GFxExternalFontFT2::decomposeGlyphOutline(const FT_Outline& outline,
                                               GFxShapeCharacterDef* shape)
{   
    FT_Vector   v_last;
    FT_Vector   v_control;
    FT_Vector   v_start;

    GFxPathPacker path;

    FT_Vector*  point;
    FT_Vector*  limit;
    char*       tags;

    int   n;         // index of contour in outline
    int   first;     // index of first point in contour
    char  tag;       // current point's state

    first = 0;

    for(n = 0; n < outline.n_contours; n++)
    {
        int  last;  // index of last point in contour

        last  = outline.contours[n];
        limit = outline.points + last;

        v_start = outline.points[first];
        v_last  = outline.points[last];

        v_control = v_start;

        point = outline.points + first;
        tags  = outline.tags  + first;
        tag   = FT_CURVE_TAG(tags[0]);

        // A contour cannot start with a cubic control point!
        if(tag == FT_CURVE_TAG_CUBIC) return false;

        // check first point to determine origin
        if( tag == FT_CURVE_TAG_CONIC)
        {
            // first point is conic control.  Yes, this happens.
            if(FT_CURVE_TAG(outline.tags[last]) == FT_CURVE_TAG_ON)
            {
                // start at last point if it is on the curve
                v_start = v_last;
                limit--;
            }
            else
            {
                // if both first and last points are conic,
                // start at their middle and record its position
                // for closure
                v_start.x = (v_start.x + v_last.x) / 2;
                v_start.y = (v_start.y + v_last.y) / 2;

                v_last = v_start;
            }
            point--;
            tags--;
        }

        path.Reset();
        path.SetFill0(1);
        path.SetFill1(0);
        path.SetMoveTo(v_start.x >> 6, -v_start.y >> 6);

        while(point < limit)
        {
            point++;
            tags++;

            tag = FT_CURVE_TAG(tags[0]);
            switch(tag)
            {
                case FT_CURVE_TAG_ON:  // emit a single line_to
                {
                    path.LineToAbs(point->x >> 6, -point->y >> 6);
                    continue;
                }

                case FT_CURVE_TAG_CONIC:  // consume conic arcs
                {
                    v_control.x = point->x;
                    v_control.y = point->y;

                Do_Conic:
                    if(point < limit)
                    {
                        FT_Vector vec;
                        FT_Vector v_middle;

                        point++;
                        tags++;
                        tag = FT_CURVE_TAG(tags[0]);

                        vec.x = point->x;
                        vec.y = point->y;

                        if(tag == FT_CURVE_TAG_ON)
                        {
                            path.CurveToAbs(v_control.x >> 6, -v_control.y >> 6,
                                            vec.x       >> 6, -vec.y       >> 6);

                            continue;
                        }

                        if(tag != FT_CURVE_TAG_CONIC) return false;

                        v_middle.x = (v_control.x + vec.x) / 2;
                        v_middle.y = (v_control.y + vec.y) / 2;
                        path.CurveToAbs(v_control.x >> 6, -v_control.y >> 6,
                                        v_middle.x  >> 6, -v_middle.y  >> 6);


                        v_control = vec;
                        goto Do_Conic;
                    }
                    path.CurveToAbs(v_control.x >> 6, -v_control.y >> 6,
                                    v_start.x   >> 6, -v_start.y   >> 6);
                    goto Close;
                }

                default:  // FT_CURVE_TAG_CUBIC
                {
                    FT_Vector vec1, vec2;
                    SInt lastX, lastY;

                    if(point + 1 > limit || FT_CURVE_TAG(tags[1]) != FT_CURVE_TAG_CUBIC)
                    {
                        return false;
                    }

                    vec1.x = point[0].x; 
                    vec1.y = point[0].y;
                    vec2.x = point[1].x; 
                    vec2.y = point[1].y;

                    point += 2;
                    tags  += 2;

                    path.GetLastVertex(&lastX, &lastY);
                    if(point <= limit)
                    {
                        FT_Vector vec;

                        vec.x = point->x;
                        vec.y = point->y;
                        GMath2D::CubicToQuadratic(GCoordType(lastX),       GCoordType(lastY), // While cubics aren't available
                                                  GCoordType(vec1.x >> 6), GCoordType(-vec1.y >> 6),
                                                  GCoordType(vec2.x >> 6), GCoordType(-vec2.y >> 6),
                                                  GCoordType(vec.x  >> 6), GCoordType(-vec.y  >> 6),
                                                  path, GFxExternalFontFT2_CubicTolerance);
                        //path.CubicToAbs(vec1.x >> 6, -vec1.y >> 6, // Uncomment wnen available
                        //                vec2.x >> 6, -vec2.y >> 6,
                        //                vec.x  >> 6, -vec.y  >> 6);
                        continue;
                    }

                    //path.CubicToAbs(vec1.x   >> 6, -vec1.y    >> 6, // Uncomment wnen available
                    //                vec2.x   >> 6, -vec2.y    >> 6,
                    //                v_start.x>> 6, -v_start.y >> 6);
                    GMath2D::CubicToQuadratic(GCoordType(lastX),          GCoordType(lastY), // While cubics aren't available
                                              GCoordType(vec1.x    >> 6), GCoordType(-vec1.y    >> 6),
                                              GCoordType(vec2.x    >> 6), GCoordType(-vec2.y    >> 6),
                                              GCoordType(v_start.x >> 6), GCoordType(-v_start.y >> 6),
                                              path, GFxExternalFontFT2_CubicTolerance);
                    goto Close;
                }
            }
        }

    Close:
        path.ClosePath();
        shape->AddPath(&path);
        first = last + 1; 
    }
    return true;
}







//------------------------------------------------------------------------
GFxExternalFontFT2::~GFxExternalFontFT2()
{
    if (Face)
        FT_Done_Face(Face);
}

//------------------------------------------------------------------------
GFxExternalFontFT2::GFxExternalFontFT2(GFxFontProviderFT2* pprovider, 
                                       FT_Library lib, 
                                       const GFxString& fontName, 
                                       UInt fontFlags,
                                       const char* fileName, 
                                       UInt faceIndex) :
    GFxFont(fontFlags),
    pFontProvider(pprovider),
    Name(fontName)
{
    int err = FT_New_Face(lib, fileName, faceIndex, &Face);
    if (err)
    {
        Face = 0;
        return;
    }
    setFontMetrics();
}

//------------------------------------------------------------------------
GFxExternalFontFT2::GFxExternalFontFT2(GFxFontProviderFT2* pprovider, 
                                       FT_Library lib, 
                                       const GFxString& fontName, 
                                       UInt fontFlags,
                                       const char* fontMem, 
                                       UInt fontMemSize, 
                                       UInt faceIndex) :
    GFxFont(fontFlags),
    pFontProvider(pprovider),
    Name(fontName)
{
    int err = FT_New_Memory_Face(lib, 
                                 (const FT_Byte*)fontMem, 
                                 fontMemSize, 
                                 faceIndex,
                                 &Face);
    if (err)
    {
        Face = 0;
        return;
    }
    setFontMetrics();
}

//------------------------------------------------------------------------
void GFxExternalFontFT2::setFontMetrics()
{
    FT_Set_Pixel_Sizes(Face, FontHeight, FontHeight);
    Float ascent  =  Float(Face->ascender)  * FontHeight / Face->units_per_EM;
    Float descent = -Float(Face->descender) * FontHeight / Face->units_per_EM;
    Float height  =  Float(Face->height)    * FontHeight / Face->units_per_EM;
    SetFontMetrics(height - ascent + descent, ascent, descent);
}


//------------------------------------------------------------------------
int GFxExternalFontFT2::GetGlyphIndex(UInt16 code)
{
    if (Face)
    {
        UInt* indexPtr = CodeTable.get(code);
        if (indexPtr)
            return *indexPtr;

        UInt ftIndex = FT_Get_Char_Index(Face, code);
        int  err     = FT_Load_Glyph(Face, ftIndex, FT_LOAD_NO_HINTING);

        if (err)
            return -1;

        GlyphType glyph;
        glyph.FtIndex       =  ftIndex;
        glyph.Advance       =  Float((Face->glyph->advance.x + 32) >> 6);
        glyph.Bounds.Left   =  Float(Face->glyph->metrics.horiBearingX >> 6);
        glyph.Bounds.Top    = -Float(Face->glyph->metrics.horiBearingY >> 6);
        glyph.Bounds.Right  =  Float(Face->glyph->metrics.width  >> 6) + glyph.Bounds.Left;
        glyph.Bounds.Bottom =  Float(Face->glyph->metrics.height >> 6) + glyph.Bounds.Top;
        glyph.pShapeNode    =  0;
        glyph.Stamp         =  0;

        Glyphs.push_back(glyph);
        CodeTable.add(code, Glyphs.size()-1);
        return Glyphs.size()-1;
    }
    return -1;
}

//------------------------------------------------------------------------
GFxShapeCharacterDef* GFxExternalFontFT2::GetGlyphShape(UInt glyphIndex)
{
    GlyphType& glyph = Glyphs[glyphIndex];

    if (glyph.pShapeNode &&
        glyph.pShapeNode->pShape.GetPtr() &&
        glyph.pShapeNode->Stamp == glyph.Stamp)
    {
        pFontProvider->GetShapeCache()->SendToBack(glyph.pShapeNode);
        return glyph.pShapeNode->pShape;
    }

    int err = FT_Load_Glyph(Face,
                            Glyphs[glyphIndex].FtIndex,
                            FT_LOAD_DEFAULT);//FT_LOAD_NO_HINTING);
    if (err)
        return 0;

    glyph.pShapeNode = pFontProvider->GetShapeCache()->GetNewShapeNode();
    if (Face->glyph->outline.n_contours)
    {
        glyph.pShapeNode->pShape = *new GFxShapeCharacterDef(ShapePageSize);
        glyph.Stamp = glyph.pShapeNode->Stamp;
        if(!decomposeGlyphOutline(Face->glyph->outline, glyph.pShapeNode->pShape))
        {
            glyph.pShapeNode->pShape = 0;
        }
    }
    return glyph.pShapeNode->pShape;
}

//------------------------------------------------------------------------
Float GFxExternalFontFT2::GetAdvance(UInt glyphIndex) const
{
    return Glyphs[glyphIndex].Advance;
}

//------------------------------------------------------------------------
Float GFxExternalFontFT2::GetKerningAdjustment(UInt lastCode, UInt thisCode) const
{
    if(Face && FT_HAS_KERNING(Face))
    {
        FT_Vector delta;
        FT_Get_Kerning(Face, 
                       FT_Get_Char_Index(Face, lastCode), 
                       FT_Get_Char_Index(Face, thisCode),
                       FT_KERNING_DEFAULT, &delta);
        return Float(delta.x >> 6);
    }
    return 0;
}

//------------------------------------------------------------------------
Float GFxExternalFontFT2::GetGlyphWidth(UInt glyphIndex) const
{
    const GRectF& r = Glyphs[glyphIndex].Bounds;
    return r.Width();
}

//------------------------------------------------------------------------
Float GFxExternalFontFT2::GetGlyphHeight(UInt glyphIndex) const
{
    const GRectF& r = Glyphs[glyphIndex].Bounds;
    return r.Height();
}

//------------------------------------------------------------------------
GRectF& GFxExternalFontFT2::GetGlyphBounds(UInt glyphIndex, GRectF* prect) const
{
    *prect = Glyphs[glyphIndex].Bounds;
    return *prect;
}





//------------------------------------------------------------------------
GFxFontProviderFT2::GFxFontProviderFT2(FT_Library lib, UInt maxNumShapes):
    pCache(new GFxShapeCache(maxNumShapes)),
    Lib(lib),
    ExtLibFlag(true),
    NamesEndIdx(0)
{
    if (Lib == 0)
    {
        int err = FT_Init_FreeType(&Lib);
        if (err) 
            Lib = 0;
        ExtLibFlag = false;
    }
}

//------------------------------------------------------------------------
GFxFontProviderFT2::~GFxFontProviderFT2()
{
    if (Lib && !ExtLibFlag)
        FT_Done_FreeType(Lib);
    delete pCache;
}

//------------------------------------------------------------------------
void GFxFontProviderFT2::MapFontToFile(const char* fontName, 
                                       UInt fontFlags,
                                       const char* fileName, 
                                       UInt faceIndex)
{
    // Mask flags to be safe.
    fontFlags &= GFxFont::FF_CreateFont_Mask;
    fontFlags |= GFxFont::FF_DeviceFont;

    FontType font;
    font.FontName     = fontName;
    font.FontFlags    = fontFlags;
    font.FileName     = fileName;
    font.FontData     = 0;
    font.FontDataSize = 0;
    font.FaceIndex    = faceIndex;
    Fonts.push_back(font);
}

//------------------------------------------------------------------------
void GFxFontProviderFT2::MapFontToMemory(const char* fontName, 
                                         UInt fontFlags, 
                                         const char* fontData, UInt dataSize,
                                         UInt faceIndex)
{
    // Mask flags to be safe.
    fontFlags &= GFxFont::FF_CreateFont_Mask;
    fontFlags |= GFxFont::FF_DeviceFont;

    FontType font;
    font.FontName     = fontName;
    font.FontFlags    = fontFlags;
    font.FontData     = fontData;
    font.FontDataSize = dataSize;
    font.FaceIndex    = faceIndex;
    Fonts.push_back(font);
}

//------------------------------------------------------------------------
GFxExternalFontFT2* GFxFontProviderFT2::createFont(FontType* font, const GFxString& fontName, UInt fontFlags)
{
    GFxExternalFontFT2* newFont = font->FontData?
        new GFxExternalFontFT2(this, Lib, fontName, fontFlags, font->FontData, font->FontDataSize, font->FaceIndex):
        new GFxExternalFontFT2(this, Lib, fontName, fontFlags, font->FileName, font->FaceIndex);

    if (newFont && !newFont->IsValid())
    {
        newFont->Release();
        return 0;
    }
    return newFont;
}

//------------------------------------------------------------------------
GFxFont* GFxFontProviderFT2::CreateFont(const char* name, UInt fontFlags)
{
    if (Lib == 0)
        return 0;

    // Mask flags to be safe.
    fontFlags &= GFxFont::FF_CreateFont_Mask;
    fontFlags |= GFxFont::FF_DeviceFont;

    UInt i;
    for (i = 0; i < Fonts.size(); ++i)
    {
        FontType& font = Fonts[i];
        if (font.FontName.CompareNoCase(name) == 0 && 
            fontFlags == font.FontFlags)
        {
            return createFont(&font, font.FontName, fontFlags);
        }
    }

    // Direct mapping is not found, try to find the same name 
    // with other attributes.
    for (i = 0; i < Fonts.size(); ++i)
    {
        FontType& font = Fonts[i];
        if (font.FontName.CompareNoCase(name) == 0)
        {
            return createFont(&font, font.FontName, fontFlags);
        }
    }

    return 0;
}

