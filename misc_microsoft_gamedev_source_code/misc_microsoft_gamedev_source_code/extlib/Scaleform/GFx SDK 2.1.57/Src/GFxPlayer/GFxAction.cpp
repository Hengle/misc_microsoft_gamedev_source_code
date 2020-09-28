/**********************************************************************

Filename    :   GFxAction.cpp
Content     :   ActionScript opcode execution engine core
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GFunctions.h"

#include "GFxAction.h"
#include "GFxFunction.h"
#include "GFxCharacter.h"
#include "GFxLoaderImpl.h"
#include "GFxLog.h"
#include "GFxStream.h"
#include "GFxRandom.h"

#include "GFxPlayerImpl.h"
#include "GFxSprite.h"
#include "GFxText.h"
#include "GFxColor.h"
#include "GFxTransform.h"
#include "GFxMatrix.h"
#include "GFxMovieClipLoader.h"
#include "GFxBitmapData.h"
#include "GFxPoint.h"
#include "GFxRectangle.h"
#include "GFxColorTransform.h"
#include "GFxStage.h"

#include "GFxASString.h"
#include "GFxStringObject.h"

#include "GFxArray.h"
#include "GFxNumber.h"
#include "GFxBoolean.h"

#include "GFxTimers.h"
//#include "GFxTextFormat.h"
#include "GFxSoundObject.h"
#include "GFxAsBroadcaster.h"
#include "GFxDate.h"
#include "GFxMouse.h"
#include "GFxExternalInterface.h"
#include "GFxSelection.h"
#include "GFxTextFormat.h"

#include "GFxLoadProcess.h"
#include "GFxIMEManager.h"

#include "GFxFontResource.h"

#include "GFxCapabilities.h"
#include "GFxLoadVars.h"

#ifdef GFC_XML_EXPOSED
#ifdef GFC_USE_XML
#include "GFxASXmlNode.h"
#include "GFxASXml.h"
#endif
#endif

#include <stdio.h>
#include <stdlib.h>


#ifdef GFC_OS_WIN32
#define snprintf _snprintf
#endif // GFC_OS_WIN32

#ifdef GFC_OS_WII
#pragma opt_usedef_mem_limit 250
#endif

// NOTES:
//
// Buttons
// On (press)                 onPress
// On (release)               onRelease
// On (releaseOutside)        onReleaseOutside
// On (rollOver)              onRollOver
// On (rollOut)               onRollOut
// On (dragOver)              onDragOver
// On (dragOut)               onDragOut
// On (keyPress"...")         onKeyDown, onKeyUp      <----- IMPORTANT
//
// Sprites
// OnClipEvent (load)         onLoad
// OnClipEvent (unload)       onUnload                Hm.
// OnClipEvent (enterFrame)   onEnterFrame
// OnClipEvent (mouseDown)    onMouseDown
// OnClipEvent (mouseUp)      onMouseUp
// OnClipEvent (mouseMove)    onMouseMove
// OnClipEvent (keyDown)      onKeyDown
// OnClipEvent (keyUp)        onKeyUp
// OnClipEvent (data)         onData

// Text fields have event handlers too!

GFxActionLogger::GFxActionLogger(GFxCharacter *ptarget, const GFxString& suffixStr)
{
    GFxMovieRoot *proot = ptarget->GetMovieRoot();
    VerboseAction = proot->VerboseAction;
    VerboseActionErrors = !proot->SuppressActionErrors;
    LogSuffix = (char *)suffixStr.ToCStr();

    // Check if it is the main movie
    if (suffixStr == proot->GetMovieDef()->GetFileURL())
        UseSuffix = proot->LogRootFilenames;
    else
        UseSuffix = proot->LogChildFilenames;

    pLog = proot->GetCachedLog();
    if(UseSuffix)
    {
        //find short filename
        if (!proot->LogLongFilenames)
        {
            for (SPInt i=suffixStr.GetLength() ; i>0; i--) 
            {
                if (LogSuffix[i]=='/' || LogSuffix[i]=='\\') 
                {
                    LogSuffix = LogSuffix+i+1;
                    break;
                }
            }
        }
    }
}

GINLINE void   GFxActionLogger::LogScriptMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
{
    if (UseSuffix)
    { 
        char     FmtBuffer[256];
        gfc_strcpy(FmtBuffer, 256, pfmt);
        if (FmtBuffer[strlen(FmtBuffer)-1]=='\n')
            FmtBuffer[strlen(FmtBuffer)-1]='\0';
        gfc_sprintf(FmtBuffer,256,"%s : %s\n", FmtBuffer, LogSuffix);
        pLog->LogMessageVarg(messageType,FmtBuffer,argList);
        //pLog->LogScriptError(" : %s\n",LogSuffix);
    }
    else
        pLog->LogMessageVarg(messageType,pfmt,argList);
}

void    GFxActionLogger::LogScriptError(const char* pfmt,...)
{   
    va_list argList; va_start(argList, pfmt);
    LogScriptMessageVarg(Log_ScriptError, pfmt, argList);
    va_end(argList);
}

void    GFxActionLogger::LogScriptWarning(const char* pfmt,...)
{    
    va_list argList; va_start(argList, pfmt);
    LogScriptMessageVarg(Log_ScriptWarning, pfmt, argList);
    va_end(argList);
}

void    GFxActionLogger::LogScriptMessage(const char* pfmt,...)
{  
    va_list argList; va_start(argList, pfmt);
    LogScriptMessageVarg(Log_ScriptMessage, pfmt, argList);
    va_end(argList);
}

GINLINE void    GFxActionLogger::LogDisasm(const unsigned char* instructionData)
{
#ifndef GFC_NO_FXPLAYER_VERBOSE_ACTION
    if (pLog && IsVerboseAction())
    {
        GFxDisasm da(pLog, GFxLog::Log_Action);
        da.LogDisasm(instructionData);
    }
#else
    GUNUSED(instructionData);
#endif
}
//
// Function/method dispatch.
//

// FirstArgBottomIndex is the stack index, from the bottom, of the first argument.
// Subsequent arguments are at *lower* indices.  E.G. if FirstArgBottomIndex = 7,
// then arg1 is at env->Bottom(7), arg2 is at env->Bottom(6), etc.
bool        GAS_Invoke(const GASValue& method,
                       GASValue* presult,
                       GASObjectInterface* pthis,
                       GASEnvironment* penv,
                       int nargs,
                       int firstArgBottomIndex)
{
    GASFunctionRef  func = method.ToFunction();
    if (presult) presult->SetUndefined();
    if (func != NULL)
    {
        func(GASFnCall(presult, pthis, penv, nargs, firstArgBottomIndex));
        return true;
    }
    else
    {           
        if (penv && penv->IsVerboseActionErrors())
            penv->LogScriptError("Error: Invoked method is not a function\n");
    }

    return false;
}

bool        GAS_Invoke(const GASValue& method,
                       GASValue* presult,
                       const GASValue& pthis,
                       GASEnvironment* penv,
                       int nargs,
                       int firstArgBottomIndex)
{
    GASFunctionRef  func = method.ToFunction();
    if (presult) presult->SetUndefined();
    if (func != NULL)
    {
        func(GASFnCall(presult, pthis, penv, nargs, firstArgBottomIndex));
        return true;
    }
    else
    {           
        if (penv && penv->IsVerboseActionErrors())
            penv->LogScriptError("Error: Invoked method is not a function\n");
    }

    return false;
}

// Printf-like vararg interface for calling ActionScript.
// Handy for external binding.
bool        GAS_InvokeParsed(const char* pmethodName,
                             GASValue* presult,
                             GASObjectInterface* pthis,
                             GASEnvironment* penv,
                             const char* pmethodArgFmt,
                             va_list args)
{
    if (!pmethodName || *pmethodName == '\0')
        return false;

    GFxASCharacter* pnewTarget  = 0;
    GASValue        method, owner;
    if (!penv->GetVariable(penv->CreateString(pmethodName), &method, NULL, &pnewTarget, &owner))
    {
        penv->LogScriptError("Error: Can't find method '%s' to invoke.\n", pmethodName);
        return false;
    }

    // Check method
    GASFunctionRef  func = method.ToFunction();
    if (func == NULL)
    {           
        penv->LogScriptError("Error: Invoked method '%s' is not a function\n", pmethodName);
        return false;
    }

    // Parse vaList args
    int  nargs = 0;
    if (pmethodArgFmt)
    {
        int startingIndex = penv->GetTopIndex();
        const char* p = pmethodArgFmt;
        for (;;)
        {
            char    c = *p++;
            if (c == 0)
            {
                // End of args.
                break;
            }
            else if (c == '%')
            {
                c = *p++;
                // Here's an arg.
                if (c == 'd')
                {
                    // Integer.
                    penv->Push(va_arg(args, int));
                }
                else if (c == 'u')
                {
                    // Undefined.
                    penv->Push(GASValue());
                }
                else if (c == 'n')
                {
                    // Null.
                    penv->Push(GASValue::NULLTYPE);
                }
                else if (c == 'b')
                {
                    // Boolean
                    penv->Push(bool(va_arg(args, int) != 0));
                }
                else if (c == 'f')
                {
                    // Double                   
                    penv->Push((GASNumber)va_arg(args, double));

                    // MA: What about float? C specification states that "float converts to
                    // double by the standard argument promotion", so this works. But,
                    // what happens on PS2?  Do we needs '%hf' specifier for float?
                }
                else if (c == 'h')
                {
                    c = *p++;
                    if (c == 'f')
                    {
                        // AB: %hf will be treated as double too, since C spec states
                        // that float is converted to double.
                        // Double                   
                        penv->Push((GASNumber)va_arg(args, double));
                        GFC_DEBUG_WARNING2(1, "InvokeParsed('%s','%s') - '%%hf' will be treated as double", 
                            pmethodName,
                            pmethodArgFmt);
                    }
                    else
                    {
                        penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid format '%%h%c'\n",
                            pmethodName,
                            pmethodArgFmt,
                            c);
                    }
                }
                else if (c == 's')
                {
                    // String                
                    penv->Push(penv->CreateString(va_arg(args, const char *)));
                }
                else if (c == 'l')
                {
                    c = *p++;
                    if (c == 's')
                    {
                        // Wide string.
                        penv->Push(penv->CreateString(va_arg(args, const wchar_t *)));
                    }
                    else
                    {
                        penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid format '%%l%c'\n",
                                                pmethodName,
                                                pmethodArgFmt,
                                                c);
                    }
                }
                else
                {
                    // Invalid fmt, warn.
                    penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid format '%%%c'\n",
                                pmethodName,
                                pmethodArgFmt,
                                c);
                }
            }
            else
            {
                // Invalid arg; warn.
                penv->LogScriptError("Error: InvokeParsed('%s','%s') - invalid char '%c'\n",
                                pmethodName,
                                pmethodArgFmt,
                                c);
            }
            // skip all whitespaces
            for (c = *p; c != '\0' && (c == ' ' || c == '\t' || c == ','); c = *++p) 
                ;
        }
        // Reverse the order of pushed args
        nargs  = penv->GetTopIndex() - startingIndex;
        for (int i = 0; i < (nargs >> 1); i++)
        {
            int i0 = startingIndex + 1 + i;
            int i1 = startingIndex + nargs - i;
            GASSERT(i0 < i1);

            GTL::gswap((penv->Bottom(i0)), (penv->Bottom(i1)));
        }
    }

    // Do the call.
    // MA: For C functions we need to specify a new target pthis here, if mehodName was nested.
    if (owner.IsCharacter() || owner.IsObject())
        pthis = owner.ToObjectInterface(penv);
    else if (pnewTarget)
        pthis = (GASObjectInterface*)pnewTarget;
    bool retVal = GAS_Invoke(method, presult, pthis, penv, nargs, penv->GetTopIndex());
    penv->Drop(nargs);

    return retVal;
}

// arguments should be ALREADY pushed into the penv's stack!
bool GAS_Invoke(const char* pmethodName,
                GASValue* presult,
                GASObjectInterface* pthis,
                GASEnvironment* penv,
                int numArgs,
                int firstArgBottomIndex)
{
    if (!pmethodName || *pmethodName == '\0')
        return false;

    GFxASCharacter* pnewTarget  = 0;
    GASValue        method, owner;
    if (!penv->GetVariable(penv->CreateString(pmethodName), &method, NULL, &pnewTarget, &owner))
    {
        penv->LogScriptError("Error: Can't find method '%s' to invoke.\n", pmethodName);
        return false;
    }

    // Check method
    GASFunctionRef  func = method.ToFunction();
    if (func == NULL)
    {           
        penv->LogScriptError("Error: Invoked method '%s' is not a function\n", pmethodName);
        return false;
    }

    // Do the call.
    // MA: For C functions we need to specify a new target pthis here, if mehodName was nested.
    if (owner.IsCharacter() || owner.IsObject())
        pthis = owner.ToObjectInterface(penv);
    else if (pnewTarget)
        pthis = (GASObjectInterface*)pnewTarget;
    return GAS_Invoke(method, presult, pthis, penv, numArgs, firstArgBottomIndex);
}

//
// Built-in objects
//


//
// math object
//


// One-argument simple functions.
#define MATH_WRAP_FUNC1(funcname)                                               \
void    GAS_Math_##funcname(const GASFnCall& fn)                                    \
    {                                                                           \
    GASNumber   arg = fn.Arg(0).ToNumber(fn.Env);                               \
    fn.Result->SetNumber((GASNumber)funcname(arg));                             \
}

MATH_WRAP_FUNC1(fabs);
MATH_WRAP_FUNC1(acos);
MATH_WRAP_FUNC1(asin);
MATH_WRAP_FUNC1(atan);
MATH_WRAP_FUNC1(ceil);
MATH_WRAP_FUNC1(cos);
MATH_WRAP_FUNC1(exp);
MATH_WRAP_FUNC1(floor);
MATH_WRAP_FUNC1(log);
MATH_WRAP_FUNC1(sin);
MATH_WRAP_FUNC1(sqrt);
MATH_WRAP_FUNC1(tan);


// Two-argument functions.
#define MATH_WRAP_FUNC2_EXP(funcname, expr)                                     \
void    GAS_Math_##funcname(const GASFnCall& fn)                                    \
    {                                                                           \
    GASNumber   arg0 = fn.Arg(0).ToNumber(fn.Env);                              \
    GASNumber   arg1 = fn.Arg(1).ToNumber(fn.Env);                              \
    fn.Result->SetNumber((GASNumber)expr);                                      \
}

MATH_WRAP_FUNC2_EXP(atan2, (atan2(arg0, arg1)));
MATH_WRAP_FUNC2_EXP(max, (arg0 > arg1 ? arg0 : arg1));
MATH_WRAP_FUNC2_EXP(min, (arg0 < arg1 ? arg0 : arg1));
MATH_WRAP_FUNC2_EXP(pow, (pow(arg0, arg1)));

// A couple of oddballs.
void    GAS_Math_random(const GASFnCall& fn)
{
    // Random number between 0 and 1.
#ifdef GFC_NO_DOUBLE
    fn.Result->SetNumber((GFxRandom::NextRandom() & 0x7FFFFF) / Float(UInt32(0x7FFFFF)));
#else
    fn.Result->SetNumber(GFxRandom::NextRandom() / double(UInt32(0xFFFFFFFF)));
#endif
}
void    GAS_Math_round(const GASFnCall& fn)
{
    // round argument to nearest int.
    GASNumber   arg0 = fn.Arg(0).ToNumber(fn.Env);
    fn.Result->SetNumber((GASNumber)floor(arg0 + 0.5f));
}

GASObject* GFxAction_CreateMathGlobalObject(GASStringContext *psc)
{
    // Create built-in math object.
    GASObject*  pmath = new GASObject;

    // constant
    pmath->SetConstMemberRaw(psc, "E",       (GASNumber)2.7182818284590452354    );
    pmath->SetConstMemberRaw(psc, "LN2",     (GASNumber)0.69314718055994530942   );
    pmath->SetConstMemberRaw(psc, "LOG2E",   (GASNumber)1.4426950408889634074    );
    pmath->SetConstMemberRaw(psc, "LN10",    (GASNumber)2.30258509299404568402   );
    pmath->SetConstMemberRaw(psc, "LOG10E",  (GASNumber)0.43429448190325182765   );
    pmath->SetConstMemberRaw(psc, "PI",      (GASNumber)3.14159265358979323846   );
    pmath->SetConstMemberRaw(psc, "SQRT1_2", (GASNumber)0.7071067811865475244    );
    pmath->SetConstMemberRaw(psc, "SQRT2",   (GASNumber)1.4142135623730950488    );

    // math methods
    pmath->SetConstMemberRaw(psc, "abs",      &GAS_Math_fabs);
    pmath->SetConstMemberRaw(psc, "acos",     &GAS_Math_acos);
    pmath->SetConstMemberRaw(psc, "asin",     &GAS_Math_asin);
    pmath->SetConstMemberRaw(psc, "atan",     &GAS_Math_atan);
    pmath->SetConstMemberRaw(psc, "ceil",     &GAS_Math_ceil);
    pmath->SetConstMemberRaw(psc, "cos",      &GAS_Math_cos);
    pmath->SetConstMemberRaw(psc, "exp",      &GAS_Math_exp);
    pmath->SetConstMemberRaw(psc, "floor",    &GAS_Math_floor);
    pmath->SetConstMemberRaw(psc, "log",      &GAS_Math_log);
    pmath->SetConstMemberRaw(psc, "random",   &GAS_Math_random);
    pmath->SetConstMemberRaw(psc, "round",    &GAS_Math_round);
    pmath->SetConstMemberRaw(psc, "sin",      &GAS_Math_sin);
    pmath->SetConstMemberRaw(psc, "sqrt",     &GAS_Math_sqrt);
    pmath->SetConstMemberRaw(psc, "tan",      &GAS_Math_tan);

    pmath->SetConstMemberRaw(psc, "atan2",    &GAS_Math_atan2);
    pmath->SetConstMemberRaw(psc, "max",      &GAS_Math_max);
    pmath->SetConstMemberRaw(psc, "min",      &GAS_Math_min);
    pmath->SetConstMemberRaw(psc, "pow",      &GAS_Math_pow);

    return pmath;       
}

/*
void EventTest(const GASFnCall& fn)
{
    GFC_DEBUG_WARNING(1, "FIXME: EventTest");
    } */

//
// key object
//


class GASKeyAsObject : public GASObject
{
protected:

    // Hack to store both WeakPtrs and correct interfaces.
    // If weak ptr is null, the interface pointer is unusable.
    // TBD: This should be an array of GASValues, or at least a union of { GASObject, GFxCharacterHandle }.
    GTL::garray<GWeakPtr<GRefCountBaseImpl> >   ListenerWeakRefs;
    GTL::garray<GASObjectInterface*>            Listeners;

    int                         LastKeyCode;
    UByte                       LastAsciiCode;
    UInt32                      LastWcharCode;
    GWeakPtr<GFxKeyboardState>  Keyboard;

    
    class KeyListener : public GFxKeyboardState::IListener  
    {
        GASKeyAsObject *pOwner;

    public:

        KeyListener(GASKeyAsObject *powner)
        { pOwner = powner; }

        virtual void OnKeyDown(GASStringContext *psc, int code, UByte ascii, UInt32 wcharCode) 
        {
            pOwner->NotifyListeners(psc, code, ascii, wcharCode, GFxEventId::Event_KeyDown);
        }       
        virtual void OnKeyUp(GASStringContext *psc, int code, UByte ascii, UInt32 wcharCode) 
        {
            pOwner->NotifyListeners(psc, code, ascii, wcharCode, GFxEventId::Event_KeyUp);
        }
        virtual void Update(int code, UByte ascii, UInt32 wcharCode)
        {
            pOwner->Update(code, ascii, wcharCode);
        }
    };

    GPtr<KeyListener>           pDelegateKeyListener;
    
public:

    GASKeyAsObject(GFxKeyboardState* key)
        : LastKeyCode(0), LastAsciiCode(0), LastWcharCode(0), Keyboard(key)
    {
        pDelegateKeyListener = *new KeyListener(this);
    }

    ObjectType          GetObjectType() const   { return Object_Key; }

    KeyListener* GetDelegateListener() const
    {
        return pDelegateKeyListener;
    }

    void Update(int code, UByte ascii, UInt32 wcharCode)
    {
        LastKeyCode = code;
        LastAsciiCode = ascii;
        LastWcharCode = wcharCode;
    }

    void NotifyListeners(GASStringContext *psc, int code, UByte ascii, UInt32 wcharCode, int eventId)
    {
        LastKeyCode = code;
        LastAsciiCode = ascii;
        LastWcharCode = wcharCode;

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
                    GASStringContext *  psctemp = psc;
                    if (listener->IsASCharacter())
                        psctemp = listener->ToASCharacter()->GetASEnvironment()->GetSC();

                    if (listener
                        && listener->GetMemberRaw(psctemp, GFxEventId(UByte(eventId)).GetFunctionName(psctemp), &method))
                    {
                        // MA: Warning: environment should no longer be null.
                        // May be ok because function will substitute its own environment.
                        GAS_Invoke(method, NULL, listener, NULL /* or root? */, 0, 0);
                    }
                }       
            }
        }
    } 

    bool    IsKeyDown(int code)
    {
        GPtr<GFxKeyboardState> Keyboard = this->Keyboard;
        if (Keyboard)
        {
            return Keyboard->IsKeyDown (code);
        }
        return false;
    }

    bool    IsKeyToggled(int code)
    {
        GPtr<GFxKeyboardState> Keyboard = this->Keyboard;
        if (Keyboard)
        {
            return Keyboard->IsKeyToggled (code);
        }
        return false;
    }

    // Remove dead entries in the listeners list.  (Since
    // we use WeakPtr's, listeners can disappear without
    // notice.)
    void    CleanupListeners()
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

    void    AddListener(GRefCountBaseImpl *pref, GASObjectInterface* listener)
    {
        CleanupListeners();

        for (UPInt i = 0, n = Listeners.size(); i < n; i++)
        {
            if (Listeners[i] == listener)
            {
                // Already in the list.
                return;
            }
        }

        Listeners.push_back(listener);
        ListenerWeakRefs.push_back(pref);
    }

    void    RemoveListener(GRefCountBaseImpl *pref, GASObjectInterface* listener)
    {
        GUNUSED(pref);

        CleanupListeners();

        for (int i = (int)Listeners.size() - 1; i >= 0; i--)
        {
            if (Listeners[i] == listener)
            {
                Listeners.remove(i);
                ListenerWeakRefs.remove(i);
            }
        }
    }

    // support for _listeners
    bool GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
    {
        if (penv && (name == penv->GetBuiltin(GASBuiltin__listeners)))
        {
            // Create a temporary GASArrayObject...
            GPtr<GASObject> obj = penv->OperatorNew(penv->GetBuiltin(GASBuiltin_Array));
            if (obj)
            {
                GASArrayObject* arrObj = static_cast<GASArrayObject*>(obj.GetPtr());

                // Iterate through listeners.
                UPInt i;
                UPInt n = Listeners.size();
                for (i = 0; i < n; i++)
                {
                    GPtr<GRefCountBaseImpl>  listernerRef = ListenerWeakRefs[i];

                    if (listernerRef)
                    {
                        GASObjectInterface* listener = Listeners[i];
                        GASValue v;
                        if (!listener->IsASCharacter())
                        {
                            GASObject* obj = static_cast<GASObject*>(listener);
                            v.SetAsObject(obj);
                        }
                        else
                        {
                            v.SetAsCharacter(listener->ToASCharacter());
                        }
                        arrObj->PushBack(v);
                    }       
                }

            }
            val->SetAsObject(obj);
        }
        return GASObject::GetMember(penv, name, val);
    }

    inline int GetLastKeyCode() const { return LastKeyCode; }
    inline int GetLastAsciiCode() const { return LastAsciiCode; }
    inline UInt32 GetLastWcharCode() const { return LastWcharCode; }

    static void KeyAddListener(const GASFnCall& fn);
    static void KeyGetAscii(const GASFnCall& fn);
    static void KeyGetCode(const GASFnCall& fn);
    static void KeyIsDown(const GASFnCall& fn);
    static void KeyIsToggled(const GASFnCall& fn);
    static void KeyRemoveListener(const GASFnCall& fn);

    static GASObject* CreateGlobalObject(GASStringContext *psc, GFxMovieRoot* root);
};


// Add a Listener (first arg is object reference) to our list.
// Listeners will have "onKeyDown" and "onKeyUp" methods
// called on them when a key changes state.
void    GASKeyAsObject::KeyAddListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyAddListener needs one Argument (the listener object)\n");
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
        fn.Env->LogScriptError("Error: KeyAddListener passed a NULL object; ignored\n");
        return;
    }

    GASKeyAsObject* ko = (GASKeyAsObject*)fn.ThisPtr;
    GASSERT(ko);

    ko->AddListener(listenerRef, listener);
}

// Return the ascii value of the last key pressed.
void    GASKeyAsObject::KeyGetAscii(const GASFnCall& fn)
{
    GASKeyAsObject* ko = (GASKeyAsObject*)fn.ThisPtr;
    GASSERT(ko);

    fn.Result->SetInt(ko->GetLastAsciiCode());
}

// Returns the keycode of the last key pressed.
void    GASKeyAsObject::KeyGetCode(const GASFnCall& fn)
{
    GASKeyAsObject* ko = (GASKeyAsObject*)fn.ThisPtr;
    GASSERT(ko);

    fn.Result->SetInt(ko->GetLastKeyCode());
}

// Return true if the Specified (first arg keycode) key is pressed.
void    GASKeyAsObject::KeyIsDown(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyIsDown needs one Argument (the key code)\n");
        return;
    }

    int code = fn.Arg(0).ToInt32(fn.Env);

    GASKeyAsObject* ko = (GASKeyAsObject*)fn.ThisPtr;
    GASSERT(ko);

    fn.Result->SetBool(ko->IsKeyDown(code));
}

// Given the keycode of NUM_LOCK or CAPSLOCK, returns true if
// the associated state is on.
void    GASKeyAsObject::KeyIsToggled(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyIsToggled needs one Argument (the key code)\n");
        return;
    }

    int code = fn.Arg(0).ToInt32(fn.Env);

    GASKeyAsObject* ko = (GASKeyAsObject*)fn.ThisPtr;
    GASSERT(ko);

    fn.Result->SetBool(ko->IsKeyToggled(code));
}

// Remove a previously-added listener.
void    GASKeyAsObject::KeyRemoveListener(const GASFnCall& fn)
{
    if (fn.NArgs < 1)
    {
        fn.Env->LogScriptError("Error: KeyRemoveListener needs one Argument (the listener object)\n");
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
        fn.Env->LogScriptError("Error: KeyRemoveListener passed a NULL object; ignored\n");
        return;
    }

    GASKeyAsObject* ko = (GASKeyAsObject*)fn.ThisPtr;
    GASSERT(ko);

    ko->RemoveListener(listenerRef, listener);
}


