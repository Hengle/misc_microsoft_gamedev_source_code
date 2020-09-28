/**********************************************************************

Filename    :   GStd.cpp
Content     :   Standard C function implementation
Created     :   2/25/2007
Authors     :   Ankur Mohan, Michael Antonov, Artem Bolgar
Copyright   :   (c) 2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "GStd.h"

// Source for functions not available on all platforms is included here.

// Case insensitive compare implemented in platform-specific way.
int gfc_stricmp(const char* a, const char* b)
{
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || defined(GFC_OS_WINCE)
    #if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
        return ::_stricmp(a, b);
    #else
        return ::stricmp(a, b);
    #endif

#elif defined(GFC_CC_RENESAS)
    const char *pa = a, *pb = b;
    while (*pa && *pb)
    {
        char ca = tolower(*pa);
        char cb = tolower(*pb);
        if (ca < cb)
            return -1;
        else if (ca > cb)
            return 1;
        pa++;
        pb++;
    }
    if (*pa)
        return 1;
    else if (*pb)
        return -1;
    else
        return 0;

#else
    return strcasecmp(a, b);
#endif
}

int gfc_strnicmp(const char* a, const char* b, size_t count)
{
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || defined(GFC_OS_WINCE)
#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
    return ::_strnicmp(a, b, count);
#else
    return ::strnicmp(a, b, count);
#endif

#elif defined(GFC_CC_RENESAS)
    if (count)
    {
        int f,l;
        do {
            f = (SPInt)tolower((int)(*(a++)));
            l = (SPInt)tolower((int)(*(b++)));
        } while (--count && f && (f == l));

        return int(f - l);
    }
    return 0;
#else
    return strncasecmp(a, b, count);
#endif
}

wchar_t* gfc_wcscpy(wchar_t* dest, size_t destsize, const wchar_t* src)
{
#if defined(GFC_MSVC_SAFESTRING)
    wcscpy_s(dest, destsize, src);
    return dest;
#elif defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || defined(GFC_OS_WINCE) || defined(GFC_OS_PS3) || defined(GFC_OS_WII)
    GUNUSED(destsize);
    wcscpy(dest, src);
    return dest;
#else
    size_t l = gfc_wcslen(src) + 1; // incl term null
    l = (l < destsize) ? l : destsize;
    memcpy(dest, src, l * sizeof(wchar_t));
    return dest;
#endif
}

wchar_t* gfc_wcscat(wchar_t* dest, size_t destsize, const wchar_t* src)
{
#if defined(GFC_MSVC_SAFESTRING)
    wcscat_s(dest, destsize, src);
    return dest;
#elif defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || defined(GFC_OS_WINCE) || defined(GFC_OS_PS3) || defined(GFC_OS_WII)
    GUNUSED(destsize);
    wcscat(dest, src);
    return dest;
#else
    size_t dstlen = gfc_wcslen(dest); // do not incl term null
    size_t srclen = gfc_wcslen(src) + 1; // incl term null
    size_t copylen = (dstlen + srclen < destsize) ? srclen : destsize - dstlen;
    memcpy(dest + dstlen, src, copylen * sizeof(wchar_t));
    return dest;
#endif
}

size_t   gfc_wcslen(const wchar_t* str)
{
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360) || defined(GFC_OS_WINCE) || defined(GFC_OS_PS3) || defined(GFC_OS_WII)
    return wcslen(str);
#else
    size_t i = 0;
    while(str[i] != '\0')
        ++i;
    return i;
#endif
}
