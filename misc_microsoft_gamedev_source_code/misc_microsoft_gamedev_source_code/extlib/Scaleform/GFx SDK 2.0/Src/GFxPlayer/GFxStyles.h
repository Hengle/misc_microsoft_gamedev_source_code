/**********************************************************************

Filename    :   GFxStyles.h
Content     :   SWF (Shockwave Flash) player library
Created     :   July 7, 2005
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXSTYLES_H
#define INC_GFXSTYLES_H

//#include "GFxCharacter.h"
#include "GFxMovieDef.h"


// ***** Declared Classes
class GFxGradientRecord;
class GFxBaseFillStyle;
class GFxFillStyle;
class GFxLineStyle;

// ***** External Classes
class GFxStream;

class GFxImageResource;


// Types of fills possible in the SWF/GFX File
enum  GFxFillType
{
    GFxFill_Solid              = 0x00,
    GFxFill_LinearGradient     = 0x10,
    GFxFill_RadialGradient     = 0x12,
    GFxFill_FocalPointGradient = 0x13,
    GFxFill_TiledSmoothImage   = 0x40,
    GFxFill_ClippedSmoothImage = 0x41,
    GFxFill_TiledImage         = 0x42,
    GFxFill_ClippedImage       = 0x43,

    // Test bits for efficient type discrimination.
    GFxFill_Gradient_TestBit   = 0x10,
    GFxFill_Image_TestBit      = 0x40,
};




#ifndef GFX_GRADIENT_SIZE_DEFAULT
#define GFX_GRADIENT_SIZE_DEFAULT 64
#endif // GFX_GRADIENT_SIZE_DEFAULT

#ifndef GFC_OS_PSP
#define GFX_ADAPTIVE_GRADIENT_SIZE
#endif // GFC_OS_PSP

class GFxGradientRecord
{
public:
    GFxGradientRecord();
    void    Read(GFxLoadProcess* p, GFxTagType tagType);

    UByte   Ratio;
    GColor  Color;

    size_t    GetHashCode() const
    {
        return Ratio ^ (Color.Raw ^ (Color.Raw >> 16));
    }

    bool        operator == (const GFxGradientRecord& other) const
    {
        return (Ratio == other.Ratio) && (Color == other.Color);
    }
    bool        operator != (const GFxGradientRecord& other) const
    {
        return !operator == (other);
    }
};

// Hash-able gradient descriptor - contains all of the info
// about the gradient. 
class GFxGradientData : public GRefCountBase<GFxGradientData>
{
    bool                LinearRGB;
    UByte               Type;
    UInt16              RecordCount;

    GFxGradientRecord*  pRecords;

    Float               FocalRatio;

public:
    GFxGradientData(GFxFillType type, UInt16 recordCount = 0, bool linearRgb = 0);
    ~GFxGradientData();

    bool            SetRecordCount(UInt16 count);
    inline  UInt16  GetRecordCount() const      { return RecordCount; }

    inline void     SetFocalRatio(Float ratio)  { FocalRatio = ratio; }
    inline Float    GetFocalRatio() const       { return FocalRatio; }    

    bool            IsLinearRGB() const         { return LinearRGB; }
    void            SetLinearRGB(bool lrgb)     { LinearRGB = lrgb; }

    GFxFillType     GetFillType() const         { return (GFxFillType)Type; }

    // Record access for modifiction/etc.
    GFxGradientRecord& operator [] (UInt index)
    {
        GASSERT(index < RecordCount);
        return pRecords[index];
    }
    const GFxGradientRecord& operator [] (UInt index) const
    {
        GASSERT(index < RecordCount);
        return pRecords[index];
    }

    // Compute hash code and equality; used for hashing.
    size_t      GetHashCode() const;

    bool        operator == (const GFxGradientData& other) const;
    

    // Creates a GFxResourceData object from this GFxGradientData;
    // this object can be used to create a resource.
    GFxResourceData     CreateGradientResourceData();
    GFxResourceKey      CreateGradientKey();

    GImage*             CreateGradientImage(GFxGradientParams *pparams) const;     
    UInt                ComputeRadialGradientImageSize(GFxGradientParams *pparams) const;

    // Creates an image resource from gradient.
    GFxImageResource*   CreateImageResource(GFxResourceWeakLib *plib,
                                            GFxGradientParams *pparams,
                                            GFxImageCreator *pimageCreator,
                                            GFxRenderConfig* prconfig);
};




class GFxBaseFillStyle : public GNewOverrideBase
{
public:
    virtual ~GFxBaseFillStyle () { }
    virtual void Apply(const GFxDisplayContext& dc,
                       Float scaleMultiplier) const = 0;
};

// For the interior of outline shapes.
class GFxFillStyle : public GFxBaseFillStyle
{
public:
    GFxFillStyle();
    GFxFillStyle(const GFxFillStyle& other);
    virtual ~GFxFillStyle();


    enum FillFlagConstants
    {
        FF_LinearRGB        = 0x01,
        FF_HasAlpha         = 0x02
    };


    void            Read(GFxLoadProcess* p, GFxTagType tagType);


    virtual void    Apply(const GFxDisplayContext& dc,
                          Float scaleMultiplier) const;

    // Creates a FillTexture for a style
    bool            GetFillTexture(GRenderer::FillTexture *ptexture,
                                   const GFxDisplayContext& dc,
                                   Float scaleMultiplier) const;

    // Returns 1 if the line style can blend (i.e. has alpha values).
    bool            CanBlend() const            { return (FillFlags & FF_HasAlpha) != 0; }
    bool            IsLinearRGB() const         { return (FillFlags & FF_LinearRGB) != 0; }

    GFxFillType     GetType() const             { return (GFxFillType)Type; }
    bool            IsSolidFill() const         { return (Type == GFxFill_Solid); }
    bool            IsGradientFill() const      { return (Type & GFxFill_Gradient_TestBit) != 0; }
    bool            IsImageFill() const         { return (Type & GFxFill_Image_TestBit) != 0; }

    GColor          GetColor() const            { GASSERT(IsSolidFill()); return GColor(RawColor); }
    void            SetColor(GColor newColor)   { GASSERT(IsSolidFill()); RawColor = newColor.Raw; }

    GFxGradientData* GetGradientData() const    { GASSERT(IsGradientFill()); return pGradientData; }
    
    // Must have assignment operator because it is used when
    // fill styles are stored in an array.
    GFxFillStyle& operator = (const GFxFillStyle& src);


    // Apply different types of fill.
    void            SetFillType(GFxFillType type);

    // Sets fill style to an image, used to support externally loaded images.
    void            SetImageFill(GFxFillType fillType,
                                 GFxImageResource *pimage,
                                 const GRenderer::Matrix& mtx);

    // Sets a fill to a gradient. Users should be careful to not modify
    // gradient data after creation if it is already used to hash images.
    void            SetGradientFill(GFxFillType fillType,
                                    GFxGradientData *pgradientData,
                                    const GRenderer::Matrix& mtx);


    // For shape morphing
    void            SetLerp(const GFxFillStyle& a, const GFxFillStyle& b, Float t);

private:
    friend class GFxMorphCharacterDef;


    UByte           Type;
    UByte           FillFlags;
  
    // SWF 8 gradient/Color data.
    union {
        UInt32              RawColor;
        GFxGradientData*    pGradientData;
    };

    GFxResourcePtr<GFxImageResource>    pImage;

    // Matrix used to transform gradients or bitmaps.
    GRenderer::Matrix       ImageMatrix;
};


// For the outside of outline shapes, or just bare lines.
class GFxLineStyle
{
    friend class GFxMorphCharacterDef;
        
    UInt16          Width;          // in TWIPS
    GColor          Color;
    GFxFillStyle*   pComplexFill;   // Complex fill, if not null.

    // Need to keep StyleFlags even with GFC_NO_FXPLAYER_STROKER
    // because complex fill loading logic checks for them.
    UInt16      StyleFlags;

#ifndef GFC_NO_FXPLAYER_STROKER
    Float       MiterSize;  // 1 - 60, fp
#endif

public:
    GFxLineStyle();
    GFxLineStyle(const GFxLineStyle& other);
    virtual ~GFxLineStyle();

    // Line styles are stored in an array, so implement =
    // for safety of data structures on assignment.
    GFxLineStyle&   operator = (const GFxLineStyle& src);

    void            Read(GFxLoadProcess* p, GFxTagType tagType);
    virtual void    Apply(GRenderer *prenderer) const;

    // Returns 1 if the line style can blend (i.e. has alpha values).
    bool            CanBlend() const 
    { return (!pComplexFill) ? (Color.GetAlpha() < 255) : pComplexFill->CanBlend(); }


    enum LineStyle
    {
        LineFlag_StrokeHinting  = 0x0001,
        // Scaling
        LineScaling_Normal      = 0x0000,
        LineScaling_Horizontal  = 0x0002,
        LineScaling_Vertical    = 0x0004,
        LineScaling_None        = 0x0006,
        LineScaling_Mask        = 0x0006,

        // Complex fill flag
        LineFlag_ComplexFill    = 0x0008,

        // Joins
        LineJoin_Round          = 0x0000,
        LineJoin_Bevel          = 0x0010,
        LineJoin_Miter          = 0x0020,
        LineJoin_Mask           = 0x0030,

        // Line start caps
        LineCap_Round           = 0x0000,
        LineCap_None            = 0x0040,
        LineCap_Square          = 0x0080,
        LineCap_Mask            = 0x00C0,

        // Line end caps (internal use only)
        LineEndCap_Round        = 0x0000,
        LineEndCap_None         = 0x0100,
        LineEndCap_Square       = 0x0200,
        LineEndCap_Mask         = 0x0300,
        LineEndCap_ShiftToCap   = 2
    };

    void SetWidth(UInt16 w) { Width = w; }
    void SetColor(GColor c) { Color = c; }
    void SetComplexFill(const GFxFillStyle& f);
    GFxFillStyle* CreateComplexFill();

#ifndef GFC_NO_FXPLAYER_STROKER
    void SetJoin(LineStyle s) 
    { 
        StyleFlags = UInt16((StyleFlags & ~LineJoin_Mask) | (s & LineJoin_Mask));
    }

    void SetStartCap(LineStyle s) 
    {
        StyleFlags = UInt16((StyleFlags & ~LineCap_Mask) | (s & LineCap_Mask)); 
    }

    void SetEndCap(LineStyle s) 
    {
        StyleFlags = UInt16((StyleFlags & ~LineEndCap_Mask) | 
                           ((s << LineEndCap_ShiftToCap) & LineEndCap_Mask));
    }

    void SetCaps(LineStyle s) 
    { 
        SetStartCap(s);
        SetEndCap(s);
    }
    
    void SetScaling(LineStyle s)
    {
        StyleFlags = UInt16((StyleFlags & ~LineScaling_Mask) | (s & LineScaling_Mask)); 
    }

    void SetHinting(bool hinting)
    {
        StyleFlags &= ~LineFlag_StrokeHinting;
        if(hinting) StyleFlags |= LineFlag_StrokeHinting;
    }

    void SetMiterSize(Float s) { MiterSize = s; }
#endif



    UInt16          GetWidth() const        { return Width; }
    GColor          GetColor() const        { return Color; }
    bool            HasComplexFill() const  { return pComplexFill != 0; }
    GFxFillStyle*   GetComplexFill() const  { return pComplexFill; }    


#ifndef GFC_NO_FXPLAYER_STROKER
    UInt16          GetStyle() const        { return StyleFlags; }
    LineStyle       GetStartCap() const     { return (LineStyle)(StyleFlags & LineCap_Mask); }
    LineStyle       GetEndCap() const       { return (LineStyle)((StyleFlags & LineEndCap_Mask) >> LineEndCap_ShiftToCap); }
    LineStyle       GetJoin() const         { return (LineStyle)(StyleFlags & LineJoin_Mask);       }
    LineStyle       GetScaling() const      { return (LineStyle)(StyleFlags & LineScaling_Mask);    }
    bool            GetStrokeHinting()      { return (StyleFlags & LineFlag_StrokeHinting) != 0;    }

    Float           GetMiterSize() const    { return MiterSize; }
    
    // Compute stroke extent as an floating point value. 
    // Usually half width, but can be extended by miter, which is a multiple of half width.
    Float           GetStrokeExtent() const { return (1.0f/10.0f) * MiterSize * Width; }

#else
    // Stroke extent == half width.
    Float           GetStrokeExtent() const { return (1.0f/10.0f) * Width; }
#endif

};


#endif // INC_GFXSTYLES_H
