/**********************************************************************

Filename    :   GFxFontMap.cpp
Content     :   Implementation of font name mapping.
Created     :   July 18, 2007
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxLoader.h"
#include "GFxStringHash.h"


// ***** GFxFontMap class

// GFxFontMapImpl contains the font mapping hash table.
class GFxFontMapImpl : public GNewOverrideBase
{
public:    

    GFxStringHash<GFxFontMap::MapEntry>    FontMap;       
};


GFxFontMap::GFxFontMap()
   : GFxState(GFxState::State_FontMap),
     pImpl(new GFxFontMapImpl)
{
}

GFxFontMap::~GFxFontMap()
{
    delete pImpl;
}

bool    GFxFontMap::MapFont(const char* pfontName, const char* pnewFontName, MapFontFlags mff, Float scaleFactor)
{
    if (!pImpl || !pfontName)
        return 0;

    GFxString fontName(pfontName);    
    if (pImpl->FontMap.get_CaseInsensitive(fontName))
    {
        GFC_DEBUG_WARNING1(1, "GFxFontMap::MapFont failed - font name '%s' already mapped",
                           fontName.ToCStr());
        return 0;
    }
    
    MapEntry  me(pnewFontName, mff, scaleFactor);
    pImpl->FontMap.set(fontName, me);
    return 1;
}

bool    GFxFontMap::MapFont(const wchar_t* pfontName, const wchar_t* pnewFontName, MapFontFlags mff, Float scaleFactor)
{
    if (!pImpl || !pfontName)
        return 0;
  
    GFxString fontName(pfontName);
    if (pImpl->FontMap.get_CaseInsensitive(fontName))
    {
        GFC_DEBUG_WARNING1(1, "GFxFontMap::MapFont failed - font name '%s' already mapped",
                           fontName.ToCStr());
        return 0;
    }

    MapEntry  me = MapEntry(GFxString(pnewFontName), mff, scaleFactor);
    pImpl->FontMap.set(fontName, me);
    return 1;
}

// Obtains a font mapping, returning a const string (UTF8).
bool        GFxFontMap::GetFontMapping(MapEntry* pentry, const char *pfontName)
{
    if (!pImpl)
        return 0;

    const MapEntry* pfound = pImpl->FontMap.get_CaseInsensitive(pfontName);   
    if (pfound)
    {
        *pentry = *pfound;
        return 1;
    }
    return 0;
}