GASObject* GASKeyAsObject::CreateGlobalObject(GASStringContext *psc, GFxMovieRoot* root)   
{
    // Create built-in key object.
    GPtr<GFxKeyboardState> Keyboard = root->pKeyboardState;
    GASKeyAsObject* KeyObj = new GASKeyAsObject (Keyboard);

    // constants
#define KEY_CONST(kname, key) KeyObj->SetConstMemberRaw(psc, #kname, GFxKey::key)
    KEY_CONST(BACKSPACE, Backspace);
    KEY_CONST(CAPSLOCK, CapsLock);
    KEY_CONST(CONTROL, Control);
    KEY_CONST(DELETEKEY, Delete);
    KEY_CONST(DOWN, Down);
    KEY_CONST(END, End);
    KEY_CONST(ENTER, Return);
    KEY_CONST(ESCAPE, Escape);
    KEY_CONST(HOME, Home);
    KEY_CONST(INSERT, Insert);
    KEY_CONST(LEFT, Left);
    KEY_CONST(PGDN, PageDown);
    KEY_CONST(PGUP, PageUp);
    KEY_CONST(RIGHT, Right);
    KEY_CONST(SHIFT, Shift);
    KEY_CONST(SPACE, Space);
    KEY_CONST(TAB, Tab);
    KEY_CONST(UP, Up);

    // methods
    KeyObj->SetConstMemberRaw(psc, "addListener", &GASKeyAsObject::KeyAddListener);
    KeyObj->SetConstMemberRaw(psc, "getAscii", &GASKeyAsObject::KeyGetAscii);
    KeyObj->SetConstMemberRaw(psc, "getCode", &GASKeyAsObject::KeyGetCode);
    KeyObj->SetConstMemberRaw(psc, "isDown", &GASKeyAsObject::KeyIsDown);
    KeyObj->SetConstMemberRaw(psc, "isToggled", &GASKeyAsObject::KeyIsToggled);
    KeyObj->SetConstMemberRaw(psc, "removeListener", &GASKeyAsObject::KeyRemoveListener);

    Keyboard->AddListener (KeyObj->GetDelegateListener());

    return KeyObj;
}

//
// global init
//


static void    GAS_GlobalTrace(const GASFnCall& fn)
{
    GASSERT(fn.NArgs >= 1);

    // Special case for objects: try the ToString(env) method.
    GASObjectInterface* piobj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (piobj) // OBJECT or CHARACTER
    {
        GASValue method;
        if (piobj->GetMemberRaw(fn.Env->GetSC(),
                                fn.Env->GetBuiltin(GASBuiltin_toString), &method) &&
            method.IsFunction())
        {
            GASValue result;
            GAS_Invoke0(method, &result, piobj, fn.Env);
            
            // Should be just a message
            GASString traceStr(result.ToString(fn.Env));            
            fn.LogScriptMessage("%s\n", traceStr.ToCStr());
            return;
        }
    }

    // Log our argument.
    //
    // @@ what if we get extra args?
    GASString arg0 = fn.Arg(0).ToString(fn.Env);
    char    buffStr[2048];
    if (arg0.GetSize() < sizeof(buffStr))
    {
        // replace '\r' by '\n'
        gfc_strncpy(buffStr, sizeof(buffStr), arg0.ToCStr(), arg0.GetSize() + 1);
        for (char* p = buffStr; *p != 0; ++p)
        {
            if (*p == '\r')
                *p = '\n';
        }
        fn.LogScriptMessage("%s\n", buffStr);
    }
    else
        fn.LogScriptMessage("%s\n", arg0.ToCStr());
}

static void    GAS_GlobalParseInt(const GASFnCall& fn)
{       
    if (fn.NArgs < 1)
    {
        // return undefined
        return;
    }

    int         radix = 10;
    GASString   str(fn.Arg(0).ToString(fn.Env));
    UInt        len = str.GetSize();
    UInt        offset = 0;

    if (fn.NArgs >= 2)
    {
        radix = fn.Arg(1).ToInt32(fn.Env);
        if ((radix < 2) || (radix > 36))
        {
            // Should be NaN!!
            fn.Result->SetNumber(GASNumberUtil::NaN());
            return;
        }
    }
    else
    {
        // Need to skip whitespaces; but flash doesn't for HEX!
        //while ((str[offset] == ' ') && (offset<len))
        //  offset++;

        if (len > 1)
        {
            if (str[offset] == '0')
            {
                offset++;
                radix = 8;
                if (len > offset)
                {
                    if ((str[offset] == 'x') || (str[offset] == 'X'))
                    {
                        radix = 16;
                        offset++;
                    }
                }
            }
        }
    }
    
    // Convert to number.
    const char* pstart = str.ToCStr() + offset;
    char *      pstop  = 0;    
    long        result = strtol(str.ToCStr() + offset, &pstop, radix);

    // If text was not parsed or empty, return NaN.
    // If only "0x" is parsed - return NaN
    // If something like "0abc" or "0.2333" (looks like radix is 8) was parsed - return 0;
    if (pstart != pstop || radix == 8)
        fn.Result->SetInt(result);
    else
        fn.Result->SetNumber(GASNumberUtil::NaN());
}

static void    GAS_GlobalParseFloat(const GASFnCall& fn)
{       
    if (fn.NArgs < 1) // return undefined
        return;     
    // Convert.    
    GASString sa0(fn.Arg(0).ToString(fn.Env));
    const char* pstart = sa0.ToCStr();
    char *      pstop  = 0;
    GASNumber   result = (GASNumber)strtod(sa0.ToCStr(), &pstop);
    // If digits were consumed, report result.
    fn.Result->SetNumber((pstart != pstop) ? result : GASNumberUtil::NaN());
}

static void    GAS_GlobalIfFrameLoaded(const GASFnCall& fn)
{    
    if (fn.NArgs < 1) // return undefined
        return;     

    // Mark frame as not loaded initially.
    fn.Result->SetBool(false);

    GFxSprite* psprite = NULL;
    if (fn.ThisPtr == NULL)    
        psprite = (GFxSprite*) fn.Env->GetTarget();    
    else if (fn.ThisPtr->IsSprite())        
        psprite = (GFxSprite*) fn.ThisPtr;

    if (psprite)
    {
        // If frame is within range, mark it as loaded.
        SInt frame = fn.Arg(0).ToInt32(fn.Env);
        if (frame < (SInt)psprite->GetLoadingFrame())
            fn.Result->SetBool(true);
    }
}


static void    GAS_GlobalIsNaN(const GASFnCall& fn)
{       
    // isNaN() with no arguments should return true.
    if (fn.NArgs < 1)
        fn.Result->SetBool(true);
    else
        fn.Result->SetBool(GASNumberUtil::IsNaN(fn.Arg(0).ToNumber(fn.Env)));   
}

static void    GAS_GlobalIsFinite(const GASFnCall& fn)
{       
    // isFinite() with no arguments should return false.
    if (fn.NArgs < 1)
        fn.Result->SetBool(false);
    else    
    {
        GASNumber val = fn.Arg(0).ToNumber(fn.Env);

        if (GASNumberUtil::IsNaN(val) ||
            GASNumberUtil::IsNEGATIVE_INFINITY(val) ||
            GASNumberUtil::IsPOSITIVE_INFINITY(val))
            fn.Result->SetBool(false);
        else
            fn.Result->SetBool(true);
    }
}


// ASSetPropFlags function
static void    GAS_GlobalASSetPropFlags(const GASFnCall& fn)
{
    UInt version = fn.Env->GetVersion();
    
    // Check the arguments
    GASSERT(fn.NArgs == 3 || fn.NArgs == 4);
    GASSERT((version == 5) ? (fn.NArgs == 3) : true);
    
    GASObjectInterface* const obj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (!obj)
        return;
    
    // list of child names
    const GASValue& arg1 = fn.Arg(1);
    GPtr<GASArrayObject> props;
    
    if (arg1.IsString ())
    {
        // special case, if the properties are specified as a comma-separated string
        props = GASStringProto::StringSplit(fn.Env, arg1.ToString(fn.Env), ",");
    }
    else if (arg1.IsObject ()) 
    {
        GASObject* _props = fn.Arg(1).ToObject();
        if (_props == NULL)
        {
            GASSERT(fn.Arg(1).GetType() == GASValue::NULLTYPE);
        }
        else if (_props->GetObjectType () == GASObject::Object_Array)
            props = static_cast<GASArrayObject*>(_props);
        else if (_props->GetObjectType () == GASObject::Object_String)
            props = GASStringProto::StringSplit(fn.Env, arg1.ToString(fn.Env), ",");
        else
            return;
    }
    else if (!arg1.IsNull ())
        return;
    // a number which represents three bitwise flags which
    // are used to determine whether the list of child names should be hidden,
    // un-hidden, protected from over-write, un-protected from over-write,
    // protected from deletion and un-protected from deletion
    UByte setTrue = (UByte) (fn.Arg(2).ToInt32(fn.Env) & GASPropFlags::PropFlag_Mask);
    
    // Is another integer bitmask that works like setTrue,
    // except it sets the attributes to false. The
    // setFalse bitmask is applied before setTrue is applied
    
    // ASSetPropFlags was exposed in Flash 5, however the fourth argument 'setFalse'
    // was not required as it always defaulted to the value '~0'.
    UByte setFalse = (UByte) ((fn.NArgs == 3 ?
        (version == 5 ? ~0 : 0) : fn.Arg(3).ToUInt32(fn.Env))
        & GASPropFlags::PropFlag_Mask);

    GASStringContext* psc = fn.Env->GetSC();
    
    if (!props)
    {
        // Take all the members of the object
        struct MemberVisitor : GASObjectInterface::MemberVisitor
        {
            GASObjectInterface* obj;
            GASStringContext*   pSC;
            UByte               SetTrue, SetFalse;

            MemberVisitor(GASStringContext* psc, GASObjectInterface* _obj, UByte setTrue, UByte setFalse)
                : obj(_obj), pSC(psc), SetTrue(setTrue), SetFalse(setFalse) {}

            virtual void Visit(const GASString& name, const GASValue&, UByte flags)
            {
                GASPropFlags fl(flags);
                fl.SetFlags(SetTrue, SetFalse);
                obj->SetMemberFlags (pSC, name, fl.Flags);
            }
        } visitor (psc, obj, setTrue, setFalse);

        obj->VisitMembers(psc, &visitor, GASObject::VisitMember_DontEnum);
    }
    else 
    {
        for (int i = 0, n = props->GetSize(); i < n; ++i)
        {
            const GASValue* elem = props->GetElementPtr(i);
            if (elem == 0)
                continue;

            GASString key = elem->ToString(fn.Env);
            GASMember member;
            if (obj->FindMember(psc, key, &member))
            {
                GASPropFlags fl = member.GetMemberFlags();
                fl.SetFlags(setTrue, setFalse);

                obj->SetMemberFlags(psc, key, fl.Flags);
            }
        }
    }
}

// ASnative, class 800, function 2 - returns mouse button state
static void    GAS_ASnativeMouseButtonStates(const GASFnCall& fn)
{
    if (fn.NArgs >= 1)
    {
        UInt mask = fn.Arg(0).ToUInt32(fn.Env);
        fn.Result->SetBool((fn.Env->GetMovieRoot()->MouseButtons & mask) == mask);
    }
}

// undocumented function ASnative. 
// Now, we support only class 800, function 2 - returns mouse button state
static void    GAS_GlobalASnative(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs >= 2)
    {
        UInt classId = (UInt)fn.Arg(0).ToUInt32(fn.Env);
        UInt funcId  = (UInt)fn.Arg(1).ToUInt32(fn.Env);
        if (classId == 800 && funcId == 2)
        {
            GPtr<GASFunctionObject> func = *new GASFunctionObject(GAS_ASnativeMouseButtonStates);
            fn.Result->SetAsFunction(GASFunctionRef(func));
        }
    }
}

void GASGlobalContext::EscapeWithMask(const char* psrc, UPInt length, GFxString* pescapedStr, const UInt* escapeMask)
{   
    GASSERT(pescapedStr);

    char            buf[100];
    char*           pbuf = buf;
    char* const     endp = pbuf + sizeof (buf) - 1;    

    for (UInt i = 0; i < length; ++i)
    {
        int ch = (unsigned char)psrc[i];
        if (pbuf + 4 >= endp)
        {
            // flush
            *pbuf = '\0';
            *pescapedStr += buf;
            pbuf = buf;
        }
        if (ch < 128 && (escapeMask[ch/32]&(1u<<(ch%32)))) // isalnum (ch)
        {
            *pbuf++ = char(ch);
        }
        else 
        {
            *pbuf++ = '%';
            *pbuf++ = char(ch/16 + ((ch/16 <= 9) ? '0' : 'A'-10));
            *pbuf++ = char(ch%16 + ((ch%16 <= 9) ? '0' : 'A'-10));
        }
    }
    // flush
    *pbuf = '\0';
    *pescapedStr += buf;
}

void GASGlobalContext::Escape(const char* psrc, UPInt length, GFxString* pescapedStr)
{
    // each bit in mask represent the condition "isalnum(char) == 1"
    // mask positioning is as follows: mask[char/32]&(1<<(char%32))
    static const UInt mask[] = { 0x00000000, 0x03FF0000, 0x07FFFFFE, 0x07FFFFFE };
    EscapeWithMask(psrc, length, pescapedStr, mask);
}

void GASGlobalContext::EscapePath(const char* psrc, UPInt length, GFxString* pescapedStr)
{
    // each bit in mask represent the condition "isalnum(char) || char == '\\' || char == '/' || char == '.' || char == ':'"
    // mask positioning is as follows: mask[char/32]&(1<<(char%32))
    static const UInt mask[] = { 0x00000000, 0x07FFC000, 0x17FFFFFE, 0x07FFFFFE };
    EscapeWithMask(psrc, length, pescapedStr, mask);
}

void GASGlobalContext::Unescape(const char* psrc, UPInt length, GFxString* punescapedStr)
{   
    GASSERT(punescapedStr);

    char                buf[100];
    const char* const   endcstr = psrc + length;
    char*               pbuf = buf;
    char* const         endp = pbuf + sizeof (buf) - 1;

    while (psrc < endcstr)
    {
        int ch = (unsigned char) *psrc++;
        if (pbuf + 1 >= endp)
        {
            // flush
            *pbuf = '\0';
            *punescapedStr += buf;
            pbuf = buf;
        }
        if (ch == '%')
        {
            int fd = toupper((unsigned char)*psrc++);
            int sd = toupper((unsigned char)*psrc++);
            fd = (fd - '0' > 9) ? fd - 'A' + 10 : fd - '0';
            sd = (sd - '0' > 9) ? sd - 'A' + 10 : sd - '0';
            if (fd < 16 && sd < 16)
            {
                *pbuf++ = char(fd*16 + sd);
            }
        }
        else 
        {
            *pbuf++ = char(ch);
        }
    }
    // flush
    *pbuf = '\0';
    *punescapedStr += buf;
}

static void GAS_GlobalEscape(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GFxString escapedStr;
        GASGlobalContext::Escape(str.ToCStr(), str.GetSize(), &escapedStr);
        fn.Result->SetString(fn.Env->CreateString(escapedStr));
    }
}

static void GAS_GlobalUnescape(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GFxString unescapedStr;
        GASGlobalContext::Unescape(str.ToCStr(), str.GetSize(), &unescapedStr);
        fn.Result->SetString(fn.Env->CreateString(unescapedStr));
    }
}


static void GAS_GlobalEscapeSpecialHTML(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GFxString escapedStr;
        GFxString::EscapeSpecialHTML(str.ToCStr(), str.GetLength(), &escapedStr);
        fn.Result->SetString(fn.Env->CreateString(escapedStr));
    }
}

static void GAS_GlobalUnescapeSpecialHTML(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    if (fn.NArgs == 1)
    {
        GASString str(fn.Arg(0).ToString(fn.Env));
        GFxString unescapedStr;
        GFxString::UnescapeSpecialHTML(str.ToCStr(), str.GetLength(), &unescapedStr);
        fn.Result->SetString(fn.Env->CreateString(unescapedStr));
    }
}


static void GAS_GlobalUpdateAfterEvent(const GASFnCall& fn)
{
    GUNUSED(fn);
    // do nothing!
}

#ifndef GFC_NO_IME_SUPPORT
static void GAS_GlobalIMECommand(const GASFnCall& fn)
{   
    if (fn.NArgs >= 2)
    {
        GFxASCharacter* const poriginalTarget   = fn.Env->GetTarget();   
        GFxMovieRoot* proot                     = poriginalTarget->GetMovieRoot();
        GPtr<GFxIMEManager> pimeMgr             = proot->GetIMEManager();
        if (pimeMgr)
        {
            pimeMgr->IMECommand(proot, fn.Arg(0).ToString(fn.Env).ToCStr(), fn.Arg(1).ToString(fn.Env).ToCStr());
        }
    }
}

static void GAS_SetIMECandidateListStyle(const GASFnCall& fn)
{     

    if (fn.NArgs >= 1)
    {
        GFxASCharacter* const poriginalTarget   = fn.Env->GetTarget();   
        GFxMovieRoot* proot                     = poriginalTarget->GetMovieRoot();
        GPtr<GFxIMEManager> pimeMgr             = proot->GetIMEManager();
        if (pimeMgr)
        {
            GPtr<GASObject> pobj = fn.Arg(0).ToObject();
            if (pobj)
            {
                GFxIMECandidateListStyle st;
                GASValue val;
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("textColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetTextColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("backgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("indexBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetIndexBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("selectedTextColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetSelectedTextColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("selectedTextBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetSelectedBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("selectedIndexBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetSelectedIndexBackgroundColor((UInt32)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("fontSize"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetFontSize((UInt)n);
                }

                // Reading window parameters.
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("readingWindowTextColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetReadingWindowTextColor((UInt)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("readingWindowBackgroundColor"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetReadingWindowBackgroundColor((UInt)n);
                }
                if (pobj->GetMember(fn.Env, fn.Env->CreateConstString("readingWindowFontSize"), &val))
                {
                    GASNumber n = val.ToNumber(fn.Env);
                    if (!GASNumberUtil::IsNaNOrInfinity(n))
                        st.SetReadingWindowFontSize((UInt)n);
                }
                pimeMgr->SetCandidateListStyle(st);
            }
        }
    }
}

static void GAS_GetIMECandidateListStyle(const GASFnCall& fn)
{       
    GFxASCharacter* const poriginalTarget   = fn.Env->GetTarget();   
    GFxMovieRoot* proot                     = poriginalTarget->GetMovieRoot();
    GPtr<GFxIMEManager> pimeMgr             = proot->GetIMEManager();
    if (pimeMgr)
    {
        GFxIMECandidateListStyle st; 
        if (pimeMgr->GetCandidateListStyle(&st))
        {
            GPtr<GASObject> pobj = *new GASObject(fn.Env);
            if (st.HasTextColor())
            {
                GASNumber c = (GASNumber)(st.GetTextColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "textColor", GASValue(c));
            }
            if (st.HasBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "backgroundColor", GASValue(c));
            }
            if (st.HasIndexBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetIndexBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "indexBackgroundColor", GASValue(c));
            }
            if (st.HasSelectedTextColor())
            {
                GASNumber c = (GASNumber)(st.GetSelectedTextColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "selectedTextColor", GASValue(c));
            }
            if (st.HasSelectedBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetSelectedBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "selectedTextBackgroundColor", GASValue(c));
            }
            if (st.HasSelectedIndexBackgroundColor())
            {
                GASNumber c = (GASNumber)(st.GetSelectedIndexBackgroundColor() & 0xFFFFFFu);
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "selectedIndexBackgroundColor", GASValue(c));
            }
            if (st.HasFontSize())
            {
                GASNumber c = (GASNumber)st.GetFontSize();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "fontSize", GASValue(c));
            }
            // Reading Window Styles

            if (st.HasReadingWindowTextColor())
            {
                GASNumber c = (GASNumber)st.GetReadingWindowTextColor();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "readingWindowTextColor", GASValue(c));
            }

            if (st.HasReadingWindowBackgroundColor())
            {
                GASNumber c = (GASNumber)st.GetReadingWindowBackgroundColor();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "readingWindowBackgroundColor", GASValue(c));
            }

            if (st.HasReadingWindowFontSize())
            {
                GASNumber c = (GASNumber)st.GetReadingWindowFontSize();
                pobj->SetConstMemberRaw(fn.Env->GetSC(), "readingWindowFontSize", GASValue(c));
            }

            fn.Result->SetAsObject(pobj);
        }
    }
}
#endif // GFC_NO_IME_SUPPORT

GASObject* GASGlobalContext::AddPackage(GASStringContext *psc, GASObject* pparent, GASObject* objProto, const char* const packageName)
{
    char            buf[256];
    size_t          nameSz = strlen(packageName) + 1;
    const char*     pname = packageName;
    GPtr<GASObject> parent = pparent;

    while(pname)
    {
        const char* p = strchr(pname, '.');
        size_t sz;
        if (p)
            sz = p++ - pname + 1;
        else
            sz = nameSz - (pname - packageName);

        GASSERT(sz <= sizeof(buf));
        if (sz > sizeof(buf)) 
            sz = sizeof(buf);
    
        //strncpy(buf, pname, sz-1);
        memcpy(buf, pname, sz-1);
        buf[sz-1] = '\0';

        pname = p;

        GASValue        pkgObjVal;
        GPtr<GASObject> pkgObj;
        GASString       memberName(psc->CreateString(buf));

        if (parent->GetMemberRaw(psc, memberName, &pkgObjVal))
        {
            pkgObj = pkgObjVal.ToObject();
        }
        else
        {
            pkgObj = *new GASObject(psc, objProto);
            parent->SetMemberRaw(psc, memberName, GASValue(pkgObj));
        }
        parent = pkgObj;
    }
    return parent;
}


class GASGlobalObject : public GASObject
{
    GASGlobalContext* pGC;
public:
    GASGlobalObject(GASGlobalContext* pgc) : pGC(pgc) {}

    virtual bool    GetMember(GASEnvironment *penv, const GASString& name, GASValue* val)
    {
        if (name == penv->GetSC()->GetBuiltin(GASBuiltin_gfxExtensions))
        {
            if (pGC->GFxExtensions.IsDefined())
            {
                val->SetBool(pGC->GFxExtensions.IsTrue());
                return true;
            }
            else
                val->SetUndefined();
            return false;
        }
        return GASObject::GetMember(penv, name, val);
    }
    virtual bool    SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        if (name == penv->GetSC()->GetBuiltin(GASBuiltin_gfxExtensions))
        {
            pGC->GFxExtensions = val.ToBool(penv);
            if (pGC->GFxExtensions.IsTrue())
            {
                SetConstMemberRaw(penv->GetSC(), "gfxVersion", GASValue(pGC->CreateConstString(GFC_FX_VERSION_STRING)));
            }
            else
            {
                DeleteMember(penv->GetSC(), pGC->CreateConstString("gfxVersion"));
            }
            return GASObject::SetMember(penv, name, GASValue(GASValue::UNSET), flags);
        }
        else if (pGC->GFxExtensions.IsTrue())
        {
            if (name == penv->GetSC()->GetBuiltin(GASBuiltin_noInvisibleAdvance))
            {
                GFxMovieRoot* proot = penv->GetMovieRoot();
                if (proot)
                    proot->SetNoInvisibleAdvanceFlag(val.ToBool(penv));
            }
        }
        return GASObject::SetMember(penv, name, val, flags);
    }
};

