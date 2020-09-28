/**********************************************************************

Filename    :   GFxObject.cpp
Content     :   ActionScript Object implementation classes
Created     :   
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxAction.h"
#include "GFxSprite.h"

// **** Object Interface implementation

GASObjectInterface::GASObjectInterface()
{

}

GASObjectInterface::~GASObjectInterface()
{

}

GFxASCharacter* GASObjectInterface::ToASCharacter()
{
    return IsASCharacter() ? static_cast<GFxASCharacter*>(this) : 0;
}

GASObject* GASObjectInterface::ToASObject()
{
    return IsASObject() ? static_cast<GASObject*>(this) : 0;
}

GFxSprite*      GASObjectInterface::ToSprite()
{
    return IsSprite() ? static_cast<GFxSprite*>(this) : 0;
}

const GFxASCharacter* GASObjectInterface::ToASCharacter() const 
{
    return IsASCharacter() ? static_cast<const GFxASCharacter*>(this) : 0;
}

const GASObject* GASObjectInterface::ToASObject() const
{
    return IsASObject() ? static_cast<const GASObject*>(this) : 0;
}


const GFxSprite*        GASObjectInterface::ToSprite() const 
{
    return IsSprite() ? static_cast<const GFxSprite*>(this) : 0;
}

GASFunctionRef  GASObjectInterface::ToFunction()
{
    return 0;
}

GASObject* GASObjectInterface::FindOwner(GASStringContext *psc, const GASString& name)
{
    for (GASObjectInterface* proto = this; proto != 0; proto = proto->Get__proto__())
    {
        if (proto->HasMember(psc, name, false)) 
            return static_cast<GASObject*>(proto);
    }
    return NULL;
}


//
// ***** GASObject Implementation
//
GASObject::GASObject(GASStringContext *psc) : pWatchpoints(0), ArePropertiesSet(false)
{
    // NOTE: psc is potentially NULL here.
    GUNUSED(psc);
    Init();
    pProto = 0;//*new GASObjectProto ();
}

GASObject::GASObject(GASStringContext *psc, GASObject* proto) : pWatchpoints(0), ArePropertiesSet(false)
{
    Init();
    Set__proto__(psc, proto);
}

GASObject::GASObject (GASEnvironment* penv) : pWatchpoints(0), ArePropertiesSet(false)
{
    Init();    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Object));
}

void GASObject::Init()
{
    IsListenerSet = false; // See comments in GFxValue.cpp, CheckForListenersMemLeak.
}

void GASObject::SetValue(GASEnvironment* penv, const GASValue& val)
{
    GUNUSED2(penv, val);
}

GASValue GASObject::GetValue() const
{
    return GASValue();
}

bool GASObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    static const GASValue notsetVal(GASValue::UNSET);
    GASPropFlags _flags;

    MemberHash::iterator it = Members.find_CaseCheck(name, penv->IsCaseSensitive());
    GASMember*           pmember;
    if (it == Members.end())
    {
        // Need to check whole prototypes' chain in the case if this is a property set into
        // the prototype.
        for (GASObject* pproto = Get__proto__(); pproto != 0; pproto = pproto->Get__proto__())
        {
            if (pproto->ArePropertiesSet)
            {
                MemberHash::iterator protoit = pproto->Members.find_CaseCheck(name, penv->IsCaseSensitive());
                if (protoit != pproto->Members.end())
                {
                    if (protoit->second.Value.IsProperty())
                        it = protoit;
                    break;
                }
            }
        }
    }
    if (it != Members.end())
    {
        pmember = &(*it).second;
        _flags = pmember->GetMemberFlags();
        // Is the member read-only ?
        if (_flags.GetReadOnly())
            return false;
        if (pmember->Value.IsProperty())
        {
            GASValue propVal = pmember->Value;
            // check, is the object 'this' came from GFxASCharacter. If so,
            // use original GFxASCharacter as this.
            GPtr<GFxASCharacter> paschar = GetASCharacter();
            propVal.SetPropertyValue(penv, (paschar)?(GASObjectInterface*)paschar.GetPtr():this, val);
            return true;
        }
    }
    else
    {
        _flags = flags;
        pmember = 0;
    }

    GASValue        newVal;
    const GASValue *pval = &val;

    if (penv && pWatchpoints) // have set any watchpoints?
    {
        if (InvokeWatchpoint(penv, name, val, &newVal))
        {
            pval = &newVal;
        }
    }

    if (!pmember)
    {
        return SetMemberRaw(penv->GetSC(), name, *pval, _flags);
    }
    else
    {
        // Check is the value property or not. If yes - set ArePropertiesSet flag to true
        if (val.IsProperty())
            ArePropertiesSet = true;

        if (penv->IsCaseSensitive())
        {
            if (name == penv->GetBuiltin(GASBuiltin___proto__))
            {   
                if (val.IsSet())
                    Set__proto__(penv->GetSC(), val.ToObject());
                pval = &notsetVal;
            }
            else if (name == penv->GetBuiltin(GASBuiltin___resolve))
            {   
                if (val.IsSet())
                    ResolveHandler = val.ToFunction();
                pval = &notsetVal;
            }
        }
        else
        {
            name.ResolveLowercase();
            if (penv->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                if (val.IsSet())
                    Set__proto__(penv->GetSC(), val.ToObject());
                pval = &notsetVal;
            }
            else if (penv->GetBuiltin(GASBuiltin___resolve).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                if (val.IsSet())
                    ResolveHandler = val.ToFunction();
                pval = &notsetVal;
            }
        }
        pmember->Value = *pval;
    }
    return true;
}

static const GASValue   notsetVal(GASValue::UNSET);

bool GASObject::SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    const GASValue*         pval = &val;
    MemberHash::iterator    it;

    // Hack for Tween class memory leak prevention. See comments in GFxValue.cpp, CheckForListenersMemLeak.
    // This is also necessary for MovieClipLoader since it adds itself as a listener.
    if (!IsListenerSet && val.IsObject() && 
        (name == psc->GetBuiltin(GASBuiltin__listeners)))
    {
        GASObject *pobject = val.ToObject();
        if (pobject && (pobject->GetObjectType() == Object_Array))
            IsListenerSet = true;
    }

    // For efficiency, do extra case-sensitivity branch.
    if (psc->IsCaseSensitive())
    {
        if (name == psc->GetBuiltin(GASBuiltin___proto__))
        {   
            if (val.IsSet())
                Set__proto__(psc, val.ToObject());
            pval = &notsetVal;
        }
        else if (name == psc->GetBuiltin(GASBuiltin___resolve))
        {   
            if (val.IsSet())
                ResolveHandler = val.ToFunction();
            pval = &notsetVal;
        }
        // Find member case-sensitively.
        it = Members.find(name);
    }
    else
    {
        name.ResolveLowercase();
        if (psc->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
        {   
            if (val.IsSet())
                Set__proto__(psc, val.ToObject());
            pval = &notsetVal;
        }
        else if (psc->GetBuiltin(GASBuiltin___resolve).CompareBuiltIn_CaseInsensitive_Unchecked(name))
        {   
            if (val.IsSet())
                ResolveHandler = val.ToFunction();
            pval = &notsetVal;
        }
        // Find member case-insensitively.
        it = Members.find_CaseInsensitive(name);
    }

    // Check is the value property or not. If yes - set ArePropertiesSet flag to true
    if (val.IsProperty())
        ArePropertiesSet = true;
    
    if (it != Members.end())
    {   
        // This matches correct Flash behavior for both case sensitive and insensitive versions.
        // I.E. for case insensitive strings the identifier name is not overwritten, retaining
        // the original capitalization it had before assignment.
        GASMember* pmember = &(*it).second;
        GASSERT(pmember);
        pmember->Value = *pval;
    }
    else
    {
        // Since object wasn't there, no need to check capitalization again.
        Members.set(name, GASMember(*pval, flags));
    }

    return true;
}

bool    GASObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    bool rv = GetMemberRaw(penv->GetSC(), name, val);
    if (rv && val->IsProperty())
    {
        // check, is the object 'this' came from GFxASCharacter. If so,
        // use original GFxASCharacter as this.
        GPtr<GFxASCharacter> paschar = GetASCharacter();
        val->GetPropertyValue(penv, (paschar)?(GASObjectInterface*)paschar.GetPtr():this, val);
    }
    // Treat __resolve here
    if (!rv && !ResolveHandler.IsNull())
    {
        penv->Push(name);
        GPtr<GFxASCharacter> paschar = GetASCharacter();
        ResolveHandler(GASFnCall(val, (paschar)?(GASObjectInterface*)paschar.GetPtr():this, penv, 1, penv->GetTopIndex()));
        penv->Drop1();
        rv = true;
    }
    return rv;
}

bool    GASObject::GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
{    
    /*
    int sz = sizeof(GTL::ghash_node<GASString, GASMember, GASStringHashFunctor>);
    int sz2 = sizeof(GASMember);
    int sz3 = sizeof(GASValue);
    int sz4 = sizeof(GASFunctionRefBase);

    printf("Size GASValue: = %d\n", sz3);
    */
    
    GASObject* current = this;

    // For efficiency, do extra case-sensitivity branch.
    if (psc->IsCaseSensitive())
    {
        while (current != NULL)
        {
            MemberHash::iterator im;
            if (name == psc->GetBuiltin(GASBuiltin___proto__))
            {   
                GASObject* pproto = current->Get__proto__();
                if (pproto) 
                    val->SetAsObject(pproto);
                else
                    val->SetUndefined();
                return true;
            }
            else if (name == psc->GetBuiltin(GASBuiltin___resolve))
            {   
                if (!current->ResolveHandler.IsNull())
                    val->SetAsFunction(current->ResolveHandler);
                else
                    val->SetUndefined();
                return true;
            }

            // Find member case-sensitively.
            im = current->Members.find(name);

            if (im != current->Members.end())
            {
                const GASValue& memval = im->second.GetMemberValue();
                // if member's value is unset and current is not equal to "this" (to avoid
                // indefinite recursion, then we need to call virtual method GetMemberRaw 
                // to get the value.
                if (!memval.IsSet() && current != this)
                    return current->GetMemberRaw(psc, name, val);
                *val = memval;
                return true;
            }

            current = current->Get__proto__();
        }
    }
    else
    {
        name.ResolveLowercase();
        GASString::NoCaseKey ikey(name);
        while (current != NULL)
        {
            if (psc->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                GASObject* pproto = current->Get__proto__();
                if (pproto) 
                    val->SetAsObject(pproto);
                else
                    val->SetUndefined();
                return true;
            }
            else if (psc->GetBuiltin(GASBuiltin___resolve).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {   
                if (!current->ResolveHandler.IsNull())
                    val->SetAsFunction(current->ResolveHandler);
                else
                    val->SetUndefined();
                return true;
            }

            // Find member case-insensitively.
            MemberHash::iterator im;

            im = current->Members.find_alt(ikey);

            if (im != current->Members.end())
            {
                const GASValue& memval = im->second.GetMemberValue();
                // if member's value is unset and current is not equal to "this" (to avoid
                // indefinite recursion, then we need to call virtual method GetMemberRaw 
                // to get the value.
                if (!memval.IsSet() && current != this)
                    return current->GetMemberRaw(psc, name, val);
                *val = memval;
                return true;
            }

            current = current->Get__proto__();
        }
    }
/*
    if (im != Members.end())
    {
        *val = im->second.GetMemberValue();
        return true;
    }
  */  
    // MA: Could optimize this a bit by unrolling recursion and thus avoiding
    // nested '__proto__' and '__resolve' checks.
    /*GASObject* pproto = Get__proto__();
    if (pproto)
        return pproto->GetMemberRaw(psc, name, val);*/
    return false;
}

