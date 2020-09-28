/**********************************************************************

Filename    :   GFxObject.h
Content     :   ActionScript Object implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXOBJECT_H
#define INC_GFXOBJECT_H

#include "GRefCount.h"
#include "GTLTypes.h"
#include "GFxASString.h"
#include "GFxPlayer.h"
#include "GFxActionTypes.h"
#include "GFxStringBuiltins.h"
#include "GFxFunction.h"
#include "GFxValue.h"


// ***** Declared Classes
class GASPropFlags;
class GASMember;
class GASObject;
class GASObjectInterface;
class GASObjectProto;
class GASObjectCtorFunction;
class GASFunctionObject;
class GASFunctionProto;
class GASFunctionCtorFunction;
class GASCustomFunctionObject;
class GASSuperObject;

// Parameterized so GASEnvironment can be used before definition
template <typename BaseClass, typename GASEnvironment = GASEnvironment> 
class GASPrototype;

// ***** External Classes
class GASEnvironment;
class GFxSprite;



// ***** GASPropFlags

// Flags defining the level of protection of an ActionScript member (GASMember).
// Members can be marked as readOnly, non-enumeratebale, etc.

class GASPropFlags
{
public:
    // Numeric flags
    UByte        Flags;
    // Flag bit values
    enum PropFlagConstants
    {
        PropFlag_ReadOnly   =   0x04,
        PropFlag_DontDelete =   0x02,
        PropFlag_DontEnum   =   0x01,
        PropFlag_Mask       =   0x07
    };
    
    // Constructors
    GASPropFlags()
    {
        Flags = 0;        
    }
    
    // Constructor, from numerical value
    GASPropFlags(UByte flags)    
    {
        Flags = flags;
    }
            
    GASPropFlags(bool readOnly, bool dontDelete, bool dontEnum)         
    {
        Flags = (UByte) ( ((readOnly) ?   PropFlag_ReadOnly : 0) |
                          ((dontDelete) ? PropFlag_DontDelete : 0) |
                          ((dontEnum) ?   PropFlag_DontEnum : 0) );
    }
        
    // Accessors
    bool    GetReadOnly() const     { return ((Flags & PropFlag_ReadOnly)   ? true : false); }
    bool    GetDontDelete() const   { return ((Flags & PropFlag_DontDelete) ? true : false); }
    bool    GetDontEnum() const     { return ((Flags & PropFlag_DontEnum)   ? true : false); }
    UByte   GetFlags() const        { return Flags; }       
    
    // set the numerical flags value (return the new value )
    // If unlocked is false, you cannot un-protect from over-write,
    // you cannot un-protect from deletion and you cannot
    // un-hide from the for..in loop construct
    UByte SetFlags(UByte setTrue, UByte setFalse)
    {
        Flags = Flags & (~setFalse);
        Flags |= setTrue;
        return GetFlags();
    }
    UByte SetFlags(UByte flags)
    {        
        Flags = flags;
        return GetFlags();
    }
};
 

// ActionScript member: combines value and its properties. Used in GASObject,
// and potentially other types.
class GASMember
{    
public:
    GASValue        Value;
    
    // Constructors
    GASMember()
    { Value.SetPropFlags(0); }
    
    GASMember(const GASValue &value,const GASPropFlags flags = GASPropFlags())
        : Value(value)
    { Value.SetPropFlags(flags.GetFlags()); }

    GASMember(const GASMember &src)
        : Value(src.Value)
    {
        // Copy PropFlags explicitly since they are not copied as part of value.
        Value.SetPropFlags(src.Value.GetPropFlags());
    }

    void operator = (const GASMember &src)     
    {
        // Copy PropFlags explicitly since they are not copied as part of value.
        Value = src.Value;
        Value.SetPropFlags(src.Value.GetPropFlags());
    }
    
    // Accessors
    const GASValue& GetMemberValue() const                      { return Value; }
    GASPropFlags    GetMemberFlags() const                      { return GASPropFlags(Value.GetPropFlags()); }
    void            SetMemberValue(const GASValue &value)       { Value = value; }
    void            SetMemberFlags(const GASPropFlags &flags)   { Value.SetPropFlags(flags.GetFlags()); }
};




// This is the base class for all ActionScript-able objects ("GAS_" stands for ActionScript).   
class GASObjectInterface
{
public:
    GASObjectInterface();
    virtual ~GASObjectInterface();

    // Specific class types.
    enum ObjectType
    {
        Object_Unknown,

        // This type is for non-scriptable characters; it is not GASObjectInterface.
        Object_BaseCharacter,
        
        // These need to be grouped in range for IsASCharacter().
        Object_Sprite,
        Object_ASCharacter_Begin    = Object_Sprite,
        Object_Button,
        Object_TextField,
        Object_ASCharacter_End      = Object_TextField,

        Object_ASObject,
        Object_ASObject_Begin       = Object_ASObject,
        Object_Array,
        Object_String,
        Object_Number,
        Object_Boolean,
        Object_MovieClipObject,
        Object_ButtonASObject,
        Object_TextFieldASObject,
        Object_Matrix,
        Object_Point,
        Object_Rectangle,
        Object_ColorTransform,
        Object_Capabilities,
        Object_Transform,
        Object_Color,
        Object_Key,
        Object_Function,
        Object_Stage,
        Object_MovieClipLoader,
        Object_BitmapData,
        Object_LoadVars,
        Object_XML,
        Object_XMLNode,
        Object_TextFormat,
        Object_Date,
        Object_ASObject_End        = Object_TextFormat
    };

    // Returns text representation of the object.
    // - penv parameter is optional and may be not required for most of the types.
    virtual const char*         GetTextValue(GASEnvironment* penv = 0) const    { GUNUSED(penv); return 0; }

    virtual ObjectType          GetObjectType() const                           { return Object_Unknown; }

    struct MemberVisitor {
        virtual ~MemberVisitor () { }
        virtual void    Visit(const GASString& name, const GASValue& val, UByte flags) = 0;
    };

    enum VisitMemberFlags
    {
        VisitMember_Prototype   = 0x01, // Visit prototypes.
        VisitMember_ChildClips  = 0x02, // Visit child clips in sprites as members.
        VisitMember_DontEnum    = 0x04  // Visit members marked as not enumerable.
    };
    
    virtual bool            SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags()) = 0;
    virtual bool            GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)                       = 0;
    virtual bool            FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)                = 0;
    virtual bool            DeleteMember(GASStringContext *psc, const GASString& name)                                  = 0;
    virtual bool            SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)              = 0;
    virtual void            VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags = 0) const     = 0;
    virtual bool            HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)                = 0;

    virtual bool            SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val,
                                         const GASPropFlags& flags = GASPropFlags())                                    = 0;
    virtual bool            GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)                   = 0;

    // Helper for string names: registers string automatically.
    // Should not be used for built-ins.
    bool    SetMemberRaw(GASStringContext *psc, const char *pname, const GASValue& val)
    {
        return SetMemberRaw(psc, GASString(psc->CreateString(pname)), val);        
    }
    bool    SetConstMemberRaw(GASStringContext *psc, const char *pname, const GASValue& val)
    {
        return SetMemberRaw(psc, GASString(psc->CreateConstString(pname)), val);
    }
    bool    SetConstMemberRaw(GASStringContext *psc, const char *pname, const GASValue& val, const GASPropFlags& flags)
    {
        return SetMemberRaw(psc, GASString(psc->CreateConstString(pname)), val, flags);
    }
    bool    GetConstMemberRaw(GASStringContext *psc, const char *pname, GASValue* val)
    {
        return GetMemberRaw(psc, GASString(psc->CreateConstString(pname)), val);
    }

    // Convenience commonly used RTTI inlines.
    GINLINE static bool     IsTypeASCharacter(ObjectType t) { return (t>=Object_ASCharacter_Begin) && (t<=Object_ASCharacter_End);  }
    GINLINE static bool     IsTypeASObject(ObjectType t) { return (t>=Object_ASObject_Begin) && (t<=Object_ASObject_End);  }

    GINLINE bool            IsASCharacter() const   { return IsTypeASCharacter(GetObjectType());  }
    GINLINE bool            IsASObject() const      { return IsTypeASObject(GetObjectType());  }
    GINLINE bool            IsSprite() const        { return GetObjectType() == Object_Sprite; }
    GINLINE bool            IsFunction() const      { return GetObjectType() == Object_Function; }

    // Convenience casts; in cpp file because these require GFxSprite/GFxASCharacter definitions.
    GFxASCharacter*         ToASCharacter();
    GASObject*              ToASObject();
    GFxSprite*              ToSprite();
    virtual GASFunctionRef  ToFunction();

    const GFxASCharacter*   ToASCharacter() const;
    const GASObject*        ToASObject() const;
    const GFxSprite*        ToSprite() const;

    // sets __proto__ property 
    virtual void            Set__proto__(GASStringContext *psc, GASObject* protoObj) = 0;

    // gets __proto__ property
    inline GASObject*       Get__proto__() { return pProto; }


    // sets __constructor__ property
    void                    Set__constructor__(GASStringContext *psc, const GASFunctionRef& ctorFunc) 
    {   
        SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin___constructor__), GASValue(ctorFunc),
                     GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }

    // gets __constructor__ property
    // virtual because it's overridden in GASSuperObject
    virtual GASFunctionRef  Get__constructor__(GASStringContext *psc)
    { 
        GASValue val;
        if (GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin___constructor__), &val))
            return val.ToFunction();
        return 0; 
    }

    // sets "constructor" property
    void                    Set_constructor(GASStringContext *psc, const GASFunctionRef& ctorFunc) 
    {
        SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_constructor), GASValue(ctorFunc),
                     GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
    // gets "constructor" property
    GASFunctionRef          Get_constructor(GASStringContext *psc)
    { 
        GASValue val;
        if (GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_constructor), &val))
            return val.ToFunction();
        return 0; 
    }
    
    // is it super-class object
    virtual bool            IsSuper() const { return false; }

    // adds interface to "implements-of" list. If ctor is null, then index
    // specifies the total number of interfaces to pre-allocate the array.
    virtual void AddInterface(GASStringContext *psc, int index, GASFunctionObject* ctor) { GUNUSED3(psc, index, ctor); } // implemented for prototypes only

    virtual bool InstanceOf(GASEnvironment*, const GASObject* prototype, bool inclInterfaces = true) const { GUNUSED2(prototype, inclInterfaces); return false; }

    virtual bool Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData) { GUNUSED4(psc, prop,callback,userData); return false; }
    virtual bool Unwatch(GASStringContext *psc, const GASString& prop)  { GUNUSED2(psc, prop); return false; }

    GASObject*   FindOwner(GASStringContext *psc, const GASString& name);

protected:
    GPtr<GASObject>     pProto;          // __proto__
};




// ***** GASObject

// A generic bag of attributes.  Base-class for ActionScript
// script-defined objects.

class GASObject : public GRefCountBase<GASObject>, public GASObjectInterface
{
    void Init();
public:

    struct Watchpoint
    {
        GASFunctionRef  Callback;
        GASValue        UserData;
    };

    // Hash table used for ActionScript members.
    typedef GASStringHash<GASMember>    MemberHash;
    typedef GASStringHash<Watchpoint>   WatchpointHash;

    MemberHash          Members;
    GASFunctionRef      ResolveHandler;  // __resolve
    
    WatchpointHash*     pWatchpoints;

    bool                ArePropertiesSet; // indicates if any property (Object.addProperty) was set
    bool                IsListenerSet; // See comments in GFxValue.cpp, CheckForListenersMemLeak. 


    GASObject(GASStringContext *psc = 0); // Can be 0 as long as it is not used.
    GASObject(GASStringContext *psc, GASObject* proto);
    GASObject(GASEnvironment* pEnv);

    virtual ~GASObject()
    { 
        if (pWatchpoints)
            delete pWatchpoints;
    }
    
    virtual const char* GetTextValue(GASEnvironment* penv = 0) const { GUNUSED(penv); return NULL; }

    virtual bool    SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);
    virtual bool    FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember);
    virtual bool    DeleteMember(GASStringContext *psc, const GASString& name);
    virtual bool    SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags);
    virtual void    VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags) const;
    virtual bool    HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes);

    virtual bool    SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);

    virtual ObjectType  GetObjectType() const
    {
        return Object_ASObject;
    }

    void    Clear()
    {
        Members.clear();
        pProto = 0;             
    }

    // sets __proto__ property 
    virtual void            Set__proto__(GASStringContext *psc, GASObject* protoObj) 
    {   
        if (!pProto)
            SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin___proto__), GASValue(GASValue::UNSET),
                         GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
        pProto = protoObj;
    }
    // gets __proto__ property
    /*virtual GASObject*      Get__proto__()
    { 
        return pProto; 
    }*/

    virtual void        SetValue(GASEnvironment* penv, const GASValue&);
    virtual GASValue    GetValue() const;

    virtual bool        InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces = true) const;
    virtual bool        DoesImplement(GASEnvironment*, const GASObject* prototype) const { return this == prototype; }

    virtual bool        Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData);
    virtual bool        Unwatch(GASStringContext *psc, const GASString& prop);
    bool                InvokeWatchpoint(GASEnvironment* penv, const GASString& prop, const GASValue& newVal, GASValue* resultVal);

    // these method will return an original GFxASCharacter for GASObjects from GFxSprite, GFxButton,
    // GFxText.
    virtual GFxASCharacter*         GetASCharacter() { return 0; }
    virtual const GFxASCharacter*   GetASCharacter() const { return 0; }
};

