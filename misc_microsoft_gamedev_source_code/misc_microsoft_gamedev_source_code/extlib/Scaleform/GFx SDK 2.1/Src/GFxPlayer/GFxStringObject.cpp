/**********************************************************************

Filename    :   GFxStringObject.cpp
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

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxASString.h"
#include "GFxAction.h"
#include "GFxArray.h"
#include "GFxNumber.h"
#include "GFxStringObject.h"
#include "GUTF8Util.h"

GASStringObject::GASStringObject(GASEnvironment* penv)
    : Value(penv->GetBuiltin(GASBuiltin_empty_))
{
    commonInit(penv);
}

GASStringObject::GASStringObject(GASEnvironment* penv, const GASString& val)
    : Value (val)
{
    commonInit(penv);
}

void GASStringObject::commonInit(GASEnvironment* penv)
{    
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_String));
}
    
const char* GASStringObject::GetTextValue(GASEnvironment*) const
{
    return Value.ToCStr();
}

bool GASStringObject::GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
{
    // if (name == "length")        
    if (psc->GetBuiltin(GASBuiltin_length).CompareBuiltIn_CaseCheck(name, psc->IsCaseSensitive()))
    {
        if (!GASObject::GetMemberRaw(psc, name, val) || !val->IsSet())
            val->SetInt(Value.GetLength());
        return true;
    }
    if (GASObject::GetMemberRaw(psc, name, val))
    {
        return true;
    }
    return false;
}

void        GASStringObject::SetValue(GASEnvironment* penv, const GASValue& v)
{
    GASString s(v.ToString(penv));
    Value = s.ToCStr();
}

GASValue    GASStringObject::GetValue() const
{
    return GASValue(Value);
}

//////////////////////////////////////////
//
void GASStringProto::StringCharAt(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);

    const GASString& str = pThis->GetString();

    GASString retVal(fn.Env->GetBuiltin(GASBuiltin_empty_));
    int index = (int) fn.Arg(0).ToNumber(fn.Env);
    if (index >= 0 && index < (int)str.GetLength())
    {
        retVal = retVal.AppendChar(str.GetCharAt(index));
    }
    fn.Result->SetString(retVal);
}

void GASStringProto::StringCharCodeAt(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);

    const GASString& str = pThis->GetString();

    if (fn.NArgs >= 1)
    {
        int index = (int) fn.Arg(0).ToNumber(fn.Env);
        if (index >= 0 && index < (int)str.GetLength())
        {
            fn.Result->SetNumber((GASNumber)str.GetCharAt(index));
            return;
        }
    }
    fn.Result->SetNumber(GASNumberUtil::NaN());
}

void GASStringProto::StringConcat(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);

    // Use resizable GFxString buffer (avoid hash re-calculation).
    GFxString retVal(pThis->GetString().ToCStr(), pThis->GetString().GetSize());    
    for (int i = 0; i < fn.NArgs; i++)
    {
        // Must create temporary to cold pointer.
        GASString s(fn.Arg(i).ToString(fn.Env));
        retVal += s.ToCStr();
    }

    fn.Result->SetString(fn.Env->CreateString(retVal));
}

void GASStringProto::StringIndexOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
    {
        fn.Result->SetNumber(-1);
        return;
    }
    
    GASString   searchAddRef(fn.Arg(0).ToString(fn.Env));
    if (searchAddRef.GetLength() == 0)
    {
        // if an empty string is being searched - return zero.
        fn.Result->SetNumber(0);
        return;
    }
    const char* search = searchAddRef.ToCStr();
    // Ok since GetString() returns ref to living object
    const char* str = pThis->GetString().ToCStr();

    int start = 0;
    if (fn.NArgs > 1)
    {
        start = (int) fn.Arg(1).ToNumber(fn.Env);
    }

    UInt32 chr;
    UInt32 first = GUTF8Util::DecodeNextChar(&search);

    for(int i = 0; 
        (chr = GUTF8Util::DecodeNextChar(&str)) != 0;
        i++)
    {
        if(i >= start && chr == first)
        {
            const char* s1 = str;
            const char* s2 = search;
            UInt32 c1, c2=0;
            for(;;)
            {
                c1 = GUTF8Util::DecodeNextChar(&s1);
                c2 = GUTF8Util::DecodeNextChar(&s2);

                if(c1 == 0 || c2 == 0 || c1 != c2) 
                    break;
            }
            if(c2 == 0)
            {
                fn.Result->SetInt(i);
                return;
            }
            if(c1 == 0)
            {
                fn.Result->SetInt(-1);
                return;
            }
        }
    }
    fn.Result->SetNumber(-1);
    return;
}

void GASStringProto::StringLastIndexOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
    {
        fn.Result->SetNumber(-1);
        return;
    }

    GASString asStr(pThis->GetString());
    GASString searchAddRef(fn.Arg(0).ToString(fn.Env));
    if (searchAddRef.GetLength() == 0)
    {
        // if an empty string is being searched - return the length of the
        // "this" string.
        fn.Result->SetNumber(GASNumber(asStr.GetLength()));
        return;
    }
    const char* str = asStr.ToCStr();

    const char* search = searchAddRef.ToCStr();
    int start = 0x7FFFFFF;
    if (fn.NArgs > 1)
    {
        start = (int) fn.Arg(1).ToNumber(fn.Env);
    }

    UInt32 chr;
    UInt32 first = GUTF8Util::DecodeNextChar(&search);
    int lastIndex = -1;

    for(int i = 0; 
        (chr = GUTF8Util::DecodeNextChar(&str)) != 0;
        i++)
    {
        if(i <= start && chr == first)
        {
            const char* s1 = str;
            const char* s2 = search;
            UInt32 c1, c2=0;
            for(;;)
            {
                c1 = GUTF8Util::DecodeNextChar(&s1);
                c2 = GUTF8Util::DecodeNextChar(&s2);

                if(c1 == 0 || c2 == 0 || c1 != c2) 
                    break;
            }
            if(c2 == 0)
            {
                lastIndex = i;
            }
            if(c1 == 0)
            {
                break;
            }
        }
    }
    fn.Result->SetInt(lastIndex);
    return;
}

void GASStringProto::StringSlice(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);
    int start  = 0;
    int length = -1;
        
    if (fn.NArgs >= 1)
    {
        start = (int) fn.Arg(0).ToNumber(fn.Env);
        if (start < 0)
            start += pThis->GetString().GetLength();
    }
    if (fn.NArgs >= 2)
    {
        int end = (int) fn.Arg(1).ToNumber(fn.Env);
        if (end < 0)
            end += pThis->GetString().GetLength();
        if (end < start) 
        {   // empty string
            fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_empty_));
            return;
        }
        length = end - start;
    }    
    fn.Result->SetString(StringSubstring(pThis->GetString(), start, length));
}

void GASStringProto::StringSplit(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);

    GASString   search(fn.Env->GetBuiltin(GASBuiltin_empty_));    
    const char* psearch = 0;
    
    if (fn.NArgs >= 1)
    {        
        search = fn.Arg(0).ToString(fn.Env);
        psearch = search.ToCStr();
    }

    int limit = 0x3FFFFFFF;
    if (fn.NArgs >= 2)
    {
        limit = (int)fn.Arg(1).ToNumber(fn.Env);
        if(limit < 0) limit = 0;
    }

    GPtr<GASArrayObject> retVal = StringSplit(fn.Env, pThis->GetString(), psearch, limit);
    fn.Result->SetAsObject(retVal.GetPtr());
}

GPtr<GASArrayObject> GASStringProto::StringSplit(GASEnvironment* penv, const GASString& str, const char* delimiters, int limit)
{
    GPtr<GASArrayObject> retVal = static_cast<GASArrayObject*>(
                                  penv->OperatorNew(penv->GetBuiltin(GASBuiltin_Array)).GetPtr());

    const char*         search = delimiters;
    GASStringContext*   psc = penv->GetSC();    
    const char*         pstr = str.ToCStr();
    
    if(limit < 0) limit = 0;

    UInt32 c1, c2;
    
    if (search == 0)
    {
        retVal->PushBack(GASValue(str));
    }
    else if (*search == 0)
    {
        GFxString tmpStr;

        while((c1 = GUTF8Util::DecodeNextChar(&pstr)) != 0)
        {
            tmpStr.Clear();
            tmpStr.AppendChar(c1);
            retVal->PushBack(GASValue(psc->CreateString(tmpStr)));
        }
    }
    else
    {
        int count = 0;
        const char* start = pstr;

        for(;;)
        {
            const char* s2   = search;
            const char* end  = pstr;
            const char* prev = pstr;
            const char* pnext = NULL;

            for(;;)
            {
                prev = pstr;
                c1 = GUTF8Util::DecodeNextChar(&pstr);
                c2 = GUTF8Util::DecodeNextChar(&s2);
                if (!pnext)
                    pnext = pstr;
                if (c1 == 0 || c2 == 0) 
                {
                    break;
                }
                if (c1 != c2)
                {
                    pstr = pnext;
                    break;
                }
            }
            if (c2 == 0)
            {
                if (count >= limit) break;
                retVal->PushBack(GASValue(CreateStringFromCStr(psc, start, end)));
                pstr = start = prev;
                ++count;
            }
            if (c1 == 0)
            {
                if (count < limit)
                {                    
                    retVal->PushBack(GASValue(CreateStringFromCStr(psc, start, end)));
                }
                break;
            }
        }
    }
    return retVal;
}

// This function reproduces the behavior of 
// function "substring" (not method!) except
// for that it's zero-based instead of one-based
//
// The rules:
// start:  zero-based, 
//         if < 0 then start = 0
//         if >= strlen then return empty
//
// length: if = 0 then return empty
//         if < 0    then return tail
//         if > tail then return tail
GASString GASStringProto::StringSubstring(const GASString& self, int start, int length)
{
    if(length == 0)    
        return self.GetManager()->CreateEmptyString();

    if (start < 0)
        start = 0;
    
    int utf8Len = self.GetLength();
    if (start >= utf8Len)    
        return self.GetManager()->CreateEmptyString();

    if (length < 0 || start + length > utf8Len)
    {
        length = utf8Len - start;
    }

    return self.Substring(start, start + length);
}

GASString GASStringProto::CreateStringFromCStr(GASStringContext* psc, const char* start, const char* end)
{
    int len = (end == 0) ? (int)strlen(start) : int(end - start);    
    if (len < 0)
        len = 0;    
    if (len > 0)
        return psc->CreateString(start, len);
    return psc->GetBuiltin(GASBuiltin_empty_);
}


void GASStringProto::StringSubstr(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);
    int start  = 0;
    int length = -1;
    if (fn.NArgs >= 1)
    {
        start = (int) fn.Arg(0).ToNumber(fn.Env);
        if(start < 0) start += pThis->GetString().GetLength();
    }
    if (fn.NArgs >= 2)
    {
        length = (int) fn.Arg(1).ToNumber(fn.Env);
        if (length < 0) 
        {
            // The behavior of method "substr" differs
            // from function "substring". 
            // In "substring" if length is negative it 
            // returns the tail, while in method it returns 
            // an empty string.
            length = 0;
        }
    }    
    fn.Result->SetString(StringSubstring(pThis->GetString(), start, length));
}

void GASStringProto::StringSubstring(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);
    
    int start  = 0;
    int length = -1;

    if (fn.NArgs >= 1)
    {
        start = (int) fn.Arg(0).ToNumber(fn.Env);
    }
    if (fn.NArgs >= 2)
    {
        int end = (int) fn.Arg(1).ToNumber(fn.Env);
        if (end < start) 
        {
            // This code reproduces the behavior of the
            // original Flash player. The docs say:
            //-------
            // "If the value of start is greater than the value 
            //  of end, the parameters are automatically swapped 
            //  before the function executes"
            //-------
            // But it's more complex than that: 
            // If 'start' is out of range it always returns 
            // an empty string, no matter if the actual 
            // start-end range intersects with 0...length-1.
            // In this case it calls GetUTF8Length() twice, 
            // but it's OK because the end < start case is pretty
            // rear (almost never happens).
            int utf8Len = pThis->GetString().GetLength();
            if(start >= utf8Len)
            {
                fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_empty_));
                return;

            }
            GTL::gswap(start, end);
        }
        if (start < 0)
            start = 0;
        length = end - start;
    }

    fn.Result->SetString(StringSubstring(pThis->GetString(), start, length));
}

void GASStringProto::StringToLowerCase(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);
    fn.Result->SetString(pThis->GetString().ToLower());
}

void GASStringProto::StringToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);
    fn.Result->SetString(pThis->GetString());
}

void GASStringProto::StringToUpperCase(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);    
    fn.Result->SetString(pThis->GetString().ToUpper());    
}

void GASStringProto::StringValueOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, String, "String");
    GASStringObject* pThis = (GASStringObject*) fn.ThisPtr;
    GASSERT(pThis);
    fn.Result->SetString(pThis->GetString());
}

static const GASNameFunction GAS_StringFunctionTable[] = 
{
    { "charAt",       &GASStringProto::StringCharAt       },
    { "charCodeAt",   &GASStringProto::StringCharCodeAt   },
    { "concat",       &GASStringProto::StringConcat       },
    { "indexOf",      &GASStringProto::StringIndexOf      },
    { "lastIndexOf",  &GASStringProto::StringLastIndexOf  },
    { "slice",        &GASStringProto::StringSlice        },
    { "split",        &GASStringProto::StringSplit        },
    { "substr",       &GASStringProto::StringSubstr       },
    { "substring",    &GASStringProto::StringSubstring    },
    { "toLowerCase",  &GASStringProto::StringToLowerCase  },
    { "toString",     &GASStringProto::StringToString     },
    { "toUpperCase",  &GASStringProto::StringToUpperCase  },
    { "valueOf",      &GASStringProto::StringValueOf      },
    { 0, 0 }
};

GASStringProto::GASStringProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASStringObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_StringFunctionTable, pprototype);
}

//////////////////
const GASNameFunction GASStringCtorFunction::StaticFunctionTable[] = 
{
    { "fromCharCode", &GASStringCtorFunction::StringFromCharCode },
    { 0, 0 }
};

GASStringCtorFunction::GASStringCtorFunction(GASStringContext *psc) :
    GASFunctionObject(GlobalCtor)
{
    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue(StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASStringCtorFunction::StringFromCharCode(const GASFnCall& fn)
{
    // Takes a variable number of args.  Each arg
    // is a numeric character code.  Construct The
    // string from the character codes.

    GFxString result;

    for (int i = 0; i < fn.NArgs; i++)
    {
        UInt32 c = (UInt32) fn.Arg(i).ToNumber(fn.Env);
        result.AppendChar(c);
    }
    fn.Result->SetString(fn.Env->CreateString(result));
}

GASObject* GASStringCtorFunction::CreateNewObject(GASStringContext *psc) const 
{ 
    GASObject* o = new GASStringObject(psc); 
    o->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_length), GASValue(GASValue::UNSET));
    return o;
}

void GASStringCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    if (fn.NArgs == 0)
    {
        fn.Result->SetString(fn.Env->GetBuiltin(GASBuiltin_empty_));
    }
    else
    {
        fn.Result->SetString(fn.Arg(0).ToString(fn.Env));
    }
}
