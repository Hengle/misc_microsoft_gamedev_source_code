/**********************************************************************

Filename    :   GFxBoolean.cpp
Content     :   Boolean object functinality
Created     :   April 10, 2006
Authors     :   
Notes       :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxBoolean.h"

#include <stdio.h>
#include <stdlib.h>


GASBooleanObject::GASBooleanObject(GASEnvironment* penv) : Value (0)
{
    CommonInit(penv);
}

GASBooleanObject::GASBooleanObject(GASEnvironment* penv, bool v) : Value (v)
{
    CommonInit(penv);
}

void GASBooleanObject::CommonInit(GASEnvironment* penv)
{ 
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Boolean));
}

const char* GASBooleanObject::GetTextValue(GASEnvironment*) const
{
    return (Value) ? "true" : "false";
}

bool GASBooleanObject::InvokePrimitiveMethod(const GASFnCall& fnCall, const GASString& methodName)
{
    GASBooleanObject* pthis = (GASBooleanObject*) fnCall.ThisPtr;
    GASSERT(pthis);

    // if (methodName == "toString" || methodName == "valueOf")
    if (fnCall.Env->GetBuiltin(GASBuiltin_toString).CompareBuiltIn_CaseCheck(methodName, fnCall.Env->IsCaseSensitive()) ||
        fnCall.Env->GetBuiltin(GASBuiltin_valueOf).CompareBuiltIn_CaseCheck(methodName, fnCall.Env->IsCaseSensitive()) )
    {
        GASValue method;
        if (pthis->GetMemberRaw(fnCall.Env->GetSC(), methodName, &method)) 
        {
            GASFunctionRef  func = method.ToFunction();
            if (!func.IsNull())
            {
                // It's a C function.  Call it.
                func(fnCall);
                return true;
            }
        }
    }
    fnCall.Result->SetUndefined();
    return false;
}

void        GASBooleanObject::SetValue(GASEnvironment* penv, const GASValue& v)
{
    Value = v.ToBool(penv);
}

GASValue    GASBooleanObject::GetValue() const
{
    return GASValue(Value);
}

////////////////////////
static void GAS_BooleanToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Boolean, "Boolean");
    GASBooleanObject* pthis = (GASBooleanObject*) fn.ThisPtr;
    GASSERT(pthis);

    fn.Result->SetString(pthis->GetValue().ToString(fn.Env));
}

static void GAS_BooleanValueOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Boolean, "Boolean");
    GASBooleanObject* pthis = (GASBooleanObject*) fn.ThisPtr;
    GASSERT(pthis);

    fn.Result->SetBool(pthis->GetValue().ToBool(fn.Env));
}


static const GASNameFunction GAS_BooleanFunctionTable[] = 
{
    { "toString",       &GAS_BooleanToString  },
    { "valueOf",        &GAS_BooleanValueOf  },
    { 0, 0 }
};

GASBooleanProto::GASBooleanProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASBooleanObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_BooleanFunctionTable, pprototype);
}

GASBooleanCtorFunction::GASBooleanCtorFunction(GASStringContext *psc) :
    GASFunctionObject(GlobalCtor)
{
    GUNUSED(psc);
}

void GASBooleanCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    if (fn.NArgs == 0)
    {
        fn.Result->SetBool(0);
    }
    else
    {
        fn.Result->SetBool(fn.Arg(0).ToBool(fn.Env));
    }
}

