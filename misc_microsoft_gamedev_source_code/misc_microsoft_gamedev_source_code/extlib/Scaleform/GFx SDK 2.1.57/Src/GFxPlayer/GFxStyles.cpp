/**********************************************************************

Filename    :   GFxStyles.cpp
Content     :   
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GRenderer.h"
#include "GImage.h"

#include "GFxStyles.h"
#include "GFxLog.h"
#include "GFxStream.h"
#include "GFxFontResource.h"

#include "GFxLoaderImpl.h"
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"

#include "GFxImageResource.h"

#include "GFxLoadProcess.h"

#include "GFxDisplayContext.h"


#ifdef GFC_ASSERT_ON_GRADIENT_BITMAP_GEN
#define GASSERT_ON_GRADIENT_BITMAP_GEN GASSERT(0)
#else
#define GASSERT_ON_GRADIENT_BITMAP_GEN ((void)0)
#endif //GFC_ASSERT_ON_GRADIENT_BITMAP_GEN



//
// *** GFxGradientRecord
//

GFxGradientRecord::GFxGradientRecord()
    : Ratio(0)
{
}

void    GFxGradientRecord::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    Ratio = p->ReadU8();
    p->ReadRgbaTag(&Color, tagType);
}


//
// *** GFxGradientData
//

GFxGradientData::GFxGradientData(GFxFillType type, UInt16 recordCount, bool linearRgb)
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
    GASSERT((type < 256) && (type & GFxFill_Gradient_TestBit));
    Type        = (UByte) type;

    pRecords    = 0;
    RecordCount = 0;
    SetRecordCount(recordCount);

    LinearRGB = linearRgb;
    FocalRatio = 0.0f;
}

GFxGradientData::~GFxGradientData()
{
    if (pRecords)
        GFREE(pRecords);
}

bool    GFxGradientData::SetRecordCount(UInt16 count)
{
    if (count == RecordCount)
        return 1;

    GFxGradientRecord* pnewRecords = 0;    
    if (count)
        if ((pnewRecords = (GFxGradientRecord*)GALLOC(sizeof(GFxGradientRecord)*count)) == 0)
            return 0;

    if (pRecords)
    {
        UInt copyCount = GTL::gmin(count, RecordCount);
        for (UInt i=0; i<copyCount; i++)
            pnewRecords[i] = pRecords[i];
        GFREE(pRecords);
    }

    pRecords    = pnewRecords;
    RecordCount = count;
    return 1;
}

size_t    GFxGradientData::GetHashCode() const
{
    UInt    i;
    size_t  hashCode = Type;
    for (i = 0; i< RecordCount; i++)
        hashCode ^= pRecords[i].GetHashCode();
    return hashCode;
}

bool    GFxGradientData::operator == (const GFxGradientData& other) const
{
    if ((RecordCount != other.RecordCount) ||
        (Type != other.Type) ||
        (FocalRatio != other.FocalRatio) ||
        (LinearRGB != other.LinearRGB))
        return 0;
    
    UInt i;
    for (i=0; i< RecordCount; i++)    
        if (pRecords[i] != other.pRecords[i])
            return 0;    
    return 1;
}

// Creates an image resource from gradient.
GFxImageResource*   GFxGradientData::CreateImageResource(
                        GFxResourceWeakLib *plib,
                        GFxGradientParams *pparams,
                        GFxImageCreator *pimageCreator,
                        GFxRenderConfig* prconfig,
                        bool threadedLoading)
{
    // Create a key and check it against library.
    GFxResourceKey      gradientKey = CreateGradientKey();
    GFxImageResource *  pres        = 0;

    GFxResourceLib::BindHandle bh;
    if (plib->BindResourceKey(&bh, gradientKey) == GFxResourceLib::RS_NeedsResolve)
    {
        // If resource does not exist, create it and resolve it in library.
        GPtr<GImage> pimage = *CreateGradientImage(pparams);
        if (pimage)
        {
            GFxImageCreateInfo   icreateInfo(pimage, GFxResource::Use_Gradient, prconfig);
            icreateInfo.ThreadedLoading = threadedLoading;
            GPtr<GImageInfoBase> pimageInfo;
            
            if (pimageCreator)
                pimageInfo = *pimageCreator->CreateImage(icreateInfo);
            else
            {
                GFC_DEBUG_WARNING(1, "GFxGradientData::CreateImageResource failed - GFxImageCreator not installed");
            }

            // Add bitmap resource, if image was created; otherwise, it will
            // be added by loader.
            pres = new GFxImageResource(pimageInfo, gradientKey,
                                        GFxResource::Use_Gradient);
            if (pres)
                bh.ResolveResource(pres);
        }

        if (!pres)
            bh.CancelResolve("Failed to create gradient");
    }
    else
    {
        // WaitForResolve AddRefs, so we are ok to return result.
        pres = (GFxImageResource*) bh.WaitForResolve();
    }

    return pres;
}



class GFxGradientImageResourceKey : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;

    virtual void    AddRef(KeyHandle hdata)
    {
        GASSERT(hdata); ((GFxGradientData*) hdata)->AddRef();
    }
    virtual void    Release(KeyHandle hdata)
    {        
        GASSERT(hdata); ((GFxGradientData*) hdata)->Release();
    }

    // Key/Hash code implementation.
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const
    {
        GUNUSED(hdata);
        return GFxResourceKey::Key_Gradient;
    }

    virtual size_t  GetHashCode(KeyHandle hdata) const
    {
        GASSERT(hdata);        
        return ((GFxGradientData*) hdata)->GetHashCode();
    }

    virtual bool    KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
    {
        if (this != other.GetKeyInterface())
            return 0;        
        return *((GFxGradientData*) hdata) == *((GFxGradientData*) other.GetKeyData());
    }
};


class GFxGradientImageResourceCreator : public GFxResourceData::DataInterface
{
    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(GFxResourceData::DataHandle hdata, GFxResourceBindData *pbindData,
                                   GFxLoadStates *pls) const
    {
        GFxGradientData *pgd = (GFxGradientData*) hdata;
        GASSERT(pgd);        
        pbindData->pResource = *pgd->CreateImageResource(pls->GetLib(),
                                                         pls->GetBindStates()->pGradientParams,
                                                         pls->GetBindStates()->pImageCreator,
                                                         pls->GetRenderConfig(), pls->IsThreadedLoading());
        return pbindData->pResource ? 1 : 0;
    }
};



static GFxGradientImageResourceCreator static_inst_data;
static GFxGradientImageResourceKey     static_inst_key;

// Creates a GFxResourceData object from this GFxGradientData;
// this object can be used to create a resource.
GFxResourceData GFxGradientData::CreateGradientResourceData()
{
    return GFxResourceData(&static_inst_data, this);
}

GFxResourceKey  GFxGradientData::CreateGradientKey()
{
    return GFxResourceKey(&static_inst_key, this);
}



//=========================================================================
// The code of this class was taken from the Anti-Grain Geometry
// Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corporation by the author of Anti-Grain Geometry Project.
//=========================================================================
// This class is called G..., not GFx... because it's independent and
// actually belongs to the renderer. 
//=========================================================================
class GFocalRadialGradient
{
public:
    //---------------------------------------------------------------------
    GFocalRadialGradient() : 
        Radius(100),
        FocusX(0), 
        FocusY(0)
    {
        updateValues();
    }

    //---------------------------------------------------------------------
    GFocalRadialGradient(Float r, Float fx, Float fy) : 
        Radius(r), 
        FocusX(fx), 
        FocusY(fy)
    {
        updateValues();
    }

    //---------------------------------------------------------------------
    void Init(Float r, Float fx, Float fy)
    {
        Radius = r;
        FocusX = fx;
        FocusY = fy;
        updateValues();
    }

    Float Calculate(Float x, Float y) const;

private:
    void updateValues();

    Float Radius;
    Float FocusX;
    Float FocusY;
    Float Radius2;
    Float Multiplier;
};



//---------------------------------------------------------------------
Float GFocalRadialGradient::Calculate(Float x, Float y) const
{
    Float dx = x - FocusX;
    Float dy = y - FocusY;
    Float d2 = dx * FocusY - dy * FocusX;
    Float d3 = Radius2 * (dx * dx + dy * dy) - d2 * d2;
    return (dx * FocusX + dy * FocusY + sqrtf(fabsf(d3))) * Multiplier;
}

//---------------------------------------------------------------------
void GFocalRadialGradient::updateValues()
{
    // Calculate the invariant values. In case the focal center
    // lies exactly on the gradient circle the divisor degenerates
    // into zero. In this case we just move the focal center by
    // one subpixel unit possibly in the direction to the origin (0,0)
    // and calculate the values again.
    //-------------------------
    Float fx2 = FocusX * FocusX;
    Float fy2 = FocusY * FocusY;
    Radius2   = Radius * Radius;
    Float d   = Radius2 - (fx2 + fy2);
    if(d == 0)
    {
        if(FocusX != 0) { if(FocusX < 0) FocusX += 1; else FocusX -= 1; }
        if(FocusY != 0) { if(FocusY < 0) FocusY += 1; else FocusY -= 1; }
        fx2 = FocusX * FocusX;
        fy2 = FocusY * FocusY;
        d   = Radius2 - (fx2 + fy2);
    }
    Multiplier = Radius / d;
}


//---------------------------------------------------------------------
class GFxGradientRamp
{
public:
    enum { RampSize = 256 };

    GFxGradientRamp() {}
    GFxGradientRamp(const GFxGradientRecord* colorStops, 
                    unsigned numColors, 
                    Float gamma=1)
    {
        Init(colorStops, numColors, gamma);
    }

    void Init(const GFxGradientRecord* colorStops, 
              unsigned numColors, 
              Float gamma=1);

    const GColor& GetColor(unsigned i) const 
    { 
        return Ramp[(i < RampSize) ? i : RampSize-1]; 
    }

private:
    struct ColorType
    {
        UInt16 r,g,b,a;
        ColorType() {}
        ColorType(UInt16 r_, UInt16 g_, UInt16 b_, UInt16 a_) :
            r(r_), g(g_), b(b_), a(a_) {}
    };

    static ColorType applyGamma(const GColor& c, const UInt16* gammaLut)
    {
        int a = c.GetAlpha();
        return ColorType(gammaLut[c.GetRed()  ], 
                         gammaLut[c.GetGreen()], 
                         gammaLut[c.GetBlue() ],
                         UInt16((a << 8) | a));
    }

    static UByte applyGamma(UInt16 v, Float gamma)
    {
        return (UByte)floor(pow(v / 65535.0f, gamma) * 255.0f + 0.5f);
    }

    static GColor blendColors(const ColorType& c1, const ColorType& c2, 
                              int ratio, int maxRatio,
                              Float gamma)
    {
        int alphaDiv = (maxRatio << 8) | maxRatio;
        return GColor(applyGamma(UInt16(c1.r + int(c2.r - c1.r) * ratio / maxRatio), gamma), 
                      applyGamma(UInt16(c1.g + int(c2.g - c1.g) * ratio / maxRatio), gamma), 
                      applyGamma(UInt16(c1.b + int(c2.b - c1.b) * ratio / maxRatio), gamma),
                           UByte(c1.a + int(c2.a - c1.a) * ratio / alphaDiv));
    }

    static GColor blendColors(const ColorType& c1, const ColorType& c2, 
                              int ratio, int maxRatio)
    {
        maxRatio = (maxRatio << 8) | maxRatio;
        return GColor(UByte(c1.r + int(c2.r - c1.r) * ratio / maxRatio), 
                      UByte(c1.g + int(c2.g - c1.g) * ratio / maxRatio), 
                      UByte(c1.b + int(c2.b - c1.b) * ratio / maxRatio),
                      UByte(c1.a + int(c2.a - c1.a) * ratio / maxRatio));
    }

    GColor Ramp[RampSize];
};


//---------------------------------------------------------------------
void GFxGradientRamp::Init(const GFxGradientRecord* colorStops, 
                           unsigned numColors, 
                           Float gamma)
{
    unsigned i;
    if (numColors == 0 || colorStops == 0)
    {
        GFxGradientRecord fakeColor;
        fakeColor.Ratio = 0;
        fakeColor.Color = 0xFF000000;
        numColors  = 1;
        colorStops = &fakeColor;
    }

    if(numColors > 1)
    {
        UInt16 gammaDir[256];
        Float  gammaInv = 1;

        if(gamma == 1)
        {
            for(i = 0; i < 256; i++)
            {
                gammaDir[i] = UInt16( (i << 8) | i );
            }
        }
        else
        {
            // It's a simplified calculation of the linear color gradient LUT
            // Replace this LUT with sRGB if necessary
            for(i = 0; i < 256; i++)
            {
                gammaDir[i] = (UInt16)floor(pow(i / 255.0f, gamma) * 65535.0f + 0.5f);
            }
            gammaInv = 1/gamma;
        }

        unsigned start  = colorStops[0].Ratio;
        unsigned end    = 0;
        GColor c = colorStops[0].Color;
        for(i = 0; i < start; i++) 
        {
            Ramp[i] = c;
        }
        for(i = 1; i < numColors; i++)
        {
            end = colorStops[i].Ratio;
            if(end < start) end = start;

            ColorType c1 = applyGamma(colorStops[i-1].Color, gammaDir);
            ColorType c2 = applyGamma(colorStops[i].Color,   gammaDir);

            unsigned x0 = start;
            unsigned dx = end - start;
            if(gamma == 1)
            {
                while(start < end)
                {
                    c = Ramp[start] = blendColors(c1, c2, start-x0+1, dx);
                    ++start;
                }
            }
            else
            {
                while(start < end)
                {
                    c = Ramp[start] = blendColors(c1, c2, start-x0+1, dx, gammaInv);
                    ++start;
                }
            }
        }

        for(; end < RampSize; end++)
        {
            Ramp[end] = c;
        }
    }
    else
    {
        for(i = 0; i < RampSize; i++)
        {
            Ramp[i] = colorStops[0].Color;
        }
    }
}

#ifdef GFX_ADAPTIVE_GRADIENT_SIZE
const UInt GFxGradientData_RadialGradientSizeTable[] = 
{
    16, 16, 32, 32, 64, 64, 64, 128, 128, 128, 128, 128, 128, 256, 256, 256, 256, 256
};
#endif


UInt GFxGradientData::ComputeRadialGradientImageSize(GFxGradientParams *pparams) const
{   
    UInt radialGradientImageSize = 0;

    if (pparams && !pparams->IsAdaptive())
    {
        radialGradientImageSize = pparams->GetRadialImageSize() ?
                                    pparams->GetRadialImageSize() :
                                    GFX_GRADIENT_SIZE_DEFAULT;
    }

#ifdef GFX_ADAPTIVE_GRADIENT_SIZE
    if (radialGradientImageSize == 0)
    {
        // Adaptive gradient size. The function heuristically
        // computes the optimal gradient size depending on the 
        // maximal slope in the color ramp, focal point, and
        // linearRGB flag. See also RadialGradientSizeTable that may
        // be modified to improve the result. This table and all other 
        // coefficients are defined empirically, on the basis of 
        // the visual result in typical cases. It's a kind of magic...
        //----------------------------------
        UInt i;
        Float maxSlope = 0;
        for (i = 1; i < GetRecordCount(); ++i)
        {
            Float slope;
            const GFxGradientRecord& g1 = pRecords[i - 1];
            const GFxGradientRecord& g2 = pRecords[i];
            Float dx = (Float)g2.Ratio - (Float)g1.Ratio;
            if (dx > 0)
            {
                slope = abs((int)g1.Color.GetRed()   - (int)g2.Color.GetRed())   / dx;
                if (slope > maxSlope) maxSlope = slope;
                slope = abs((int)g1.Color.GetGreen() - (int)g2.Color.GetGreen()) / dx;
                if (slope > maxSlope) maxSlope = slope;
                slope = abs((int)g1.Color.GetBlue()  - (int)g2.Color.GetBlue())  / dx;
                if (slope > maxSlope) maxSlope = slope;
                slope = abs((int)g1.Color.GetAlpha() - (int)g2.Color.GetAlpha()) / dx;
                if (slope > maxSlope) maxSlope = slope;
            }
        }

        if (maxSlope == 0)
            return GFX_GRADIENT_SIZE_DEFAULT;

        if (LinearRGB) 
            maxSlope *= 1.5f;

        if (Type == GFxFill_FocalPointGradient)
        {
            Float r = fabsf(FocalRatio);
            if (r > 0.5f)
                maxSlope /= 1.01f - r;
        }

        if (maxSlope < 0)
            maxSlope = 0;

        maxSlope = sqrtf((maxSlope + 0.18f) * 5);
        UInt idx = (UInt)maxSlope;
        if (idx >= sizeof(GFxGradientData_RadialGradientSizeTable) / sizeof(UInt)) 
            idx  = sizeof(GFxGradientData_RadialGradientSizeTable) / sizeof(UInt) - 1;
        return GFxGradientData_RadialGradientSizeTable[idx];
    }
    else
        return radialGradientImageSize;
#else
    return radialGradientImageSize ? radialGradientImageSize : GFX_GRADIENT_SIZE_DEFAULT;
#endif // GFX_ADAPTIVE_GRADIENT_SIZE
}


// Make a GTexture* corresponding to our gradient.
// We can use this to set the gradient fill style.
GImage* GFxGradientData::CreateGradientImage(GFxGradientParams *pparams) const
{
#ifndef GFC_NO_GRADIENT_GEN
    GASSERT(Type == GFxFill_LinearGradient ||
            Type == GFxFill_RadialGradient ||
            Type == GFxFill_FocalPointGradient);
    GASSERT_ON_GRADIENT_BITMAP_GEN;

    GImage *pim = 0;

    UInt i, j, size;
    int ratio;
    Float center, radius;
    GColor sample;
    UByte r, g, b, a;
    Float x, y;
    UInt imgSize;

    // Gamma=2.17 is the closest approximation of the sRGB curve.
    //--------------------
    GFxGradientRamp ramp(pRecords, RecordCount, LinearRGB ? 2.17f : 1.0f);

    switch(Type)
    {
    case GFxFill_LinearGradient: // Linear gradient.
        if ((pim = GImage::CreateImage(GImage::Image_ARGB_8888, 256, 1))!=0)
        {
            for (i = 0; i < pim->Width; i++)
            {
                sample = ramp.GetColor(i);
                pim->SetPixelRGBA(i, 0, 
                                  sample.GetRed(), 
                                  sample.GetGreen(), 
                                  sample.GetBlue(), 
                                  sample.GetAlpha());
            }
        }           
        break;

    case GFxFill_RadialGradient: // Radial gradient.
        imgSize = ComputeRadialGradientImageSize(pparams);
        if ((pim = GImage::CreateImage(GImage::Image_ARGB_8888, imgSize, imgSize))!=0)
        {
            center = pim->Height / 2.0f;
            radius = center - 1;
            size   = imgSize - 1;

            for (j = 1; j < size; j++)
            {
                for (i = 1; i < size; i++)
                {                       
                    y = j - center + 0.5f;
                    x = i - center + 0.5f;
                    ratio = (int)floorf((float)sqrt(x * x + y * y) * 256 / radius + 0.5f);
                    if (ratio > 255)
                    {
                        ratio = 255;
                    }
                    sample = ramp.GetColor(ratio);
                    pim->SetPixelRGBA(i, j, 
                                      sample.GetRed(), 
                                      sample.GetGreen(), 
                                      sample.GetBlue(), 
                                      sample.GetAlpha());
                }
            }
            sample = ramp.GetColor(255);
            r = sample.GetRed();
            g = sample.GetGreen();
            b = sample.GetBlue();
            a = sample.GetAlpha();
            for(i = 1; i < imgSize; i++)
            {
                pim->SetPixelRGBA(i, 0,                          r, g, b, a);
                pim->SetPixelRGBA(pim->Width-1, i,               r, g, b, a);
                pim->SetPixelRGBA(pim->Width-i-1, pim->Height-1, r, g, b, a);
                pim->SetPixelRGBA(0, pim->Height-i-1,            r, g, b, a);
            }
        }
        break;

    case GFxFill_FocalPointGradient: // Radial gradient with focal point.
        imgSize = ComputeRadialGradientImageSize(pparams);
        if ((pim = GImage::CreateImage(GImage::Image_ARGB_8888, imgSize, imgSize))!=0)
        {
            center = pim->Height / 2.0f;
            radius = center - 1;
            size   = imgSize - 1;

            GFocalRadialGradient gr(radius, FocalRatio * radius, 0);

            for (j = 1; j < size; j++)
            {
                for (i = 1; i < size; i++)
                {
                    y = j - center + 0.5f;
                    x = i - center + 0.5f;
                    ratio = (int)floorf(gr.Calculate(x, y) * 256 / radius + 0.5f);
                    if (ratio > 255)
                    {
                        ratio = 255;
                    }
                    sample = ramp.GetColor(ratio);
                    pim->SetPixelRGBA(i, j, 
                                      sample.GetRed(), 
                                      sample.GetGreen(), 
                                      sample.GetBlue(), 
                                      sample.GetAlpha());
                }
            }
            sample = ramp.GetColor(255);
            r = sample.GetRed();
            g = sample.GetGreen();
            b = sample.GetBlue();
            a = sample.GetAlpha();
            for(i = 1; i < imgSize; i++)
            {
                pim->SetPixelRGBA(i, 0,                          r, g, b, a);
                pim->SetPixelRGBA(pim->Width-1, i,               r, g, b, a);
                pim->SetPixelRGBA(pim->Width-i-1, pim->Height-1, r, g, b, a);
                pim->SetPixelRGBA(0, pim->Height-i-1,            r, g, b, a);
            }
        }
        break;
    }       
    return pim;
#else //GFC_NO_GRADIENT_GEN
    return 0;
#endif //GFC_NO_GRADIENT_GEN
}




//
// *** GFxFillStyle
//


GFxFillStyle::GFxFillStyle()
{
    Type        = GFxFill_Solid;
    FillFlags   = FF_HasAlpha;
    pGradientData = 0;
}

GFxFillStyle::GFxFillStyle(const GFxFillStyle& other)
{
    Type = GFxFill_Solid;
    // We need to take care of gradient assignment, etc;   
    // so just call operator ==.
    *this = other;
}

GFxFillStyle::~GFxFillStyle()
{
    if (IsGradientFill())
    {
        if (pGradientData)
            pGradientData->Release();
    }
}


void    GFxFillStyle::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    Type        = p->ReadU8();
    FillFlags   = 0;

    p->LogParse("  FillStyle read type = 0x%X\n", Type);

    if (Type == GFxFill_Solid)
    {
        // Read solid fill color.
        GColor color;
        p->ReadRgbaTag(&color, tagType);
        
        p->LogParse("  color: ");
        p->GetStream()->LogParseClass(color);

        if (color.GetAlpha() < 255)
            FillFlags |= FF_HasAlpha;

        RawColor = color.Raw;
    }

    else if (IsGradientFill())
    {
        // 0x10: linear gradient fill
        // 0x12: radial gradient fill
        // 0x13: focal-point gradient fill for SWF 8
        
        p->GetStream()->ReadMatrix(&ImageMatrix);

        // GRADIENT
        UInt16 numGradients = p->ReadU8();

        // For DefineShape5 gradient records are interpreted differently.
        // Note that Flash 8 can include these extra bits even for old tag types.
        if (numGradients & 0x10)
            FillFlags |= FF_LinearRGB;
        // TBD: Other bits may be set here, such as 0x80 for focal point
        // Do they need custom processing?
        numGradients &= 0xF;
        
        // SWF 8 allows up to 15 points.
        GASSERT(numGradients >= 1 && numGradients <= 15);
        // Allocate gradients.
        pGradientData = new GFxGradientData((GFxFillType)Type, numGradients, IsLinearRGB());
        if (!pGradientData ||
            pGradientData->GetRecordCount() != numGradients)
        {
            // Memory error...
            if (pGradientData)
                pGradientData->Release();
            RawColor = 0;
            Type = GFxFill_Solid;
            return;
        }
                
        for (int i = 0; i < numGradients; i++)
        {
            (*pGradientData)[i].Read(p, tagType);
            // Flag if the gradient has alpha channel.
            if ((*pGradientData)[i].Color.GetAlpha() < 255)
                FillFlags |= FF_HasAlpha;
        }

        p->LogParse("  gradients: numGradients = %d\n", numGradients);

        // Record focal point.          
        if (Type == GFxFill_FocalPointGradient)           
            pGradientData->SetFocalRatio((float) (p->ReadS16()) / 256.0f);


        // The image gradient must be replaced by gradient data,
        // which controls how it will be generated during binding.

        // 1. Get gradient id.
        
        GFxResourceId charId = p->GetNextGradientId();


        // If the gradient corresponds to an image in a file,
        // cache it. Otherwise get the corresponding handle

        const GFxExporterInfo* pexporterInfo = p->GetExporterInfo();
        bool imageFromFile =  pexporterInfo && 
                              (pexporterInfo->ExportFlags & GFxExporterInfo::EXF_GradientTexturesExported);

        if (imageFromFile)
        {
            // We do not have to use bitmap lookup table any longer since
            // gradient file loading is matched through GFxResourceLib.                              
            GFxResourceHandle rh;
            if (p->GetResourceHandle(&rh, charId))            
                pImage.SetFromHandle(rh);            
        }
        else
        {
            // 2. Create an image handle for gradient
            GFxResourceData     resData = pGradientData->CreateGradientResourceData();
            GFxResourceHandle   rh      = p->AddDataResource(charId, resData);

            if (!rh.IsNull())
                pImage.SetFromHandle(rh);
        }

// for 'planeta.swf' loading
//        GASSERT(!(pImage.IsIndex() && pImage.GetBindIndex() == 1));


        // We used to pre-compute parts of image matrices here, so that inversion 
        // did not need to happen in GetTexture. However, two reasons caused this
        // logic to be moved to rendering side:
        //   - the original matrix has to be lerped for correct gradient morph results
        //   - with ResourceLibn we don't know image sizes are they are computed later
        //     based on GFxGradientParams, so those sizes can not be factored into the
        //     image matrix.
    }

    else if (IsImageFill())
    {
        // 0x40: tiled bitmap fill
        // 0x41: clipped bitmap fill
        // 0x42: non-smoothed tiled fill
        // 0x43: non-smoothed clipped bitmap fill

        GFxResourceId bitmapResourceId = GFxResourceId(p->ReadU16());
        p->LogParse("  BitmapChar = %d\n", bitmapResourceId.GetIdIndex());

        //      
        if (!p->GetResourceHandle(&pImage, bitmapResourceId))
        {
            // Should not happen..
            // New image ??
        }


        // Look up the bitmap character.
        /*
        pBitmap = md->GetBitmapResource(bitmapResourceId);
        if (!pBitmap && bitmapResourceId != 0xFFFF)
        {
            // probably, this bitmap will be imported later.
            // we need to create an empty bitmap resource that will be
            // filled in the ImportResolve.
            pBitmap = *new GFxBitmapResource(0);
            md->AddBitmapResource(bitmapResourceId, pBitmap);
        }
        */
        
        p->GetStream()->ReadMatrix(&ImageMatrix);
        
        // Always 1 for now, should probably check formats in the future.
        FillFlags |= FF_HasAlpha;        
        p->GetStream()->LogParseClass(ImageMatrix);
    }
}