// Helper function, initializes the global object.
GASGlobalContext::GASGlobalContext(GFxMovieRoot* pRoot)
{    
    // Initialization of built-ins is done in base.

    // String context for construction, delegates to us.
    // SWF Version does not matter for member creation.
    GASStringContext sc(this, 8);

    pGlobal = *new GASGlobalObject(this);
    
    // Create constructors
    GASFunctionRef objCtor (*new GASObjectCtorFunction(&sc));
    GASFunctionRef arrayCtor (*new GASArrayCtorFunction(&sc));
    GASFunctionRef stringCtor (*new GASStringCtorFunction(&sc));
    GASFunctionRef numberCtor (*new GASNumberCtorFunction(&sc));
    GASFunctionRef boolCtor (*new GASBooleanCtorFunction(&sc));
    GASFunctionRef movieClipCtor (*new GASFunctionObject(GASMovieClipProto::GlobalCtor));
    GASFunctionRef functionCtor (*new GASFunctionCtorFunction(&sc));
    GASFunctionRef soundCtor (*new GASFunctionObject (GASSoundProto::GlobalCtor));
    GASFunctionRef buttonCtor (*new GASFunctionObject (GASButtonProto::GlobalCtor));
    GASFunctionRef textFieldCtor (*new GASFunctionObject (GASTextFieldProto::GlobalCtor));
    GASFunctionRef colorCtor (*new GASFunctionObject (GASColorProto::GlobalCtor));
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM
    GASFunctionRef transformCtor (*new GASFunctionObject (GASTransformProto::GlobalCtor));
#endif
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    GASFunctionRef matrixCtor (*new GASFunctionObject (GASMatrixProto::GlobalCtor));
#endif
#ifndef GFC_NO_FXPLAYER_AS_POINT
    GASFunctionRef pointCtor (*new GASPointCtorFunction(&sc));
#endif
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    GASFunctionRef rectangleCtor (*new GASFunctionObject (GASRectangleProto::GlobalCtor));
#endif
#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    GASFunctionRef colorTransformCtor (*new GASFunctionObject (GASColorTransformProto::GlobalCtor));
#endif
    GASFunctionRef stageCtor (*new GASStageCtorFunction(&sc, pRoot));
    GASFunctionRef selectionCtor (*new GASSelectionCtorFunction(&sc));
    GASFunctionRef broadcasterCtor (*new GASAsBroadcasterCtorFunction(&sc));
#ifndef GFC_NO_FXPLAYER_AS_DATE
    GASFunctionRef dateCtor (*new GASDateCtorFunction(&sc));
#endif
    GASFunctionRef mouseCtor (*new GASMouseCtorFunction(&sc, pRoot));
    GASFunctionRef extIntfCtor (*new GASExternalInterfaceCtorFunction(&sc));
    GASFunctionRef movieClipLdrCtor (*new GASMovieClipLoaderCtorFunction(&sc));
    GASFunctionRef bmpDataCtor (*new GASBitmapDataCtorFunction(&sc));
    GASFunctionRef capabilitiesCtor (*new GASCapabilitiesCtorFunction(&sc));
    GASFunctionRef loadVarsCtor (*new GASFunctionObject (GASLoadVarsProto::GlobalCtor));
    GASFunctionRef textFormatCtor (*new GASTextFormatCtorFunction(&sc));

#ifdef GFC_XML_EXPOSED
#ifdef GFC_USE_XML
    GASFunctionRef xmlCtor (*new GASFunctionObject(GASXmlProto::GlobalCtor));
    GASFunctionRef xmlNodeCtor (*new GASFunctionObject(GASXmlNodeProto::GlobalCtor));
#endif
#endif

    // Init built-in prototype instances
    GPtr<GASObject> objProto        = *new GASObjectProto(&sc, objCtor);
    GPtr<GASObject> arrayProto      = *new GASArrayProto(&sc, objProto, arrayCtor);
    GPtr<GASObject> stringProto     = *new GASStringProto(&sc, objProto, stringCtor);
    GPtr<GASObject> numberProto     = *new GASNumberProto(&sc, objProto, numberCtor);
    GPtr<GASObject> boolProto       = *new GASBooleanProto(&sc, objProto, boolCtor);
    GPtr<GASObject> movieClipProto  = *new GASMovieClipProto(&sc, objProto, movieClipCtor);
    GPtr<GASObject> functionProto   = *new GASFunctionProto(&sc, objProto, functionCtor);
    GPtr<GASObject> soundProto      = *new GASSoundProto(&sc, objProto, soundCtor);
    GPtr<GASObject> buttonProto     = *new GASButtonProto(&sc, objProto, buttonCtor);
    GPtr<GASObject> textFieldProto  = *new GASTextFieldProto(&sc, objProto, textFieldCtor);
    GPtr<GASObject> colorProto      = *new GASColorProto(&sc, objProto, colorCtor);
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM
    GPtr<GASObject> transformProto  = *new GASTransformProto(&sc, objProto, transformCtor);
#endif
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    GPtr<GASObject> matrixProto     = *new GASMatrixProto(&sc, objProto, matrixCtor);
#endif
#ifndef GFC_NO_FXPLAYER_AS_POINT
    GPtr<GASObject> pointProto      = *new GASPointProto(&sc, objProto, pointCtor);
#endif
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    GPtr<GASObject> rectangleProto  = *new GASRectangleProto(&sc, objProto, rectangleCtor);
#endif
#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    GPtr<GASObject> colorTransformProto = *new GASColorTransformProto(&sc, objProto, colorTransformCtor);
#endif
    GPtr<GASObject> stageProto      = *new GASStageProto(&sc, objProto, stageCtor);
    GPtr<GASObject> broadcasterProto= *new GASAsBroadcasterProto(&sc, objProto, broadcasterCtor);
#ifndef GFC_NO_FXPLAYER_AS_DATE
    GPtr<GASObject> dateProto       = *new GASDateProto(&sc, objProto, dateCtor);
#endif
    GPtr<GASObject> mouseProto      = *new GASMouseProto(&sc, objProto, mouseCtor);
    GPtr<GASObject> extIntfProto    = *new GASExternalInterfaceProto(&sc, objProto, extIntfCtor);
    GPtr<GASObject> movieClipLdrProto = *new GASMovieClipLoaderProto(&sc, objProto, movieClipLdrCtor);
    GPtr<GASObject> bmpDataProto    = *new GASBitmapDataProto(&sc, objProto, bmpDataCtor);
    GPtr<GASObject> loadVarsProto   = *new GASLoadVarsProto(&sc, objProto, loadVarsCtor);
    GPtr<GASObject> textFormatProto = *new GASTextFormatProto(&sc, objProto, textFormatCtor);

#ifdef GFC_XML_EXPOSED
#ifdef GFC_USE_XML
    GPtr<GASObject> xmlProto        = *new GASXmlProto(&sc, objProto, xmlCtor);
    GPtr<GASObject> xmlNodeProto    = *new GASXmlNodeProto(&sc, objProto, xmlNodeCtor);
#endif
#endif

    // A special case for Function class: its __proto__ is equal to Function.prototype.
    // This is the undocumented feature.
    functionCtor->Set__proto__(&sc, functionProto);

    Prototypes.add (GASBuiltin_Object, objProto);
    Prototypes.add (GASBuiltin_Array, arrayProto);
    Prototypes.add (GASBuiltin_String, stringProto);
    Prototypes.add (GASBuiltin_Number, numberProto);
    Prototypes.add (GASBuiltin_Boolean, boolProto);
    Prototypes.add (GASBuiltin_MovieClip, movieClipProto);
    Prototypes.add (GASBuiltin_Function, functionProto);
    Prototypes.add (GASBuiltin_Sound, soundProto);
    Prototypes.add (GASBuiltin_Button, buttonProto);
    Prototypes.add (GASBuiltin_TextField, textFieldProto);
    Prototypes.add (GASBuiltin_Color, colorProto);
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM
    Prototypes.add (GASBuiltin_Transform, transformProto);
#endif
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    Prototypes.add (GASBuiltin_Matrix, matrixProto);
#endif
#ifndef GFC_NO_FXPLAYER_AS_POINT
    Prototypes.add (GASBuiltin_Point, pointProto);
#endif
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    Prototypes.add (GASBuiltin_Rectangle, rectangleProto);
#endif
#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    Prototypes.add (GASBuiltin_ColorTransform, colorTransformProto);
#endif
    Prototypes.add (GASBuiltin_Stage, stageProto);
    Prototypes.add (GASBuiltin_AsBroadcaster, broadcasterProto);
#ifndef GFC_NO_FXPLAYER_AS_DATE
    Prototypes.add (GASBuiltin_Date, dateProto);
#endif
    Prototypes.add (GASBuiltin_Mouse, mouseProto);
    Prototypes.add (GASBuiltin_ExternalInterface, extIntfProto);
    Prototypes.add (GASBuiltin_MovieClipLoader, movieClipLdrProto);
    Prototypes.add (GASBuiltin_BitmapData, bmpDataProto);
    Prototypes.add (GASBuiltin_LoadVars, loadVarsProto);
    Prototypes.add (GASBuiltin_TextFormat, textFormatProto);

#ifdef GFC_XML_EXPOSED
#ifdef GFC_USE_XML
    Prototypes.add (GASBuiltin_XML, xmlProto);
    Prototypes.add (GASBuiltin_XMLNode, xmlNodeProto);
#endif
#endif

    // @@ pGlobal should really be a
    // client-visible player object, which
    // contains one or more actual GFxASCharacter
    // instances.

    pGlobal->SetConstMemberRaw(&sc, "trace", GASValue(GAS_GlobalTrace));   
    pGlobal->SetConstMemberRaw(&sc, "parseInt", GASValue(GAS_GlobalParseInt));
    pGlobal->SetConstMemberRaw(&sc, "parseFloat", GASValue(GAS_GlobalParseFloat));
    pGlobal->SetConstMemberRaw(&sc, "ifFrameLoaded", GASValue(GAS_GlobalIfFrameLoaded));
    pGlobal->SetConstMemberRaw(&sc, "isNaN", GASValue(GAS_GlobalIsNaN));
    pGlobal->SetConstMemberRaw(&sc, "isFinite", GASValue(GAS_GlobalIsFinite));    

#ifndef GFC_NO_IME_SUPPORT
    // extensions for IME
    pGlobal->SetConstMemberRaw(&sc, "imecommand", GASValue(GAS_GlobalIMECommand));    
    pGlobal->SetConstMemberRaw(&sc, "setIMECandidateListStyle", GASValue(GAS_SetIMECandidateListStyle));    
    pGlobal->SetConstMemberRaw(&sc, "getIMECandidateListStyle", GASValue(GAS_GetIMECandidateListStyle));    
#endif // GFC_NO_IME_SUPPORT

    // Extension: gfxVersion.
    //pGlobal->SetConstMemberRaw(&sc, "gfxVersion", GASValue(CreateConstString(GFC_FX_VERSION_STRING))); // see GASGlobalObject

    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Object), GASValue(objCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_String), GASValue(stringCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Sound),  GASValue(soundCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Array),  GASValue(arrayCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Number), GASValue(numberCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Boolean), GASValue(boolCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Function), GASValue(functionCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Color), GASValue(colorCtor));

    //  pGlobal->SetMemberRaw("TextFormat", GASValue(GAS_TextFormatNew));
    //pGlobal->SetMemberRaw("MovieClipLoader", GASValue(GAS_MovieClipLoaderNew));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_MovieClip), GASValue(movieClipCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Button), GASValue(buttonCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_TextField), GASValue(textFieldCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Stage), GASValue(stageCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Selection), GASValue(selectionCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_AsBroadcaster), GASValue(broadcasterCtor));
#ifndef GFC_NO_FXPLAYER_AS_DATE
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Date), GASValue(dateCtor));
#endif
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_MovieClipLoader), GASValue(movieClipLdrCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_LoadVars), GASValue(loadVarsCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_TextFormat), GASValue(textFormatCtor));

#ifdef GFC_XML_EXPOSED
#ifdef GFC_USE_XML
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_XML), GASValue(xmlCtor));
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_XMLNode), GASValue(xmlNodeCtor));
#endif
#endif

    // ASSetPropFlags/ASnative
    pGlobal->SetConstMemberRaw(&sc, "ASSetPropFlags", GASValue(GAS_GlobalASSetPropFlags));
    pGlobal->SetConstMemberRaw(&sc, "ASnative", GASValue(GAS_GlobalASnative));

    GPtr<GASObject> pmath = * GFxAction_CreateMathGlobalObject(&sc);
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Math), pmath.GetPtr());
    GPtr<GASObject> pkey = * GASKeyAsObject::CreateGlobalObject(&sc, pRoot);
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Key), pkey.GetPtr());
    pGlobal->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Mouse), GASValue(mouseCtor));

    // Global functions
    pGlobal->SetConstMemberRaw(&sc, "escape", GASValue(GAS_GlobalEscape));
    pGlobal->SetConstMemberRaw(&sc, "unescape", GASValue(GAS_GlobalUnescape));
    pGlobal->SetConstMemberRaw(&sc, "escapeSpecialHTML", GASValue(GAS_GlobalEscapeSpecialHTML));
    pGlobal->SetConstMemberRaw(&sc, "unescapeSpecialHTML", GASValue(GAS_GlobalUnescapeSpecialHTML));
    pGlobal->SetConstMemberRaw(&sc, "updateAfterEvent", GASValue(GAS_GlobalUpdateAfterEvent));
    pGlobal->SetConstMemberRaw(&sc, "setInterval", GASValue(GASIntervalTimer::SetInterval));
    pGlobal->SetConstMemberRaw(&sc, "clearInterval", GASValue(GASIntervalTimer::ClearInterval));
    pGlobal->SetConstMemberRaw(&sc, "setTimeout", GASValue(GASIntervalTimer::SetTimeout));
    pGlobal->SetConstMemberRaw(&sc, "clearTimeout", GASValue(GASIntervalTimer::ClearTimeout));

    // classes from packages
    GPtr<GASObject> flash_geom_package = AddPackage(&sc, pGlobal, objProto, "flash.geom");
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM
    flash_geom_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Transform), GASValue(transformCtor));
#endif
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    flash_geom_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Matrix), GASValue(matrixCtor));
#endif
#ifndef GFC_NO_FXPLAYER_AS_POINT
    flash_geom_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Point), GASValue(pointCtor));
#endif
#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    flash_geom_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Rectangle), GASValue(rectangleCtor));
#endif
#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    flash_geom_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_ColorTransform), GASValue(colorTransformCtor));
#endif

    GPtr<GASObject> system_package = AddPackage(&sc, pGlobal, objProto, "System");
    system_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_Capabilities), GASValue(capabilitiesCtor));

    GPtr<GASObject> flash_external_package = AddPackage(&sc, pGlobal, objProto, "flash.external");
    flash_external_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_ExternalInterface), GASValue(extIntfCtor));

    GPtr<GASObject> flash_display_package = AddPackage(&sc, pGlobal, objProto, "flash.display");
    flash_display_package->SetMemberRaw(&sc, GetBuiltin(GASBuiltin_BitmapData), GASValue(bmpDataCtor));

    InitStandardMembers();

    // hide all members
    struct MemberVisitor : GASObjectInterface::MemberVisitor
    {
        GPtr<GASObject> obj;
        GASStringContext *psc;
        
        MemberVisitor (GASStringContext *_psc, GASObject* _obj) : obj(_obj), psc(_psc) {}

        virtual void Visit(const GASString& name, const GASValue& val, UByte flags)
        {
            GUNUSED(val);
            obj->SetMemberFlags(psc, name, (UByte)(flags| GASPropFlags::PropFlag_DontEnum));

        }
    } visitor (&sc, pGlobal);
    pGlobal->VisitMembers(&sc, &visitor, GASObjectInterface::VisitMember_DontEnum);
}

GASGlobalContext::~GASGlobalContext()
{
    // ReleaseBuiltins() - release of built-ins is done in base destructor.
    // If that was not the case we would've had to clean up all local string maps first explicitly.
}



GASString GASGlobalContext::FindClassName(GASStringContext *psc, GASObjectInterface* iobj)
{
    if (iobj)
    {
        GASObject* obj;
        if (iobj->IsASCharacter())
            obj = static_cast<GFxASCharacter*>(iobj)->GetASObject();
        else
            obj = static_cast<GASObject*>(iobj);

        GASObject::MemberHash::const_iterator it = pGlobal->Members.begin();
        
        for (; it != pGlobal->Members.end(); ++it)
        {
            const GASString& nm = it->first;
            const GASMember& m = it->second;
            const GASValue& val = m.Value;
            if (obj->IsFunction())
            {
                if (val.IsFunction() && val.ToFunction().GetObjectPtr() == obj)
                    return nm;
            }
            else
            {
                if (val.IsObject() && val.ToObject() == obj)
                    return nm;
                if (val.IsFunction())
                {
                    GASFunctionRef f = val.ToFunction();
                    GASValue protoVal;
                    if (f->GetMemberRaw(psc, GetBuiltin(GASBuiltin_prototype), &protoVal))
                    {
                        if (protoVal.IsObject() && protoVal.ToObject() == obj)
                            return nm + ".prototype";
                    }
                }
            }
        }
    }
    return GetBuiltin(GASBuiltin_unknown_);
}

void    GASGlobalContext::InitStandardMembers()
{
    GFxASCharacter::InitStandardMembers(this);
}

bool GASGlobalContext::RegisterClass(GASStringContext* psc, const GASString& className, const GASFunctionRef& ctorFunction)
{
    RegisteredClasses.set_CaseCheck(className, ctorFunction, psc->IsCaseSensitive());
    return true;
}

bool GASGlobalContext::UnregisterClass(GASStringContext* psc, const GASString& className)
{    
    if (RegisteredClasses.get_CaseCheck(className, psc->IsCaseSensitive()) == 0)
        return false;
    RegisteredClasses.remove_CaseCheck(className, psc->IsCaseSensitive());
    return true;
}

bool GASGlobalContext::FindRegisteredClass(GASStringContext* psc, const GASString& className, GASFunctionRef* pctorFunction)
{
    // MA TBD: Conditional case sensitivity
    const GASFunctionRef* ctor = RegisteredClasses.get_CaseCheck(className, psc->IsCaseSensitive());
    if (ctor == 0)
        return false;
    if (pctorFunction)
        *pctorFunction = *ctor;
    return true;
}

void GASGlobalContext::UnregisterAllClasses()
{
    RegisteredClasses.resize(0);
}

//
// GASDoAction
//

// Thin wrapper around GASActionBuffer.
class GASDoAction : public GASExecuteTag
{
public:
    GPtr<GASActionBufferData> pBuf;

    void    Read(GFxStream* in)
    {
        pBuf = *new GASActionBufferData();
        pBuf->Read(in);
    }

    virtual void    Execute(GFxSprite* m)
    {
        if (pBuf && !pBuf->IsNull())
        {
            GPtr<GASActionBuffer> pbuff =
                *new GASActionBuffer(m->GetASEnvironment()->GetSC(), pBuf);
            m->AddActionBuffer(pbuff.GetPtr());
        }
    }

    virtual void    ExecuteWithPriority(GFxSprite* m, GFxActionPriority::Priority prio) 
    { 
        if (pBuf && !pBuf->IsNull())
        {
            GPtr<GASActionBuffer> pbuff =
                *new GASActionBuffer(m->GetASEnvironment()->GetSC(), pBuf);
            m->AddActionBuffer(pbuff.GetPtr(), prio);
        }
    }
    // Don't override because actions should not be replayed when seeking the GFxSprite.
    //void  ExecuteState(GFxSprite* m) {}

    // Tell the caller that we are an action tag.
    virtual bool    IsActionTag() const
    {
        return true;
    }
};

class GASDoInitAction : public GASDoAction
{
    GASDoInitAction(const GASDoInitAction&) {} // suppress warning
    GASDoInitAction& operator=(const GASDoInitAction&) { return *this; } // suppress warning
public:
    GASDoInitAction() {}

    virtual void    Execute(GFxSprite* m)
    {
        if (pBuf && !pBuf->IsNull())
        {        
            GPtr<GASActionBuffer> pbuff = *new GASActionBuffer(m->GetASEnvironment()->GetSC(), pBuf);
            m->AddActionBuffer(pbuff.GetPtr(), GFxMovieRoot::AP_InitClip);
        }
    }
    
    // Tell the caller that we are not a regular action tag.
    virtual bool    IsActionTag() const
    {
        return false;
    }
};

void    GSTDCALL GFx_DoActionLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    p->LogParse("tag %d: DoActionLoader\n", tagInfo.TagType);
    p->LogParseAction("-- actions in frame %d\n", p->GetLoadingFrame());

    GASSERT(p);
    GASSERT(tagInfo.TagType == GFxTag_DoAction);    
    
    GASDoAction* da = p->AllocTag<GASDoAction>();
    da->Read(p->GetStream());
    p->AddExecuteTag(da);
}


//
// DoInitAction
//


void    GSTDCALL GFx_DoInitActionLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DoInitAction);

    int spriteCharacterId = p->ReadU16();

    p->LogParse("  tag %d: DoInitActionLoader\n", tagInfo.TagType);
    p->LogParseAction("  -- init actions for sprite %d\n", spriteCharacterId);

    GASDoAction* da = p->AllocTag<GASDoInitAction>();
    da->Read(p->GetStream());
    p->AddInitAction(GFxResourceId(spriteCharacterId), da);
}


//
// GASActionBuffer
//



// ***** With Stack implementation


GASWithStackEntry::GASWithStackEntry(GASObject* pobj, int end)
{
    pObjectOrChar = static_cast<GRefCountBase<GASObject>*>(pobj); // Compiler does not yet know GASObject.
    IsObject      = 1;
    BlockEndPc    = end;
}
GASWithStackEntry::GASWithStackEntry(GFxASCharacter* pcharacter, int end)
{
    pObjectOrChar = static_cast<GRefCountBase<GFxCharacter>*>(pcharacter);
    IsObject      = 0;
    BlockEndPc    = end;
}

GASObject*          GASWithStackEntry::GetObject() const
{
    return IsObject ? static_cast<GASObject*>(pObjectOrChar.GetPtr()) : 0;
}
GFxASCharacter*     GASWithStackEntry::GetCharacter() const
{
    return IsObject ? 0 : static_cast<GFxASCharacter*>(pObjectOrChar.GetPtr());
}

GASObjectInterface* GASWithStackEntry::GetObjectInterface() const
{
    return IsObject ?   (GASObjectInterface*) static_cast<GASObject*>(pObjectOrChar.GetPtr()) :
                        (GASObjectInterface*) static_cast<GFxASCharacter*>(pObjectOrChar.GetPtr());
}



// ***** GASActionBuffer implementation

void    GASActionBufferData::Read(GFxStream* in)
{
    FileName = in->GetFileName();

    for (;;)
    {
        UPInt   instructionStart    = Buffer.size();
        UPInt   pc                  = Buffer.size();
        UByte   actionId            = in->ReadU8();

        Buffer.push_back(actionId);

        if (actionId & 0x80)
        {
            // Action contains extra data.  Read it.
            int length = in->ReadU16();
            Buffer.push_back(UByte(length & 0x0FF));
            Buffer.push_back(UByte((length >> 8) & 0x0FF));
            for (int i = 0; i < length; i++)
            {
                UByte   b = in->ReadU8();
                Buffer.push_back(b);
            }
        }
        

#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE_ACTION
        if (in->IsVerboseParseAction())
        {
            in->LogParseAction("%4d\t", (int)pc);
            GFxDisasm da(in->GetLog(), GFxLog::Log_ParseAction);
            da.LogDisasm(&Buffer[instructionStart]);
        }
#else
        GUNUSED2(instructionStart,pc);
#endif        

        if (actionId == 0)
        {
            // end of action buffer.
            break;
        }
    }

    // Last byte in buffer will always be zero.
    GASSERT(Buffer[Buffer.size()-1] == 0);
}


// Interpret the DeclDict opcode.  Don't read StopPc or
// later.  A dictionary is some static strings embedded in the
// action buffer; there should only be one dictionary per
// action buffer.
//
// NOTE: Normally the dictionary is declared as the first
// action in an action buffer, but I've seen what looks like
// some form of copy protection that amounts to:
//
// <start of action buffer>
//          push true
//          BranchIfTrue label
//          DeclDict   [0]   // this is never executed, but has lots of orphan data declared in the opcode
// label:   // (embedded inside the previous opcode; looks like an invalid jump)
//          ... "protected" code here, including the real DeclDict opcode ...
//          <end of the dummy DeclDict [0] opcode>
//
// So we just interpret the first DeclDict we come to, and
// cache the results.  If we ever hit a different DeclDict in
// the same GASActionBuffer, then we log an error and ignore it.
void    GASActionBuffer::ProcessDeclDict(GASStringContext *psc, UInt StartPc, UInt StopPc, class GFxActionLogger &log)
{
    GASSERT(StopPc <= GetLength());
    const UByte*  Buffer = GetBufferPtr();

    if (DeclDictProcessedAt == (int)StartPc)
    {
        // We've already processed this DeclDict.
        UInt    count = Buffer[StartPc + 3] | (Buffer[StartPc + 4] << 8);
        GASSERT(Dictionary.size() == count);
        GUNUSED(count);
        return;
    }

    if (DeclDictProcessedAt != -1)
    {
        if (log.IsVerboseActionErrors())
            log.LogScriptError("Error: ProcessDeclDict(%d, %d) - DeclDict was already processed at %d\n",
                StartPc,
                StopPc,
                DeclDictProcessedAt);
        return;
    }

    DeclDictProcessedAt = (int)StartPc;

    // Actual processing.
    UInt    i = StartPc;
    UInt    length = Buffer[i + 1] | (Buffer[i + 2] << 8);
    UInt    count = Buffer[i + 3] | (Buffer[i + 4] << 8);
    i += 2;
    GUNUSED(length);
    GASSERT(StartPc + 3 + length == StopPc);

    Dictionary.resize(count);    

    // Index the strings.
    for (UInt ct = 0; ct < count; ct++)
    {
        // Point into the current action buffer.        
        Dictionary[ct] = psc->CreateString((const char*) &Buffer[3 + i]);

        while (Buffer[3 + i])
        {
            // safety check.
            if (i >= StopPc)
            {
                if (log.IsVerboseActionErrors()) 
                    log.LogScriptError("Error: Action buffer dict length exceeded\n");

                // Jam something into the Remaining (invalid) entries.
                while (ct < count)
                {
                    Dictionary[ct] = psc->CreateString("<invalid>");
                    ct++;
                }
                return;
            }
            i++;
        }
        i++;
    }
}

// Interpret the actions in this action buffer, and evaluate
// them in the given environment.  Execute our whole buffer,
// without any arguments passed in.
void    GASActionBuffer::Execute(GASEnvironment* env)
{
    //int   LocalStackTop = env->GetLocalFrameTop();
    //env->AddFrameBarrier(); //??AB, should I create a local frame here??? AddFrameBarrier does nothing.
    //GPtr<GASLocalFrame> curLocalFrame = env->CreateNewLocalFrame();

    GTL::garray<GASWithStackEntry>  EmptyWithStack;
    Execute(env, 0, GetLength(), NULL, EmptyWithStack, Exec_Unknown /* not function2 */);

    //curLocalFrame->ReleaseFramesForLocalFuncs ();
    //env->SetLocalFrameTop(LocalStackTop);
}



