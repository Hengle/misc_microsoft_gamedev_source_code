/**********************************************************************

Filename    :   GFxFunction.cpp
Content     :   
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxPlayerImpl.h"
#include "GFxAction.h"
#include "GFxFunction.h"
#include "GFxArray.h"
#include "GFxASString.h"
#include "GFxStringObject.h"

#include <stdio.h>
#include <stdlib.h>

GASCFunctionDef::GASCFunctionDef (GASObjectInterface* prototype, GASCFunctionPtr func) :
   pFunction (func)
{
    GUNUSED(prototype);
}

GASCFunctionDef::GASCFunctionDef (GASCFunctionPtr func) :
   pFunction (func)
{
}

void GASCFunctionDef::Invoke (const GASFnCall& fn, GASLocalFrame*, GASEnvironment*)
{
    if (0 != pFunction) {
        GASObjectInterface* pthis   = fn.ThisPtr;
        if (pthis && pthis->IsSuper())
        {
            GASObjectInterface* prealThis = static_cast<GASSuperObject*>(pthis)->GetRealThis();
            GASFnCall fn2(fn.Result, prealThis, fn.Env, fn.NArgs, fn.FirstArgBottomIndex);
            (pFunction)(fn2);
            static_cast<GASSuperObject*>(pthis)->ResetAltProto(); // reset alternative proto, if set
        }
        else
            (pFunction)(fn);
    }
}

bool GASCFunctionDef::operator== (const GASFunctionDef& f) const
{
    if (f.IsCFunction ()) {
        const GASCFunctionDef& cf = static_cast <const GASCFunctionDef&> (f);
        return pFunction == cf.pFunction;
    }
    return false;
}

bool GASCFunctionDef::operator!= (const GASFunctionDef& f) const
{
    if (f.IsCFunction ()) {
        const GASCFunctionDef& cf = static_cast <const GASCFunctionDef&> (f);
        return pFunction != cf.pFunction;
    }
    return false;
}

GASEnvironment* GASCFunctionDef::GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>*)
{
    return fn.Env;
}

//
// GASAsFunctionDef
//
struct GASAsFunctionContext : public GNewOverrideBase
{
    GWeakPtr<GFxMovieRoot>      MovieRoot;
    GPtr<GFxCharacterHandle>    TargetHandle;
};

GASAsFunctionDef::GASAsFunctionDef(GASActionBuffer* ab, GASEnvironment* env, int start, 
                                   const GTL::garray<GASWithStackEntry>& WithStack,
                                   GASActionBuffer::ExecuteType execType)
    :
    
    pActionBuffer(ab),
    WithStack(WithStack),
    StartPc(start),
    Length(0),
    ExecType((UByte)execType),
    LocalRegisterCount(0),
    Function2Flags(0),
    // Need to pass default value for copy-constructing array
    Args(ArgSpec(0, env->GetBuiltin(GASBuiltin_empty_)))
{
    GASSERT(pActionBuffer);
    
    if (env && (execType!= GASActionBuffer::Exec_Event))
    {
        GASAsFunctionContext* pContextData = new GASAsFunctionContext();

        GFxASCharacter* ch = env->GetTarget();
        GASSERT (ch != 0);
        pContextData->TargetHandle = ch->GetCharacterHandle();
        pContextData->MovieRoot = ch->GetMovieRoot();
        this->pContextData = pContextData;
    }
    else
        pContextData = 0;
}
GASAsFunctionDef::~GASAsFunctionDef()
{
    if (pContextData) delete reinterpret_cast<GASAsFunctionContext*>(pContextData);
}

GFxASCharacter* GASAsFunctionDef::GetTargetCharacter()
{
    if (!pContextData) return 0;

    GASAsFunctionContext* pContextData = reinterpret_cast<GASAsFunctionContext*>(this->pContextData);
    GPtr<GFxMovieRoot> movieRoot = pContextData->MovieRoot;
    if (!movieRoot) return 0;

    return pContextData->TargetHandle->ResolveCharacter(movieRoot);
}

GASEnvironment* GASAsFunctionDef::GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh)
{
    GPtr<GFxASCharacter> ch = GetTargetCharacter();
    GASEnvironment*     pourEnv = 0;
    if (ch)
    {
        pourEnv = ch->GetASEnvironment();
    }
    if (ptargetCh)
        *ptargetCh = ch;

    GASSERT(pourEnv || fn.Env);

    pourEnv = pourEnv ? pourEnv : fn.Env; // the final decision about environment...
    return pourEnv;
}

// Dispatch.
void    GASAsFunctionDef::Invoke (const GASFnCall& fn, GASLocalFrame* localFrame, GASEnvironment* pourEnv)
{
    // if environment (pourEnv) is not NULL then use it (and it MUST be the same as 
    // GASAsFunctionDef::GetEnvironment method returns, otherwise - call to GASAsFunctionDef::GetEnvironment
    GPtr<GFxASCharacter> ch;
    if (!pourEnv)
    {
        pourEnv = GetEnvironment(fn, &ch);
    }
    else
    {
        GASSERT(pourEnv == GetEnvironment(fn, 0));
    }

    // 'this' can be different from currently accessible environment scope.
    // In most 'this' is passed through fn, while the implicit scope
    // 'pEnv' is the scope of the original function declaration.

    GASObjectInterface* pthis   = fn.ThisPtr;
    //GASSERT(pthis); // pthis can be null (i.e. undefined), for example inside interval's handler

    GASSERT(pourEnv);   

    // Set up local stack frame, for parameters and locals.
    int LocalStackTop = pourEnv->GetLocalFrameTop();

    GPtr<GASLocalFrame> curLocalFrame;
    if (IsFunction2() || (GetExecType() == GASActionBuffer::Exec_Function))
    {
        curLocalFrame = pourEnv->CreateNewLocalFrame();
        curLocalFrame->PrevFrame = localFrame;
    }
    else
        pourEnv->AddFrameBarrier();

    GPtr<GFxASCharacter> passedThisCh;
    GPtr<GASObject>      passedThisObj;
    GASObjectInterface* passedThis = pthis;
    // need to hold passedThis to avoid its release during the function execution
    if (passedThis)
    {
        if (passedThis->IsASCharacter())
            passedThisCh = passedThis->ToASCharacter();
        else if (passedThis->IsASObject())
            passedThisObj = passedThis->ToASObject();
    }

    if (pthis && pthis->IsSuper())
    {
        pthis = static_cast<GASSuperObject*>(pthis)->GetRealThis();
    }

    if (!IsFunction2())
    {
        int i;
        int version = pourEnv->GetVersion();

        // AB, Flash 6 and below uses function 1 for methods and events too.
        // So, we need to care about setting correct "this".
        if (pthis)
        {
            GASValue thisVal;
            thisVal.SetAsObjectInterface(pthis);
            pourEnv->AddLocal(pourEnv->GetBuiltin(GASBuiltin_this), thisVal);
        }

        // save some info for recreation of "super". For function1 "super" is created
        // on demand.
        if (version >= 6 && curLocalFrame)
        {
            curLocalFrame->SuperThis = passedThis;
            /*GASValue superVal;
            superVal.SetAsObjectInterface(passedThis);
            pourEnv->AddLocal("super", superVal);*/
        }
        // Conventional function.

        if (version >= 5)
        {
            GASStringContext *psc = pourEnv->GetSC();
            // arguments array. There is no way to detect necessity of this
            // array for Function1 unlike to Function2, that is why create it always.
            GPtr<GASArrayObject> pargArray = *new GASArrayObject(pourEnv);
            pargArray->Resize(fn.NArgs);
            for (i = 0; i < fn.NArgs; i++)
                pargArray->SetElement(i, fn.Arg(i));
            
            pourEnv->AddLocal(psc->GetBuiltin(GASBuiltin_arguments), GASValue(pargArray.GetPtr()));
            pargArray->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_callee), pourEnv->CallTop(0), 
                GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
            pargArray->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_caller), pourEnv->CallTop(1), 
                GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
        }

        // Push the arguments onto the local frame.
        int ArgsToPass = GTL::gmin<int>(fn.NArgs, (int)Args.size());
        for (i = 0; i < ArgsToPass; i++)
        {
            GASSERT(Args[i].Register == 0);
            pourEnv->AddLocal(Args[i].Name, fn.Arg(i));
        }
        // need to fill remaining arguments by "undefined" values
        for (int n = (int)Args.size(); i < n; i++)
        {
            GASSERT(Args[i].Register == 0);
            pourEnv->AddLocal(Args[i].Name, GASValue());
        }
    }
    else
    {
        // function2: most args go in registers; any others get pushed.
        
        // Create local registers.
        pourEnv->AddLocalRegisters(LocalRegisterCount);

        // Handle the explicit args.
        int ArgsToPass = GTL::gmin<int>(fn.NArgs, (int)Args.size());
        int i;
        for (i = 0; i < ArgsToPass; i++)
        {
            if (Args[i].Register == 0)
            {
                // Conventional arg passing: create a local var.
                pourEnv->AddLocal(Args[i].Name, fn.Arg(i));
            }
            else
            {
                // Pass argument into a register.
                int reg = Args[i].Register;
                *(pourEnv->LocalRegisterPtr(reg)) = fn.Arg(i);
            }
        }
        // need to fill remaining aguments by "undefined" values
        for (int n = (int)Args.size(); i < n; i++)
        {
            if (Args[i].Register == 0)
            {
                // Conventional arg passing: create a local var.
                pourEnv->AddLocal(Args[i].Name, GASValue());
            }
        }

        GPtr<GASObject> superObj;
        if ((Function2Flags & 0x10) || !(Function2Flags & 0x20))
        {
            // need to create super
            GASSERT(passedThis);
        
            GPtr<GASObject> proto = passedThis->Get__proto__();
            //printf ("!!! passedThis.__proto__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(proto));

            //printf ("!!! real pthis.__proto__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(pthis->Get__proto__(pourEnv)));
            if (proto)
            {
                GASFunctionRef __ctor__ = proto->Get__constructor__(pourEnv->GetSC());
                //printf ("!!! __proto__.__ctor__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(__ctor__.GetObjectPtr()));
                //printf ("!!! __proto__.__proto__ = %s\n", (const char*)pourEnv->GetGC()->FindClassName(proto->Get__proto__(pourEnv)));
                superObj = *new GASSuperObject(proto->Get__proto__(), pthis, __ctor__);
            }
        }

        // Handle the implicit args.
        int CurrentReg = 1;
        if (Function2Flags & 0x01)
        {
            // preload 'this' into a register.
            if (pthis)
                (*(pourEnv->LocalRegisterPtr(CurrentReg))).SetAsObjectInterface(pthis);
            else
                (*(pourEnv->LocalRegisterPtr(CurrentReg))).SetUndefined();
            CurrentReg++;
        }

        if (Function2Flags & 0x02)
        {
            // Don't put 'this' into a local var.
        }
        else
        {
            // Put 'this' in a local var.
            GASValue pthisVal;
            if (pthis) pthisVal.SetAsObjectInterface(pthis);
            pourEnv->AddLocal(pourEnv->GetBuiltin(GASBuiltin_this), pthisVal);
        }

        // Init arguments GTL::garray, if it's going to be needed.
        GPtr<GASArrayObject>    pargArray;
        if ((Function2Flags & 0x04) || ! (Function2Flags & 0x08))
        {
            pargArray = *new GASArrayObject(pourEnv);
            pargArray->Resize(fn.NArgs);
            for (int i = 0; i < fn.NArgs; i++)
                pargArray->SetElement(i, fn.Arg(i));
        }

        if (Function2Flags & 0x04)
        {
            // preload 'arguments' into a register.
            (*(pourEnv->LocalRegisterPtr(CurrentReg))).SetAsObject(pargArray.GetPtr());
            CurrentReg++;
        }

        if (Function2Flags & 0x08)
        {
            // Don't put 'arguments' in a local var.
        }
        else
        {
            // Put 'arguments' in a local var.
            pourEnv->AddLocal(pourEnv->GetBuiltin(GASBuiltin_arguments), GASValue(pargArray.GetPtr()));
            pargArray->SetMemberRaw(pourEnv->GetSC(), pourEnv->GetBuiltin(GASBuiltin_callee), pourEnv->CallTop(0), 
                GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
            pargArray->SetMemberRaw(pourEnv->GetSC(), pourEnv->GetBuiltin(GASBuiltin_caller), pourEnv->CallTop(1), 
                GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
        }

        if (Function2Flags & 0x10)
        {
            // Put 'super' in a register.
            //GASSERT(superObj);

            (*(pourEnv->LocalRegisterPtr(CurrentReg))).SetAsObject(superObj);
            //(*(pourEnv->LocalRegisterPtr(CurrentReg))).SetAsFunction(superObj);
            CurrentReg++;
        }

        if (Function2Flags & 0x20)
        {
            // Don't put 'super' in a local var.
        }
        else
        {
            // Put 'super' in a local var.
            //GASSERT(superObj);

            GASValue superVal;
            superVal.SetAsObject(superObj);
            pourEnv->AddLocal(pourEnv->GetBuiltin(GASBuiltin_super), superVal);
        }

        if (Function2Flags & 0x40)
        {
            // Put '_root' in a register.
            (*(pourEnv->LocalRegisterPtr(CurrentReg))).SetAsCharacter(
                pourEnv->GetTarget()->GetASRootMovie());
            CurrentReg++;
        }

        if (Function2Flags & 0x80)
        {
            // Put 'Parent' in a register.
            GASValue    parent; 
            pourEnv->GetVariable(pourEnv->GetBuiltin(GASBuiltin__parent), &parent);
            (*(pourEnv->LocalRegisterPtr(CurrentReg))) = parent;
            CurrentReg++;
        }

        if (Function2Flags & 0x100)
        {
            // Put 'pGlobal' in a register.
            (*(pourEnv->LocalRegisterPtr(CurrentReg))).SetAsObject(pourEnv->GetGC()->pGlobal);
            CurrentReg++;
        }
    }

    // Execute the actions.
    pActionBuffer->Execute(pourEnv, StartPc, Length, fn.Result, WithStack, GetExecType());

    if (passedThis && passedThis->IsSuper())
    {
        static_cast<GASSuperObject*>(passedThis)->ResetAltProto(); // reset alternative proto, if set
    }

    if (!IsFunction2() || !(Function2Flags & 0x02))
    {
        // wipe explicit "this" off from the local frame to prevent memory leak
        pourEnv->SetLocal(pourEnv->GetBuiltin(GASBuiltin_this), GASValue());
    }
    if (!IsFunction2() || !(Function2Flags & 0x20))
    {
        // wipe explicit "super" off from the local frame to prevent memory leak
        pourEnv->SetLocal(pourEnv->GetBuiltin(GASBuiltin_super), GASValue());
    }

    if (curLocalFrame)
        curLocalFrame->ReleaseFramesForLocalFuncs ();

    // Clean up stack frame.
    pourEnv->SetLocalFrameTop(LocalStackTop);

    if (IsFunction2())
    {
        // Clean up the local registers.
        pourEnv->DropLocalRegisters(LocalRegisterCount);
    }
}


bool GASAsFunctionDef::operator== (const GASFunctionDef& f) const
{
    if (f.IsAsFunction ()) {
        //AB not sure how to compare and even do we need it or not...
        //const GASAsFunctionDef& cf = static_cast <const GASAsFunctionDef&> (f);
        //return pFunction == cf.pFunction;
    }
    return false;
}

bool GASAsFunctionDef::operator!= (const GASFunctionDef& f) const
{
    if (f.IsAsFunction ()) {
        //AB not sure how to compare and even do we need it or not...
        //const GASAsFunctionDef& cf = static_cast <const GASAsFunctionDef&> (f);
        //return pFunction == cf.pFunction;
    }
    return false;
}

//
////////// GASFunctionObject ///////////
//
GASFunctionObject::GASFunctionObject (GASStringContext* psc) 
    : Def (0)
{
    GUNUSED(psc);
    // NOTE: psc can be 0 here to allow for default constructor; if we need 
    // a valid 'psc' here in the future we need to remove the default constructor.
}

GASFunctionObject::GASFunctionObject (GASEnvironment* penv) 
    : Def (0)
{
    GUNUSED(penv);
}

GASFunctionObject::GASFunctionObject (GASEnvironment* penv, GASFunctionDef* func) 
    : Def (func)
{
    GUNUSED(penv);
}

// this constructor is used for C functions
GASFunctionObject::GASFunctionObject(GASStringContext* psc, GASObject* pprototype, GASCFunctionPtr func)
    : Def (new GASCFunctionDef (func))
{
    // C-function objects have __proto__ = Object.prototype
    //!AB, correction: C-function objects have __proto__ = Function.prototype (see GASFunctionObject::GetMember)
    //Set__proto__ (pprototype);
    GUNUSED(pprototype); // not sure yet, is it necessary or not at all... (AB)
    Set__proto__(psc, 0);
}

GASFunctionObject::GASFunctionObject (GASCFunctionPtr func) :
    Def (new GASCFunctionDef (func))
{
}

GASFunctionObject::GASFunctionObject (GASFunctionDef* func) 
    : Def (func)
{
}

GASFunctionObject::~GASFunctionObject ()
{
    delete Def;
}

void GASFunctionObject::SetProtoAndCtor(GASStringContext* psc, GASObject* pprototype)
{
    Set__proto__(psc, pprototype);
    
    // function objects have "constructor" property as well
    GASFunctionRef ctor = pprototype->Get_constructor(psc);
    if (!ctor.IsNull())
        Set_constructor(psc, ctor);
}

void GASFunctionObject::Invoke (const GASFnCall& fn, GASLocalFrame* localFrame) const
{
    if (!Def) return;

    GASEnvironment* penv = 0;
    GPtr<GFxASCharacter> targetCh;
    if (IsAsFunction())
    {
        // push current function object into the stack.
        // need for atguments.callee/.caller.
        penv = Def->GetEnvironment(fn, &targetCh);
        if (penv) penv->CallPush(this);
    }
    Def->Invoke (fn, localFrame, penv);
    if (IsAsFunction() && penv)
        penv->CallPop();
}

bool GASFunctionObject::operator== (const GASFunctionObject& f) const
{
    return Def == f.Def;
}

bool GASFunctionObject::operator!= (const GASFunctionObject& f) const
{
    return Def != f.Def;
}

bool GASFunctionObject::operator== (const GASFunctionDef& f) const
{
    if (Def == NULL) return false;
    return (*Def) == f;
}

bool GASFunctionObject::operator!= (const GASFunctionDef& f) const
{
    if (Def == NULL) return false;
    return (*Def) != f;
}
 
bool GASFunctionObject::IsCFunction () const
{
    if (!Def) return false;
    return Def->IsCFunction ();
}

bool GASFunctionObject::IsAsFunction () const
{
    if (!Def) return false;
    return Def->IsAsFunction ();
}

GASFunctionRef  GASFunctionObject::ToFunction()
{
    return GASFunctionRef(this);
}

/*
GASObject* GASFunctionObject::Get__proto__()
{ 
    //!AB should return pointer to Function.prototype for C func. TODO!
    //if (!pProto && penv)
    //{
    //    GASObject* funcProto = penv->GetPrototype(GASBuiltin_Function);
    //    return funcProto;
    //}
    return pProto; 
}*/

// returns number of arguments expected by this function;
// returns -1 if number of arguments is unknown (for C functions)
int     GASFunctionObject::GetNumArgs() const
{
    if (!Def || Def->IsCFunction()) return -1;
    return static_cast<GASAsFunctionDef*>(Def)->GetNumArgs();
}

//
//////////// GASFunctionRefBase //////////////////
//
void GASFunctionRefBase::Init(GASFunctionObject* funcObj, UByte flags)
{
    Flags = flags;
    Function = funcObj; 
    if (!(Flags & FuncRef_Weak) && Function)
        Function->AddRef();
    LocalFrame = 0; 
}

void GASFunctionRefBase::Init(GASFunctionObject& funcObj, UByte flags)
{
    Flags = flags;
    Function = &funcObj; 
    LocalFrame = 0; 
}

void GASFunctionRefBase::Init(const GASFunctionRefBase& orig, UByte flags)
{
    Flags = flags;
    Function = orig.Function; 
    if (!(Flags & FuncRef_Weak) && Function)
        Function->AddRef();
    LocalFrame = 0; 
    if (orig.LocalFrame != 0) 
        SetLocalFrame (orig.LocalFrame, orig.Flags & FuncRef_Internal);
}

void GASFunctionRefBase::DropRefs()
{
    if (!(Flags & FuncRef_Weak) && Function)
        Function->Release ();
    Function = 0;
    if (!(Flags & FuncRef_Internal) && LocalFrame != 0)
        LocalFrame->Release ();
    LocalFrame = 0;

}

void GASFunctionRefBase::Assign(const GASFunctionRefBase& orig)
{
    if (this != &orig)
    {
        GASFunctionObject* pprevFunc = Function;
        if (!(Flags & FuncRef_Weak) && Function && (Function != orig.Function))
            Function->Release ();

        Function = orig.Function;

        if (!(Flags & FuncRef_Weak) && Function && (pprevFunc != orig.Function))
            Function->AddRef ();

        if (orig.LocalFrame != 0) 
            SetLocalFrame (orig.LocalFrame, (orig.Flags & FuncRef_Internal));
        else
            SetLocalFrame (0, 0);
    }
}

void GASFunctionRefBase::SetInternal (bool internal)
{ 
    if (LocalFrame != 0 && bool(Flags & FuncRef_Internal) != internal)
    {
        if (!(Flags & FuncRef_Internal))
            LocalFrame->Release ();
        else
            LocalFrame->AddRef ();
    }
    if (internal) Flags |= FuncRef_Internal;
    else          Flags &= (~FuncRef_Internal);
}

void GASFunctionRefBase::SetLocalFrame (GASLocalFrame* localFrame, bool internal)
{
    if (LocalFrame != 0 && !(Flags & FuncRef_Internal))
        LocalFrame->Release ();
    LocalFrame = localFrame;
    if (internal) Flags |= FuncRef_Internal;
    else          Flags &= (~FuncRef_Internal);
    if (LocalFrame != 0 && !(Flags & FuncRef_Internal))
        LocalFrame->AddRef ();
}

void GASFunctionRefBase::operator() (const GASFnCall& fn) const
{
    GASSERT (Function);
    Function->Invoke(fn, LocalFrame);
}

bool GASFunctionRefBase::operator== (const GASFunctionRefBase& f) const
{
    return Function == f.Function;
}

bool GASFunctionRefBase::operator== (const GASFunctionDef& f) const
{
    GASSERT (Function);
    return (*Function) == f;
}

bool GASFunctionRefBase::operator!= (const GASFunctionRefBase& f) const
{
    return Function != f.Function;
}

bool GASFunctionRefBase::operator!= (const GASFunctionDef& f) const
{
    GASSERT (Function);
    return (*Function) != f;
}

bool GASFunctionRefBase::operator== (const GASFunctionRefBase* f) const
{
    if (f == NULL)
        return Function == NULL;
    return Function == f->Function;
}

bool GASFunctionRefBase::operator!= (const GASFunctionRefBase* f) const
{
    if (f == NULL)
        return Function != NULL;
    return Function != f->Function;
}

bool GASFunctionRefBase::IsCFunction () const
{
    GASSERT (Function);
    return Function->IsCFunction ();
}

bool GASFunctionRefBase::IsAsFunction () const
{
    GASSERT (Function);
    return Function->IsAsFunction ();
}


///////////// GASFunctionProto //////////////////
static const GASNameFunction GAS_FunctionObjectTable[] = 
{
    { "apply",               &GASFunctionProto::Apply },
    { "call",                &GASFunctionProto::Call  },
    { 0, 0 }
};

GASFunctionProto::GASFunctionProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor, bool initFuncs) : 
    GASPrototype<GASObject>(psc, pprototype, constructor)
{
    if (initFuncs)
        InitFunctionMembers(psc, GAS_FunctionObjectTable, pprototype);
}

void GASFunctionProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASFunctionObject> obj = *new GASFunctionObject(fn.Env);
    fn.Result->SetAsObject(obj.GetPtr());
}

void GASFunctionProto::ToString(const GASFnCall& fn)
{
    fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_typeFunction_));
}

void GASFunctionProto::ValueOf(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    fn.Result->SetAsObject(static_cast<GASObject*>(fn.ThisPtr));
}

void GASFunctionProto::Apply(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    GPtr<GASObject> objectHolder;
    GPtr<GFxASCharacter> charHolder;
    GASObjectInterface* thisObj = 0;
    int nArgs = 0;

    fn.Result->SetUndefined();
    GPtr<GASArrayObject> arguments;

    if (fn.NArgs >= 1)
    {
        thisObj = fn.Arg(0).ToObjectInterface(fn.Env);
        if (thisObj)
        {
            if (thisObj->IsASCharacter())
                charHolder = thisObj->ToASCharacter();
            else
                objectHolder = static_cast<GASObject*>(thisObj);
        }
    }
    if (fn.NArgs >= 2)
    {
        // arguments array
        GASObject* args = fn.Arg(1).ToObject();
        if (args && args->GetObjectType() == GASObjectInterface::Object_Array)
        {
            arguments = static_cast<GASArrayObject*>(args);
            nArgs = arguments->GetSize();

            // push arguments into the environment's stack
            if (nArgs > 0)
            {
                for (int i = nArgs - 1; i >= 0; --i)
                    fn.Env->Push(*arguments->GetElementPtr(i));
            }
        }
    }
    
    GASValue result;
    // invoke function
    if (!fn.ThisFunctionRef.IsNull())
    {
        //!AB: if ThisFunctionRef is not null then we may do call with using it.
        // In this case localFrame will be correct, and this is critical for calling nested functions.
        fn.ThisFunctionRef(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()));
    }
    else //!AB: is it necessary or ThisFunctionRef should be not null allways?
    {
        GPtr<GASFunctionObject> func = static_cast<GASFunctionObject*>(fn.ThisPtr);
        func->Invoke(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()), 0);
    }
    
    // wipe out arguments
    if (nArgs > 0)
    {
        fn.Env->Drop(nArgs);
    }
    *fn.Result = result;
}

