/**********************************************************************

Filename    :   GFxMovieClipLoader.cpp
Content     :   Implementation of MovieClipLoader class
Created     :   March, 2007
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxASString.h"
#include "GFxAction.h"
#include "GFxPlayerImpl.h"
#include "GFxMovieClipLoader.h"
#include "GUTF8Util.h"
#include "GFxAsBroadcaster.h"

GASMovieClipLoader::GASMovieClipLoader(GASEnvironment* penv)
{
    commonInit(penv);
}

void GASMovieClipLoader::commonInit (GASEnvironment* penv)
{    
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_MovieClipLoader));
    GASAsBroadcaster::Initialize(penv->GetSC(), this);
}

int GASMovieClipLoader::GetLoadedBytes(GFxASCharacter* pch) const
{
    GFxString path;
    pch->GetAbsolutePath(&path);
    const ProgressDesc* pprogress = ProgressInfo.get(path.ToCStr());
    if (pprogress)
        return pprogress->LoadedBytes;
    return 0;
}

int GASMovieClipLoader::GetTotalBytes(GFxASCharacter* pch)  const
{
    GFxString path;
    pch->GetAbsolutePath(&path);
    const ProgressDesc* pprogress = ProgressInfo.get(path.ToCStr());
    if (pprogress)
        return pprogress->TotalBytes;
    return 0;
}

void GASMovieClipLoader::NotifyOnLoadStart(GASEnvironment* penv, GFxASCharacter* ptarget)
{
    penv->Push(ptarget);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onLoadStart"), 1, penv->GetTopIndex());
    penv->Drop1();
}

void GASMovieClipLoader::NotifyOnLoadComplete(GASEnvironment* penv, GFxASCharacter* ptarget, int status)
{
    penv->Push(status);
    penv->Push(ptarget);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onLoadComplete"), 2, penv->GetTopIndex());
    penv->Drop2();
}

void GASMovieClipLoader::NotifyOnLoadInit(GASEnvironment* penv, GFxASCharacter* ptarget)
{
    penv->Push(ptarget);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onLoadInit"), 1, penv->GetTopIndex());
    penv->Drop1();
}

void GASMovieClipLoader::NotifyOnLoadError(GASEnvironment* penv, GFxASCharacter* ptarget, const char* errorCode, int status)
{
    penv->Push(status);
    penv->Push(penv->CreateConstString(errorCode));
    penv->Push(ptarget);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onLoadError"), 3, penv->GetTopIndex());
    penv->Drop3();
}

void GASMovieClipLoader::NotifyOnLoadProgress(GASEnvironment* penv, GFxASCharacter* ptarget, int loadedBytes, int totalBytes)
{
    if (ptarget)
    {
        GFxString path;
        ptarget->GetAbsolutePath(&path);
        ProgressDesc* pprogress = ProgressInfo.get(path.ToCStr());
        if (pprogress)
        {
            pprogress->LoadedBytes = loadedBytes;
            pprogress->TotalBytes  = totalBytes;
        }
        else
        {
            ProgressInfo.add(path, ProgressDesc(loadedBytes, totalBytes));
        }
    }

    penv->Push(totalBytes);
    penv->Push(loadedBytes);
    penv->Push(ptarget);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onLoadProgress"), 3, penv->GetTopIndex());
    penv->Drop3();
}

//////////////////////////////////////////
//
void GASMovieClipLoaderProto::GetProgress(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);
    fn.Result->SetUndefined();

    GASMovieClipLoader* pmovieClipLoader = NULL;
    if (fn.ThisPtr->GetObjectType() == GASObject::Object_MovieClipLoader)
    {
        pmovieClipLoader = static_cast<GASMovieClipLoader*>(fn.ThisPtr);

        GPtr<GASObject> pobj = *new GASObject(fn.Env);

        GASString path = fn.Arg(0).ToString(fn.Env);
        ProgressDesc* pprogress = pmovieClipLoader->ProgressInfo.get(path.ToCStr());
        if (pprogress)
        {
            pobj->SetConstMemberRaw(fn.Env->GetSC(), "bytesLoaded", GASValue(pprogress->LoadedBytes));
            pobj->SetConstMemberRaw(fn.Env->GetSC(), "bytesTotal",  GASValue(pprogress->TotalBytes));
        }
        fn.Result->SetAsObject(pobj);
    }
}

void GASMovieClipLoaderProto::LoadClip(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.NArgs < 2)
        return;

    GASSERT(fn.ThisPtr);

    GASMovieClipLoader* pmovieClipLoader = NULL;
    if (fn.ThisPtr->GetObjectType() == GASObject::Object_MovieClipLoader)
        pmovieClipLoader = static_cast<GASMovieClipLoader*>(fn.ThisPtr);

    // Post loadMovie into queue.
    GASString urlStr(fn.Arg(0).ToString(fn.Env));
    GPtr<GFxASCharacter> psprite;
    if (fn.Arg(1).IsCharacter())
        psprite = fn.Arg(1).ToASCharacter(fn.Env);
    else
        psprite = fn.Env->FindTarget(fn.Arg(1).ToString(fn.Env));
    if (!psprite)
        return;

    fn.Env->GetMovieRoot()->AddLoadQueueEntry(psprite, urlStr.ToCStr(), GFxLoadQueueEntry::LM_None, pmovieClipLoader);
    fn.Result->SetBool(true);
}

void GASMovieClipLoaderProto::UnloadClip(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GPtr<GFxASCharacter> psprite;
    if (fn.Arg(0).IsCharacter())
        psprite = fn.Arg(0).ToASCharacter(fn.Env);
    else
        psprite = fn.Env->FindTarget(fn.Arg(0).ToString(fn.Env));
    if (!psprite)
        return;

    // Post unloadMovie into queue (empty url == unload).
    fn.Env->GetMovieRoot()->AddLoadQueueEntry(psprite, "");
    fn.Result->SetBool(true);
}

static const GASNameFunction GAS_MovieClipLoaderFuncTable[] = 
{
    { "getProgress",        &GASMovieClipLoaderProto::GetProgress       },
    { "loadClip",           &GASMovieClipLoaderProto::LoadClip          },
    { "unloadClip",         &GASMovieClipLoaderProto::UnloadClip        },
    { 0, 0 }
};

GASMovieClipLoaderProto::GASMovieClipLoaderProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASMovieClipLoader>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_MovieClipLoaderFuncTable, pprototype);
}

//////////////////
GASMovieClipLoaderCtorFunction::GASMovieClipLoaderCtorFunction(GASStringContext *) :
    GASFunctionObject(GASMovieClipLoaderCtorFunction::GlobalCtor)
{
}

void GASMovieClipLoaderCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASMovieClipLoader> ab = *new GASMovieClipLoader(fn.Env);
    GASAsBroadcaster::AddListener(fn.Env, ab, ab);
    fn.Result->SetAsObject(ab.GetPtr());
}