// Push our style parameters into the renderer.
void    GFxFillStyle::Apply(const GFxDisplayContext& dc,
                            Float scaleMultiplier,
                            const GRenderer::Matrix* s9gImgAdjust) const
{   
    GRenderer *prenderer = dc.GetRenderer();

    if (Type == GFxFill_Solid)
    {
        // 0x00: solid fill
        prenderer->FillStyleColor(GColor(RawColor));
    }
    else
    {
        GRenderer::FillTexture ft;
        ft.pTexture = 0;
        
        // Get the texture for gradient/bitmap fill and use it.
        GetFillTexture(&ft, dc, scaleMultiplier, s9gImgAdjust);
        if (ft.pTexture)        
            prenderer->FillStyleBitmap(&ft);

    }
}

// Creates a FillTexture for a style
bool    GFxFillStyle::GetFillTexture(GRenderer::FillTexture *pftexture,
                                     const GFxDisplayContext& dc,
                                     Float scaleMultiplier,
                                     const GRenderer::Matrix* s9gImgAdjust) const
{
    GRenderer *prenderer = dc.GetRenderer();
    GRenderer::Matrix imgMtx;
    GASSERT(prenderer);
    GASSERT(dc.pResourceBinding);

    if (IsGradientFill())
    {
        // Type == GFxFill_LinearGradient ||
        // Type == GFxFill_RadialGradient ||
        // Type == GFxFill_FocalPointGradient (SWF 8)
        
        GFxImageResource* pimageRes = (*dc.pResourceBinding)[pImage];
        GASSERT(!pimageRes || (pimageRes->GetResourceType() == GFxResource::RT_Image));
        
        if (!pimageRes)
        {
            // If our gradient image IS an index and is NOT valid it means that 
            // it is being rendered before being properly resolved at bind time.
            if (pImage.IsIndex())
            {
                GASSERT(0);
                return 0;
            }

            // We can get here when morphing gradient styles.
            // TBD: May need to do locking here!

            pimageRes = pGradientData->CreateImageResource(
                            dc.pWeakLib, dc.pGradientParams, dc.pImageCreator,
                            dc.GetRenderConfig(),false);

            if (pimageRes)
            {
                // TBD: Bad because we shouldn't we storing temporary
                //      instance - related data in fill style.
                // Threading - the fact that we can only display in one thread
                // may help us.
                GFxFillStyle*   thisNonConst = const_cast<GFxFillStyle*>(this);
                thisNonConst->pImage = pimageRes;

                // Release the extra gradient reference created above.
                pimageRes->Release();
            }            
        }

        if (pimageRes)
        {
            GImageInfoBase* pimageInfo =  pimageRes->GetImageInfo();

            if (pimageInfo)
            {   
                pftexture->pTexture         = pimageInfo->GetTexture(prenderer);
                pftexture->WrapMode         = GRenderer::Wrap_Clamp;
                pftexture->SampleMode       = GRenderer::Sample_Linear;

                pftexture->TextureMatrix.SetIdentity();

                if (Type == GFxFill_LinearGradient)
                {
                    pftexture->TextureMatrix.AppendScaling(1.0f / 128.0f);
                    pftexture->TextureMatrix.AppendTranslation(128.F, 0.F);
                }
                else
                {
                    // TBD: Some of these matrix computations could have been pre-computed;
                    // however, that is hard to do since we don't know the image size at load time.
                    // Furthermore, morphing requires original matrices to be used.
                    UInt  radialGradientImageSize = pimageInfo->GetWidth();
                    Float halfSize                = Float(radialGradientImageSize)/2;
                
                    pftexture->TextureMatrix.AppendScaling(1.0f / (32768/(radialGradientImageSize-2)));
                    pftexture->TextureMatrix.AppendTranslation(halfSize, halfSize);
                    // gradient's square size is 32768x32768 (-16,384 - 16,384)
                }

                // When using scale9grid with gradients or images, 
                // the image matrix has to be adjusted according to the 
                // "logical difference" between the world matrix and the 
                // central part of the scale9grid. The s9gImgAdjust (if not null)
                // represents this adjustment matrix.
                imgMtx = ImageMatrix;
                if (s9gImgAdjust)
                    imgMtx.Append(*s9gImgAdjust);

                pftexture->TextureMatrix.Prepend(imgMtx.GetInverse());
                pftexture->TextureMatrix.PrependScaling(scaleMultiplier);
                
                return 1;
            }
        }
    }

    else if (IsImageFill())
    {
        // Bitmap Fill (either tiled or clipped)
        GFxImageResource* pimageRes     = (*dc.pResourceBinding)[pImage];
        GImageInfoBase*   pimageInfo    = pimageRes ? pimageRes->GetImageInfo() : 0;

        if (pimageInfo)
        {
            GTexture* ptexture = pimageInfo->GetTexture(prenderer);
            if (ptexture)
            {
                GRenderer::BitmapWrapMode   wm = GRenderer::Wrap_Repeat;
                GRenderer::BitmapSampleMode sm = GRenderer::Sample_Linear;

                switch(Type)
                {
                case GFxFill_TiledSmoothImage:     wm = GRenderer::Wrap_Repeat;    sm = GRenderer::Sample_Linear; break;
                case GFxFill_ClippedSmoothImage:   wm = GRenderer::Wrap_Clamp;     sm = GRenderer::Sample_Linear; break;
                case GFxFill_TiledImage:           wm = GRenderer::Wrap_Repeat;    sm = GRenderer::Sample_Point; break;
                case GFxFill_ClippedImage:         wm = GRenderer::Wrap_Clamp;     sm = GRenderer::Sample_Point; break;
                }

                // When using scale9grid with gradients or images, 
                // the image matrix has to be adjusted according to the 
                // "logical difference" between the world matrix and the 
                // central part of the scale9grid. The s9gImgAdjust (if not null)
                // represents this adjustment matrix.
                imgMtx = ImageMatrix;
                if (s9gImgAdjust)
                    imgMtx.Append(*s9gImgAdjust);

                pftexture->pTexture  = ptexture;
                pftexture->TextureMatrix.SetInverse(imgMtx);
                pftexture->TextureMatrix.PrependScaling(scaleMultiplier);
                pftexture->WrapMode  = wm;
                pftexture->SampleMode= sm;
                return 1;
            }
        }
    }

    return 0;
}



