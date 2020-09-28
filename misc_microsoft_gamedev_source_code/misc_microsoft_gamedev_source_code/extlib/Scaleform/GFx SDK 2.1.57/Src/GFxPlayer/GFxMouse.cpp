/**********************************************************************

Filename    :   GFxMouse.cpp
Content     :   Implementation of Mouse class
Created     :   November, 2006
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
#include "GFxString.h"
#include "GFxAction.h"
#include "GFxArray.h"
#include "GFxNumber.h"
#include "GFxSprite.h"
#include "GFxMouse.h"
#include "GUTF8Util.h"

GASMouse::GASMouse(GASEnvironment* penv)
{
    commonInit(penv);
}

void GASMouse::commonInit (GASEnvironment* penv)
{
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_Mouse));
}
    
//////////////////////////////////////////
//
static void GFx_MouseFuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GAS_MouseFunctionTable[] = 
{
    { "addListener",        &GFx_MouseFuncStub },
    { "removeListener",     &GFx_MouseFuncStub },
    { "show",               &GFx_MouseFuncStub },
    { "hide",               &GFx_MouseFuncStub },
    { 0, 0 }
};

GASMouseProto::GASMouseProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASMouse>(psc, pprototype, constructor)
{
    InitFunctionMembers (psc, GAS_MouseFunctionTable, pprototype);
}

void GASMouseProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASMouse> ab = *new GASMouse(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}

//////////////////
const GASNameFunction GASMouseCtorFunction::StaticFunctionTable[] = 
{
    { "addListener",        &GASMouseCtorFunction::AddListener        },
    { "removeListener",     &GASMouseCtorFunction::RemoveListener     },
    { "show",               &GASMouseCtorFunction::Show               },
    { "hide",               &GASMouseCtorFunction::Hide               },
    { 0, 0 }
};

GASMouseCtorFunction::GASMouseCtorFunction (GASStringContext *psc, GFxMovieRoot* proot) :
    GASFunctionObject(GASMouseProto::GlobalCtor)
{
    GASSERT(proot->pASMouseListener == 0); // shouldn't be set!
    proot->pASMouseListener = this;

    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue (StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
    SetCursorTypeFunc = GASValue(GASMouseCtorFunction::SetCursorType).ToFunction();
}

// Remove dead entries in the listeners list.  (Since
// we use WeakPtr's, listeners can disappear without
// notice.)
void    GASMouseCtorFunction::CleanupListeners()
{
    for (int i = (int)ListenerWeakRefs.size() - 1; i >= 0; i--)
    {
        if (ListenerWeakRefs[i] == NULL)
        {
            Listeners.remove(i);
            ListenerWeakRefs.remove(i);
        }
    }
}

void GASMouseCtorFunction::NotifyListeners(GASEnvironment *penv, GASBuiltinType eventName, const GASString* ptargetName, UInt button, int delta) const
{
    if (Listeners.size() > 0)
    {
        GTL::garray<GWeakPtr<GRefCountBaseImpl> >   listenerWeakRefs = ListenerWeakRefs;
        GTL::garray<GASObjectInterface*>            listeners = Listeners;

        // Notify listeners.
        UPInt i;
        UPInt n = listeners.size();
        for (i = 0; i < n; i++)
        {
            GPtr<GRefCountBaseImpl>  listernerRef = listenerWeakRefs[i];

            if (listernerRef)
            {
                GASValue            method;
                GASObjectInterface* listener = listeners[i];                    
                GASEnvironment *    penvtemp = penv;
                if (listener->IsASCharacter())
                    penvtemp = listener->ToASCharacter()->GetASEnvironment();
                GASStringContext *  psc = penvtemp->GetSC(); 
                if (listener
                    && listener->GetMemberRaw(psc, psc->GetBuiltin(eventName), &method))
                {
                    GASSERT(penvtemp);
                    bool noExtraParams = false;
                    if (button > 0 && (eventName == GASBuiltin_onMouseDown || eventName == GASBuiltin_onMouseUp))
                    {
                        // check for number of parameters for onMouseDown/onMouseUp handler functions;
                        // if more than zero - we can call them for right and middle buttons and we can
                        // pass extra parameters for the left button's handler.
                        GASFunctionRef mfref = method.ToFunction();
                        if (!mfref.IsNull())
                        {
                            if (mfref->GetNumArgs() <= 0)
                            {
                                if (button > 1)
                                    continue; // do not call handlers for right/middle buttons, since handler has 0 parameters
                                noExtraParams = true; // do not pass extra parameters in left button's handler
                            }
                        }
                        else
                            continue; // method is empty! (assertion?)
                    }
                    int nArgs = 0;
                    if (ptargetName && (eventName == GASBuiltin_onMouseWheel || !noExtraParams))
                    {
                        // push target for onMouseWheel and onMouseDown/Up, if allowed
                        penvtemp->Push(*ptargetName);
                        ++nArgs;
                    }
                    switch(eventName)
                    {
                    case GASBuiltin_onMouseWheel:
                        penvtemp->Push(delta);
                        ++nArgs;
                        break;
                    case GASBuiltin_onMouseDown:
                    case GASBuiltin_onMouseUp:
                        if (button && !noExtraParams)
                        {
                            penvtemp->Push(GASNumber(button));
                            ++nArgs;
                        }
                        break;

                    default:
                        break;
                    }
                    GAS_Invoke(method, NULL, listener, penvtemp, nArgs, penvtemp->GetTopIndex());
                    if (nArgs > 0)
                    {
                        penvtemp->Drop(nArgs);
                    }
                }
            }       
        }
    }
} 

void GASMouseCtorFunction::OnMouseMove(GASEnvironment *penv) const
{
    NotifyListeners(penv, GASBuiltin_onMouseMove);
}

void GASMouseCtorFunction::OnMouseDown(GASEnvironment *penv, UInt button, GFxASCharacter* ptarget) const
{
    if (ptarget)
    {
        GASString stringVal = ptarget->GetCharacterHandle()->GetNamePath();
        NotifyListeners(penv, GASBuiltin_onMouseDown, &stringVal, button);
    }
    else
        NotifyListeners(penv, GASBuiltin_onMouseDown, NULL, button);
}

void GASMouseCtorFunction::OnMouseUp(GASEnvironment *penv, UInt button, GFxASCharacter* ptarget) const
{
    if (ptarget)
    {
        GASString stringVal = ptarget->GetCharacterHandle()->GetNamePath();
        NotifyListeners(penv, GASBuiltin_onMouseUp, &stringVal, button);
    }
    else
        NotifyListeners(penv, GASBuiltin_onMouseUp, NULL, button);
}

void GASMouseCtorFunction::OnMouseWheel(GASEnvironment *penv, int sdelta, GFxASCharacter* ptarget) const
{
    if (ptarget)
    {
        GASString stringVal = ptarget->GetCharacterHandle()->GetNamePath();
        NotifyListeners(penv, GASBuiltin_onMouseWheel, &stringVal, 0, sdelta);
    }
    else
        NotifyListeners(penv, GASBuiltin_onMouseWheel, 0, 0, sdelta);
}

void    GASMouseCtorFunction::AddListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: Mouse.addListener needs one argument (the listener object)\n");
        return;
    }

    GASObject*          listenerObj     = fn.Arg(0).IsObject() ? fn.Arg(0).ToObject() : 0;
    GFxASCharacter*     listenerChar    = fn.Arg(0).ToASCharacter(fn.Env);
    GASObjectInterface* listener;
    GRefCountBaseImpl*  listenerRef;

    if (listenerObj)
    {
        listener    = listenerObj;
        listenerRef = listenerObj;  
    }
    else
    {
        listener    = listenerChar;
        listenerRef = listenerChar; 
    }

    if (listener == NULL)
    {
        fn.Env->LogScriptError("Error: Mouse.addListener passed a NULL object; ignored\n");
        return;
    }

    GASMouseCtorFunction* mo = static_cast<GASMouseCtorFunction*>(fn.ThisPtr);
    GASSERT(mo);

    mo->CleanupListeners();

    for (UPInt i = 0, n = mo->Listeners.size(); i < n; i++)
    {
        if (mo->Listeners[i] == listener)
        {
            // Already in the list.
            return;
        }
    }

    mo->Listeners.push_back(listener);
    mo->ListenerWeakRefs.push_back(listenerRef);
}

void    GASMouseCtorFunction::RemoveListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: Mouse.removeListener needs one Argument (the listener object)\n");
        return;
    }

    GASObject*          listenerObj     = fn.Arg(0).IsObject() ? fn.Arg(0).ToObject() : 0;
    GFxASCharacter*     listenerChar    = fn.Arg(0).ToASCharacter(fn.Env);
    GASObjectInterface* listener;
    GRefCountBaseImpl*  listenerRef;

    if (listenerObj)
    {
        listener    = listenerObj;
        listenerRef = listenerObj;  
    }
    else
    {
        listener    = listenerChar;
        listenerRef = listenerChar; 
    }

    if (listener == NULL)
    {
        fn.Env->LogScriptError("Error: Mouse.removeListener passed a NULL object; ignored\n");
        return;
    }

    GASMouseCtorFunction* mo = static_cast<GASMouseCtorFunction*>(fn.ThisPtr);
    GASSERT(mo);

    mo->CleanupListeners();

    for (SPInt i = (SPInt)mo->Listeners.size() - 1; i >= 0; i--)
    {
        if (mo->Listeners[i] == listener)
        {
            mo->Listeners.remove(i);
            mo->ListenerWeakRefs.remove(i);
        }
    }
}

void    GASMouseCtorFunction::Show(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    if (proot->pUserEventHandler)
        proot->pUserEventHandler->HandleEvent(proot, GFxEvent(GFxEvent::DoShowMouse));
    else
        fn.Env->LogScriptWarning("Warning: no user event handler interface is installed; Mouse.show failed.\n");
}

void    GASMouseCtorFunction::Hide(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    if (proot->pUserEventHandler)
        proot->pUserEventHandler->HandleEvent(proot, GFxEvent(GFxEvent::DoHideMouse));
    else
        fn.Env->LogScriptWarning("Warning: no user event handler interface is installed; Mouse.hide failed.\n");
}

void    GASMouseCtorFunction::SetCursorType(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    int cursorShapeId = GFxMouseCursorEvent::ARROW;
    if (fn.NArgs > 0)
        cursorShapeId = (int)fn.Arg(0).ToNumber(fn.Env);
    if (!SetCursorType(proot, cursorShapeId))
        fn.Env->LogScriptWarning("Warning: no user event handler interface is installed; Mouse.setCursorType failed.\n");
}

bool    GASMouseCtorFunction::SetCursorType(GFxMovieRoot* proot, UInt cursorType)
{
    GASSERT(proot);

    if (proot->pUserEventHandler)
    {
        proot->pUserEventHandler->HandleEvent(proot, GFxMouseCursorEvent(cursorType));
    }
    else
        return false;
    return true;
}

void    GASMouseCtorFunction::GetTopMostEntity(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxMovieRoot* proot = fn.Env->GetMovieRoot();
    GPointF mousePos;
    bool testAll = false;
    if (fn.NArgs <= 1)
    {
        // one parameter version: parameter is a boolean value, indicates
        // do we need to test all characters (param is true) or only ones with button event handlers (false).
        // Coordinates - current mouse cursor coordinates

        // no parameters version: takes current mouse coordinates, testing all.

        int x, y, buttons;          
        proot->GetMouseState(&x, &y, &buttons);
        mousePos.x = (Float)PixelsToTwips(x);
        mousePos.y = (Float)PixelsToTwips(y);
        if (fn.NArgs == 1)
            testAll = fn.Arg(0).ToBool(fn.Env);
        else
            testAll = true;
    }
    else if (fn.NArgs >= 2)
    {
        // two parameters version: _x and _y in _root coordinates
        Float x = (Float)PixelsToTwips(fn.Arg(0).ToNumber(fn.Env));
        Float y = (Float)PixelsToTwips(fn.Arg(1).ToNumber(fn.Env));
        if (fn.NArgs >= 3)
            testAll = fn.Arg(2).ToBool(fn.Env);
        else
            testAll = true;

        if (proot->pLevel0Movie)
        {
            // x and y vars are in _root coords. Need to transform them
            // into world coords
            GRenderer::Matrix m = proot->pLevel0Movie->GetWorldMatrix();
            GPointF a(x, y);
            m.Transform(&mousePos, a);
        }
        else
            return;
    }
    GFxASCharacter* ptopCh = proot->GetTopMostEntity(mousePos, testAll);
    if (ptopCh)
    {
        fn.Result->SetAsCharacter(ptopCh);
    }
}


bool    GASMouseCtorFunction::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    // Extensions are always case-insensitive
    if (name == penv->GetBuiltin(GASBuiltin_setCursorType))
    {
        if (penv->CheckExtensions())
        {
            SetCursorTypeFunc = val.ToFunction();
        }
    }
    return GASFunctionObject::SetMember(penv, name, val, flags);
}

bool    GASMouseCtorFunction::GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
{
    if (penv->CheckExtensions())
    {
        // Extensions are always case-insensitive
        if (name == penv->GetBuiltin(GASBuiltin_setCursorType))
        {
            pval->SetAsFunction(SetCursorTypeFunc);
            return true;
        }
        else if (name == penv->GetBuiltin(GASBuiltin_LEFT))
        {
            pval->SetNumber(Mouse_LEFT);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_RIGHT))
        {
            pval->SetNumber(Mouse_RIGHT);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_MIDDLE))
        {
            pval->SetNumber(Mouse_MIDDLE);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_ARROW))
        {
            pval->SetNumber(GFxMouseCursorEvent::ARROW);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_HAND))
        {
            pval->SetNumber(GFxMouseCursorEvent::HAND);
        }
        else if (name == penv->GetBuiltin(GASBuiltin_IBEAM))
        {
            pval->SetNumber(GFxMouseCursorEvent::IBEAM);
        }
        else if (name == "getTopMostEntity")
        {
            *pval = GASValue(GASMouseCtorFunction::GetTopMostEntity);
            return true;
        }
    }
    return GASObject::GetMember(penv, name, pval);
}