#ifdef GFXACTION_COUNT_OPS
// Debug counters for performance statistics.
UInt Execute_OpCounters[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

UInt Execute_PushDataTypeCounter[16] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

UInt Execute_NotType[16] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
#endif

// Interpret the specified subset of the actions in our
// buffer.  Caller is responsible for cleaning up our local
// stack Frame (it may have passed its arguments in via the
// local stack frame).
// 
// The execType option determines whether to use global or local registers.
void    GASActionBuffer::Execute(
                        GASEnvironment* env,
                        int StartPc,
                        int ExecBytes,
                        GASValue* retval,
                        const GTL::garray<GASWithStackEntry>& InitialWithStack,
                        ExecuteType execType)
{
    GASSERT(env);

    GTL::garray<GASWithStackEntry>  WithStack(InitialWithStack);

    // Some corner case behaviors depend on the SWF file version.
    UInt version        = env->GetTarget()->GetVersion();
    bool isFunction2    = (execType == Exec_Function2);
    const UByte* Buffer = GetBufferPtr();

#if 0
    // Check the time
    if (PeriodicEvents.Expired()) {
        PeriodicEvents.PollEventHandlers(env);
    }
#endif
        
    GFxASCharacter* const poriginalTarget = env->GetTarget();   
    bool isOriginalTargetValid = env->IsTargetValid();
    GFxActionLogger log(poriginalTarget, pBufferData->GetFileName());
    const bool verboseAction        = log.IsVerboseAction();
    const bool verboseActionErrors  = log.IsVerboseActionErrors();

    //Sets and clears Log for env
    class ASLoggerHelper
    {
        GASEnvironment*     Env;
        GFxActionLogger*    pPrevLog;

     public:
         ASLoggerHelper(GASEnvironment* env, GFxActionLogger* plog): Env(env) 
         { 
             pPrevLog = Env->GetASLogger();  
             Env->SetASLogger(plog); 
         };
        ~ASLoggerHelper() 
        { 
             Env->SetASLogger(pPrevLog); 
        };
    };

    ASLoggerHelper asLoggerHelper(env, &log);

    if (verboseAction) log.LogAction("\n");

    int StopPc = StartPc + ExecBytes;

    for (int pc = StartPc; pc < StopPc; )
    {
        // Cleanup any expired "with" blocks.
        while (WithStack.size() > 0
               && pc >= WithStack.back().BlockEndPc)
        {
            // Drop this stack element
            WithStack.resize(WithStack.size() - 1);
        }

        // Get the opcode.
        int actionId = Buffer[pc];

#ifdef GFXACTION_COUNT_OPS
        Execute_OpCounters[actionId]++;
#endif

        if ((actionId & 0x80) == 0)
        {
            if (verboseAction) 
            {
                log.LogAction("EX: %4.4X\t", pc);
                log.LogDisasm(&Buffer[pc]);
            }

            // log.LogAction("Action ID is: 0x%x\n", actionId));
        
            // Simple action; no extra data.
            switch (actionId)
            {
                default:
                    break;

                case 0x00:  // end of actions.
                    return;

                case 0x04:  // next frame.
                    if (env->IsTargetValid())
                        env->GetTarget()->GotoFrame(env->GetTarget()->GetCurrentFrame() + 1);
                    break;

                case 0x05:  // prev frame.
                    if (env->IsTargetValid())
                        env->GetTarget()->GotoFrame(env->GetTarget()->GetCurrentFrame() - 1);
                    break;

                case 0x06:  // action play
                    if (env->IsTargetValid())
                        env->GetTarget()->SetPlayState(GFxMovie::Playing);
                    break;

                case 0x07:  // action stop
                    if (env->IsTargetValid())
                        env->GetTarget()->SetPlayState(GFxMovie::Stopped);
                    break;

                case 0x08:  // toggle quality
                case 0x09:  // stop sounds
                    break;

                case 0x0A:  // add
                {
                    env->Top1().Add(env, env->Top());
                    env->Drop1();
                    break;
                }
                case 0x0B:  // subtract
                {
                    env->Top1().Sub(env, env->Top());
                    env->Drop1();
                    break;
                }
                case 0x0C:  // multiply
                {
                    env->Top1().Mul(env, env->Top());
                    env->Drop1();
                    break;
                }
                case 0x0D:  // divide
                {
                    env->Top1().Div(env, env->Top());
                    env->Drop1();
                    break;
                }
                case 0x0E:  // equal
                {
                    env->Top1().SetBool(env->Top1().IsEqual (env, env->Top()));
                    env->Drop1();
                    break;
                }
                case 0x0F:  // less than
                {
                    env->Top1() = env->Top1().Compare (env, env->Top(), -1);
                    env->Drop1();
                    break;
                }
                case 0x10:  // logical and
                {
                    env->Top1().SetBool(env->Top1().ToBool(env) && env->Top().ToBool(env));
                    env->Drop1();
                    break;
                }
                case 0x11:  // logical or
                {
                    env->Top1().SetBool(env->Top1().ToBool(env) || env->Top().ToBool(env));
                    env->Drop1();
                    break;
                }
                case 0x12:  // logical not
                {
                    GASValue& topVal = env->Top();
                    // LOgical NOT is a very frequently executed op (used to invert conditions 
                    // for branching?). Optimize it for most common values.                    
                    if (topVal.GetType() == GASValue::BOOLEAN)
                    {
                        // Here, BOOLEAN is by far the most common type.
                        topVal.V.BooleanValue = !topVal.V.BooleanValue;
                    }
                    else  if (topVal.GetType() == GASValue::UNDEFINED)
                    {
                        topVal.T.Type = GASValue::BOOLEAN;
                        topVal.V.BooleanValue = true;
                    }
                    else 
                    {
                        topVal.SetBool(!topVal.ToBool(env));
                    }
                    break;
                }
                case 0x13:  // string equal
                {
                    env->Top1().SetBool(env->Top1().ToStringVersioned(env, version) == env->Top().ToStringVersioned(env, version));
                    env->Drop1();
                    break;
                }
                case 0x14:  // string length
                {
                    env->Top().SetInt(env->Top().ToStringVersioned(env, version).GetLength());
                    break;
                }
                case 0x15:  // substring
                {
                    GASString retVal(GASStringProto::StringSubstring(
                                        env->Top(2).ToStringVersioned(env, version),
                                        env->Top1().ToInt32(env) - 1,
                                        env->Top().ToInt32(env) ));
                    env->Drop2();
                    env->Top().SetString(retVal);
                    break;
                }
                case 0x17:  // pop
                {
                    //!AB, here is something I can't understand for now: if class is
                    // declared as com.package.Class then AS code contains
                    // creation of "com" and "com.package" objects as "new Object".
                    // But, the problem is in one extra "pop" command after that, when
                    // the stack is already empty. Thus, I commented out the assert below
                    // for now....
                    //GASSERT(env->GetTopIndex() >= 0); //!AB
                    env->Drop1();
                    break;
                }
                case 0x18:  // int
                {
                    env->Top().SetInt(env->Top().ToInt32(env));
                    break;
                }
                case 0x1C:  // get variable
                {                    
                    // Use a reference to Top() here for efficiency to avoid value assignment overhead,
                    // as this is a very frequently executed op. This value is overwritten by the result.
                    GASValue&   variable = env->Top();
                    GASString   varString(variable.ToString(env));

                    // ATTN: Temporary hack to recognize "NaN", "Infinity" and "-Infinity".
                    // We want this to be as efficient as possible, so do a separate branch for case sensitivity.
                    if (env->IsCaseSensitive())
                    {
                        if (varString == env->GetBuiltin(GASBuiltin_NaN))
                            variable.SetNumber(GASNumberUtil::NaN());
                        else if (varString == env->GetBuiltin(GASBuiltin_Infinity))
                            variable.SetNumber(GASNumberUtil::POSITIVE_INFINITY());
                        else if (varString == env->GetBuiltin(GASBuiltin_minusInfinity_)) // "-Infinity"
                            variable.SetNumber(GASNumberUtil::NEGATIVE_INFINITY());
                        else
                            if (!env->GetVariable(varString, &variable, &WithStack))
                                variable.SetUndefined();
                    }
                    else
                    {
                        if (env->GetBuiltin(GASBuiltin_NaN).CompareBuiltIn_CaseInsensitive(varString))
                            variable.SetNumber(GASNumberUtil::NaN());
                        else if (env->GetBuiltin(GASBuiltin_Infinity).CompareBuiltIn_CaseInsensitive(varString))
                            variable.SetNumber(GASNumberUtil::POSITIVE_INFINITY());
                        else if (env->GetBuiltin(GASBuiltin_minusInfinity_).CompareBuiltIn_CaseInsensitive(varString))
                            variable.SetNumber(GASNumberUtil::NEGATIVE_INFINITY());
                        else
                            if (!env->GetVariable(varString, &variable, &WithStack))
                                variable.SetUndefined();
                    }

                    if (verboseAction) 
                    {    
                        GASString sv(variable.ToDebugString(env));
                        if (!variable.ToObjectInterface(env))
                        {
                            log.LogAction("-- get var: %s=%s\n",
                                        varString.ToCStr(), sv.ToCStr());
                        }
                        else
                        {
                            log.LogAction("-- get var: %s=%s at %p\n",
                                        varString.ToCStr(), sv.ToCStr(),
                                        variable.ToObjectInterface(env));
                        }
                    }
                    
                    break;
                }
                case 0x1D:  // set variable
                {
                    env->SetVariable(env->Top1().ToString(env), env->Top(), &WithStack);
                    
                    if (verboseAction) 
                    {
                        GASString s1(env->Top1().ToDebugString(env));
                        log.LogAction("-- set var: %s \n", s1.ToCStr());
                    }

                    env->Drop2();
                    break;
                }
                case 0x20:  // set target expression (used with tellTarget)
                {               
                    GFxASCharacter* target = 0;

                    GASValue targetVal = env->Top();
                    if (!targetVal.IsString() && !targetVal.IsCharacter())
                    {
                        targetVal.SetString(targetVal.ToStringVersioned(env, version));
                    }

                    if (targetVal.IsString())
                    {
                        if (targetVal.ToString(env).IsEmpty())
                        {
                            // special case for empty string - originalTarget will be used
                            target = poriginalTarget;
                        }
                        else
                        {
                            // target is specified as a string, like /mc1/nmc1/mmm1
                            GASValue val;
                            env->GetVariable(env->Top().ToString(env), &val, &WithStack, &target);

                            if (verboseAction) 
                            {
                                GASString s1(env->Top().ToDebugString(env));

                                if (target && target->IsASCharacter())
                                    log.LogAction("-- ActionSetTarget2: %s (%d)\n",
                                                s1.ToCStr(),
                                                target->ToASCharacter()->GetId().GetIdIndex());
                                else
                                    log.LogAction("-- ActionSetTarget2: %s - no target found\n",
                                                s1.ToCStr());
                            }
                        }
                    }
                    else if (targetVal.IsCharacter())
                    {
                        target = env->Top().ToASCharacter(env);
                    }
                    else
                    {   
                        GASSERT(0);
                    }
                    
                    if (!target) 
                    {
                        if (verboseActionErrors)
                        {
                            GASString sv(targetVal.ToDebugString(env));                            
                            log.LogScriptError("Error: SetTarget2(tellTarget) with invalid target '%s'.\n",
                                               sv.ToCStr());
                        }
                        //!AB: if target is invalid, then set originalTarget as a target and
                        // mark it as "invalid" target. This means that frame-related
                        // functions (such as gotoAndStop, etc) should do nothing
                        // inside this tellTarget.
                        env->SetInvalidTarget(poriginalTarget);
                    }
                    else
                    {
                        env->SetTarget(target);
                    }
                    env->Drop (1);
                    break;
                }
                case 0x21:  // string concat
                {
                    env->Top1().ConvertToStringVersioned(env, version);
                    env->Top1().StringConcat(env, env->Top().ToStringVersioned(env, version));
                    env->Drop1();
                    break;
                }
                case 0x22:  // get property
                {

                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top1());
                    if (ptarget)
                    {
                        // Note: GetStandardMember does bounds checks for opcodes.
                        GFxASCharacter::StandardMember member =
                                (GFxASCharacter::StandardMember) env->Top().ToInt32(env);
                        ptarget->GetStandardMember(member, &env->Top1(), 1);
                    }
                    else
                    {
                        env->Top1() = GASValue();
                    }
                    env->Drop1();
                    break;
                }

                case 0x23:  // set property
                {
                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top(2));
                    if (ptarget)
                    {
                        // Note: SetStandardMember does bounds checks for opcodes.
                        GFxASCharacter::StandardMember member =
                                (GFxASCharacter::StandardMember) env->Top1().ToInt32(env);
                        ptarget->SetStandardMember(member, env->Top(), 1);
                    }
                    env->Drop3();
                    break;
                }

                case 0x24:  // duplicate Clip (sprite?)
                {
                    // MA: The target object can be a string, a path, or an object. 
                    // The duplicated clip will be created within its parent.
                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top(2));
                    if (ptarget)
                    {
                        ptarget->CloneDisplayObject(
                                    env->Top1().ToString(env),
                                    env->Top().ToInt32(env) + 16384, 0);
                    }
                    env->Drop3();
                    break;
                }

                case 0x25:  // remove clip
                {               
                    GFxASCharacter* ptarget = env->FindTargetByValue(env->Top());
                    if (ptarget)
                    {
                        if (ptarget->GetDepth() < 16384)
                        {
                            log.LogScriptWarning("removeMovieClip(\"%s\") failed - depth must be >= 0\n",
                                                    ptarget->GetName().ToCStr());
                            return;
                        }
                        ptarget->RemoveDisplayObject();
                    }
                    env->Drop1();
                    break;
                }
                    

                case 0x26:  // trace
                {
                    // Log the stack val.
                    GAS_GlobalTrace(GASFnCall(&env->Top(), NULL, env, 1, env->GetTopIndex()));
                    env->Drop1();
                    break;
                }

                case 0x27:  // start drag GFxASCharacter
                {
                    GFxMovieRoot::DragState st;
                    bool lockCenter = env->Top1().ToBool(env);

                    st.pCharacter = env->FindTargetByValue(env->Top());
                    if (st.pCharacter == NULL)
                    {
                        if (verboseActionErrors)
                        {
                            GASString s0(env->Top().ToDebugString(env));
                            log.LogScriptError("Error: StartDrag of invalid target '%s'.\n",
                                               s0.ToCStr());
                        }
                    }

                    st.Bound = env->Top(2).ToBool(env);
                    if (st.Bound)
                    {
                        st.BoundLT.x = GFC_PIXELS_TO_TWIPS((Float) env->Top(6).ToNumber(env));
                        st.BoundLT.y = GFC_PIXELS_TO_TWIPS((Float) env->Top(5).ToNumber(env));
                        st.BoundRB.x = GFC_PIXELS_TO_TWIPS((Float) env->Top(4).ToNumber(env));
                        st.BoundRB.y = GFC_PIXELS_TO_TWIPS((Float) env->Top(3).ToNumber(env));
                        env->Drop(4);
                    }

                    // Init mouse offsets based on LockCenter flag.
                    st.InitCenterDelta(lockCenter);

                    env->Drop3();

                    GFxMovieRoot* pmovieRoot = env->GetTarget()->GetMovieRoot();
                    GASSERT(pmovieRoot);

                    if (pmovieRoot && st.pCharacter)                
                        pmovieRoot->SetDragState(st);   
                    
                    break;
                }

                case 0x28:  // stop drag GFxASCharacter
                {
                    GFxMovieRoot* pmovieRoot = env->GetTarget()->GetMovieRoot();
                    GASSERT(pmovieRoot);

                    pmovieRoot->StopDrag();
                    break;
                }

                case 0x29:  // string less than
                {
                    env->Top1().SetBool(env->Top1().ToStringVersioned(env, version) < env->Top().ToStringVersioned(env, version));
                    //!AB: as I understand we need to Drop1() here
                    env->Drop (1);
                    break;
                }

                case 0x2A:  // throw
                {
                    if (verboseActionErrors) log.LogScriptError("Error: Unsupported opcode %02X\n", actionId);
                    break;
                }

                case 0x2B:  // CastObject
                {
                    // Pop object, pop constructor function
                    // Make sure o1 is an instance of s2.
                    // If the cast succeeds, push object back, else push NULL.

                    const GASValue& objVal = env->Top();
                    const GASValue& ctorFuncVal = env->Top1();
                    GASValue rv(GASValue::NULLTYPE);

                    if (ctorFuncVal.IsFunction())
                    {
                        GASFunctionRef ctorFunc = ctorFuncVal.ToFunction();
                        if (!ctorFunc.IsNull())
                        {
                            GASObjectInterface* obj = objVal.ToObjectInterface(env);
                            if (obj != 0)
                            {
                                GASValue prototypeVal;
                                if (ctorFunc->GetMemberRaw(env->GetSC(), env->GetBuiltin(GASBuiltin_prototype), &prototypeVal))
                                {
                                    GASObject* prototype = prototypeVal.ToObject();
                                    if (obj->InstanceOf(env, prototype))
                                    {
                                        rv.SetAsObjectInterface(obj);
                                    }
                                }
                                else
                                {
                                    if (verboseActionErrors)
                                        log.LogScriptError("Error: The constructor function in 'cast' should have 'prototype'.\n");
                                }
                            }
                        }
                    }
                    else
                    {
                        if (verboseActionErrors)
                            log.LogScriptError("Error: The parameter of 'cast' should be a function.\n");
                    }

                    env->Drop2();
                    env->Push(rv);
                    break;
                }

                case 0x2C:  // implements
                {
                    // Declare that a class s1 implements one or more
                    // Interfaces (i2 == number of interfaces, s3..Sn are the names
                    // of the interfaces).
                    GASValue    ctorFuncVal(env->Top());
                    int         intfNum = env->Top1().ToInt32(env);
                    env->Drop2();

                    if (ctorFuncVal.IsFunction())
                    {
                        GASFunctionRef ctorFunc = ctorFuncVal.ToFunction();
                        if (!ctorFunc.IsNull())
                        {
                            GASValue protoVal;
                            if (ctorFunc->GetMemberRaw(env->GetSC(), env->GetBuiltin(GASBuiltin_prototype), &protoVal))
                            {
                                GASObject* proto = protoVal.ToObject();
                                if (proto != 0)
                                {
                                    proto->AddInterface(env->GetSC(), intfNum, NULL);
                                    for (int i = 0; i < intfNum; ++i)
                                    {
                                        const GASValue& intfVal = env->Top(i);
                                        if (intfVal.IsFunction())
                                        {
                                            GASFunctionRef intfFunc = intfVal.ToFunction();
                                            if (!intfFunc.IsNull())
                                            {
                                                proto->AddInterface(env->GetSC(), i, intfFunc.GetObjectPtr());
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (verboseActionErrors)
                                    log.LogScriptError("Error: The constructor function in 'implements' should have 'prototype'.\n");
                            }
                        }
                    }
                    else
                    {
                        if (verboseActionErrors)
                            log.LogScriptError("Error: The parameter of 'implements' should be a function.\n");
                    }
                    env->Drop(intfNum);
                    break;
                }

                case 0x30:  // random
                {
                    int max = env->Top().ToInt32(env);
                    if (max < 1) 
                        max = 1;
                    env->Top().SetInt(GFxRandom::NextRandom() % max);
                    break;
                }
                case 0x31:  // mb length
                {
                    // Must use version check here, for SWF <= 6, "undefined" length is 0.              
                    int length = env->Top().ToStringVersioned(env, version).GetLength();
                    env->Top().SetInt(length);
                    break;
                }
                case 0x32:  // ord
                {
                    // ASCII code of first GFxCharacter
                    env->Top().SetInt(env->Top().ToString(env)[0]);
                    break;
                }
                case 0x33:  // chr
                {
                    char    buf[2];
                    buf[0] = char(env->Top().ToInt32(env));
                    buf[1] = 0;
                    env->Top().SetString(env->CreateString(buf));
                    break;
                }

                case 0x34:  // get timer
                    // Push milliseconds since we started playing.
                    env->Push((GASNumber)floorf(env->GetTarget()->GetMovieRoot()->GetTimer() * 1000.0f));
                    break;

                case 0x35:  // mbsubstring
                {               
                    GASString retVal(GASStringProto::StringSubstring(
                                        env->Top(2).ToStringVersioned(env, version),
                                        env->Top1().ToInt32(env) - 1,
                                        env->Top().ToInt32(env) ));
                    env->Drop2();
                    env->Top().SetString(retVal);
                    break;
                }

                case 0x36: // mbord
                {
                    // Convert first character to its numeric value.
                    // MA: This is correct for SWF6+ versions, but there is also some strange rounding
                    // that occurs if SWF 5 is selected. (i.e. mbord(mbchr(450)) returns 1). TBD.
                    GASString s(env->Top().ToStringVersioned(env, version));
                    // Note: GetUTF8CharAt does bounds check, so return 0 to avoid accessing empty string.
                    env->Top().SetInt( (s.GetSize() == 0) ? 0 : s.GetCharAt(0) );
                    break;
                }

                case 0x37:  // mbchr
                {
                    // Convert number to a multi-byte aware character string
                    wchar_t buff[2];
                    buff[0] = (wchar_t)env->Top().ToUInt32(env);
                    buff[1] = 0;
                    env->Top().SetString(env->CreateString(buff));
                    break;
                }
                case 0x3A:  // delete
                {                   
                    // This is used to remove properties from an object.
                    // AB: For Flash 6, if Top1 is null or undefined then delete works by the same way
                    // as delete2
                    bool retVal = false;
                    const GASValue& top1 = env->Top1();
                    if (version <= 6 && (top1.IsNull() || top1.IsUndefined()))
                    {
                        const GASString& memberName = env->Top().ToString(env);
                        GASValue ownerVal;
                        if (env->FindOwnerOfMember(memberName, &ownerVal, &WithStack))
                        {
                            GASObjectInterface *piobj = ownerVal.ToObjectInterface(env);
                            if (piobj)
                            {                                       
                                retVal = piobj->DeleteMember(env->GetSC(), memberName);
                            }
                        }
                    }
                    else
                    {
                        GASObjectInterface *piobj = env->Top1().ToObjectInterface(env);
                        if (piobj)
                            retVal = piobj->DeleteMember(env->GetSC(), env->Top().ToString(env));
                    }
                    // Delete pops two parameters and push the return value (bool) into the stack.
                    // Usually it is removed by explicit "pop" operation, if return value is not used.
                    env->Drop1();
                    env->Top().SetBool(retVal);
                    break;
                }
                case 0x3B:  // delete2
                {
                    GASString           memberName(env->Top().ToString(env));
                    GASValue            ownerVal;
                    GASObjectInterface *piobj = NULL;

                    // !AB: 'delete' may be invoked with string parameter containing a whole
                    // path to the member being deleted: delete "obj.mem1.mem2". Or,
                    // it might be used with eval: delete eval("/anObject:member")
                    bool found = false;
                    if (memberName.IsNotPath() || !env->IsPath(memberName))
                    {
                        found = env->FindOwnerOfMember(memberName, &ownerVal, &WithStack);
                    }
                    else
                    {
                        // In the case, if path is specified we need to resolve it,
                        // get the owner and modify the memberName to be relative to the
                        // found owner.
                        bool retVal = env->FindVariable(memberName, NULL, &WithStack, NULL, &ownerVal, false, &memberName);
                        found = (retVal && !ownerVal.IsUndefined());
                    }

                    if (found)
                    {
                        piobj = ownerVal.ToObjectInterface(env);
                        if (piobj)
                        {                                       
                            env->Top().SetBool(piobj->DeleteMember(env->GetSC(), memberName));
                        }
                    }
                    if (!piobj)
                    {
                        env->Top().SetBool(false);
                    }
                    break;
                }

                case 0x3C:  // set local
                {                 
                    GASString   varname(env->Top1().ToString(env));

                    // This is used in 'var' declarations.
                    // For functions, the value becomes local; for frame actions and events, it is global to clip.
                    if (isFunction2 || (execType == GASActionBuffer::Exec_Function))
                        env->SetLocal(varname, env->Top());
                    else
                        env->SetVariable(varname, env->Top(), &WithStack);

                    env->Drop2();
                    break;
                }

                case 0x3D:  // call function
                {
                    GASValue    function;
                    GASValue    owner;
                    bool        invokeFunction=true; 

                    if (env->Top().GetType() == GASValue::STRING)
                    {
                        // Function is a string; lookup the function.
                        const GASString functionName(env->Top().ToString(env));
                        env->GetVariable(functionName, &function, &WithStack, 0, &owner);

                        if (!function.IsFunction())
                        {
                            GPtr<GASObject> obj;
                            if (function.IsObject() && (obj = function.ToObject()) && obj->IsSuper())
                            {
                                // special case for "super": call to super.__constructor__ (SWF6+)
                                function = obj->Get__constructor__(env->GetSC());
                                //printf ("!!! function = %s\n", (const char*)env->GetGC()->FindClassName(function.ToObject()));
                                owner.SetAsObject(obj);
                            }
                            else
                            {
                                if (verboseActionErrors)
                                    log.LogScriptError("Error: CallFunction - '%s' is not a function\n",
                                        functionName.ToCStr());
                                invokeFunction=false;
                            }
                        }
                    }
                    else
                    {
                        // Hopefully the actual function object is here.
                        function = env->Top();
                    }

            
                    int nargs = (int) env->Top1().ToNumber(env);
                    GASValue    result;
                    if(invokeFunction)
                    {           
                        //!AB, Sometimes methods can be invoked as functions. For example,
                        // a movieclip "mc" has the method "foo" (mc.foo). If this method
                        // is invoked from "mc"'s timeline as "foo();" (not "this.foo();"!)
                        // then call_func opcode (0x3D) will be generated instead of call_method (0x52).
                        // In this case, "this" should be set to current target ("mc").
                        // Similar issue exists, if method is invoked from "with" statement:
                        // with (mc) { foo(); } // inside foo "this" should be set to "mc"
                        GASObjectInterface *ownerObj = owner.ToObjectInterface(env);
                        GAS_Invoke(function, &result, ownerObj, env, nargs, env->GetTopIndex() - 2);
                    }
                    env->Drop(nargs + 1);
                    env->Top() = result;

                    break;
                }
                case 0x3E:  // return
                {
                    // Put top of stack in the provided return slot, if
                    // it's not NULL.
                    if (retval)
                    {
                        *retval = env->Top();
                    }
                    env->Drop1();

                    // Skip the rest of this Buffer (return from this GASActionBuffer).
                    pc = StopPc;

                    break;
                }
                case 0x3F:  // modulo
                {
                    GASNumber   result = 0;
                    GASNumber   y = env->Top().ToNumber(env);
                    GASNumber   x = env->Top1().ToNumber(env);
                    if (y != 0)
                    {
    //                  env->Top1().SetNumber(fmod(env->Top1().ToBool(env) && env->Top().ToBool(env));
    //                  env->Drop1();
                        result = (GASNumber) fmod(x, y);
                    }
    //              log.LogScriptError("modulo x=%f, y=%f, z=%f\n",x,y,result.ToNumber(env));

                    env->Drop2();
                    env->Push(result);
                    break;
                }
                case 0x40:  // new
                {                    
                    GASString   classname(env->Top().ToString(env));
                    int         nargs = (int) env->Top1().ToNumber(env);
                    env->Drop2();

                    if (verboseAction) 
                        log.LogAction("---new object: %s\n", classname.ToCStr());
                    
                    GPtr<GASObject> pnewObj;
                    GASValue constructor;

                    if (env->GetVariable(classname, &constructor, &WithStack) && constructor.IsFunction())
                    {
                        GASFunctionRef func = constructor.ToFunction ();
                        GASSERT (!func.IsNull ());

                        pnewObj = env->OperatorNew(func, nargs);

                        if (!pnewObj)
                        {
                            if (verboseActionErrors)
                                log.LogScriptError("Error: can't create object with unknown class '%s'\n",
                                                   classname.ToCStr());
                        }
                    }
                    else
                    {
                        if (verboseActionErrors)
                            log.LogScriptError("Error: can't create object with unknown class '%s'\n",
                                               classname.ToCStr());
                    }

                    env->Drop(nargs);
                    env->Push(pnewObj);
    #if 1
                    if (verboseAction)
                        log.LogAction("New object %s at %p\n", classname.ToCStr(), pnewObj.GetPtr());
    #endif
                    break;
                }
                case 0x41:  // declare local
                {
                    const GASString varname(env->Top().ToString(env));
                    // This is used in 'var' declarations.
                    // For functions, the value becomes local; for frame actions and events, it is global to clip.
                    if (isFunction2 || (execType == GASActionBuffer::Exec_Function))                
                        env->DeclareLocal(varname);
                    env->Drop1();
                    break;
                }
                case 0x42:  // init array
                {               
                    int ArraySize = (int) env->Top().ToNumber(env);
                    env->Drop1();

                    // Call the array constructor with the given elements as args
                    GASValue    result;
                    GASArrayProto::DeclareArray(GASFnCall(&result, NULL, env, ArraySize, env->GetTopIndex()));

                    if (ArraySize > 0) 
                    {
                        env->Drop(ArraySize);
                    }
                    // Push array 
                    env->Push(result);
                    break;
                }

                case 0x43:  // declare object
                {
                    // Use an initialize list to build an object.
                    int             argCount    = (int) env->Top().ToNumber(env);
                    GPtr<GASObject> pobj        = env->OperatorNew(env->GetBuiltin(GASBuiltin_Object));
                    env->Drop1();

                    for(int i=0; i< argCount; i++)
                    {
                        // Pop {value, name} pairs.
                        if (env->GetTopIndex() >=1)
                        {
                            // Validity/null check.
                            if (env->Top1().GetType() == GASValue::STRING)
                                pobj->SetMember(env, env->Top1().ToString(env), env->Top());
                            env->Drop2();
                        }
                    }

                    env->Push(pobj.GetPtr());
                    break;
                }
                    
                case 0x44:  // type of
                {
                    GASBuiltinType typeStrIndex = GASBuiltin_undefined;
                    const GASValue& top = env->Top();
                    switch(top.GetType())
                    {
                        case GASValue::UNDEFINED:
                            //typeStrIndex = GASBuiltin_undefined;                            
                            break;
                        case GASValue::STRING:
                            typeStrIndex = GASBuiltin_string;
                            break;
                        case GASValue::BOOLEAN:
                            typeStrIndex = GASBuiltin_boolean;
                            break;
                        case GASValue::CHARACTER:
                            {                           
                                GFxASCharacter* pchar = env->Top().ToASCharacter(env);
                                // ActionScript returns "movieclip" for null clips and sprite types only.
                                if (!pchar || (pchar && pchar->IsSprite()))
                                    typeStrIndex = GASBuiltin_movieclip;
                                else
                                    typeStrIndex = GASBuiltin_object;
                            }
                            break;

                        case GASValue::OBJECT:
                            typeStrIndex = GASBuiltin_object;
                            break;
                        case GASValue::NULLTYPE:
                            typeStrIndex = GASBuiltin_null;
                            break;
                        case GASValue::FUNCTION:
                            typeStrIndex = GASBuiltin_function;
                            break;
                        default:
                            if (top.IsNumber())
                                typeStrIndex = GASBuiltin_number;
                            else
                            {
                                if (verboseActionErrors)
                                    log.LogScriptError("Error: typeof unknown: %02X\n", env->Top().GetType());
                            }
                            break;
                        }

                    env->Top().SetString(env->GetBuiltin(typeStrIndex));
                    break;
                }

                case 0x45:  // get target path
                {
                    GFxASCharacter* pcharacter = env->Top().ToASCharacter(env);
                    if (pcharacter)             
                        env->Top().SetString(pcharacter->GetCharacterHandle()->GetNamePath());
                    else
                        env->Top().SetUndefined();
                    break;
                }

                case 0x46:  // enumerate
                case 0x55:  // enumerate object 2
                {                   
                    const GASObjectInterface* piobj   = 0;
                    GASValue                  varName = env->Top();
                    env->Drop1();

                    // The end of the enumeration
                    GASValue nullvalue;
                    nullvalue.SetNull();
                    env->Push(nullvalue);
            
                    if (actionId == 0x55)
                    {   
                        // For opcode 0x55, top of stack IS the object.                 
                        piobj = varName.ToObjectInterface(env);
                        if (!piobj)
                            break;
                    }
                    else
                    {
                        GASString   varString(varName.ToString(env));
                        GASValue    variable;
                        if (env->GetVariable(varString, &variable, &WithStack))
                        {
                            piobj = variable.ToObjectInterface(env);
                            if (!piobj)
                                break;
                        }
                        else
                            break;
                    }

                    if (verboseAction) 
                        log.LogAction("---enumerate - Push: NULL\n");

                    // Use enumerator class to handle members other then GASObject.
                    // For example, we may enumerate members added to sprite.
                    struct EnumerateOpVisitor : public GASObjectInterface::MemberVisitor
                    {
                        GASEnvironment*     pEnv;
                        GFxActionLogger*    pLog;

                        EnumerateOpVisitor(GASEnvironment* penv, GFxActionLogger* plog)
                        { pEnv = penv; pLog = plog; }
                        virtual void    Visit(const GASString& name, const GASValue& val, UByte flags)
                        {
                            GUNUSED2(val, flags);
                            pEnv->Push(name);
                            pLog->LogAction("---enumerate - Push: %s\n", name.ToCStr());
                        }
                    };

                    // Visit all members, including prototype & child clips.
                    EnumerateOpVisitor memberVisitor(env, &log);
                    piobj->VisitMembers(env->GetSC(), &memberVisitor, 
                                        GASObjectInterface::VisitMember_Prototype|
                                        GASObjectInterface::VisitMember_ChildClips);
                    break;
                }

                case 0x47:  // AddT (typed)
                {
                    env->Top1().Add (env, env->Top());
                    env->Drop1();
                    break;
                }
                case 0x48:  // less Than (typed)
                {
                    env->Top1() = env->Top1().Compare (env, env->Top(), -1);
                    env->Drop1();
                    break;
                }
                case 0x49:  // Equal (typed)
                {
                    // @@ identical to untyped equal, as far as I can tell...
                    env->Top1().SetBool(env->Top1().IsEqual (env, env->Top()));
                    env->Drop1();
                    break;
                }
                case 0x4A:  // to number
                {
                    env->Top().ConvertToNumber(env);
                    break;
                }
                case 0x4B:  // to string
                {
                    env->Top().ConvertToStringVersioned(env, version);
                    break;
                }
                case 0x4C:  // dup
                    env->Push(env->Top());
                    break;
                
                case 0x4D:  // swap
                {
                    GASValue    temp = env->Top1();
                    env->Top1() = env->Top();
                    env->Top() = temp;
                    break;
                }
                case 0x4E:  // get member
                {               
                    // Use a reference to Top1 access since stack does not change here,
                    // and this is one of the most common ops.
                    GASValue&          top1Ref = env->Top1();
                    GASObjectInterface *pobj = top1Ref.ToObjectInterface(env);
        
                    if (!pobj)
                    {
                        // get member for non-object case
                        // try to create temporary object and get member from it.
                        GASValue objVal = env->PrimitiveToTempObject(1);
                        if ((pobj = objVal.ToObject())!=0)
                        {
                            if (!pobj->GetMember(env, env->Top().ToString(env), &top1Ref))
                            {
                                top1Ref.SetUndefined();
                            }
                        }
                        else
                        {
                            top1Ref.SetUndefined();
                        }
                    }
                    else
                    {
                        // Save object ref just in case its released by SetUndefined; does not matter for characters.
                        GPtr<GASObject> pobjSave;
                        if (top1Ref.IsObject() || top1Ref.IsFunction()) // Avoid debug warning.
                            pobjSave = top1Ref.ToObject();

                        top1Ref.SetUndefined();

                        //printf ("!!! pobj.__proto__ = %s\n", (const char*)env->GetGC()->FindClassName(pobj->Get__proto__(env)));
                        GASString member(env->Top().ToString(env));
                        pobj->GetMember(env, member, &top1Ref);

                        if (verboseAction) 
                        {     
                            GASString s1(top1Ref.ToDebugString(env));

                            if (top1Ref.ToObjectInterface(env) == NULL)
                            {                           
                                log.LogAction("-- GetMember %s=%s\n",
                                            member.ToCStr(), s1.ToCStr());
                            }
                            else
                            {                           
                                log.LogAction("-- GetMember %s=%s at %p\n",
                                            member.ToCStr(), s1.ToCStr(),
                                            top1Ref.ToObjectInterface(env));
                            }
                        }
                    }

                    env->Drop1();
                    break;                  
                }
                case 0x4F:  // set member
                {
                    GASObjectInterface* piobj = env->Top(2).ToObjectInterface(env);
                    if (piobj)
                    {
                        GASString member(env->Top1().ToString(env));
                        piobj->SetMember(env, member, env->Top());

                        if (verboseAction)
                        {
                            GASString s2(env->Top(2).ToDebugString(env));
                            GASString s0(env->Top().ToDebugString(env));

                            log.LogAction("-- SetMember %s.%s=%s\n",
                                    s2.ToCStr(), member.ToCStr(), s0.ToCStr());
                        }
                    }
                    else
                    {
                        // Invalid object, can't set.
                        if (verboseAction)
                        {
                            GASString s2(env->Top(2).ToDebugString(env));
                            GASString s1(env->Top1().ToDebugString(env));
                            GASString s0(env->Top().ToDebugString(env));

                            log.LogAction(
                                    "- SetMember %s.%s=%s on invalid object\n",
                                    s2.ToCStr(), s1.ToCStr(), s0.ToCStr());
                        }
                    }
                    env->Drop3();
                    break;
                }
                case 0x50:  // increment
                    env->Top().Add(env, 1);
                    break;
                case 0x51:  // decrement
                    env->Top().Sub(env, 1);
                    break;
                case 0x52:  // call method
                {
                    int nargs = (int) env->Top(2).ToNumber(env);
                    GASValue    result;

                    GASString           methodName(env->Top().ToString(env));
                    GASObjectInterface* piobj = 0;

                    // If the method name is blank or undefined, the object is taken to be a function object
                    // that should be invoked.
                    if (env->Top().IsUndefined() || methodName.IsEmpty())
                    {
                        //!AB
                        // This code is invoked when nested function is called from parent function, for example:
                        //foo = function() {
                        //  var local = 10;
                        //  var nestedFoo = function () {}
                        //  nestedFoo(); // <- here
                        //}
                        // NOTE: in this case, "this" inside the "nestedFoo" is reported as "undefined",
                        // but this is not true: it is kinda "stealth" object. typeof(this) says this is 
                        // an "object". It is possible to get access to "local" by using "this.local". Also,
                        // it is possible to create local variable by setting "this.newLocal = 100;"
                        GASValue    function = env->Top1();

                        GPtr<GASObject> obj;
                        GASValue thisVal;

                        if (function.IsObject() && (obj = function.ToObject()) && obj->IsSuper())
                        {
                            // special case for "super": call to super.__constructor__
                            function = obj->Get__constructor__(env->GetSC());
                            thisVal.SetAsObject(obj);
                        }
                        else
                        {
                            env->GetVariable(env->GetBuiltin(GASBuiltin_this), &thisVal, &WithStack);
                        }

                        if (function.IsFunction ())
                        {
                            GAS_Invoke(function, &result, thisVal, env, nargs, env->GetTopIndex() - 3);
                        }
                        else
                        {
                            if (verboseActionErrors)
                                log.LogScriptError("Error: CallMethod \"as a function\" - pushed object is not a function object\n");
                        }
                    }
                    
                    // Object or character.
                    else if (!env->Top1().IsFunction() && (piobj = env->Top1().ToObjectInterface(env)) != 0)
                    {
                        GASValue    method;

                        // check, is the piobj super or not. If yes, get the correct "this" (piobj)
                        if (piobj->IsSuper())
                        {
                            //!AB: looks like we are calling something like "super.func()".
                            // The super object now contains a __proto__ pointing to the nearest
                            // base class. But, the nearest base class might not have the "func()"
                            // method, but the base of base call might have. 
                            // In this case we need to find the appropriate base class containing
                            // the calling method. Once we found it we set it as alternative prototype
                            // to the super object. It will be reseted to original prototype automatically 
                            // after call is completed.
                            GPtr<GASSuperObject> superObj = static_cast<GASSuperObject*>(piobj);
                            //printf ("!!! superObj->GetSuperProto() = %s\n", (const char*)env->GetGC()->FindClassName(superObj->GetSuperProto()));
                            GPtr<GASObject> newProto = superObj->GetSuperProto()->FindOwner(env->GetSC(), methodName);
                            if (newProto)
                            {
                                //printf ("!!! newProto = %s\n", (const char*)env->GetGC()->FindClassName(newProto));
                                superObj->SetAltProto(newProto);
                            }
                            else
                                piobj = 0; // looks like there is no such method in super class
                        }
                        //printf ("!!! piobj.__proto__ = %s\n", (const char*)env->GetGC()->FindClassName(piobj->Get__proto__(env)));

                        if (piobj)
                        {
                            if (piobj->GetMember(env, methodName, &method))
                            {
                                if (!method.IsFunction())
                                {
                                    if (verboseActionErrors)
                                        log.LogScriptError("Error: CallMethod - '%s' is not a method\n",
                                            methodName.ToCStr());
                                }
                                else
                                {
                                    GAS_Invoke(
                                        method,
                                        &result,
                                        piobj,
                                        env,
                                        nargs,
                                        env->GetTopIndex() - 3);
                                }
                            }
                            else
                            {
                                if (verboseActionErrors)
                                    log.LogScriptError("Error: CallMethod - can't find method %s\n",
                                        methodName.ToCStr());
                            }
                        }
                    }
                    else if (env->Top1().IsFunction())
                    {
                        // Looks like we want to call a static method.

                        // get the function object first
                        const GASValue& thisVal = env->Top1();
                        piobj = thisVal.ToObject();
                        if (piobj != 0) 
                        {
                            GASValue method;
                            if (piobj->GetMember(env, methodName, &method))
                            {                                                       
                                // invoke method
                                GAS_Invoke(
                                    method,
                                    &result,
                                    thisVal,
                                    env,
                                    //NULL, // this is null for static calls
                                    nargs,
                                    env->GetTopIndex() - 3);
                            }
                            else
                            {
                                if (verboseActionErrors)
                                    log.LogScriptError("Error: Static method '%s' is not found.\n",
                                        methodName.ToCStr());
                            }
                        }
                        else 
                        {
                            if (verboseActionErrors)
                                log.LogScriptError("Error: Function is not an object for static method '%s'.\n",
                                    methodName.ToCStr());
                        }
                    }
                    else
                    {
                        // Handle methods on primitive types (string, boolean, number).
                        // In this case, an appropriate temporary object is created
                        // (String, Boolean, Number) and method is called for it.
                        GASValue objVal = env->PrimitiveToTempObject(1);
                        if (objVal.IsObject())
                        {
                            piobj = objVal.ToObject();
                            GASValue method;
                            if (piobj->GetMember(env, methodName, &method))
                            {
                                if (method.GetType() != GASValue::FUNCTION)
                                {
                                    if (verboseActionErrors)
                                        log.LogScriptError("Error: CallMethod - '%s' is not a method\n",
                                            methodName.ToCStr());
                                }
                                else
                                {
                                    GAS_Invoke(
                                        method,
                                        &result,
                                        piobj,
                                        env,
                                        nargs,
                                        env->GetTopIndex() - 3);
                                }
                            }
                            else
                            {
                                if (verboseActionErrors)
                                    log.LogScriptError("Error: CallMethod - can't find method %s\n",
                                        methodName.ToCStr());
                            }
                        }
                        else
                        {
                            if (verboseActionErrors)
                                log.LogScriptError("Error: CallMethod - '%s' on invalid object.\n",
                                    methodName.ToCStr());
                        }
                    }
                    env->Drop(nargs + 2);
                    env->Top() = result;
                    break;
                }
                case 0x53:  // new method
                {
                    GASValue    constructorName(env->Top());
                    GASValue    object(env->Top1());
                    int         nargs = (int) env->Top(2).ToNumber(env);
                    env->Drop3();

                    GASValue    constructor;
                    GASString   constructorNameStr(constructorName.ToString(env));

                    if (verboseAction) 
                        log.LogAction("---new method: %s\n", constructorNameStr.ToCStr());
                    
                    if (constructorName.IsUndefined() || (constructorName.IsString() && constructorNameStr.IsEmpty()))
                    {
                        // if constructor method's name is blank use the "object" as function object
                        constructor = object.ToFunction();
                    }
                    else
                    {
                        // MA: Can object be null?
                        // get the method
                        GASObjectInterface* piobj = object.ToObjectInterface(env);
                        if (!piobj || !piobj->GetMember(env, constructorNameStr, &constructor))
                        {
                            // method not found!
                            if (verboseActionErrors)
                                log.LogScriptError("Error: Method '%s' is not found.\n",
                                    constructorNameStr.ToCStr());
                        }
                    }

                    GPtr<GASObject> pnewObj;
                    if (constructor.IsFunction())
                    {
                        GASFunctionRef func = constructor.ToFunction ();
                        GASSERT (func != NULL);

                        pnewObj = env->OperatorNew(func, nargs);
                    }
                    else
                    {
                        if (verboseActionErrors)
                            log.LogScriptError("Error: can't create object with unknown ctor\n");
                    }

                    env->Drop(nargs);
                    env->Push(GASValue(pnewObj));
                    if (verboseAction) 
                        log.LogAction("New object created at %p\n", pnewObj.GetPtr());
                    break;
                }
                case 0x54:  // instance of
                {
                    const GASValue& ctorFuncVal = env->Top();
                    const GASValue& objVal = env->Top1();
                    bool rv = false;

                    if (ctorFuncVal.IsFunction())
                    {
                        GASFunctionRef ctorFunc = ctorFuncVal.ToFunction();
                        if (!ctorFunc.IsNull())
                        {
                            GASObjectInterface* obj = objVal.ToObjectInterface(env);
                            if (obj != 0)
                            {
                                GASValue prototypeVal;
                                if (ctorFunc->GetMemberRaw(env->GetSC(), env->GetBuiltin(GASBuiltin_prototype), &prototypeVal))
                                {
                                    GASObject* prototype = prototypeVal.ToObject();
                                    rv = obj->InstanceOf(env, prototype);
                                }
                                else
                                {
                                    if (verboseActionErrors)
                                        log.LogScriptError("Error: The constructor function in InstanceOf should have 'prototype'.\n");
                                }
                            }
                        }
                    }
                    else
                    {
                        if (verboseActionErrors)
                            log.LogScriptError("Error: The parameter of InstanceOf should be a function.\n");
                    }

                    env->Drop2();
                    env->Push(rv);
                    break;
                }                
                //case 0x55: // enumerate object 2: see op code 0x46
                case 0x60:  // bitwise and
                    env->Top1().And (env, env->Top());
                    env->Drop1();
                    break;
                case 0x61:  // bitwise or
                    env->Top1().Or (env, env->Top());
                    env->Drop1();
                    break;
                case 0x62:  // bitwise xor
                    env->Top1().Xor (env, env->Top());
                    env->Drop1();
                    break;
                case 0x63:  // shift left
                    env->Top1().Shl(env, env->Top());
                    env->Drop1();
                    break;
                case 0x64:  // shift Right (signed)
                    env->Top1().Asr(env, env->Top());
                    env->Drop1();
                    break;
                case 0x65:  // shift Right (unsigned)
                    env->Top1().Lsr(env, env->Top());
                    env->Drop1();
                    break;
                case 0x66:  // strict equal
                    if (!env->Top1().TypesMatch(env->Top()))
                    {
                        // Types don't match.
                        env->Top1().SetBool(false);
                        env->Drop1();
                    }
                    else
                    {
                        env->Top1().SetBool(env->Top1().IsEqual (env, env->Top()));
                        env->Drop1();
                    }
                    break;
                case 0x67:  // Gt (typed)
                    env->Top1() = env->Top1().Compare (env, env->Top(), 1);
                    env->Drop1();
                    break;
                case 0x68:  // string gt
                    env->Top1().SetBool(env->Top1().ToStringVersioned(env, version) > env->Top().ToStringVersioned(env, version));
                    env->Drop1();
                    break;

                case 0x69:  // extends
                {   
                    // Extends actually does the following:
                    // Pop(Superclass)
                    // Pop(Subclass)
                    // Subclass.prototype = new Object();
                    // Subclass.prototype.__proto__ = Superclass.prototype;
                    // Subclass.prototype.__constructor__ = Superclass;                 
                    GASValue superClassCtorVal = env->Top();
                    GASValue subClassCtorVal = env->Top1();
                    GASFunctionRef superClassCtor = superClassCtorVal.ToFunction();
                    GASFunctionRef subClassCtor = subClassCtorVal.ToFunction();
                    if (!superClassCtor.IsNull() && !subClassCtor.IsNull())
                    {
                        GASValue superProtoVal;
                        if (!superClassCtor->GetMemberRaw(env->GetSC(), env->GetBuiltin(GASBuiltin_prototype), &superProtoVal) ||
                            !superProtoVal.IsObject())
                        {
                            if (verboseActionErrors)
                                log.LogScriptError("Error: can't extends by the class w/o prototype.\n");
                        }
                        else
                        {
                            GPtr<GASObject> superProto = superProtoVal.ToObject();
                            //!AB: need to create instance of ObjectProto to make sure "implements" and
                            // "instanceof" work correctly.
                            GPtr<GASObject> newSubclassProto = *new GASObjectProto(env->GetSC(), superProto);
                            //GPtr<GASObject> newSubclassProto = *new GASObject(env->GetSC(), superProto);

                            subClassCtor->SetMemberRaw(env->GetSC(), env->GetBuiltin(GASBuiltin_prototype), GASValue(newSubclassProto));
                            newSubclassProto->Set__constructor__(env->GetSC(), superClassCtor);
                        }
                    }
                    else 
                    {
                        if (superClassCtor.IsNull())
                            if (verboseActionErrors)
                                log.LogScriptError("Error: can't extends with unknown super class.\n");
                        else
                            if (verboseActionErrors)
                                log.LogScriptError("Error: can't extends the unknown class.\n");
                    }

                    env->Drop2();
                    break;
                }
            }
            pc++;   // advance to next action.
        }
        else
        {
            if (verboseAction) 
                log.LogAction("EX: %4.4X\t", pc); log.LogDisasm(&Buffer[pc]);

            // Action containing extra data.
            int length = Buffer[pc + 1] | (Buffer[pc + 2] << 8);
            int NextPc = pc + length + 3;

            switch (actionId)
            {
            default:
                break;

            case 0x81:  // goto frame
            {
                // Used by gotoAndPlay(n), gotoAndStop(n) where n is a constant.
                // If n is a "string" use 0x8C, if variable - 0x9F.
                // Produced frame is already zero-based.
                if (env->IsTargetValid())
                {
                    int frame = Buffer[pc + 3] | (Buffer[pc + 4] << 8);
                    if (env->GetTarget())
                        env->GetTarget()->GotoFrame(frame);
                }
                break;
            }

            case 0x83:  // get url
            {               
                // Two strings as args.
                const char*     purl        = (const char*) &(Buffer[pc + 3]);
                UInt            urlLen      = (UInt)strlen(purl);
                const char*     ptargetPath = (const char*) &(Buffer[pc + 3 + urlLen + 1]);

                if (verboseAction) 
                    log.LogAction("GetURL - path: %s  URL: %s", ptargetPath, purl);
                
                // If the url starts with "FSCommand:" then this is a message for the host app,
                // so call the callback handler, if any.
                if (strncmp(purl, "FSCommand:", 10) == 0)
                {
                    GFxFSCommandHandler *phandler =
                            poriginalTarget->GetMovieRoot()->pFSCommandHandler;
                    if (phandler)
                    {
                        // Call into the app.
                        phandler->Callback(env->GetTarget()->GetMovieRoot(), purl + 10, ptargetPath);
                    }
                }
                else
                {
                    // This is a loadMovie/loadMovieNum/unloadMovie/unloadMovieNum call.
                    env->GetMovieRoot()->AddLoadQueueEntry(ptargetPath, purl, env);
                }
                break;
            }

            case 0x87:  // StoreRegister
            {
                int reg = Buffer[pc + 3];
                // Save top of stack in specified register.
                if (isFunction2)
                {
                    *(env->LocalRegisterPtr(reg)) = env->Top();

                    if (verboseAction) 
                    {
                        GASString s0(env->Top().ToDebugString(env));
                        log.LogAction(
                                "-------------- local register[%d] = '%s'",
                                reg, s0.ToCStr());

                        GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                        if (piobj)
                           log.LogAction(" at %p", piobj);
                        log.LogAction("\n");
                    }
                }
                else if (reg >= 0 && reg < 4)
                {
                    env->GetGlobalRegister(reg) = env->Top();
                
                    if (verboseAction)
                    {
                        GASString s0(env->Top().ToDebugString(env));
                        log.LogAction(
                                "-------------- global register[%d] = '%s'",
                                reg, s0.ToCStr());
                        
                        GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                        if (piobj)
                           log.LogAction(" at %p", piobj);
                        log.LogAction("\n");
                    }
                }
                else
                {
                    if (verboseActionErrors)
                        log.LogScriptError("Error: StoreRegister[%d] - register out of bounds!", reg);
                }

                break;
            }

            case 0x88:  // DeclDict: declare dictionary
            {
                int i = pc;
                //int   count = Buffer[pc + 3] | (Buffer[pc + 4] << 8);
                i += 2;

                ProcessDeclDict(env->GetSC(), pc, NextPc, log);

                break;
            }

            case 0x8A:  // wait for frame
            {                
                GFxSprite* ptarget = env->IsTargetValid() ? env->GetTarget()->ToSprite() : 0;
                if (ptarget)
                {
                    UInt frame  = Buffer[pc + 3] | (Buffer[pc + 4] << 8);
                    UInt skipCount = Buffer[pc + 5];
                    
                    // if frame is bigger than totalFrames - cut it down to totalFrames
                    UInt totalFrames = ptarget->GetFrameCount();
                    if (totalFrames > 0 && frame >= totalFrames)
                        frame = totalFrames - 1;

                    // If we haven't loaded a specified frame yet, then we're supposed to skip
                    // some specified number of actions.
                    if (frame >= ptarget->GetLoadingFrame())
                    {
                        // need to calculate the offset by counting actions
                        UInt len = GetLength();
                        UInt curpc = (UInt)NextPc;
                        for(UInt i = 0; i < skipCount && curpc < len; ++i)
                        {
                            UByte cmd = Buffer[curpc++];
                            if (cmd & 0x80) // complex action?
                            {
                                // skip the content of action by getting its length
                                curpc += (Buffer[curpc] | (Buffer[curpc + 1] << 8));
                                curpc += 2; // skip the length itself
                            }
                        }
                        UInt nextOffset = curpc;

                        // Check ActionBuffer bounds.
                        if (nextOffset >= len)
                        {
                            if (verboseActionErrors)
                                log.LogScriptError(
                                    "Error: WaitForFrame branch to offset %d - this section only runs to %d\n",
                                    NextPc, StopPc);
                        }
                        else
                        {
                            NextPc = (int)nextOffset;
                        }
                    }
                }
                
                // Fall through if this frame is loaded.
                break;
            }

            case 0x8B:  // set target (used for non-stacked tellTarget)
            {
                // Change the GFxASCharacter we're working on.
                const char* ptargetName = (const char*) &Buffer[pc + 3];
                if (ptargetName[0] == 0)
                {
                    env->SetTarget(poriginalTarget);
                }
                else
                {
                    GASString targetName(env->CreateString(ptargetName));
                    GFxASCharacter *ptarget = env->FindTarget(targetName);

                    //!AB: if ptarget is NULL, we need to set kind of fake target
                    // to avoid any side effects. Like:
                    // tellTarget(bullshit)
                    // {  gotoAndStop(1); } // if 'bullshit' is not found here,
                    // then gotoAndStop should do nothing
                    if (!ptarget) 
                    {
                        if (verboseActionErrors)
                            log.LogScriptError("Error: SetTarget(tellTarget) with invalid target '%s'.\n",
                                               ptargetName);
                        env->SetInvalidTarget(poriginalTarget);
                    }
                    else
                    {
                        env->SetTarget(ptarget);
                    }
                }
                break;
            }

            case 0x8C:  // go to labeled frame, GotoFrameLbl
            {
                // MA: This op does NOT interpret numbers in a string, so "4" is actually 
                // treated as a label and NOT as frame. This is different from ops and
                // functions with stack arguments, which DO try to parse frame number first.
                if (env->IsTargetValid())
                {
                    char*       pframeLabel = (char*) &Buffer[pc + 3];
                    GFxSprite*  psprite = env->GetTarget()->ToSprite();
                    if (psprite)
                        psprite->GotoLabeledFrame(pframeLabel);
                }
                break;
            }

            case 0x8D:  // wait for frame Expression (?)
            {
                // Pop the frame number to wait for; if it's not loaded skip the
                // specified number of actions.
                //
                // Since we don't support incremental loading, pop our arg and
                // don't do anything.
                env->Drop1();
                break;
            }

            case 0x8E:  // function2
            {
                GASAsFunctionDef* funcDef = new GASAsFunctionDef (this, env, NextPc, WithStack);
                funcDef->SetExecType(GASActionBuffer::Exec_Function2);

                int i = pc;
                i += 3;

                // Extract name.
                // @@ security: watch out for possible missing terminator here!
                GASString   name = env->CreateString((const char*) &Buffer[i]);
                i += (int)name.GetSize() + 1;

                // Get number of arguments.
                int nargs = Buffer[i] | (int(Buffer[i + 1]) << 8);
                i += 2;

                // Get the count of local registers used by this function.
                UByte   RegisterCount = Buffer[i];
                i += 1;
                funcDef->SetLocalRegisterCount(RegisterCount);

                // Flags, for controlling register assignment of implicit args.
                UInt16  flags = Buffer[i] | (UInt16(Buffer[i + 1]) << 8);
                i += 2;
                funcDef->SetFunction2Flags(flags);

                // Get the register assignments and names of the arguments.
                for (int n = 0; n < nargs; n++)
                {
                    int ArgRegister = Buffer[i];
                    i++;

                    UInt     length = (UInt)strlen((const char*) &Buffer[i]);
                    GASSERT((length + i) <= GetLength());
                    GASString argstr(env->CreateString((const char*) &Buffer[i], length));

                    // @@ security: watch out for possible missing terminator here!
                    i += (int)funcDef->AddArg(ArgRegister, argstr).Name.GetSize() + 1;                    
                }

                // Get the length of the actual function code.
                int length = Buffer[i] | (Buffer[i + 1] << 8);
                i += 2;
                funcDef->SetLength(length);

                // Skip the function Body (don't interpret it now).
                NextPc += length;

                // If we have a name, then save the function in this
                // environment under that name. FunctionValue will AddRef to the function.
                GPtr<GASFunctionObject> funcObj = *new GASCustomFunctionObject (env, funcDef);
                GASFunctionRef funcRef (funcObj);
                GASLocalFrame* plocalFrame = env->GetTopLocalFrame ();
                if (plocalFrame)
                {
                    funcRef.SetLocalFrame (plocalFrame);
                    plocalFrame->LocallyDeclaredFuncs.set(funcObj.GetPtr(), 0);
                }

                GASValue    FunctionValue(funcRef);
                if (!name.IsEmpty())
                {
                    // @@ NOTE: should this be Target->SetVariable()???
                    //env->SetMember(name, FunctionValue);
                    env->GetTarget()->SetMemberRaw(env->GetSC(), name, FunctionValue);
                }

                // AB, Set prototype property for function
                // The prototype for function is the instance of FunctionProto. 
                // Also, the "constructor" property of this prototype
                // instance should point to GASAsFunction instance (funcDef)
                GASStringContext*   psc = env->GetSC();
                GPtr<GASFunctionProto> funcProto = *new GASFunctionProto(psc, env->GetPrototype(GASBuiltin_Object), funcRef, false);
                funcRef->SetProtoAndCtor(psc, env->GetPrototype(GASBuiltin_Function));
                
                funcObj->SetMemberRaw(psc, env->GetBuiltin(GASBuiltin_prototype), GASValue(funcProto));

                if (name.IsEmpty())
                {
                    // Also leave it on the stack if function is anonymous.
                    env->Push(FunctionValue);
                }
                break;
            }

            case 0x8F:  // try
            {
                if (verboseActionErrors)
                    log.LogScriptError("Error: Unsupported opcode %02X\n", actionId);
                break;
            }

            case 0x94:  // with
            {
                //int frame = Buffer[pc + 3] | (Buffer[pc + 4] << 8);
                
                if (verboseAction) 
                    log.LogAction("-------------- with block start: stack size is %d\n", (int)WithStack.size());
                if (WithStack.size() < 8)
                {
                    int BlockLength = Buffer[pc + 3] | (Buffer[pc + 4] << 8);
                    int BlockEnd = NextPc + BlockLength;
                    
                    if (env->Top().IsCharacter())
                    {
                        GFxASCharacter* pwithChar = env->Top().ToASCharacter(env);
                        WithStack.push_back(GASWithStackEntry(pwithChar, BlockEnd));
                    }
                    else
                    {
                        GASObject* pwithObj = env->Top().ToObject();
                        WithStack.push_back(GASWithStackEntry(pwithObj, BlockEnd));
                    }               
                }
                env->Drop1();
                break;
            }

            case 0x96:  // PushData
            {
                SInt i = pc;

                // MA: Length must be greater then 0 here, otherwise push would make no sense;
                // so it shouldn't happen in practice. Hence, use do {} while for efficiency,
                // as PushData is the *most* common op. This assertion can be checked for by the 
                // bytecode verifier in the future (during action buffer Read, etc).
                GASSERT(length > 0);
               
                do
                {
                    SPInt   type = Buffer[3 + i];
                    i++;

                    // Push register is the most common value type.
                    // Push dictionary is the second common type.
                    if (type == 4)
                    {
                        // contents of register
                        SInt    reg = Buffer[3 + i];
                        i++;
                        if (isFunction2)
                        {
                            env->Push(*(env->LocalRegisterPtr(reg)));
                            if (verboseAction) 
                            {
                                GASString s0(env->Top().ToDebugString(env));

                                log.LogAction(
                                    "-------------- pushed local register[%d] = '%s'",
                                    reg, s0.ToCStr());

                                GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                                if (piobj)
                                    log.LogAction(" at %p", piobj);                          
                                log.LogAction("\n");
                            }
                        }
                        else if (reg < 0 || reg >= 4)
                        {
                            env->Push(GASValue());
                            if (verboseActionErrors)
                                log.LogScriptError("Error: push register[%d] - register out of bounds\n", reg);
                        }
                        else
                        {
                            env->Push(env->GetGlobalRegister(reg));
                            if (verboseAction) 
                            {
                                GASString s0(env->Top().ToDebugString(env));
                                log.LogAction(
                                    "-------------- pushed global register[%d] = '%s'",
                                    reg, s0.ToCStr());
                                GASObjectInterface* piobj = env->Top().ToObjectInterface(env);
                                if (piobj)
                                    log.LogAction(" at %p", piobj);  
                                log.LogAction("\n");
                            }
                        }

                    }
                    else if (type == 8)
                    {
                        UInt    id = Buffer[3 + i];
                        i++;
                        if (id < Dictionary.size())
                        {
                            // Push string directly with a copy constructor.
                            env->Push(Dictionary[id]);

                            if (verboseAction) 
                                log.LogAction("-------------- pushed '%s'\n", Dictionary[id].ToCStr());
                        }
                        else
                        {
                            if (verboseActionErrors)
                                log.LogScriptError("Error: DictLookup(%d) is out of bounds\n", id);
                            env->Push(GASValue());
                            if (verboseAction) 
                                log.LogAction("-------------- pushed 0\n");
                        }
                    }
                    else if (type == 0)
                    {
                        // string
                        GASString str(env->CreateString((const char*) &Buffer[3 + i]));                        
                        i += (SInt)str.GetSize() + 1;
                        env->Push(str);

                        if (verboseAction) 
                            log.LogAction("-------------- pushed '%s'\n", str.ToCStr());
                    }
                    else if (type == 1)
                    {
                        // Float (little-endian)
                        union {
                            Float   F;
                            UInt32  I;
                        } u;
                        GCOMPILER_ASSERT(sizeof(u) == sizeof(u.I));

                        memcpy(&u.I, &Buffer[3 + i], 4);
                        u.I = GByteUtil::LEToSystem(u.I);
                        i += 4;

                        env->Push(u.F);

                        if (verboseAction) 
                            log.LogAction("-------------- pushed '%f'\n", u.F);
                    }
                    else if (type == 2)
                    {                           
                        GASValue nullValue;
                        nullValue.SetNull();
                        env->Push(nullValue);

                        if (verboseAction) 
                            log.LogAction("-------------- pushed NULL\n");
                    }
                    else if (type == 3)
                    {
                        env->Push(GASValue());

                        if (verboseAction) 
                            log.LogAction("-------------- pushed UNDEFINED\n");
                    }
                    else if (type == 5)
                    {
                        bool    BoolVal = Buffer[3 + i] ? true : false;
                        i++;
//                          LogMsg("bool(%d)\n", BoolVal);
                        env->Push(BoolVal);

                        if (verboseAction) 
                            log.LogAction("-------------- pushed %s\n", BoolVal ? "true" : "false");
                    }
                    else if (type == 6)
                    {
                        // double
                        // wacky format: 45670123
#ifdef GFC_NO_DOUBLE
                        union
                        {
                            Float   F;
                            UInt32  I;
                        } u;

                        // convert ieee754 64bit to 32bit for systems without proper double
                        SInt    sign = (Buffer[3 + i + 3] & 0x80) >> 7;
                        SInt    expo = ((Buffer[3 + i + 3] & 0x7f) << 4) + ((Buffer[3 + i + 2] & 0xf0) >> 4);
                        SInt    mant = ((Buffer[3 + i + 2] & 0x0f) << 19) + (Buffer[3 + i + 1] << 11) +
                            (Buffer[3 + i + 0] << 3) + ((Buffer[3 + i + 7] & 0xf8) >> 5);

                        if (expo == 2047)
                            expo = 255;
                        else if (expo - 1023 > 127)
                        {
                            expo = 255;
                            mant = 0;
                        }
                        else if (expo - 1023 < -126)
                        {
                            expo = 0;
                            mant = 0;
                        }
                        else
                            expo = expo - 1023 + 127;

                        u.I = (sign << 31) + (expo << 23) + mant;
                        i += 8;

                        env->Push((GASNumber)u.F);
                        
                        if (verboseAction) 
                            log.LogAction("-------------- pushed double %f\n", u.F);
#else
                        union {
                            double  D;
                            UInt64  I;
                            struct {
                                UInt32  Lo;
                                UInt32  Hi;
                            } Sub;
                        } u;
                        GCOMPILER_ASSERT(sizeof(UInt32) == 4);
                        GCOMPILER_ASSERT(sizeof(u) == sizeof(u.I));

                        memcpy(&u.Sub.Hi, &Buffer[3 + i], 4);
                        memcpy(&u.Sub.Lo, &Buffer[3 + i + 4], 4);
                        u.I = GByteUtil::LEToSystem(u.I);
                        i += 8;

                        env->Push((GASNumber)u.D);

                        if (verboseAction) 
                            log.LogAction("-------------- pushed double %f\n", u.D);
#endif
                    }
                    else if (type == 7)
                    {
                        // int32
                        SInt32  val = Buffer[3 + i]
                            | (Buffer[3 + i + 1] << 8)
                            | (Buffer[3 + i + 2] << 16)
                            | (Buffer[3 + i + 3] << 24);
                        i += 4;
                    
                        env->Push(val);

                        if (verboseAction) 
                            log.LogAction("-------------- pushed int32 %d\n", (int)val);
                    }
                    else if (type == 9)
                    {
                        UInt    id = Buffer[3 + i] | (Buffer[4 + i] << 8);
                        i += 2;
                        if (id < Dictionary.size())
                        {                            
                            env->Push(Dictionary[id]);
                            if (verboseAction) 
                                log.LogAction("-------------- pushed '%s'\n", Dictionary[id].ToCStr());
                        }
                        else
                        {
                            if (verboseActionErrors)
                                log.LogScriptError("Error: DictLookup(%d) is out of bounds\n", id);
                            env->Push(GASValue());

                            if (verboseAction) 
                                log.LogAction("-------------- pushed 0");
                        }
                    }

                } while (i - pc < length);
                
                break;
            }
            case 0x99:  // branch Always (goto)
            {
                SInt16  offset = Buffer[pc + 3] | (SInt16(Buffer[pc + 4]) << 8);
                NextPc += offset;
                                  
                // Range checks.
                if (((UInt)NextPc) >= GetLength())
                {
                    // MA: Seems that ActionScript can actually jump outside of action buffer
                    // bounds into the binary content stored within other tags. Handling this
                    // would complicate things. Ex: f1.swf 
                    // Perhaps the entire SWF is stored within one chunk of memory and
                    // ActionScript goto instruction is welcome to seek arbitrarily in it ?!?

                    env->SetTarget(poriginalTarget);
                    if (verboseActionErrors)
                        log.LogScriptError(" Error: Branch destination %d is out of action buffer bounds!\n",
                                            NextPc);
                    return;
                }
                break;
            }
            case 0x9A:  // get url 2
            {
                UByte   method = Buffer[pc + 3];

                GASString   targetPath(env->Top().ToString(env));
                GASString   url(env->Top1().ToString(env));

                if (verboseAction) 
                    log.LogAction("GetURL2 - path: %s  URL: %s", targetPath.ToCStr(), url.ToCStr());

                // If the url starts with "FSCommand:", then this is
                // a message for the host app.
                if (strncmp(url.ToCStr(), "FSCommand:", 10) == 0)
                {
                    GFxFSCommandHandler *phandler =
                        poriginalTarget->GetMovieRoot()->pFSCommandHandler;
                    if (phandler)
                    {
                        // Call into the app.
                        phandler->Callback(env->GetTarget()->GetMovieRoot(),
                                           url.ToCStr() + 10, targetPath.ToCStr());
                    }
                }
                else
                {
                    GFxLoadQueueEntry::LoadMethod loadMethod = GFxLoadQueueEntry::LM_None;
                    switch(method & 3)
                    {
                        case 1: loadMethod = GFxLoadQueueEntry::LM_Get; break;
                        case 2: loadMethod = GFxLoadQueueEntry::LM_Post; break;
                    }

                    // 0x40 is set if target is a path to a clip; otherwise it is a path to a window
                    // 0x40 -> loadMovie(); if not loadURL()
                    // Note: loadMovieNum() can call this without target flag, so check url for _levelN.
                    const char* ptail = "";
                    int         level = GFxMovieRoot::ParseLevelName(
                                                targetPath.ToCStr(), &ptail,
                                                env->GetTarget()->IsCaseSensitive());
                    if (method & 0x80)
                    {   
                        // loadVars\loadVarsNum
                        env->GetMovieRoot()->AddVarLoadQueueEntry(targetPath.ToCStr(), url.ToCStr(), loadMethod);
                    }
                    else if ((method & 0x40) || ((level != -1) && (*ptail==0)))
                    {
                        // This is a loadMovie/loadMovieNum/unloadMovie/unloadMovieNum call.
                        env->GetMovieRoot()->AddLoadQueueEntry(targetPath.ToCStr(), url.ToCStr(), env, loadMethod);
                    }
                    else
                    {   
                        // ??? not sure what to do.
                    }
                
                }
                env->Drop2();
                break;
            }

            case 0x9B:  // declare function
            {
                GASAsFunctionDef* funcDef = new GASAsFunctionDef (this, env, NextPc, WithStack);

                int i = pc;
                i += 3;

                // Extract name.
                // @@ security: watch out for possible missing terminator here!
                GASString   name = env->CreateString((const char*) &Buffer[i]);
                i += (int)name.GetSize() + 1;

                // Get number of arguments.
                int nargs = Buffer[i] | (Buffer[i + 1] << 8);
                GASSERT((i + nargs*2) <= (int)GetLength());
                i += 2;

                // Get the names of the arguments.
                for (int n = 0; n < nargs; n++)
                {
                    // @@ security: watch out for possible missing terminator here!
                    GASString argstr(env->CreateString((const char*) &Buffer[i]));
                    i += (int)funcDef->AddArg(0, argstr).Name.GetSize() + 1;
                }

                // Get the length of the actual function code.
                int length = Buffer[i] | (Buffer[i + 1] << 8);
                i += 2;
                funcDef->SetLength(length);

                // Skip the function Body (don't interpret it now).
                NextPc += length;

                // If we have a name, then save the function in this
                // environment under that name. FunctionValue will AddRef to the function.
                GPtr<GASFunctionObject> funcObj = *new GASCustomFunctionObject (env, funcDef);
                GASFunctionRef funcRef (funcObj);
                GASLocalFrame* plocalFrame = env->GetTopLocalFrame ();
                if (plocalFrame)
                {
                    funcRef.SetLocalFrame (plocalFrame);
                    plocalFrame->LocallyDeclaredFuncs.set(funcObj.GetPtr(), 0);
                }
                GASValue    FunctionValue(funcRef);
                if (!name.IsEmpty())
                {
                    // @@ NOTE: should this be Target->SetVariable()???
                    //env->SetMember(name, FunctionValue);
                    env->GetTarget()->SetMemberRaw(env->GetSC(), name, FunctionValue);
                    //env->SetVariable(name, FunctionValue, WithStack);
                }

                // AB, Set prototype property for function
                // The prototype for function is the instance of FunctionProto. 
                // Also, the "constructor" property of this prototype
                // instance should point to GASAsFunction instance (funcDef)
                GPtr<GASFunctionProto> funcProto = *new GASFunctionProto(env->GetSC(), env->GetPrototype(GASBuiltin_Object), funcRef, false);
                funcRef->SetProtoAndCtor(env->GetSC(), env->GetPrototype(GASBuiltin_Function));
                funcObj->SetMemberRaw(env->GetSC(), env->GetBuiltin(GASBuiltin_prototype), GASValue(funcProto));

                if (name.IsEmpty())
                {
                    // Also leave it on the stack, if function is anonymous
                    env->Push(FunctionValue);
                }
                break;
            }

            case 0x9D:  // branch if true
            {
                SInt16  offset = Buffer[pc + 3] | (SInt16(Buffer[pc + 4]) << 8);
                
                bool    test = env->Top().ToBool(env);
                env->Drop1();
                if (test)
                {
                    NextPc += offset;

                    if (NextPc > StopPc)
                    {
                        if (verboseActionErrors)
                            log.LogScriptError("Error: branch to offset %d - this section only runs to %d\n",
                                                NextPc,
                                                StopPc);
                    }
                }
                break;
            }
            case 0x9E:  // call frame
                {
                    // Note: no extra data in this instruction!
                    //UInt frameNumber;
                    const GASValue& frameValue = env->Top();
                    GFxASCharacter* ptarget  = env->GetTarget();
                    GASSERT(ptarget);
                    if (env->IsTargetValid())
                    {
                        UInt frameNumber = 0;   

                        bool success            = false;

                        if (frameValue.GetType() == GASValue::STRING)
                        {   
                            GASString   str(env->Top().ToString(env));
                            int         mbLength = str.GetLength();
                            int         i;

                            // Parse possible sprite path...
                            for (i=0; i<mbLength; i++)
                            {
                                if (str.GetCharAt(i) == ':')
                                {
                                    // Must be a target label.
                                    GASString targetStr(str.Substring(0, i));
                                    if ((ptarget = env->FindTarget(targetStr))!=0)
                                    {
                                        if (i >= mbLength)
                                            ptarget = 0; // No frame.
                                        else
                                        {   
                                            // The remainder of the string is frame number or label.
                                            str = str.Substring(i+1, mbLength+1);
                                            break;
                                        }
                                    }
                                }
                            }

                            if (ptarget && ptarget->GetLabeledFrame(str.ToCStr(), &frameNumber))
                                success = true;                  
                        }
                        else if (frameValue.GetType() == GASValue::OBJECT)
                        {
                            // This is a no-op; see TestGotoFrame.Swf
                        }
                        else if (frameValue.IsNumber())
                        {
                            // MA: Unlike other places, this frame number is NOT zero-based,
                            // because it is computed by action script; so subtract 1.
                            frameNumber = int(frameValue.ToNumber(env) - 1);
                            success = true;
                        }

                        if (success)
                        {
                            GFxSprite* psprite = ptarget->ToSprite();
                            if (psprite)
                                psprite->CallFrameActions(frameNumber);
                        }
                        else if (verboseActionErrors)
                        {
                            log.LogScriptError("Error: CallFrame('%s') - unknown frame\n", env->Top().ToString(env).ToCStr());
                        }
                        env->Drop1();
                    }
               
                    break;
                }

            case 0x9F:  // goto frame expression, GotoFrameExp
            {
                // From Alexi's SWF ref:
                //
                // Pop a value or a string and jump to the specified
                // frame. When a string is specified, it can include a
                // path to a sprite as in:
                // 
                //   /Test:55
                // 
                // When F_play is ON, the action is to play as soon as that 
                // frame is reached. Otherwise, the frame is shown in stop mode.

                UByte               optionFlag  = Buffer[pc + 3];
                GFxMovie::PlayState state       = (optionFlag & 1) ?
                                                    GFxMovie::Playing : GFxMovie::Stopped;
                SInt                sceneOffset = 0;

                // SceneOffset is used if multiple scenes are created in Flash studio,
                // and then gotoAndPlay("Scene 2", val) is executed. The name of the
                // scene is converted into offset and passed here.
                if (optionFlag & 0x2)
                    sceneOffset = Buffer[pc + 4] | (Buffer[pc + 5] << 8);

                GFxASCharacter* target  = env->GetTarget();
                bool            success = false;

                if (env->Top().GetType() == GASValue::STRING)
                {   
                                                                
                    GASString   str(env->Top().ToString(env));
                    int         mbLength = str.GetLength();
                    int         i;

                    // Parse possible sprite path...
                    for (i=0; i<mbLength; i++)
                    {
                        if (str.GetCharAt(i) == ':')
                        {
                            // Must be a target label.
                            GASString targetStr(str.Substring(0, i));
                            if ((target = env->FindTarget(targetStr))!=0)
                            {
                                if (i >= mbLength)
                                    target = 0; // No frame.
                                else
                                {   
                                    // The remainder of the string is frame number or label.
                                    str = str.Substring(i+1, mbLength+1);
                                    break;
                                }
                            }
                        }
                    }
                    

                    UInt frameNumber = 0;                       
                    if (target && target->GetLabeledFrame(str.ToCStr(), &frameNumber))
                    {
                        // There actually is a bug here *In Flash*. SceneOffset should not be 
                        // considered if we go to label, as labels already have absolute
                        // scene-inclusive offsets. However, we replicate their player bug.
                        // MA: Verified correct.
                        target->GotoFrame(frameNumber + sceneOffset);
                        success = true;
                    }                       
                
                }
                else if (env->Top().GetType() == GASValue::OBJECT)
                {
                    // This is a no-op; see TestGotoFrame.Swf
                }
                else if (env->Top().IsNumber())
                {
                    // MA: Unlike other places, this frame number is NOT zero-based,
                    // because it is computed by action script; so subtract 1.
                    int FrameNumber = int(env->Top().ToNumber(env));
                    target->GotoFrame(FrameNumber + sceneOffset - 1);
                    success = true;
                }

                if (success)
                    target->SetPlayState(state);
                
                env->Drop1();
                break;
            }
            
            }
            pc = NextPc;
        }
        if (verboseAction) 
            log.LogAction ("==== stack top index %d ====\n", env->GetTopIndex());
    }
    
    // AB: if originally set target was "invalid" (means tellTarget was invoked with incorrect
    // target as a parameter) then we need restore it as "invalid"...
    if (isOriginalTargetValid)
        env->SetTarget(poriginalTarget);
    else
        env->SetInvalidTarget(poriginalTarget);
    if (verboseAction) 
        log.LogAction("\n");
}


//
// GASEnvironment
//

// Target functions placed here because GFxAction.h does not know GFxASCharacter
void    GASEnvironment::SetTarget(GFxASCharacter* ptarget) 
{ 
    Target          = ptarget; 
    IsInvalidTarget = false;
    StringContext.UpdateVersion(ptarget->GetVersion());
}
void    GASEnvironment::SetInvalidTarget(GFxASCharacter* ptarget) 
{ 
    Target          = ptarget; 
    IsInvalidTarget = true;
    StringContext.UpdateVersion(ptarget->GetVersion());
}

// Used to set target right after construction
void    GASEnvironment::SetTargetOnConstruct(GFxASCharacter* ptarget)
{    
    SetTarget(ptarget);
    StringContext.pContext = ptarget->GetGC();  
}

// GFxLogBase overrides to support logging correctly
void        GASEnvironment::LogScriptError(const char* pfmt,...) const
{
    va_list argList; va_start(argList, pfmt);
    if(pASLogger)
        pASLogger->LogScriptMessageVarg(Log_ScriptError,pfmt,argList);
    else
        GetLog()->LogMessageVarg(Log_ScriptError, pfmt, argList);
    va_end(argList);
}

void        GASEnvironment::LogScriptWarning(const char* pfmt,...) const
{
    va_list argList; va_start(argList, pfmt);
    if(pASLogger)
        pASLogger->LogScriptMessageVarg(Log_ScriptWarning,pfmt,argList);
    else
        GetLog()->LogMessageVarg(Log_ScriptWarning, pfmt, argList);
    va_end(argList);
}

void        GASEnvironment::LogScriptMessage(const char* pfmt,...) const
{
    va_list argList; va_start(argList, pfmt);
    if(pASLogger)
        pASLogger->LogScriptMessageVarg(Log_ScriptMessage, pfmt, argList);
    else
        GetLog()->LogMessageVarg(Log_ScriptMessage,pfmt,argList);
    va_end(argList);
}

GFxLog*     GASEnvironment::GetLog() const
{
    return Target->GetLog();
}

bool        GASEnvironment::IsVerboseAction() const
{   
    return Target->GetMovieRoot()->VerboseAction;
}

bool        GASEnvironment::IsVerboseActionErrors() const
{   
    return !Target->GetMovieRoot()->SuppressActionErrors;
}

GFxMovieRoot*   GASEnvironment::GetMovieRoot() const
{   
    return Target->GetMovieRoot();
}

// Return the value of the given var, if it's defined.
// *ppnewTarget receives a new value if it changes.
// excludeFlags - mask consisted of ExcludeFlags bits.
bool        GASEnvironment::GetVariable(const GASString& varname, 
                                        GASValue* presult,
                                        const GTL::garray<GASWithStackEntry>* pwithStack, 
                                        GFxASCharacter **ppnewTarget,
                                        GASValue* powner,
                                        UInt excludeFlags) const
{
    // GFxPath lookup rigmarole.
    // NOTE: IsPath caches 'NotPath' bit in varname string for efficiency.
    if (varname.IsNotPath() || !IsPath(varname))
    {
        return GetVariableRaw(varname, presult, pwithStack, powner, excludeFlags);
    }
    else
    {
        GASValue owner;
        bool retVal = FindVariable(varname, presult, pwithStack, ppnewTarget, &owner, false);
        if (owner.IsUndefined())
        {
            if (!(excludeFlags & NoLogOutput))
                LogScriptError("Error: GetVariable failed: can't resolve the path \"%s\"\n", varname.ToCStr());
            return false;
        }
        else
        {
            if (powner) *powner = owner;
            return retVal;
        }
    }
}


// Determine if a specified variable is available for access.
bool    GASEnvironment::IsAvailable(const GASString& varname, const GTL::garray<GASWithStackEntry>* pwithStack) const
{
    // *** GFxPath lookup.
    GFxASCharacter* target = Target;
    GASString       path(GetBuiltin(GASBuiltin_empty_));
    GASString       var(GetBuiltin(GASBuiltin_empty_));
    GASValue        val;
    GASStringContext* psc = GetSC();

    if (GetVariable(varname, &val, pwithStack, NULL, NULL, NoLogOutput))
        return true;

    //!AB: do we still need this here after GetVariable?
    if (ParsePath(psc, varname, &path, &var))
    {
        if ((target = FindTarget(path, NoLogOutput))==0)
            return 0;
        return target->GetMemberRaw(psc, var, &val);
    }

    // *** NOT Path, do RAW variable check.

    if (pwithStack)
    {
        // Check the with-stack.
        for (int i = (int)pwithStack->size() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*pwithStack)[i].GetObjectInterface();
            // Found the var in this context ?
            if (obj && obj->GetMemberRaw(psc, varname, &val))
                return true;
        }
    }

    if (FindLocal(varname))
        return true;
    if (Target && Target->GetMemberRaw(psc, varname, &val))
        return true;
    
    // Pre-canned names.
    if (IsCaseSensitive())
    {
        if (GetBuiltin(GASBuiltin_this) == varname ||
            GetBuiltin(GASBuiltin__root) == varname ||
            GetBuiltin(GASBuiltin__global) == varname)
            return true;
    }
    else
    {
        varname.ResolveLowercase();
        if (GetBuiltin(GASBuiltin_this).CompareBuiltIn_CaseInsensitive_Unchecked(varname) ||
            GetBuiltin(GASBuiltin__root).CompareBuiltIn_CaseInsensitive_Unchecked(varname) ||
            GetBuiltin(GASBuiltin__global).CompareBuiltIn_CaseInsensitive_Unchecked(varname))
            return true;
    }

    // Check for _levelN.
    if (varname[0] == '_')
    {
        const char* ptail = 0;
        SInt        level = GFxMovieRoot::ParseLevelName(varname.ToCStr(), &ptail,
                                                         IsCaseSensitive());
        if ((level != -1) && !*ptail )
        {
            if (Target->GetLevelMovie(level))
                return true;
        }
    }

    GASObject* pglobal = GetGC()->pGlobal;
    if (pglobal && pglobal->GetMemberRaw(psc, varname, &val))       
        return true;

    //  Not found -> not available.
    return false;
}

// excludeFlags: IgnoreWithoutOwners, IgnoreLocals
// varname must be a plain variable name; no path parsing.
bool    GASEnvironment::GetVariableRaw(const GASString& varname,
                                       GASValue* presult,
                                       const GTL::garray<GASWithStackEntry>* pwithStack,
                                       GASValue* powner,
                                       UInt excludeFlags) const
{
    // Debug checks: these should never be called for path variables.
    GASSERT(strchr(varname.ToCStr(), ':') == NULL);
    GASSERT(strchr(varname.ToCStr(), '/') == NULL);
    GASSERT(strchr(varname.ToCStr(), '.') == NULL);

    if (!presult) return false;

    if (powner)
        *powner = 0;
    // Check the with-stack.
    if (pwithStack)
    {
        for (int i = (int)pwithStack->size() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*pwithStack)[i].GetObjectInterface();
            
            if (obj == NULL)
            {
                //!AB, looks like, invalid obj in withStack should stop lookup
                // and return "undefined"
                return false;
            }

            if (obj && obj->GetMember(const_cast<GASEnvironment*>(this), varname, presult))
            {
                // Found the var in this context.
                if (powner)
                {
                    // Object interfaces can be characters or objects only.
                    if (obj->IsASCharacter())
                        powner->SetAsCharacter((GFxASCharacter*) obj);
                    else
                        powner->SetAsObject((GASObject*) obj);
                }
                return true;
            }
        }
    }

    if (!(excludeFlags & IgnoreLocals))
    {
        // Check locals.
        if (FindLocal(varname, presult))
        {
            // Get local var.
            return true;
        }
        else if (GetVersion() >= 6)
        {
            // Looking for "super" in SWF6?            
            if (GetBuiltin(GASBuiltin_super).CompareBuiltIn_CaseCheck(varname, IsCaseSensitive()))                
            {
                GASLocalFrame* pLocFr = GetTopLocalFrame(0);
                if (pLocFr && pLocFr->SuperThis)
                {
                    GASObjectInterface* const iobj = pLocFr->SuperThis;
                    GPtr<GASObject> superObj;
                    GPtr<GASObject> proto = iobj->Get__proto__();
                    //printf ("!!! savedThis.__proto__ = %s\n", (const char*)GetGC()->FindClassName(proto));
                    //printf ("!!! real pthis.__proto__ = %s\n", (const char*)GetGC()->FindClassName(pthis->Get__proto__(this)));

                    if (proto)
                    {
                        // need to get a real "this" to save in "super"
                        GASValue thisVal;
                        GetVariable(GetBuiltin(GASBuiltin_this), &thisVal, pwithStack);

                        GASFunctionRef __ctor__ = proto->Get__constructor__(GetSC());
                        //printf ("!!! __proto__.__ctor__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(__ctor__.GetObjectPtr()));
                        //printf ("!!! __proto__.__proto__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(proto->Get__proto__(pourEnv)));
                        superObj = *new GASSuperObject(proto->Get__proto__(), thisVal.ToObjectInterface(this), __ctor__);

                        presult->SetAsObject(superObj);
                        //!AB, don't like this hack, but it is better to cache the "super"...
                        // don't want to remove the constness from GetVariable/Raw either
                        const_cast<GASEnvironment*>(this)->SetLocal(GetBuiltin(GASBuiltin_super), *presult);
                        return true;
                    }
                }
            }
        }

        // Looking for "this"?
        if (GetBuiltin(GASBuiltin_this).CompareBuiltIn_CaseCheck(varname, IsCaseSensitive()))
        {
            presult->SetAsCharacter(Target);
            return true;
        }
    }

    // Check GFxASCharacter members.
    if (!Target) // Target should not be null, but just in case..
        return false;
    if (Target->GetMemberRaw(GetSC(), varname, presult))
    {
        if (powner)
            *powner = Target;
        return true; 
    }

    GASObject* pglobal = GetGC()->pGlobal;
    if (!(excludeFlags & IgnoreContainers))
    {
        // Check built-in constants.
        if (varname.GetLength() > 0 && varname[0] == '_')
        {
            // Handle _root and _global.
            if (IsCaseSensitive())
            {
                if (GetBuiltin(GASBuiltin__root) == varname)
                {
                    presult->SetAsCharacter(Target->GetASRootMovie());
                    return true;
                }
                else if (GetBuiltin(GASBuiltin__global) == varname)
                {
                    presult->SetAsObject(pglobal);
                    return true;
                }
            }
            else
            {
                if (GetBuiltin(GASBuiltin__root).CompareBuiltIn_CaseInsensitive(varname))
                {
                    presult->SetAsCharacter(Target->GetASRootMovie());
                    return true;
                }
                else if (GetBuiltin(GASBuiltin__global).CompareBuiltIn_CaseInsensitive(varname))
                {
                    presult->SetAsObject(pglobal);
                    return true;
                }
            }

            // Check for _levelN.
            const char* ptail = 0;
            SInt        level = GFxMovieRoot::ParseLevelName(varname.ToCStr(), &ptail,
                                                             IsCaseSensitive());
            if ((level != -1) && !*ptail)
            {
                GFxASCharacter* lm = Target->GetLevelMovie(level);
                presult->SetAsCharacter(lm);
                return lm ? true : false;
            }
        }
    }

    if (pglobal && pglobal->GetMember(const_cast<GASEnvironment*>(this), varname, presult))
    {
        if (powner)
            *powner = pglobal;
        return true;
    }

    // Fallback.
    if (!(excludeFlags & NoLogOutput))
        LogAction("GetVariableRaw(\"%s\") failed, returning UNDEFINED.\n", varname.ToCStr());
    return false;
}