bool    GASObject::FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)
{
    //printf("GET MEMBER: %s at %p for object %p\n", name.C_str(), val, this);
    GASSERT(pmember != NULL);
    return Members.get_CaseCheck(name, pmember, psc->IsCaseSensitive());
}

bool    GASObject::DeleteMember(GASStringContext *psc, const GASString& name)
{
    // Find member.
    MemberHash::const_iterator it = Members.find_CaseCheck(name, psc->IsCaseSensitive());
    // Member must exist and be available for deletion.
    if (it == Members.end())
        return false;
    if (it->second.GetMemberFlags().GetDontDelete())
        return false;
    Members.remove(name);
    return true;
}

bool    GASObject::SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)
{
    GASMember member;
    if (FindMember(psc, name, &member))
    {
        GASPropFlags f = member.GetMemberFlags();
        f.SetFlags(flags);
        member.SetMemberFlags(f);

        Members.set(name, member);
        return true;
    }
    return false;
}

void    GASObject::VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags) const
{
    MemberHash::const_iterator it = Members.begin();
    while(it != Members.end())
    {   
        const GASPropFlags& memberFlags = it->second.GetMemberFlags();
        if (!memberFlags.GetDontEnum() || (visitFlags & VisitMember_DontEnum))
        {
            const GASValue& value = it->second.GetMemberValue();
            if (value.IsSet())
                pvisitor->Visit(it->first, value, memberFlags.Flags);
            else
            {
                // If value is not set - get it by GetMember
                GASValue value;
                const_cast<GASObject*>(this)->GetMemberRaw(psc, it->first, &value);
                pvisitor->Visit(it->first, value, memberFlags.Flags);
            }
        }
        ++it;
    }

    if ((visitFlags & VisitMember_Prototype) && pProto)
        pProto->VisitMembers(psc, pvisitor, visitFlags);
}