// Sets this style to a blend of a and b.  t = [0,1]
void    GFxFillStyle::SetLerp(const GFxFillStyle& a, const GFxFillStyle& b, Float t)
{
    GASSERT(t >= 0 && t <= 1);

    // fill style type
    Type = (UByte)a.GetType();
    GASSERT(Type == b.GetType());


    if (Type == GFxFill_Solid)
    {
        // Fill style color
        RawColor = GColor::Blend(a.GetColor(), b.GetColor(), t).Raw;
        return;
    }

    else if (IsGradientFill())
    {
        GASSERT(a.pGradientData && b.pGradientData);
        GASSERT(a.pGradientData->GetRecordCount() == b.pGradientData->GetRecordCount());

        GFxGradientData &agd = *a.pGradientData,
                        &bgd = *b.pGradientData;

        // We have to create a new GradientData object here because
        // gradient data objects are used as keys when generating gradient 
        // image resources and thus can not be modified!!
        if (pGradientData)
            pGradientData->Release();
        pGradientData = new GFxGradientData(a.GetType(),
                                            agd.GetRecordCount(),
                                            a.IsLinearRGB());
        if (pGradientData)
        {
            for (UInt j=0; j < pGradientData->GetRecordCount(); j++)
            {                
                (*pGradientData)[j].Ratio =
                    (UByte) gfrnd(gflerp(agd[j].Ratio, bgd[j].Ratio, t));
                (*pGradientData)[j].Color = GColor::Blend(agd[j].Color,
                                                          bgd[j].Color, t);
            }
        }

        pGradientData->SetFocalRatio(gflerp(agd.GetFocalRatio(),
                                            bgd.GetFocalRatio(), t));

        // Reset image handle ??? So that it is cached??
        // Need to test gradient morphing
        // NULL Handle -> uninitialized image pointer ??
        pImage = 0;

        // We will need to do this in thread safe manner
        // and coordinate with the library...
    }

    else if (IsImageFill())
    {
        pImage = a.pImage;
        GASSERT(pImage == b.pImage);
    }

    // fill style bitmap GRenderer::Matrix
    ImageMatrix.SetLerp(a.ImageMatrix, b.ImageMatrix, t);
}


