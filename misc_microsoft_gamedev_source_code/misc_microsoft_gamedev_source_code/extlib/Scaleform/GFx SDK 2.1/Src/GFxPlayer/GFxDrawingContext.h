/**********************************************************************

Filename    :   GFxDrawingContext.h
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

#ifndef INC_GFXDRAWINGCONTEXT_H
#define INC_GFXDRAWINGCONTEXT_H

#include "GFxShape.h"
#include "GRenderer.h"

//
// GFxDrawingContext
//

class GFxDrawingContext : public GRefCountBase<GFxDrawingContext>
{
public:
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GRenderer::BlendType    BlendType;
protected:
    GPtr<GFxShapeWithStylesDef> Shapes;
    GFxPathPacker              PathPacker;
    GFxFillStyle               FillStyle;
    GFxLineStyle               LineStyle;
    bool                       NewShapeFlag;
    bool                       FreshLineStyle;

public:
    GFxDrawingContext();
    ~GFxDrawingContext() {}

    // Add accumulated data from FGxPackedPath to GFxShapeCharacterDef.
    // Returns true if the path is not empty.
    // Function is used internally. 
    bool AcquirePath(bool newShapeFlag);
    void AddPath();

    void SetNonZeroFill(bool fill);

    // The function begins new fill with an "empty" style. 
    // It returns the pointer to just created fill style, so the caller
    // can set any values. The function is used in Action Script, beginGradientFill
    GFxFillStyle* SetNewFill();
    void SetFill(UInt rgba);

    void SetBitmapFill(
        GFxFillType fillType,
        GFxImageResource* pimageRes,
        const Matrix& mtx);

    void Clear();
    void ComputeBound(GRectF *pRect) const;

    GFxFillStyle* CreateLineComplexFill();

    void Display(GFxDisplayContext &context, 
                 const Matrix& mat, 
                 const Cxform& cx, 
                 BlendType blend, 
                 bool edgeAADisabled);

    void ResetFill();

    // all coordinates are in twips!
    void MoveTo(Float x, Float y);
    void LineTo(Float x, Float y);
    void CurveTo(Float cx, Float cy, Float ax, Float ay);

    bool DefPointTestLocal(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const;

    void StartNewShape() { Shapes->StartNewShape(); }
    void SetNoLine();
    void SetNoFill();
    // lineWidth - in twips
    void SetLineStyle(  
        Float lineWidth, 
        UInt  rgba, 
        bool  hinting, 
        GFxLineStyle::LineStyle scaling, 
        GFxLineStyle::LineStyle caps,
        GFxLineStyle::LineStyle joins,
        Float miterLimit);

    bool IsEmpty() const 
    { 
        return !Shapes || Shapes->IsEmpty();
    }
};

#endif //INC_GFXDRAWINGCONTEXT_H
