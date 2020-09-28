/**********************************************************************

Filename    :   GFxMovieClipLoader.h
Content     :   Implementation of MovieClipLoader class
Created     :   March, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXMOVIECLIPLOADER_H
#define INC_GFXMOVIECLIPLOADER_H

#include "GFxAction.h"
#include "GFxString.h"
#include "GFxStringHash.h"
#include "GFxCharacter.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASMovieClipLoader;
class GASMovieClipLoaderProto;
class GASMovieClipLoaderCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASMovieClipLoader : public GASObject
{
    friend class GASMovieClipLoaderProto;

    struct ProgressDesc
    {
        int LoadedBytes;
        int TotalBytes;

        ProgressDesc() {}
        ProgressDesc(int loadedBytes, int totalBytes): LoadedBytes(loadedBytes), TotalBytes(totalBytes) {}
    };
    GFxStringHash<ProgressDesc> ProgressInfo;
    void commonInit (GASEnvironment* penv);

public:
    GASMovieClipLoader(GASStringContext* psc = 0) : GASObject() { GUNUSED(psc); }
    GASMovieClipLoader(GASEnvironment* penv);

    ObjectType      GetObjectType() const   { return Object_MovieClipLoader; }

    virtual void NotifyOnLoadStart(GASEnvironment* penv, GFxASCharacter* ptarget);
    virtual void NotifyOnLoadComplete(GASEnvironment* penv, GFxASCharacter* ptarget, int status);
    virtual void NotifyOnLoadInit(GASEnvironment* penv, GFxASCharacter* ptarget);
    virtual void NotifyOnLoadError(GASEnvironment* penv, GFxASCharacter* ptarget, const char* errorCode, int status);
    virtual void NotifyOnLoadProgress(GASEnvironment* penv, GFxASCharacter* ptarget, int loadedBytes, int totalBytes);

    int GetLoadedBytes(GFxASCharacter* pch) const;
    int GetTotalBytes(GFxASCharacter* pch)  const;
};

class GASMovieClipLoaderProto : public GASPrototype<GASMovieClipLoader>
{
public:
    GASMovieClipLoaderProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GetProgress(const GASFnCall& fn);
    static void LoadClip(const GASFnCall& fn);
    static void UnloadClip(const GASFnCall& fn);
};

class GASMovieClipLoaderCtorFunction : public GASFunctionObject
{
public:
    GASMovieClipLoaderCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
};


#endif // INC_GFXMOVIECLIPLOADER_H
