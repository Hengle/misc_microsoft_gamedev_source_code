/**********************************************************************

Filename    :   GFxFontLib.cpp
Content     :   Implementation of font sharing and lookup through
                a list of registed GFxMovieDef's.
Created     :   July 9, 2007
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxFontLib.h"
#include "GFxFontResource.h"

#include "GFxMovieDef.h"
#include "GFxLoadProcess.h"


// ***** GFxFontLib class

// GFxFontLib stores smart pointers to GFxMovieDataDef instead of 
// GFxMovieDefImpl. This is important to avoid circular ref-counting
// cycles between the states, movie def and font lib. The actual
// DefImpl is instead looked up through CreateMovie, which in most cases
// will just return a new pointer from GFxResourceLib.
// An additional benefit we get from such late binding is the use of
// correct states for the imported font movies; these states are taken
// from the context that referenced the font. These states affect
// texture packing, image creation and other aspects of loading.


// GFxFontLibImpl is used to hide the implementation details of GFxFontLib.
class GFxFontLibImpl : public GNewOverrideBase
{
public:
   
    // A list of GFxMovieDataDef pointers to movies
    // that serve as a source for fonts
    GTL::garray<GPtr<GFxMovieDataDef> >  FontMovies;
};


//------------------------------------------------------------------------
GFxFontLib::GFxFontLib()
   : GFxState(GFxState::State_FontLib),
     pImpl(new GFxFontLibImpl)
{
}

GFxFontLib::~GFxFontLib()
{
    delete pImpl;
}

void     GFxFontLib::AddFontsFrom(GFxMovieDef* md, bool pin)
{
    GFxMovieDefImpl *pmdi = (GFxMovieDefImpl*)md;

    if (pImpl && pmdi)
    {        
        pImpl->FontMovies.push_back(pmdi->GetDataDef());
        if (pin)
           md->PinResource();
    }
}


// *** FontResult - stores smart pointers to result with hidden implementation

void    GFxFontLib::FontResult::SetResult(GFxMovieDef* pmovieDef, GFxFontResource* pfont)
{
    if (pmovieDef)
        pmovieDef->AddRef();
    if (pfont)
        pfont->AddRef();

    if (pMovieDef)
        pMovieDef->Release();
    if (pFontResource)
        pFontResource->Release();

    pMovieDef     = pmovieDef;
    pFontResource = pfont;        
}

GFxFontLib::FontResult::~FontResult()
{
    if (pMovieDef)
        pMovieDef->Release();
    if (pFontResource)
        pFontResource->Release();
}



// Finds or creates a font of specified style by searching all of the
// registered GFxMovieDef objects. The font resource is created to be compatible
// with puserDef's binding states and utilizing psharedStates for non-binding
// state values.
bool GFxFontLib::FindFont(FontResult *presult,
                          const char* pfontname, UInt fontFlags,
                          GFxMovieDef* puserDef, GFxSharedState *psharedStates)
{    
    // We keep device font bits for matching so that FontManager can correctly
    // try device fonts before non-device ones.
    fontFlags &= GFxFont::FF_CreateFont_Mask| GFxFont::FF_DeviceFont;

    // First, find a matching font in our MovieDefs.
    if (!pImpl)
        return 0;

    UInt             i, resIndex = 0;
    GFxMovieDataDef *pdataDef = 0;
    bool             fontFound = 0;

    for (i = 0; (i<pImpl->FontMovies.size()) && !fontFound; i++)
    {
        pdataDef = pImpl->FontMovies[i];
        // TBD: We will need to do something about threading here, since it is legit
        // to call GetFontResource while loading still hasn't completed.
        for(resIndex=0; resIndex<pdataDef->Fonts.size(); resIndex++)
        {
            if (pdataDef->Fonts[resIndex].pFontData->MatchFont(pfontname, fontFlags))
            {
                   
                fontFound = 1;
                break;
            }
        }
    }

    if (!fontFound)
        return 0;


    GFxMovieDefImpl* pudi = (GFxMovieDefImpl*) puserDef;

    // Create loadStates with:
    //  1) Binding states from puserDef
    //  2) Dynamic states from psharedStates. These will probably come
    //     from GFxMovieRoot for the instance which is using this font.
    GPtr<GFxLoadStates> pls = *new GFxLoadStates(pudi->pLoaderImpl,
                                                 psharedStates,
                                                 pudi->pBindStates);    
    // Get movieDef. Most of the time this will just look up MovieDefImpl
    // in the library without performing the actual binding.
    GPtr<GFxMovieDefImpl> pfontDefImpl = 
        *pudi->pLoaderImpl->CreateMovie_LoadState(pls, pdataDef, pudi->LoadFlags);
    if (!pfontDefImpl)
        return 0;

    // TBD: Threading - Should we wait for binding to complete?
    GFxResourceBindData rbd;
    pfontDefImpl->ResourceBinding.GetResourceData(&rbd,
                        pdataDef->Fonts[resIndex].ResourceBindIndex);
    if (rbd.pResource)
    {
        GASSERT(rbd.pResource->GetResourceType() == GFxResource::RT_Font);        
        presult->SetResult(pfontDefImpl, (GFxFontResource*) rbd.pResource.GetPtr());
        return 1;        
    }           
    return 0;
}