bool    GASObject::HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)
{
    GASMember member;
    if (Members.get(name, &member))
    {
        return true;
    }
    else if (inclPrototypes && pProto)
    {
        return pProto->HasMember(psc, name, true);
    }
    return false;
}

bool GASObject::InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces) const
{
    for (const GASObject* proto = this; proto != 0; proto = const_cast<GASObject*>(proto)->Get__proto__())
    {
        if (inclInterfaces)
        {
            if (proto->DoesImplement(penv, prototype)) return true;
        }
        else
        {
            if (proto == prototype) return true;
        }
    }
    return false;
}

bool GASObject::Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData)
{
    Watchpoint wp;
    wp.Callback = callback;
    wp.UserData = userData;
    if (pWatchpoints == NULL)
    {
        pWatchpoints = new WatchpointHash;
    }
    // Need to do a case check on set too, to differentiate overwrite behavior. 
    pWatchpoints->set_CaseCheck(prop, wp, psc->IsCaseSensitive());
    return true;
}

bool GASObject::Unwatch(GASStringContext *psc, const GASString& prop)
{
    if (pWatchpoints != NULL)
    {
        if (pWatchpoints->get_CaseCheck(prop, psc->IsCaseSensitive()) != 0)
        {
            pWatchpoints->remove_CaseCheck(prop, psc->IsCaseSensitive());
            if (pWatchpoints->size() == 0)
            {
                delete pWatchpoints;
                pWatchpoints = 0;
            }
            return true;
        }
    }
    return false;
}

