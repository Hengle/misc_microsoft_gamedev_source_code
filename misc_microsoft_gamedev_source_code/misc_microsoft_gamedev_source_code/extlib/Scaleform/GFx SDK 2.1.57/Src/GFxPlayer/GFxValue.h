/**********************************************************************

Filename    :   GFxValue.h
Content     :   ActionScript Value implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXVALUE_H
#define INC_GFXVALUE_H

#include "GRefCount.h"
#include "GTLTypes.h"
#include "GFxActionTypes.h"
#include "GFxFunction.h"


// ***** Declared Classes
class GASValue;
class GASFnCall;

// ***** External Classes
class GFxCharacterHandle;
class GASEnvironment;

struct GASValueProperty : public GRefCountBase<GASValueProperty>
{
    GASFunctionRef              GetterMethod;
    GASFunctionRef              SetterMethod;

    GASValueProperty(const GASFunctionRef& getterMethod, const GASFunctionRef& setterMethod);
};

// ***** GASValue


#if defined(GFC_CC_MSVC) && defined(GFC_CPU_X86)
// On Win32 ActionScript performs better with smaller structures,
// even if those cause GASNumberValues to be *misalligned*.
#pragma pack(push, 4)
#endif

// This is the main ActionScript value type, used for execution stack
// entry and all computations. It combines numeric, string, object
// and other necessary types.
class GASValue
{
public:

    // Override 'operator new' to use our allocator.
    // We avoid using GNewOverrideBase because in some compilers
    // it causes overhead and small size is critical for GASValue.
    #if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
    # undef new
    #endif
        GFC_MEMORY_REDEFINE_NEW(GASValue)
    #if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
    # define new GFC_DEFINE_NEW
    #endif


    enum type
    {
        UNDEFINED,
        NULLTYPE,
        BOOLEAN,        
        NUMBER,

        INTEGER,    // for internal use only
        // These objects require DropRefs:
        STRING,
        OBJECT,
        CHARACTER,  // 'movieclip' - like type
        FUNCTION,   // C or AS function
        PROPERTY,   // getter/setter property

        UNSET       // special value indicating value is not set
    };


    // *** GASValue Packing/Alignment Notes
    //
    // The arrangement of structures below is necessary to ensure that two
    // alignment conditions ensured in a portable manner:
    //  1) GASNumberValue gets correctly aligned.
    //  2) GASValue does not get padded to 24 bytes in 32-bit machines,
    //     when it can be 16.    
    // The condition (2) means that we can not put NumberValue into
    // the same same union as other values, as that would force such a
    // union to become 16 bytes; forcing GASValue to be 12.


    // Type and property flags of value.
    struct TypeDesc
    {        
        UByte               Type;        
        // Property flags. In general, not initialized or copied; these 
        // are only accessible through GetPropFlags() and SetPropFlags().
        // Property flags are managed explicitly in GASMember.
        UByte               PropFlags;
    };

    struct ValueType : public TypeDesc
    {
        union
        {
            GASStringNode*          pStringNode;
            GASObject*              pObjectValue;
            GFxCharacterHandle*     pCharHandle;
            GASValueProperty*       pProperty;
            GASFunctionRefBase      FunctionValue;
            bool                    BooleanValue;
        };
    };

    struct NumericType : public TypeDesc
    {
#if defined(GFC_CC_MSVC) && defined(GFC_CPU_X86)
        // Avoid packing warning on Win32.
//        UInt32          Dummy;
#endif        
        union
        {
            SInt32          Int32Value;
            UInt32          UInt32Value;
            GASNumberValue  NumberValue;
        };
    };
    
    union
    {
        TypeDesc        T;
        ValueType       V;
        NumericType     NV;
    };



    // *** Constructors.

    GASValue()        
    {
        T.Type = UNDEFINED;
    }    
    GASValue(type typeVal)
    {
        T.Type = (UByte)typeVal;
    }
    GASValue(const GASString& str)        
    {
        T.Type = STRING;
        V.pStringNode = str.GetNode();
        V.pStringNode->AddRef();
    }
    GASValue(bool val)
    { 
        T.Type = BOOLEAN;
        V.BooleanValue = val;
    }
    GASValue(int val)
    {
        T.Type = INTEGER;
        NV.Int32Value = SInt32(val);
        //@DBG
        //T.Type = NUMBER;
        //NV.NumberValue = val;
    }
    GASValue(long val)
    {
        T.Type = INTEGER;
        NV.Int32Value = SInt32(val); //!AB: should it be Int64? 
        //@DBG
        //T.Type = NUMBER;
        //NV.NumberValue = val;
    }   
    // Floating - point constructor.
    GASValue(GASNumber val)
    {
        T.Type = NUMBER;
        NV.NumberValue = val;
    }
#ifndef GFC_NO_DOUBLE
    GASValue(Float val)
    {
        T.Type = NUMBER;
        NV.NumberValue = double(val);
    }
#endif
    
    GASValue(GASObject* obj);
    GASValue(GFxASCharacter* pcharacter);
    GASValue(GASCFunctionPtr func);
    GASValue(const GASFunctionRef& func);
    GASValue(const GASFunctionRef& getter, const GASFunctionRef& setter);

    // Copy constructor - optimized implementation in C++.
    // This is called *very* frequently, so we do *NOT* want
    // to make use of operator =.
    GASValue(const GASValue& v);

    // Destructor: cleanup only necessary for allocated types.
    ~GASValue()
    {
        if (T.Type >= STRING)
            DropRefs();
    }

    // Cleanup: Useful when changing types/values.
    void    DropRefs();
    
    // Assignment.
    void    operator = (const GASValue& v);
    
    type GetType() const        { return (type)T.Type; }
    bool TypesMatch(const GASValue& val) const
    {
        if (GetType() != val.GetType())
        {
            // special case to compare numeric types. INTEGER and NUMBER
            // should be treated as matching types.
            return ((GetType()     == NUMBER || GetType()     == INTEGER) &&
                    (val.GetType() == NUMBER || val.GetType() == INTEGER));
        }
        return true;
    }

    // Property flag value access for GASMember.
    UByte GetPropFlags() const  { return T.PropFlags;  }
    void  SetPropFlags(UByte f) { T.PropFlags = f;  }

    bool IsObject() const       { return T.Type == OBJECT; }
    bool IsFunction() const     { return T.Type == FUNCTION; }
    bool IsUndefined() const    { return T.Type == UNDEFINED; }
    bool IsCharacter() const    { return T.Type == CHARACTER; }
    bool IsString() const       { return T.Type == STRING; }
    bool IsNumber() const       { return T.Type == NUMBER || T.Type == INTEGER; }
    bool IsFloatingNumber() const { return T.Type == NUMBER; }
    bool IsIntegerNumber() const  { return T.Type == INTEGER; }
    bool IsBoolean() const      { return T.Type == BOOLEAN; }
    bool IsNull() const         { return T.Type == NULLTYPE; }
    bool IsSet() const          { return T.Type != UNSET; }
    bool IsProperty() const     { return T.Type == PROPERTY; }
    bool IsPrimitive() const    { return IsString() || IsBoolean() || IsNull() || IsNumber(); }
    
    // precision - >0 - specifies the precision for Number-to-String conversion
    //             <0 - default precision will be used (maximal)
    //             ignored, if conversion is not from Number.
    GASString           ToString(GASEnvironment* penv, int precision = -1) const;
    GASString           ToStringVersioned(GASEnvironment* penv, UInt version) const;
    // Same as above, but will not invoke action script; useful for trace logs.
    GASString           ToDebugString(GASEnvironment* penv) const;
    // Used to implement above.
    GASString           ToStringImpl(GASEnvironment* penv, int precision, bool debug) const;


    GASNumber           ToNumber(GASEnvironment* penv) const;
    SInt32                ToInt32(GASEnvironment* penv) const;
    UInt32                ToUInt32(GASEnvironment* penv) const;

    bool                ToBool(const GASEnvironment* penv) const;
    GASObject*          ToObject(const GASEnvironment* penv = NULL) const;
    GFxASCharacter*     ToASCharacter(const GASEnvironment* penv) const;
    GASObjectInterface* ToObjectInterface(const GASEnvironment* penv) const;
    GASFunctionRef      ToFunction() const;

    enum Hint {
        NoHint, HintToNumber, HintToString
    };
    GASValue            ToPrimitive(GASEnvironment* penv, Hint hint = NoHint) const;

    void    ConvertToNumber(GASEnvironment* penv);
    void    ConvertToString(GASEnvironment* penv);
    void    ConvertToStringVersioned(GASEnvironment* penv, UInt version);

    // These Set*()'s are more type-safe; should be used
    // in preference to generic overloaded Set().  You are
    // more likely to get a warning/error if misused.
    void    SetUndefined()                      { DropRefs(); T.Type = UNDEFINED; }
    void    SetNull()                           { DropRefs(); T.Type = NULLTYPE; }
    void    SetString(const GASString& str)     { if (T.Type >= STRING) DropRefs(); T.Type = STRING; V.pStringNode = str.GetNode(); V.pStringNode->AddRef(); }    
    void    SetNumber(GASNumber val)            { if (T.Type >= STRING) DropRefs(); T.Type = NUMBER; NV.NumberValue = val; }
    void    SetBool(bool val)                   { DropRefs(); T.Type = BOOLEAN; V.BooleanValue = val; }
    void    SetInt(int val)                     { if (T.Type >= STRING) DropRefs(); T.Type = INTEGER; NV.Int32Value = val; }
    void    SetUInt(UInt val)                   { if (T.Type >= STRING) DropRefs(); T.Type = INTEGER; NV.UInt32Value = val; }
    void    SetAsObject(GASObject* pobj);   
    void    SetAsCharacterHandle(GFxCharacterHandle* pchar);
    void    SetAsCharacter(GFxASCharacter* pchar);
    void    SetAsObjectInterface(GASObjectInterface* pobj);
    void    SetAsFunction(const GASFunctionRefBase& func);
    void    SetUnset()                          { DropRefs(); T.Type = UNSET; }  

    // return true, if this equals to v
    bool    IsEqual (GASEnvironment* penv, const GASValue& v) const;
    // compares this and v. 
    // action: 0 - equal, <0 - less, >0 - greater
    // return Boolean true or false, if comparison satisfies\not satisfies to the action. Otherwise, undefined.
    GASValue  Compare (GASEnvironment* penv, const GASValue& v, int action) const;
    void    Add (GASEnvironment* penv, const GASValue& v);
    void    Sub (GASEnvironment* penv, const GASValue& v)   { SetNumber(ToNumber(penv) - v.ToNumber(penv)); }
    void    Mul (GASEnvironment* penv, const GASValue& v)   { SetNumber(ToNumber(penv) * v.ToNumber(penv)); }
    void    Div (GASEnvironment* penv, const GASValue& v)   { SetNumber(ToNumber(penv) / v.ToNumber(penv)); }   // @@ check for div/0
    void    And (GASEnvironment* penv, const GASValue& v)   { SetInt(ToInt32(penv) & v.ToInt32(penv)); }
    void    Or  (GASEnvironment* penv, const GASValue& v)   { SetInt(ToInt32(penv) | v.ToInt32(penv)); }
    void    Xor (GASEnvironment* penv, const GASValue& v)   { SetInt(ToInt32(penv) ^ v.ToInt32(penv)); }
    void    Shl (GASEnvironment* penv, const GASValue& v)   { SetInt(ToInt32(penv) << v.ToInt32(penv)); }
    void    Asr (GASEnvironment* penv, const GASValue& v)   { SetInt(ToInt32(penv) >> v.ToInt32(penv)); }
    void    Lsr (GASEnvironment* penv, const GASValue& v)   { SetInt((ToUInt32(penv) >> v.ToInt32(penv))); }

    void    StringConcat(GASEnvironment* penv, const GASString& str);

    bool    GetPropertyValue(GASEnvironment* penv, GASObjectInterface* pthis, GASValue* value) const;
    void    SetPropertyValue(GASEnvironment* penv, GASObjectInterface* pthis, const GASValue& val); 

    const GASString& GetCharacterNamePath(GASEnvironment* penv) const; 
};

#if defined(GFC_CC_MSVC) && defined(GFC_CPU_X86)
#pragma pack(pop)
#endif


// Parameters/environment for C functions callable from ActionScript.
class GASFnCall : public GFxLogBase<GASFnCall>
{
public:
    GASValue*           Result;
    GASObjectInterface* ThisPtr;
    GASFunctionRef      ThisFunctionRef; // if this is represented as function
    GASEnvironment*     Env;
    int                 NArgs;
    int                 FirstArgBottomIndex;

    GASFnCall(GASValue* ResIn, GASObjectInterface* ThisIn, GASEnvironment* EnvIn, int NargsIn, int FirstIn)
        :
        Result(ResIn),
        ThisPtr(ThisIn),
        Env(EnvIn),
        NArgs(NargsIn),
        FirstArgBottomIndex(FirstIn)
    {
    }

    GASFnCall(GASValue* ResIn, const GASValue& ThisIn, GASEnvironment* EnvIn, int NargsIn, int FirstIn)
        :
        Result(ResIn),
        ThisPtr(ThisIn.ToObjectInterface(EnvIn)),
        Env(EnvIn),
        NArgs(NargsIn),
        FirstArgBottomIndex(FirstIn)
    {
        GASSERT(!ThisIn.IsPrimitive());
        if (ThisIn.IsFunction())
            ThisFunctionRef = ThisIn.ToFunction();
    }

    virtual ~GASFnCall () { }

    // Access a particular argument.
    GASValue& Arg(int n) const;

    // Logging script support
    GFxLog*         GetLog() const;
    bool            IsVerboseAction() const;
    bool            IsVerboseActionErrors() const;

    bool            CheckThisPtr
        (UInt type, const char* className, const char* psrcfile, int line) const;
};

#ifdef GFC_BUILD_DEBUG
#define CHECK_THIS_PTR(fn,classname,classnamestr)\
    do {\
    if (!fn.CheckThisPtr(GASObjectInterface::Object_##classname, classnamestr, __FILE__, __LINE__)) \
    return; \
    } while(0)
#else
#define CHECK_THIS_PTR(fn,classname,classnamestr)\
    do {\
    if (!fn.CheckThisPtr(GASObjectInterface::Object_##classname, classnamestr, NULL, 0)) \
    return; \
    } while(0)
#endif

#endif //INC_GFXVALUE_H