// Function Object
class GASFunctionObject : public GASObject 
{
    friend class GASFunctionProto;
    friend class GASFunctionCtorFunction;
    friend class GASCustomFunctionObject;
protected:
    GASFunctionDef*     Def;

    GASFunctionObject(GASStringContext* psc = 0);
    GASFunctionObject(GASEnvironment* penv);
public:
    GASFunctionObject(GASEnvironment* penv, GASFunctionDef* func);
    GASFunctionObject(GASStringContext* psc, GASObject* pprototype, GASCFunctionPtr func);
    GASFunctionObject(GASCFunctionPtr func);
    GASFunctionObject(GASFunctionDef* func);
    ~GASFunctionObject();

    virtual ObjectType      GetObjectType() const   { return Object_Function; }

    //virtual GASObject*      Get__proto__();

    virtual void Invoke (const GASFnCall& fn, GASLocalFrame*) const;
    virtual bool operator== (const GASFunctionObject& f) const;
    virtual bool operator!= (const GASFunctionObject& f) const;

    virtual bool operator== (const GASFunctionDef& f) const;
    virtual bool operator!= (const GASFunctionDef& f) const;

    virtual bool IsCFunction() const;
    virtual bool IsAsFunction() const;

    /* This method is used for creation of object by AS "new" (GASEnvironment::OperatorNew)
       If this method returns NULL, then constructor function is responsible for
       object's creation. */
    virtual GASObject* CreateNewObject(GASStringContext*) const { return 0; }

    void SetProtoAndCtor(GASStringContext* psc, GASObject* pprototype);

    virtual GASFunctionRef  ToFunction();

    // returns number of arguments expected by this function;
    // returns -1 if number of arguments is unknown (for C functions)
    int     GetNumArgs() const;
};

// A constructor function object for Object
class GASObjectCtorFunction : public GASFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void RegisterClass (const GASFnCall& fn);
public:
    GASObjectCtorFunction(GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASStringContext*) const { return 0; }
};

// A constructor function object for class "Function"
class GASFunctionCtorFunction : public GASFunctionObject
{
public:
    GASFunctionCtorFunction(GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASStringContext*) const { return new GASFunctionObject(); }
};

// A function object for custom user-defined ActionScript's functions
class GASCustomFunctionObject : public GASFunctionObject
{
public:
    GASCustomFunctionObject(GASEnvironment* penv, GASFunctionDef* func) :
      GASFunctionObject(penv, func) {}

    //virtual GASObject* CreateNewObject(GASStringContext*) const { return new GASFunctionObject(); }
    // "new UserFunc()" creates GASObject
    virtual GASObject* CreateNewObject(GASStringContext*) const { return new GASObject(); }
};

#endif //INC_GFXOBJECT_H