bool GASObject::InvokeWatchpoint(GASEnvironment* penv, const GASString& prop, const GASValue& newVal, GASValue* resultVal)
{
    GASValue oldVal;
    // if property doesn't exist at this point, the "oldVal" value will be "undefined"
    GetMember(penv, prop, &oldVal);

    GASSERT(resultVal);

    GASValue result;
    const Watchpoint* wp = pWatchpoints->get_CaseCheck(prop, penv->IsCaseSensitive());
    if (wp && penv && pWatchpoints)
    {
        penv->Push(wp->UserData);
        penv->Push(newVal);
        penv->Push(oldVal);
        penv->Push(GASValue(prop));
        
        // check, is the object 'this' came from GFxASCharacter. If so,
        // use original GFxASCharacter as this.
        GPtr<GFxASCharacter> paschar = GetASCharacter();

        wp->Callback(GASFnCall(&result, (paschar)?(GASObjectInterface*)paschar.GetPtr():this, penv, 4, penv->GetTopIndex()));
        penv->Drop(4);
        *resultVal = result;
        return true;
    }
    return false;
}

///////////////////////
static const GASNameFunction GAS_ObjectFunctionTable[] = 
{
    { "addProperty",            &GASObjectProto::AddProperty },
    { "hasOwnProperty",         &GASObjectProto::HasOwnProperty },
    { "watch",                  &GASObjectProto::Watch },
    { "unwatch",                &GASObjectProto::Unwatch },
    { "isPropertyEnumerable",   &GASObjectProto::IsPropertyEnumerable },
    { "isPrototypeOf",          &GASObjectProto::IsPrototypeOf },
    { "toString",               &GASObjectProto::ToString },
    { "valueOf",                &GASObjectProto::ValueOf },
    { 0, 0 }
};

