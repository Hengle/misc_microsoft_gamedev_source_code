/**********************************************************************

Filename    :   GFxColor.cpp
Content     :   Color object functinality
Created     :   
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxColor.h"
#include "GFxFunction.h"

#include <stdio.h>
#include <stdlib.h>

// NOTE: SetTransform allows for negative color transforms (inverse mul/color subtract?).
// TODO: Such transforms probably won't work in renderer -> needs testing!

void GASColorProto::SetTransform(const GASFnCall& fn)
{
    GASColorObject* pthis = (GASColorObject*) fn.ThisPtr;       
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GFxASCharacter>    pc = pthis->pCharacter;
    if ((fn.NArgs < 1) || !pc)
        return;
    GASObjectInterface* pobj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (!pobj)
        return;

    // Get color transform, so that we can assign only
    // the fields available in the object. Other fields must
    // remain untouched.
    GRenderer::Cxform   c = pc->GetCxform();
    GASValue            val;
    GASStringContext*   psc = fn.Env->GetSC();
    
    // Multiply.
    if (pobj->GetConstMemberRaw(psc, "ba", &val))
        c.M_[2][0] = (Float) val.ToNumber(fn.Env) / 100.0f;
    if (pobj->GetConstMemberRaw(psc, "ga", &val))
        c.M_[1][0] = (Float) val.ToNumber(fn.Env) / 100.0f;
    if (pobj->GetConstMemberRaw(psc, "ra", &val))
        c.M_[0][0] = (Float) val.ToNumber(fn.Env) / 100.0f;
    if (pobj->GetConstMemberRaw(psc, "aa", &val))
        c.M_[3][0] = (Float) val.ToNumber(fn.Env) / 100.0f;
    // Add.
    if (pobj->GetConstMemberRaw(psc, "bb", &val))
        c.M_[2][1] = (Float) val.ToNumber(fn.Env);
    if (pobj->GetConstMemberRaw(psc, "gb", &val))
        c.M_[1][1] = (Float) val.ToNumber(fn.Env);
    if (pobj->GetConstMemberRaw(psc, "rb", &val))
        c.M_[0][1] = (Float) val.ToNumber(fn.Env);
    if (pobj->GetConstMemberRaw(psc, "ab", &val))
        c.M_[3][1] = (Float) val.ToNumber(fn.Env);

    pc->SetCxform(c);
    pc->SetAcceptAnimMoves(false);
}

void GASColorProto::GetTransform(const GASFnCall& fn)
{
    GASColorObject* pthis = (GASColorObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GFxASCharacter>    pc = pthis->pCharacter;
    if (!pc)
        return;
    GRenderer::Cxform c = pc->GetCxform();

    GPtr<GASObject>     pobj = *new GASObject;
    GASStringContext*   psc = fn.Env->GetSC();

    // Percentage: should be -100 to 100.
    pobj->SetConstMemberRaw(psc, "ba", c.M_[2][0] * 100.0f);
    pobj->SetConstMemberRaw(psc, "ga", c.M_[1][0] * 100.0f);
    pobj->SetConstMemberRaw(psc, "ra", c.M_[0][0] * 100.0f);
    pobj->SetConstMemberRaw(psc, "aa", c.M_[3][0] * 100.0f);
    // Add. Should be -255 to 255.
    pobj->SetConstMemberRaw(psc, "bb", c.M_[2][1]);
    pobj->SetConstMemberRaw(psc, "gb", c.M_[1][1]);
    pobj->SetConstMemberRaw(psc, "rb", c.M_[0][1]);
    pobj->SetConstMemberRaw(psc, "ab", c.M_[3][1]);

    fn.Result->SetAsObject(pobj.GetPtr());
}

void GASColorProto::SetRGB(const GASFnCall& fn)
{
    GASColorObject* pthis = (GASColorObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GFxASCharacter>    pc = pthis->pCharacter;
    if ((fn.NArgs < 1) || !pc)
        return;         
    UInt32  colorVal = (UInt32)fn.Arg(0).ToNumber(fn.Env);

    // Get the transform so that we can keep the Alpha unmodified.
    GRenderer::Cxform c = pc->GetCxform();
    // No multiply.
    c.M_[0][0] = c.M_[1][0] = c.M_[2][0] = 0;
    // Add.
    c.M_[0][1] = (Float) ((colorVal >> 16) & 0xFF); // R
    c.M_[1][1] = (Float) ((colorVal >> 8) & 0xFF);  // G
    c.M_[2][1] = (Float) (colorVal & 0xFF);         // B
    pc->SetCxform(c);
    pc->SetAcceptAnimMoves(false);
}

void GASColorProto::GetRGB(const GASFnCall& fn)
{
    GASColorObject* pthis = (GASColorObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GPtr<GFxASCharacter>    pc = pthis->pCharacter;
    if (!pc)
        return;         

    GRenderer::Cxform c = pc->GetCxform();
    UInt32  colorVal = 
        (((UInt)c.M_[2][1]) & 0xFF)         |
        ((((UInt)c.M_[1][1]) & 0xFF) << 8)  |
        ((((UInt)c.M_[0][1]) & 0xFF) << 16);

    fn.Result->SetUInt((UInt)colorVal);
}

static const GASNameFunction GAS_ColorFunctionTable[] = 
{
    { "setTransform",  &GASColorProto::SetTransform  },
    { "getTransform",  &GASColorProto::GetTransform  },
    { "setRGB",        &GASColorProto::SetRGB  },
    { "getRGB",        &GASColorProto::GetRGB  },
    { 0, 0 }
};

GASColorProto::GASColorProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASColorObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_ColorFunctionTable, pprototype);
}

void GASColorProto::GlobalCtor(const GASFnCall& fn)
{
    GFxASCharacter* ptarget = 0;
    if (fn.NArgs >= 1)
        ptarget = fn.Env->FindTargetByValue(fn.Arg(0));

    GPtr<GASColorObject> pcolor = *new GASColorObject(fn.Env, ptarget);
    fn.Result->SetAsObject(pcolor.GetPtr());
}

