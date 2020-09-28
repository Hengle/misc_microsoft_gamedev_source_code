/**********************************************************************

Filename    :   GFxObjectProto.h
Content     :   ActionScript Object prototype implementation classes
Created     :   
Authors     :   Artem Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXOBJECTPROTO_H
#define INC_GFXOBJECTPROTO_H

#include "GFxObject.h"


template <typename BaseClass, typename GASEnvironment> 
class GASPrototype : public BaseClass
{
protected:
    GASFunctionWeakRef Constructor;     // Weak reference, constructor has strong reference
                                        // to this prototype instance
    GASFunctionWeakRef __Constructor__; // Weak reference, __constructor__ has strong reference
                                        // to this prototype instance
    GTL::garray<GWeakPtr<GASObject> >*  pInterfaces; // array of implemented interfaces

    void InitFunctionMembers(GASStringContext *psc, const GASNameFunction* funcTable)
    {
        for (int i = 0; funcTable[i].Name; i++)
        {
            BaseClass::SetMemberRaw(psc, psc->CreateConstString(funcTable[i].Name),
                                    funcTable[i].Function, GASPropFlags::PropFlag_DontEnum);
        }
    }
    /* pprototype should be Function.prototype! */
    void InitFunctionMembers(GASStringContext *psc, const GASNameFunction* funcTable, GASObject* pprototype)
    {
        for(int i = 0; funcTable[i].Name; i++)
        {
            BaseClass::SetMemberRaw(psc, psc->CreateConstString(funcTable[i].Name), 
                                GASFunctionRef (*new GASFunctionObject(psc, pprototype, funcTable[i].Function)),
                                GASPropFlags::PropFlag_DontEnum);
        }
    }

public:

    // MA: We pass psc to base class in case it may need to have member variables of GASString class,
    // as in GASStringObject. Forces GASStringContext constructors on bases. Any way to avoid this?
    GASPrototype(GASStringContext *psc, const GASFunctionRef& constructor)
        : BaseClass(psc), pInterfaces(0)
    {
        SetConstructor(psc, constructor); 
        Constructor->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), GASValue(this),
                                  GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete);
    }

    GASPrototype(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor)
        : BaseClass(psc), pInterfaces(0)
    {
        SetConstructor(psc, constructor); 
        Constructor->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), GASValue(this),
                                  GASPropFlags::PropFlag_DontEnum | GASPropFlags::PropFlag_DontDelete);

        // set __proto__ for this prototype to Object.prototype
        BaseClass::Set__proto__(psc, pprototype);
    }

    GASPrototype(GASStringContext *psc, GASObject* pprototype)
        : BaseClass(psc), pInterfaces(0)
    {
        // set __proto__ for this prototype to Object.prototype
        BaseClass::Set__proto__(psc, pprototype);
    }

    ~GASPrototype()
    {
        if (pInterfaces)
            delete pInterfaces;
    }

    GASFunctionRef  GetConstructor() { return Constructor; }
    bool            SetConstructor(GASStringContext *psc, const GASValue& ctor) 
    { 
        Constructor = ctor.ToFunction();
        BaseClass::SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_constructor), GASValue(GASValue::UNSET),
                                GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
        return true; 
    }

    inline bool     GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
    {
        GUNUSED(penv);
        return GetMemberRaw(penv->GetSC(), name, val);
    }

    bool            GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
    {
        bool isConstructor2 = psc->GetBuiltin(GASBuiltin___constructor__).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive());
        
        // if (name == "constructor" || name == "__constructor__")
        if ( isConstructor2 ||
             psc->GetBuiltin(GASBuiltin_constructor).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()) )
        {
            GASMember m;
            GASValue lval(GASValue::UNSET);

            // if "constructor" is set as a member - return it.
            if (GASObject::FindMember(psc, name, &m))
            {
                lval = m.GetMemberValue();
            }
            if (lval.IsSet())
                *val = lval;
            else
            {
                GASFunctionRef ctor = GetConstructor ();
                if (isConstructor2)
                    ctor = __Constructor__;
                else
                    ctor = GetConstructor ();
                if (ctor.IsNull())
                {
                    if (BaseClass::pProto)
                        return BaseClass::pProto->GetMemberRaw(psc, name, val);
                }
                val->SetAsFunction(ctor);
            }
            return true;
        }
        return BaseClass::GetMemberRaw(psc, name, val);
    }

    void AddInterface(GASStringContext *psc, int index, GASFunctionObject* pinterface)
    {
        if (pInterfaces == 0 && pinterface == 0)
        {
            pInterfaces = new GTL::garray<GWeakPtr<GASObject> >(index);
            return;
        }
        GASSERT(pinterface);
        GASValue prototypeVal;
        if (pinterface->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), &prototypeVal))
        {
            (*pInterfaces)[index] = GPtr<GASObject>(prototypeVal.ToObject());
        }
    }

    bool DoesImplement(GASEnvironment* penv, const GASObject* prototype) const 
    { 
        if (BaseClass::DoesImplement(penv, prototype)) return true; 
        if (pInterfaces)
        {
            for(UInt i = 0, n = UInt(pInterfaces->size()); i < n; ++i)
            {
                GPtr<GASObject> intf = (*pInterfaces)[i];
                if (intf && intf->InstanceOf(penv, prototype))
                    return true;
            }
        }
        return false;
    }
};