GASObjectProto::GASObjectProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor)
    : GASPrototype<GASObject>(psc, pprototype, constructor) 
{
    InitFunctionMembers(psc, GAS_ObjectFunctionTable, pprototype);
}

GASObjectProto::GASObjectProto(GASStringContext *psc, GASObject* pprototype)
    : GASPrototype<GASObject>(psc, pprototype) 
{
    InitFunctionMembers(psc, GAS_ObjectFunctionTable, pprototype);
}

GASObjectProto::GASObjectProto(GASStringContext *psc, const GASFunctionRef& constructor)
    : GASPrototype<GASObject>(psc, constructor) 
{
    InitFunctionMembers(psc, GAS_ObjectFunctionTable); //?? should I add prototype?
}

void GASObjectProto::AddProperty(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 2)
    {
        GASString       propName(fn.Arg(0).ToString(fn.Env));
        GASFunctionRef  getter = fn.Arg(1).ToFunction();

        if (!getter.IsNull())
        {
            GASFunctionRef setter;
            if (fn.NArgs >= 3 && fn.Arg(2).IsFunction())
                setter = fn.Arg(2).ToFunction();
            GASValue val(getter, setter);
            fn.ThisPtr->SetMemberRaw(fn.Env->GetSC(), propName, val);
            fn.Result->SetBool(true);
        }
        else
            fn.Result->SetBool(false);
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::HasOwnProperty(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    fn.Result->SetBool(fn.ThisPtr->HasMember(fn.Env->GetSC(), fn.Arg(0).ToString(fn.Env), false));
}

void GASObjectProto::Watch(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 2)
    {        
        GASFunctionRef callback = fn.Arg(1).ToFunction();
        if (!callback.IsNull())
        {
            GASValue userData;
            if (fn.NArgs >= 3)
            {
                userData = fn.Arg(2);
            }
            fn.Result->SetBool(fn.ThisPtr->Watch(fn.Env->GetSC(), fn.Arg(0).ToString(fn.Env), callback, userData));
        }
        else
            fn.Result->SetBool(false);
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::Unwatch(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 1)
    {        
        fn.Result->SetBool(fn.ThisPtr->Unwatch(fn.Env->GetSC(), fn.Arg(0).ToString(fn.Env)));
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::IsPropertyEnumerable(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 1)
    {
        GASString prop(fn.Arg(0).ToString(fn.Env));
        bool rv = fn.ThisPtr->HasMember(fn.Env->GetSC(), prop, false);
        if (rv)
        {
            GASMember m;
            fn.ThisPtr->FindMember(fn.Env->GetSC(), prop, &m);
            if (m.GetMemberFlags().GetDontEnum())
                rv = false;
        }
        fn.Result->SetBool(rv);
    }
    else
        fn.Result->SetBool(false);
}

void GASObjectProto::IsPrototypeOf(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    if (fn.NArgs >= 1 && !fn.ThisPtr->IsASCharacter())
    {
        GASObject* pthis = (GASObject*)fn.ThisPtr;
        GASObjectInterface* iobj = fn.Arg(0).ToObjectInterface(fn.Env);
        if (iobj)
        {
            fn.Result->SetBool(iobj->InstanceOf(fn.Env, pthis, false));
            return;
        }
    }
    fn.Result->SetBool(false);
}

void GASObjectProto::ToString(const GASFnCall& fn)
{
    if (fn.ThisPtr->IsFunction()) // "[type Function]"
        fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_typeFunction_));
    else if (fn.ThisPtr->IsASCharacter())
        fn.Result->SetString(GASValue(static_cast<GFxASCharacter*>(fn.ThisPtr)).GetCharacterNamePath(fn.Env));
    else // "[object Object]"
        fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_objectObject_));
}