// varname must be a plain variable name; no path parsing.
bool    GASEnvironment::FindOwnerOfMember(const GASString& varname,
                                          GASValue* presult,
                                          const GTL::garray<GASWithStackEntry>* pwithStack) const
{
    GASSERT(strchr(varname.ToCStr(), ':') == NULL);
    GASSERT(strchr(varname.ToCStr(), '/') == NULL);
    GASSERT(strchr(varname.ToCStr(), '.') == NULL); 

    if (!presult) return false;

    if (pwithStack)
    {
        // Check the with-stack.
        for (int i = (int)pwithStack->size() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*pwithStack)[i].GetObjectInterface();
            if (obj && obj->HasMember(GetSC(), varname, false))
            {
                // Found the var in this context.
                // Object interfaces can be characters or objects only.
                if (obj->IsASCharacter())
                    presult->SetAsCharacter((GFxASCharacter*) obj);
                else
                    presult->SetAsObject((GASObject*) obj);
                return true;
            }
        }
    }

    // Check GFxMovieSub members.
    if (!Target) // Target should not be null, but just in case..
        return false;
    if (Target->HasMember(GetSC(), varname, false))  
    {
        presult->SetAsCharacter(Target);
        return true;  
    }

    GASObject* pglobal = GetGC()->pGlobal;
    if (pglobal && pglobal->HasMember(GetSC(), varname, false))
    {
        presult->SetAsObject(pglobal);
        return true;  
    }
    return false;
}