void GASFunctionProto::Call(const GASFnCall& fn)
{
    GASSERT(fn.ThisPtr);

    GPtr<GASObject> objectHolder;
    GPtr<GFxASCharacter> charHolder;
    GASObjectInterface* thisObj = 0;
    int nArgs = 0;

    fn.Result->SetUndefined();
    GPtr<GASArrayObject> arguments;

    if (fn.NArgs >= 1)
    {
        thisObj = fn.Arg(0).ToObjectInterface(fn.Env);
        if (thisObj)
        {
            if (thisObj->IsASCharacter())
                charHolder = thisObj->ToASCharacter();
            else
                objectHolder = static_cast<GASObject*>(thisObj);
        }
    }
    if (fn.NArgs >= 2)
    {
        nArgs = fn.NArgs - 1;
    }

    GASValue result;
    // invoke function
    if (!fn.ThisFunctionRef.IsNull())
    {
        //!AB: if ThisFunctionRef is not null then we may do call with using it.
        // In this case localFrame will be correct, and this is critical for calling nested functions.
        fn.ThisFunctionRef(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()-4));
    }
    else //!AB: is it necessary or ThisFunctionRef should be not null allways?
    {
        GPtr<GASFunctionObject> func = static_cast<GASFunctionObject*>(fn.ThisPtr);
        func->Invoke(GASFnCall(&result, thisObj, fn.Env, nArgs, fn.Env->GetTopIndex()-4), 0);
    }
    *fn.Result = result;
}

/////////////////////////////////////////////////////
GASFunctionCtorFunction::GASFunctionCtorFunction (GASStringContext* psc) :
    GASFunctionObject(GASFunctionProto::GlobalCtor)
{
    GUNUSED(psc);
}
