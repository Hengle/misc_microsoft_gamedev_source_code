/**********************************************************************

Filename    :   GFxFontManager.h
Content     :   Font manager functionality
Created     :   May 18, 2007
Authors     :   Artyom Bolgar, Michael Antonov

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXFONTMANAGER_H
#define INC_GFXFONTMANAGER_H

#include "GUTF8Util.h"
#include "GTLTypes.h"
#include "GFxFontResource.h"
#include "GFxStringHash.h"


class GFxFontManager;
class GFxMovieDefImpl;

class GFxFontHandle : public GNewOverrideBase
{
    friend class GFxFontManager;    

    // Our ref-count. This ref-count is not thread-safe
    // because font managers are local to 'root' sprites.
    SInt                    RefCount;    

    // Font manager which we come from. Can be null for static text fields.
    GFxFontManager*         pFontManager;

    // A font device flag applied to a non-device font, as can be done
    // if the font is taken from GFxFontLib due to its precedence.
    UInt                    OverridenFontFlags;   

    // Search FontName - could be an export name instead of the
    // name of the font.
    GFxString               FontName;

public:
    // Font we are holding.
    GPtr<GFxFontResource>   pFont;

    // A strong pointer to GFxMovieDefImpl from which the font was created.
    // Font manager needs to hold these pointers for those fonts which came
    // from the GFxFontProvider, since our instance can be the only reference
    // to them. Null for all other cases.
    GPtr<GFxMovieDef>   pSourceMovieDef;

    Float               FontScaleFactor;


    GFxFontHandle(GFxFontManager *pmanager, GFxFontResource *pfont,
                  const char* pfontName = 0, UInt overridenFontFlags = 0,
                  GFxMovieDef* pdefImpl = 0)
        : RefCount(1), pFontManager(pmanager), OverridenFontFlags(overridenFontFlags),
          pFont(pfont), pSourceMovieDef(pdefImpl), FontScaleFactor(1.f)
    {
        // If font name doesn't match, store it locally.   
        if (pfontName && (GFxString::CompareNoCase(pfont->GetName(), pfontName) != 0))
            FontName = pfontName;
    }
    GFxFontHandle(const GFxFontHandle& f) : RefCount(1), pFontManager(f.pFontManager),
        OverridenFontFlags(f.OverridenFontFlags), FontName(f.FontName), pFont(f.pFont),
        pSourceMovieDef(f.pSourceMovieDef), FontScaleFactor(f.FontScaleFactor)
    {
    }
    inline ~GFxFontHandle();
    
    GFxFontResource*    GetFont() const { return pFont; }
    const char*         GetFontName() const
    {
        return FontName.IsEmpty() ? GetFont()->GetName() : FontName.ToCStr();
    }
    UInt                GetFontFlags() const
    {
        return GetFont()->GetFontFlags() | OverridenFontFlags;
    }
    bool                IsFauxBold() const       { return (OverridenFontFlags & GFxFont::FF_Bold) != 0; }
    bool                IsFauxItalic() const     { return (OverridenFontFlags & GFxFont::FF_Italic) != 0; }
    bool                IsFauxBoldItalic() const { return (OverridenFontFlags & GFxFont::FF_BoldItalic) == GFxFont::FF_BoldItalic; }
    UInt                GetFauxFontStyle() const { return (OverridenFontFlags & GFxFont::FF_Style_Mask); }
    UInt                GetFontStyle() const     
    { 
        return ((GetFont()->GetFontFlags() | GetFauxFontStyle()) & GFxFont::FF_Style_Mask);
    }

    void                AddRef()    { RefCount++; }
    void                Release()   { if (--RefCount == 0) delete this; }

    bool                operator==(const GFxFontHandle& f) const
    {
        return pFontManager == f.pFontManager && pFont == f.pFont && 
               OverridenFontFlags == f.OverridenFontFlags && FontName == f.FontName &&
               FontScaleFactor == f.FontScaleFactor && pSourceMovieDef == f.pSourceMovieDef;
    }
};



class GFxFontManager : public GRefCountBase<GFxFontManager>
{
    friend class GFxFontHandle;    

protected:
    
    // Make a hash-set entry that tracks font handle nodes.
    struct FontKey 
    {
        const char*    pFontName;
        UInt           FontStyle;

        FontKey() : FontStyle(0) {}
        FontKey(const char* pfontName, UInt style)
            : pFontName(pfontName), FontStyle(style) { }

        static size_t CalcHashCode(const char* pname, size_t namelen, UInt fontFlags)
        {
            size_t hash = GFxString::BernsteinHashFunctionCIS(pname, namelen);
            return (hash ^ (fontFlags & GFxFont::FF_Style_Mask));
        }
    };

    struct NodePtr
    {
        GFxFontHandle* pNode;
               
        NodePtr() { }
        NodePtr(GFxFontHandle* pnode) : pNode(pnode) { }
        NodePtr(const NodePtr& other) : pNode(other.pNode) { }
        
        // Two nodes are identical if their fonts match.
        bool operator == (const NodePtr& other) const
        {
            if (pNode == other.pNode)
                return 1;
            // For node comparisons we have to handle DeviceFont strictly ALL the time,
            // because it is possible to have BOTH versions of font it the hash table.
            enum { FontFlagsMask = GFxFont::FF_CreateFont_Mask | GFxFont::FF_DeviceFont | GFxFont::FF_BoldItalic};
            UInt ourFlags   = pNode->GetFontFlags() & FontFlagsMask;
            UInt otherFlags = other.pNode->GetFontFlags() & FontFlagsMask;
            return ((ourFlags == otherFlags) &&
                    !GFxString::CompareNoCase(pNode->GetFontName(), other.pNode->GetFontName()));
        }        

        // Key search uses MatchFont to ensure that DeviceFont flag is handled correctly.
        bool operator == (const FontKey &key) const
        {
            return (GFxFont::MatchFontFlags_Static(pNode->GetFontFlags(), key.FontStyle) &&
                    !GFxString::CompareNoCase(pNode->GetFontName(), key.pFontName));
        }
    };
    
    struct NodePtrHashOp
    {                
        // Hash code is computed based on a state key.
        size_t  operator() (const NodePtr& other) const
        {            
            GFxFontHandle* pnode = other.pNode;
            const char* pfontName = pnode->GetFontName();
            return (size_t) FontKey::CalcHashCode(pfontName, gfc_strlen(pfontName), 
                pnode->pFont->GetFontFlags() | pnode->GetFontStyle());
        }
        size_t  operator() (const GFxFontHandle* pnode) const
        {
            GASSERT(pnode != 0);            
            const char* pfontName = pnode->GetFontName();
            return (size_t) FontKey::CalcHashCode(pfontName, gfc_strlen(pfontName), 
                pnode->pFont->GetFontFlags() | pnode->GetFontStyle());
        }
        size_t  operator() (const FontKey& data) const
        {
            return (size_t) FontKey::CalcHashCode(data.pFontName, gfc_strlen(data.pFontName), data.FontStyle);
        }
    };


    // State hash
    typedef GTL::ghash_set<NodePtr, NodePtrHashOp> FontSet;

    // Keep a hash of all allocated nodes, so that we can find one when necessary.
    // Nodes automatically remove themselves when all their references die.
    FontSet                 CreatedFonts;

    // GFxMovieDefImpl which this font manager is associated with. We look up
    // fonts here by default before considering pFontProvider. Note that this
    // behavior changes if device flags are passed.
    GFxMovieDefImpl *       pDefImpl;
    // Shared state for the instance, comes from GFxMovieRoot. Shouldn't need 
    // to AddRef since nested sprite (and font manager) should die before MovieRoot.    
    GFxSharedState*         pSharedState;


    // Empty fake font returned if requested font is not found.
    GPtr<GFxFontHandle>      pEmptyFont; 

    // FontMap Entry temporary; here to avoid extra constructor/destructor calls.
    GFxFontMap::MapEntry     FontMapEntry;

   
    // Helper function to remove handle from CreatedFonts when it dies.
    void                     RemoveFontHandle(GFxFontHandle *phandle);

    GFxFontHandle*           FindOrCreateHandle
        (const char* pfontName, UInt matchFontFlags, GFxFontResource** ppfoundFont);
public:
    GFxFontManager(GFxMovieDefImpl *pdefImpl,
                   GFxSharedState* psharedState);
    ~GFxFontManager();


    // Returns font by name and style
    GFxFontHandle*          CreateFontHandle(const char* pfontName, UInt matchFontFlags); 

    GFxFontHandle*          CreateFontHandle(const char* pfontName, bool bold, bool italic, bool device = 0)
    { 
        UInt matchFontFlags = ((bold) ? GFxFont::FF_Bold : 0) |
                              ((italic) ? GFxFont::FF_Italic : 0) |
                              ((device) ? GFxFont::FF_DeviceFont : 0);
        return CreateFontHandle(pfontName, matchFontFlags);
    }

    // Returns any font with the font name 'pfontName' or the first one
    // in the hash.
    GFxFontHandle*          CreateAnyFontHandle(const char* pfontName);
};


// Font handle inlined after font manager because it uses manager's APIs.
inline GFxFontHandle::~GFxFontHandle()
{        
    // Remove from hash.
    if (pFontManager)
        pFontManager->RemoveFontHandle(this);
}


#endif //INC_GFXFONTMANAGER_H