// Given a path to variable, set its value.
bool    GASEnvironment::SetVariable(
                const GASString& varname,
                const GASValue& val,
                const GTL::garray<GASWithStackEntry>* pwithStack,
                bool doDisplayErrors)
{
    if (IsVerboseAction())
    {
        GASString vs(GASValue(val).ToDebugString(this));
        LogAction("-------------- %s = %s\n", varname.ToCStr(), vs.ToCStr());
    }

    // GFxPath lookup rigamarole.
    // NOTE: IsPath caches 'NotPath' bit in varname string for efficiency.
    if (varname.IsNotPath() || !IsPath(varname))
    {
        SetVariableRaw(varname, val, pwithStack);
        // Essentially SetMember, which should not fail.
        return true;
    }
    else
    {
        GASValue owner;
        GASString var(GetBuiltin(GASBuiltin_empty_));
        GASValue curval;
        FindVariable(varname, &curval, pwithStack, 0, &owner, false, &var);
        if (!owner.IsUndefined())
        {
            GASObjectInterface* pobj = owner.ToObjectInterface(this);
            GASSERT(pobj); // owner always should be an object!
            if (pobj)
            {
                pobj->SetMember(this, var, val);
                return true;
            }
        }
        else
        {
            if (doDisplayErrors && IsVerboseActionErrors())
                LogScriptError("Error: SetVariable failed: can't resolve the path \"%s\"\n", varname.ToCStr());
        }
    }    
    return false;
}

