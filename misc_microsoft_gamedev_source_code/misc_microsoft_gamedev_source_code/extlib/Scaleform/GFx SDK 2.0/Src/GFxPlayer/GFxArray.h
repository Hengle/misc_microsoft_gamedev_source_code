/**********************************************************************

Filename    :   GFxArray.h
Content     :   Array object functinality
Created     :   March 10, 2006
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXARRAY_H
#define INC_GFXARRAY_H

#include "GContainers.h"
#include "GFxAction.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASRecursionGuard;
class GASArraySortFunctor;
class GASArraySortOnFunctor;
class GASArrayObject;
class GASArrayProto;
class GASArrayCtorFunction;



class GASRecursionGuard
{
public:
    GASRecursionGuard(const GASArrayObject* pThis);
    ~GASRecursionGuard();
    int Count() const;
private:
    const GASArrayObject* ThisPtr;
};


class GASArraySortFunctor
{
public:
    GASArraySortFunctor() {}
    GASArraySortFunctor(GASObjectInterface* pThis, 
                        int flags,
                        const GASFunctionRef& func,
                        GASEnvironment* env,
                        const GFxLog* log=0) : 
        This(pThis), 
        Flags(flags), 
        Func(func), 
        Env(env), 
        LogPtr(log)
    {}

    int Compare(const GASValue* a, const GASValue* b) const;
    bool operator()(const GASValue* a, const GASValue* b) const
    {
        return Compare(a, b) < 0;
    }

private:
    GASObjectInterface* This;
    int                 Flags;
    GASFunctionRef      Func;
    GASEnvironment*     Env;
    const GFxLog*       LogPtr;
};


class GASArraySortOnFunctor
{
public:
    GASArraySortOnFunctor(GASObjectInterface* pThis, 
                          const GTL::garray_cc<GASString>& fieldArray,
                          const GTL::garray<int>& flagsArray,
                          GASEnvironment* env,
                          const GFxLog* log=0); 

    int Compare(const GASValue* a, const GASValue* b) const;
    bool operator()(const GASValue* a, const GASValue* b) const
    {
        return Compare(a, b) < 0;
    }

private:
    GASObjectInterface*              This;
    const GTL::garray_cc<GASString>* FieldArray;
    GASEnvironment*                  Env;
    const GFxLog*                    LogPtr;
    GTL::garray<GASArraySortFunctor> FunctorArray;
};




class GASArrayObject : public GASObject
{
    friend class GASArrayProto;
protected:

    void InitArray(const GASFnCall& fn);
public:
    enum SortFlags
    {
        SortFlags_CaseInsensitive    = 1,
        SortFlags_Descending         = 2, 
        SortFlags_UniqueSort         = 4, 
        SortFlags_ReturnIndexedArray = 8,
        SortFlags_Numeric            = 16 
    };
    
    friend class GASRecursionGuard;

    GASArrayObject(GASEnvironment* penv);
    GASArrayObject(GASStringContext *psc);
    ~GASArrayObject();
    
    virtual bool        SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool        GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);
    virtual bool        DeleteMember(GASStringContext *psc, const GASString& name);
    virtual void        VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags) const;   

    // pEnv is required for Array's GetTextValue.
    virtual const char* GetTextValue(GASEnvironment* penv) const;
    virtual ObjectType  GetObjectType() const;

    int             GetSize() const { return (UInt)Elements.size(); }
    void            Resize(int newSize);
          GASValue* GetElementPtr(int i)       { return Elements[i]; }
    const GASValue* GetElementPtr(int i) const { return Elements[i]; }
    void            SetElement(int i, const GASValue& val);
    void            PushBack(const GASValue& val);
    void            PushBack();
    void            PopBack();
    void            PopFront();
    void            RemoveElements(int start, int count);
    void            InsertEmpty(int start, int count);
    const char*     JoinToString(GASEnvironment* pEnv, const char* pDelimiter) const;
    void            Concat(const GASValue& val);
    void            ShallowCopyFrom(const GASArrayObject& ao);
    void            MakeDeepCopy();
    void            DetachAll();
    void            Reverse();

    template<class SortFunctor>
    void            Sort(SortFunctor& sf)
    {
        if (Elements.size() > 0)
        {
            GArrayAdaptor<GASValue*> a(&Elements[0], (UInt)Elements.size());
            GAlg::QuickSort(a, sf);
        }
    }

    static int      ParseIndex(const GASString& name);

    bool            RecursionLimitReached() const;
    const GFxLog*   GetLogPtr() const { return LogPtr; }

private:
    void CallProlog() const { RecursionCount++; }
    void CallEpilog() const { RecursionCount--; }

    const GFxLog*          LogPtr;
    GTL::garray<GASValue*> Elements;
    mutable GFxString      StringValue;
    mutable int            RecursionCount;
};


class GASArrayProto : public GASPrototype<GASArrayObject>
{
public:
    GASArrayProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
    static void DeclareArray(const GASFnCall& fn);
};


class GASArrayCtorFunction : public GASFunctionObject
{
public:
    GASArrayCtorFunction (GASStringContext *psc);
};

#endif