GFxFillStyle& GFxFillStyle::operator = (const GFxFillStyle& src)
{
    // Implement correct ref-counting for pGradientData.
    if (src.IsGradientFill() && src.pGradientData)
        src.pGradientData->AddRef();
    if (IsGradientFill() && pGradientData)
        pGradientData->Release();

    Type            = src.Type;
    FillFlags       = src.FillFlags;

    pGradientData   = src.pGradientData;
    GCOMPILER_ASSERT(sizeof(GFxGradientData*) >= sizeof(RawColor));

    pImage          = src.pImage;
    ImageMatrix     = src.ImageMatrix;

    return *this;
}




void GFxFillStyle::SetFillType(GFxFillType type)
{
    if (IsGradientFill() && pGradientData)
        pGradientData->Release();    

    GASSERT(type <= 255);
    Type = (UByte)type; 
}


// Sets fill style to an image, used to support externally loaded images.
void    GFxFillStyle::SetImageFill(GFxFillType fillType,
                                   GFxImageResource *pimage,
                                   const GRenderer::Matrix& mtx)
{   
    SetFillType(fillType);  
    GASSERT(IsImageFill());

    /*
    if (bilinear)
    {
        if (tiled) Type = (UByte) GFxFill_TiledSmoothImage;
        else       Type = (UByte) GFxFill_ClippedSmoothImage;
    }
    else
    {
        if (tiled) Type = (UByte) GFxFill_TiledImage;
        else       Type = (UByte) GFxFill_ClippedImage;
    }
    */

    FillFlags   = FF_HasAlpha;
    ImageMatrix = mtx;
    ImageMatrix.AppendScaling(20);
    pImage      = pimage;
}

