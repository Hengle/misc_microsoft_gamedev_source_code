/**********************************************************************

Filename    :   GFxAsBroadcaster.cpp
Content     :   Implementation of AsBroadcaster class
Created     :   October, 2006
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

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
#include "GFxArray.h"
#include "GFxNumber.h"
#include "GFxAsBroadcaster.h"
#include "GUTF8Util.h"

GASAsBroadcaster::GASAsBroadcaster(GASEnvironment* penv)
{
    commonInit(penv);
}

void GASAsBroadcaster::commonInit (GASEnvironment* penv)
{    
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_AsBroadcaster));
}
    
//////////////////////////////////////////
//
void GASAsBroadcasterProto::AddListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GASObjectInterface* plistener = fn.Arg(0).ToObjectInterface(fn.Env);
    GASAsBroadcaster::AddListener(fn.Env, fn.ThisPtr, plistener);
    fn.Result->SetBool(true);
}

void GASAsBroadcasterProto::BroadcastMessage(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GASString eventName(fn.Arg(0).ToString(fn.Env));
    GASAsBroadcaster::BroadcastMessage(fn.Env, fn.ThisPtr, eventName, fn.NArgs - 1, fn.Env->GetTopIndex() - 4);
    fn.Result->SetUndefined();
}

void GASAsBroadcasterProto::RemoveListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GASObjectInterface* plistener = fn.Arg(0).ToObjectInterface(fn.Env);
    fn.Result->SetBool(GASAsBroadcaster::RemoveListener(fn.Env, fn.ThisPtr, plistener));
}

static const GASNameFunction GAS_AsBcFunctionTable[] = 
{
    { "addListener",        &GASAsBroadcasterProto::AddListener        },
    { "broadcastMessage",   &GASAsBroadcasterProto::BroadcastMessage   },
    { "removeListener",     &GASAsBroadcasterProto::RemoveListener     },
    { 0, 0 }
};

GASAsBroadcasterProto::GASAsBroadcasterProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASAsBroadcaster>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_AsBcFunctionTable, pprototype);
}

void GASAsBroadcasterProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASAsBroadcaster> ab = *new GASAsBroadcaster(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}

//////////////////
const GASNameFunction GASAsBroadcasterCtorFunction::StaticFunctionTable[] = 
{
    { "initialize", &GASAsBroadcasterCtorFunction::Initialize },
    { 0, 0 }
};

GASAsBroadcasterCtorFunction::GASAsBroadcasterCtorFunction(GASStringContext *psc) :
    GASFunctionObject(GASAsBroadcasterProto::GlobalCtor)
{
    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue (StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASAsBroadcasterCtorFunction::Initialize (const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;
    GASObjectInterface* ptr = fn.Arg(0).ToObjectInterface(fn.Env);
    fn.Result->SetUndefined();
    GASAsBroadcaster::Initialize(fn.Env->GetSC(), ptr);
}
//////////////////
bool GASAsBroadcaster::Initialize(GASStringContext* psc, GASObjectInterface* pobj)
{
    InitializeProto(psc, pobj);
    return InitializeInstance(psc, pobj);
}

bool GASAsBroadcaster::InitializeProto(GASStringContext* psc, GASObjectInterface* pobj)
{
    if (!pobj)
        return false;

    for(int i = 0; GAS_AsBcFunctionTable[i].Name; i++)
    {
        pobj->SetConstMemberRaw(psc, GAS_AsBcFunctionTable[i].Name, GASValue (GAS_AsBcFunctionTable[i].Function), GASPropFlags::PropFlag_DontEnum);
    }
    return true;
}

bool GASAsBroadcaster::InitializeInstance(GASStringContext* psc, GASObjectInterface* pobj)
{
    if (!pobj)
        return false;

    GPtr<GASArrayObject> ao = *new GASArrayObject(psc);
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin__listeners), GASValue(ao), GASPropFlags::PropFlag_DontEnum);
    return true;
}

bool GASAsBroadcaster::AddListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener)
{
    if (!pthis || !plistener)
        return false;

    GASValue listenersVal;
    if (pthis->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin__listeners), &listenersVal))
    {
        GASObject* pobj = listenersVal.ToObject();
        if (pobj && pobj->GetObjectType() == Object_Array)
        {
            GPtr<GASArrayObject> parrObj = static_cast<GASArrayObject*>(pobj);

            for (UInt i = 0, n = parrObj->GetSize(); i < n; i++)
            {
                GASValue* pelem = parrObj->GetElementPtr(i);
                if (pelem && pelem->ToObjectInterface(penv) == plistener)
                {
                    // Already in the list.
                    return false;
                }
            }
            GASValue val;
            val.SetAsObjectInterface(plistener);
            parrObj->PushBack(val);
        }
    }
    return true;
}

bool GASAsBroadcaster::RemoveListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener)
{
    if (!pthis || !plistener)
        return false;

    GASValue listenersVal;
    if (pthis->GetMemberRaw(penv->GetSC(),
        penv->GetBuiltin(GASBuiltin__listeners), &listenersVal))
    {
        GASObject* pobj = listenersVal.ToObject();
        if (pobj && pobj->GetObjectType() == Object_Array)
        {
            GPtr<GASArrayObject> parrObj = static_cast<GASArrayObject*>(pobj);

            for (int i = (int)parrObj->GetSize() - 1; i >= 0; --i)
            {
                GASValue* pelem = parrObj->GetElementPtr(i);
                if (pelem && pelem->ToObjectInterface(penv) == plistener)
                {
                    // Already in the list.
                    parrObj->RemoveElements(i, 1);
                    return false;
                }
            }
        }
    }
    return true;
}

bool GASAsBroadcaster::BroadcastMessage
    (GASEnvironment* penv, GASObjectInterface* pthis, const GASString& eventName, int nArgs, int firstArgBottomIndex)
{
    if (!pthis)
        return false;

    GASValue listenersVal;
    if (pthis->GetMemberRaw(penv->GetSC(),
        penv->GetBuiltin(GASBuiltin__listeners), &listenersVal))
    {
        GASObject* pobj = listenersVal.ToObject();
        if (pobj && pobj->GetObjectType() == Object_Array)
        {
            GPtr<GASArrayObject> parrObj = static_cast<GASArrayObject*>(pobj);

            for (UInt i = 0, n = parrObj->GetSize(); i < n; i++)
            {
                GASValue*           pelem = parrObj->GetElementPtr(i);
                GASObjectInterface* plistener;

                if (pelem && (plistener = pelem->ToObjectInterface(penv)) != NULL)
                {
                    GPtr<GASObject> objHolder;
                    GPtr<GFxASCharacter> charHolder;
                    if (pelem->IsCharacter())
                        charHolder = pelem->ToASCharacter(penv);
                    else
                        objHolder = pelem->ToObject();

                    GASValue methodVal;
                    if (plistener->GetMemberRaw(penv->GetSC(),
                        eventName, &methodVal))
                    {
                        GASFunctionRef method = methodVal.ToFunction();
                        GASValue result;
                        method(GASFnCall(&result, plistener, penv, nArgs, firstArgBottomIndex));
                    }
                }
            }
        }
    }
    return true;
}

