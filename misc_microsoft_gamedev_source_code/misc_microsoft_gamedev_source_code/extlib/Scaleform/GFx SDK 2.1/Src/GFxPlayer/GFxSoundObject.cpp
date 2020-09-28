/**********************************************************************

Filename    :   GFxSoundObject.cpp
Content     :   Number object functinality
Created     :   October 20, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

//#include "GFxSound.h"
#include "GFxSoundObject.h"

#include <stdio.h>
#include <stdlib.h>


void    GASSoundProto::Start(const GASFnCall& fn)
{
    GUNUSED(fn);
}


void    GASSoundProto::Stop(const GASFnCall& fn)
{
    GUNUSED(fn);
}

void    GASSoundProto::Attach(const GASFnCall& fn)
{
    GUNUSED(fn);
}
void    GASSoundProto::GetBytesLoaded(const GASFnCall& fn) { fn.Result->SetInt(0); }
void    GASSoundProto::GetBytesTotal(const GASFnCall& fn)  { fn.Result->SetInt(0); }
void    GASSoundProto::GetPan(const GASFnCall& fn)         { fn.Result->SetInt(0); }
void    GASSoundProto::GetTransform(const GASFnCall& fn)   { fn.Result->SetUndefined(); }
void    GASSoundProto::GetVolume(const GASFnCall& fn)      { fn.Result->SetInt(0); }
void    GASSoundProto::LoadSound(const GASFnCall& fn)      { GUNUSED(fn); }
void    GASSoundProto::SetPan(const GASFnCall& fn)         { GUNUSED(fn); }
void    GASSoundProto::SetTransform(const GASFnCall& fn)   { GUNUSED(fn); }
void    GASSoundProto::SetVolume(const GASFnCall& fn)      { GUNUSED(fn); }

// Constructor for ActionScript class Sound.
void    GASSoundProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASObject> SoundObj = *new GASSoundObject(fn.Env);
    fn.Result->SetAsObject(SoundObj.GetPtr());
}

static const GASNameFunction GAS_SoundFunctionTable[] = 
{
    { "attachSound",    &GASSoundProto::Attach  },
    { "getBytesLoaded", &GASSoundProto::GetBytesLoaded },
    { "getBytesTotal",  &GASSoundProto::GetBytesTotal },
    { "getPan",         &GASSoundProto::GetPan },
    { "getTransform",   &GASSoundProto::GetTransform },
    { "getVolume",      &GASSoundProto::GetVolume },
    { "loadSound",      &GASSoundProto::LoadSound },
    { "setPan",         &GASSoundProto::SetPan },
    { "setTransform",   &GASSoundProto::SetTransform },
    { "setVolume",      &GASSoundProto::SetVolume },
    { "start",          &GASSoundProto::Start },
    { "stop",           &GASSoundProto::Stop },
    { 0, 0 }
};

GASSoundProto::GASSoundProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) :
GASPrototype<GASSoundObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_SoundFunctionTable, pprototype);
}