void    GFxFillStyle::SetGradientFill(GFxFillType fillType,
                                      GFxGradientData *pgradientData,
                                      const GRenderer::Matrix& mtx)
{
    if (!(IsGradientFill() && pgradientData == pGradientData))
    {
        SetFillType(fillType);
        GASSERT(pgradientData->GetFillType() == fillType);
        GASSERT(IsGradientFill());

        pGradientData = pgradientData;
        if (pGradientData)
            pGradientData->AddRef();
    }
    
    ImageMatrix = mtx;
    ImageMatrix.M_[0][2] = PixelsToTwips(ImageMatrix.M_[0][2]);
    ImageMatrix.M_[1][2] = PixelsToTwips(ImageMatrix.M_[1][2]);
}




//
// GFxLineStyle
//


GFxLineStyle::GFxLineStyle()        
{
    Width       = 0;
    pComplexFill= 0;
    StyleFlags  = 0;
#ifndef GFC_NO_FXPLAYER_STROKER
    MiterSize   = 1.0f; // Must be 1.0 for GetStrokeExtent() to work correctly.
#endif
}

GFxLineStyle::GFxLineStyle(const GFxLineStyle& other)
{
    pComplexFill = 0;
    *this = other;
}

GFxLineStyle::~GFxLineStyle()
{
    if (pComplexFill)
        delete pComplexFill;
}

