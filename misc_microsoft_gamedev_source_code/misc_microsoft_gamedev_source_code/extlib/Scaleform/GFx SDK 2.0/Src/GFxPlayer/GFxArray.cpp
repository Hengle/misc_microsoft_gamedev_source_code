/**********************************************************************

Filename    :   GFxArray.cpp
Content     :   Array object functinality
Created     :   March 10, 2006
Authors     :   Maxim Shemanarev
Notes       :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GFxArray.h"
#include "GFxNumber.h"
#include "GFxLog.h"
#include "GContainers.h"

#include <stdlib.h>


GASRecursionGuard::GASRecursionGuard(const GASArrayObject* pthis) :
    ThisPtr(pthis)
{
    ThisPtr->CallProlog();
}

GASRecursionGuard::~GASRecursionGuard()
{
    ThisPtr->CallEpilog();
}

int GASRecursionGuard::Count() const
{
    return ThisPtr->RecursionCount;
}

static bool GAS_ParseNumber(const char* str, GASNumber* retVal)
{
    if(str && *str)
    {
        if((*str >= '0' && *str <= '9') ||
            *str == '+' ||
            *str == '-' ||
            *str == '.')
        {
            char* end;
            *retVal = (GASNumber)strtod(str, &end);
            if(end == 0 || *end == 0)
            {
                return true;
            }
        }
    }
    return false;
}

int GASArraySortFunctor::Compare(const GASValue* a, const GASValue* b) const
{
    GASValue dummy;
    if(a == 0) a = &dummy;
    if(b == 0) b = &dummy;
    int cmpVal = 0;

    if(Func != NULL)
    {
        GASValue retVal;
        Env->Push(*b);
        Env->Push(*a);

        // The "This" isn't correct here. Needs to support "undefined";
        // Since GASFnCall doesn't support it we just use "This" from
        // the array object.
        GASFnCall fn(&retVal, This, Env, 2, Env->GetTopIndex());

        Func(fn);
        Env->Drop(2);
        if(fn.Result) 
        {
            cmpVal = (int)fn.Result->ToNumber(Env);
            return (Flags & GASArrayObject::SortFlags_Descending) ? -cmpVal : cmpVal;
        }
    }
    else
    {
        bool numericA = false;
        bool numericB = false;
        GASNumber valA=0;
        GASNumber valB=0;

        if(Flags & GASArrayObject::SortFlags_Numeric)
        {
            // This is an attempt to make the "NUMERIC"
            // sort logic compliant with Flash. If flag
            // "NUMERIC" is set we try to convert the
            // values to numbers. If both can be successfully
            // converted we compare them as numbers, otherwise
            // they are compared as strings.
            //
            // It actually breaks the logic of sorting if we use
            // something faster than O(N^2). Any sort algorithm that
            // is better than O(N^2) relies on the transitivity
            // of the "less-than" relation. With this logic there's
            // no transitivity. That is, a<b<c does not obligatory 
            // mean that a<c. For example:
            // a = "99 ";
            // b = 99.9;
            // c = 100;
            // Here 'a' is a string and it's compared with 'b' as a string.
            // So, we have a<b. Then, 'b' and 'c" are numbers and they
            // are compared as numbers, so, we have b<c. But 'a' and 'c' 
            // are compared as strings again, and we have a>c. 
            // 
            // It means that if we use quick sort on a mixed array
            // with Array.NUMERIC flag the result will differ from 
            // the original Flash, but still correct. 
            // It should be fine because in real life there is 
            // no need to sort mixed arrays as NUMERIC (it's a crazy 
            // case and it'd be crazy to make it 100% Flash compliant).
            if(a->GetType() == GASValue::NUMBER)
            {
                valA = a->ToNumber(Env);
                numericA = true;
            }
            else
            {
                GASString sa(a->ToString(Env));
                numericA = GAS_ParseNumber(sa.ToCStr(), &valA);
            }

            if(b->GetType() == GASValue::NUMBER)
            {
                valB = b->ToNumber(Env);
                numericB = true;
            }
            else
            {
                GASString sb(b->ToString(Env));
                numericB = GAS_ParseNumber(sb.ToCStr(), &valB);
            }
        }

        if(numericA && numericB)
        {
            if(valA < valB) cmpVal = -1;
            if(valB < valA) cmpVal =  1;
            return (Flags & GASArrayObject::SortFlags_Descending) ? -cmpVal : cmpVal;
        }
        else
        {
            GASString sa(a->ToString(Env));
            GASString sb(b->ToString(Env));
            
            cmpVal = (Flags & GASArrayObject::SortFlags_CaseInsensitive)?
                GFxString::CompareNoCase(sa.ToCStr(), sb.ToCStr()) :
                         ::gfc_strcmp (sa.ToCStr(), sb.ToCStr());
            return (Flags & GASArrayObject::SortFlags_Descending) ? -cmpVal : cmpVal;
        }
    }
    return 0;
}


GASArraySortOnFunctor::GASArraySortOnFunctor(GASObjectInterface* pThis, 
                                             const GTL::garray_cc<GASString>& fieldArray,
                                             const GTL::garray<int>& flagsArray,
                                             GASEnvironment* env,
                                             const GFxLog* log) : 
    This(pThis),
    FieldArray(&fieldArray),
    Env(env),
    LogPtr(log)
{
    FunctorArray.resize(flagsArray.size());
    unsigned i;
    for(i = 0; i < FunctorArray.size(); i++)
    {
        FunctorArray[i] = GASArraySortFunctor(pThis, 
                                              flagsArray[i],
                                              0,
                                              env,
                                              log);
    }
}


int GASArraySortOnFunctor::Compare(const GASValue* a, const GASValue* b) const
{
    GASValue dummy;
    if(a == 0) a = &dummy;
    if(b == 0) b = &dummy;
    int cmpVal = 0;

    unsigned i;
    GASSERT(Env);
    GASStringContext *psc = Env->GetSC();    

    for(i = 0; i < FunctorArray.size(); i++)
    {
        GASObjectInterface* objA = a->ToObjectInterface(Env);
        GASObjectInterface* objB = b->ToObjectInterface(Env);
        if(objA && objB)
        {
            GASValue valA;
            GASValue valB;
            const GASString& fieldName = (*FieldArray)[i];
            if(objA->GetMemberRaw(psc, fieldName, &valA) &&
               objB->GetMemberRaw(psc, fieldName, &valB))
            {
                cmpVal = FunctorArray[i].Compare(&valA, &valB);
                if(cmpVal) return cmpVal;
            }
        }
    }
    return 0;
}






GASArrayObject::GASArrayObject(GASEnvironment* penv) : 
    LogPtr(penv->GetLog()), Elements(), StringValue(), RecursionCount(0) 
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Array));
}

GASArrayObject::GASArrayObject(GASStringContext *psc) :
    LogPtr(0), Elements(), StringValue(), RecursionCount(0) 
{
    GUNUSED(psc);
}

GASArrayObject::~GASArrayObject()
{
    Resize(0);
}

bool GASArrayObject::RecursionLimitReached() const
{
    if(RecursionCount >= 255)
    {
        LogPtr->LogScriptError("256 levels of recursion is reached\n");
        return true;
    }
    return false;
}

void GASArrayObject::Resize(int size) 
{ 
    if(size < 0) size = 0;
    UPInt i;
    UPInt newSize = (UPInt)size;
    UPInt oldSize = Elements.size();
    for(i = newSize; i < oldSize; i++)
    {
        delete Elements[i];
    }
    Elements.resize(newSize);
    for(i = oldSize; i < newSize; i++)
    {
        Elements[i] = 0;
    }
}

void GASArrayObject::ShallowCopyFrom(const GASArrayObject& ao)
{
    Elements.resize(ao.Elements.size());
    UPInt i, n;
    for(i = 0, n = Elements.size(); i < n; i++)
    {
        Elements[i] = ao.Elements[i];
    }
}

void GASArrayObject::MakeDeepCopy()
{
    unsigned i;
    for(i = 0; i < Elements.size(); i++)
    {
        if(Elements[i] != 0)
        {
            GASValue* newVal = new GASValue(*Elements[i]);
            Elements[i] = newVal;
        }
    }
}

void GASArrayObject::DetachAll()
{ 
    Elements.clear(); 
}

const char* GASArrayObject::JoinToString(GASEnvironment* pEnv, const char* pDelimiter) const
{
    StringValue = "";
    GASValue undefined;

    for(unsigned i = 0; i < Elements.size(); i++)
    {
        if (i) StringValue += pDelimiter;

        if (Elements[i] != 0)
        {
            GASString s(Elements[i]->ToString(pEnv));
            StringValue += s.ToCStr();
        }
        else
        {
            GASString s(undefined.ToString(pEnv));
            StringValue += s.ToCStr();
        }
    }
    return StringValue.ToCStr();
}

void GASArrayObject::Concat(const GASValue& val)
{
    GASRecursionGuard rg(this);
    if(RecursionLimitReached()) return;

    GASObject* pObj = val.ToObject();
    if(pObj && pObj->GetObjectType() == GASObjectInterface::Object_Array)
    {
        GASArrayObject* pArray = (GASArrayObject*)pObj;
        if(pArray->Elements.size())
        {
            UInt oldSize = (UInt)Elements.size();
            Resize((int)(oldSize + pArray->Elements.size()));
            for(UPInt i = 0; i < pArray->Elements.size(); i++)
            {
                Elements[oldSize + i] = new GASValue(*pArray->Elements[i]);
            }
        }
    }
    else
    {
        Elements.push_back(new GASValue(val));
    }
}

void GASArrayObject::PushBack(const GASValue& val)
{
     Elements.push_back(new GASValue(val));
}

void GASArrayObject::PushBack()
{
     Elements.push_back(0);
}

void GASArrayObject::PopBack()
{
    if(Elements.size()) 
    {
        Resize((int)(Elements.size() - 1));
    }
}

void GASArrayObject::RemoveElements(int start, int count)
{
    if(Elements.size())
    {
        int i;
        for(i = 0; i < count; i++)
        {
            delete Elements[start + i];
        }
        for(i = start + count; i < (int)Elements.size(); i++)
        {
            Elements[i - count] = Elements[i];
            Elements[i] = 0;
        }
        Elements.resize(Elements.size() - count);
    }
}

void GASArrayObject::InsertEmpty(int start, int count)
{
    int i;
    int oldSize = (int)Elements.size();
    Elements.resize(oldSize + count);
    if(oldSize)
    {
        for(int i = (int)Elements.size() - 1; i >= start + count; --i)
        {
            Elements[i] = Elements[i - count];
        }
    }
    for(i = 0; i < count; i++)
    {
        Elements[start + i] = 0;
    }
}

void GASArrayObject::PopFront()
{
    if(Elements.size())
    {
        delete Elements[0];
        for(unsigned i = 1; i < Elements.size(); i++)
        {
            Elements[i-1] = Elements[i];
        }
        Elements[Elements.size() - 1] = 0;
        Elements.resize(Elements.size() - 1);
    }
}

void GASArrayObject::SetElement(int i, const GASValue& val)
{
    if (i < 0 || i >= int(Elements.size ())) return;

    if(Elements[i] == 0) Elements[i] = new GASValue;
    *Elements[i] = val;
}

void GASArrayObject::Reverse()
{
    int i = 0;
    int j = (int)Elements.size() - 1;
    while(i < j)
    {
        GTL::gswap(Elements[i], Elements[j]);
        ++i;
        --j;
    }
}

void GAS_ArrayConcat(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    //GPtr<GASArrayObject> ao = *new GASArrayObject(fn.Env);
    GPtr<GASArrayObject> ao = static_cast<GASArrayObject*>(
                                fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Array)).GetPtr());
    ao->Concat(pThis);
    for(int i = 0; i < fn.NArgs; i++)
    {
        ao->Concat(fn.Arg(i));
    }
    fn.Result->SetAsObject(ao.GetPtr());
}

void GAS_ArrayJoin(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);    

    //const char* pDelimiter = (fn.NArgs == 0) ? "," : fn.Arg(0).ToString(fn.Env);
    const char* pjoin;
    if (fn.NArgs == 0)
    {
        pjoin = pThis->JoinToString(fn.Env, ",");
    }
    else
    {
        GASString sa0(fn.Arg(0).ToString(fn.Env));
        pjoin = pThis->JoinToString(fn.Env, sa0.ToCStr());
    }    
    fn.Result->SetString(fn.Env->CreateString(pjoin));
}

void GAS_ArrayPop(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);
    if(pThis->GetSize())
    {
        GASValue* pVal = pThis->GetElementPtr(pThis->GetSize() - 1);
        if(pVal) *fn.Result = *pVal;
        else      fn.Result->SetUndefined();
        pThis->PopBack();
    }
    else
    {
        fn.Result->SetUndefined();
    }
}

void GAS_ArrayPush(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);
    for(int i = 0; i < fn.NArgs; i++)
    {
        pThis->PushBack(fn.Arg(i));
    }
    fn.Result->SetInt(pThis->GetSize());
}

void GAS_ArrayReverse(const GASFnCall& fn)
{
    GASArrayObject* pthis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pthis);
    pthis->Reverse();

    // The documentation says that "reverse" returns Void, but
    // actually Flash 8 returns self.
    fn.Result->SetAsObject(pthis);
}

void GAS_ArrayShift(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    if(pThis->GetSize() == 0)
    {
        fn.Result->SetUndefined();
        return;
    }
    
    GASValue* pVal = pThis->GetElementPtr(0);
    if(pVal) *fn.Result = *pVal;
    else      fn.Result->SetUndefined();
    pThis->PopFront();
}

void GAS_ArraySlice(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    int start = 0;
    int end   = pThis->GetSize();
    if (fn.NArgs >= 1)
    {
        start = (int) fn.Arg(0).ToNumber(fn.Env);
        if(start < 0) start += pThis->GetSize();
        if(start < 0) start = 0;
        if(start > pThis->GetSize()) start = pThis->GetSize();
    }
    if (fn.NArgs >= 2)
    {
        end = (int) fn.Arg(1).ToNumber(fn.Env);
        if(end < 0) end += pThis->GetSize();
        if(end < 0) end = 0;
        if(end > pThis->GetSize()) end = pThis->GetSize();
    }
    
    //GPtr<GASArrayObject> ao = *new GASArrayObject(fn.Env);
    GPtr<GASArrayObject> ao = static_cast<GASArrayObject*>(
                              fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Array)).GetPtr() );
    for(int i = start; i < end; i++)
    {
        if(pThis->GetElementPtr(i) != 0) 
        {
            ao->PushBack(*pThis->GetElementPtr(i));
        }
        else
        {
            ao->PushBack();
        }
    }
    fn.Result->SetAsObject(ao.GetPtr());
}

void GAS_ArraySort(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    GASFunctionRef scriptFunctor;
    int  flags = 0;
    if(fn.NArgs >= 1)
    {
        if(fn.Arg(0).IsFunction()) scriptFunctor = fn.Arg(0).ToFunction();
        else flags = (int)fn.Arg(0).ToNumber(fn.Env);
    }

    if(scriptFunctor != NULL && fn.NArgs >= 2)
    {
        flags = (int)fn.Arg(1).ToNumber(fn.Env);
    }

    //GPtr<GASArrayObject> retArray = *new GASArrayObject(fn.Env);
    GPtr<GASArrayObject> retArray = static_cast<GASArrayObject*>(
                                    fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Array)).GetPtr());
    retArray->ShallowCopyFrom(*pThis);

    GASArraySortFunctor sortFunctor(retArray, 
                                    flags, 
                                    scriptFunctor, 
                                    fn.Env, 
                                    pThis->GetLogPtr());
    retArray->Sort(sortFunctor);

    bool uniqueSortFailed = false;
    if(flags & GASArrayObject::SortFlags_UniqueSort)
    {
        int i;
        for(i = 1; i < retArray->GetSize(); i++)
        {
            if(sortFunctor.Compare(retArray->GetElementPtr(i-1), 
                                   retArray->GetElementPtr(i)) == 0) 
            {
                uniqueSortFailed = true;
                break;
            }
        }
    }

    if(uniqueSortFailed)
    {
        fn.Result->SetInt(0);
        retArray->DetachAll();
        return;
    }

    if(flags & GASArrayObject::SortFlags_ReturnIndexedArray)
    {
        retArray->MakeDeepCopy();
        fn.Result->SetAsObject(retArray);
    }
    else
    {
        pThis->ShallowCopyFrom(*retArray);
        retArray->DetachAll();
        fn.Result->SetAsObject(pThis);
    }
}

void GAS_ArraySortOn(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    // GASString array requires a default value, since it only uses copy constructors.
    GTL::garray_cc<GASString>  fieldNames(fn.Env->GetBuiltin(GASBuiltin_empty_));
    GTL::garray<int>           fieldFlags;
    int                        commonFlags = 0;

    if(fn.NArgs == 0)
    {
        fn.Result->SetUndefined();
        return;
    }

    int       i;
    GASString dummyName(fn.Env->GetBuiltin(GASBuiltin_undefined_));

    if(fn.NArgs >= 1)
    {
        // *** Create an array of field names

        GASObject* fields = fn.Arg(0).ToObject();
        if(fields && fields->GetObjectType() == GASObjectInterface::Object_Array)
        {
            GASArrayObject* pFieldArray = (GASArrayObject*)fields;
            for(i = 0; i < pFieldArray->GetSize(); i++)
            {
                const GASValue* pName = pFieldArray->GetElementPtr(i);

                // If by accident there's no field name defined use some name
                // that the function GetMember() will return 'false' for sure.
                // This field will simply be skipped.
                fieldNames.push_back(pName ? pName->ToString(fn.Env) : dummyName);
            }
        }
        else
        {
            fieldNames.push_back(fn.Arg(0).ToString(fn.Env));
        }
    }

    for(i = 0; i < (int)fieldNames.size(); i++)
    {
        fieldFlags.push_back(0);
    }

    if(fn.NArgs >= 2)
    {
        // *** Create an array of flags

        GASObject* flags = fn.Arg(1).ToObject();
        if(flags && flags->GetObjectType() == GASObjectInterface::Object_Array)
        {
            GASArrayObject* pFlagsArray = (GASArrayObject*)flags;
            for(i = 0; i < pFlagsArray->GetSize() && 
                       i < (int)fieldNames.size(); i++)
            {
                const GASValue* pFlags = pFlagsArray->GetElementPtr(i);
                if(pFlags)
                {
                    fieldFlags[i] = (int)pFlags->ToNumber(fn.Env);
                }
            }
        }
        else
        {
            commonFlags = (int)fn.Arg(1).ToNumber(fn.Env);
            for(i = 0; i < (int)fieldNames.size(); i++)
            {
                fieldFlags[i] = commonFlags;
            }
        }
    }

    //GPtr<GASArrayObject> retArray = *new GASArrayObject(fn.Env);
    GPtr<GASArrayObject> retArray = static_cast<GASArrayObject*>(
                                    fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Array)).GetPtr());
    retArray->ShallowCopyFrom(*pThis);

    GASArraySortOnFunctor sortFunctor(retArray, 
                                      fieldNames, 
                                      fieldFlags, 
                                      fn.Env, 
                                      pThis->GetLogPtr());
    retArray->Sort(sortFunctor);

    bool uniqueSortFailed = false;
    if(commonFlags & GASArrayObject::SortFlags_UniqueSort)
    {
        int i;
        for(i = 1; i < retArray->GetSize(); i++)
        {
            if(sortFunctor.Compare(retArray->GetElementPtr(i-1), retArray->GetElementPtr(i)) == 0)
            {
                uniqueSortFailed = true;
                break;
            }
        }
    }

    if(uniqueSortFailed)
    {
        fn.Result->SetInt(0);
        return;
    }

    if(commonFlags & GASArrayObject::SortFlags_ReturnIndexedArray)
    {
        retArray->MakeDeepCopy();
        fn.Result->SetAsObject(retArray);
    }
    else
    {
        pThis->ShallowCopyFrom(*retArray);
        retArray->DetachAll();
        fn.Result->SetAsObject(pThis);
    }
}

void GAS_ArraySplice(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs == 0)
    {
        fn.Result->SetUndefined();
        return;
    }

    int start = (int) fn.Arg(0).ToNumber(fn.Env);
    if(start < 0) start += pThis->GetSize();
    if(start < 0) start = 0;
    if(start > pThis->GetSize()) start = pThis->GetSize();

    int count = pThis->GetSize() - start;
    if (fn.NArgs >= 2)
    {
        count = (int) fn.Arg(1).ToNumber(fn.Env);
        if(count < 0) count = 0;
        if(start + count >= pThis->GetSize()) 
        {
            count = pThis->GetSize() - start;
        }
    }

    int i;
    //GPtr<GASArrayObject> ao = *new GASArrayObject(fn.Env);
    GPtr<GASArrayObject> ao = static_cast<GASArrayObject*>(
                              fn.Env->OperatorNew(fn.Env->GetBuiltin(GASBuiltin_Array)).GetPtr());
    for(i = 0; i < count; i++)
    {
        if(pThis->GetElementPtr(start + i) != 0) 
        {
            ao->PushBack(*pThis->GetElementPtr(start + i));
        }
        else
        {
            ao->PushBack();
        }
    }
    fn.Result->SetAsObject(ao.GetPtr());

    if(count)
    {
        pThis->RemoveElements(start, count);
    }

    if (fn.NArgs >= 3)
    {
        pThis->InsertEmpty(start, fn.NArgs - 2);
        for(i = 2; i < fn.NArgs; i++)
        {
            pThis->SetElement(start + i - 2, fn.Arg(i));
        }
    }
}

void GAS_ArrayToString(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);
    GASRecursionGuard rg(pThis);
    if(pThis->RecursionLimitReached()) 
    {
        fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_empty_));
    }
    else
    {
        fn.Result->SetString(fn.Env->CreateString(pThis->JoinToString(fn.Env, ",")));
    }
}

void GAS_ArrayUnshift(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    if(fn.NArgs > 0)
    {
        pThis->InsertEmpty(0, fn.NArgs);
        for(int i = 0; i < fn.NArgs; i++)
        {
            pThis->SetElement(i, fn.Arg(i));
        }
    }
    fn.Result->SetInt(pThis->GetSize());
}

void GAS_ArrayLength(const GASFnCall& fn)
{
    GASArrayObject* pThis = (GASArrayObject*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetInt(pThis->GetSize());
}

void GAS_ArrayValueOf(const GASFnCall& fn)
{
    // looks like Array.valueOf returns same value as .toString.
    GAS_ArrayToString (fn);
}

static const GASNameNumber GASArrayConstTable[] = 
{
    { "CASEINSENSITIVE",    GASArrayObject::SortFlags_CaseInsensitive },
    { "DESCENDING",         GASArrayObject::SortFlags_Descending },
    { "NUMERIC",            GASArrayObject::SortFlags_Numeric    },
    { "RETURNINDEXEDARRAY", GASArrayObject::SortFlags_ReturnIndexedArray },
    { "UNIQUESORT",         GASArrayObject::SortFlags_UniqueSort },
    { 0, 0 }
};

// Must be sorted!
static const GASNameFunction GASArrayFunctionTable[] = 
{
    { "concat",             &GAS_ArrayConcat     },
    { "join",               &GAS_ArrayJoin       },
    { "pop",                &GAS_ArrayPop        },
    { "push",               &GAS_ArrayPush       },
    { "reverse",            &GAS_ArrayReverse    },
    { "shift",              &GAS_ArrayShift      },
    { "slice",              &GAS_ArraySlice      },
    { "sort",               &GAS_ArraySort       },
    { "sortOn",             &GAS_ArraySortOn     },
    { "splice",             &GAS_ArraySplice     },
    { "toString",           &GAS_ArrayToString   },
    { "unshift",            &GAS_ArrayUnshift    },
    { "valueOf",            &GAS_ArrayValueOf    },
    { 0, 0 }
};




// TODO: Check this function for possible incompatibility
// with Flash. The behavior of Flash is somewhat strange:
//
// var a = new Array; 
// var b = new Array; 
// a["100"]  = 100; trace(a.length + " " + a[100]); // Output: 101 100
// b[" 100"] = 100; trace(b.length + " " + b[100]); // Output: 101 undefined
//
// That is, an index with leading spaces affects the size, but
// does not create an element. This function is more strict, that is, 
// if  'name' is not a non-negative number (leading spaces are 
// considered as not a number) it returns -1, so that, in any suspicious
// case the standard "member" mechanism works for the array.
// 
// The UTF-8 strings should work correctly: There ain't no digits in UNICODE
// other than ASCII 0...9, are there? And in the UTF-8 encoding, as soon
// as the most significant bit is set (multibyte character) the function will 
// return -1 right away.
//---------------------------------------------------------------------------
int GASArrayObject::ParseIndex(const GASString& name)
{
    const char* p = name.ToCStr();
    while(*p && *p >= '0' && *p <= '9') 
    {
        p++;
    }
    if(*p) return -1;
    return atoi(name.ToCStr());
}

// Overridden GASObject::SetMember
bool GASArrayObject::SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    int idx;
    if (psc->GetBuiltin(GASBuiltin_length).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))        
    {
        int nlen = (int)val.ToNumber(0);
        Resize((nlen < 0) ? 0 : nlen);   // don't resize to negative length, but...
        return GASObject::SetMemberRaw(psc, name, val, flags); // store the original length's value in members.
    }
    else if((idx = ParseIndex(name)) >= 0)
    {
        if(idx >= (int)Elements.size())
        {
            Resize(idx + 1);
        }
        if(Elements[idx] == 0)
        {
            Elements[idx] = new GASValue();
        }
        *Elements[idx] = val;
        return true;
    }
    else
    {
        return GASObject::SetMemberRaw(psc, name, val, flags);
    }
}

// Overridden GASObject::GetMemberRaw
bool GASArrayObject::GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
{
    int idx;
    if((idx = ParseIndex(name)) >= 0)
    {
        if(idx >= (int)Elements.size() || Elements[idx] == 0)
        {
            val->SetUndefined();
            return true;
        }
        *val = *Elements[idx];
        return true;
    }
    else

    // if (name == "length")
    if (psc->GetBuiltin(GASBuiltin_length).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))
    {
        if (Elements.size() > 0 || !HasMember(psc, name, false))
        {
            val->SetInt((int)Elements.size());
            return true;
        }
        // if Elements.size () == 0 && HasMember ("length") then 
        // report length as it set. For example, "length" may be set to
        // negative value and it should be reported as negative value
        // while the actual size of array is 0.
    }
    return GASObject::GetMemberRaw(psc, name, val);
}

bool GASArrayObject::DeleteMember(GASStringContext *psc, const GASString& name)
{
    // special case for delete array[n]: in this case name is 
    // an index in array to be removed. Item at the index becomes
    // "undefined" after removing.
    if (name.GetSize() > 0 && isdigit (name[0]))
    {
        int index;
        if((index = ParseIndex(name)) >= 0)
        {
            SetElement(index, GASValue());
            return true;
        }
        return false;
    }
    return GASObject::DeleteMember(psc, name);
}

void GASArrayObject::VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags) const
{
    // invoke base class method first
    // for .. in operator puts all elements in stack first
    // and then pops them up one by one. So, after popping elements from the stack
    // the last array element will be first, and after all elements all other 
    // properties will be iterated (if not hidden)
    GASObject::VisitMembers(psc, pvisitor, visitFlags);

    // iterate through array's elements first...
    UPInt i, n = Elements.size();
    UPInt n1 = (n > GASBuiltinConst_DigitMax_) ? (GASBuiltinConst_DigitMax_+1) : n;
    char buffer[20];

    for(i = 0; i < n1; ++i)
    {
        if (!Elements[i]) continue;      
        pvisitor->Visit(psc->GetBuiltin((GASBuiltinType)(GASBuiltin_0 + i)), *Elements[i], 0);
    }
    for(; i < n; ++i)
    {
        if (!Elements[i]) continue;
        gfc_sprintf(buffer, sizeof (buffer), "%d", i);
        pvisitor->Visit(psc->CreateString(buffer), *Elements[i], 0);
    }
}

// Overridden GASObject::GetTextValue
const char* GASArrayObject::GetTextValue(GASEnvironment* pEnv) const
{
    GASRecursionGuard rg(this);
    if (RecursionLimitReached()) return "";
    return JoinToString(pEnv, ",");
}

// Overridden GASObject::GetObjectType
GASObjectInterface::ObjectType  GASArrayObject::GetObjectType() const
{
    return Object_Array;
}

void GASArrayObject::InitArray(const GASFnCall& fn)
{
    // Use the arguments as initializers.
    GASValue    IndexNumber;
    for (int i = 0; i < fn.NArgs; i++)
    {
        IndexNumber.SetInt(i);
        SetMemberRaw(fn.Env->GetSC(), IndexNumber.ToString(fn.Env), fn.Arg(i));
    }
}


//////////////////////////
GASArrayProto::GASArrayProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASArrayObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GASArrayFunctionTable, pprototype);
}

void GASArrayProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASArrayObject> ao = *new GASArrayObject(fn.Env);
    ao->SetMemberRaw(fn.Env->GetSC(), fn.Env->GetBuiltin(GASBuiltin_length),
                     GASValue(int(0)), GASPropFlags::PropFlag_DontEnum);

    if (fn.NArgs == 0)
    {
    }
    else if (fn.NArgs == 1
         && fn.Arg(0).GetType() == GASValue::NUMBER)
    {
        ao->Resize((int)fn.Arg(0).ToNumber(fn.Env));
    }
    else
    {
        // Use the arguments as initializers.
        ao->InitArray (fn);
    }

    fn.Result->SetAsObject(ao.GetPtr());
}

void GASArrayProto::DeclareArray(const GASFnCall& fn)
{
    GPtr<GASArrayObject> ao = *new GASArrayObject(fn.Env);
    ao->SetMemberRaw(fn.Env->GetSC(), fn.Env->GetBuiltin(GASBuiltin_length),
                     GASValue(int(0)), GASPropFlags::PropFlag_DontEnum);

    // declared array has a "constructor" property and doesn't have "__constructor__"
    GASFunctionRef ctor = fn.Env->GetConstructor(GASBuiltin_Array);
    ao->Set_constructor(fn.Env->GetSC(), ctor);

    if (fn.NArgs == 0)
    {
    }
    else
    {
        // Use the arguments as initializers.
        ao->InitArray (fn);
    }

    fn.Result->SetAsObject(ao.GetPtr());
}

GASArrayCtorFunction::GASArrayCtorFunction(GASStringContext *psc) :
    GASFunctionObject (GASArrayProto::GlobalCtor)
{
    for(int i = 0; GASArrayConstTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, GASArrayConstTable[i].Name, GASArrayConstTable[i].Number, 
            GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}