void GASObjectProto::ValueOf(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    fn.Result->SetAsObjectInterface(fn.ThisPtr);
}

// Constructor for ActionScript class Object.
void GASObjectProto::GlobalCtor(const GASFnCall& fn)
{
    if (fn.NArgs >= 1)
    {
        //!AB, special undocumented(?) case: if Object's ctor is invoked
        // with a parameter of type:
        // 1. number - it will create instance of Number;
        // 2. boolean - it will create instance of Boolean;
        // 3. array - it will create instance of Array;
        // 4. string - it will create instance of String;
        // 5. object - it will just return the same object.
        // "null" and "undefined" are ignored.
        const GASValue& arg0 = fn.Arg(0);
        GASValue res;
        if (arg0.IsNumber() || arg0.IsBoolean() || arg0.IsString())
        {
            res = fn.Env->PrimitiveToTempObject(0);
        }
        else if (arg0.IsObject() || arg0.IsCharacter())
        {
            res = arg0;
        }
        if (!res.IsUndefined())
        {
            *fn.Result = res;
            return;
        }
    }
    GPtr<GASObject> o = *new GASObject(fn.Env);

    // for some reasons Object always contains "constructor" property instead of __constructor__
    GASFunctionRef ctor = fn.Env->GetConstructor(GASBuiltin_Object);
    o->Set_constructor(fn.Env->GetSC(), ctor);

    fn.Result->SetAsObject(o);
}

//////////////////
const GASNameFunction GASObjectCtorFunction::StaticFunctionTable[] = 
{
    { "registerClass",       &GASObjectCtorFunction::RegisterClass },
    { 0, 0 }
};

GASObjectCtorFunction::GASObjectCtorFunction(GASStringContext *psc) :
    GASFunctionObject(GASObjectProto::GlobalCtor)
{
    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetMemberRaw(psc, psc->CreateConstString(StaticFunctionTable[i].Name),
                     GASValue(StaticFunctionTable[i].Function), 
                     GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete |
                     GASPropFlags::PropFlag_DontEnum);
    }
}

void GASObjectCtorFunction::RegisterClass(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.NArgs < 2)
    {
        fn.Env->LogScriptError ("Error: Too few parameters for Object.registerClass (%d)", fn.NArgs);   
        return;
    }

    GASGlobalContext* pgctxt = fn.Env->GetGC();
    GASString         classname(fn.Arg(0).ToString(fn.Env));
    GASSERT(pgctxt);
    if (fn.Arg(1).IsFunction())
    {
        // Registration
        GASFunctionRef func = fn.Arg(1).ToFunction();        
        fn.Result->SetBool(pgctxt->RegisterClass(fn.Env->GetSC(), classname, func));
    }
    else if (fn.Arg(1).IsNull())
    {
        // De-registration
        fn.Result->SetBool(pgctxt->UnregisterClass(fn.Env->GetSC(), classname));
    }
    else
    {
        GASString a1(fn.Arg(1).ToString(fn.Env));
        fn.Env->LogScriptError("Error: Second parameter of Object.registerClass(%s, %s) should be function or null\n", 
                               classname.ToCStr(), a1.ToCStr());
    }
}