// Line styles are stored in an array, so implement =
// for safety of data structures on assignment.
GFxLineStyle&   GFxLineStyle::operator = (const GFxLineStyle& src)
{
    Width       = src.Width;
    Color       = src.Color;

#ifndef GFC_NO_FXPLAYER_STROKER
    StyleFlags  = src.StyleFlags;
    MiterSize   = src.MiterSize;
#endif

    if (pComplexFill)
    {
        delete pComplexFill;
        pComplexFill = 0;
    }
    if (src.pComplexFill)
        pComplexFill = new GFxFillStyle(*src.pComplexFill);

    return *this;
}

void GFxLineStyle::SetComplexFill(const GFxFillStyle& f)
{
    if (pComplexFill)
        delete pComplexFill;
    pComplexFill = new GFxFillStyle(f);
}

GFxFillStyle* GFxLineStyle::CreateComplexFill()
{ 
    if (pComplexFill == 0)
        pComplexFill = new GFxFillStyle;
    return pComplexFill;
}

void    GFxLineStyle::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    Width = p->ReadU16();

    UInt16 styleFlags = 0;

    // Extra values come here for DefineShape5.
    if (tagType == GFxTag_DefineShape4)
    {
        styleFlags = p->ReadU16();
        StyleFlags = styleFlags;

        // Miter is followed by a special 16.16 value
        if (styleFlags & LineJoin_Miter)
        {
#ifndef GFC_NO_FXPLAYER_STROKER     
            MiterSize = (Float)(p->ReadU16()) / 256.0f;
#else 
            p->ReadU16();
#endif
        }
    }
    
    // If we have complex SWF 8 fill style, read it in.
    if (styleFlags & LineFlag_ComplexFill)
    {
        GFxFillStyle    fs, *pfill;         

        if ((pComplexFill = new GFxFillStyle)!=0)
            pfill = pComplexFill;
        else
            pfill = &fs;

        pfill->Read(p, tagType);

        // Avoid ASSERT in GetColor() for gradient strokes.
        if (pfill->IsSolidFill())
            Color = pfill->GetColor();
        else if (pfill->IsGradientFill())
        {
            GFxGradientData *pgdata = pfill->GetGradientData();
            if (pgdata && pgdata->GetRecordCount() > 0)
                Color = (*pgdata)[0].Color;
        }
    }
    else
    {
        // Usual case: solid color line.
        p->ReadRgbaTag(&Color, tagType);
    }
}


void    GFxLineStyle::Apply(GRenderer *prenderer) const
{   
    prenderer->LineStyleColor(Color);
    //prenderer->LineStyleWidth(Width);
}
