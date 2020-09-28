/**********************************************************************

Filename    :   GFxFunction.h
Content     :   ActionScript implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXFUNCTION_H
#define INC_GFXFUNCTION_H

#include "GFxActionTypes.h"

// ***** Declared Classes
class GASFunctionDef;
class GASCFunctionDef;
class GASAsFunctionDef;
class GASFunctionRefBase;
class GASFunctionRef;
class GASFunctionWeakRef;

// ***** External Classes
class GASFnCall;
class GASEnvironment;
class GASActionBuffer;
class GASLocalFrame;
class GASFunctionObject;


// ***** GASFunction

// Common function interface.

class GASFunctionDef : public GNewOverrideBase
{
public:
    virtual ~GASFunctionDef () {}

    virtual void Invoke(const GASFnCall& fn, GASLocalFrame*, GASEnvironment*) =0;
    virtual bool operator== (const GASFunctionDef& f) const =0;
    virtual bool operator!= (const GASFunctionDef& f) const =0;
    virtual GASEnvironment* GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh) = 0;

    virtual bool IsNull () const { return true; } 
    virtual bool IsCFunction () const { return false; }
    virtual bool IsAsFunction () const { return false; }
};

class GASCFunctionDef : public GASFunctionDef
{
protected:
    GASCFunctionPtr pFunction;
public:
    GASCFunctionDef (GASObjectInterface* prototype, GASCFunctionPtr func);
    GASCFunctionDef (GASCFunctionPtr func);

    void Invoke(const GASFnCall& fn, GASLocalFrame*, GASEnvironment*);
    bool operator== (const GASFunctionDef& f) const;
    bool operator!= (const GASFunctionDef& f) const;
    GASEnvironment* GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh);

    virtual bool IsCFunction () const { return true; }
    virtual bool IsNull () const { return pFunction == 0; }
};

// ActionScript function.
class GASAsFunctionDef : public GASFunctionDef
{
    GPtr<GASActionBuffer>       pActionBuffer;
    GTL::garray<GASWithStackEntry>  WithStack;      // initial with-stack on function entry.
    int                         StartPc;
    int                         Length;
    void*                       pContextData;

    struct ArgSpec
    {
        int         Register;
        GASString   Name;

        ArgSpec(int r, const GASString &n)
            : Register(r), Name(n) { }
        ArgSpec(const ArgSpec& s)
            : Register(s.Register), Name(s.Name) { }
    };

    UByte                   ExecType;   // GASActionBuffer::ExecuteType - Function2, Event, etc.    
    UByte                   LocalRegisterCount;
    UInt16                  Function2Flags; // used by function2 to control implicit arg register assignments
    GTL::garray_cc<ArgSpec> Args;
public:

    // NULL environment is allowed -- if so, then
    // functions will be executed in the caller's
    // environment, rather than the environment where they
    // were defined.
    GASAsFunctionDef(GASActionBuffer* ab, GASEnvironment* env, int start, 
                     const GTL::garray<GASWithStackEntry>& WithStack,
                     GASActionBuffer::ExecuteType execType = GASActionBuffer::Exec_Function);
    ~GASAsFunctionDef();

    void    SetExecType(GASActionBuffer::ExecuteType execType)      { ExecType = (UByte)execType; }
    GASActionBuffer::ExecuteType GetExecType() const                { return (GASActionBuffer::ExecuteType)ExecType; }

    bool    IsFunction2() const { return ExecType == GASActionBuffer::Exec_Function2; }
    
    void    SetLocalRegisterCount(UByte ct) { GASSERT(IsFunction2()); LocalRegisterCount = ct; }
    void    SetFunction2Flags(UInt16 flags) { GASSERT(IsFunction2()); Function2Flags = flags; }

    class GFxASCharacter* GetTargetCharacter();

    const ArgSpec&  AddArg(int argRegister, const GASString& name)
    {
        GASSERT(argRegister == 0 || IsFunction2() == true);
        Args.resize(Args.size() + 1);
        ArgSpec& arg = Args.back();
        arg.Register = argRegister;
        arg.Name = name;
        return arg;
    }

    void    SetLength(int len) { GASSERT(len >= 0); Length = len; }

    // Dispatch.
    void Invoke(const GASFnCall& fn, GASLocalFrame*, GASEnvironment*);

    virtual bool IsAsFunction () const { return true; }
    virtual bool IsNull () const { return false; }

    bool operator== (const GASFunctionDef& f) const;
    bool operator!= (const GASFunctionDef& f) const;
    GASEnvironment* GetEnvironment(const GASFnCall& fn, GPtr<GFxASCharacter>* ptargetCh);

    UInt GetNumArgs() const { return (UInt)Args.size(); }
};

 
// DO NOT use GASFunctionRefBase anywhere but as a base class for 
// GASFunctionRef and GASFunctionWeakRef. DO NOT add virtual methods,
// since GASFunctionRef and GASFunctionWeakRef should be very simple 
// and fast. DO NOT add ctor/dtor and assignment operators to GASFunctionRefBase
// since it is used in union.
class GASFunctionRefBase
{
protected:
    GASFunctionObject*      Function;
    GASLocalFrame*          LocalFrame; 
    enum FuncRef_Flags
    {
        FuncRef_Internal = 1,
        FuncRef_Weak     = 2
    };
    UByte                   Flags;
public:
    void Init (GASFunctionObject* funcObj = 0, UByte flags = 0);
    void Init (GASFunctionObject& funcObj, UByte flags = 0);
    void Init (const GASFunctionRefBase& orig, UByte flags = 0);
    void DropRefs();

    void SetLocalFrame (GASLocalFrame* localFrame, bool internal = false);

    bool IsInternal () const { return (Flags & FuncRef_Internal); }
    void SetInternal (bool internal = false);

    void operator() (const GASFnCall& fn) const;
    bool operator== (const GASFunctionRefBase& f) const;
    bool operator== (const GASFunctionRefBase* f) const;
    bool operator== (const GASFunctionDef& f) const;
    bool operator!= (const GASFunctionRefBase& f) const;
    bool operator!= (const GASFunctionRefBase* f) const;
    bool operator!= (const GASFunctionDef& f) const;

    bool IsCFunction () const;
    bool IsAsFunction () const;
    bool IsNull () const { return Function == 0; }

    void Assign(const GASFunctionRefBase& orig);

    GINLINE GASFunctionObject*           operator-> () { return Function; }
    GINLINE GASFunctionObject*           operator-> () const { return Function; }
    GINLINE GASFunctionObject&           operator* () { return *Function; }
    GINLINE GASFunctionObject&           operator* () const { return *Function; }
    GINLINE GASFunctionObject*           GetObjectPtr () { return Function; }
    GINLINE GASFunctionObject*           GetObjectPtr () const { return Function; }
};

class GASFunctionRef : public GASFunctionRefBase
{
public:
    inline GASFunctionRef(GASFunctionObject* funcObj = 0)  { Init(funcObj);  }
    inline GASFunctionRef(GASFunctionObject& funcObj)      { Init(funcObj); }
    inline GASFunctionRef(const GASFunctionRefBase& orig)
    {
        Init(orig);
    }
    inline GASFunctionRef(const GASFunctionRef& orig)
    {
        Init(orig);
    }
    inline ~GASFunctionRef()
    {
        DropRefs();
    }

    inline const GASFunctionRefBase& operator = (const GASFunctionRefBase& orig)
    {
        Assign(orig);
        return *this;
    }
    inline const GASFunctionRef& operator = (const GASFunctionRef& orig)
    {
        Assign(orig);
        return *this;
    }
};

// used for "constructor" in prototypes, they should have 
// weak reference to the function, because constructor itself
// contains strong reference to its prototype.
// Convertible to and from regular GASFunctionRef
class GASFunctionWeakRef : public GASFunctionRefBase
{
public:
    GASFunctionWeakRef(GASFunctionObject* funcObj = 0) 
    { 
        Init(funcObj, FuncRef_Weak); 
        SetInternal(true);
    }
    GASFunctionWeakRef(const GASFunctionRefBase& orig)
    {
        Init(orig, FuncRef_Weak);
        SetInternal(true);
    }
    GASFunctionWeakRef(const GASFunctionWeakRef& orig)
    {
        Init(orig, FuncRef_Weak);
        SetInternal(true);
    }
    inline ~GASFunctionWeakRef()
    {
        DropRefs();
    }
    inline const GASFunctionRefBase& operator = (const GASFunctionRefBase& orig)
    {
        Assign(orig);
        return *this;
    }
    inline const GASFunctionWeakRef& operator = (const GASFunctionWeakRef& orig)
    {
        Assign(orig);
        return *this;
    }
};


#endif // INC_GFXFUNCTION_H