// No path rigamarole.
void    GASEnvironment::SetVariableRaw(
            const GASString& varname, const GASValue& val,
            const GTL::garray<GASWithStackEntry>* pwithStack)
{
    if (pwithStack)
    {
        // Check the with-stack.
        for (int i = (int)pwithStack->size() - 1; i >= 0; i--)
        {
            GASObjectInterface* obj = (*pwithStack)[i].GetObjectInterface();
            GASValue    dummy;
            if (obj && obj->GetMember(this, varname, &dummy))
            {
                // This object has the member; so set it here.
                obj->SetMember(this, varname, val);
                return;
            }
        }
    }

    // Check locals.
    GASValue* value = FindLocal(varname);
    if (value)
    {
        // Set local var.
        *value = val;
        return;
    }

    GASSERT(Target);
    Target->SetMember(this, varname, val);
}


// Set/initialize the value of the local variable.
void    GASEnvironment::SetLocal(const GASString& varname, const GASValue& val)
{
    if (LocalFrames.size() == 0) //!AB if no local frame exist - do nothing
        return; 
    if (!LocalFrames[LocalFrames.size()-1])
        return; 

    // Is it in the current frame already?
    GASValue* pvalue = FindLocal(varname);
    if (!pvalue)
    {
        GASSERT(!varname.IsEmpty());   // null varnames are invalid!
        
        // Not in frame; create a new local var.
        AddLocal (varname, val);
        //LocalFrames.push_back(GASFrameSlot(varname, val));
    }
    else
    {
        // In frame already; modify existing var.
        //LocalFrames[index].Value = val;
        *pvalue = val;
    }
}


// Add a local var with the given name and value to our
// current local frame.  Use this when you know the var
// doesn't exist yet, since it's faster than SetLocal();
// e.G. when setting up args for a function.
void    GASEnvironment::AddLocal(const GASString& varname, const GASValue& val)
{
    GASSERT(varname.GetSize() > 0);

    //if (LocalFrames.size() == 0) //!AB Create frame if no frame exists
    //  CreateNewLocalFrame();
    //!AB Don't do this, local frame should be created intentionally. If no local frame
    // exists - ASSERT.
    GASSERT(LocalFrames.size() > 0);

    GPtr<GASLocalFrame> frame = LocalFrames[LocalFrames.size()-1];
    if (frame)
        frame->Variables.set_CaseCheck(varname, val, IsCaseSensitive());
}


// Create the specified local var if it doesn't exist already.
void    GASEnvironment::DeclareLocal(const GASString& varname)
{
    if (LocalFrames.size() == 0) //!AB if no local frame exist - do nothing
        return; 
    if (!LocalFrames[LocalFrames.size()-1])
        return; 

    // Is it in the current frame already?
    GASValue* pvalue = FindLocal(varname);
    if (!pvalue)
    {
        // Not in frame; create a new local var.
        GASSERT(varname.GetSize() > 0);   // null varnames are invalid!
        
        AddLocal(varname, GASValue());
    }
    else
    {
        // In frame already; don't mess with it.
    }
}


bool    GASEnvironment::GetMember(const GASString& varname, GASValue* val) const
{    
    return Variables.get_CaseCheck(varname, val, IsCaseSensitive());
}
void    GASEnvironment::SetMember(const GASString& varname, const GASValue& val)
{
    Variables.set_CaseCheck(varname, val, IsCaseSensitive());
}

bool    GASEnvironment::DeleteMember(const GASString& name)
{    
    GASStringHash<GASValue>::const_iterator it = Variables.find_CaseCheck(name, IsCaseSensitive());
    bool retVal = (it != Variables.end());
    if (retVal)
        Variables.remove_CaseCheck(name, IsCaseSensitive());
    return retVal;
}

void    GASEnvironment::VisitMembers(GASObjectInterface::MemberVisitor *pvisitor, UInt visitFlags) const
{    
    GUNUSED(visitFlags);
    GASStringHash<GASValue>::const_iterator it = Variables.begin();
    while(it != Variables.end())
    {
        pvisitor->Visit(it->first, it->second, 0);
        ++it;
    }
}

GASValue*   GASEnvironment::LocalRegisterPtr(UInt reg)
// Return a pointer to the specified local register.
// Local registers are numbered starting with 0.
// MA: Although DefineFunction2 index 0 has a special meaning,
// ActionScript code store_register still references it occasionally.
//
// Return value will never be NULL.  If reg is out of bounds,
// we log an error, but still return a valid Pointer (to
// global reg[0]).  So the behavior is a bit undefined, but
// not dangerous.
{
    // We index the registers from the end of the register
    // GTL::garray, so we don't have to keep base/frame
    // pointers.

    if (reg >= LocalRegister.size())
    {       
        LogError("Invalid local register %d, stack only has %d entries\n",
                 reg, (int)LocalRegister.size());

        // Fallback: use global 0.
        return &GlobalRegister[0];
    }

    return &LocalRegister[LocalRegister.size() - reg - 1];
}

const GASValue* GASEnvironment::FindLocal(const GASString& varname) const
{
    if (LocalFrames.size() > 0) 
    {
        GPtr<GASLocalFrame> localFrame = GetTopLocalFrame();
        if (!localFrame) return false;
        do {
            const GASValue* ptr = localFrame->Variables.get_CaseCheck(varname, IsCaseSensitive());
            if (ptr)
                return ptr;
            localFrame = localFrame->PrevFrame;
        } while (localFrame);
    }
    return 0;
}

GASValue*   GASEnvironment::FindLocal(const GASString& varname)
{
    if (LocalFrames.size() > 0) 
    {
        GPtr<GASLocalFrame> localFrame = GetTopLocalFrame();
        if (!localFrame) return false;
        do {
            GASValue* ptr = localFrame->Variables.get_CaseCheck(varname, IsCaseSensitive());
            if (ptr)
                return ptr;
            localFrame = localFrame->PrevFrame;
        } while (localFrame);
    }
    return 0;
}

bool    GASEnvironment::FindLocal(const GASString& varname, GASValue* pvalue) const
{
    if (LocalFrames.size() > 0) 
    {
        GPtr<GASLocalFrame> localFrame = GetTopLocalFrame();
        if (!localFrame) return false;
        do {
            GASValue* presult;
            if ((presult = localFrame->Variables.get_CaseCheck(varname, IsCaseSensitive())) != NULL)
            {
                if (pvalue)
                    *pvalue = *presult;
                return true;
            }
            localFrame = localFrame->PrevFrame;
        } while (localFrame);
    }
    return false;
}

GASLocalFrame* GASEnvironment::GetTopLocalFrame (int off) const 
{ 
    if (LocalFrames.size() - off > 0)
    {
        return LocalFrames[LocalFrames.size() - off - 1]; 
    }
    return 0;
}

GASLocalFrame*  GASEnvironment::CreateNewLocalFrame ()
{
    GPtr<GASLocalFrame> frame = *new GASLocalFrame;
    LocalFrames.push_back (frame);
    return frame;
}


// See if the given variable name is actually a sprite path
// followed by a variable name.  These come in the format:
//
//  /path/to/some/sprite/:varname
//
// (or same thing, without the last '/')
//
// or
//  path.To.Some.Var
//
// If that's the format, puts the path Part (no colon or
// trailing slash) in *path, and the varname Part (no color)
// in *var and returns true.
//
// If no colon, returns false and leaves *path & *var alone.
bool    GASEnvironment::ParsePath(GASStringContext* psc, const GASString& varPath, GASString* ppath, GASString* pvar)
{
    // Search for colon.
    int         colonIndex = -1;
    const char* cstr = varPath.ToCStr();
    const char* p;
    
    if ((p = strchr(cstr, ':')) != 0)
        colonIndex = int(p - cstr);
    else
    {
        if ((p = strrchr(cstr, '.')) != 0)
            colonIndex = int(p - cstr);
        else
        {
            if ((p = strrchr(cstr, '/')) == 0)
                return false;
        }
    }

    // Make the subparts.

    // Var.
    if (colonIndex >= 0)
        *pvar = psc->CreateString(varPath.ToCStr() + colonIndex + 1);
    else
        *pvar = psc->GetBuiltin(GASBuiltin_empty_);

    // GFxPath.
    if (colonIndex > 0)
    {
        if (varPath[colonIndex - 1] == '/')
        {
            // Trim off the extraneous trailing slash.
            colonIndex--;
        }
    }
    
    // @@ could be better.  This whole usage of GFxString is very flabby...
    if (colonIndex >= 0)
        *ppath = psc->CreateString(varPath.ToCStr(), colonIndex);
    else
        *ppath = varPath;

    return true;
}

bool    GASEnvironment::IsPath(const GASString& varPath)
{
    // This should be checked for externally, for efficiency.   
    //if (varPath.IsNotPath())
    //    return 0;
    GASSERT(varPath.IsNotPath() == false);
    if (varPath.GetHashFlags() & GASString::Flag_PathCheck) // IS a path!
        return 1;  

    // This string was not checked for being a path yet, so do the check.
    const char* pstr = varPath.ToCStr();
    if (strchr(pstr, ':') != NULL ||
        strchr(pstr, '/') != NULL ||
        strchr(pstr, '.') != NULL)
    {
        // This *IS* a path
        varPath.SetPathFlags(GASString::Flag_PathCheck|0);
        return 1;
    }
    else
    {
        varPath.SetPathFlags(GASString::Flag_PathCheck|GASString::Flag_IsNotPath);
        return 0;
    }
}

// Find the sprite/Movie represented by the given value.  The
// value might be a reference to the object itself, or a
// string giving a relative path name to the object.
GFxASCharacter* GASEnvironment::FindTargetByValue(const GASValue& val)
{
    if (val.GetType() == GASValue::OBJECT)
    {   
        // TBD: Can object be convertible to string path? Should it be?
    }
    if (val.GetType() == GASValue::CHARACTER)
    {
        return val.ToASCharacter(this);
    }
    else if (val.GetType() == GASValue::STRING)
    {
        return FindTarget(val.ToString(this));
    }
    else
    {
        LogScriptError("Error: Invalid movie clip path; neither string nor object\n");
    }
    return NULL;
}


// Search for next '.' or '/' GFxCharacter in this word.  Return
// a pointer to it, or to NULL if it wasn't found.
static const char*  GAS_NextSlashOrDot(const char* pword)
{
    for (const char* p = pword; *p; p++)
    {
        if (*p == '.' && p[1] == '.')
        {
            p++;
        }
        else if (*p == '.' || *p == '/')
        {
            return p;
        }
    }

    return NULL;
}

class StringTokenizer 
{
    const char* Str;
    const char* const EndStr;
    const char* Delimiters;
    GASString   Token;

    bool IsDelimiter(int ch) const
    {
        return strchr(Delimiters, ch) != 0;
    }

    const char* SkipDelimiters()
    {
        while (Str < EndStr) {
            if (!IsDelimiter(*Str))
                break;
            Str++;
        }
        return Str;
    }

public:
    StringTokenizer(GASStringContext *psc, const char* str, const char* delim) :
        Str(str), EndStr(str + strlen(str)), Delimiters(delim), Token(psc->GetBuiltin(GASBuiltin_empty_))
    {
    }
    StringTokenizer(GASStringContext *psc, const char* str, size_t strSize, const char* delim) :
        Str(str), EndStr(str + strSize), Delimiters(delim), Token(psc->GetBuiltin(GASBuiltin_empty_))
    {
    }
    const char* SetDelimiters(const char* delim)
    {
        Delimiters = delim;
        return delim;
    }
    bool NextToken(char* sep)
    {
        if (Str >= EndStr) return false;
        const char* pToken;

        pToken = Str;// = SkipDelimiters();

        while (Str < EndStr) {
            if (IsDelimiter(*Str))
                break;
            Str++;
        }
        *sep = *Str;
        
        if (pToken != Str && Str <= EndStr) 
        {
            size_t lastTokenSize = (size_t)(Str - pToken);
            Token = Token.GetManager()->CreateString(pToken, lastTokenSize);
        }
        else
        {
            Token = Token.GetManager()->CreateEmptyString();
        }

        ++Str; // skip delimiter
        return true;
    }

    const GASString& GetToken() const { return Token; }

    StringTokenizer& operator=(const StringTokenizer&) { return *this; }
};

static bool GAS_IsRelativePathToken(GASStringContext* psc, const GASString& pathComponent)
{
    // return (pathComponent == ".." || pathComponent == "_parent");
    return (psc->GetBuiltin(GASBuiltin_dotdot_) == pathComponent ||
            psc->GetBuiltin(GASBuiltin__parent).CompareBuiltIn_CaseCheck(pathComponent, psc->IsCaseSensitive()));
}

// Find the sprite/Movie referenced by the given path.
bool GASEnvironment::FindVariable(const GASString& path, GASValue* presult, const GTL::garray<GASWithStackEntry>* pwithStack, 
                                  GFxASCharacter **pplastTarget,
                                  GASValue* powner, bool onlyTargets, GASString* varName) const
{
    if (path.IsEmpty())
    {
        if (presult)
            presult->SetAsCharacter(Target);
        return true;
    }

    GASValue                    current;  
    bool                        currentFound        = false;
    const char*                 p                   = path.ToCStr();
    size_t                      plength             = path.GetSize();
    static const char*          onlySlashesDelim    = ":/";
    static const char* const    regularDelim        = ":./";
    const char*                 delim               = regularDelim;

    if (powner) // reset owner if necessary
        powner->SetUndefined();
    if (pplastTarget) // reset last target
        *pplastTarget = 0;

    GASSERT(Target);
    
    if (*p == '/') // if the first symbol is '/' - root level is the beginning
    {
        // Absolute path; start at the _root.
        current.SetAsCharacter(Target->GetASRootMovie());
        currentFound = true;
        p++;
        plength--;
        delim = onlySlashesDelim;
        if (powner)
            *powner = current;
    }
    else if (*p == '.') // if the first symbol is '.' use only slashes delim
    {
        delim = onlySlashesDelim;
    }

    StringTokenizer parser(GetSC(), p, plength, delim);
    char            sep;

    bool first_token = true;
    while(parser.NextToken(&sep))
    {
        const GASString& token = parser.GetToken();
        
        if (token.GetSize())
        {
            if (varName)
            {
                *varName = token;
            }
            GASValue member;
            bool     memberFound = false;
            if (current.IsCharacter() || (!currentFound && GAS_IsRelativePathToken(GetSC(), token)))
            {
                if (!currentFound)
                {
                    // the special case, if relative path is used and current is not set yet:
                    // check withStack first, or use the Target as a current
                    if (pwithStack && pwithStack->size() > 0)
                    {
                        GASObjectInterface* obj = (*pwithStack)[pwithStack->size()-1].GetObjectInterface();
                        if (obj->IsASCharacter())
                            current.SetAsCharacter(obj->ToASCharacter());
                    }
                    if (current.IsUndefined())
                        current.SetAsCharacter(Target);
                    currentFound = true;
                }
                GFxASCharacter* m = current.ToASCharacter(this)->GetRelativeTarget(token, first_token);
                if (m)
                {
                    member.SetAsCharacter(m);
                    memberFound = true;
                }
            }   
            if (!memberFound)
            {
                if (!currentFound)
                {
                    // if no current set yet - check local vars in environment
                    memberFound = GetVariableRaw(token, &member, pwithStack);
                }
                else
                {
                    if (!current.IsObject() && !current.IsCharacter() && !current.IsFunction())
                    {
                        member.SetUndefined();
                        memberFound = false;
                    }
                    else
                    {
                        GASObjectInterface* iobj = current.ToObjectInterface(this);
                        if (iobj)
                        {
                            memberFound = iobj->GetMember(const_cast<GASEnvironment*>(this), token, &member);
                            if (!memberFound)
                            {
                                member.SetUndefined();
                                memberFound = false;
                            }
                        }
                    }
                }
            }
            if (powner) // save owner if necessary
                *powner = current;
            if ((onlyTargets && !member.IsCharacter()) || !memberFound)
            {
                current.SetUndefined();
                currentFound = false;
                if (parser.NextToken(&sep))
                {   // if this is not the last token - set owner and target to null
                    if (powner) powner->SetUndefined();
                    if (pplastTarget) *pplastTarget = 0;
                    if (varName) *varName = GetBuiltin(GASBuiltin_empty_);
                }
                break; // abort the loop
            }

            current = member;
            currentFound = memberFound;
        }
        if (delim == onlySlashesDelim)
        {
            if (sep == ':')
            {
                delim = parser.SetDelimiters(regularDelim);
                if (pplastTarget && current.IsCharacter())
                {
                    *pplastTarget = current.ToASCharacter(this);
                }
            }
        }
        else
        {
            if (sep == '.')
            {
                if (pplastTarget && current.IsCharacter())
                {
                    *pplastTarget = current.ToASCharacter(this);
                }
            }
        }

        if (sep == '/')
        {
            delim = parser.SetDelimiters(onlySlashesDelim);
        }
        first_token = false;
    }
    if (pplastTarget && current.IsCharacter())
    {
        *pplastTarget = current.ToASCharacter(this);
    }
    if (powner && (!powner->IsObject() && !powner->IsCharacter() && !powner->IsFunction()))
    {
        // owner can only be an object/character/function object
        powner->SetUndefined();
    }
    if (!currentFound)
        return false;
    if (presult)
        *presult = current;
    return true;
}

