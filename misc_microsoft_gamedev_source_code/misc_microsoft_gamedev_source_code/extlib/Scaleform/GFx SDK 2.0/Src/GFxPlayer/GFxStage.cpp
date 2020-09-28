/**********************************************************************

Filename    :   GFxStage.cpp
Content     :   Stage class implementation
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

#include "GFxStage.h"
#include "GFxFunction.h"
#include "GFxValue.h"
#include "GFxAsBroadcaster.h"

#include <stdio.h>
#include <stdlib.h>

GASStageObject::GASStageObject(GASEnvironment* penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Stage));
}


//////////////////////////////////////////
//
static void GFx_StageFuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GFx_StageFunctionTable[] = 
{
    { "addListener",        &GFx_StageFuncStub },
    { "removeListener",     &GFx_StageFuncStub },
    { "broadcastMessage",   &GFx_StageFuncStub },
    { 0, 0 }
};

GASStageProto::GASStageProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASStageObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GFx_StageFunctionTable, pprototype);
}

void GASStageProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASStageObject> ab = *new GASStageObject(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}

GASStageCtorFunction::GASStageCtorFunction(GASStringContext *psc, GFxMovieRoot* movieRoot) : 
    GASFunctionObject(GASStageProto::GlobalCtor), MovieRoot(movieRoot) 
{ 
    // Stage is a broadcaster
    GASAsBroadcaster::Initialize(psc, this);

    GASFunctionObject::SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_width), GASValue(GASValue::UNSET));
    GASFunctionObject::SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_height), GASValue(GASValue::UNSET));
    SetConstMemberRaw(psc, "scaleMode", GASValue(GASValue::UNSET));
    SetConstMemberRaw(psc, "align", GASValue(GASValue::UNSET));
    
    SetConstMemberRaw(psc, "showMenu", GASValue(true));
}

bool    GASStageCtorFunction::GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
{
    GPtr<GFxMovieRoot> movieRoot = MovieRoot;
    GASSERT(movieRoot);

    if (psc->GetBuiltin(GASBuiltin_width).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))        
    {
        val->SetInt(int(TwipsToPixels(movieRoot->VisibleFrameRect.Width())));
        return true;
    }
    else if (psc->GetBuiltin(GASBuiltin_height).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))
    {
        val->SetInt(int(TwipsToPixels(movieRoot->VisibleFrameRect.Height())));
        return true;
    }
    else if (psc->CompareConstString_CaseCheck(name, "scaleMode"))
    {
        const char* scaleMode; 
        switch(movieRoot->GetViewScaleMode())
        {
            case GFxMovieView::SM_NoScale:  scaleMode = "noScale"; break;
            case GFxMovieView::SM_ExactFit: scaleMode = "exactFit"; break;
            case GFxMovieView::SM_NoBorder: scaleMode = "noBorder"; break;
            default: scaleMode = "showAll";
        }
        val->SetString(psc->CreateConstString(scaleMode));
        return true;
    }
    else if (psc->CompareConstString_CaseCheck(name, "align"))
    {
        const char* align; 
        switch(movieRoot->GetViewAlignment())
        {
            case GFxMovieView::Align_TopCenter:     align = "T"; break;
            case GFxMovieView::Align_BottomCenter:  align = "B"; break;
            case GFxMovieView::Align_CenterLeft:    align = "L"; break;
            case GFxMovieView::Align_CenterRight:   align = "R"; break;
            case GFxMovieView::Align_TopLeft:       align = "LT"; break; // documented as TL, but returns LT
            case GFxMovieView::Align_TopRight:      align = "TR"; break;
            case GFxMovieView::Align_BottomLeft:    align = "LB"; break; // documented as BL, but returns LB
            case GFxMovieView::Align_BottomRight:   align = "RB"; break; // documented as BR, but returns RB
            default: align = "";
        }
        val->SetString(psc->CreateConstString(align));
        return true;
    }
    return GASFunctionObject::GetMemberRaw(psc, name, val);
}

bool    GASStageCtorFunction::SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    GPtr<GFxMovieRoot> pmovieRoot = MovieRoot;
    GASSERT(pmovieRoot);
    GASStringContext * const psc = penv->GetSC();

    if (psc->CompareConstString_CaseCheck(name, "scaleMode"))
    {   
        const GASString& scaleModeStr = val.ToString(penv);
        GFxMovieView::ScaleModeType scaleMode, prevScaleMode = pmovieRoot->GetViewScaleMode();

        if (psc->CompareConstString_CaseInsensitive(scaleModeStr, "noScale"))
            scaleMode = GFxMovieView::SM_NoScale;
        else if (psc->CompareConstString_CaseInsensitive(scaleModeStr, "exactFit"))
            scaleMode = GFxMovieView::SM_ExactFit;
        else if (psc->CompareConstString_CaseInsensitive(scaleModeStr, "noBorder"))
            scaleMode = GFxMovieView::SM_NoBorder;
        else
            scaleMode = GFxMovieView::SM_ShowAll;
        pmovieRoot->SetViewScaleMode(scaleMode);
        if (prevScaleMode != scaleMode && scaleMode == GFxMovieView::SM_NoScale)
        {
            // invoke onResize
            NotifyOnResize(penv);
        }
        return true;
    }
    else if (psc->CompareConstString_CaseCheck(name, "align"))
    {
        const GASString& alignStr = val.ToString(penv).ToUpper();
        GFxMovieView::AlignType align;
        UInt char1 = 0, char2 = 0;
        UInt len = alignStr.GetLength();
        if (len >= 1)
        {
            char1 = alignStr.GetCharAt(0);
            if (len >= 2)
                char2 = alignStr.GetCharAt(1);
        }
        if ((char1 == 'T' && char2 == 'L') || (char1 == 'L' && char2 == 'T')) // TL or LT
            align = GFxMovieView::Align_TopLeft;
        else if ((char1 == 'T' && char2 == 'R') || (char1 == 'R' && char2 == 'T')) // TR or RT
            align = GFxMovieView::Align_TopRight;
        else if ((char1 == 'B' && char2 == 'L') || (char1 == 'L' && char2 == 'B')) // BL or LB
            align = GFxMovieView::Align_BottomLeft;
        else if ((char1 == 'B' && char2 == 'R') || (char1 == 'R' && char2 == 'B')) // BR or RB
            align = GFxMovieView::Align_BottomRight;
        else if (char1 == 'T')
            align = GFxMovieView::Align_TopCenter;
        else if (char1 == 'B')
            align = GFxMovieView::Align_BottomCenter;
        else if (char1 == 'L')
            align = GFxMovieView::Align_CenterLeft;
        else if (char1 == 'R')
            align = GFxMovieView::Align_CenterRight;
        else 
            align = GFxMovieView::Align_Center;
        pmovieRoot->SetViewAlignment(align);
        return true;
    }
    return GASFunctionObject::SetMember(penv, name, val, flags);
}

bool    GASStageCtorFunction::SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    if (psc->GetBuiltin(GASBuiltin_width).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()) ||
        psc->GetBuiltin(GASBuiltin_height).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))
    {
        // read-only, do nothing
        return true;
    }
    return GASFunctionObject::SetMemberRaw(psc, name, val, flags);
}

void GASStageCtorFunction::NotifyOnResize(GASEnvironment* penv)
{
    GASAsBroadcaster::BroadcastMessage(penv, this, penv->CreateConstString("onResize"), 0, 0);
}

void GASStageCtorFunction::NotifyOnResize(const GASFnCall& fn)
{
    GASValue stageCtorVal;
    if (fn.Env->GetGC()->pGlobal->GetMemberRaw(fn.Env->GetSC(), fn.Env->GetBuiltin(GASBuiltin_Stage), &stageCtorVal))
    {
        GASObjectInterface* pstageObj = stageCtorVal.ToObject();
        if (pstageObj)
        {
            GASAsBroadcaster::BroadcastMessage(fn.Env, pstageObj, fn.Env->CreateConstString("onResize"), 0, 0);
        }
        else
            GASSERT(0);
    }
    else
        GASSERT(0);
}
