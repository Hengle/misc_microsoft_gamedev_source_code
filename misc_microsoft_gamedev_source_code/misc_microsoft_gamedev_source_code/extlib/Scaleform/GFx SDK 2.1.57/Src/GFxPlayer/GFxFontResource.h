/**********************************************************************

Filename    :   GFxFontResource.h
Content     :   Representation of SWF/GFX font data and resources.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxFontResource_H
#define INC_GFxFontResource_H

#include "GTLTypes.h"
#include "GRenderer.h"

#include "GImageInfo.h"

#include "GFxPlayer.h"
#include "GFxLoader.h"

#include "GFxResourceHandle.h"
#include "GFxImageResource.h"

#include "GFxTags.h"

#include "GFxFont.h"

// ***** Declared Classes
class GFxFontResource;
class GFxFontData;
class GFxTextureGlyph;
class GFxTextureGlyphData;


// ***** External Classes
class GFile;
class GFxMovieSub;
class GFxMovieDataDef;
class GFxShapeCharacterDef;
class GFxStream;

class GFxLoadProcess;


#ifndef GFX_FONT_CACHE_TEXTURE_SIZE_MINIMUM
#define GFX_FONT_CACHE_TEXTURE_SIZE_MINIMUM 32
#endif // GFX_FONT_CACHE_TEXTURE_SIZE_MINIMUM




// Class for Holding (cached) textured glyph info.
class GFxTextureGlyph : public GRefCountBase<GFxTextureGlyph>
{
public:
    GFxResourcePtr<GFxImageResource>    pImage;
    GRenderer::Rect                     UvBounds;
    GRenderer::Point                    UvOrigin;   // the origin of the glyph box, in uv coords

    GFxTextureGlyph()
        {}
    ~GFxTextureGlyph()
        { }
    GFxTextureGlyph(const GFxTextureGlyph &src)
        : GRefCountBase<GFxTextureGlyph>()
    {
        pImage      = src.pImage;
        UvBounds    = src.UvBounds;
        UvOrigin    = src.UvOrigin;
    }
    const GFxTextureGlyph& operator = (const GFxTextureGlyph &src)
    {
        pImage      = src.pImage;
        UvBounds    = src.UvBounds;
        UvOrigin    = src.UvOrigin;
        return *this;
    }
    
    
    // Sets image info to a resource.
    void            SetImageResource(GFxImageResource *pres)        { pImage = pres; }
    void            SetImageResource(const GFxResourceHandle& rh)   { pImage.SetFromHandle(rh); }

    // Return this texture glyph has an image resource assigned,
    // and can thus be used for rendering.
    bool            HasImageResource() const    { return !pImage.IsNull(); }

    GImageInfoBase* GetImageInfo(const GFxResourceBinding *pbinding) const;    
};



//
// *** GFxTextureGlyphData
//

// GFxTextureGlyphData stores a collection of all textures and glyphs that
// can be used while rendering a font. It can either (1) be generated  automatically
// during SWF binding, or (2) be loaded from the GFX file. In the first case,
// the only pointer to data will be stored in GFxFontResource, in the second case it
// will also be stored in GFxFontData, so that it can be shared.
//
// The 'gfxexport' tool can cause GFxTextureGlyphData to transition from
// dynamically generated to file-loaded state.


class GFxTextureGlyphData : public GRefCountBase<GFxTextureGlyphData>
{
public:
    typedef GFxFontPackParams::TextureConfig TextureConfig;

private:
    // Describes texture packing settings used.
    TextureConfig  PackTextureConfig;

    typedef GTL::ghash<GFxResourceId, GFxResourcePtr<GFxImageResource> > ImageResourceHash;

    // Texture glyphs strucutre; either built up by tha font packer
    // or loaded from GFX file by tag loaders.
    GTL::garray<GFxTextureGlyph>        TextureGlyphs;
    // Generated textureID -> texture.
    ImageResourceHash                   GlyphsTextures;

    // Set to 1 if info was loaded from GFX file, 0 if created dynamically.
    bool                FileCreation;
public:

    GFxTextureGlyphData(UInt glyphCount = 0, bool isLoadedFromFile = false)
    {
        FileCreation = isLoadedFromFile;
        TextureGlyphs.resize(glyphCount);
    }

    // Obtain/modify variables that control how glyphs are packed in textures.
    void    GetTextureConfig(TextureConfig* pconfig) const { *pconfig = PackTextureConfig; }
    void    SetTextureConfig(const TextureConfig& config)  { PackTextureConfig   = config; }
  
    Float   GetTextureGlyphScale() const;

    bool    IsLoadedFromFile() const { return FileCreation;  }


    // *** Texture assignment / enumeration interfaces

    UInt                    GetTextureGlyphCount() const { return (UInt)TextureGlyphs.size(); }
    const GFxTextureGlyph&  GetTextureGlyph(UInt glyphIndex) const;

    void                    WipeTextureGlyphs();
    void                    AddTextureGlyph(UInt glyphIndex, const GFxTextureGlyph& glyph);

    void                    AddTexture(GFxResourceId textureId, GFxImageResource* pimageRes);
    void                    AddTexture(GFxResourceId textureId, const GFxResourceHandle &rh);

    UInt                    GetConfigTextureWidth()  const { return PackTextureConfig.TextureWidth;  }
    UInt                    GetConfigTextureHeight() const { return PackTextureConfig.TextureHeight; }

    struct TextureGlyphVisitor
    {
        virtual ~TextureGlyphVisitor() {}
        virtual void Visit(UInt index, GFxTextureGlyph* ptextureGlyph) = 0;
    };
    void                    VisitTextureGlyphs(TextureGlyphVisitor*);
    
    struct TexturesVisitor
    {
        virtual ~TexturesVisitor() {}
        virtual void Visit(GFxResourceId textureId, GFxImageResource* pimageRes) = 0;
    };
    void                    VisitTextures(TexturesVisitor*, GFxResourceBinding* pbinding);
};



//
// *** GFxFontData
//

// GFxFontData contains font data that is loaded from an SWF/GFX file.
// This data is associated with GFxMovieDataDef and is thus shared among
// all of the GFxMovieDefImpl instances that can potentially be created.
// This means that GFxFontData object can not cache any instance-specific
// information, such as information about texture size and packing.
//  
// In general, GFxFontData stores the following types of information
//  - Font name and attributes
//  - Kerning, Adance and layout variables
//  - Code points identifying characters in the font
//  - pTextureGlyphData loaded from exported file (*)
//
//  (*) pTextureGlyphData is special in that it contains texture size and
//  image information normally associated with a binding state. This information
//  is stored here ONLY if is loaded from that file; in that case it is shared
//  among all instances. If GFxTextureGlyphData is instead generated dynamically
//  based on GFxFontPackParams setting, as happens during SWF loading, it will only be
//  stored within GFxFontResource instances (GFxFontData::pTextureGlyphData will be null).


class GFxFontData : public GFxFont
{
public:
    GFxFontData();
    GFxFontData(const char* name, UInt fontFlags);
    ~GFxFontData();

    // *** GFX/SWF Loading methods.
    void            Read(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
    void            ReadFontInfo(GFxStream* in, GFxTagType tagType);
   
    // Matches font name and type for substitution purposes.
    bool            MatchSubstituteFont(GFxFontData *fd) const
    {
        return MatchFont(fd->GetName(), fd->GetCreateFontFlags());
    }

    
    // *** GFxFont implementation
    
    GFxTextureGlyphData*    GetTextureGlyphData() const { return pTextureGlyphData;  }
    int                     GetGlyphIndex(UInt16 code);
    bool                    IsHintedVectorGlyph(UInt, UInt) const { return false; }
    bool                    IsHintedRasterGlyph(UInt, UInt) const { return false; }
    GFxShapeCharacterDef*   GetGlyphShape(UInt glyphIndex, UInt glyphSize);
    GFxGlyphRaster*         GetGlyphRaster(UInt, UInt) { return 0; }
    Float                   GetAdvance(UInt glyphIndex) const;
    Float                   GetKerningAdjustment(UInt lastCode, UInt thisCode) const;

    UInt                    GetGlyphShapeCount() const      { return (UInt)Glyphs.size(); }

    Float                   GetGlyphWidth(UInt glyphIndex) const;
    Float                   GetGlyphHeight(UInt glyphIndex) const;
    GRectF&                 GetGlyphBounds(UInt glyphIndex, GRectF* prect) const;

    // Slyph/State access.
    virtual const char*     GetName() const         { return Name; }

     
    // *** Other APIs

    inline bool             HasVectorOrRasterGlyphs() const { return CodeTable.size() != 0; }    
    
    // TextureGlyphData access - only available if loaded from file.   
    void                    SetTextureGlyphData(GFxTextureGlyphData* pdata)
    {
        GASSERT(pdata->IsLoadedFromFile());
        pTextureGlyphData = pdata;
    }

    GFxResourceData     CreateFontResourceData();

private:
    void    ReadCodeTable(GFxStream* in);
    
    // General font information. 
    char*                       Name;

    // We store texture glyph data only if it comes from a file. Dynamically
    // created texture glyphs are accessible only through GFxFontResource.
    GPtr<GFxTextureGlyphData>  pTextureGlyphData;
    
    // If Glyph shapes are loaded and not stripped, they are stored here.    
    GTL::garray< GPtr<GFxShapeCharacterDef> >   Glyphs;


    // This table maps from Unicode character number to glyph index.
    // CodeTable[CharacterCode] = GlyphIndex    
    GTL::ghash_identity<UInt16, UInt> CodeTable;

    // Layout stuff.
    // Note that Leading, Advance and Descent are in the base class
    struct AdvanceEntry
    {
        Float       Advance;
        SInt16      Left;   // offset by X-axis, in twips
        SInt16      Top;    // offset by Y-axis, in twips
        UInt16      Width;  // width, in twips
        UInt16      Height; // height, in twips
    };
    GTL::garray<AdvanceEntry>       AdvanceTable;
    
    struct KerningPair
    {           
        UInt16  Char0, Char1;
        bool    operator==(const KerningPair& k) const
            { return Char0 == k.Char0 && Char1 == k.Char1; }
    };
    GTL::ghash<KerningPair, Float>  KerningPairs;

    // @@ we don't seem to use this thing at all, so don't bother keeping it.
    // GTL::garray<GRenderer::Rect> BoundsTable;    // @@ this thing should be optional.
};




//
// *** GFxFontResource
//
// GFxFontResource is a font-instance resource that delegates a lot of its functionality
// to GFxFontData and GFxTextureData.


class GFxFontResource : public GFxResource
{
public:
    
    // We store fonts binding within font resource for convenient access; this
    // binding is used to look up image resources from TextureGlyphData. It is ok
    // because separate GFxFontResource is always created for every MovieDefImpl.
    // Note that the pbinding can also be null for system fonts.
    GFxFontResource(GFxFont *pfont, GFxResourceBinding* pbinding);
    // For system provider fonts we pass a key.
    GFxFontResource(GFxFont *pfont, const GFxResourceKey& key);
    ~GFxFontResource();

    typedef GFxFont::FontFlags FontFlags;

    
    GFxFont*               GetFont()    const { return pFont; }
    GFxResourceBinding*    GetBinding() const { return pBinding; }

    
    // *** Handler List support.

    // Handlers are used to allow font cache manager to
    // detect when fonts are released.        
    class DisposeHandler
    {
    public:
        virtual ~DisposeHandler() {}
        virtual void OnDispose(GFxFontResource*) = 0;
    };

    void                    AddDisposeHandler(DisposeHandler* h);
    void                    RemoveDisposeHandler(DisposeHandler* h);


    // *** Delegates to GFxFont

    // implement GFxFontData.States = *pFont;

    bool                    MatchFont(const char* name, UInt matchFlags) const { return pFont->MatchFont(name, matchFlags); }
    bool                    MatchFontFlags(UInt matchFlags) const              { return pFont->MatchFontFlags(matchFlags); }
    
    inline const char*      GetName() const                     { return pFont->GetName(); }
    inline UInt             GetGlyphShapeCount() const          { return pFont->GetGlyphShapeCount(); }

    GFxTextureGlyphData*    GetTextureGlyphData() const          { return pTGData.GetPtr(); }
    GFxShapeCharacterDef*   GetGlyphShape(UInt glyphIndex, UInt glyphSize) const 
    { 
        return pFont->GetGlyphShape(glyphIndex, glyphSize); 
    }
    GFxGlyphRaster*         GetGlyphRaster(UInt glyphIndex, UInt glyphSize) const 
    {
        return pFont->GetGlyphRaster(glyphIndex, glyphSize); 
    }

    int                     GetGlyphIndex(UInt16 code) const     { return pFont->GetGlyphIndex(code); }
    bool                    IsHintedVectorGlyph(UInt glyphIndex, UInt glyphSize) const { return pFont->IsHintedVectorGlyph(glyphIndex, glyphSize); } 
    bool                    IsHintedRasterGlyph(UInt glyphIndex, UInt glyphSize) const { return pFont->IsHintedRasterGlyph(glyphIndex, glyphSize); }
    Float                   GetAdvance(UInt glyphIndex) const    { return pFont->GetAdvance(glyphIndex); }
    Float                   GetKerningAdjustment(UInt lastCode, UInt thisCode) const { return pFont->GetKerningAdjustment(lastCode, thisCode);  }
    Float                   GetGlyphWidth(UInt glyphIndex) const  { return pFont->GetGlyphWidth(glyphIndex); }
    Float                   GetGlyphHeight(UInt glyphIndex) const { return pFont->GetGlyphHeight(glyphIndex);  }
    GRectF&                 GetGlyphBounds(UInt glyphIndex, GRectF* prect) const { return pFont->GetGlyphBounds(glyphIndex, prect);  }
    Float                   GetLeading() const                   { return pFont->GetLeading(); }
    Float                   GetDescent() const                   { return pFont->GetDescent(); }
    Float                   GetAscent() const                    { return pFont->GetAscent(); }

    inline UInt             GetFontFlags() const                { return pFont->GetFontFlags(); }
    inline UInt             GetFontStyleFlags() const           { return pFont->GetFontStyleFlags(); }
    inline UInt             GetCreateFontFlags() const          { return pFont->GetCreateFontFlags(); }
    inline bool             IsItalic() const                    { return pFont->IsItalic(); } 
    inline bool             IsBold() const                      { return pFont->IsBold(); } 
    inline bool             IsDeviceFont() const                { return pFont->IsDeviceFont(); } 
    inline bool             HasNativeHinting() const            { return pFont->HasNativeHinting(); }

    inline bool             AreUnicodeChars() const             { return pFont->AreUnicodeChars(); } 
    inline FontFlags        GetCodePage() const                 { return pFont->GetCodePage(); } 
    inline bool             GlyphShapesStripped() const         { return pFont->GlyphShapesStripped(); }
    inline bool             HasLayout() const                   { return pFont->HasLayout(); }     
    inline bool             AreWideCodes() const                { return pFont->AreWideCodes(); }     

    inline UInt16 GetLowerCaseTop(GFxLog* log)
    { 
        if (LowerCaseTop == 0)
            calcLowerUpperTop(log);
        return (LowerCaseTop <= 0) ? 0 : LowerCaseTop; 
    }
    inline UInt16 GetUpperCaseTop(GFxLog* log) 
    { 
        if (UpperCaseTop == 0)
            calcLowerUpperTop(log);
        return (UpperCaseTop <= 0) ? 0 : UpperCaseTop; 
    }

    // *** Delegates to GFxTextureGlyphData

    typedef GFxFontPackParams::TextureConfig TextureConfig;

    // TBD: Necessary?

    inline void GetTextureConfig(TextureConfig* pconfig) const { if (pTGData) pTGData->GetTextureConfig(pconfig); }

    // TextureGlyphData access
    void                    SetTextureGlyphData(GFxTextureGlyphData* pdata) { pTGData = pdata; }


    // *** GFxResource implementation
    
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_Font); }
    virtual GFxResourceKey  GetKey()                        { return FontKey; }

    // Helper function used to lookup and/or create a font resource from provider.
    static GFxFontResource* CreateFontResource(const char* pname, UInt fontFlags,
                                               GFxFontProvider* pprovider, GFxResourceWeakLib *plib);
    // Creates a system font resource key.
    static GFxResourceKey   CreateFontResourceKey(const char* pname, UInt fontFlags,
                                                  GFxFontProvider* pfontProvider);

private:
    UInt16 calcTopBound(UInt16 code);
    void   calcLowerUpperTop(GFxLog* log);

    GPtr<GFxFont>               pFont;

    // Font texture glyphs data. This data is either generated dynamically based
    // on GFxFontPackParams arguments or loaded from disc (in which case it is 
    // shared through pFont).
    GPtr<GFxTextureGlyphData>   pTGData;

    // Pointer to font binding that should be used with embedded fonts; null for
    // system fonts. This pointer would be different in case of imports. It is the 
    // responsibility of the importing file (for direct imports) or font manager 
    // (for font provider lookups) to hold the GFxMovieDefImpl for this binding.    
    GFxResourceBinding*         pBinding;

    // Font key - we store these only for provider-obtained fonts.
    GFxResourceKey              FontKey;
   
    // Handlers list data structures. 
    bool                        HandlerArrayFlag;
    union {
        DisposeHandler*                 pHandler;
        GTL::garray<DisposeHandler*> *  pHandlerArray;  
    };

    // Values used for pixel-grid fitting. Defined as the
    // top Y coordinate for 'z' and 'Z' respectively
    SInt16 LowerCaseTop;
    SInt16 UpperCaseTop;
};

#endif 
