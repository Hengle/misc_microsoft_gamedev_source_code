/**********************************************************************

Filename    :   GFxDrawingContext.cpp
Content     :   Drawing API implementation
Created     :   Aug 7, 2007
Authors     :   Maxim Shemanarev, Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.
Notes       :   


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxDrawingContext.h"

GFxDrawingContext::GFxDrawingContext() : 
    Shapes(*new GFxShapeWithStylesDef()),
    PathPacker(),
    FillStyle(),
    LineStyle(),
    NewShapeFlag(false),
    FreshLineStyle(false)
{
}

void GFxDrawingContext::Clear()
{
    PathPacker.Reset();
    Shapes = *new GFxShapeWithStylesDef();
    NewShapeFlag = false;
}

void GFxDrawingContext::SetNoLine()
{
    PathPacker.SetLine(0);
    FreshLineStyle = false;
}

void GFxDrawingContext::SetNoFill()
{
    PathPacker.SetFill0(0);
    PathPacker.SetFill1(0);
}

void GFxDrawingContext::SetLineStyle(Float lineWidth, 
                                     UInt  rgba, 
                                     bool  hinting, 
                                     GFxLineStyle::LineStyle scaling, 
                                     GFxLineStyle::LineStyle caps,
                                     GFxLineStyle::LineStyle joins,
                                     Float miterLimit)
{
    LineStyle.SetWidth(UInt16(lineWidth));
    LineStyle.SetColor(rgba);
#ifndef GFC_NO_FXPLAYER_STROKER
    LineStyle.SetHinting(hinting);
    LineStyle.SetScaling(scaling);
    LineStyle.SetCaps(caps);
    LineStyle.SetJoin(joins);
    LineStyle.SetMiterSize(miterLimit);
#else
    GUNUSED2(hinting, scaling);
    GUNUSED3(caps, joins, miterLimit);
#endif
    PathPacker.SetLine(Shapes->AddLineStyle(LineStyle));
    FreshLineStyle = true;
}

// The function begins new fill with an "empty" style. 
// It returns the pointer to just created fill style, so the caller
// can set any values. The function is used in Action Script, beginGradientFill
GFxFillStyle* GFxDrawingContext::SetNewFill()
{
    FillStyle.SetFillType(GFxFill_Solid);
    FillStyle.SetColor(0);
    PathPacker.SetFill0(Shapes->AddFillStyle(FillStyle));
    PathPacker.SetFill1(0);
    return &Shapes->GetLastFillStyle();
}

GFxFillStyle* GFxDrawingContext::CreateLineComplexFill()
{
    if(!FreshLineStyle)
        PathPacker.SetLine(Shapes->AddLineStyle(LineStyle));
    return Shapes->GetLastLineStyle().CreateComplexFill();
}

void GFxDrawingContext::SetNonZeroFill(bool fill)
{
    Shapes->SetNonZeroFill(fill);
}

void GFxDrawingContext::SetFill(UInt rgba)
{
    FillStyle.SetFillType(GFxFill_Solid);
    FillStyle.SetColor(rgba);
    PathPacker.SetFill0(Shapes->AddFillStyle(FillStyle));
    PathPacker.SetFill1(0);
}


void GFxDrawingContext::SetBitmapFill(GFxFillType fillType,
                                      GFxImageResource* pimageRes,
                                      const Matrix& mtx)
{
    GFxFillStyle* style = SetNewFill();
    if (style)
    {
        style->SetImageFill(fillType, pimageRes, mtx);
    }
}

void GFxDrawingContext::ResetFill()
{
    PathPacker.SetFill0(0);
    PathPacker.SetFill1(0);
}

void GFxDrawingContext::MoveTo(Float x, Float y)
{
    PathPacker.SetMoveTo(SInt(x), 
                         SInt(y));
}

void GFxDrawingContext::LineTo(Float x, Float y)
{
    PathPacker.LineToAbs(SInt(x), 
                         SInt(y));
    FreshLineStyle = false;
}

void GFxDrawingContext::CurveTo(Float cx, Float cy, Float ax, Float ay)
{
    PathPacker.CurveToAbs(SInt(cx), 
                          SInt(cy),
                          SInt(ax), 
                          SInt(ay));
    FreshLineStyle = false;
}

bool GFxDrawingContext::AcquirePath(bool newShapeFlag)
{
    if(!PathPacker.IsEmpty())
    {
        if(PathPacker.GetFill0() != 0)
        {
            PathPacker.ClosePath();
        }

        if(NewShapeFlag)
        {
            Shapes->StartNewShape();
        }

        Shapes->AddPath(&PathPacker);
        Shapes->ResetCache();
        Shapes->SetValidBoundsFlag(false);
        PathPacker.ResetEdges();
        PathPacker.SetMoveToLastVertex();
        NewShapeFlag = newShapeFlag;
        return true;
    }
    return false;
}

void GFxDrawingContext::AddPath()
{
    Shapes->AddPath(&PathPacker);
}

void GFxDrawingContext::ComputeBound(GRectF *pRect) const
{
    GASSERT(pRect);
    const_cast<GFxDrawingContext*>(this)->AcquirePath(true);
    Shapes->ComputeBound(pRect);
}

void GFxDrawingContext::Display(GFxDisplayContext &context, 
                                const Matrix& mat, 
                                const Cxform& cx, 
                                BlendType blend, 
                                bool edgeAADisabled)
{
    AcquirePath(true);

    if(!Shapes->IsEmpty())
    {
        if(!Shapes->HasValidBounds())
        {
            GRectF bounds;
            Shapes->ComputeBound(&bounds);
            Shapes->SetBound(bounds);
            //Shapes->SetRectBound(bounds); // TODO: Initialize it for Flash-8 strokes features (not yet supported)
            Shapes->SetValidBoundsFlag(true);
        }
        GFxDisplayParams params(context, mat, cx, blend);
        Shapes->GetFillAndLineStyles(&params);
        Shapes->Display(params, edgeAADisabled, 0);
    }
}

bool GFxDrawingContext::DefPointTestLocal(const GPointF &pt, bool testShape, const GFxCharacter *pinst) const
{
    const_cast<GFxDrawingContext*>(this)->AcquirePath(true);
    return (Shapes->DefPointTestLocal(pt, testShape, pinst));
}

