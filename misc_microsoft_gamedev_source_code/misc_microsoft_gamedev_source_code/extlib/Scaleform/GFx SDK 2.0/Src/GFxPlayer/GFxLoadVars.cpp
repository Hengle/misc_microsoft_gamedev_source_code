/******************************************************************************

Filename    :   GFxLoadVars.cpp
Content     :   LoadVars reference class for ActionScript 2.0
Created     :   3/28/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

******************************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxAction.h"
#include "GFxNumber.h"
#include "GUTF8Util.h"
#include "GFxLoadVars.h"
#include "GFxAsBroadcaster.h"
#include "GFxPlayerImpl.h"

// ****************************************************************************
// GASLoadVars constructors
//
GASLoadVarsObject::GASLoadVarsObject(GASEnvironment *penv)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_LoadVars));
    GASAsBroadcaster::Initialize(penv->GetSC(), this);

    CommonInit();

    // @TODO
    // NOTE: The following call is REQUIRED.
    // It's causing memory leaks because of circular refs. Look for a fix
    GASAsBroadcaster::AddListener(penv, this, this);
    
    GASObject::SetMemberRaw(penv->GetSC(), penv->CreateConstString("contentType"), penv->CreateConstString("application/x-www-form-urlencoded"), GASPropFlags::PropFlag_DontEnum);
    GASObject::SetMemberRaw(penv->GetSC(), penv->CreateConstString("loaded"), GASValue(GASValue::UNDEFINED), GASPropFlags::PropFlag_DontEnum);
}

// ****************************************************************************
// Intercept to force enumeration for all properties set 
//
bool GASLoadVarsObject::SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val, const GASPropFlags& flags)
{
    GUNUSED(flags);
    GASObject::SetMemberFlags(penv->GetSC(), name, 0);
    return GASObject::SetMember(penv, name, val);
}

// ****************************************************************************
// Invoked when data has completely downloaded from the server or when an 
// error occurs while data is downloading from a server.
//
void GASLoadVarsObject::NotifyOnData(GASEnvironment* penv, GASString& src)
{
    penv->Push(src);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onData"), 1, penv->GetTopIndex());
    penv->Drop1();
}

// ****************************************************************************
// Invoked when Flash Player receives an HTTP status code from the server
//
void GASLoadVarsObject::NotifyOnHTTPStatus(GASEnvironment* penv, GASNumber httpStatus)
{
    penv->Push(httpStatus);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onHTTPStatus"), 1, penv->GetTopIndex());
    penv->Drop1();    
}

// ****************************************************************************
// Invoked when a LoadVars.load() or LoadVars.sendAndLoad() operation has ended
//
void GASLoadVarsObject::NotifyOnLoad(GASEnvironment* penv, bool success)
{
    penv->Push(success);
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onLoad"), 1, penv->GetTopIndex());
    penv->Drop1();    
}

// ****************************************************************************
// AS to GFx function mapping
//
const GASNameFunction GASLoadVarsProto::FunctionTable[] = 
{
    { "addRequestHeader", &GASLoadVarsProto::AddRequestHeader },
    { "decode", &GASLoadVarsProto::Decode },
    { "getBytesLoaded", &GASLoadVarsProto::GetBytesLoaded },
    { "getBytesTotal", &GASLoadVarsProto::GetBytesTotal },
    { "load", &GASLoadVarsProto::Load },
    { "send", &GASLoadVarsProto::Send },
    { "sendAndLoad", &GASLoadVarsProto::SendAndLoad },
    { "toString", &GASLoadVarsProto::ToString },
    { 0, 0 }
};

// ****************************************************************************
// GASLoadVars prototype constructor
//
GASLoadVarsProto::GASLoadVarsProto(GASStringContext *psc, GASObject *pprototype, const GASFunctionRef &constructor) : 
GASPrototype<GASLoadVarsObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, FunctionTable, pprototype);
    SetConstMemberRaw(psc, "onData", &GASLoadVarsProto::DefaultOnData, GASPropFlags::PropFlag_DontEnum);
}

// ****************************************************************************
// Called when the constructor is invoked for the LoadVars class (new LoadVars())
//
void GASLoadVarsProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASLoadVarsObject> ab = *new GASLoadVarsObject(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}

// ****************************************************************************
// LoadVars.addRequestHeader method
//
void GASLoadVarsProto::AddRequestHeader(const GASFnCall& fn)
{
    // Adds or changes HTTP request headers (such as Content-Type or 
    // SOAPAction) sent with POST actions. In the first usage, you pass two 
    // strings to the method: header and headerValue. In the second usage, you 
    // pass an array of strings, alternating header names and header values. 
    GUNUSED(fn);
    fn.GetLog()->LogScriptError("LoadVars.addRequestHeader is not implemented.");
}

// ****************************************************************************
// LoadVars.decode method
//
void GASLoadVarsProto::Decode(const GASFnCall& fn)
{
    if (fn.NArgs > 0)
    {
        GASLoadVarsObject* pthis = static_cast<GASLoadVarsObject*>(fn.ThisPtr);
        GASString urlStr(fn.Arg(0).ToString(fn.Env));
        GFxString data;
        GASGlobalContext::Unescape(urlStr.ToCStr(), urlStr.GetLength(), &data);
        LoadVariables(fn.Env, pthis, data);
    }
}

// ****************************************************************************
// LoadVars.getBytesLoaded method
//
void GASLoadVarsProto::GetBytesLoaded(const GASFnCall& fn)
{
    GASLoadVarsObject* pthis = static_cast<GASLoadVarsObject*>(fn.ThisPtr);
    if (pthis->BytesLoadedCurrent < 0)
    {
        fn.Result->SetUndefined();
    }
    else
    {
        fn.Result->SetNumber(pthis->BytesLoadedCurrent);
    }
}

// ****************************************************************************
// LoadVars.getBytesLoaded method
//
void GASLoadVarsProto::GetBytesTotal(const GASFnCall& fn)
{
    GASLoadVarsObject* pthis = static_cast<GASLoadVarsObject*>(fn.ThisPtr);
    if (pthis->BytesLoadedTotal < 0)
    {
        fn.Result->SetUndefined();
    }
    else
    {
        fn.Result->SetNumber(pthis->BytesLoadedTotal);
    }
}

// ****************************************************************************
// LoadVars.load method
//
void GASLoadVarsProto::Load(const GASFnCall& fn)
{
    if (fn.NArgs == 0)
    {
        fn.Result->SetBool(false);
        return;
    }
    GASLoadVarsObject* pthis = static_cast<GASLoadVarsObject*>(fn.ThisPtr);
    GASString urlStr(fn.Arg(0).ToString(fn.Env));
    
    pthis->BytesLoadedCurrent = 0;
    fn.Env->GetMovieRoot()->AddVarLoadQueueEntry(pthis, urlStr.ToCStr(), 
        GFxLoadQueueEntry::LM_None);
    fn.Result->SetBool(true);
}

// ****************************************************************************
// LoadVars.send method
//
void GASLoadVarsProto::Send(const GASFnCall& fn)
{
    GUNUSED(fn);
    fn.GetLog()->LogError("LoadVars.send is not implemented.");
}

// ****************************************************************************
// LoadVars.sendAndLoad method
//
void GASLoadVarsProto::SendAndLoad(const GASFnCall& fn)
{
    GUNUSED(fn);
    fn.GetLog()->LogError("LoadVars.sendAndLoad is not implemented.");
}

// ****************************************************************************
// LoadVars.toString method
//
void GASLoadVarsProto::ToString(const GASFnCall& fn)
{
    GASLoadVarsObject* pthis = static_cast<GASLoadVarsObject*>(fn.ThisPtr);
    GASSERT(pthis);

    GFxString str;
    struct MemberVisitor : GASObjectInterface::MemberVisitor
    {
        GASEnvironment* pEnvironment;
        GFxString* pString;

        MemberVisitor(GASEnvironment *penv, GFxString *str)
        {
            pEnvironment = penv;
            pString = str;
        }

        virtual void Visit(const GASString& name, const GASValue& val, UByte flags)
        {
            GUNUSED(flags);

            GFxString str, tmp;
            str = name.ToCStr();
			GASGlobalContext::Escape(str.ToCStr(), str.GetSize(), &tmp);
            *pString += tmp;
            *pString += "=";
            tmp.Clear();
            str = val.ToString(pEnvironment).ToCStr();
           // GASGlobalContext::Escape(str.ToCStr(), str.GetLength(), &tmp);
			GASGlobalContext::Escape(str.ToCStr(), str.GetSize(), &tmp);
            *pString += tmp;
            *pString += "&";
        }
    } visitor(fn.Env, &str);
    pthis->VisitMembers(fn.Env->GetSC(), &visitor, 0);
    str.Remove(str.GetLength()-1); //here the use of GetLength() seems appropriate
    fn.Result->SetString(fn.Env->CreateString(str));
}

// ****************************************************************************
// Default LoadVars.onData method
//
void GASLoadVarsProto::DefaultOnData(const GASFnCall& fn)
{
    // calls onLoad
    GASLoadVarsObject* pthis = static_cast<GASLoadVarsObject*>(fn.ThisPtr);
    GFxString str( fn.Arg(0).ToString(fn.Env).ToCStr() );
    pthis->NotifyOnLoad(fn.Env, LoadVariables(fn.Env, pthis, fn.Arg(0).ToString(fn.Env).ToCStr()));
}


// ****************************************************************************
// Helper function to load variables from an unescaped string
//
bool GASLoadVarsProto::LoadVariables(GASEnvironment *penv, GASObjectInterface* pchar, const GFxString& data)
{
    GFxString           name, value;

    if (data.GetLength() == 0)
        return false;

    // parser state machine
    Char c;
    bool parseName = true;

    for (UPInt i=0; i < data.GetSize(); i++)
    {
        c = data[i];    // read char
        if ('\r' == c)
        {
            // flash converts LF to CR, but console printing
            // required LFs
            c = '\n';
        }
        if ('&' == c)
        {
            pchar->SetMember(penv, penv->CreateString(name), penv->CreateString(value) );
            name.Clear();
            value.Clear();
            parseName = true;
        }
        else if (parseName)
        {
            if ('=' == c)
            {
                parseName = false;
            }
            else
            {
                name += c;
            }
        }
        else
        {
            value += c;
        }
    }
    // clean up stragglers
    if (name.GetLength() > 0)
    {
        pchar->SetMember(penv, penv->CreateString(name), penv->CreateString(value) );
    }    
    return true;
}
