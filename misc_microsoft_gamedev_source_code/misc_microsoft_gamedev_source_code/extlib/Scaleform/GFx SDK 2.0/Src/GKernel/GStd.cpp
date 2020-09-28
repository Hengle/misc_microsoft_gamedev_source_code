/**********************************************************************

Filename    :   GStd.cpp
Content     :   Standard C function implementation
Created     :   2/25/2007
Authors     :   Brendan Iribe, Michael Antonov
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
