/**********************************************************************

Filename    :   GFxTimers.cpp
Content     :   
Created     :   August, 2006
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
#include "GFxAction.h"
#include "GFxPlayerImpl.h"
#include "GFxFontResource.h"
#include "GFxSprite.h"
#include "GFxTimers.h"
#include "GFxNumber.h"

GASIntervalTimer::GASIntervalTimer(const GASFunctionRef& function, GASStringContext* psc) :
    Function(function), MethodName(psc->GetBuiltin(GASBuiltin_empty_)), Interval(0), InvokeTime(0), Id(0), 
    Active(true), Timeout(false)
{
    // Note: ignore 
}

GASIntervalTimer::GASIntervalTimer(GASObject* object, const GASString& methodName) :
    Object(object), MethodName(methodName), Interval(0), InvokeTime(0), Id(0), Active(true), Timeout(false)
{
}

GASIntervalTimer::GASIntervalTimer(GFxASCharacter* character, const GASString& methodName) :
    Character(character), MethodName(methodName), Interval(0), InvokeTime(0), Id(0), Active(true), Timeout(false)
{
}

void GASIntervalTimer::Start(GFxMovieRoot* proot)
{
    GASSERT(proot);

    Float startTime = proot->GetTimer();
    InvokeTime = startTime + Interval*0.001f;
}

bool GASIntervalTimer::Invoke(GFxMovieRoot* proot, Float frameTime)
{
    GASSERT(proot);
    if (!Active) return false;

    Float currentTime = proot->GetTimer();
    bool retval = false;
    if (currentTime >= InvokeTime)
    {
        GASFunctionRef function;
        GASObjectInterface* thisPtr = 0;
        GPtr<GASObject> thisHolder;
        GPtr<GFxASCharacter> targetHolder;
        GASEnvironment* penv = 0;
        
        // invoke timer handle
        if (!Function.IsNull())
        {
            // just function
            function = Function;
        }
        else
        {
            thisHolder = GPtr<GASObject>(Object);
            if (thisHolder)
            {
                // object
                thisPtr = thisHolder;

            }
            else if ((targetHolder = GPtr<GFxASCharacter>(Character)))
            {
                // character
                thisPtr = targetHolder;
                penv = targetHolder->GetASEnvironment();
            }
            if (thisPtr)
            {
                GASValue methodVal;
                GASStringContext *psc = proot->GetLevelMovie(0)->GetASEnvironment()->GetSC();
                if (thisPtr->GetMemberRaw(psc, MethodName, &methodVal))
                {
                    function = methodVal.ToFunction();
                }
            }
        }

        if (!function.IsNull())
        {
            GASValue result;
            if (!penv)
            {
                // if env is not set (it might be set only if character is used) then
                // get parent's _levelN's one.
                if (LevelHandle)
                {
                    GPtr<GFxASCharacter> levelCh = LevelHandle->ResolveCharacter(proot);
                    penv = levelCh->GetASEnvironment();
                }
                if (!penv)
                {
                    // still no penv? Well, let get _level0 ones, but not
                    // sure it is correct... (AB)
                    penv = proot->GetLevelMovie(0)->GetASEnvironment();
                }
            }
            GASSERT(penv);

            int nArgs = (int)Params.size();
            if (nArgs > 0)
            {
                for (int i = nArgs - 1; i >= 0; --i)
                    penv->Push(Params[i]);
            }
            function(GASFnCall(&result, thisPtr, penv, nArgs, penv->GetTopIndex()));
            if (nArgs > 0)
            {
                penv->Drop(nArgs);
            }
        }

        if (Timeout)
        {
            // clear the timeout timer
            Active = false;
        }
        else
        {
            // set new invoke time
            Float interval = GetNextInterval(currentTime, frameTime);
            if (interval > 0)
                InvokeTime += interval;
            else
                InvokeTime = currentTime; //?
        }
        retval = true;
    }
    return retval;
}

Float GASIntervalTimer::GetNextInterval(Float currentTime, Float frameTime) const
{
    int interval; 
    if (Interval < frameTime*1000/10)
        interval = int(frameTime*1000.f/10.f);
    else
        interval = Interval;
    if (interval==0)
        return 0;
    return (int((currentTime - InvokeTime)*1000.f + interval)/interval)*interval*0.001f;
}

void GASIntervalTimer::Clear()
{
    Active = false;
}

void GASIntervalTimer::SetInterval(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs < 2)
        return;
    GASIntervalTimer* timer;
    int nextArg = 1;
    if (fn.Arg(0).IsFunction())
    {
        // function variant
        timer = new GASIntervalTimer(fn.Arg(0).ToFunction(), fn.Env->GetSC());
    }
    else if (fn.Arg(0).IsObject())
    {
        // object variant
        timer = new GASIntervalTimer(fn.Arg(0).ToObject(), fn.Arg(1).ToString(fn.Env));
        ++nextArg;
    }
    else if (fn.Arg(0).IsCharacter())
    {
        // character variant
        timer = new GASIntervalTimer(fn.Arg(0).ToASCharacter(fn.Env), fn.Arg(1).ToString(fn.Env));
        ++nextArg;
    }
    else
        return;
    
    if (fn.NArgs <= nextArg) // there is no timeout set
    {
        delete timer;
        return;
    }
    if (fn.Env->GetTarget())
    {
        timer->LevelHandle = fn.Env->GetTarget()->GetASRootMovie()->GetCharacterHandle();
    }
    timer->Interval = int(fn.Arg(nextArg++).ToNumber(fn.Env));
    for (int i = nextArg; i < fn.NArgs; ++i)
    {
        timer->Params.push_back(fn.Arg(i));
    }
    
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GASSERT(proot);

    int id = proot->AddIntervalTimer(timer);
    fn.Result->SetNumber((GASNumber)id);

    timer->Start(proot);
}

void GASIntervalTimer::ClearInterval(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
        return;

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GASSERT(proot);

    GASNumber x = fn.Arg(0).ToNumber(fn.Env);
    if (!GASNumberUtil::IsNaN(x))
        proot->ClearIntervalTimer(int(x));
}

void GASIntervalTimer::SetTimeout(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs < 2)
        return;
    GASIntervalTimer* timer;

    if (fn.Arg(0).IsFunction())
    {
        // function variant
        timer = new GASIntervalTimer(fn.Arg(0).ToFunction(), fn.Env->GetSC());
    }
    else
        return;

    if (fn.Env->GetTarget())
    {
        timer->LevelHandle = fn.Env->GetTarget()->GetASRootMovie()->GetCharacterHandle();
    }
    timer->Interval = int(fn.Arg(1).ToNumber(fn.Env));
    timer->Timeout  = true;
    for (int i = 2; i < fn.NArgs; ++i)
    {
        timer->Params.push_back(fn.Arg(i));
    }

    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GASSERT(proot);

    int id = proot->AddIntervalTimer(timer);
    fn.Result->SetNumber((GASNumber)id);

    timer->Start(proot);
}

void GASIntervalTimer::ClearTimeout(const GASFnCall& fn)
{
    ClearInterval(fn);
}

