/**********************************************************************

Filename    :   GFxFontManager.cpp
Content     :   Font manager functinality
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
#include "GFxFontManager.h"
#include "GFxMovieDef.h"

//////////////////////////////////
// GFxFontManager
//
GFxFontManager::GFxFontManager(GFxMovieDefImpl *pdefImpl,
                               GFxSharedState *psharedState)
{
    pDefImpl      = pdefImpl;
    pSharedState  = psharedState;

    GPtr<GFxFontData> pfontData = *new GFxFontData();
    GPtr<GFxFontResource> pemptyFont = *new GFxFontResource(pfontData, 0);
    pEmptyFont = *new GFxFontHandle(0, pemptyFont);
}

GFxFontManager::~GFxFontManager()
{
    // CachedFonts must be empty here.
    GASSERT(CreatedFonts.size() == 0);
}


void GFxFontManager::RemoveFontHandle(GFxFontHandle *phandle)
{
    if (phandle != pEmptyFont)
    {
        UInt fontCount = (UInt)CreatedFonts.size();
        GUNUSED(fontCount);
        CreatedFonts.remove(phandle);
        // This should not fail but has been seen to do so due to font style mask
        // mismatch. The new operator == which compares pointers resolves that.
        GASSERT(CreatedFonts.size() == fontCount - 1);
    }
}

GFxFontHandle* GFxFontManager::FindOrCreateHandle
    (const char* pfontName, UInt matchFontFlags, GFxFontResource** ppfoundFont)
{
// DBG
//GASSERT(!(matchFontFlags & GFxFont::FF_BoldItalic));
//if (matchFontFlags & GFxFont::FF_BoldItalic)
//    return 0;

    FontKey          key(pfontName, matchFontFlags);
    GFxFontHandle*   phandle       = 0;    
    GFxFontLib*      pfontLib      = pDefImpl->pBindStates->pFontLib;
    GFxFontProvider* pfontProvider = pDefImpl->pBindStates->pFontProvider;
    GFxFontMap*      pfontMap      = pDefImpl->pBindStates->pFontMap;

    // Our matchFontFlag argument may include FF_DeviceFont flag if device preference is
    // requested. The biggest part this is handled correctly by MatchFont used in our
    // CreatedFonts.get(), pDefImpl->GetFontResource and GFxFontLib lookups. However,
    // if device font match is not found we may need to do a second cycle to find an embedded
    // font instead (default Flash behavior, since DeviceFont is only a hint).
retry_font_lookup:

    // If the font has already been created, return it.
    const NodePtr* psetNode = CreatedFonts.get(key);
    if (psetNode)
    {
        psetNode->pNode->AddRef();
        return psetNode->pNode;
    }

    // Search MovieDef for the desired font and create it if necessary.
    // The resource is not AddRefed here, so don't do extra release.
    GFxFontResource* pfontRes = pDefImpl->GetFontResource(pfontName, matchFontFlags);
    if (pfontRes)
    {
        // If we are looking for bold/italic, and there is an empty font found, 
        // then return NULL to give opportunity to faux bold/italic.
        if ((matchFontFlags & GFxFont::FF_BoldItalic) && 
            !pfontRes->GetFont()->HasVectorOrRasterGlyphs())
        {
            // need to pass the found font out, just in case if font manager
            // fails looking for a regular font for faux bold/italic
            if (ppfoundFont) 
                *ppfoundFont = pfontRes;
            return 0;
        }
        // The above lookup can return a font WITH A DIFFERENT name in
        // case export names were used for a search. If that happens,
        // the passed name will be stored and used in GFxFontHandle.
        phandle = new GFxFontHandle(this, pfontRes, pfontName);
    }

    // Lookup an alternative font name for the font. Font flags can
    // also be altered if the font map forces a style.
    const char* plookupFontName = pfontName;
    UInt        lookupFlags     = matchFontFlags;
    Float       scaleFactor     = 1.0f;

    if (!phandle && pfontMap)
    {       
        if (pfontMap->GetFontMapping(&FontMapEntry, pfontName))
        {
            plookupFontName = FontMapEntry.Name.ToCStr();
            lookupFlags     = FontMapEntry.UpdateFontFlags(matchFontFlags);
            scaleFactor     = FontMapEntry.ScaleFactor;

            // When we substitute a font name, it is possible that
            // this name already existed - so do a hash lookup again.
            FontKey    lookupKey(plookupFontName, lookupFlags);
            psetNode = CreatedFonts.get(lookupKey);
            if (psetNode)
            {
                //?GASSERT(psetNode->pNode->FontScaleFactor == scaleFactor); //?AB: should they be equal?
                // do we need to create another font handle if not?
                psetNode->pNode->AddRef();
                return psetNode->pNode;
            }

            // Well, we need to check for (pfontName, changed lookupFlags) also,
            // since in the case if a normal font with name "name" is substituted by 
            // the bold one, that substitution will be placed in the hash table as (name, Bold),
            // even though we were (and will be next time!) looking for the (name, 0).
            FontKey    lookupKey2(pfontName, lookupFlags);
            psetNode = CreatedFonts.get(lookupKey2);
            if (psetNode)
            {
                psetNode->pNode->AddRef();
                return psetNode->pNode;
            }
        }
    }

    if (!phandle && pfontLib)
    {
        // Find/create compatible font through FontLib.
        GFxFontLib::FontResult fr;
        if (pfontLib->FindFont(&fr, plookupFontName, lookupFlags,
            pDefImpl, pSharedState))
        {
            // Use font handle to hold reference to resource's MovieDef.
            phandle = new GFxFontHandle(this, fr.GetFontResource(),
                pfontName, 0, fr.GetMovieDef());
        }
    }

    if (!phandle && pfontProvider)
    {
        // Create or lookup a system font if system is available.
        GPtr<GFxFontResource> pfont = 
            *GFxFontResource::CreateFontResource(plookupFontName, lookupFlags,
            pfontProvider, pDefImpl->GetWeakLib());
        if (pfont)
        {
            phandle = new GFxFontHandle(this, pfont, pfontName);
        }
    }


    // If device font was not found, try fontLib without that flag,
    // as GFxFontLib is allowed to masquerade as a device font source.
    if (!phandle && pfontLib)
    {
        // Find/create compatible font through FontLib.
        GFxFontLib::FontResult fr;
        if (pfontLib->FindFont(&fr, plookupFontName,
            lookupFlags & ~GFxFont::FF_DeviceFont,
            pDefImpl, pSharedState))
        {
            // Use font handle to hold reference to resource's MovieDef.
            phandle = new GFxFontHandle(this, fr.GetFontResource(),
                pfontName, GFxFont::FF_DeviceFont,
                fr.GetMovieDef());
        }
    }


    // If font handle was successfully created, add it to list and return.
    if (phandle)
    {
        GASSERT(CreatedFonts.get(phandle) == 0);
        phandle->FontScaleFactor = scaleFactor;
        CreatedFonts.add(phandle);
        return phandle;
    }

    // If DeviceFont was not found, search for an embedded font.
    if (matchFontFlags & GFxFont::FF_DeviceFont)
    {
        matchFontFlags &= ~GFxFont::FF_DeviceFont;
        // Must modify key flags value otherwise incorrect multiple copies of the same
        // font would get created and put into a hash (causing waste and crash later).
        key.FontStyle  = matchFontFlags;
        // Do not search font provider in a second pass since failure in
        // the first pass would imply failure in the second one as well.
        pfontProvider = 0;
        pfontLib = 0;
        goto retry_font_lookup;
    }
    return 0;
}

GFxFontHandle* GFxFontManager::CreateFontHandle(const char* pfontName, UInt matchFontFlags)
{
    GFxFontResource*    pfoundFont = NULL;
    GFxFontHandle*      phandle = FindOrCreateHandle(pfontName, matchFontFlags, &pfoundFont);

    if (!phandle)
    {
        // if Bold or/and Italic was not found, search for a regular font to 
        // make faux bold/italic
        if (matchFontFlags & GFxFont::FF_BoldItalic)
        {
            UInt newMatchFontFlags = matchFontFlags & (~GFxFont::FF_BoldItalic);
            GPtr<GFxFontHandle> pplainHandle = *FindOrCreateHandle(pfontName, newMatchFontFlags, NULL);
            if (pplainHandle)
            {
                // copy the handle, modify with faux bold/italic flags and add
                // it to a table.
                phandle = new GFxFontHandle(*pplainHandle);
                phandle->OverridenFontFlags |= (matchFontFlags & GFxFont::FF_BoldItalic);
                
                // If font handle was successfully created, add it to list and return.
                GASSERT(CreatedFonts.get(phandle) == 0);
                CreatedFonts.add(phandle);
            }
        }
    }

    // if handle is not created, but the original font (empty) was found - 
    // use it.
    if (!phandle && pfoundFont)
    {
        phandle = new GFxFontHandle(this, pfoundFont, pfontName);
        CreatedFonts.add(phandle);
    }

    return phandle;
}


GFxFontHandle* GFxFontManager::CreateAnyFontHandle(const char* pfontName) 
{
    GFxFontHandle* pfontHandle = CreateFontHandle(pfontName, 0);
    if (!pfontHandle)
    {
        pEmptyFont->AddRef();
        return pEmptyFont;
    }
    return pfontHandle;    
}