// Find the sprite/Movie referenced by the given path.
// excludeFlags - 0 or NoLogOutput
//@TODO: should it be redesigned to work with FindVariable? (AB)
GFxASCharacter* GASEnvironment::FindTarget(const GASString& path, UInt excludeFlags) const
{
    if (path.IsEmpty())    
        return IsInvalidTarget ? NULL : Target;
    GASSERT(path.GetSize() > 0);

    GFxASCharacter* env = Target;
    GASSERT(env);
    
    const char* p = path.ToCStr();
    GASString   subpart(GetBuiltin(GASBuiltin_empty_));

    if (*p == '/')
    {
        // Absolute path; start at the _root.
        env = env->GetASRootMovie();
        p++;
    }
    bool first_call = true;
    for (;;)
    {
        const char* pnextSlash = GAS_NextSlashOrDot(p);    
        if (pnextSlash == p)
        {
            if (!(excludeFlags & NoLogOutput))
                LogError("Error: invalid path '%s'\n", path.ToCStr());
            break;
        }
        else if (pnextSlash)
        {
            // Cut off the slash and everything after it.
            subpart = CreateString(p, (size_t)(pnextSlash - p));
        }
        else
        {
            subpart = CreateString(p);
        }

        if (subpart.GetSize())
        {
            // Handle: '.', '..', this, _root, _parent, _levelN, display list characters
            env = env->GetRelativeTarget(subpart, first_call);
        }       

        if (env == NULL || pnextSlash == NULL)
        {
            break;
        }

        p = pnextSlash + 1;
        first_call = false;
    }
    return env;
}

GPtr<GASObject> GASEnvironment::OperatorNew(const GASFunctionRef& constructor, int nargs, int argsTopOff)
{
    GASValue    newObjVal;

    GASSERT (!constructor.IsNull ());

    if (argsTopOff < 0) argsTopOff = GetTopIndex();

    GPtr<GASObject> pnewObj;                    

    // get the prototype
    GASValue prototypeVal;
    if (!constructor->GetMemberRaw(GetSC(), GetBuiltin(GASBuiltin_prototype), &prototypeVal))
    {
        prototypeVal.SetAsObject(GetPrototype(GASBuiltin_Object));
    }

    pnewObj = *constructor->CreateNewObject(GetSC());
    if (pnewObj)
    {
        // object is allocated by CreateNewObject - set __proto__ and __ctor__

        pnewObj->Set__proto__(GetSC(), prototypeVal.ToObject());
        pnewObj->Set__constructor__(GetSC(), constructor);
    }

    GASValue result;
    GAS_Invoke(constructor, &result, pnewObj.GetPtr(), this, nargs, argsTopOff);
    if (!pnewObj)
    {
        pnewObj = result.ToObject();
        if (pnewObj)
        {
            GASFunctionRef ctor = pnewObj->Get_constructor(GetSC());
            if (ctor.IsNull() || ctor == constructor) // this is necessary for Object ctor that can return object of different type
                                                      // depending on parameter (see GASObjectProto::GlobalCtor) (!AB)
            {
                GPtr<GASObject> prototype = prototypeVal.ToObject();

                // if constructor returned an object - set __proto__ and __ctor__ for it
                pnewObj->Set__proto__(GetSC(), prototype);
                pnewObj->Set__constructor__(GetSC(), constructor);
            }
        }
    }

    if (pnewObj)
    {
        // set value to object, if constructor returned primitive value
        if (result.IsPrimitive ())
            pnewObj->SetValue(this, result);
    }

    return pnewObj;
}

GPtr<GASObject> GASEnvironment::OperatorNew(const GASString &className, int nargs, int argsTopOff)
{
    GASSERT(className != "");

    GASObject* pglobal = GetGC()->pGlobal;
    GASValue   ctor;

    if (pglobal->GetMember(this, className, &ctor))
    {
        if (ctor.IsFunction())
        {
            return OperatorNew(ctor.ToFunction(), nargs, argsTopOff);
        }
    }
    return 0;
}

GASFunctionRef GASEnvironment::GetConstructor(GASBuiltinType className)
{    
    GASValue ctor;
    if (GetGC()->pGlobal->GetMemberRaw(GetSC(), GetBuiltin(className), &ctor) &&
        ctor.IsFunction())
    {
        return ctor.ToFunction();
    }
    return 0;
}

void GASEnvironment::ForceReleaseLocalFrames ()
{
    for (GASStringHash<GASValue>::iterator iter = Variables.begin (); iter != Variables.end (); ++iter)
    {
        //GFxStringIHash<GASValue>::hash_node& var = *iter;
        GASValue& value = iter->second;
        if (value.IsFunction ())
        {
            value.V.FunctionValue.SetLocalFrame(0);
        }
    }
}

// Creates an appropriate temporary object for primitive type at env->Top(index)
// and return it as GASValue.
GASValue GASEnvironment::PrimitiveToTempObject(int index)
{    
    GASBuiltinType  ctorName;    

    const GASValue& top = Top(index);
    switch (top.GetType())
    {
    case GASValue::BOOLEAN:
        ctorName = GASBuiltin_Boolean;
        break;
    case GASValue::STRING:
        ctorName = GASBuiltin_String;
        break;
    default:
        if (top.IsNumber())
            ctorName = GASBuiltin_Number;        
        else
            return GASValue(); // undefined
    }
    
    return OperatorNew(GetBuiltin(ctorName), 1, GetTopIndex() - index).GetPtr();
}

bool GASEnvironment::RecursionGuardStart(RecursionType type, const void* ptr, int maxRecursionAllowed)
{
    if (int(type) >= int(RecursionGuards.size()))
    {
        RecursionGuards.resize(type + 1);
    }
    RecursionDescr* p;
    if ((p = RecursionGuards[type].get(ptr)) == NULL)
    {
        RecursionGuards[type].set(ptr, RecursionDescr(maxRecursionAllowed));
    }
    else
    {
        if (p->CurLevel >= p->MaxRecursionLevel)
        {
            return false;
        }
        ++p->CurLevel;
    }
    return true;
}

void GASEnvironment::RecursionGuardEnd(RecursionType type, const void* ptr)
{
    GASSERT(int(type) < int(RecursionGuards.size()));
    RecursionDescr* p;
    if ((p = RecursionGuards[type].get(ptr)) != NULL)
    {
        GASSERT(p->CurLevel);
        if (--p->CurLevel == 0)
            RecursionGuards[type].remove(ptr);
    }
    else
        GASSERT(0);
}

void GASEnvironment::Reset()
{
    ForceReleaseLocalFrames();

    Stack.Reset();
    UInt i;
    for (i = 0; i < sizeof(GlobalRegister)/sizeof(GlobalRegister[0]); ++i)
    {
        GlobalRegister[i].SetUndefined();
    }
    LocalRegister.resize(0);
    IsInvalidTarget = false;

    Variables.resize(0);
    CallStack.Reset(); // stack of calls
    RecursionGuards.resize(0); // recursion guards
    LocalFrames.resize(0);
}

//
// GFxEventId
//

GASBuiltinType      GFxEventId::GetFunctionNameBuiltinType() const
{
    // Note: There are no function names for Event_KeyPress, Event_Initialize, and 
    // Event_Construct. We use "@keyPress@", "@initialize@", and "@construct@" strings.
    static GASBuiltinType  functionTypes[Event_COUNT] =
    {        
        GASBuiltin_INVALID,             // Event_Invalid "INVALID"
        GASBuiltin_onLoad,              // Event_Load
        GASBuiltin_onEnterFrame,        // Event_EnterFrame
        GASBuiltin_onUnload,            // Event_Unload
        GASBuiltin_onMouseMove,         // Event_MouseMove
        GASBuiltin_onMouseDown,         // Event_MouseDown
        GASBuiltin_onMouseUp,           // Event_MouseUp
        GASBuiltin_onKeyDown,           // Event_KeyDown
        GASBuiltin_onKeyUp,             // Event_KeyUp
        GASBuiltin_onData,              // Event_Data
        GASBuiltin_ainitializea_,       // Event_Initialize "@initialize@"
        GASBuiltin_onPress,             // Event_Press
        GASBuiltin_onRelease,           // Event_Release
        GASBuiltin_onReleaseOutside,    // Event_ReleaseOutside
        GASBuiltin_onRollOver,          // Event_RollOver
        GASBuiltin_onRollOut,           // Event_RollOut
        GASBuiltin_onDragOver,          // Event_DragOver
        GASBuiltin_onDragOut,           // Event_DragOut
        GASBuiltin_akeyPressa_,         // Event_KeyPress  "@keyPress@"
        GASBuiltin_aconstructa_,        // Event_Construct "@construct@"
        // These are for the MoveClipLoader ActionScript only
        GASBuiltin_onLoadStart,         // Event_LoadStart
        GASBuiltin_onLoadError,         // Event_LoadError
        GASBuiltin_onLoadProgress,      // Event_LoadProgress
        GASBuiltin_onLoadInit,          // Event_LoadInit
        // These are for the XMLSocket ActionScript only
        GASBuiltin_onSockClose,         // Event_SockClose
        GASBuiltin_onSockConnect,       // Event_SockConnect
        GASBuiltin_onSockData,          // Event_SockData
        GASBuiltin_onSockXML,           // Event_SockXML
        // These are for the XML ActionScript only
        GASBuiltin_onXMLLoad,           // Event_XMLLoad
        GASBuiltin_onXMLData,           // Event_XMLData

        //!AB:? GFXSTATICSTRING("onTimer"),  // setInterval GFxTimer expired
    };  
    GASSERT(GetEventsCount() == 1);
    UInt idx;
    if (Id >= Event_Invalid && Id <= Event_LastCombined)
        idx = GBitsUtil::BitCount32(Id);
    else
        idx = (Id - Event_NextAfterCombined) + 20;
    GASSERT(idx > Event_Invalid && idx < Event_COUNT);
    //DBG:GASSERT(Id != Event_Press);

    if (idx > Event_Invalid && idx < Event_COUNT)
        return functionTypes[idx];
    return GASBuiltin_unknown_;
}

GASString   GFxEventId::GetFunctionName(GASStringContext *psc) const
{
    return psc->GetBuiltin(GetFunctionNameBuiltinType()); 
}

// converts keyCode/asciiCode from this event to the on(keyPress <>) format
int GFxEventId::ConvertToButtonKeyCode() const
{
    int kc = 0;

    // convert keycode/ascii to button's keycode
    switch (KeyCode) 
    {
        case GFxKey::Left:      kc = 1; break;
        case GFxKey::Right:     kc = 2; break;
        case GFxKey::Home:      kc = 3; break;
        case GFxKey::End:       kc = 4; break;
        case GFxKey::Insert:    kc = 5; break;
        case GFxKey::Delete:    kc = 6; break;
        case GFxKey::Backspace: kc = 8; break;
        case GFxKey::Return:    kc = 13; break;
        case GFxKey::Up:        kc = 14; break;
        case GFxKey::Down:      kc = 15; break;
        case GFxKey::PageUp:    kc = 16; break;
        case GFxKey::PageDown:  kc = 17; break;
        case GFxKey::Tab:       kc = 18; break;
        case GFxKey::Escape:    kc = 19; break;
        default:
            if (AsciiCode >= 32)
                kc = AsciiCode;
    }
    return kc;
}

// Scan for functions in local frame and convert them to internal ones.
void GASLocalFrame::ReleaseFramesForLocalFuncs()
{
    for (GASStringHash<GASValue>::iterator iter = Variables.begin (); iter != Variables.end (); ++iter)
    {
        //GFxStringIHash<GASValue>::Node& var = *iter;
        GASValue& value = iter->second;
        if (value.IsFunction ())
        {
            // check, is this function declared locally or not. Only if the function
            // was declared locally, then we need to "weak" it by SetInternal. 
            // Otherwise, it might be just a local pointer to non-local function.
            if (LocallyDeclaredFuncs.get(value.V.FunctionValue.GetObjectPtr()) != 0)
                value.V.FunctionValue.SetInternal(true);
        }
    }
    SuperThis = 0;
}

//
// Disassembler
//


#define COMPILE_DISASM 1



#ifndef COMPILE_DISASM

// No disassembler in this version...
void    GFxDisasm::LogDisasm(const unsigned char* InstructionData)
{
    Log("<no disasm>\n");
}

#else // COMPILE_DISASM

// Disassemble one instruction to the log.
void    GFxDisasm::LogDisasm(const unsigned char* InstructionData)
{
    enum ArgFormatType {
        ARG_NONE = 0,
        ARG_STR,
        ARG_HEX,    // default hex dump, in case the format is unknown or unsupported
        ARG_U8,
        ARG_U16,
        ARG_S16,
        ARG_PUSH_DATA,
        ARG_DECL_DICT,
        ARG_FUNCTION,
        ARG_FUNCTION2
    };
    class GASInstInfo
    {
    public:
        int actionId;
        const char* Instruction;

        ArgFormatType   ArgFormat;
    };

    static GASInstInfo  InstructionTable[] = {
        { 0x04, "next_frame", ARG_NONE },
        { 0x05, "prev_frame", ARG_NONE },
        { 0x06, "play", ARG_NONE },
        { 0x07, "stop", ARG_NONE },
        { 0x08, "toggle_qlty", ARG_NONE },
        { 0x09, "stop_sounds", ARG_NONE },
        { 0x0A, "add", ARG_NONE },
        { 0x0B, "sub", ARG_NONE },
        { 0x0C, "mul", ARG_NONE },
        { 0x0D, "div", ARG_NONE },
        { 0x0E, "equ", ARG_NONE },
        { 0x0F, "lt", ARG_NONE },
        { 0x10, "and", ARG_NONE },
        { 0x11, "or", ARG_NONE },
        { 0x12, "not", ARG_NONE },
        { 0x13, "str_eq", ARG_NONE },
        { 0x14, "str_len", ARG_NONE },
        { 0x15, "substr", ARG_NONE },
        { 0x17, "pop", ARG_NONE },
        { 0x18, "floor", ARG_NONE },
        { 0x1C, "get_var", ARG_NONE },
        { 0x1D, "set_var", ARG_NONE },
        { 0x20, "set_target2", ARG_NONE },
        { 0x21, "str_cat", ARG_NONE },
        { 0x22, "get_prop", ARG_NONE },
        { 0x23, "set_prop", ARG_NONE },
        { 0x24, "dup_sprite", ARG_NONE },
        { 0x25, "rem_sprite", ARG_NONE },
        { 0x26, "trace", ARG_NONE },
        { 0x27, "start_drag", ARG_NONE },
        { 0x28, "stop_drag", ARG_NONE },
        { 0x29, "str_lt", ARG_NONE },
        { 0x2A, "throw", ARG_NONE },
        { 0x2B, "cast_object", ARG_NONE },
        { 0x2C, "implements", ARG_NONE },
        { 0x30, "random", ARG_NONE },
        { 0x31, "mb_length", ARG_NONE },
        { 0x32, "ord", ARG_NONE },
        { 0x33, "chr", ARG_NONE },
        { 0x34, "get_timer", ARG_NONE },
        { 0x35, "substr_mb", ARG_NONE },
        { 0x36, "ord_mb", ARG_NONE },
        { 0x37, "chr_mb", ARG_NONE },
        { 0x3A, "delete", ARG_NONE },
        { 0x3B, "delete2", ARG_STR },
        { 0x3C, "set_local", ARG_NONE },
        { 0x3D, "call_func", ARG_NONE },
        { 0x3E, "return", ARG_NONE },
        { 0x3F, "mod", ARG_NONE },
        { 0x40, "new", ARG_NONE },
        { 0x41, "decl_local", ARG_NONE },
        { 0x42, "decl_array", ARG_NONE },
        { 0x43, "decl_obj", ARG_NONE },
        { 0x44, "type_of", ARG_NONE },
        { 0x45, "get_target", ARG_NONE },
        { 0x46, "enumerate", ARG_NONE },
        { 0x47, "add_t", ARG_NONE },
        { 0x48, "lt_t", ARG_NONE },
        { 0x49, "eq_t", ARG_NONE },
        { 0x4A, "number", ARG_NONE },
        { 0x4B, "string", ARG_NONE },
        { 0x4C, "dup", ARG_NONE },
        { 0x4D, "swap", ARG_NONE },
        { 0x4E, "get_member", ARG_NONE },
        { 0x4F, "set_member", ARG_NONE },
        { 0x50, "inc", ARG_NONE },
        { 0x51, "dec", ARG_NONE },
        { 0x52, "call_method", ARG_NONE },
        { 0x53, "new_method", ARG_NONE },
        { 0x54, "is_inst_of", ARG_NONE },
        { 0x55, "enum_object", ARG_NONE },
        { 0x60, "bit_and", ARG_NONE },
        { 0x61, "bit_or", ARG_NONE },
        { 0x62, "bit_xor", ARG_NONE },
        { 0x63, "shl", ARG_NONE },
        { 0x64, "asr", ARG_NONE },
        { 0x65, "lsr", ARG_NONE },
        { 0x66, "eq_strict", ARG_NONE },
        { 0x67, "gt_t", ARG_NONE },
        { 0x68, "gt_str", ARG_NONE },
        { 0x69, "extends", ARG_NONE },
        
        { 0x81, "goto_frame", ARG_U16 },
        { 0x83, "get_url", ARG_STR },
        { 0x87, "store_register", ARG_U8 },
        { 0x88, "decl_dict", ARG_DECL_DICT },
        { 0x8A, "wait_for_frame", ARG_HEX },
        { 0x8B, "set_target", ARG_STR },
        { 0x8C, "goto_frame_lbl", ARG_STR },
        { 0x8D, "wait_for_fr_exp", ARG_HEX },
        { 0x8E, "function2", ARG_FUNCTION2 },
        { 0x8F, "try", ARG_NONE },
        { 0x94, "with", ARG_U16 },
        { 0x96, "push_data", ARG_PUSH_DATA },
        { 0x99, "goto", ARG_S16 },
        { 0x9A, "get_url2", ARG_HEX },
        { 0x9B, "function", ARG_FUNCTION },
        { 0x9D, "branch_if_true", ARG_S16 },
        { 0x9E, "call_frame", ARG_HEX },
        { 0x9F, "goto_frame_exp", ARG_HEX },
        { 0x00, "<end>", ARG_NONE }
    };

    int actionId = InstructionData[0];
    GASInstInfo*    info = NULL;

    for (int i = 0; ; i++)
    {
        if (InstructionTable[i].actionId == actionId)
        {
            info = &InstructionTable[i];
        }

        if (InstructionTable[i].actionId == 0)
        {
            // Stop at the end of the table and give up.
            break;
        }
    }

    ArgFormatType   fmt = ARG_HEX;

    // Show instruction.
    if (info == NULL)
    {
        Log("<unknown>[0x%02X]", actionId);
    }
    else
    {
        Log("%-15s", info->Instruction);
        fmt = info->ArgFormat;
    }

    // Show instruction Argument(s).
    if (actionId & 0x80)
    {
        GASSERT(fmt != ARG_NONE);

        int length = InstructionData[1] | (InstructionData[2] << 8);

        // Log(" [%d]", length);

        if (fmt == ARG_HEX)
        {
            for (int i = 0; i < length; i++)
            {
                Log(" 0x%02X", InstructionData[3 + i]);
            }
            Log("\n");
        }
        else if (fmt == ARG_STR)
        {
            Log(" \"");
            for (int i = 0; i < length; i++)
            {
                Log("%c", InstructionData[3 + i]);
            }
            Log("\"\n");
        }
        else if (fmt == ARG_U8)
        {
            int val = InstructionData[3];
            Log(" %d\n", val);
        }
        else if (fmt == ARG_U16)
        {
            int val = InstructionData[3] | (InstructionData[4] << 8);
            Log(" %d\n", val);
        }
        else if (fmt == ARG_S16)
        {
            int val = InstructionData[3] | (InstructionData[4] << 8);
            if (val & 0x8000) val |= ~0x7FFF;   // sign-extend
            Log(" %d\n", val);
        }
        else if (fmt == ARG_PUSH_DATA)
        {
            Log("\n");
            int i = 0;
            while (i < length)
            {
                int type = InstructionData[3 + i];
                i++;
                Log("\t\t");    // indent
                if (type == 0)
                {
                    // string
                    Log("\"");
                    while (InstructionData[3 + i])
                    {
                        Log("%c", InstructionData[3 + i]);
                        i++;
                    }
                    i++;
                    Log("\"\n");
                }
                else if (type == 1)
                {
                    // Float (little-endian)
                    union {
                        Float   F;
                        UInt32  I;
                    } u;
                    //compiler_assert(sizeof(u) == sizeof(u.I));

                    memcpy(&u.I, InstructionData + 3 + i, 4);
                    u.I = GByteUtil::LEToSystem(u.I);
                    i += 4;

                    Log("(Float) %f\n", u.F);
                }
                else if (type == 2)
                {
                    Log("NULL\n");
                }
                else if (type == 3)
                {
                    Log("undef\n");
                }
                else if (type == 4)
                {
                    // contents of register
                    int reg = InstructionData[3 + i];
                    i++;
                    Log("reg[%d]\n", reg);
                }
                else if (type == 5)
                {
                    int boolVal = InstructionData[3 + i];
                    i++;
                    Log("bool(%d)\n", boolVal);
                }
                else if (type == 6)
                {
                    // double
                    // wacky format: 45670123
#ifdef GFC_NO_DOUBLE
                    union
                    {
                        Float   F;
                        UInt32  I;
                    } u;

                    // convert ieee754 64bit to 32bit for systems without proper double
                    SInt    sign = (InstructionData[3 + i + 3] & 0x80) >> 7;
                    SInt    expo = ((InstructionData[3 + i + 3] & 0x7f) << 4) + ((InstructionData[3 + i + 2] & 0xf0) >> 4);
                    SInt    mant = ((InstructionData[3 + i + 2] & 0x0f) << 19) + (InstructionData[3 + i + 1] << 11) +
                        (InstructionData[3 + i + 0] << 3) + ((InstructionData[3 + i + 7] & 0xf8) >> 5);

                    if (expo == 2047)
                        expo = 255;
                    else if (expo - 1023 > 127)
                    {
                        expo = 255;
                        mant = 0;
                    }
                    else if (expo - 1023 < -126)
                    {
                        expo = 0;
                        mant = 0;
                    }
                    else
                        expo = expo - 1023 + 127;

                    u.I = (sign << 31) + (expo << 23) + mant;
                    i += 8;

                    Log("(double) %f\n", u.F);
#else
                    union {
                        double  D;
                        UInt64  I;
                        struct {
                            UInt32  Lo;
                            UInt32  Hi;
                        } Sub;
                    } u;
                    GCOMPILER_ASSERT(sizeof(u) == sizeof(u.I));

                    memcpy(&u.Sub.Hi, InstructionData + 3 + i, 4);
                    memcpy(&u.Sub.Lo, InstructionData + 3 + i + 4, 4);
                    u.I = GByteUtil::LEToSystem(u.I);
                    i += 8;

                    Log("(double) %f\n", u.D);
#endif
                }
                else if (type == 7)
                {
                    // int32
                    SInt32  val = InstructionData[3 + i]
                        | (InstructionData[3 + i + 1] << 8)
                        | (InstructionData[3 + i + 2] << 16)
                        | (InstructionData[3 + i + 3] << 24);
                    i += 4;
                    Log("(int) %d\n", val);
                }
                else if (type == 8)
                {
                    int id = InstructionData[3 + i];
                    i++;
                    Log("DictLookup[%d]\n", id);
                }
                else if (type == 9)
                {
                    int id = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
                    i += 2;
                    Log("DictLookupLg[%d]\n", id);
                }
            }
        }
        else if (fmt == ARG_DECL_DICT)
        {
            int i = 0;
            int count = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log(" [%d]\n", count);

            // Print strings.
            for (int ct = 0; ct < count; ct++)
            {
                Log("\t\t");    // indent

                Log("\"");
                while (InstructionData[3 + i])
                {
                    // safety check.
                    if (i >= length)
                    {
                        Log("<disasm error -- length exceeded>\n");
                        break;
                    }

                    Log("%c", InstructionData[3 + i]);
                    i++;
                }
                Log("\"\n");
                i++;
            }
        }
        else if (fmt == ARG_FUNCTION2)
        {
            // Signature info for a function2 opcode.
            int i = 0;
            const char* functionName = (const char*) &InstructionData[3 + i];
            i += (int)strlen(functionName) + 1;

            int argCount = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            int regCount = InstructionData[3 + i];
            i++;

            Log("\n\t\tname = '%s', ArgCount = %d, RegCount = %d\n",
                functionName, argCount, regCount);

            UInt16  flags = (InstructionData[3 + i]) | (InstructionData[3 + i + 1] << 8);
            i += 2;

            // @@ What is the difference between "super" and "Parent"?

            bool    preloadGlobal = (flags & 0x100) != 0;
            bool    preloadParent = (flags & 0x80) != 0;
            bool    preloadRoot   = (flags & 0x40) != 0;
            bool    suppressSuper = (flags & 0x20) != 0;
            bool    preloadSuper  = (flags & 0x10) != 0;
            bool    suppressArgs  = (flags & 0x08) != 0;
            bool    preloadArgs   = (flags & 0x04) != 0;
            bool    suppressThis  = (flags & 0x02) != 0;
            bool    preloadThis   = (flags & 0x01) != 0;

            Log("\t\t        pg = %d\n"
                "\t\t        pp = %d\n"
                "\t\t        pr = %d\n"
                "\t\tss = %d, ps = %d\n"
                "\t\tsa = %d, pa = %d\n"
                "\t\tst = %d, pt = %d\n",
                int(preloadGlobal),
                int(preloadParent),
                int(preloadRoot),
                int(suppressSuper),
                int(preloadSuper),
                int(suppressArgs),
                int(preloadArgs),
                int(suppressThis),
                int(preloadThis));

            for (int argi = 0; argi < argCount; argi++)
            {
                int argRegister = InstructionData[3 + i];
                i++;
                const char* argName = (const char*) &InstructionData[3 + i];
                i += (int)strlen(argName) + 1;

                Log("\t\targ[%d] - reg[%d] - '%s'\n", argi, argRegister, argName);
            }

            int functionLength = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log("\t\tfunction length = %d\n", functionLength);
        }
        else if (fmt == ARG_FUNCTION)
        {
            // Signature info for a function opcode.
            int i = 0;
            const char* functionName = (const char*) &InstructionData[3 + i];
            i += (int)strlen(functionName) + 1;

            int argCount = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log("\n\t\tname = '%s', ArgCount = %d\n",
                functionName, argCount);

            for (int argi = 0; argi < argCount; argi++)
            {
                const char* argName = (const char*) &InstructionData[3 + i];
                i += (int)strlen(argName) + 1;

                Log("\t\targ[%d] - '%s'\n", argi, argName);
            }

            int functionLength = InstructionData[3 + i] | (InstructionData[3 + i + 1] << 8);
            i += 2;

            Log("\t\tfunction length = %d\n", functionLength);
        }

    }
    else
    {
        Log("\n");
    }
}


#endif // COMPILE_DISASM