class GASObjectProto : public GASPrototype<GASObject>
{
public:
    GASObjectProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);
    GASObjectProto(GASStringContext *psc, GASObject* pprototype);
    GASObjectProto(GASStringContext *psc, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
    static void AddProperty(const GASFnCall& fn);
    static void HasOwnProperty(const GASFnCall& fn);
    static void IsPropertyEnumerable(const GASFnCall& fn);
    static void IsPrototypeOf(const GASFnCall& fn);
    static void Watch(const GASFnCall& fn);
    static void Unwatch(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);
    static void ValueOf(const GASFnCall& fn);
};

// Function Prototype
class GASFunctionProto : public GASPrototype<GASObject> 
{
public:
    GASFunctionProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor, bool initFuncs = true);

    static void GlobalCtor(const GASFnCall& fn);
    static void Apply(const GASFnCall& fn);
    static void Call(const GASFnCall& fn);
    static void ToString(const GASFnCall& fn);
    static void ValueOf(const GASFnCall& fn);
};

// A special implementation for "super" operator
class GASSuperObject : public GASObject
{
    GPtr<GASObject>     SuperProto;
    GPtr<GASObject>     SavedProto;
    GASObjectInterface* RealThis;
    GASFunctionRef      Constructor;
public:
    GASSuperObject(GASObject* superProto, GASObjectInterface* _this, const GASFunctionRef& ctor) :
        SuperProto(superProto), RealThis(_this), Constructor(ctor) { pProto = superProto; }

    virtual bool    SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        return SuperProto->SetMember(penv, name, val, flags);
    }
    virtual bool    SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {
        return SuperProto->SetMemberRaw(psc, name, val, flags);
    }
    virtual bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
    {
        return SuperProto->GetMember(penv, name, val);
    }
    virtual bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
    {
        return SuperProto->GetMemberRaw(psc, name, val);
    }
    virtual bool    FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)
    {
        return SuperProto->FindMember(psc, name, pmember);
    }
    virtual bool    DeleteMember(GASStringContext *psc, const GASString& name)
    {
        return SuperProto->DeleteMember(psc, name);
    }
    virtual bool    SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)
    {
        return SuperProto->SetMemberFlags(psc, name, flags);
    }
    virtual void    VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags) const
    {
        SuperProto->VisitMembers(psc, pvisitor, visitFlags);
    }
    virtual bool    HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)
    {
        return SuperProto->HasMember(psc, name, inclPrototypes);
    }

    GASObjectInterface* GetRealThis() { return RealThis; }

    /* is it super-class object */
    virtual bool            IsSuper() const { return true; }

    /*virtual GASObject*      Get__proto__()
    { 
        return SuperProto;
    }*/

    /* gets __constructor__ property */
    virtual GASFunctionRef   Get__constructor__(GASStringContext *psc)
    { 
        GUNUSED(psc);
        return Constructor; 
    }

    void SetAltProto(GASObject* altProto)
    {
        if (altProto == SuperProto) return;
        GASSERT(!SavedProto);
        SavedProto = SuperProto;
        SuperProto = altProto;
        pProto = SuperProto;
    }
    void ResetAltProto()
    {
        if (SavedProto)
        {
            SuperProto = SavedProto;
            SavedProto = NULL;
            pProto = SuperProto;
        }
    }
    GASObject* GetSuperProto() { return SuperProto; }
};

#endif //INC_GFXOBJECTPROTO_H
