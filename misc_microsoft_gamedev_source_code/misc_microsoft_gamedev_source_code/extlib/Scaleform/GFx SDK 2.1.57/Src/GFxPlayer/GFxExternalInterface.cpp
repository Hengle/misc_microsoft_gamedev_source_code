/**********************************************************************

Filename    :   GFxExternalInterface.cpp
Content     :   ExternalInterface AS class implementation
Created     :   
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxExternalInterface.h"
#include "GFxFunction.h"
#include "GFxValue.h"

#include <stdio.h>
#include <stdlib.h>

#define GFX_MAX_NUM_OF_VALUES_ON_STACK 10

GASExternalInterface::GASExternalInterface(GASEnvironment* penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_ExternalInterface));
}


static const GASNameFunction GFx_EIFunctionTable[] = 
{
    { 0, 0 }
};

GASExternalInterfaceProto::GASExternalInterfaceProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASExternalInterface>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GFx_EIFunctionTable, pprototype);
}

void GASExternalInterfaceProto::GlobalCtor(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

const GASNameFunction GASExternalInterfaceCtorFunction::StaticFunctionTable[] = 
{
    { "addCallback", &GASExternalInterfaceCtorFunction::AddCallback },
    { "call",        &GASExternalInterfaceCtorFunction::Call   },
    { 0, 0 }
};

GASExternalInterfaceCtorFunction::GASExternalInterfaceCtorFunction(GASStringContext *psc) : 
    GASFunctionObject(GASExternalInterfaceProto::GlobalCtor)
{ 
    SetConstMemberRaw(psc, "available", GASValue(GASValue::UNSET));

    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue (StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

bool    GASExternalInterfaceCtorFunction::GetMember(GASEnvironment *penv, const GASString& name, GASValue* val)
{
    if (penv->GetSC()->CompareConstString_CaseCheck(name, "available"))
    {
        // "available" returns "true" if external interface handler is set.
        GFxMovieRoot* proot = penv->GetMovieRoot();
        GASSERT(proot);

        val->SetBool((proot->pExtIntfHandler)?true:false);
        return true;
    }
    return GASFunctionObject::GetMember(penv, name, val);
}

void    GASExternalInterfaceCtorFunction::AddCallback(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    fn.LogScriptWarning("Warning: ExternalInterface.addCallback is not implemented yet.\n");
}

void    GASExternalInterfaceCtorFunction::Call(const GASFnCall& fn)
{
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GASSERT(proot);

    if (proot->pExtIntfHandler)
    {
        UInt nArgs = 0;
        GASString methodName(fn.Env->GetBuiltin(GASBuiltin_empty_));
        if (fn.NArgs >= 1)
        {
            methodName = fn.Arg(0).ToString(fn.Env);
            nArgs = fn.NArgs - 1;
        }

        // convert arguments into GFxValue, starting from index 1
        void* argArrayOnStack[GFX_MAX_NUM_OF_VALUES_ON_STACK*((sizeof(GFxValue)+sizeof(void*)-1)/sizeof(void*))];
        GFxValue* pargArray;
        if (nArgs > GFX_MAX_NUM_OF_VALUES_ON_STACK)
            pargArray = (GFxValue*)GALLOC(sizeof(GFxValue) * nArgs);
        else
            pargArray = (GFxValue*)argArrayOnStack;

        // convert params to GFxValue array
        proot->pRetValHolder->ResetPos();
        for (UInt i = 0, n = nArgs; i < n; ++i)
        {
            const GASValue& val = fn.Arg(i + 1);
            GFxValue* pdestVal = GTL::gconstruct<GFxValue>(&pargArray[i]);
            proot->ASValue2GFxValue(fn.Env, val, pdestVal);
        }
        
        proot->ExternalIntfRetVal.SetUndefined();

        proot->pExtIntfHandler->Callback
            (proot, (methodName.IsEmpty()) ? NULL : methodName.ToCStr(), pargArray, nArgs);

        *fn.Result = proot->ExternalIntfRetVal;

        // destruct elements in GFxValue array
        for (UInt i = 0, n = nArgs; i < n; ++i)
        {
            pargArray[i].~GFxValue();
        }
        if (nArgs > sizeof(argArrayOnStack)/sizeof(argArrayOnStack[0]))
        {
            GFREE(pargArray);
        }
    }
    else
    {
        fn.LogScriptWarning("Warning: ExternalInterface.call - handler is not installed.\n");
        fn.Result->SetUndefined();
    }
}


