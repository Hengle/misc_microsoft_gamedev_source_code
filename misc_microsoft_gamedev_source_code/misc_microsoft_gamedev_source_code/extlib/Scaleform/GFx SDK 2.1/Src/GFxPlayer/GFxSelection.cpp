/**********************************************************************

Filename    :   GFxSelection.cpp
Content     :   Implementation of Selection class
Created     :   February, 2007
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
#include "GFxString.h"
#include "GFxAction.h"
#include "GFxArray.h"
#include "GFxNumber.h"
#include "GFxSprite.h"
#include "GFxSelection.h"
#include "GFxText.h"
#include "GUTF8Util.h"
#include "GFxAsBroadcaster.h"

GASSelection::GASSelection(GASEnvironment* penv)
{
    commonInit(penv);
}

void GASSelection::commonInit (GASEnvironment* penv)
{
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_Selection));
}
    
//////////////////////////////////////////
//
static void GFx_FuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GAS_SelectionFunctionTable[] = 
{
    { "addListener",        &GFx_FuncStub },
    { "broadcastMessage",   &GFx_FuncStub },
    { "getBeginIndex",      &GFx_FuncStub },
    { "getCaretIndex",      &GFx_FuncStub },
    { "getEndIndex",        &GFx_FuncStub },
    { "getFocus",           &GFx_FuncStub },
    { "setFocus",           &GFx_FuncStub },
    { "setSelection",       &GFx_FuncStub },
    { "removeListener",     &GFx_FuncStub },
    { 0, 0 }
};

GASSelectionProto::GASSelectionProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASSelection>(psc, pprototype, constructor)
{
    InitFunctionMembers (psc, GAS_SelectionFunctionTable, pprototype);
}

void GASSelectionProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASSelection> ab = *new GASSelection(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}

//////////////////
const GASNameFunction GASSelectionCtorFunction::StaticFunctionTable[] = 
{
    { "getBeginIndex",  &GASSelectionCtorFunction::GetBeginIndex },
    { "getCaretIndex",  &GASSelectionCtorFunction::GetCaretIndex },
    { "getEndIndex",    &GASSelectionCtorFunction::GetEndIndex },
    { "getFocus",       &GASSelectionCtorFunction::GetFocus },
    { "setFocus",       &GASSelectionCtorFunction::SetFocus },
    { "setSelection",   &GASSelectionCtorFunction::SetSelection },
    { 0, 0 }
};

GASSelectionCtorFunction::GASSelectionCtorFunction(GASStringContext *psc) :
    GASFunctionObject(GASObjectProto::GlobalCtor)
{
    GASAsBroadcaster::Initialize(psc, this);

    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue(StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASSelectionCtorFunction::GetFocus(const GASFnCall& fn)
{
    fn.Result->SetNull();
    if (!fn.Env)
        return;

    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter();
    if (focusedChar)
    {
        fn.Result->SetString(focusedChar->GetCharacterHandle()->GetNamePath());
    }
}

void GASSelectionCtorFunction::SetFocus(const GASFnCall& fn)
{
    fn.Result->SetBool(false);
    if (fn.NArgs < 1 || !fn.Env)
        return;

    GPtr<GFxASCharacter> newFocus;
    if (fn.Arg(0).IsString())
    {
        // resolve path
        GASValue val;
        bool retVal = fn.Env->FindVariable(fn.Arg(0).ToString(fn.Env), &val);
        if (retVal && val.IsCharacter())
        {
            newFocus = val.ToASCharacter(fn.Env);
        }
    }
    else
        newFocus = fn.Arg(0).ToASCharacter(fn.Env);
    if (newFocus)
    {
        if (newFocus->IsFocusEnabled())
        {
            fn.Env->GetMovieRoot()->SetKeyboardFocusTo(newFocus);
            fn.Result->SetBool(true);
        }
    }
    else
    {
        // just remove the focus
        fn.Env->GetMovieRoot()->SetKeyboardFocusTo(NULL);
        fn.Result->SetBool(true);

    }
}

void GASSelectionCtorFunction::GetCaretIndex(const GASFnCall& fn)
{
    fn.Result->SetNumber(-1);
    if (!fn.Env)
        return;

    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter();
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());
        fn.Result->SetNumber((GASNumber)ptextField->GetCaretIndex());
    }
}

void GASSelectionCtorFunction::SetSelection(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (!fn.Env)
        return;

    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter();
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());

        ptextField->SetSelection(int(fn.Arg(0).ToNumber(fn.Env)), int(fn.Arg(1).ToNumber(fn.Env)));
    }
}

void GASSelectionCtorFunction::GetBeginIndex(const GASFnCall& fn)
{
    fn.Result->SetNumber(-1);
    if (!fn.Env)
        return;

    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter();
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());
        fn.Result->SetNumber((GASNumber)ptextField->GetBeginIndex());
    }
}

void GASSelectionCtorFunction::GetEndIndex(const GASFnCall& fn)
{
    fn.Result->SetNumber(-1);
    if (!fn.Env)
        return;

    GPtr<GFxASCharacter> focusedChar = fn.Env->GetMovieRoot()->GetFocusedCharacter();
    if (focusedChar && focusedChar->GetObjectType() == Object_TextField)
    {
        GASTextField* ptextField = static_cast<GASTextField*>(focusedChar.GetPtr());
        fn.Result->SetNumber((GASNumber)ptextField->GetEndIndex());
    }
}

void GASSelectionCtorFunction::CaptureFocus(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    bool capture;
    if (fn.NArgs >= 1)
        capture = fn.Arg(0).ToBool(fn.Env);
    else
        capture = true;
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GPtr<GFxASCharacter> focusedChar = proot->GetFocusedCharacter();
    if (!focusedChar)
    {
        // if no characters currently focused - simulate pressing of Tab
        proot->ActivateFocusCapture();
        focusedChar = proot->GetFocusedCharacter();
    }
    if (capture)
    {
        if (focusedChar && focusedChar->IsFocusEnabled())
        {
             proot->SetKeyboardFocusTo(focusedChar);
        }
    }
    else
    {
        proot->HideFocusRect();
    }
    if (focusedChar)
        fn.Result->SetAsCharacter(focusedChar);
}

bool GASSelectionCtorFunction::GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
{
    if (penv->CheckExtensions())
    {
        GFxMovieRoot* proot = penv->GetMovieRoot();
        // Extensions are always case-insensitive
        if (name == "captureFocus") 
        {
            *pval = GASValue(GASSelectionCtorFunction::CaptureFocus);
            return true;
        }
        else if (name == "disableFocusAutoRelease")
        {
            if (proot->DisableFocusAutoRelease.IsDefined())
                pval->SetBool(proot->DisableFocusAutoRelease.IsTrue());
            else
                pval->SetUndefined();
            return true;
        }
        else if (name == "alwaysEnableArrowKeys")
        {
            if (proot->AlwaysEnableFocusArrowKeys.IsDefined())
                pval->SetBool(proot->AlwaysEnableFocusArrowKeys.IsTrue());
            else
                pval->SetUndefined();
            return true;
        }
        else if (name == "disableFocusRolloverEvent")
        {
            if (proot->DisableFocusRolloverEvent.IsDefined())
                pval->SetBool(proot->DisableFocusRolloverEvent.IsTrue());
            else
                pval->SetUndefined();
            return true;
        }
    }
    return GASFunctionObject::GetMember(penv, name, pval);
}

bool GASSelectionCtorFunction::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    if (penv->CheckExtensions())
    {
        GFxMovieRoot* proot = penv->GetMovieRoot();
        if (name == "disableFocusAutoRelease")
        {
            proot->DisableFocusAutoRelease = val.ToBool(penv);
            return true;
        }
        else if (name == "alwaysEnableArrowKeys")
        {
            proot->AlwaysEnableFocusArrowKeys = val.ToBool(penv);
            return true;
        }
        else if (name == "disableFocusRolloverEvent")
        {
            proot->DisableFocusRolloverEvent = val.ToBool(penv);
            return true;
        }
    }
    return GASFunctionObject::SetMember(penv, name, val, flags);
}

void GASSelection::BroadcastOnSetFocus(GASEnvironment* penv, GFxASCharacter* pOldFocus, GFxASCharacter* pNewFocus)
{
    GASValue selectionCtorVal;
    if (penv->GetGC()->pGlobal->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_Selection), &selectionCtorVal))
    {
        GASObjectInterface* pselectionObj = selectionCtorVal.ToObject();
        if (pselectionObj)
        {
            if (pNewFocus)
                penv->Push(GASValue(pNewFocus));
            else
                penv->Push(GASValue::NULLTYPE);
            if (pOldFocus)
                penv->Push(GASValue(pOldFocus));
            else
                penv->Push(GASValue::NULLTYPE);
            GASAsBroadcaster::BroadcastMessage(penv, pselectionObj, penv->CreateConstString("onSetFocus"), 2, penv->GetTopIndex());
            penv->Drop(2);
        }
    }
}

void GASSelection::DoTransferFocus(const GASFnCall& fn)
{
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    
    GASSERT(proot && fn.NArgs == 2);
    GASSERT(fn.Arg(0).IsCharacter() || fn.Arg(0).IsNull());
    GFxFocusMovedType fmt = GFxFocusMovedType(int(fn.Arg(1).ToNumber(fn.Env)));

    proot->TransferFocus(fn.Arg(0).ToASCharacter(fn.Env), fmt);
}

void GASSelection::QueueSetFocus
    (GASEnvironment* penv, GFxASCharacter* pNewFocus, GFxFocusMovedType fmt)
{
    GASSERT(penv);

    GASValueArray params;
    if (pNewFocus)
        params.push_back(GASValue(pNewFocus));
    else
        params.push_back(GASValue::NULLTYPE);
    params.push_back(GASValue((int)fmt));
    penv->GetMovieRoot()->InsertEmptyAction()->SetAction(penv->GetMovieRoot()->GetLevelMovie(0), 
        GASSelection::DoTransferFocus, &params);
}
